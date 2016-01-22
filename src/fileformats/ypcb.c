/*!
 * \file sff-template.c
 *
 * \brief template file for pluggable file formats.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996,1997,1998,2005,2006 Thomas Nau
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 *
 * \note How to compile (include paths can differ, depending on your system):
 *    gcc -I<pcbroot>/src -I<pcbroot> -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include -DHAVE_CONFIG_H -fPIC -shared fftemplate.c -o fftemplate.so
 *    - <pcbroot> is the location of *configured* PCB souce tree (config.h is available)
 *    - copy the .so file into one of plugin directories (e.g. ~/.pcb/plugins)
 *    - set executable flag
 */



#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "global.h"

#include <dirent.h>

#include <errno.h>

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include <string.h>

#include <time.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include <sys/stat.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#include <setjmp.h>

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <yaml.h>

#include "buffer.h"
#include "change.h"
#include "create.h"
#include "crosshair.h"
#include "data.h"
#include "edif_parse.h"
#include "error.h"
#include "file.h"
#include "hid.h"
#include "layerflags.h"
#include "misc.h"
#include "mymem.h"
#include "parse_l.h"
#include "pcb-printf.h"
#include "polygon.h"
#include "rats.h"
#include "remove.h"
#include "set.h"
#include "strflags.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

// Identifying name for this format (this ID must be unique accross formats)
#define FORMAT_ID "ypcb"

#define YPCB_FILE_VERSION_IMPLEMENTED 20160115

/*
*  Function to check file format
*  function should return 0, the file provided is of specified format, otherwise non-zero value.
*  This function is optional; if it is not provided, the system tries to load the file. If loading fails,
*  it is supposed that file format is not correct.
*/
int
CheckYPCBFile(char *filename)
{
  char *last_dot = rindex (filename, '.');
  return strcmp (last_dot, ".ypcb");
}

/*
*  Function to check supported version
*  function should return 0, if supports the provided data structure version (at least minimal one)
*  function should return non-zero value, if does not support the version of datya structures.
*/
int
CheckYPCBVersion(unsigned long current, unsigned long minimal)
{
    return (YPCB_FILE_VERSION_IMPLEMENTED >= minimal)?0:1;
}

// This type lets us dinstinguish between event initialization failures
// (e.g. yamle_stream_start_event_initialize() failure) and actual
// emissions failures (e.g. yaml_emitter_emit() failing).  Note that in
// the latter case more details are available in the .problem file of the
// yaml_emitter_t object.
typedef enum {
  EMIT_ERROR_NONE = 0,
  EMIT_ERROR_EVENT_INITIALIZE_FAILED,
  EMIT_ERROR_EMITTER_EMIT_FAILED,
  EMIT_ERROR_OTHER_ERROR
} emit_error_t;

int yaml_return_code;      // Hold yaml_* return codes (to keep macros hygenic)
yaml_emitter_t emitter;    // The YAML emitter object  
yaml_event_t event;        // The YAML event object
emit_error_t emit_error;   // The most recent emit_error_t (maybe NONE)
jmp_buf env;

// Try to emit YAML event name-EVENT with appropriate arguments __VA_ARGS__,
// and longjmp to location in env on error.
#define EMIT_EVENT_NEW(name, ...)                                    \
  do {                                                               \
    emit_error = EMIT_ERROR_NONE;                                    \
                                                                     \
    yaml_return_code                                                 \
      = yaml_ ## name ## _event_initialize (&event, ## __VA_ARGS__); \
                                                                     \
    if ( ! yaml_return_code ) {                                      \
      emit_error = EMIT_ERROR_EVENT_INITIALIZE_FAILED;               \
    }                                                                \
                                                                     \
    yaml_return_code = yaml_emitter_emit (&emitter, &event);         \
                                                                     \
    if ( ! yaml_return_code ) {                                      \
      emit_error = EMIT_ERROR_EMITTER_EMIT_FAILED;                   \
    }                                                                \
                                                                     \
    if ( emit_error != EMIT_ERROR_NONE ) {                           \
      longjmp (env, emit_error);                                     \
    }                                                                \
  } while ( 0 )

// Shorthand
#define EE EMIT_EVENT_NEW

// Shorthand for common events
#define E_MS(style) EE (mapping_start, NULL, NULL, true, style)
#define E_ME()      EE (mapping_end)
#define E_SS(style) EE (sequence_start, NULL, NULL, true, style)
#define E_SE()      EE (sequence_end)

// We define a construct as a YAML event serializing a particular type
// (e.g. string, integer via SCALAR-EVENT, or a sequence of YAML events
// that do so for more complicated types.  Each of these get their own
// function anway but we call them via this macro to clarify and enforce
// our function naming scheme.  The should be functions returning void that
// longjmp() on error (either direction or via emission macros or other
// construct functions).
#define EMIT_CONSTRUCT(construct, ...) emit_ ## construct (__VA_ARGS__)

// Shorthand
#define EC EMIT_CONSTRUCT

// Shorthand for common series and constructs

// Emit String
#define E_S(string_value)  EC (string_new, string_value)

// Emit Integer
#define E_I(integer_value) EC (integer_new, integer_value)

// Emit Named String (key and value, presumably of a mapping)
#define E_NS(name, value) \
  do {                    \
    E_S (name);           \
    E_S (value);          \
  } while ( 0 );

// Emit Named Integer (key and value of a mapping).  value is cast to int64_t.
#define E_NI(name, value) \
  do {                    \
    E_S (name);           \
    E_I (value);          \
  } while ( 0 );

// Emit Named String/Integer Element.  The name is used both in stringified
// form for the YAML element name, and as a field of CURRENT_STRUCT_POINTER
// (which clients should ensure is correctly #define'ed at the use point).
#define E_NSE(name) E_NS(#name, CURRENT_STRUCT_POINTER -> name)
#define E_NIE(name) E_NI(#name, CURRENT_STRUCT_POINTER -> name)

// Functions implementing construct emmission

static void
emit_string_new (char const *string)
{
  EMIT_EVENT_NEW (
      scalar,
      NULL,
      NULL,
      (yaml_char_t *) string,
      strlen (string),
      true,
      true,
      YAML_ANY_SCALAR_STYLE );
}

#define MAX_NUMBER_STRING_SIZE 42

static void
emit_integer_new (int64_t integer)
{
  char sr[MAX_NUMBER_STRING_SIZE + 1];   // String Representation
  int chars_printed
    = snprintf (sr, MAX_NUMBER_STRING_SIZE + 1, "%" PRIi64, integer);
 
  if ( chars_printed >= MAX_NUMBER_STRING_SIZE + 1 ) {
    assert (false);   // This really shouldn't happen
    longjmp (env, EMIT_ERROR_OTHER_ERROR);
  }

  E_S (sr);
}

static void
emit_entire_yaml_file (PCBType *pcb)
{
  EMIT_EVENT_NEW (stream_start, YAML_UTF8_ENCODING);
  // FIXME: perl round-tripping does not use implicit doc start I think
  EMIT_EVENT_NEW (document_start, NULL, NULL, NULL, true);

  // FIXME: verify that this undef/redef is sane when used at multiple point.
  // It should also be possible to re-order places that set it differently
  // without breaking either half, though I think if it works at all this
  // should be possible.
#undef  CURRENT_STRUCT_POINTER
#define CURRENT_STRUCT_POINTER pcb
  E_MS (YAML_BLOCK_MAPPING_STYLE);
  {
    E_NIE (MaxWidth);
    E_S ("foo");
    E_S ("bar");
    E_S ("baz");
    E_I (42);
  }
  E_ME ();

  EMIT_EVENT_NEW (document_end, true);
  EMIT_EVENT_NEW (stream_end);
}

static int
emit_stream_start (yaml_emitter_t *emitter, yaml_event_t *event)
{
  int return_code
    = yaml_stream_start_event_initialize (event, YAML_UTF8_ENCODING);

  if ( ! return_code ) {
    // Unlikely.  2 to be different from a yaml_emitter_emit() fail
    return 2; 
  }

  return ! yaml_emitter_emit (emitter, event);
}

static int
emit_stream_end (yaml_emitter_t *emitter, yaml_event_t *event)
{
  int return_code = yaml_stream_end_event_initialize (event);

  if ( ! return_code ) {
    // Unlikely.  2 to be different from a yaml_emitter_emit() fail
    return 2; 
  }

  return ! yaml_emitter_emit (emitter, event);
}

static int
emit_document_start (yaml_emitter_t *emitter, yaml_event_t *event)
{
  int return_code
    = yaml_document_start_event_initialize (
        event,
        NULL,
        NULL,
        NULL,
        true );

  if ( ! return_code ) {
    // Unlikely.  2 to be different from a yaml_emitter_emit() fail
    return 2; 
  }

  return ! yaml_emitter_emit (emitter, event);
}

static int
emit_document_end (yaml_emitter_t *emitter, yaml_event_t *event)
{
  // FIXME: do we want implicit end or not (second arg here)
  int return_code
    = yaml_document_end_event_initialize (event, true);

  if ( ! return_code ) {
    // Unlikely.  2 to be different from a yaml_emitter_emit() fail
    return 2; 
  }

  return ! yaml_emitter_emit (emitter, event);
}


static int
emit_string_common (
    yaml_emitter_t *emitter,
    yaml_event_t *event,
    char const *string,
    yaml_scalar_style_t style )
{
  int return_code
    = yaml_scalar_event_initialize (
        event,
        NULL,
        NULL,
        (yaml_char_t *) string,
        strlen (string),
        true,
        true,
        style );   // FIXME: probably want to set style

  if ( ! return_code ) {
    // Unlikely.  2 to be different from a yaml_emitter_emit() fail
    return 2; 
  }

  return ! yaml_emitter_emit (emitter, event);
}

static int
emit_string (yaml_emitter_t *emitter, yaml_event_t *event, char const *string)
{
  // FIXME: Probably want to set style to something consistant always
  return emit_string_common (emitter, event, string, YAML_ANY_SCALAR_STYLE);
}

static int
emit_single_quoted_string (
    yaml_emitter_t *emitter,
    yaml_event_t *event,
    char const *string )
{
  return
    emit_string_common (
        emitter, event, string, YAML_SINGLE_QUOTED_SCALAR_STYLE );
}

static int
emit_integer (yaml_emitter_t *emitter, yaml_event_t *event, int64_t integer)
{
  char sr[MAX_NUMBER_STRING_SIZE + 1];   // String Representation
  int chars_printed
    = snprintf (sr, MAX_NUMBER_STRING_SIZE + 1, "%" PRIi64, integer);
 
  if ( chars_printed >= MAX_NUMBER_STRING_SIZE + 1 ) {
    // This really shouldn't happen but just in case we give it it's own
    // integer value which isn't 1 or 2 (they are already used :)
    return 3;
  }

  return emit_string (emitter, event, sr);
}

static int
emit_double (
    yaml_emitter_t *emitter,
    yaml_event_t *event,
    double double_value )
{
  char sr[MAX_NUMBER_STRING_SIZE + 1];   // String Representation
  int chars_printed
    = snprintf (sr, MAX_NUMBER_STRING_SIZE + 1, "%f", double_value);

  if ( chars_printed >= MAX_NUMBER_STRING_SIZE + 1 ) {
    // This really shouldn't happen but just in case we give it it's own
    // integer value which isn't 1 or 2 (they are already used :)
    return 3;
  }

  return emit_string (emitter, event, sr);
}

static int
emit_sequence_start (
    yaml_emitter_t *emitter,
    yaml_event_t *event,
    yaml_sequence_style_t style )
{
  int return_code
    = yaml_sequence_start_event_initialize (
        event,
        NULL,
        NULL,
        true,
        style );

  if ( ! return_code ) {
    // Unlikely.  2 to be different from a yaml_emitter_emit() fail
    return 2; 
  }

  return ! yaml_emitter_emit (emitter, event);
}

static int
emit_sequence_end (yaml_emitter_t *emitter, yaml_event_t *event)
{
  int return_code = yaml_sequence_end_event_initialize (event);

  if ( ! return_code ) {
    // Unlikely.  2 to be different from a yaml_emitter_emit() fail
    return 2; 
  }

  return ! yaml_emitter_emit (emitter, event);
}

static int
emit_mapping_start (
    yaml_emitter_t *emitter,
    yaml_event_t *event,
    yaml_mapping_style_t style )
{
  int return_code
    = yaml_mapping_start_event_initialize (
        event,
        NULL,
        NULL,
        true,
        style );

  if ( ! return_code ) {
    // Unlikely.  2 to be different from a yaml_emitter_emit() fail
    return 2; 
  }

  return ! yaml_emitter_emit (emitter, event);
}

static int
emit_mapping_end (yaml_emitter_t *emitter, yaml_event_t *event)
{
  int return_code = yaml_mapping_end_event_initialize (event);

  if ( ! return_code ) {
    // Unlikely.  2 to be different from a yaml_emitter_emit() fail
    return 2; 
  }

  return ! yaml_emitter_emit (emitter, event);

}

static int
emit_pcb_flag_strings (
    yaml_emitter_t *emitter,
    yaml_event_t *event,
    FlagType flags )
{
  // Emit strings for the individual pcb flags in flags that don't get
  // specially ignored (see below).  Note that pcb flags are not the same
  // as per-object flags (which require a different conversion function to
  // get the string equivalents).

  // We call this function because it has some funny stuff that filters
  // out certain flags that get forced on at load anyway, and we want to do
  // exactly whatever insane stuff the existing pcb format does for now.
  // This result of pcbflags_to_string() must not be free()'ed.
  char *flags_string = pcbflags_to_string (flags);

  // pcbflags_to_string() double-quotes the result for us, we don't want that
  assert (flags_string[0] == '"');
  char *fsndq = strdup (flags_string + 1);   // Flags String No Double Quotes
  assert (fsndq[strlen (fsndq) - 1] == '"');
  fsndq[strlen (fsndq) - 1] = '\0';

  // Break the flags string up at ',' chars and emit the individual flags
  gint const max_flag_count = 424242;   // Arbitrary large value
  char **flag_strings = g_strsplit (fsndq, ",", max_flag_count);
  free (fsndq);
  char **cfsp;   // Current Flag String Pointer
  int return_code = 0;
  for ( cfsp = flag_strings ; *cfsp != NULL ; cfsp++ ) {
    return_code = emit_string (emitter, event, *cfsp);
    if ( return_code != 0 ) {
      break;
    }
  }
  g_strfreev (flag_strings);

  return return_code;
}

static int
emit_style_mappings (
    yaml_emitter_t *emitter,
    yaml_event_t *event,
    Cardinal style_count,
    RouteStyleType *styles )
{
  int return_code = 0;

  Cardinal csi;   // Current Style Index
   
  // FIXME: it's clunky not to have macrose that we can use for emissions
  // when we want to return.  Or we could use set/longjump.  Or emit with
  // macro and an unclea index variable.

  return_code = emit_mapping_start (emitter, event, YAML_BLOCK_MAPPING_STYLE);
  if ( return_code != 0 ) {
    return return_code;
  }
  {
    for ( csi = 0 ; csi < style_count ; csi++ ) {

      return_code = emit_string (emitter, event, styles[csi].Name);
      if ( return_code != 0 ) { return return_code; }

      return_code
        = emit_mapping_start (emitter, event, YAML_BLOCK_MAPPING_STYLE);
      if ( return_code != 0 ) {
        return return_code;
      }
      {
        return_code = emit_string (emitter, event, "Thick");
        if ( return_code != 0 ) { return return_code; }
        return_code = emit_integer (emitter, event, styles[csi].Thick);
        if ( return_code != 0 ) { return return_code; }

        return_code = emit_string (emitter, event, "Diameter");
        if ( return_code != 0 ) { return return_code; }
        return_code = emit_integer (emitter, event, styles[csi].Diameter);
        if ( return_code != 0 ) { return return_code; }

        return_code = emit_string (emitter, event, "Hole");
        if ( return_code != 0 ) { return return_code; }
        return_code = emit_integer (emitter, event, styles[csi].Hole);
        if ( return_code != 0 ) { return return_code; }

        return_code = emit_string (emitter, event, "Keepaway");
        if ( return_code != 0 ) { return return_code; }
        return_code = emit_integer (emitter, event, styles[csi].Keepaway);
        if ( return_code != 0 ) { return return_code; }
      }
      return_code = emit_mapping_end (emitter, event);
      if ( return_code != 0 ) { return return_code; }
    }
  }
  return_code = emit_mapping_end (emitter, event);
  if ( return_code != 0 ) { return return_code; }

  return return_code;
}

// These macros all require context, and Return On Fail the return_code.

// Emit Mapping Start
#define EMSROF(style)                                         \
  do {                                                        \
    return_code = emit_mapping_start (emitter, event, style); \
    if ( return_code != 0 ) { return return_code; }           \
  } while ( 0 )
// Emit Mapping End
#define EMEROF()                                     \
  do {                                               \
    return_code = emit_mapping_end (emitter, event); \
    if ( return_code != 0 ) { return return_code; }  \
  } while ( 0 )
// Emit Sequence Start
#define ESSROF(style)                                          \
  do {                                                         \
    return_code = emit_sequence_start (emitter, event, style); \
    if ( return_code != 0 ) { return return_code; }            \
  } while ( 0 )
// Emit Sequence End
#define ESEROF()                                      \
  do {                                                \
    return_code = emit_sequence_end (emitter, event); \
    if ( return_code != 0 ) { return return_code; }   \
  } while ( 0 )
// Emit String
#define ESROF(string)                                   \
  do {                                                  \
    return_code = emit_string (emitter, event, string); \
    if ( return_code != 0 ) { return return_code; }     \
  } while ( 0 )
// Emit Single-Quoted String
#define ESQSROF(string)                                               \
  do {                                                                \
    return_code = emit_single_quoted_string (emitter, event, string); \
    if ( return_code != 0 ) { return return_code; }                   \
  } while ( 0 )
// Emit Integer
#define EIROF(integer)                                    \
  do {                                                    \
    return_code = emit_integer (emitter, event, integer); \
    if ( return_code != 0 ) { return return_code; }       \
  } while ( 0 )

static int
emit_font (
    yaml_emitter_t *emitter,
    yaml_event_t *event,
    Cardinal max_symbol_count,
    FontType *font )
{
  int return_code = 0;
  Cardinal ii;

  EMSROF (YAML_BLOCK_MAPPING_STYLE);
  {
    for ( ii = 0 ; ii <= max_symbol_count ; ii++ ) {
      // The key is the character (possibly non-printing, in which case integer
      // value is used), and the value is a hash describing how it's rendered.
      if ( ! font->Symbol[ii].Valid ) {
        continue;
      }
      if ( isprint (ii) ) {
        char as_string[2] = "";
        as_string[0] = ii;
        as_string[1] = '\0';
        ESQSROF (as_string);
      }
      else {
        EIROF (ii);
      }

      EMSROF (YAML_BLOCK_MAPPING_STYLE);
      {
        ESROF ("Lines");
        ESSROF (YAML_FLOW_SEQUENCE_STYLE);
        {
          int jj = font->Symbol[ii].LineN;
          LineType *line = font->Symbol[ii].Line;
          for ( jj = 0 ; jj < font->Symbol[ii].LineN ; jj++, line++ ) {
            EMSROF (YAML_FLOW_MAPPING_STYLE);
            {
              ESROF ("Point1");
              ESSROF (YAML_FLOW_SEQUENCE_STYLE);
              {
                EIROF (line->Point1.X);
                EIROF (line->Point1.Y);
              }
              ESEROF ();
              ESROF ("Point2");
              ESSROF (YAML_FLOW_SEQUENCE_STYLE);
              {
                EIROF (line->Point2.X);
                EIROF (line->Point2.Y);
              }
              ESEROF ();
              ESROF ("Thickness");
              EIROF (line->Thickness);
            }
            EMEROF ();
          }
        }
        ESEROF ();
      }
      EMEROF ();
    }
  }
  EMEROF ();

  return return_code;
}

// FIXME: this comma-swallowing __VA_ARGS__ needs a GCC extension, could
// make a EMIT_EVENT_WITH_ARG() macro if people care and we actually want
// style args.s

// Call emit_thing_to_emit (a constructed function name), and go to the
// emission failure handler if emission fails.
#define EMIT(thing_to_emit, ...)                                      \
  do {                                                                \
    if ( emit_ ## thing_to_emit (&emitter, &event, ##__VA_ARGS__) ) { \
      goto handle_yaml_emission_error;                                \
    }                                                                 \
  } while ( 0 )

// By convention we use this when emitting a single YAML event (as opposed
// to a construct).
#define EMIT_EVENT EMIT

// FIXME: these wrappers should maybe get shorter like ESS ESE etc., at
// least for the ones that get used a lot

// These emit individual YAML events.
#define EMIT_STREAM_START()        EMIT_EVENT (stream_start)
#define EMIT_STREAM_END()          EMIT_EVENT (stream_end)
#define EMIT_DOCUMENT_START()      EMIT_EVENT (document_start)
#define EMIT_DOCUMENT_END()        EMIT_EVENT (document_end)
#define EMIT_STRING(arg)           EMIT_EVENT (string, arg)
#define EMIT_INTEGER(arg)          EMIT_EVENT (integer, arg)
#define EMIT_DOUBLE(double_value)  EMIT_EVENT (double, double_value)
#define EMIT_SEQUENCE_START(style) EMIT_EVENT (sequence_start, style)
#define EMIT_SEQUENCE_END()        EMIT_EVENT (sequence_end)
#define EMIT_MAPPING_START(style)  EMIT_EVENT (mapping_start, style)
#define EMIT_MAPPING_END()         EMIT_EVENT (mapping_end)

// These emit constructs of multiple YAML events.
#define EMIT_PCB_FLAG_STRINGS(pcb_flags) \
  EMIT (pcb_flag_strings, pcb_flags)
#define EMIT_STYLE_MAPPINGS(style_count, styles) \
  EMIT (style_mappings, style_count, styles)
#define EMIT_FONT(max_symbol_count, font_pointer) \
  EMIT (font, max_symbol_count, font_pointer)

// Emit a pair of integers as a sequence  // FIXME: we might not use this
#define EMIT_INTEGER_PAIR_SEQUENCE(int1, int2)      \
  do {                                              \
    EMIT_SEQUENCE_START (YAML_FLOW_SEQUENCE_STYLE); \
    EMIT_INTEGER (int1);                            \
    EMIT_INTEGER (int2);                            \
    EMIT_SEQUENCE_END ();                           \
  } while ( 0 )

// Emit labeled coordinate pair as a mapping  // FIXME: we might not use this 
#define EMIT_COORDINATE_PAIR_MAPPING(x, y)        \
  do {                                            \
    EMIT_MAPPING_START (YAML_FLOW_MAPPING_STYLE); \
    EMIT_STRING ("x");                            \
    EMIT_INTEGER (x);                             \
    EMIT_STRING ("y");                            \
    EMIT_INTEGER (y);                             \
    EMIT_MAPPING_END ();                          \
  } while ( 0 )

#define EMIT_NAMED_STRING(name, string) \
  do {                                  \
    EMIT_STRING (name);                 \
    EMIT_STRING (string);               \
  } while ( 0 )

#define EMIT_NAMED_STRING_MAPPING(name, string)   \
  do {                                            \
    EMIT_MAPPING_START (YAML_FLOW_MAPPING_STYLE); \
    EMIT_STRING (name);                           \
    EMIT_STRING (string);                         \
    EMIT_MAPPING_END ();                          \
  } while ( 0 )

#define EMIT_NAMED_INTEGER(name, integer)         \
  do {                                            \
    EMIT_STRING (name);                           \
    EMIT_INTEGER (integer);                       \
  } while ( 0 )

#define EMIT_NAMED_DOUBLE(name, double_value) \
  do {                                        \
    EMIT_STRING (name);                       \
    EMIT_DOUBLE (double_value);               \
  } while ( 0 )

// To save typing: short aliases with stringification
#define ENS(name, string)       EMIT_NAMED_STRING (#name, string)
#define ENI(name, integer)      EMIT_NAMED_INTEGER (#name, integer)
#define END(name, double_value) EMIT_NAMED_DOUBLE (#name, double_value)

// Even easier and less redundant for emmiting things on level 1 from PCB
// that use the same names as are used in the structure.
#define ENS1(name) ENS (name, PCB->name)
#define ENI1(name) ENI (name, PCB->name)
#define END1(name) END (name, PCB->name)

int
output_pcb_yaml (PCBType *pcb, FILE *output_file)
{
  // Output YAML form of pcb into already open output_file.  Return 0 on
  // success, non-zero otherwise.
 
  int setjmp_result;

  // Create the emitter object
  if ( ! yaml_emitter_initialize (&emitter) ) {
    return 1;
  }
  
  yaml_emitter_set_output_file (&emitter, output_file);

  setjmp_result = setjmp (env);
  if ( setjmp_result == 0 ) {
    emit_entire_yaml_file (pcb);
  }
  else {
    switch ( setjmp_result ) {
      case EMIT_ERROR_EVENT_INITIALIZE_FAILED:
        fprintf (stderr, "YAML event initialization failed\n");
        yaml_emitter_delete (&emitter);
        return 1;
        break;
      case EMIT_ERROR_EMITTER_EMIT_FAILED:
        fprintf (stderr, "YAML emission failed: %s\n",  emitter.problem);
        yaml_emitter_delete (&emitter);
        return 1;
        break;
      default:
        assert (false);   // Shouldn't be here
        break;
    }
  }
        
  yaml_emitter_delete (&emitter);
  return 0;

  int return_code;
  yaml_emitter_t emitter;
  yaml_event_t event;

  /* Create the Emitter object.  */
  return_code = yaml_emitter_initialize (&emitter);
  if ( ! return_code ) {
    return 1;
  }

  yaml_emitter_set_output_file (&emitter, output_file);

  EMIT_STREAM_START ();
  EMIT_DOCUMENT_START ();

  // Whole format is a mapping
  EMIT_MAPPING_START (YAML_BLOCK_MAPPING_STYLE);

  // FIXME: remove some of the below whole-doc delimiting whitespace and
  // maybe block the whole thing for clarity




  // Instead of the top-line comment in the original pcb format
  ENS (Program, Progname);
  // This doesn't have an analog in the original pcb format
  ENS (Fileformat, FORMAT_ID);
  // Instead of the top-line comment in the original pcb format
  ENI (YPCB_FILE_VERSION_IMPLEMENTED, YPCB_FILE_VERSION_IMPLEMENTED);

  // FIXME: bad name, but we're mostly following old pcb format for now.
  // The function that gets this value is badly named also.  Should be
  // pcb_version_required assuming the explanatory commentis correct.
  ENI (FileVersion, PCBFileVersionNeeded ());

  EMIT_STRING ("PCB");
  EMIT_MAPPING_START (YAML_BLOCK_MAPPING_STYLE);
  {
    ENS (Name, EMPTY (PCB->Name));
    // FIXME: we actually need to do number output as string and use the
    // already-made pcb_fprintf() or one of its siblings, its worth having
    // string in the scripting languages if it gets you units, provided the
    // unit extensions work in some sane way...
    ENI1 (MaxWidth);
    ENI1 (MaxHeight);
  }
  EMIT_MAPPING_END ();

  EMIT_STRING ("Grid");
  EMIT_MAPPING_START (YAML_BLOCK_MAPPING_STYLE);
  {
    // FIXME: Struct member same name as the name of this block.  How ugly
    // that turns out to be.  No great way to fix either without datastructure
    // rename or abandoning the name convention we're trying for
    ENI1 (Grid);
    ENI1 (GridOffsetX);
    ENI1 (GridOffsetY);
    // FIXME: its bad that we need stuff from Settings here
    ENS (DrawGrid, Settings.DrawGrid ? "true" : "false");
  }
  EMIT_MAPPING_END ();

  // FIXME: whyyyyy do we have these weird conversions here
  END (PolyArea, COORD_TO_MIL (COORD_TO_MIL (PCB->IsleArea) * 100) * 100);

  END (ThermScale, PCB->ThermScale);

  EMIT_STRING ("DRC");
  EMIT_MAPPING_START (YAML_BLOCK_MAPPING_STYLE);
  {
    ENI1 (Bloat);
    ENI1 (Shrink);
    ENI1 (minWid);
    ENI1 (minSlk);
    ENI1 (minDrill);
    ENI1 (minRing);
  }
  EMIT_MAPPING_END ();

  EMIT_STRING ("Flags");
  EMIT_SEQUENCE_START (YAML_FLOW_SEQUENCE_STYLE);
  {
    EMIT_PCB_FLAG_STRINGS (PCB->Flags);
  }
  EMIT_SEQUENCE_END ();

  // FIXME: these should be broken down, not left as a complex string
  ENS (Groups, LayerGroupsToString (&(PCB->LayerGroups)));
  
  EMIT_STRING ("Styles");
  EMIT_STYLE_MAPPINGS (NUM_STYLES, PCB->RouteStyle);

  EMIT_STRING ("Font");
  EMIT_FONT (MAX_FONTPOSITION, &(PCB->Font));

  


  // FIXME: remove some of the above whole-doc delimiting whitespace and
  // maybe block the whole thing for clarity

  // End of mapping corresponding to whole data file
  EMIT_MAPPING_END ();

  EMIT_DOCUMENT_END ();
  EMIT_STREAM_END ();

  /* Destroy the Emitter object. */
  yaml_emitter_delete (&emitter);

  return 0;

  handle_yaml_emission_error:
  fprintf (stderr, "YAML emission failed: %s\n",  emitter.problem);
  yaml_emitter_delete (&emitter);
  return 1;
}

/*
*  Function to save layout to specified file
*  Functions saves contens of PCBtype structure to specified file
*  Returns 0, if succeeded, other value indicates error.
*  No user interactions are necessary.
*/
int
SaveYPCB(PCBType *pcb, char *filename)
{
  int return_code;
  FILE *filename_fp;
  int yaml_emission_error = 0;
  int result = 0;
  
  /* Open output file.  */
  filename_fp = fopen (filename, "w");
  if ( filename_fp == NULL ) {
    // FIXME: remove this fprintf someday in favor of any real error handling
    // that should happen at this point (probably nothing).
    fprintf (
        stderr,
        "%s: %i: %s: fopen() failed to open %s for writing: %s",
        __FILE__,
        __LINE__,
        __func__,
        filename,
        strerror (errno) );
    return 1;
  }

  yaml_emission_error = output_pcb_yaml (pcb, filename_fp);  

  /* On error.  */
  if ( yaml_emission_error ) {
    result = 1;
  }
  
  return_code = fclose (filename_fp);
  if ( return_code != 0 ) {
    fprintf (
        stderr,
        "%s: %i: %s: fclose() failed: %s",
        __FILE__,
        __LINE__,
        __func__,
        strerror (errno) );
    result = 1;
  }
  
  return result;
}


/*
*  Function to load layout from specified file
*  Functions fills provided PCBtype structure with data
*  Returns 0, if succeeded, other value indicates error.
*  No user interactions are necessary.
*/
static int
ParseYPCB(PCBType *pcb, char *filename)
{
  /* Follofing function call is placeholder, which creates empty board */
  
  CreateNewPCBPost (pcb, 1); /* REMOVE ME!!!" */
  
  return 0;
}

/*
*  Array of file format registration structures:
*    - unique identifiction string
*    - name shown in Open/Save dialogs
*    - pointer to NULL-terminated list of file patterns
*    - mime-type
*    - file format is default format (1)
*    - function to check format version
*    - function to check if file is of acceptable format
*    - function to load layout
*    - function to save layout
*    - function to load element(s) - not implemented yet
*    - function to save element(s) - not implemented yet
*/

static char *ypcb_format_list_patterns[]={"*.ypcb", "*.YPCB", 0};

static HID_Format ypcb_format_list[]={
  {FORMAT_ID,"YAML PCB", ypcb_format_list_patterns, "application/x-pcb-layout", 0, (void *)CheckYPCBVersion, (void *)CheckYPCBFile, (void *)ParseYPCB, (void *)SaveYPCB, 0, 0}
};

REGISTER_FORMATS (ypcb_format_list)

/*
* Plugin registration. Rename it to hid_<basename>_init
*/

void
hid_ypcb_format_init()
{
  register_ypcb_format_list();
}
