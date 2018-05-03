/*!
 * \file src/file.c
 *
 * \brief File save, load, merge ... routines.
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 *
 * \note getpid() needs a cast to (int) to get rid of compiler warnings
 * on several architectures.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "global.h"

#include <assert.h>
#include <dirent.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
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

#if !defined(HAS_ATEXIT) && !defined(HAS_ON_EXIT)
/* ---------------------------------------------------------------------------
 * some local identifiers for OS without an atexit() or on_exit()
 * call
 */
static char TMPFilename[80];
#endif

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void PrintQuotedString (FILE *, char *);
static void WritePCBInfoHeader (FILE *);
static void WritePCBDataHeader (FILE *);
static void WritePCBFontData (FILE *);
static void WriteViaData (FILE *, DataType *);
static void WritePCBRatData (FILE *);
static void WriteElementData (FILE *, DataType *);
static void WriteLayerData (FILE *, Cardinal, LayerType *);
static int WritePCB (FILE *);
static int WritePCBFile (char *);
static int WritePipe (char *, bool);
static int ParseLibraryTree (void);
static int LoadNewlibFootprintsFromDir(char *path, char *toppath, bool recursive);


/* ---------------------------------------------------------------------------
 * Flag helper functions
 */

#define F2S(OBJ, TYPE) flags_to_string ((OBJ)->Flags, TYPE)

/* --------------------------------------------------------------------------- */

/* The idea here is to avoid gratuitously breaking backwards
   compatibility due to a new but rarely used feature.  The first such
   case, for example, was the polygon Hole - if your design included
   polygon holes, you needed a newer PCB to read it, but if your
   design didn't include holes, PCB would produce a file that older
   PCBs could read, if only it had the correct version number in it.

   If, however, you have to add or change a feature that really does
   require a new PCB version all the time, it's time to remove all the
   tests below and just always output the new version.

   Note: Best practices here is to add support for a feature *first*
   (and bump PCB_FILE_VERSION in file.h), and note the version that
   added that support below, and *later* update the file format to
   need that version (which may then be older than PCB_FILE_VERSION).
   Hopefully, that allows for one release between adding support and
   needing it, which should minimize breakage.  Of course, that's not
   *always* possible, practical, or desirable.

*/


#define PCB_FILE_VERSION_BURIED_VIAS 20170218 /*!< Buried vias */

#define PCB_FILE_VERSION_HOLES 20100606 /*!< Hole[] in Polygon. */

#define PCB_FILE_VERSION_BASELINE 20091103 /*!< First version ever saved. */

int
PCBFileVersionNeeded (void)
{
  ALLPOLYGON_LOOP (PCB->Data);
  {
    if (polygon->HoleIndexN > 0)
      return PCB_FILE_VERSION_HOLES;
  }
  ENDALL_LOOP;

  VIA_LOOP (PCB->Data);
    if ((via->BuriedFrom != 0) || (via->BuriedTo != 0))
      return PCB_FILE_VERSION_BURIED_VIAS;
  END_LOOP;

  return PCB_FILE_VERSION_BASELINE;
}

/* --------------------------------------------------------------------------- */

static int
string_cmp (const char *a, const char *b)
{
  while (*a && *b)
    {
      if (isdigit ((int) *a) && isdigit ((int) *b))
	{
	  int ia = atoi (a);
	  int ib = atoi (b);
	  if (ia != ib)
	    return ia - ib;
	  while (isdigit ((int) *a) && *(a+1))
	    a++;
	  while (isdigit ((int) *b) && *(b+1))
	    b++;
	}
      else if (tolower ((int) *a) != tolower ((int) *b))
	return tolower ((int) *a) - tolower ((int) *b);
      a++;
      b++;
    }
  if (*a)
    return 1;
  if (*b)
    return -1;
  return 0;
}

static int netlist_sort_offset = 0;

static int
netlist_sort (const void *va, const void *vb)
{
  LibraryMenuType *am = (LibraryMenuType *) va;
  LibraryMenuType *bm = (LibraryMenuType *) vb;
  char *a = am->Name;
  char *b = bm->Name;
  if (*a == '~')
    a++;
  if (*b == '~')
    b++;
  return string_cmp (a, b);
}

static int
netnode_sort (const void *va, const void *vb)
{
  LibraryEntryType *am = (LibraryEntryType *) va;
  LibraryEntryType *bm = (LibraryEntryType *) vb;
  char *a = am->ListEntry;
  char *b = bm->ListEntry;
  return string_cmp (a, b);
}

static void
sort_library (LibraryType *lib)
{
  int i;
  qsort (lib->Menu, lib->MenuN, sizeof (lib->Menu[0]), netlist_sort);
  for (i = 0; i < lib->MenuN; i++)
    qsort (lib->Menu[i].Entry,
	   lib->Menu[i].EntryN, sizeof (lib->Menu[i].Entry[0]), netnode_sort);
}

void
sort_netlist ()
{
  netlist_sort_offset = 2;
  sort_library (&(PCB->NetlistLib));
  netlist_sort_offset = 0;
}

/*!
 * \brief Opens a file and check if it exists.
 */
FILE *
CheckAndOpenFile (char *Filename, bool Confirm, bool AllButton,
		  bool * WasAllButton, bool * WasCancelButton)
{
  FILE *fp = NULL;
  struct stat buffer;
  char message[MAXPATHLEN + 80];
  int response;

  if (Filename && *Filename)
    {
      if (!stat (Filename, &buffer) && Confirm)
	{
	  sprintf (message, _("File '%s' exists, use anyway?"), Filename);
	  if (WasAllButton)
	    *WasAllButton = false;
	  if (WasCancelButton)
	    *WasCancelButton = false;
	  if (AllButton)
	    response =
	      gui->confirm_dialog (message, "Cancel", "Ok",
				   AllButton ? "Sequence OK" : 0);
	  else
	    response =
	      gui->confirm_dialog (message, "Cancel", "Ok", "Sequence OK");

	  switch (response)
	    {
	    case 2:
	      if (WasAllButton)
		*WasAllButton = true;
	      break;
	    case 0:
	      if (WasCancelButton)
		*WasCancelButton = true;
	    }
	}
      if ((fp = fopen (Filename, "w")) == NULL)
	OpenErrorMessage (Filename);
    }
  return (fp);
}

/*!
 * \brief Opens a file for saving connection data.
 */
FILE *
OpenConnectionDataFile (void)
{
  char *fname;
  FILE *fp;
  static char * default_file = NULL;
  bool result;		/* not used */

  /* CheckAndOpenFile deals with the case where fname already exists */
  fname = gui->fileselect (_("Save Connection Data As ..."),
			   _("Choose a file to save all connection data to."),
			   default_file, ".net", "connection_data",
			   0);
  if (fname == NULL)
    return NULL;

  if (default_file != NULL)
    {
      free (default_file);
      default_file = NULL;
    }

  if (fname && *fname)
    default_file = strdup (fname);

  fp = CheckAndOpenFile (fname, true, false, &result, NULL);
  free (fname);

  return fp;
}

/*!
 * \brief Save elements in the current buffer.
 */
int
SaveBufferElements (char *Filename)
{
  int result;

  if (SWAP_IDENT)
    SwapBuffers ();
  result = WritePipe (Filename, false);
  if (SWAP_IDENT)
    SwapBuffers ();
  return (result);
}

/*!
 * \brief Save PCB.
 */
int
SavePCB (char *file)
{
    return WritePipe (file, true);
}

/*!
 * \brief Set the route style to the first one, if the current one
 * doesn't happen to match any.
 *
 * This way, "revert" won't change the route style.
 */
static void
set_some_route_style ()
{
  if (hid_get_flag ("style"))
    return;
  SetLineSize (PCB->RouteStyle[0].Thick);
  SetViaSize (PCB->RouteStyle[0].Diameter, true);
  SetViaDrillingHole (PCB->RouteStyle[0].Hole, true);
  SetKeepawayWidth (PCB->RouteStyle[0].Keepaway);
}

/*!
 * \brief Load PCB.
 *
 * Parse the file with enabled 'PCB mode' (see parser) if successful,
 * update some other stuff.
 *
 * If revert is true, we pass "revert" as a parameter to the HID's
 * PCBChanged action.
 */
static int
real_load_pcb (char *Filename, char *Format, bool revert)
{
  const char *unit_suffix, *grid_size;
  char *new_filename, *new_format;
  PCBType *newPCB = NULL; /* = CreateNewPCB (); */
  PCBType *oldPCB;
#ifdef DEBUG
  double elapsed;
  clock_t start, end;

  start = clock ();
#endif

  new_filename = strdup (Filename);

  oldPCB = PCB;

  /* new data isn't added to the undo list */
  if (!LoadPCBWithFormat (&PCB, new_filename, Format, &new_format))
    {
      RemovePCB (oldPCB);

      CreateNewPCBPost (PCB, 0);
      ResetStackAndVisibility ();
      AssignDefaultLayerTypes ();

      /* update cursor location */
      Crosshair.X = CLAMP (PCB->CursorX, 0, PCB->MaxWidth);
      Crosshair.Y = CLAMP (PCB->CursorY, 0, PCB->MaxHeight);

      /* update cursor confinement and output area (scrollbars) */
      ChangePCBSize (PCB->MaxWidth, PCB->MaxHeight);

      /* enable default font if necessary */
      if (!PCB->Font.Valid)
	{
	  Message (_
		   ("File '%s' has no font information, using default font\n"),
		   new_filename);
	  PCB->Font.Valid = true;
	}

      /* clear 'changed flag' */
      SetChangedFlag (false);
      PCB->Filename = new_filename;
      /* we do not care about saveability anymore - it will be checked at proper places */
      PCB->Fileformat = strdup (new_format);

#if 0
      /* the fileformat must be saveable; if not, default format is used */
      if (hid_file_format_capable (new_format, HID_FFORMAT_SAVEABLE))
          PCB->Fileformat = strdup (new_format);
      else
          PCB->Fileformat = strdup (hid_get_default_format_id ());
#endif

      /* Use attribute PCB::grid::unit as unit, if we can */
      unit_suffix = AttributeGet (PCB, "PCB::grid::unit");
      if (unit_suffix && *unit_suffix)
        {
          const Unit *new_unit = get_unit_struct (unit_suffix);
          if (new_unit)
            Settings.grid_unit = new_unit;
        }
      AttributePut (PCB, "PCB::grid::unit", Settings.grid_unit->suffix);
      Settings.increments = get_increments_struct (Settings.grid_unit->family);

      /* Use attribute PCB::grid::size as size, if we can */
      grid_size = AttributeGet (PCB, "PCB::grid::size");
      if (grid_size)
        {
          PCB->Grid = GetValue (grid_size, NULL, NULL);
        }
 
      sort_netlist ();

      set_some_route_style ();

      if (revert)
        hid_actionl ("PCBChanged", "revert", NULL);
      else
        hid_action ("PCBChanged");

#ifdef DEBUG
      end = clock ();
      elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;
      gui->log ("Loading file %s took %f seconds of CPU time\n",
		new_filename, elapsed);
#endif

      return (0);
    }

  newPCB = PCB;
  PCB = oldPCB;

  hid_action ("PCBChanged");

  /* release unused memory */
  RemovePCB (newPCB);
  return (1);
}

/*!
 * \brief Load PCB.
 */
int
LoadPCB (char *file)
{
  return real_load_pcb (file, NULL, false);
}

/*!
 * \brief Revert PCB.
 */
int
RevertPCB (void)
{
  return real_load_pcb (PCB->Filename, PCB->Fileformat, true);
}

/*!
 * \brief Writes the quoted string created by another subroutine.
 */
static void
PrintQuotedString (FILE * FP, char *S)
{
  static DynamicStringType ds;

  CreateQuotedString (&ds, S);
  fputs (ds.Data, FP);
}

/*!
 * \brief Writes out an attribute list.
 */
static void
WriteAttributeList (FILE * FP, AttributeListType *list, char *prefix)
{
  int i;

  for (i = 0; i < list->Number; i++)
  {
    fprintf (FP, "%sAttribute(", prefix);
    PrintQuotedString(FP, list->List[i].name);
    fputs (" ", FP);
    PrintQuotedString(FP, list->List[i].value);
    fputs (")\n", FP);
  }
}

/*!
 * \brief Writes layout header information.
 */
static void
WritePCBInfoHeader (FILE * FP)
{
  /* write some useful comments */
  fprintf (FP, "# release: %s " VERSION "\n", Progname);

  /* avoid writing things like user name or date, as these cause merge
   * conflicts in collaborative environments using version control systems
   */
}

/*!
 * \brief Writes data header.
 *
 * The name of the PCB, cursor location, zoom and grid layergroups and
 * some flags.
 */
static void
WritePCBDataHeader (FILE * FP)
{
  Cardinal group;

  /*
   * ************************** README *******************
   * ************************** README *******************
   *
   * If the file format is modified in any way, update
   * PCB_FILE_VERSION in file.h as well as PCBFileVersionNeeded()
   * at the top of this file.
   *  
   * ************************** README *******************
   * ************************** README *******************
   */

  fprintf (FP, "\n# To read pcb files, the pcb version (or the git source date) must be >= the file version\n");
  fprintf (FP, "FileVersion[%i]\n", PCBFileVersionNeeded ());

  fputs ("\nPCB[", FP);
  PrintQuotedString (FP, (char *)EMPTY (PCB->Name));
  pcb_fprintf (FP, " %mr %mr]\n\n", PCB->MaxWidth, PCB->MaxHeight);
  pcb_fprintf (FP, "Grid[%mr %mr %mr %d]\n", PCB->Grid,
               PCB->GridOffsetX, PCB->GridOffsetY, Settings.DrawGrid);
  /* PolyArea should be output in square cmils, no suffix */
  fprintf (FP, "PolyArea[%s]\n", c_dtostr (COORD_TO_MIL (COORD_TO_MIL (PCB->IsleArea) * 100) * 100));
  pcb_fprintf (FP, "Thermal[%s]\n", c_dtostr (PCB->ThermScale));
  pcb_fprintf (FP, "DRC[%mr %mr %mr %mr %mr %mr]\n", PCB->Bloat, PCB->Shrink,
	       PCB->minWid, PCB->minSlk, PCB->minDrill, PCB->minRing);
  fprintf (FP, "Flags(%s)\n", pcbflags_to_string(PCB->Flags));
  fprintf (FP, "Groups(\"%s\")\n", LayerGroupsToString (&PCB->LayerGroups));
  fputs ("Styles[\"", FP);
  for (group = 0; group < NUM_STYLES - 1; group++)
    pcb_fprintf (FP, "%s,%mr,%mr,%mr,%mr:", PCB->RouteStyle[group].Name,
	         PCB->RouteStyle[group].Thick,
	         PCB->RouteStyle[group].Diameter,
	         PCB->RouteStyle[group].Hole, PCB->RouteStyle[group].Keepaway);
  pcb_fprintf (FP, "%s,%mr,%mr,%mr,%mr\"]\n\n", PCB->RouteStyle[group].Name,
	       PCB->RouteStyle[group].Thick,
	       PCB->RouteStyle[group].Diameter,
	       PCB->RouteStyle[group].Hole, PCB->RouteStyle[group].Keepaway);
}

/*!
 * \brief Writes font data of non empty symbols.
 */
static void
WritePCBFontData (FILE * FP)
{
  Cardinal i, j;
  LineType *line;
  FontType *font;

  for (font = &PCB->Font, i = 0; i <= MAX_FONTPOSITION; i++)
    {
      if (!font->Symbol[i].Valid)
	continue;

      if (isprint (i))
	pcb_fprintf (FP, "Symbol['%c' %mr]\n(\n", i, font->Symbol[i].Delta);
      else
	pcb_fprintf (FP, "Symbol[%i %mr]\n(\n", i, font->Symbol[i].Delta);

      line = font->Symbol[i].Line;
      for (j = font->Symbol[i].LineN; j; j--, line++)
        pcb_fprintf (FP, "\tSymbolLine[%mr %mr %mr %mr %mr]\n",
                     line->Point1.X, line->Point1.Y,
                     line->Point2.X, line->Point2.Y, line->Thickness);
      fputs (")\n", FP);
    }
}

/*!
 * \brief Writes via data.
 */
static void
WriteViaData (FILE * FP, DataType *Data)
{
  GList *iter;
  /* write information about vias */
  for (iter = Data->Via; iter != NULL; iter = g_list_next (iter))
    {
      PinType *via = iter->data;
      pcb_fprintf (FP, "Via[%mr %mr %mr %mr %mr %mr ", via->X, via->Y,
                   via->Thickness, via->Clearance, via->Mask, via->DrillingHole);
      if ((via->BuriedFrom != 0) || (via->BuriedTo != 0))
        fprintf (FP, "%d %d ", via->BuriedFrom, via->BuriedTo);
      PrintQuotedString (FP, (char *)EMPTY (via->Name));
      fprintf (FP, " %s]\n", F2S (via, VIA_TYPE));
    }
}

/*!
 * \brief Writes rat-line data.
 */
static void
WritePCBRatData (FILE * FP)
{
  GList *iter;
  /* write information about rats */
  for (iter = PCB->Data->Rat; iter != NULL; iter = g_list_next (iter))
    {
      RatType *line = iter->data;
      pcb_fprintf (FP, "Rat[%mr %mr %d %mr %mr %d ",
                   line->Point1.X, line->Point1.Y, line->group1,
                   line->Point2.X, line->Point2.Y, line->group2);
      fprintf (FP, " %s]\n", F2S (line, RATLINE_TYPE));
    }
}

/*!
 * \brief Writes netlist data.
 */
static void
WritePCBNetlistData (FILE * FP)
{
  /* write out the netlist if it exists */
  if (PCB->NetlistLib.MenuN)
    {
      int n, p;
      fprintf (FP, "NetList()\n(\n");

      for (n = 0; n < PCB->NetlistLib.MenuN; n++)
	{
	  LibraryMenuType *menu = &PCB->NetlistLib.Menu[n];
	  fprintf (FP, "\tNet(");
	  PrintQuotedString(FP, &menu->Name[2]);
	  fprintf (FP, " ");
	  PrintQuotedString(FP, (char *)UNKNOWN (menu->Style));
	  fprintf (FP, ")\n\t(\n");
	  for (p = 0; p < menu->EntryN; p++)
	    {
	      LibraryEntryType *entry = &menu->Entry[p];
	      fprintf (FP, "\t\tConnect(");
	      PrintQuotedString (FP, entry->ListEntry);
	      fprintf (FP, ")\n");
	    }
	  fprintf (FP, "\t)\n");
	}
      fprintf (FP, ")\n");
    }
}

/*!
 * \brief Writes element data.
 */
static void
WriteElementData (FILE * FP, DataType *Data)
{
  GList *n, *p;
  for (n = Data->Element; n != NULL; n = g_list_next (n))
    {
      ElementType *element = n->data;

      /* only non empty elements */
      if (!element->LineN && !element->PinN && !element->ArcN
	  && !element->PadN)
	continue;
      /* the coordinates and text-flags are the same for
       * both names of an element
       */
      fprintf (FP, "\nElement[%s ", F2S (element, ELEMENT_TYPE));
      PrintQuotedString (FP, (char *)EMPTY (DESCRIPTION_NAME (element)));
      fputc (' ', FP);
      PrintQuotedString (FP, (char *)EMPTY (NAMEONPCB_NAME (element)));
      fputc (' ', FP);
      PrintQuotedString (FP, (char *)EMPTY (VALUE_NAME (element)));
      pcb_fprintf (FP, " %mr %mr %mr %mr %d %d %s]\n(\n",
                   element->MarkX, element->MarkY,
                   DESCRIPTION_TEXT (element).X - element->MarkX,
                   DESCRIPTION_TEXT (element).Y - element->MarkY,
                   DESCRIPTION_TEXT (element).Direction,
                   DESCRIPTION_TEXT (element).Scale,
                   F2S (&(DESCRIPTION_TEXT (element)), ELEMENTNAME_TYPE));
      WriteAttributeList (FP, &element->Attributes, "\t");
      for (p = element->Pin; p != NULL; p = g_list_next (p))
	{
	  PinType *pin = p->data;
          pcb_fprintf (FP, "\tPin[%mr %mr %mr %mr %mr %mr ",
                       pin->X - element->MarkX,
                       pin->Y - element->MarkY,
                       pin->Thickness, pin->Clearance,
                       pin->Mask, pin->DrillingHole);
	  PrintQuotedString (FP, (char *)EMPTY (pin->Name));
	  fprintf (FP, " ");
	  PrintQuotedString (FP, (char *)EMPTY (pin->Number));
	  fprintf (FP, " %s]\n", F2S (pin, PIN_TYPE));
	}
      for (p = element->Pad; p != NULL; p = g_list_next (p))
	{
	  PadType *pad = p->data;
          pcb_fprintf (FP, "\tPad[%mr %mr %mr %mr %mr %mr %mr ",
                       pad->Point1.X - element->MarkX,
                       pad->Point1.Y - element->MarkY,
                       pad->Point2.X - element->MarkX,
                       pad->Point2.Y - element->MarkY,
                       pad->Thickness, pad->Clearance, pad->Mask);
	  PrintQuotedString (FP, (char *)EMPTY (pad->Name));
	  fprintf (FP, " ");
	  PrintQuotedString (FP, (char *)EMPTY (pad->Number));
	  fprintf (FP, " %s]\n", F2S (pad, PAD_TYPE));
	}
      for (p = element->Line; p != NULL; p = g_list_next (p))
	{
	  LineType *line = p->data;
          pcb_fprintf (FP, "\tElementLine [%mr %mr %mr %mr %mr]\n",
                       line->Point1.X - element->MarkX,
                       line->Point1.Y - element->MarkY,
                       line->Point2.X - element->MarkX,
                       line->Point2.Y - element->MarkY,
                       line->Thickness);
	}
      for (p = element->Arc; p != NULL; p = g_list_next (p))
	{
	  ArcType *arc = p->data;
          pcb_fprintf (FP, "\tElementArc [%mr %mr %mr %mr %ma %ma %mr]\n",
                       arc->X - element->MarkX,
                       arc->Y - element->MarkY,
                       arc->Width, arc->Height,
                       arc->StartAngle, arc->Delta,
                       arc->Thickness);
	}
      fputs ("\n\t)\n", FP);
    }
}

/*!
 * \brief Writes layer data.
 */
static void
WriteLayerData (FILE * FP, Cardinal Number, LayerType *layer)
{
  GList *n;
  /* write information about non empty layers */
  if (layer->LineN || layer->ArcN || layer->TextN || layer->PolygonN ||
      (layer->Name && *layer->Name))
    {
      fprintf (FP, "Layer(%i ", (int) Number + 1);
      PrintQuotedString (FP, (char *)EMPTY (layer->Name));
      fprintf (FP, " \"%s\")\n(\n", layertype_to_string (layer->Type));
      WriteAttributeList (FP, &layer->Attributes, "\t");

      for (n = layer->Line; n != NULL; n = g_list_next (n))
	{
	  LineType *line = n->data;
          pcb_fprintf (FP, "\tLine[%mr %mr %mr %mr %mr %mr %s]\n",
                       line->Point1.X, line->Point1.Y,
                       line->Point2.X, line->Point2.Y,
                       line->Thickness, line->Clearance,
                       F2S (line, LINE_TYPE));
	}
      for (n = layer->Arc; n != NULL; n = g_list_next (n))
	{
	  ArcType *arc = n->data;
          pcb_fprintf (FP, "\tArc[%mr %mr %mr %mr %mr %mr %ma %ma %s]\n",
                       arc->X, arc->Y, arc->Width,
                       arc->Height, arc->Thickness,
                       arc->Clearance, arc->StartAngle,
                       arc->Delta, F2S (arc, ARC_TYPE));
	}
      for (n = layer->Text; n != NULL; n = g_list_next (n))
	{
	  TextType *text = n->data;
          pcb_fprintf (FP, "\tText[%mr %mr %d %d ",
                       text->X, text->Y,
                       text->Direction, text->Scale);
	  PrintQuotedString (FP, (char *)EMPTY (text->TextString));
	  fprintf (FP, " %s]\n", F2S (text, TEXT_TYPE));
	}
      for (n = layer->Polygon; n != NULL; n = g_list_next (n))
	{
	  PolygonType *polygon = n->data;
	  int p, i = 0;
	  Cardinal hole = 0;
	  fprintf (FP, "\tPolygon(%s)\n\t(", F2S (polygon, POLYGON_TYPE));
	  for (p = 0; p < polygon->PointN; p++)
	    {
	      PointType *point = &polygon->Points[p];

	      if (hole < polygon->HoleIndexN &&
		  p == polygon->HoleIndex[hole])
		{
		  if (hole > 0)
		    fputs ("\n\t\t)", FP);
		  fputs ("\n\t\tHole (", FP);
		  hole++;
		  i = 0;
		}

	      if (i++ % 5 == 0)
		{
		  fputs ("\n\t\t", FP);
		  if (hole)
		    fputs ("\t", FP);
		}
              pcb_fprintf (FP, "[%mr %mr] ", point->X, point->Y);
	    }
	  if (hole > 0)
	    fputs ("\n\t\t)", FP);
	  fputs ("\n\t)\n", FP);
	}
      fputs (")\n", FP);
    }
}

/*!
 * \brief Writes just the elements in the buffer to file.
 */
static int
WriteBuffer (FILE * FP)
{
  Cardinal i;

  WriteViaData (FP, PASTEBUFFER->Data);
  WriteElementData (FP, PASTEBUFFER->Data);
  for (i = 0; i < max_copper_layer + SILK_LAYER; i++)
    WriteLayerData (FP, i, &(PASTEBUFFER->Data->Layer[i]));
  return (STATUS_OK);
}

/*!
 * \brief Writes PCB to file.
 */
static int
WritePCB (FILE * FP)
{
  Cardinal i;
  if (Settings.SaveMetricOnly)
    set_allow_readable (ALLOW_MM);
  else
    set_allow_readable (ALLOW_READABLE);

  WritePCBInfoHeader (FP);
  WritePCBDataHeader (FP);
  WritePCBFontData (FP);
  WriteAttributeList (FP, &PCB->Attributes, "");
  WriteViaData (FP, PCB->Data);
  WriteElementData (FP, PCB->Data);
  WritePCBRatData (FP);
  for (i = 0; i < max_copper_layer + SILK_LAYER; i++)
    WriteLayerData (FP, i, &(PCB->Data->Layer[i]));
  WritePCBNetlistData (FP);

  return (STATUS_OK);
}

/*!
 * \brief Writes PCB to file.
 */
static int
WritePCBFile (char *Filename)
{
  FILE *fp;
  int result;

  if ((fp = fopen (Filename, "w")) == NULL)
    {
      OpenErrorMessage (Filename);
      return (STATUS_ERROR);
    }
  result = WritePCB (fp);
  fclose (fp);
  return (result);
}

/*!
 * \brief Writes to pipe using the command defined by Settings.SaveCommand
 * %f are replaced by the passed filename.
 */
static int
WritePipe (char *Filename, bool thePcb)
{
  FILE *fp;
  int result;
  char *p;
  static DynamicStringType command;
  int used_popen = 0;

  if (EMPTY_STRING_P (Settings.SaveCommand))
    {
      fp = fopen (Filename, "w");
      if (fp == 0)
	{
	  Message ("Unable to write to file %s\n", Filename);
	  return STATUS_ERROR;
	}
    }
  else
    {
      used_popen = 1;
      /* setup commandline */
      DSClearString (&command);
      for (p = Settings.SaveCommand; *p; p++)
	{
	  /* copy character if not special or add string to command */
	  if (!(*p == '%' && *(p + 1) == 'f'))
	    DSAddCharacter (&command, *p);
	  else
	    {
	      DSAddString (&command, Filename);

	      /* skip the character */
	      p++;
	    }
	}
      DSAddCharacter (&command, '\0');
      printf ("write to pipe \"%s\"\n", command.Data);
      if ((fp = popen (command.Data, "w")) == NULL)
	{
	  PopenErrorMessage (command.Data);
	  return (STATUS_ERROR);
	}
    }
  if (thePcb)
    {
      if (PCB->is_footprint)
	{
	  WriteElementData (fp, PCB->Data);
	  result = 0;
	}
      else
	result = WritePCB (fp);
    }
  else
    result = WriteBuffer (fp);

  if (used_popen)
    return (pclose (fp) ? STATUS_ERROR : result);
  return (fclose (fp) ? STATUS_ERROR : result);
}

/*!
 * \brief Saves the layout in a temporary file.
 *
 * This is used for fatal errors and does not call the program specified
 * in 'saveCommand' for safety reasons.
 */
void
SaveInTMP (void)
{
  char filename[80];

  /* memory might have been released before this function is called */
  if (PCB && PCB->Changed)
    {
      sprintf (filename, EMERGENCY_NAME, (int) getpid ());
      Message (_("Trying to save your layout in '%s'\n"), filename);
      WritePCBFile (filename);
    }
}

/*!
 * \brief Front-end for 'SaveInTMP()'.
 *
 * Just makes sure that the routine is only called once.
 */
static bool dont_save_any_more = false;
void
EmergencySave (void)
{

  if (!dont_save_any_more)
    {
      SaveInTMP ();
      dont_save_any_more = true;
    }
}

 void 
DisableEmergencySave (void)
{
  dont_save_any_more = true;
}

static hidval backup_timer;

/*!
 * \brief Callback for the autosave.
 *  
 * If the backup interval is > 0 then set another timer.\n
 * Otherwise we do nothing and it is up to the GUI to call 
 * EnableAutosave() after setting Settings.\n
 * BackupInterval > 0 again.
 */
static void
backup_cb (hidval data)
{
  backup_timer.ptr = NULL;
  Backup ();
  if (Settings.BackupInterval > 0 && gui->add_timer)
    backup_timer = gui->add_timer (backup_cb, 
				   1000 * Settings.BackupInterval, data);
}

void
EnableAutosave (void)
{
  hidval x;

  x.ptr = NULL;

  /* If we already have a timer going, then cancel it out */
  if (backup_timer.ptr != NULL && gui->stop_timer)
    gui->stop_timer (backup_timer);

  backup_timer.ptr = NULL;
  /* Start up a new timer */
  if (Settings.BackupInterval > 0 && gui->add_timer)
    backup_timer = gui->add_timer (backup_cb, 
				   1000 * Settings.BackupInterval, 
				   x);
}

/*!
 * \brief Creates a backup file.
 *
 * The default is to use the pcb file name with a "~" appended (like
 * "foo.pcb~") and if we don't have a pcb file name then use the
 * template in BACKUP_NAME.
 */
void
Backup (void)
{
  char *filename = NULL;
  char *fileformat;
  char *save_savecommand;

  if (PCB && PCB->Filename && hid_file_format_capable (PCB->Fileformat, HID_FFORMAT_SAVEABLE))
    {
      filename 	= (char *) malloc (sizeof (char) * (strlen (PCB->Filename) + 2));
      if (filename == NULL)
	{
	  fprintf (stderr, "Backup():  malloc failed\n");
	  exit (1);
	}
      sprintf (filename, "%s~", PCB->Filename);
      fileformat = PCB->Fileformat;
    }
  else
    {
      /* BACKUP_NAME has %.8i which  will be replaced by the process ID */
      filename 	= (char *) malloc (sizeof (char) * (strlen (BACKUP_NAME) + 8));
      if (filename == NULL)
	{
	  fprintf (stderr, "Backup():  malloc failed\n");
	  exit (1);
	}
      sprintf (filename, BACKUP_NAME, (int) getpid ());
      fileformat = hid_get_default_format_id ();
    }

  save_savecommand = Settings.SaveCommand;
  Settings.SaveCommand = NULL;

  SavePCBWithFormat (PCB, filename, fileformat);

  Settings.SaveCommand = save_savecommand;

  free (filename);
}

#if !defined(HAS_ATEXIT) && !defined(HAS_ON_EXIT)
/*!
 * \brief Makes a temporary copy of the data.
 *
 * This is useful for systems which doesn't support calling functions on
 * exit.\n
 * We use this to save the data before LEX and YACC functions are called
 * because they are able to abort the program.
 */
void
SaveTMPData (void)
{
  char *save_savecommand;

  sprintf (TMPFilename, EMERGENCY_NAME, (int) getpid ());

  save_savecommand = Settings.SaveCommand;
  Settings.SaveCommand = NULL;

  WritePCBFile (TMPFilename);

  Settings.SaveCommand = save_savecommand;
}

/*!
 * \brief Removes the temporary copy of the data file.
 */
void
RemoveTMPData (void)
{
  unlink (TMPFilename);
}
#endif

/*!
 * \brief Parse the directory tree where newlib footprints are found.
 *
 * This is a helper function for ParseLibrary Tree.\n
 * Given a char *path, it finds all newlib footprints in that dir,
 * sticks them into the library menu structure named entry, and recurses
 * into subdirectories.
 */
static int
LoadNewlibFootprintsFromDir(char *libpath, char *toppath, bool recursive)
{
  char olddir[MAXPATHLEN + 1];    /* The directory we start out in (cwd) */
  char subdir[MAXPATHLEN + 1];    /* The directory holding footprints to load */
  DIR *subdirobj;                 /* Interable object holding all subdir entries */
  struct dirent *subdirentry;     /* Individual subdir entry */
  struct stat buffer;             /* Buffer used in stat */
  LibraryMenuType *menu = NULL; /* Pointer to PCB's library menu structure */
  LibraryEntryType *entry;      /* Pointer to individual menu entry */
  size_t l;
  size_t len;
  int n_footprints = 0;           /* Running count of footprints found in this subdir */

  /* Cache old dir, then cd into subdir because stat is given relative file names. */
  memset (subdir, 0, sizeof subdir);
  memset (olddir, 0, sizeof olddir);
  if (GetWorkingDirectory (olddir) == NULL)
    {
      Message (_("LoadNewlibFootprintsFromDir: Could not determine initial working directory\n"));
      return 0;
    }

  if (strcmp (libpath, "(local)") == 0)
    strcpy (subdir, ".");
  else
    strcpy (subdir, libpath);

  if (chdir (subdir))
    {
      ChdirErrorMessage (subdir);
      return 0;
    }

  /* Determine subdir is abs path */
  if (GetWorkingDirectory (subdir) == NULL)
    {
      Message (_("LoadNewlibFootprintsFromDir: Could not determine new working directory\n"));
      if (chdir (olddir))
        ChdirErrorMessage (olddir);
      return 0;
    }

  /* First try opening the directory specified by path */
  if ( (subdirobj = opendir (subdir)) == NULL )
    {
      OpendirErrorMessage (subdir);
      if (chdir (olddir))
        ChdirErrorMessage (olddir);
      return 0;
    }

  /* Get pointer to memory holding menu */
  menu = GetLibraryMenuMemory (&Library);
  /* Populate menuname and path vars */
  menu->Name = strdup (subdir);
  menu->directory = strdup (toppath);

  /* Now loop over files in this directory looking for files.
   * We ignore certain files which are not footprints.
   */
  while ((subdirentry = readdir (subdirobj)) != NULL)
  {
#ifdef DEBUG
/*    printf("...  Examining file %s ... \n", subdirentry->d_name); */
#endif

    /* Ignore non-footprint files found in this directory
     * We're skipping .png and .html because those
     * may exist in a library tree to provide an html browsable
     * index of the library.
     */
    l = strlen (subdirentry->d_name);
    if (!stat (subdirentry->d_name, &buffer) && S_ISREG (buffer.st_mode)
      && subdirentry->d_name[0] != '.'
      && NSTRCMP (subdirentry->d_name, "CVS") != 0
      && NSTRCMP (subdirentry->d_name, "Makefile") != 0
      && NSTRCMP (subdirentry->d_name, "Makefile.am") != 0
      && NSTRCMP (subdirentry->d_name, "Makefile.in") != 0
      && (l < 4 || NSTRCMP(subdirentry->d_name + (l - 4), ".png") != 0) 
      && (l < 5 || NSTRCMP(subdirentry->d_name + (l - 5), ".html") != 0)
      && (l < 4 || NSTRCMP(subdirentry->d_name + (l - 4), ".pcb") != 0) )
      {
#ifdef DEBUG
/*	printf("...  Found a footprint %s ... \n", subdirentry->d_name); */
#endif
	n_footprints++;
	entry = GetLibraryEntryMemory (menu);

	/* 
	 * entry->AllocatedMemory points to abs path to the footprint.
	 * entry->ListEntry points to fp name itself.
	 */
	len = strlen(subdir) + strlen("/") + strlen(subdirentry->d_name) + 1;
	entry->AllocatedMemory = (char *)calloc (1, len);
	strcat (entry->AllocatedMemory, subdir);
	strcat (entry->AllocatedMemory, PCB_DIR_SEPARATOR_S);

	/* store pointer to start of footprint name */
	entry->ListEntry = entry->AllocatedMemory
	    + strlen (entry->AllocatedMemory);

	/* Now place footprint name into AllocatedMemory */
	strcat (entry->AllocatedMemory, subdirentry->d_name);

	/* mark as directory tree (newlib) library */
	entry->Template = (char *) -1;
      }
  }
  closedir (subdirobj);

  /* Don't recurse into relatively-specified directories--we might be
     in the user's working directory, and the path might be "." */
  if (!recursive) {
    if (chdir (olddir))
      ChdirErrorMessage (olddir);
    return n_footprints;
  }

  /* Then open this dir so we can loop over its contents. */
  if ((subdirobj = opendir (subdir)) == NULL)
    {
      OpendirErrorMessage (subdir);
      if (chdir (olddir))
        ChdirErrorMessage (olddir);
      return 0;
    }

  /* Now loop over files in this directory looking for subdirs.
   * For each direntry which is a valid subdirectory,
   * try to load newlib footprints inside it.
   */
  while ((subdirentry = readdir (subdirobj)) != NULL)
    {
#ifdef DEBUG
      printf("In ParseLibraryTree loop examining 2nd level direntry %s ... \n", subdirentry->d_name);
#endif
      /* Find subdirectories.  Ignore entries beginning with "." and CVS
       * directories.
       */
      if (!stat (subdirentry->d_name, &buffer)
	  && S_ISDIR (buffer.st_mode)
	  && subdirentry->d_name[0] != '.'
	  && NSTRCMP (subdirentry->d_name, "CVS") != 0)
	{
	  /* Found a valid subdirectory.  Try to load footprints from it.
	   */
	  char *subdir_path = (char *)calloc (
	    1, strlen(subdir) + strlen("/") + strlen(subdirentry->d_name) + 1);
	  if (subdir_path == NULL)
	    {
	      fprintf (stderr, "LoadNewlibFootprintsFromDir():  "
			       "malloc failed\n");
	      closedir (subdirobj);
	      if (chdir (olddir))
		ChdirErrorMessage (olddir);
	      return n_footprints;
	    }
	  strcat (subdir_path, subdir);
	  strcat (subdir_path, PCB_DIR_SEPARATOR_S);
	  strcat (subdir_path, subdirentry->d_name);

	  n_footprints += LoadNewlibFootprintsFromDir(subdir_path, toppath, true);
	  free(subdir_path);
	}
    }
  /* Done.  Clean up, cd back into old dir, and return */
  closedir (subdirobj);
  if (chdir (olddir))
    ChdirErrorMessage (olddir);
  return n_footprints;
}


/*!
 * \brief This function loads the newlib footprints into the Library.
 *
 * It examines all directories pointed to by Settings.LibraryTree.
 * It calls the subfunction LoadNewlibFootprintsFromDir to put the
 * footprints into PCB's internal datastructures.
 */
static int
ParseLibraryTree (void)
{
  char toppath[MAXPATHLEN + 1];    /* String holding abs path to top level library dir */
  char working[MAXPATHLEN + 1];    /* String holding abs path to working dir */
  char *libpaths;                  /* String holding list of library paths to search */
  char *p;                         /* Helper string used in iteration */
  int n_footprints = 0;            /* Running count of footprints found */
  bool is_abs = false;             /* If we are processing an absolute path */

  /* Initialize path, working by writing 0 into every byte. */
  memset (toppath, 0, sizeof toppath);
  memset (working, 0, sizeof working);

  /* Save the current working directory as an absolute path.
   * This fcn writes the abs path into the memory pointed to by the input arg.
   */
  if (GetWorkingDirectory (working) == NULL)
    {
      Message (_("ParseLibraryTree: Could not determine initial working directory\n"));
      return 0;
    }

  /* Additional loop to allow for multiple 'newlib' style library directories 
   * called out in Settings.LibraryTree
   */
  libpaths = strdup (Settings.LibraryTree);
  for (p = strtok (libpaths, PCB_PATH_DELIMETER); p && *p; p = strtok (NULL, PCB_PATH_DELIMETER))
    {
      /* remove trailing path delimeter */
      strncpy (toppath, p, sizeof (toppath) - 1);

      /* start out in the working directory in case the path is a
       * relative path 
       */
      if (chdir (working))
        {
          ChdirErrorMessage (working);
          free (libpaths);
          return 0;
        }

      /*
       * Next change to the directory which is the top of the library tree
       * and extract its abs path.
       */
      if (chdir (toppath))
        {
          ChdirErrorMessage (toppath);
          continue;
        }

      if (GetWorkingDirectory (toppath) == NULL)
        {
          Message (_("ParseLibraryTree: Could not determine new working directory\n"));
          continue;
        }

      /* figure out if this is an absolute path.  Make sure it works on win32 as well. */
      if (*p == PCB_DIR_SEPARATOR_C)
	{
         is_abs = true;
        }
      else if (strlen(p) > 3 && isalpha ((int) p[0]) && p[1] == ':' && p[2] == PCB_DIR_SEPARATOR_C)
	{
         is_abs = true;
        }

#ifdef DEBUG
      printf("In ParseLibraryTree, looking for newlib footprints inside top level directory %s ... \n", 
	     toppath);
#endif

      /* Next read in any footprints in the top level dir and below */
      n_footprints += LoadNewlibFootprintsFromDir("(local)", toppath, is_abs);
    }

  /* restore the original working directory */
  if (chdir (working))
    ChdirErrorMessage (working);

#ifdef DEBUG
  printf("Leaving ParseLibraryTree, found %d footprints.\n", n_footprints);
#endif

  free (libpaths);
  return n_footprints;
}

/*!
 * \brief Read contents of the library description file (for M4)
 * and then read in M4 libs.
 *
 * Then call a fcn to read the newlib footprints.
 */
int
ReadLibraryContents (void)
{
  static char *command = NULL;
  char inputline[MAX_LIBRARY_LINE_LENGTH + 1];
  FILE *resultFP = NULL;
  LibraryMenuType *menu = NULL;
  LibraryEntryType *entry;

  /* If we don't have a command to execute to find the library contents,
   * skip this. This is used by default on Windows builds (set in main.c),
   * as we can't normally run shell scripts or expect to have m4 present.
   */
  if (Settings.LibraryContentsCommand != NULL &&
      Settings.LibraryContentsCommand[0] != '\0')
    {
      /*  First load the M4 stuff.  The variable Settings.LibraryPath
       *  points to it.
       */
      free (command);
      command = EvaluateFilename (Settings.LibraryContentsCommand,
				  Settings.LibraryPath, Settings.LibraryFilename,
				  NULL);

#ifdef DEBUG
      printf("In ReadLibraryContents, about to execute command %s\n", command);
#endif

      /* This uses a pipe to execute a shell script which provides the names of
       * all M4 libs and footprints.  The results are placed in resultFP.
       */
      if (command && *command && (resultFP = popen (command, "r")) == NULL)
	{
	  PopenErrorMessage (command);
	}

      /* the M4 library contents are separated by colons;
       * template : package : name : description
       */
      while (resultFP != NULL && fgets (inputline, MAX_LIBRARY_LINE_LENGTH, resultFP))
	{
	  size_t len = strlen (inputline);

	  /* check for maximum linelength */
	  if (len)
	    {
	      len--;
	      if (inputline[len] != '\n')
		Message
		  ("linelength (%i) exceeded; following characters will be ignored\n",
		   MAX_LIBRARY_LINE_LENGTH);
	      else
		inputline[len] = '\0';
	    }

	  /* if the line defines a menu */
	  if (!strncmp (inputline, "TYPE=", 5))
	    {
	      menu = GetLibraryMenuMemory (&Library);
	      menu->Name = strdup (UNKNOWN (&inputline[5]));
	      menu->directory = strdup (Settings.LibraryFilename);
	    }
	  else
	    {
	      /* allocate a new menu entry if not already done */
	      if (!menu)
		{
		  menu = GetLibraryMenuMemory (&Library);
		  menu->Name = strdup (UNKNOWN ((char *) NULL));
		  menu->directory = strdup (Settings.LibraryFilename);
		}
	      entry = GetLibraryEntryMemory (menu);
	      entry->AllocatedMemory = strdup (inputline);

	      /* now break the line into pieces separated by colons */
	      if ((entry->Template = strtok (entry->AllocatedMemory, ":")) !=
		  NULL)
		if ((entry->Package = strtok (NULL, ":")) != NULL)
		  if ((entry->Value = strtok (NULL, ":")) != NULL)
		    entry->Description = strtok (NULL, ":");

	      /* create the list entry */
	      len = strlen (EMPTY (entry->Value)) +
		strlen (EMPTY (entry->Description)) + 4;
	      entry->ListEntry = (char *)calloc (len, sizeof (char));
	      sprintf (entry->ListEntry,
		       "%s, %s", EMPTY (entry->Value),
		       EMPTY (entry->Description));
	    }
	}
      if (resultFP != NULL)
	pclose (resultFP);
    }

  /* Now after reading in the M4 libs, call a function to
   * read the newlib footprint libraries.  Then sort the whole
   * library.
   */
  if (ParseLibraryTree () > 0 || resultFP != NULL)
    {
      sort_library (&Library);
      return 0;
    }
  
  return (1);
}

#define BLANK(x) ((x) == ' ' || (x) == '\t' || (x) == '\n' \
		|| (x) == '\0')

/*!
 * \brief Read in a netlist and store it in the netlist menu.
 */
int
ReadNetlist (char *filename)
{
  static char *command = NULL;
  char inputline[MAX_NETLIST_LINE_LENGTH + 1];
  char temp[MAX_NETLIST_LINE_LENGTH + 1];
  FILE *fp;
  LibraryMenuType *menu = NULL;
  LibraryEntryType *entry;
  int i, j, lines, kind;
  bool continued;
  bool used_popen = false;
  int retval = 0;

  if (!filename)
    return 1;			/* nothing to do */

  Message (_("Importing PCB netlist %s\n"), filename);

  if (EMPTY_STRING_P (Settings.RatCommand))
    {
      fp = fopen (filename, "r");
      if (!fp)
	{
	  Message("Cannot open %s for reading", filename);
	  return 1;
	}
    }
  else
    {
      used_popen = true;
      free (command);
      command = EvaluateFilename (Settings.RatCommand,
				  Settings.RatPath, filename, NULL);

      /* open pipe to stdout of command */
      if (*command == '\0' || (fp = popen (command, "r")) == NULL)
	{
	  PopenErrorMessage (command);
	  return 1;
	}
    }
  lines = 0;
  /* kind = 0  is net name
   * kind = 1  is route style name
   * kind = 2  is connection
   */
  kind = 0;
  while (fgets (inputline, MAX_NETLIST_LINE_LENGTH, fp))
    {
      size_t len = strlen (inputline);
      /* check for maximum length line */
      if (len)
	{
	  if (inputline[--len] != '\n')
	    Message (_("Line length (%i) exceeded in netlist file.\n"
		       "additional characters will be ignored.\n"),
		     MAX_NETLIST_LINE_LENGTH);
	  else
	    inputline[len] = '\0';
	}
      continued = (inputline[len - 1] == '\\') ? true : false;
      if (continued)
	inputline[len - 1] = '\0';
      lines++;
      i = 0;
      while (inputline[i] != '\0')
	{
	  j = 0;
	  /* skip leading blanks */
	  while (inputline[i] != '\0' && BLANK (inputline[i]))
	    i++;
	  if (kind == 0)
	    {
	      /* add two spaces for included/unincluded */
	      temp[j++] = ' ';
	      temp[j++] = ' ';
	    }
	  while (!BLANK (inputline[i]))
	    temp[j++] = inputline[i++];
	  temp[j] = '\0';
	  while (inputline[i] != '\0' && BLANK (inputline[i]))
	    i++;
	  if (kind == 0)
	    {
	      menu = GetLibraryMenuMemory (&PCB->NetlistLib);
	      menu->Name = strdup (temp);
	      menu->flag = 1;
	      kind++;
	    }
	  else
	    {
	      if (kind == 1 && strchr (temp, '-') == NULL)
		{
		  kind++;
		  menu->Style = strdup (temp);
		}
	      else
		{
		  entry = GetLibraryEntryMemory (menu);
		  entry->ListEntry = strdup (temp);
		}
	    }
	}
      if (!continued)
	kind = 0;
    }
  if (!lines)
    {
      Message (_("Empty netlist file!\n"));
      retval = 1;
    }
  if (used_popen)
    pclose (fp);
  else
    fclose (fp);
  sort_netlist ();
  return retval;
}

static int ReadEdifNetlist (char *filename);

int ImportNetlist (char *filename)
{
  FILE *fp;
  char buf[16];
  int i;
  char* p;
  

  if (!filename) return (1);			/* nothing to do */
  fp = fopen (filename, "r");
  if (!fp) return (1);			/* bad filename */
  i = fread (buf, 1, sizeof(buf)-1, fp);
  fclose(fp);
  buf[i] = '\0';
  p=buf;
  while ( *p )
  {
      *p = tolower ((int) *p);
      p++;
  }
  p = strstr (buf, "edif");
  if (!p) return ReadNetlist (filename);
  else return ReadEdifNetlist (filename);
}

static int ReadEdifNetlist (char *filename)
{
    Message (_("Importing edif netlist %s\n"), filename);
    ParseEDIF(filename, NULL);
    
    return 0;
}


/****************************************************************************************************/

static int n_formats = 0;
static HID_Format **all_formats = 0;

#define FILEFORMAT_IDX_VALID(x) (x >= 0 && x < n_formats)
#define FILEFORMAT_INVALID_IDX (-1)

/*!
* \brief Provides format data for load/save dialogs;
* It is used for enumerations with idx starting from 0
*
* \param [in] idx - format index
* \param [in] capability - requested capability: HID_FFORMAT_LOADABLE or HID_FFORMAT_SAVEABLE
* \param [out] id - string ID; if NULL, format does not implement required operation
* \param [out] name - description
* \param [out] mime - mimetype
* \param [out] patterns - array of pattern definitions
* \return true if returned data are valid, false if index is out of range
*/
bool
hid_get_file_format (int idx, int capability, char **id, char **name, char **mime, char ***patterns)
{
    /* end of iteration test... */
    if ( ! FILEFORMAT_IDX_VALID (idx))
        return false;

    if ( ! hid_file_format_capable_by_idx (idx, capability))
      {
	*id=NULL;
	return true;
      }

    *id = all_formats[idx]->id;
    *name = all_formats[idx]->description;
    *mime = all_formats[idx]->mimetype;
    *patterns = all_formats[idx]->patterns;

    return true;
}

/*!
* \brief Function to find the format string ID by it's description.
* Function is used in Load/Save dialogs.
*
* \param [in] desc - description of the format
* \return pointer for format ID; NULL if such format does not exist
*/
char *
hid_get_format_id_by_desc (char *desc)
{
  int i;

  for (i = 0; i < n_formats; i++)
    {
      if (strcmp (all_formats[i]->description, desc) == 0)
        return all_formats[i]->id;
    }
  return NULL;
}


/*!
* \brief Function to find the format string ID by index.
*
* \param [in] idx - index of the format
* \return pointer for format ID; NULL if index is out of range
*/
char *
hid_get_format_id_by_idx (int idx)
{
  if (FILEFORMAT_IDX_VALID (idx))
    {
        return all_formats[idx]->id;
    }
  return NULL;
}

/*!
* \brief Function to find the format index by string ID.
*
* \param [in] id - string ID of the format
* \return index of the format, FILEFORMAT_INVALID_IDX if no such format exist (should not happen)
*/
int
hid_get_format_idx_by_id (char *id)
{
  int i;

  for (i = 0; i < n_formats; i++)
    {
      if (strcmp (all_formats[i]->id, id) == 0)
        return i;
    }
  return FILEFORMAT_INVALID_IDX;
}


/*!
* \brief Function to find the format suitable to act as default format:
* such format have to provide both load and save functions.
* Function is called as failback when no default format is specified
* and to check that such format exists at beginning of program
*
* \return index of the first format supporting both load & save functions.
* If no such format exists, program exits immediately.
*/

int
hid_find_full_format_idx ()
{
  int i;

  for (i = 0; i < n_formats; i++)
    {
      if (all_formats[i]->load_function != NULL
          && all_formats[i]->save_function != NULL)
        return i;
    }

  /* No format suitable to play role of default format found */
  fprintf (stderr, "No file format supporting both load and save operations found. Exiting.\n");
  exit (1);
}


/*!
* \brief Function to find the format with "default" flag.
*
* \return index first format format flagged as default; if none exists, returns FILEFORMAT_INVALID_IDX
*/

int
hid_get_flagged_default_format_idx ()
{
  int i;

  for (i = 0; i < n_formats; i++)
    {
      if (all_formats[i]->default_format)
        return i;
    }

  return FILEFORMAT_INVALID_IDX;
}

/*!
* \brief Function to find the default format string ID.
*
* \return pointer to default format ID; if none exists, returns first format
* with load/save capability
*/
char *
hid_get_default_format_id ()
{
  int i;

  i = hid_get_flagged_default_format_idx ();

  if (FILEFORMAT_IDX_VALID (i))
    return all_formats[i]->id;

  return all_formats[hid_find_full_format_idx ()]->id;
}

/*!
* \brief Function to find the default format index
*
* \return index of default format; if none exists, returns first format
* with load/save capability
*/
int
hid_get_default_format_idx ()
{
  int i;

  i = hid_get_flagged_default_format_idx ();

  if (FILEFORMAT_IDX_VALID (i))
    return i;

  return hid_find_full_format_idx ();
}


/*!
* \brief Check if specified format has capability
*
* \param [in] idx - index of format
* \param [in] capabilities - requested capabilities as combination of capability constatnts
* \return true, if format implements *all* requested capabilities
*/
bool
hid_file_format_capable_by_idx (int idx, unsigned int capabilities)
{

  if (FILEFORMAT_IDX_VALID (idx))
    {
    if (capabilities & HID_FFORMAT_LOADABLE)
      {
        if (all_formats[idx]->load_function == NULL)
	  return false;
      }
    else if (capabilities & HID_FFORMAT_SAVEABLE)
      {
        if (all_formats[idx]->save_function == NULL)
	  return false;
      }
      /* no requested capability failed the check*/
      return true;
    }

  return false;
}

/*!
* \brief Check if specified format has all requested capabilities
*
* \param [in] id - pointer to format ID
* \param [in] capabilities - requested capabilities as combination of capability constatnts
* \return true, if format implements *all* requested capabilities
*/
bool
hid_file_format_capable (char *id, unsigned int capabilities)
{

  return hid_file_format_capable_by_idx (hid_get_format_idx_by_id (id), capabilities);
}


/*!
* \brief Function to save PCB layout. The heart of modular format system
*
* \param [in] pcb - pointer to PCB layout
* \param [in] filename - current layout filename
* \param [in] fileformat - current format
* \return 0 - no error, !=0 - error occured
*/
int
SavePCBWithFormat (PCBType *pcb, char *filename, char *fileformat)
{
  int i;
  int result;

  Message (_("Saving file %s as %s\n"), filename, fileformat);

  i = hid_get_format_idx_by_id (fileformat);

  if (hid_file_format_capable_by_idx (i, HID_FFORMAT_SAVEABLE))
    {
      if ((all_formats[i]->check_version != NULL) &&  (!(*all_formats[i]->check_version)(PCB_FILE_VERSION, PCBFileVersionNeeded ())))
        {
	  gui->report_dialog (_("Incompatible file format"), _("The selected file format does not support current data structures"));
	  Message (_("Selected format \"%s\" does not support data structures version %ul:\n"), fileformat, PCB_FILE_VERSION);
	  return 1;
	}
      if (gui->notify_save_pcb != NULL)
        gui->notify_save_pcb (filename, false);

      result = (*all_formats[i]->save_function)(pcb, filename);

      if (gui->notify_save_pcb != NULL)
        gui->notify_save_pcb (filename, true);

      return result;
    }

  Message (_("INTERNAL ERROR: No suitable module for format \"%s\"\n"), fileformat);
  return 1;
}

/*!
* \brief Function to load PCB layout. The heart of modular format system
*
* \param [inout] pcb - pointer to PCB layout
* \param [in] filename - current layout filename
* \param [in] fileformat - current format
* \param [out] fileformat - format of newly loaded file
* \return 0 - no error, !=0 - error occured
*/
int
LoadPCBWithFormat (PCBType **pcb, char *filename, char *fileformat, char **new_format)
{
  int i;
  int result;

  *pcb = CreateNewPCB ();
  /* mark the default font invalid to know if the file has one */
  (*pcb)->Font.Valid = false;

  if (fileformat != NULL)
    {
      Message (_("Loading file %s as %s\n"), filename, fileformat);

      i = hid_get_format_idx_by_id (fileformat);

      if (hid_file_format_capable_by_idx (i, HID_FFORMAT_LOADABLE))
        {
	  *new_format = all_formats[i]->id;
          return (*all_formats[i]->load_function) (*pcb, filename);
	}
      else
        Message (_("INTERNAL ERROR: No suitable module for format \"%s\"\n"), fileformat);
    }
  else
    {
      Message (_("Loading file %s with autodetection.\n"), filename);
      for (i = 0; i < n_formats ; i++ )
        {
	  if (hid_file_format_capable_by_idx (i, HID_FFORMAT_LOADABLE))
	    {
              Message (_(" Probing format %s\n"), all_formats[i]->id);
              if (all_formats[i]->check_function != NULL )
                {
	          /* If check function is available and return value is OK (0), the file is loaded and no other formats are tested */
                  if ((*all_formats[i]->check_function) (filename) == 0)
	            {
		      *new_format = all_formats[i]->id;
		      return (*all_formats[i]->load_function) (*pcb, filename);
		    }
	        }
	      else
	        {
	          /* If check function is not available, the file is loaded; if fail, next format is tried */
	          result = (*all_formats[i]->load_function) (*pcb, filename);
		  if (result == 0 )
		    {
		      *new_format = all_formats[i]->id;
		      return result;
		    }
		  else
		    {
		    /* Cleanup after unsuccessful load */
		      RemovePCB (*pcb);
                      *pcb = CreateNewPCB ();
                      /* mark the default font invalid to know if the file has one */
                      (*pcb)->Font.Valid = false;
		    }
	        }
            }
	}
    }

   Message (_("No suitable module found for file \"%s\"\n"), filename);
   return 1;
}

/*!
* \brief Register new format. Several checks are performed:
* - warns about duplicity of default formats
* - refuses to set format without bothl load/save capabilities as default format
*
* \param [in] a - array of format definitions
* \param [in] n - # of formats in the array
*/
void
hid_register_formats (HID_Format * a, int n)
{
  int i, count = 0;
  bool have_default = false;

  all_formats = (HID_Format **)realloc (all_formats,
                         (n_formats + n) * sizeof (HID_Format*));

  /* look for existing default format */
  have_default = FILEFORMAT_IDX_VALID (hid_get_flagged_default_format_idx ());

  for (i = 0; i < n; i++)
    {
      all_formats[n_formats + count] = a + i;
      if (all_formats[n_formats + count]->default_format)
        {
	  if (have_default)
	    {
	      fprintf (stderr, "Cannot set format \"%s\" as default format; default format already exists\n", all_formats[n_formats + count]->id);
	      all_formats[n_formats + count]->default_format = false;
	    }
	  else if (all_formats[n_formats + count]->load_function == NULL
	               || all_formats[n_formats + count]->save_function == NULL)
	    {
	      fprintf (stderr, "Cannot set format \"%s\" as default format because does not implement both load & save functions\n", all_formats[n_formats + count]->id);
	      all_formats[n_formats + count]->default_format = false;
	    }
	  else
            have_default = true;
	}
      count++;
    }
  n_formats += count;
}

#undef FILEFORMAT_IDX_VALID
#undef FILEFORMAT_INVALID_IDX


/****************************************************************************************************/

int
SavePCB2 (void *p_pcb, char *filename)
{
  return SavePCB (filename);
}

int
ParsePCB2 (void *p_pcb, char *filename)
{
  PCBType *pcb = (PCBType *)p_pcb;

  return ParsePCB (pcb, filename);
}

int
CheckPCB (char *filename)
{
  FILE *f;
  int i;
  char buf[512];

  f=fopen(filename, "r");

  if (!f)
    return 1;

  for ( i = 0; i < 10; i++ )
    {
      fgets(buf,sizeof(buf),f);
      if (strstr(buf,"FileVersion[") != 0)
	{
	  fclose(f);
	  return 0;
	}
    }
  fclose(f);
  return 1;
}

#define PCB_FILE_VERSION_IMPLEMENTED 20170218

bool
CheckPCBVersion (unsigned long current, unsigned long minimal)
{
  return (PCB_FILE_VERSION_IMPLEMENTED >= minimal);
}

static char *pcb_format_list_patterns[]={"*.pcb", "*.PCB", 0};

static HID_Format pcb_format_list[]={
  {"pcb","Legacy PCB", pcb_format_list_patterns, "application/x-pcb-layout", false, CheckPCBVersion, CheckPCB, ParsePCB2, SavePCB2/*,0,0*/},
};

REGISTER_FORMATS (pcb_format_list)

