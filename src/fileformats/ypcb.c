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
jmp_buf env;

// Try to emit YAML event name-EVENT with appropriate arguments __VA_ARGS__,
// and longjmp to location in env on error.
#define EMIT_EVENT(name, ...)                                        \
  do {                                                               \
    yaml_return_code                                                 \
      = yaml_ ## name ## _event_initialize (&event, ## __VA_ARGS__); \
                                                                     \
    if ( ! yaml_return_code ) {                                      \
      longjmp (env, EMIT_ERROR_EVENT_INITIALIZE_FAILED);             \
    }                                                                \
                                                                     \
    yaml_return_code = yaml_emitter_emit (&emitter, &event);         \
                                                                     \
    if ( ! yaml_return_code ) {                                      \
      longjmp (env, EMIT_ERROR_EMITTER_EMIT_FAILED);                 \
    }                                                                \
  } while ( 0 )

// Shorthand
#define EE EMIT_EVENT

// Shorthand
#define FSS YAML_FLOW_SEQUENCE_STYLE
#define BSS YAML_BLOCK_SEQUENCE_STYLE
#define FMS YAML_FLOW_MAPPING_STYLE
#define BMS YAML_BLOCK_MAPPING_STYLE

// If USE_MORE_FLOW_STYLE is defined, we use flow style instead of block
// style in places where either might be reasonable, and vice versa.
#define USE_MORE_FLOW_STYLE

// Indeterminate style macros to implement USE_MORE_FLOW_STYLE (or not if
// it's not defined).
#ifdef USE_MORE_FLOW_STYLE
#  define ISS FSS
#  define IMS FMS
#else
#  define ISS BSS
#  define IMS BMS
#endif

// Shorthand for common events
#define E_MS(style) EE (mapping_start, NULL, NULL, true, style)
#define E_ME()      EE (mapping_end)
#define E_SS(style) EE (sequence_start, NULL, NULL, true, style)
#define E_SE()      EE (sequence_end)

// We define a construct as a YAML event serializing a particular type
// (e.g. string, integer via SCALAR-EVENT, or a sequence of YAML events
// that do so for more complicated types.  Each of these get their own
// function anway but we call them via this macro to clarify and enforce
// our function naming scheme.  These should be functions returning void
// that longjmp() on error (either directly or via emission macros or other
// construct functions).
#define EMIT_CONSTRUCT(construct, ...) emit_ ## construct (__VA_ARGS__)

// Shorthand
#define EC EMIT_CONSTRUCT

// Shorthand for commonly emitted things

// Emit String
#define E_S(string_value)  EC (string, string_value)

// Emit Integer
#define E_I(integer_value) EC (integer, integer_value)

// Emit Double
#define E_D(double_value)  EC (double, double_value)

// Emit Point (expressed as a sequence)
#define E_P(point_value)             \
  do {                               \
    E_SS (FSS);                      \
    {                                \
      E_I ((point_value).X);         \
      E_I ((point_value).Y);         \
    }                                \
    E_SE ();                         \
  } while ( 0 )

// Emit Named String/Int/Double/Point (key and value, presumably of a mapping)
#define E_NS(name, value) do { E_S (#name); E_S (value); } while ( 0 )
#define E_NI(name, value) do { E_S (#name); E_I (value); } while ( 0 )
#define E_ND(name, value) do { E_S (#name); E_D (value); } while ( 0 )
#define E_NP(name, value) do { E_S (#name); E_P (value); } while ( 0 )

// Emit Named String/Integer Element.  This is shorthand to avoid repeating
// the element name as both the name and the associated structure field.
// The name is used both in stringified form for the YAML element name,
// and as a field of *CURRENT_STRUCT_POINTER (which clients should ensure
// is correctly #define'ed at the use point).
#define E_NSE(name) E_NS (name, CURRENT_STRUCT_POINTER -> name)
#define E_NIE(name) E_NI (name, CURRENT_STRUCT_POINTER -> name)
#define E_NDE(name) E_ND (name, CURRENT_STRUCT_POINTER -> name)
#define E_NPE(name) E_NP (name, CURRENT_STRUCT_POINTER -> name)


// These functions take more arguments than their corresponding non-_full
// counterparts and return error codes.  They're mainly for use inside
// construct implementation in those cases where we have some cleanup to
// do after a failure, but sometimes also when a little more control over
// emitter behavior is desirable (FIXME: if it ever turns out to be).

static emit_error_t
emit_string_full (char const *string, yaml_scalar_style_t style)
{
  int return_code
    = yaml_scalar_event_initialize (
        &event,
        NULL,
        NULL,
        (yaml_char_t *) string,
        strlen (string),
        true,
        true,
        style );

  if ( ! return_code ) {
    return EMIT_ERROR_EVENT_INITIALIZE_FAILED; 
  }

  if ( ! yaml_emitter_emit (&emitter, &event) ) {
    return EMIT_ERROR_EMITTER_EMIT_FAILED;
  }

  return EMIT_ERROR_NONE;
}


// Functions implementing construct emmission

static void
emit_string (char const *string)
{
  EMIT_EVENT (
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
emit_integer (int64_t value)
{
  char sr[MAX_NUMBER_STRING_SIZE + 1];   // String Representation
  int chars_printed
    = snprintf (sr, MAX_NUMBER_STRING_SIZE + 1, "%" PRIi64, value);
 
  if ( chars_printed >= MAX_NUMBER_STRING_SIZE + 1 ) {
    assert (false);   // This really shouldn't happen
    longjmp (env, EMIT_ERROR_OTHER_ERROR);
  }

  E_S (sr);
}

static void
emit_double (double value)
{
  char sr[MAX_NUMBER_STRING_SIZE + 1];   // String Representation
  int chars_printed
    = snprintf (sr, MAX_NUMBER_STRING_SIZE + 1, "%f", value);

  if ( chars_printed >= MAX_NUMBER_STRING_SIZE + 1 ) {
    assert (false);   // This really shouldn't happen
    longjmp (env, EMIT_ERROR_OTHER_ERROR);
  }

  E_S (sr);
}

static void
emit_pcb_flags (FlagType flags)
{
  // Emit the pcb flags named block, including strings for the individual pcb
  // flags in flags that don't get specially ignored (see below).  Note that
  // pcb flags are not the same as per-object flags (which require a different
  // conversion function to get the string equivalents).

  E_S ("Flags");
  E_SS (FSS);
  {
    // We call this function because it has some funny stuff that filters
    // out certain flags that get forced on at load anyway, and we want to
    // do exactly whatever insane stuff the existing pcb format does for now.
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
    emit_error_t emit_error = EMIT_ERROR_NONE;
    for ( cfsp = flag_strings ; *cfsp != NULL ; cfsp++ ) {
      emit_error = emit_string_full (*cfsp, YAML_PLAIN_SCALAR_STYLE);
      if ( emit_error != EMIT_ERROR_NONE ) {
        break;
      }
    }
    g_strfreev (flag_strings);
  
    if ( emit_error != EMIT_ERROR_NONE ) {
      longjmp (env, emit_error);
    }
  }
  E_SE ();
}

static void
emit_object_flags (FlagType flags, int object_type)
{
  // Emit the object flags as a (name, sequence-of-strings) pair.  NOTE:
  // this function ignores the thermal strings which the original pcb
  // format (weirdly) stores in the flags string for VIA_TYPE objects.
  // The emit_thermal_details() function must be used as well for VIA_TYPE
  // objects to generate the representation of these.
    
  E_S ("Flags"); 
  E_SS (FSS);
  {
    // We call this function because it has some funny stuff that filters
    // out certain flags that get forced on at load anyway, and we want to
    // do exactly whatever insane stuff the existing pcb format does for now.
    // This result of pcbflags_to_string() must not be free()'ed.
    char *flags_string = flags_to_string (flags, object_type);

    // flags_to_string() double-quotes the result for us, we don't want that
    assert (flags_string[0] == '"');
    char *fsndq = strdup (flags_string + 1);   // Flags String No Double Quotes
    assert (fsndq[strlen (fsndq) - 1] == '"');
    fsndq[strlen (fsndq) - 1] = '\0';
  
    // Break the flags string up at ',' chars, filter out any thermal
    // specifications, and emit the individual flags
    gint const max_flag_count = 424242;   // Arbitrary large value
    char **flag_strings = g_strsplit (fsndq, ",", max_flag_count);
    free (fsndq);
    char **cfsp;   // Current Flag String Pointer
    emit_error_t emit_error = EMIT_ERROR_NONE;
    for ( cfsp = flag_strings ; *cfsp != NULL ; cfsp++ ) {
      if ( strncmp (*cfsp, "thermal(", strlen ("thermal(")) == 0 ) {
        continue;   // Skip thermal specs, because they aren't really flags
      }
      emit_error = emit_string_full (*cfsp, YAML_PLAIN_SCALAR_STYLE);
      if ( emit_error != EMIT_ERROR_NONE ) {
        break;
      }
    }
    g_strfreev (flag_strings);
  }
  E_SE ();
}

static void
emit_styles (Cardinal style_count, RouteStyleType *styles)
{
  // Emit the given styles block.  Note that we have both pcb styles and
  // YAML styles going on in the function, and they are totally different.
  
  E_S ("Styles");
  E_MS (BMS);
  {
    Cardinal csi;   // Current Style Index
    for ( csi = 0 ; csi < style_count ; csi++ ) {
      E_S (styles[csi].Name);
      E_MS (IMS);
      {
#undef  CURRENT_STRUCT_POINTER
#define CURRENT_STRUCT_POINTER (&(styles[csi]))
        E_NIE (Thick);
        E_NIE (Diameter);
        E_NIE (Hole);
        E_NIE (Keepaway);
      }
      E_ME ();
    }
  }
  E_ME ();
}

static void
emit_font (Cardinal max_font_position, FontType *font)
{
  Cardinal ii;

  E_S ("Font");
  E_MS (BMS);
  {
    for ( ii = 0 ; ii <= max_font_position ; ii++ ) {
      // The key is the character (possibly non-printing, in which case integer
      // value is used), and the value is a hash describing how it's rendered.
      if ( ! font->Symbol[ii].Valid ) {
        continue;
      }
      if ( isprint (ii) ) {
        char as_string[2] = "";
        as_string[0] = ii;
        as_string[1] = '\0';
        // Call the full function here to force single quotes on the file name
        emit_error_t emit_error
          = emit_string_full (as_string, YAML_SINGLE_QUOTED_SCALAR_STYLE);
        if ( emit_error != EMIT_ERROR_NONE ) {
          longjmp (env, emit_error);
        }
      }
      else {
        E_I (ii);
      }

      E_MS (BMS);
      {
        E_S ("Lines");
        E_SS (BSS);
        {
          int jj = font->Symbol[ii].LineN;
          LineType *line = font->Symbol[ii].Line;
          for ( jj = 0 ; jj < font->Symbol[ii].LineN ; jj++, line++ ) {
            E_MS (IMS);
            {
#undef  CURRENT_STRUCT_POINTER
#define CURRENT_STRUCT_POINTER line
              E_NPE (Point1);
              E_NPE (Point2);
              E_NIE (Thickness);
            }
            E_ME ();
          }
        }
        E_SE ();
      }
      E_ME ();
    }
  }
  E_ME ();
}

static void
emit_attributes (AttributeListType *attribute_list)
{
  // AttributeListType is really a list of name-value pairs, i.e. a hash,
  // so we emit it that way.

  E_S ("Attributes");
  E_MS (BMS);
  {
    for ( int ii = 0 ; ii < attribute_list->Number ; ii++ ) {
      E_S (attribute_list->List[ii].name);
      E_S (attribute_list->List[ii].value);
    }
  }
  E_ME ();
}

static void
emit_vias (GList *via_list)
{
  E_S ("Vias");
  E_SS (BSS);
  {
    GList *iter;
    for ( iter = via_list ; iter != NULL ; iter = g_list_next (iter) ) {
      PinType *via = iter->data;
      E_MS (IMS);
      {
#undef  CURRENT_STRUCT_POINTER
#define CURRENT_STRUCT_POINTER via
        E_NIE (X);
        E_NIE (Y);
        E_NIE (Thickness);
        E_NIE (Clearance);
        E_NIE (Mask);
        E_NIE (DrillingHole);
        E_NS (Name, EMPTY (CURRENT_STRUCT_POINTER -> Name));
        // FIXME: the mess of flags and thermals that live together in this
        // string should be disentanged into decent structures
        E_NS (Flags, flags_to_string (via->Flags, VIA_TYPE));
      }
      E_ME ();
    }
  }
  E_SE ();
}

static void
emit_pins (GList *pin_list, Coord MarkX, Coord MarkY)
{
  // Emit the pin list part of an element.  MarkX and MarkY are required in
  // order to make some on-the-fly calculations of offsets relative to the
  // element mark.

  E_S ("Pins");
  E_SS (BSS);
  {
    GList *iter;
    for ( iter = pin_list ; iter != NULL ; iter = g_list_next (iter) ) {
      E_MS (IMS);
      {
        PinType *pin = iter->data;
        // Note: these names don't exist in the original pcb format (the
        // values are computed on-the-fly), so we made up these names.
        E_NI (OffsetX, pin->X - MarkX);
        E_NI (OffsetY, pin->Y - MarkY);
        E_NI (Thickness, pin->Thickness);
        E_NI (Clearance, pin->Clearance);
        E_NI (Mask, pin->Mask);
        E_NI (DrillingHole, pin->DrillingHole);
        E_NS (Name, EMPTY (pin->Name));
        E_NS (Number, EMPTY (pin->Number));
        E_NS (Flags, flags_to_string (pin->Flags, PIN_TYPE));
      }
      E_ME ();
    }
  }
  E_SE ();
}

static void
emit_pads (GList *pad_list, Coord MarkX, Coord MarkY)
{
  // Emit the pad list part of an element.  MarkX and MarkY are required in
  // order to make some on-the-fly calculations of offsets relative to the
  // element mark.

  E_S ("Pads");
  E_SS (BSS);
  {
    GList *iter;
    for ( iter = pad_list ; iter != NULL ; iter = g_list_next (iter) ) {
      E_MS (IMS);
      {
        PadType *pad = iter->data;
        // Note: these names don't exist in the original pcb format (the
        // values are computed on-the-fly), so we made up these names.
        E_NI (Point1OffsetX, pad->Point1.X - MarkX);
        E_NI (Point1OffsetY, pad->Point1.Y - MarkY);
        E_NI (Point2OffsetX, pad->Point2.X - MarkX);
        E_NI (Point2OffsetY, pad->Point2.Y - MarkY);
        E_NI (Clearance, pad->Clearance);
        E_NI (Mask, pad->Mask);
        E_NS (Name, EMPTY (pad->Name));
        E_NS (Number, EMPTY (pad->Number));
        EC (object_flags, pad->Flags, PAD_TYPE);
      }
      E_ME ();
    }
  }
  E_SE ();
}

static void
emit_element_lines (GList *line_list, Coord MarkX, Coord MarkY)
{
  // Emit the line list part of an element.  MarkX and MarkY are required
  // in order to make some on-the-fly calculations of offsets relative to
  // the element mark.

  E_S ("Lines");
  E_SS (BSS);
  {
    GList *iter;
    for ( iter = line_list ; iter != NULL ; iter = g_list_next (iter) ) {
      E_MS (IMS);
      {
        LineType *line = iter->data;
        // Note: these names don't exist in the original pcb format (the
        // values are computed on-the-fly), so we made up these names.
        E_NI (Point1OffsetX, line->Point1.X - MarkX);
        E_NI (Point1OffsetY, line->Point1.Y - MarkY);
        E_NI (Point2OffsetX, line->Point2.X - MarkX);
        E_NI (Point2OffsetY, line->Point2.Y - MarkY);
        E_NI (Thickness, line->Thickness);
      }
      E_ME ();
    }
  }
  E_SE ();
}

static void
emit_element_arcs (GList *arc_list, Coord MarkX, Coord MarkY)
{
  // Emit the arc list part of an element.  MarkX and MarkY are required
  // in order to make some on-the-fly calculations of offsets relative to
  // the element mark.

  E_S ("Arcs");
  E_SS (BSS);
  {
    GList *iter;
    for ( iter = arc_list ; iter != NULL ; iter = g_list_next (iter) ) {
      E_MS (IMS);
      {
        ArcType *arc = iter->data;
        // Note: these names don't exist in the original pcb format (the
        // values are computed on-the-fly), so we made up these names.
        E_NI (CenterOffsetX, arc->X - MarkX);
        E_NI (CenterOffsetY, arc->Y - MarkY);
        E_NI (Width,         arc->Width);
        E_NI (Height,        arc->Height);
        E_ND (StartAngle,    arc->StartAngle);
        E_ND (Delta,         arc->Delta);
        E_NI (Thickness,     arc->Thickness);
      }
      E_ME ();
    }
  }
  E_SE ();
}

static void
emit_elements (GList *element_list)
{
  // Emit the elements as a (name, sequence-of-maps) pair.

  E_S ("Elements");
  E_SS (BSS);
  {
    GList *iter;
    for ( iter = element_list ; iter != NULL ; iter = g_list_next (iter) ) {
      ElementType *elm = iter->data;   // Element

      // Skip empty elements
      if ( !elm->LineN && !elm->PinN && !elm->ArcN && !elm->PadN ) {
        continue;
      }

      E_MS (BMS);
      {
        EC (object_flags, elm->Flags, ELEMENT_TYPE);
        E_NS (Description, EMPTY (DESCRIPTION_NAME (elm)));
        E_NS (NameOnPcb, EMPTY (NAMEONPCB_NAME (elm)));
        E_NS (Value, EMPTY (VALUE_NAME (elm)));
        E_NI (MarkX, elm->MarkX);
        E_NI (MarkY, elm->MarkY);

        E_S ("DescriptionText");
        E_MS (BMS);
        {
          // Note that these offsets don't have any names in the old pcb
          // format, but are computed during save.  So we made these names up.
          E_NI (OffsetX, DESCRIPTION_TEXT (elm).X - elm->MarkX);
          E_NI (OffsetY, DESCRIPTION_TEXT (elm).Y - elm->MarkY);
          // FIXME: pretty wack to save this next one as an integer
          E_NI (Direction, DESCRIPTION_TEXT (elm).Direction);
          // CAREFUL: this next on isn't a Coord type:
          E_NI (Scale, DESCRIPTION_TEXT (elm).Scale);
          EC (object_flags, DESCRIPTION_TEXT (elm).Flags, ELEMENTNAME_TYPE);
        }
        E_ME ();

        EC (attributes, (&(elm->Attributes)));

        EC (pins, elm->Pin, elm->MarkX, elm->MarkY);

        EC (pads, elm->Pad, elm->MarkX, elm->MarkY);

        EC (element_lines, elm->Line, elm->MarkX, elm->MarkY);

        EC (element_arcs, elm->Arc, elm->MarkX, elm->MarkY);
      }
      E_ME ();
    }
  }
  E_SE ();
}

static void
emit_rats (GList *rat_list)
{
  // Emit the rat lines in rat_list as a (name, sequence-of-maps) pair.
  
  // FIXME: perhaps names should be a first arg and emitted by the macro,
  // so e.g. E_C (Rats, emit_rats, rat_list) or perhaps E_C (Rats, ratlist)
  // arranged to call emit_Rats ("Rats", ratlist).  Or perhaps we should
  // keep it simple and just call emit_whatever and still have it put out
  // the name jesus.

  E_S ("Rats");
  E_SS (BSS);
  {
    GList *iter;
    for ( iter = rat_list ; iter != NULL ; iter = g_list_next (iter) ) {
      RatType *rat = iter->data;
      E_MS (IMS);
      {
#undef  CURRENT_STRUCT_POINTER
#define CURRENT_STRUCT_POINTER rat
        E_NPE (Point1);
        E_NIE (group1);
        E_NPE (Point2);
        E_NIE (group2);
        EC (object_flags, rat->Flags, RATLINE_TYPE);
      }
      E_ME ();
    }
  }
  E_SE ();
}

static void
emit_layer_lines (GList *line_list)
{
  // Emit the layer lines in line_list as a (name, sequence-of-maps) pair.
 
  E_S ("Lines");
  E_SS (BSS);
  {
    GList *iter;
    for ( iter = line_list ; iter != NULL ; iter = g_list_next (iter) ) {
      LineType *line = iter->data;
      E_MS (IMS);
      {
#undef  CURRENT_STRUCT_POINTER
#define CURRENT_STRUCT_POINTER line
        E_NPE (Point1);
        E_NPE (Point2);
        E_NIE (Thickness);
        E_NIE (Clearance);
        EC (object_flags, line->Flags, LINE_TYPE);
      }
      E_ME ();
    }
  }
  E_SE ();
}

static void
emit_layer_arcs (GList *arc_list)
{
  // Emit the layer arcs in arc_list as a (name, sequence-of-maps) pair.

  E_S ("Arcs");
  E_SS (BSS);
  {
    GList *iter;
    for ( iter = arc_list ; iter != NULL ; iter = g_list_next (iter) ) {
      ArcType *arc = iter->data;
      E_MS (IMS);
      {
#undef  CURRENT_STRUCT_POINTER
#define CURRENT_STRUCT_POINTER arc
        E_NIE (X);
        E_NIE (Y);
        E_NIE (Width);
        E_NIE (Height);
        E_NIE (Thickness);
        E_NIE (Clearance);
        E_NDE (StartAngle);
        E_NDE (Delta);
        EC (object_flags, arc->Flags, ARC_TYPE);
      }
      E_ME ();
    }
  }
  E_SE ();
}  

static void
emit_layer_texts (GList *texts_list)
{
  // Emit the layer text objects in texts_list as a (name, sequence-of-maps)
  // pair.

  E_S ("Texts");
  E_SS (BSS);
  {
    GList *iter;
    for ( iter = texts_list ; iter != NULL ; iter = g_list_next (iter) ) {
      TextType *text = iter->data;
      E_MS (BMS);
      {
#undef  CURRENT_STRUCT_POINTER
#define CURRENT_STRUCT_POINTER text
        E_NIE (X);
        E_NIE (Y);
        E_NIE (Direction);  // FIXME: it's weird to emit this as an int
        E_NIE (Scale);  // FIXME: CAREFUL: this isnt a coord type
        E_NS (TextString, EMPTY (text->TextString));
        EC (object_flags, text->Flags, TEXT_TYPE);
      }
      E_ME ();
    }
  }
  E_SE ();
}  

static void
emit_layer_polygons (GList *polygon_list)
{
  // Emit the layer polygon objects in polygon_list as a (name,
  // sequence-of-maps) pair.

  E_S ("Polygons");
  E_SS (BSS);
  {
    GList *iter;
    for ( iter = polygon_list ; iter != NULL ; iter = g_list_next (iter) ) {
      PolygonType *polygon = iter->data;
      E_MS (BMS);
      {
        // Polygon storage is a little weird.  The verticies for both
        // the polygon itself *and* for the holes all live together in the
        // single array polygon->Points, of which there are polygon->PointN.
        // The number of holes is polygon->HoleIndexN, and the elements of
        // polygon->HoleIndex are the indicies in polygon->Points of the
        // first vertex of each hole.
        int pvc;   // Polygon Vertex Count (actual polygon verticies)
        if ( polygon->HoleIndexN == 0 ) {
          pvc = polygon->PointN;
        }
        else {
          pvc = polygon->HoleIndex[0];
        }
        EC (object_flags, polygon->Flags, POLYGON_TYPE);
        E_S ("Verticies");
        E_SS (ISS);
        {
          int pvi;   // Polygon Vertex Index
          for ( pvi = 0 ; pvi < pvc ; pvi++ ) {
            E_P ((polygon->Points)[pvi]);
          }
        }
        E_SE ();
        E_S ("Holes");
        E_SS (BSS);
        {
          int hi;   // Hole Index (which hole, not which vertex of hole)
          for ( hi = 0 ; hi < polygon->HoleIndexN ; hi++ ) {
            E_MS (BMS);
            {
              E_S ("Verticies");
              E_SS (ISS);
              {
                int hvi;   // Hole Vertex Index
                // First/Last Hole Vertex Index (see comment above)
                int fhvi = (polygon->HoleIndex)[hi];
                int lhvi;
                if ( hi < polygon->HoleIndexN - 1 ) {
                  lhvi = (polygon->HoleIndex)[hi + 1] - 1; 
                }
                else {
                  lhvi = polygon->PointN - 1;
                }
                for ( hvi = fhvi ; hvi <= lhvi ; hvi++ ) {
                  E_P ((polygon->Points)[hvi]);
                }
              }
              E_SE ();
            }
            E_ME ();
          }
        }
        E_SE ();
      }
      E_ME ();
    }
  }
  E_SE ();
}  

static void
emit_layers (int layer_count, LayerType *layer_array)
{
  // Emit all the actual layer data, including silk layers, layer names and
  // numbers, attributes, and all the actual lines, arcs text and polygons
  // the user has drawn.

  E_S ("Layers");
  E_SS (BSS);
  {
    for ( int ii = 0 ; ii < layer_count ; ii++ ) {
      LayerType *layer = &(layer_array[ii]); 
      bool layer_not_empty
        = ( layer->Line || layer->ArcN || layer->TextN || layer->PolygonN ||
            (layer->Name && *(layer->Name)) );
      if ( layer_not_empty ) {
        E_MS (BMS);
        {
          E_NI (Number, ii + 1);
          E_NS (Name, EMPTY (layer->Name));
          EC (attributes, (&(layer->Attributes)));
          EC (layer_lines, layer->Line);
          EC (layer_arcs, layer->Arc);
          EC (layer_texts, layer->Text);
          EC (layer_polygons, layer->Polygon);
        }
        E_ME ();
      }
    }
  }
  E_SE ();
}

static void
emit_netlist (LibraryType *netlist)
{
  // Emit the netlist as a (name, sequence-of-maps pair).

  E_S ("NetList");
  E_SS (BSS);
  {
    // NOTE: Presumably a net is called a "Menu" here because somewhere in the
    // gui they crop up as a menu.  Model-view separation is for the weak :)
    int ii;
    for ( ii = 0 ; ii < netlist->MenuN ; ii++ ) {
      LibraryMenuType *menu = &((netlist->Menu)[ii]);
      E_MS (BMS);
      {
        // NOTE: the names all start with two spaces, which we don't want.
        // Presumably this is due their role in menus (see comment above).
        assert ((menu->Name)[0] == ' ');
        assert ((menu->Name)[1] == ' ');
        E_NS (Name, &((menu->Name)[2]));
        E_NS (Style, UNKNOWN (menu->Style));
        E_S ("Connections");
        E_SS (YAML_BLOCK_MAPPING_STYLE);
        {
          int jj;
          for ( jj = 0 ; jj < menu->EntryN ; jj++ ) {
            LibraryEntryType *entry = &((menu->Entry)[jj]);
            E_S (entry->ListEntry);
          }
        }
        E_SE ();
      }
      E_ME ();
    }
  }
  E_SE ();
}

static void
emit_entire_yaml_file (PCBType *pcb)
{
  EMIT_EVENT (stream_start, YAML_UTF8_ENCODING);
  // FIXME: perl round-tripping does not use implicit doc start I think
  EMIT_EVENT (document_start, NULL, NULL, NULL, true);
  E_MS (BMS);   // Entire document mapping

#undef  CURRENT_STRUCT_POINTER
#define CURRENT_STRUCT_POINTER pcb

  // Instead of the top-line comment in the original pcb format
  E_NS (Program, Progname);
  // This doesn't have an analog in the original pcb format
  E_NS (Fileformat, FORMAT_ID);
  // Instead of the top-line comment in the original pcb format
  E_NI (YPCB_FILE_VERSION_IMPLEMENTED, YPCB_FILE_VERSION_IMPLEMENTED);

  E_S ("PCB");
  E_MS (BMS);
  {
    E_NS (Name, EMPTY (CURRENT_STRUCT_POINTER -> Name));
    E_NIE (MaxWidth);
    E_NIE (MaxHeight);
  }
  E_ME ();

  E_S ("Grid");
  E_MS (BMS);
  {
    // Here we have a struct member same name as the name of this block.
    // Somewhat ugly but we want to keep the structure names.
    E_NIE (Grid);
    E_NIE (GridOffsetX);
    E_NIE (GridOffsetY);
    // FIXME: its bad that we need stuff from Settings here
    E_NS (DrawGrid, Settings.DrawGrid ? "true" : "false");
  }
  E_ME ();

  // FIXME: whyyyyyy are we doing all these wack conversions here
  E_ND (PolyArea, COORD_TO_MIL (COORD_TO_MIL (PCB->IsleArea) * 100) * 100); 

  E_NDE (ThermScale);

  E_S ("DRC");
  E_MS (BMS);
  {
    E_NIE (Bloat);
    E_NIE (Shrink);
    E_NIE (minWid);
    E_NIE (minSlk);
    E_NIE (minDrill);
    E_NIE (minRing);
  }
  E_ME ();

  EC (pcb_flags, pcb->Flags);

  // FIXME: these should be broken down, not left as a complex string
  E_NS (Groups, LayerGroupsToString (&(pcb->LayerGroups)));

  EC (styles, NUM_STYLES, pcb->RouteStyle);

  EC (font, MAX_FONTPOSITION, (&(pcb->Font)));

  EC (attributes, (&(pcb->Attributes)));

  EC (vias, pcb->Data->Via);

  EC (elements, pcb->Data->Element);

  EC (rats, pcb->Data->Rat);

  EC (layers, pcb->Data->LayerN + SILK_LAYER, pcb->Data->Layer);
  
  // The netlist is only emitted if it exists.
  if ( (pcb->NetlistLib).MenuN ) {
    EC (netlist, (&(pcb->NetlistLib)));
  }

  E_ME ();   // End of entire document mapping
  EMIT_EVENT (document_end, true);
  EMIT_EVENT (stream_end);
}

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

  // Canonical YAML is really ugly
  yaml_emitter_set_canonical (&emitter, 0);

  // We don't want YAML breaking lines for us, so specify no-line-too-long.
  yaml_emitter_set_width (&emitter, -1);
  
  // This is where we come if there are emission errors, otherwise we should
  // never jump here.  Note that it's a big error to try to jump here with
  // EMIT_ERROR_NONE as the value, since that should never be necessary and
  // longjmp will rewrite the value for us causing massive confusion.
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
