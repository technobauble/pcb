/*!
 * \file src/hid/ipcd356/ipcd356.c
 *
 * \brief IPC-D-356 Netlist export.
 *
 * \author Copyright (C) 2012 Jerome Marchand (Jerome.Marchand@gmail.com)
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
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
 *
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *
 * Thomas.Nau@rz.uni-ulm.de
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "data.h"
#include "config.h"
#include "global.h"
#include "rats.h"
#include "error.h"
#include "find.h"
#include "misc.h"
#include "pcb-printf.h"

#include "hid.h"
#include "hid/common/hidnogui.h"
#include "../hidint.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

static HID_Attribute IPCD356_options[] =
{
/* %start-doc options "95 IPC-D-356 Netlist Export"
@ftable @code
@item --netlist-file <string>
Name of the IPC-D-356 Netlist output file.
Parameter @code{<string>} can include a path.
@end ftable
%end-doc
*/
  {
    "netlistfile",
    "Name of the IPC-D-356 Netlist output file",
    HID_String,
    0, 0, {0, 0, 0}, 0, 0
  },
#define HA_IPCD356_filename 0
};

#define NUM_OPTIONS (sizeof(IPCD356_options)/sizeof(IPCD356_options[0]))

static HID_Attr_Val IPCD356_values[NUM_OPTIONS];

const char *IPCD356_filename;

typedef struct
{
  char NName[11];
  char NetName[256];
} IPCD356_Alias;

typedef struct
{
  int AliasN; /*!< Number of entries. */
  IPCD356_Alias *Alias;
} IPCD356_AliasList;

void IPCD356_WriteNet (FILE *, char *);
void IPCD356_WriteHeader (FILE *);
void IPCD356_End (FILE *);
int IPCD356_Netlist (void);
int IPCD356_WriteAliases (FILE *, IPCD356_AliasList *);
void ResetVisitPinsViasAndPads (void);
void CheckNetLength (char *, IPCD356_AliasList *);
IPCD356_AliasList *CreateAliasList (void);
IPCD356_AliasList *AddAliasToList (IPCD356_AliasList *);
int IPCD356_SanityCheck (void);

static HID_Attribute *
IPCD356_get_export_options (int *n)
{
  static char *last_IPCD356_filename = 0;

  if (PCB)
    {
      derive_default_filename (PCB->Filename, &IPCD356_options[HA_IPCD356_filename], ".net", &last_IPCD356_filename);
    }

  if (n)
    *n = NUM_OPTIONS;

  return IPCD356_options;
}

/*!
 * \brief Writes the IPC-D-356 Header to the file provided.
 *
 * The JOB name is the PCB Name (if set), otherwise the filename
 * (including the path) is used.
 *
 * The units used for the netlist depends on what is set (mils or mm).
 */
void
IPCD356_WriteHeader (FILE * fd)
{
  time_t currenttime;
  char utcTime[64];
  const char *fmt = "%c UTC";

  currenttime = time (NULL);
  strftime (utcTime, sizeof utcTime, fmt, gmtime (&currenttime));

  fprintf (fd,
    "C  IPC-D-356 Netlist generated by gEDA PCB " VERSION "\nC  \n");
  fprintf (fd, "C  File created on %s\nC  \n", utcTime);
  if (PCB->Name == NULL)
    {
      fprintf (fd, "P  JOB   %s\n", PCB->Filename); /* Use the file name if the PCB name in not set. */
    }
  else
    {
      fprintf (fd, "P  JOB   %s\n", PCB->Name);
    }
  fprintf (fd, "P  CODE  00\n");
  if (strcmp (Settings.grid_unit->suffix,"mil") == 0) /* Use whatever unit is currently in use (mil or mm). */
    {
      fprintf (fd, "P  UNITS CUST 0\n");
    }
  else
    {
      fprintf (fd, "P  UNITS CUST 1\n");
    }
  fprintf (fd, "P  DIM   N\n");
  fprintf (fd, "P  VER   IPC-D-356\n");
  fprintf (fd, "P  IMAGE PRIMARY\nC  \n");
}


/*!
 * \brief Writes a net to the file provided.
 *
 * The net name is passed through the "net" and should be 14 characters
 * max.\n
 * The function scans through pads, pins and vias  and looks for the
 * \c FOUNDFLAG.\n
 * Once the object has been added to the net list the \c VISITFLAG is
 * set on that object.
 *
 * \todo 1) The bottom layer is always written as layer #2 (A02).\n
 *          It could output the actual layer number (example: A06 on a
 *          6 layer board).\n
 *          But I could not find an easy way to do this...
 *
 * \todo 2) Objects with mutiple connections could have the "M"
 *          (column 32) field written to indicate a Mid Net Point.
 */
void
IPCD356_WriteNet (FILE * fd, char *net)
{
  int padx, pady, tmp;

  ELEMENT_LOOP (PCB->Data);
  PAD_LOOP (element);
  if (TEST_FLAG (FOUNDFLAG, pad))
    {
      fprintf (fd, "327%-17.14s", net); /* Net Name. */
      fprintf (fd, "%-6.6s", element->Name[1].TextString); /* Refdes. */
      fprintf (fd, "-%-4.4s", pad->Number); /* pin number. */
      fprintf (fd, " "); /*! \todo Midpoint indicator (M). */
      fprintf (fd, "      "); /* Drilled hole Id (blank for pads). */
      if (TEST_FLAG (ONSOLDERFLAG, pad) == true)
        {
          fprintf (fd, "A02"); /*! \todo Put actual layer # for bottom side. */
        }
      else
        {
          fprintf (fd, "A01"); /* Top side. */
        }
      padx = (pad->Point1.X + pad->Point2.X) / 2; /* X location in PCB units. */
      pady = (PCB->MaxHeight - ((pad->Point1.Y + pad->Point2.Y) / 2)); /* Y location in PCB units. */

      if (strcmp (Settings.grid_unit->suffix, "mil") == 0)
        {
          padx = padx / 2540; /* X location in 0.0001". */
          pady = pady / 2540; /* Y location in 0.0001". */
        }
      else
        {
          padx = padx / 1000; /* X location in 0.001 mm. */
          pady = pady / 1000; /* Y location in 0.001 mm. */
        }
      fprintf (fd, "X%+6.6d", padx); /* X Pad center. */
      fprintf (fd, "Y%+6.6d", pady); /* Y pad center. */

      padx = (pad->Thickness + (pad->Point2.X - pad->Point1.X)); /* Pad dimension X in PCB units. */
      pady = (pad->Thickness + (pad->Point2.Y - pad->Point1.Y)); /* Pad dimension Y in PCB units. */

      if (strcmp(Settings.grid_unit->suffix, "mil") == 0)
        {
          padx = padx / 2540; /* X location in 0.0001". */
          pady = pady / 2540; /* Y location in 0.0001". */
        }
      else
        {
          padx = padx / 1000;	// X location in 0.001mm
          pady = pady / 1000;	// Y location in 0.001mm
        }

      fprintf (fd, "X%4.4d", padx);
      fprintf (fd, "Y%4.4d", pady);
      fprintf (fd, "R000"); /* Rotation (0 degrees). */
      fprintf (fd, " "); /* Column 72 should be left blank. */
      if (pad->Mask > 0)    
        {
          if (TEST_FLAG (ONSOLDERFLAG, pad) == true)
            {
              fprintf(fd, "S2"); /* Soldermask on bottom side. */
            }
          else
            {
              fprintf(fd, "S1"); /* SolderMask on top side. */
            }
        }
      else
        {
          fprintf(fd, "S3"); /* No soldermask. */
        }
      fprintf (fd, "      "); /* Padding. */
      fprintf (fd, "\n");
      SET_FLAG (VISITFLAG, pad);
    }

  END_LOOP; /* Pad. */
  PIN_LOOP (element);
  if (TEST_FLAG (FOUNDFLAG, pin))
    {
      if (TEST_FLAG (HOLEFLAG, pin)) /* Non plated? */
        {
          fprintf (fd, "367%-17.14s", net); /* Net Name. */
        }
      else
        {
          fprintf (fd, "317%-17.14s", net); /* Net Name. */
        }
      fprintf (fd, "%-6.6s", element->Name[1].TextString); /* Refdes. */
      fprintf (fd, "-%-4.4s", pin->Number); /* Pin number. */
      fprintf (fd, " "); /*! \todo Midpoint indicator (M). */
      tmp = pin->DrillingHole;
      if (strcmp (Settings.grid_unit->suffix, "mil") == 0)
        {
          tmp = tmp / 2540; /* 0.0001". */
        }
      else
        {
          tmp = tmp / 1000; /* 0.001 mm. */
        }

      if (TEST_FLAG (HOLEFLAG, pin))
        {
          fprintf (fd, "D%-4.4dU", tmp); /* Unplated Drilled hole Id. */
        }
      else
        {
          fprintf (fd, "D%-4.4dP", tmp); /* Plated drill hole. */
        }
      fprintf (fd, "A00"); /* Accessible from both sides. */
      padx = pin->X; /* X location in PCB units. */
      pady = (PCB->MaxHeight - pin->Y); /* Y location in PCB units.*/

      if (strcmp (Settings.grid_unit->suffix, "mil") == 0)
        {
          padx = padx / 2540; /* X location in 0.0001". */
          pady = pady / 2540; /* Y location in 0.0001". */
        }
      else
        {
          padx = padx / 1000; /* X location in 0.001 mm. */
          pady = pady / 1000; /* Y location in 0.001 mm. */
        }

      fprintf (fd, "X%+6.6d", padx); /* X Pad center. */
      fprintf (fd, "Y%+6.6d", pady); /* Y pad center. */

      padx = pin->Thickness;

      if (strcmp (Settings.grid_unit->suffix, "mil") == 0)
        {
          padx = padx / 2540; /* X location in 0.0001". */
        }
      else
        {
          padx = padx / 1000; /* X location in 0.001 mm. */
        }

      fprintf (fd, "X%4.4d", padx); /* Pad dimension X. */
      if (TEST_FLAG (SQUAREFLAG, pin))
        {
          fprintf (fd, "Y%4.4d", padx); /* Pad dimension Y. */
        }
      else
        {
          fprintf (fd, "Y0000"); /*  Y is 0 for round pins. */
        }
      fprintf (fd, "R000"); /* Rotation (0 degrees). */
      fprintf (fd, " "); /* Column 72 should be left blank.*/
      if (pin->Mask > 0)    
        {
          fprintf(fd, "S0"); /* No Soldermask. */
        }
      else
        {
          fprintf(fd, "S3"); /* Soldermask on both sides. */
        }
      fprintf (fd, "      "); /* Padding. */

      fprintf (fd, "\n");

      SET_FLAG (VISITFLAG, pin);

    }

  END_LOOP; /* Pin. */
  END_LOOP; /* Element */

  VIA_LOOP (PCB->Data);
  if (TEST_FLAG (FOUNDFLAG, via))
    {
      if (TEST_FLAG (HOLEFLAG, via)) /* Non plated ? */
        {
          fprintf (fd, "367%-17.14s", net); /* Net Name. */
        }
      else
        {
          fprintf (fd, "317%-17.14s", net); /* Net Name. */
        }
      fprintf (fd, "VIA   "); /* Refdes. */
      fprintf (fd, "-    "); /* Pin number. */
      fprintf (fd, " "); /*! \todo Midpoint indicator (M). */
      tmp = via->DrillingHole;	
      if (strcmp (Settings.grid_unit->suffix, "mil") == 0)
        {
          tmp = tmp / 2540; /* 0.0001". */
        }
      else
        {
          tmp = tmp / 1000; /* 0.001 mm. */
        }

      if (TEST_FLAG (HOLEFLAG, via))
        {
          fprintf (fd, "D%-4.4dU", tmp); /* Unplated Drilled hole Id. */
        }
      else
        {
          fprintf (fd, "D%-4.4dP", tmp); /* Plated drill hole. */
        }
      fprintf (fd, "A00"); /* Accessible from both sides. */
      padx = via->X; /* X location in PCB units. */
      pady = (PCB->MaxHeight - via->Y); /* Y location in PCB units. */

      if (strcmp (Settings.grid_unit->suffix, "mil") == 0)
        {
          padx = padx / 2540; /* X location in 0.0001". */
          pady = pady / 2540; /* Y location in 0.0001". */
        }
      else
        {
          padx = padx / 1000; /* X location in 0.001 mm. */
          pady = pady / 1000; /* Y location in 0.001 mm. */
        }

      fprintf (fd, "X%+6.6d", padx); /* X Pad center. */
      fprintf (fd, "Y%+6.6d", pady); /* Y pad center. */

      padx = via->Thickness;
      
      if (strcmp (Settings.grid_unit->suffix, "mil") == 0)
        {
          padx = padx / 2540; /* X location in 0.0001". */
        }
      else
        {
          padx = padx / 1000; /* X location in 0.001 mm. */
        }

      fprintf (fd, "X%4.4d", padx); /* Pad dimension X. */
      fprintf (fd, "Y0000"); /* Y is 0 for round pins (vias always round?). */
      fprintf (fd, "R000"); /* Rotation (0 degrees). */
      fprintf (fd, " "); /* Column 72 should be left blank. */
      if (via->Mask > 0)    
        {
          fprintf(fd, "S0"); /* No Soldermask. */
        }
      else
        {
          fprintf(fd, "S3"); /* Soldermask on both sides. */
        }
      fprintf (fd, "      "); /* Padding. */
      fprintf (fd, "\n");
      SET_FLAG (VISITFLAG, via);
    }

  END_LOOP; /* Via. */
}


/*!
 * \brief The main IPC-D-356 function.
 *
 * Gets the filename for the netlist from the dialog.
 */
int
IPCD356_Netlist (void)
{
  FILE *fp;
  char nodename[256];
  char net[256];
  LibraryMenuType *netname;
  IPCD356_AliasList * aliaslist;

  if (IPCD356_SanityCheck()) /* Check for invalid names + numbers. */
    {
      Message ("Aborting.\n");
      return(1);
    }

  sprintf (net, "%s.ipc", PCB->Name);
  if (IPCD356_filename == NULL)
    return 1;

  fp = fopen (IPCD356_filename, "w+");
  if (fp == NULL)
    {
      Message ("error opening %s\n", IPCD356_filename);
      return 1;
    }
/*   free (IPCD356_filename); */


  IPCD356_WriteHeader (fp);

  aliaslist = CreateAliasList ();
  if (aliaslist == NULL)
    {
      Message ("Error Aloccating memory for IPC-D-356 AliasList\n");
      return 1;
    }

  if (IPCD356_WriteAliases (fp, aliaslist))
    {
      Message ("Error Writing IPC-D-356 AliasList\n");
      return 1;
    }


  ELEMENT_LOOP (PCB->Data);
  PIN_LOOP (element);
  if (!TEST_FLAG (VISITFLAG, pin))
    {
      ClearFlagOnLinesAndPolygons (true, FOUNDFLAG);
      ClearFlagOnPinsViasAndPads (true, FOUNDFLAG);
      LookupConnectionByPin (PIN_TYPE, pin);
      sprintf (nodename, "%s-%s", element->Name[1].TextString, pin->Number);
      netname = netnode_to_netname (nodename);
/*      Message("Netname: %s\n", netname->Name +2); */
      if (netname)
        {
          strcpy (net, &netname->Name[2]);
          CheckNetLength (net, aliaslist);
        }
      else
        {
          strcpy (net, "N/C");
        }
      IPCD356_WriteNet (fp, net);
    }
  END_LOOP; /* Pin. */
  PAD_LOOP (element);
  if (!TEST_FLAG (VISITFLAG, pad))
    {
      ClearFlagOnLinesAndPolygons (true, FOUNDFLAG);
      ClearFlagOnPinsViasAndPads (true, FOUNDFLAG);
      LookupConnectionByPin (PAD_TYPE, pad);
      sprintf (nodename, "%s-%s", element->Name[1].TextString, pad->Number);
      netname = netnode_to_netname (nodename);
/*      Message("Netname: %s\n", netname->Name +2); */
      if (netname)
        {
          strcpy (net, &netname->Name[2]);
          CheckNetLength (net, aliaslist);
        }
      else
        {
          strcpy (net, "N/C");
        }
      IPCD356_WriteNet (fp, net);
    }
  END_LOOP; /* Pad. */

  END_LOOP; /* Element. */

  VIA_LOOP (PCB->Data);
  if (!TEST_FLAG (VISITFLAG, via))
    {
      ClearFlagOnLinesAndPolygons (true, FOUNDFLAG);
      ClearFlagOnPinsViasAndPads (true, FOUNDFLAG);
      LookupConnectionByPin (PIN_TYPE, via);
      strcpy (net, "N/C");
      IPCD356_WriteNet (fp, net);
    }
  END_LOOP; /* Via. */

  IPCD356_End (fp);
  fclose (fp);
  free (aliaslist);
  ResetVisitPinsViasAndPads ();
  ClearFlagOnLinesAndPolygons (true, FOUNDFLAG);
  ClearFlagOnPinsViasAndPads (true, FOUNDFLAG);
  return 0;
}

void
IPCD356_End (FILE * fd)
{
  fprintf (fd, "999\n");
}

void
ResetVisitPinsViasAndPads ()
{
  VIA_LOOP (PCB->Data);
    CLEAR_FLAG (VISITFLAG, via);
  END_LOOP; /* Via. */
  ELEMENT_LOOP (PCB->Data);
    PIN_LOOP (element);
      CLEAR_FLAG (VISITFLAG, pin);
    END_LOOP; /* Pin. */
    PAD_LOOP (element);
      CLEAR_FLAG (VISITFLAG, pad);
    END_LOOP; /* Pad. */
  END_LOOP; /* Element. */
}

int
IPCD356_WriteAliases (FILE * fd, IPCD356_AliasList * aliaslist)
{
  int index;
  int i;

  index = 1;

  for (i = 0; i < PCB->NetlistLib.MenuN; i++)
    {
      if (strlen (PCB->NetlistLib.Menu[i].Name + 2) > 14)
        {
          if (index == 1)
            {
              fprintf (fd, "C  Netname Aliases Section\n");
            }
          aliaslist = AddAliasToList (aliaslist);
          if (aliaslist == NULL)
            {
              return 1;
            }
          sprintf (aliaslist->Alias[index].NName, "NNAME%-5.5d", index);
          strcpy (aliaslist->Alias[index].NetName, PCB->NetlistLib.Menu[i].Name + 2);

          fprintf (fd, "P  %s  %-58.58s\n", aliaslist->Alias[index].NName,
            aliaslist->Alias[index].NetName);
          index++;
        }
    }
  if (index > 1)
    {
      fprintf (fd, "C  End Netname Aliases Section\nC  \n");
    }
  return 0;
}

IPCD356_AliasList *
CreateAliasList ()
{
  IPCD356_AliasList * aliaslist;

  aliaslist = malloc (sizeof (IPCD356_AliasList)); /* Create an alias list. */
  aliaslist->AliasN = 0; /* Initialize Number of Alias. */
  return aliaslist;
}

IPCD356_AliasList *
AddAliasToList (IPCD356_AliasList * aliaslist)
{
  aliaslist->AliasN++;
  aliaslist->Alias = realloc (aliaslist->Alias,
    sizeof (IPCD356_Alias) * (aliaslist->AliasN + 1));
  if (aliaslist->Alias == NULL)
    {
      return NULL;
    }
  return aliaslist;
}

void
CheckNetLength (char *net, IPCD356_AliasList * aliaslist)
{
  int i;

  if (strlen (net) > 14)
    {
      for (i = 1; i <= aliaslist->AliasN; i++)
        {
          if (strcmp (net, aliaslist->Alias[i].NetName) == 0)
            {
              strcpy (net, aliaslist->Alias[i].NName);
            }
        }
    }
}

int
IPCD356_SanityCheck()
{
  ELEMENT_LOOP (PCB->Data);
    if (element->Name[1].TextString == '\0')
      {
        Message("Error: Found unnamed element. All elements need to be named to create an IPC-D-356 netlist.\n");
        return(1);
      }
  END_LOOP; /* Element. */
  return(0);
}

static void
IPCD356_do_export (HID_Attr_Val * options)
{
  int i;

  if (!options)
    {
      IPCD356_get_export_options (0);

      for (i = 0; i < NUM_OPTIONS; i++)
        IPCD356_values[i] = IPCD356_options[i].default_val;

      options = IPCD356_values;
    }

  IPCD356_filename = options[HA_IPCD356_filename].str_value;
  if (!IPCD356_filename)
    IPCD356_filename = "pcb-out.net";

  IPCD356_Netlist ();
}

static void
IPCD356_parse_arguments (int *argc, char ***argv)
{
  hid_register_attributes (IPCD356_options,
    sizeof (IPCD356_options) / sizeof (IPCD356_options[0]));
  hid_parse_command_line (argc, argv);
}

HID IPCD356_hid;

void
hid_ipcd356_init ()
{
  memset (&IPCD356_hid, 0, sizeof (HID));

  common_nogui_init (&IPCD356_hid);

  IPCD356_hid.struct_size         = sizeof (HID);
  IPCD356_hid.name                = "IPC-D-356";
  IPCD356_hid.description         = "Exports a IPC-D-356 Netlist";
  IPCD356_hid.exporter            = 1;

  IPCD356_hid.get_export_options  = IPCD356_get_export_options;
  IPCD356_hid.do_export           = IPCD356_do_export;
  IPCD356_hid.parse_arguments     = IPCD356_parse_arguments;

  hid_register_hid (&IPCD356_hid);

}

/* EOF */
