/*!
 * \file src/drc.c
 *
 * \brief DRC related structures and functions
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2018 Charles Parker <parker.charles@gmail.com>
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "global.h"

#include "data.h"
#include "draw.h"
#include "drc.h"
#include "error.h"
#include "find.h"
#include "misc.h"
#include "pcb_geometry.h"
#include "polygon.h"
#include "pcb-printf.h"
#include "undo.h"

DrcViolationType
*pcb_drc_violation_new (const char *title,
                        const char *explanation,
                        Coord x, Coord y,
                        Angle angle,
                        bool have_measured,
                        Coord measured_value,
                        Coord required_value,
                        int object_count,
                        long int *object_id_list,
                        int *object_type_list)
{
  DrcViolationType *violation = (DrcViolationType *)malloc (sizeof (DrcViolationType));

  violation->title = strdup (title);
  violation->explanation = strdup (explanation);
  violation->x = x;
  violation->y = y;
  violation->angle = angle;
  violation->have_measured = have_measured;
  violation->measured_value = measured_value;
  violation->required_value = required_value;
  violation->object_count = object_count;
  violation->object_id_list = object_id_list;
  violation->object_type_list = object_type_list;

  return violation;
}

void
pcb_drc_violation_free (DrcViolationType *violation)
{
  free (violation->title);
  free (violation->explanation);
  free (violation);
}

static GString *drc_dialog_message;

void
reset_drc_dialog_message(void)
{
  if (drc_dialog_message)
    g_string_free (drc_dialog_message, FALSE);
  drc_dialog_message = g_string_new ("");
  if (gui->drc_gui != NULL)
    {
      gui->drc_gui->reset_drc_dialog_message ();
    }
}

void
append_drc_dialog_message(const char *fmt, ...)
{
  gchar *new_str;
  va_list ap;
  va_start (ap, fmt);
  new_str = pcb_vprintf (fmt, ap);
  g_string_append (drc_dialog_message, new_str);
  va_end (ap);
  g_free (new_str);
}

void
append_drc_violation (DrcViolationType *violation)
{
  if (gui->drc_gui != NULL)
    {
      gui->drc_gui->append_drc_violation (violation);
    }
  else
    {
      /* Fallback to formatting the violation message as text */
      append_drc_dialog_message ("%s\n", violation->title);
      append_drc_dialog_message (_("%m+near %$mD\n"),
                                 Settings.grid_unit->allow,
                                 violation->x, violation->y);
      GotoError ();
    }

  if (gui->drc_gui == NULL || gui->drc_gui->log_drc_violations )
    {
      Message (_("WARNING!  Design Rule error - %s\n"), violation->title);
      Message (_("%m+near location %$mD\n"),
               Settings.grid_unit->allow,
               violation->x, violation->y);
    }
}

#define DRC_CONTINUE _("Press Next to continue DRC checking")
#define DRC_NEXT _("Next")
#define DRC_CANCEL _("Cancel")

/*!
 * \brief Message when asked about continuing DRC checks after next 
 * violation is found.
 */
int
throw_drc_dialog(void)
{
  int r;

  if (gui->drc_gui != NULL)
    {
      r = gui->drc_gui->throw_drc_dialog ();
    }
  else
    {
      /* Fallback to formatting the violation message as text */
      append_drc_dialog_message (DRC_CONTINUE);
      r = gui->confirm_dialog (drc_dialog_message->str, DRC_CANCEL, DRC_NEXT);
      reset_drc_dialog_message();
    }
  return r;
}

/*!
 * \brief Build a list of the of offending items by ID.
 *
 * (Currently just "thing").
 */
static void
BuildObjectList (int *object_count, long int **object_id_list, int **object_type_list)
{
  void *thing_ptr1, *thing_ptr2, *thing_ptr3;
  int thing_type = GetThing(&thing_ptr1, &thing_ptr2, &thing_ptr3);

  *object_count = 0;
  *object_id_list = NULL;
  *object_type_list = NULL;

  switch (thing_type)
    {
    case LINE_TYPE:
    case ARC_TYPE:
    case POLYGON_TYPE:
    case PIN_TYPE:
    case VIA_TYPE:
    case PAD_TYPE:
    case ELEMENT_TYPE:
    case RATLINE_TYPE:
      *object_count = 1;
      *object_id_list = (long int *)malloc (sizeof (long int));
      *object_type_list = (int *)malloc (sizeof (int));
      **object_id_list = ((AnyObjectType *)thing_ptr3)->ID;
      **object_type_list = thing_type;
      return;

    default:
      fprintf (stderr,
	       _("Internal error in BuildObjectList: unknown object type %i\n"),
	       thing_type);
    }
}

struct drc_info
{
  int flag;
};

static void *thing_ptr1, *thing_ptr2, *thing_ptr3;
static int thing_type;

bool
SetThing (int type, void *ptr1, void *ptr2, void *ptr3)
{
  thing_ptr1 = ptr1;
  thing_ptr2 = ptr2;
  thing_ptr3 = ptr3;
  thing_type = type;
  return true;
}

int GetThing(void **ptr1, void **ptr2, void **ptr3){
  
  *ptr1 = thing_ptr1;
  *ptr2 = thing_ptr2;
  *ptr3 = thing_ptr3;

  return thing_type;
}
static bool User = false;    /*!< User action causing this. */
static Cardinal drcerr_count;   /*!< Count of drc errors */

/*!
 * \brief Check for DRC violations on a single net starting from the pad
 * or pin. (Will call this the "seed object")
 *
 * TODO: Does the seed object have to be a pin or pad?
 *
 * Sees if the connectivity changes when everything is bloated, or
 * shrunk.
 */
static bool
DRCFind (int What, void *ptr1, void *ptr2, void *ptr3)
{
  Coord x, y;
  int object_count;
  long int *object_id_list;
  int *object_type_list;
  DrcViolationType *violation;
  int flag;

  DBG_MSG("Entering...\n");

  if (PCB->Shrink != 0)
    {
      /* Set the DRC and SELECTED flags on all objects that overlap with the
	   * passed object after shrinking them. 
	   * 
	   * The last parameter sets the global "drc" variable in find.c
	   * (through the call to DoIt). It's set to false here because we want
	   * to build a list of all the connections.
	   */
      start_do_it_and_dump (What, ptr1, ptr2, ptr3, /* seed object */
			                DRCFLAG | SELECTEDFLAG, /* flags to set */
							false,  /* AndDraw ?*/ 
							-PCB->Shrink, /* bloat amount */
							false); /* is_drc */

	  /* Now build the list without shrinking objects, and set the FOUND
	   * flag in the process. If a new object is found, the connectivity has
	   * changed, indicating that the minimum overlap rule is violated for
	   * something connected to the original object.
	   *
	   * The last parameter to DoIt sets the global "drc" variable in
	   * find.c. It's set to true here to cause the search to abort if any
	   * new object is found. If the rule is violated once, we stop
	   * searching.
	   *
	   * Note that an object is considered "new" if it doesn't have the
	   * SELECTEDFLAG set. This behavior is hard coded into the algorithm in
	   * add_object_to_list:find.c. This means that the first search must
	   * always set the SELECTEDFLAG.
	   *
	   * TODO: This means that we will only find one violation of this
	   * type for each seed object.
	   */
      ListStart (What, ptr1, ptr2, ptr3, FOUNDFLAG);
      if (DoIt (FOUNDFLAG, true, false, true))
        {
          DumpList ();
          ClearFlagOnAllObjects (false, FOUNDFLAG | SELECTEDFLAG);

          User = true;
          start_do_it_and_dump (What, ptr1, ptr2, ptr3, 
				                SELECTEDFLAG, true, -PCB->Shrink, false);
          start_do_it_and_dump (What, ptr1, ptr2, ptr3, 
				                FOUNDFLAG, true, 0, true);
          User = false;
          drcerr_count++;
          LocateError (&x, &y);
          BuildObjectList (&object_count, &object_id_list, &object_type_list);
          violation = pcb_drc_violation_new (_("Potential for broken trace"),
                                             _("Insufficient overlap between objects can lead to broken tracks\n"
                                               "due to registration errors with old wheel style photo-plotters."),
                                             x, y,
                                             0,     /* ANGLE OF ERROR UNKNOWN */
                                             FALSE, /* MEASUREMENT OF ERROR UNKNOWN */
                                             0,     /* MAGNITUDE OF ERROR UNKNOWN */
                                             PCB->Shrink,
                                             object_count,
                                             object_id_list,
                                             object_type_list);
          append_drc_violation (violation);
          pcb_drc_violation_free (violation);
          free (object_id_list);
          free (object_type_list);

          if (!throw_drc_dialog())
            return (true);
          IncrementUndoSerialNumber ();
          Undo (true);
        }
      DumpList ();
    }
  
  /* now check the bloated condition 
   *
   * TODO: Why isn't this wrapped in if (PCB->Bloat > 0) ?
   */

  /* Reset all of the flags */
  ClearFlagOnAllObjects (false, FOUNDFLAG | SELECTEDFLAG);

  /* Set the DRC and SELECTED flags on all objects that overlap with the
   * passed object after bloating them. 
   *
   * See above for additional notes.
   *
   * TODO: Why is the DRCFLAG used above, but not here?
   */
  start_do_it_and_dump (What, ptr1, ptr2, ptr3, /* seed object */
		                SELECTEDFLAG, /* flags to set */
						false, /* AndDraw ? */
						PCB->Bloat, /* bloat amount */
						false); /* is_drc */
  /* Now build the list without bloating objects, and set the FOUND
   * flag in the process. If a new object is found, the connectivity has
   * changed, indicating that the minimum overlap rule is violated for
   * something connected to the original object. 
   *
   * See above for additional notes. 
   *
   * TODO: Why is this a while loop when the above is an if?
   */
  flag = FOUNDFLAG;
  ListStart (What, ptr1, ptr2, ptr3, flag);
  while (DoIt (flag, true, false, true))
    {
      DumpList ();
      /* make the flag changes undoable */
      ClearFlagOnAllObjects (false, FOUNDFLAG | SELECTEDFLAG);
      User = true;
      start_do_it_and_dump (What, ptr1, ptr2, ptr3, SELECTEDFLAG, true, PCB->Bloat, false);
      start_do_it_and_dump (What, ptr1, ptr2, ptr3, FOUNDFLAG, true, 0, true);
      User = false;
      drcerr_count++;
      LocateError (&x, &y);
      BuildObjectList (&object_count, &object_id_list, &object_type_list);
      violation = pcb_drc_violation_new (_("Copper areas too close"),
                                         _("Circuits that are too close may bridge during imaging, etching,\n"
                                           "plating, or soldering processes resulting in a direct short."),
                                         x, y,
                                         0,     /* ANGLE OF ERROR UNKNOWN */
                                         FALSE, /* MEASUREMENT OF ERROR UNKNOWN */
                                         0,     /* MAGNITUDE OF ERROR UNKNOWN */
                                         PCB->Bloat,
                                         object_count,
                                         object_id_list,
                                         object_type_list);
      append_drc_violation (violation);
      pcb_drc_violation_free (violation);
      free (object_id_list);
      free (object_type_list);
      if (!throw_drc_dialog())
        return (true);
      IncrementUndoSerialNumber ();
      Undo (true);
      /* highlight the rest of the encroaching net so it's not reported again */
      flag = FOUNDFLAG | SELECTEDFLAG;
      start_do_it_and_dump (thing_type, thing_ptr1, thing_ptr2, thing_ptr3, flag, true, 0, false);
      ListStart (What, ptr1, ptr2, ptr3, flag);
    }
  DumpList ();
  ClearFlagOnAllObjects (false, FOUNDFLAG | SELECTEDFLAG);
  return (false);
}

/*!
 * \brief DRC clearance callback.
 */
static int
drc_callback (DataType *data, LayerType *layer, PolygonType *polygon,
              int type, void *ptr1, void *ptr2, void *userdata)
{
  struct drc_info *i = (struct drc_info *) userdata;
  char *message;
  Coord x, y;
  int object_count;
  long int *object_id_list;
  int *object_type_list;
  DrcViolationType *violation;

  LineType *line = (LineType *) ptr2;
  ArcType *arc = (ArcType *) ptr2;
  PinType *pin = (PinType *) ptr2;
  PadType *pad = (PadType *) ptr2;

  SetThing (type, ptr1, ptr2, ptr2);

  switch (type)
    {
    case LINE_TYPE:
      if (line->Clearance < 2 * PCB->Bloat)
        {
          AddObjectToFlagUndoList (type, ptr1, ptr2, ptr2);
          SET_FLAG (i->flag, line);
          message = _("Line with insufficient clearance inside polygon\n");
          goto doIsBad;
        }
      break;
    case ARC_TYPE:
      if (arc->Clearance < 2 * PCB->Bloat)
        {
          AddObjectToFlagUndoList (type, ptr1, ptr2, ptr2);
          SET_FLAG (i->flag, arc);
          message = _("Arc with insufficient clearance inside polygon\n");
          goto doIsBad;
        }
      break;
    case PAD_TYPE:
      if (pad->Clearance && pad->Clearance < 2 * PCB->Bloat)
	if (IsPadInPolygon(pad,polygon, PCB->Bloat))
	  {
	    AddObjectToFlagUndoList (type, ptr1, ptr2, ptr2);
	    SET_FLAG (i->flag, pad);
	    message = _("Pad with insufficient clearance inside polygon\n");
	    goto doIsBad;
	  }
      break;
    case PIN_TYPE:
      if (pin->Clearance && pin->Clearance < 2 * PCB->Bloat)
        {
          AddObjectToFlagUndoList (type, ptr1, ptr2, ptr2);
          SET_FLAG (i->flag, pin);
          message = _("Pin with insufficient clearance inside polygon\n");
          goto doIsBad;
        }
      break;
    case VIA_TYPE:
      if (pin->Clearance && pin->Clearance < 2 * PCB->Bloat)
        {
          AddObjectToFlagUndoList (type, ptr1, ptr2, ptr2);
          SET_FLAG (i->flag, pin);
          message = _("Via with insufficient clearance inside polygon\n");
          goto doIsBad;
        }
      break;
    default:
      Message ("hace: Bad Plow object in callback\n");
    }
  return 0;

doIsBad:
  AddObjectToFlagUndoList (POLYGON_TYPE, layer, polygon, polygon);
  SET_FLAG (FOUNDFLAG, polygon);
  DrawPolygon (layer, polygon);
  DrawObject (type, ptr1, ptr2);
  drcerr_count++;
  LocateError (&x, &y);
  BuildObjectList (&object_count, &object_id_list, &object_type_list);
  violation = pcb_drc_violation_new (message,
                                     _("Circuits that are too close may bridge during imaging, etching,\n"
                                       "plating, or soldering processes resulting in a direct short."),
                                     x, y,
                                     0,     /* ANGLE OF ERROR UNKNOWN */
                                     FALSE, /* MEASUREMENT OF ERROR UNKNOWN */
                                     0,     /* MAGNITUDE OF ERROR UNKNOWN */
                                     PCB->Bloat,
                                     object_count,
                                     object_id_list,
                                     object_type_list);
  append_drc_violation (violation);
  pcb_drc_violation_free (violation);
  free (object_id_list);
  free (object_type_list);

  if (!throw_drc_dialog())
    return 1;

  IncrementUndoSerialNumber ();
  Undo (true);
  return 0;
}

DrcViolationType *
drc_warning_violation(void)
{
  DrcViolationType *violation;
  /* This phony violation informs user about what DRC does NOT catch.  */
  violation
    = pcb_drc_violation_new (
        _("WARNING: DRC doesn't catch everything"),
        _("Detection of outright shorts, missing connections, etc.\n"
          "is handled via rat's nest addition.  To catch these problems,\n"
          "display the message log using Window->Message Log, then use\n"
          "Connects->Optimize rats nest (O hotkey) and watch for messages.\n"),
        /* All remaining arguments are not relevant to this application.  */
        0, 0, 0, TRUE, 0, 0, 0, NULL, NULL);
  append_drc_violation (violation);
  return violation;
}

/*!
 * \brief Check for DRC violations.
 *
 * See if the connectivity changes when everything is bloated, or shrunk.
 */
int
DRCAll (void)
{
  Coord x, y;
  int object_count;
  long int *object_id_list;
  int *object_type_list;
  DrcViolationType *violation;
  int tmpcnt;
  int nopastecnt = 0;
  bool IsBad;
  struct drc_info info;

  DBG_MSG("Entering...\n");

  reset_drc_dialog_message();

  /* Warn user that DRC doesn't catch everything... */
  violation = drc_warning_violation();
  pcb_drc_violation_free (violation);


  if (!throw_drc_dialog())
    return (true);

  IsBad = false;
  drcerr_count = 0;

  /* Since the searching functions only operate on visible layers, we need
   * to make sure that everything is turned on in order to check the entire
   * design. 
   *
   * TODO: This could be a way of giving the user control over what area to
   *       run the DRC on... ?
   * 
   */

  /* Save the layer order and visibility settings so we can restore it later */
  SaveStackAndVisibility ();
  /* Turn on everything */
  ResetStackAndVisibility ();
  hid_action ("LayersChanged");


  InitConnectionLookup ();

  if (ClearFlagOnAllObjects (true, FOUNDFLAG | DRCFLAG | SELECTEDFLAG))
    {
      IncrementUndoSerialNumber ();
      Draw ();
    }

  User = false;

  ELEMENT_LOOP (PCB->Data);
  {
    PIN_LOOP (element);
    {
      if (!TEST_FLAG (DRCFLAG, pin)
          && DRCFind (PIN_TYPE, (void *) element, (void *) pin, (void *) pin))
        {
          IsBad = true;
          break;
        }
    }
    END_LOOP;
    if (IsBad)
      break;
    PAD_LOOP (element);
    {

      /* count up how many pads have no solderpaste openings */
      if (TEST_FLAG (NOPASTEFLAG, pad))
	nopastecnt++;

      if (!TEST_FLAG (DRCFLAG, pad)
          && DRCFind (PAD_TYPE, (void *) element, (void *) pad, (void *) pad))
        {
          IsBad = true;
          break;
        }
    }
    END_LOOP;
    if (IsBad)
      break;
  }
  END_LOOP;
  if (!IsBad)
    VIA_LOOP (PCB->Data);
  {
    if (!TEST_FLAG (DRCFLAG, via)
        && DRCFind (VIA_TYPE, (void *) via, (void *) via, (void *) via))
      {
        IsBad = true;
        break;
      }
  }
  END_LOOP;

  ClearFlagOnAllObjects (false, IsBad ? DRCFLAG : (FOUNDFLAG | DRCFLAG | SELECTEDFLAG));
  info.flag = SELECTEDFLAG;
  /* check minimum widths and polygon clearances */
  if (!IsBad)
    {
      COPPERLINE_LOOP (PCB->Data);
      {
        bool plows_polygon;
        /* check line clearances in polygons */
        int old_clearance = line->Clearance;
        /* Create a bounding box with DRC clearance */
        line->Clearance = 2*PCB->Bloat;
        SetLineBoundingBox(line);
        line->Clearance = old_clearance;
        plows_polygon = PlowsPolygon (PCB->Data, LINE_TYPE, layer, line, drc_callback, &info);
        SetLineBoundingBox(line); /* Recover old bounding box */
        if (plows_polygon)
          {
            IsBad = true;
            break;
          }

        if (line->Thickness < PCB->minWid)
          {
            AddObjectToFlagUndoList (LINE_TYPE, layer, line, line);
            SET_FLAG (SELECTEDFLAG, line);
            DrawLine (layer, line);
            drcerr_count++;
            SetThing (LINE_TYPE, layer, line, line);
            LocateError (&x, &y);
            BuildObjectList (&object_count, &object_id_list, &object_type_list);
            violation = pcb_drc_violation_new (_("Line width is too thin"),
                                               _("Process specifications dictate a minimum feature-width\n"
                                                 "that can reliably be reproduced"),
                                               x, y,
                                               0,    /* ANGLE OF ERROR UNKNOWN */
                                               TRUE, /* MEASUREMENT OF ERROR KNOWN */
                                               line->Thickness,
                                               PCB->minWid,
                                               object_count,
                                               object_id_list,
                                               object_type_list);
            append_drc_violation (violation);
            pcb_drc_violation_free (violation);
            free (object_id_list);
            free (object_type_list);
            if (!throw_drc_dialog())
              {
                IsBad = true;
                break;
              }
            IncrementUndoSerialNumber ();
            Undo (false);
          }
      }
      ENDALL_LOOP;
    }
  if (!IsBad)
    {
      COPPERARC_LOOP (PCB->Data);
      {
        if (PlowsPolygon (PCB->Data, ARC_TYPE, layer, arc, drc_callback, &info))
          {
            IsBad = true;
            break;
          }
        if (arc->Thickness < PCB->minWid)
          {
            AddObjectToFlagUndoList (ARC_TYPE, layer, arc, arc);
            SET_FLAG (SELECTEDFLAG, arc);
            DrawArc (layer, arc);
            drcerr_count++;
            SetThing (ARC_TYPE, layer, arc, arc);
            LocateError (&x, &y);
            BuildObjectList (&object_count, &object_id_list, &object_type_list);
            violation = pcb_drc_violation_new (_("Arc width is too thin"),
                                               _("Process specifications dictate a minimum feature-width\n"
                                                 "that can reliably be reproduced"),
                                               x, y,
                                               0,    /* ANGLE OF ERROR UNKNOWN */
                                               TRUE, /* MEASUREMENT OF ERROR KNOWN */
                                               arc->Thickness,
                                               PCB->minWid,
                                               object_count,
                                               object_id_list,
                                               object_type_list);
            append_drc_violation (violation);
            pcb_drc_violation_free (violation);
            free (object_id_list);
            free (object_type_list);
            if (!throw_drc_dialog())
              {
                IsBad = true;
                break;
              }
            IncrementUndoSerialNumber ();
            Undo (false);
          }
      }
      ENDALL_LOOP;
    }
  if (!IsBad)
    {
      ALLPIN_LOOP (PCB->Data);
      {
        if (PlowsPolygon (PCB->Data, PIN_TYPE, element, pin, drc_callback, &info))
          {
            IsBad = true;
            break;
          }
        if (!TEST_FLAG (HOLEFLAG, pin) &&
            pin->Thickness - pin->DrillingHole < 2 * PCB->minRing)
          {
            AddObjectToFlagUndoList (PIN_TYPE, element, pin, pin);
            SET_FLAG (SELECTEDFLAG, pin);
            DrawPin (pin);
            drcerr_count++;
            SetThing (PIN_TYPE, element, pin, pin);
            LocateError (&x, &y);
            BuildObjectList (&object_count, &object_id_list, &object_type_list);
            violation = pcb_drc_violation_new (_("Pin annular ring too small"),
                                               _("Annular rings that are too small may erode during etching,\n"
                                                 "resulting in a broken connection"),
                                               x, y,
                                               0,    /* ANGLE OF ERROR UNKNOWN */
                                               TRUE, /* MEASUREMENT OF ERROR KNOWN */
                                               (pin->Thickness - pin->DrillingHole) / 2,
                                               PCB->minRing,
                                               object_count,
                                               object_id_list,
                                               object_type_list);
            append_drc_violation (violation);
            pcb_drc_violation_free (violation);
            free (object_id_list);
            free (object_type_list);
            if (!throw_drc_dialog())
              {
                IsBad = true;
                break;
              }
            IncrementUndoSerialNumber ();
            Undo (false);
          }
        if (pin->DrillingHole < PCB->minDrill)
          {
            AddObjectToFlagUndoList (PIN_TYPE, element, pin, pin);
            SET_FLAG (SELECTEDFLAG, pin);
            DrawPin (pin);
            drcerr_count++;
            SetThing (PIN_TYPE, element, pin, pin);
            LocateError (&x, &y);
            BuildObjectList (&object_count, &object_id_list, &object_type_list);
            violation = pcb_drc_violation_new (_("Pin drill size is too small"),
                                               _("Process rules dictate the minimum drill size which can be used"),
                                               x, y,
                                               0,    /* ANGLE OF ERROR UNKNOWN */
                                               TRUE, /* MEASUREMENT OF ERROR KNOWN */
                                               pin->DrillingHole,
                                               PCB->minDrill,
                                               object_count,
                                               object_id_list,
                                               object_type_list);
            append_drc_violation (violation);
            pcb_drc_violation_free (violation);
            free (object_id_list);
            free (object_type_list);
            if (!throw_drc_dialog())
              {
                IsBad = true;
                break;
              }
            IncrementUndoSerialNumber ();
            Undo (false);
          }
      }
      ENDALL_LOOP;
    }
  if (!IsBad)
    {
      ALLPAD_LOOP (PCB->Data);
      {
        if (PlowsPolygon (PCB->Data, PAD_TYPE, element, pad, drc_callback, &info))
          {
            IsBad = true;
            break;
          }
        if (pad->Thickness < PCB->minWid)
          {
            AddObjectToFlagUndoList (PAD_TYPE, element, pad, pad);
            SET_FLAG (SELECTEDFLAG, pad);
            DrawPad (pad);
            drcerr_count++;
            SetThing (PAD_TYPE, element, pad, pad);
            LocateError (&x, &y);
            BuildObjectList (&object_count, &object_id_list, &object_type_list);
            violation = pcb_drc_violation_new (_("Pad is too thin"),
                                               _("Pads which are too thin may erode during etching,\n"
                                                  "resulting in a broken or unreliable connection"),
                                               x, y,
                                               0,    /* ANGLE OF ERROR UNKNOWN */
                                               TRUE, /* MEASUREMENT OF ERROR KNOWN */
                                               pad->Thickness,
                                               PCB->minWid,
                                               object_count,
                                               object_id_list,
                                               object_type_list);
            append_drc_violation (violation);
            pcb_drc_violation_free (violation);
            free (object_id_list);
            free (object_type_list);
            if (!throw_drc_dialog())
              {
                IsBad = true;
                break;
              }
            IncrementUndoSerialNumber ();
            Undo (false);
          }
      }
      ENDALL_LOOP;
    }
  if (!IsBad)
    {
      VIA_LOOP (PCB->Data);
      {
        if (PlowsPolygon (PCB->Data, VIA_TYPE, via, via, drc_callback, &info))
          {
            IsBad = true;
            break;
          }
        if (!TEST_FLAG (HOLEFLAG, via) &&
            via->Thickness - via->DrillingHole < 2 * PCB->minRing)
          {
            AddObjectToFlagUndoList (VIA_TYPE, via, via, via);
            SET_FLAG (SELECTEDFLAG, via);
            DrawVia (via);
            drcerr_count++;
            SetThing (VIA_TYPE, via, via, via);
            LocateError (&x, &y);
            BuildObjectList (&object_count, &object_id_list, &object_type_list);
            violation = pcb_drc_violation_new (_("Via annular ring too small"),
                                               _("Annular rings that are too small may erode during etching,\n"
                                                 "resulting in a broken connection"),
                                               x, y,
                                               0,    /* ANGLE OF ERROR UNKNOWN */
                                               TRUE, /* MEASUREMENT OF ERROR KNOWN */
                                               (via->Thickness - via->DrillingHole) / 2,
                                               PCB->minRing,
                                               object_count,
                                               object_id_list,
                                               object_type_list);
            append_drc_violation (violation);
            pcb_drc_violation_free (violation);
            free (object_id_list);
            free (object_type_list);
            if (!throw_drc_dialog())
              {
                IsBad = true;
                break;
              }
            IncrementUndoSerialNumber ();
            Undo (false);
          }
        if (via->DrillingHole < PCB->minDrill)
          {
            AddObjectToFlagUndoList (VIA_TYPE, via, via, via);
            SET_FLAG (SELECTEDFLAG, via);
            DrawVia (via);
            drcerr_count++;
            SetThing (VIA_TYPE, via, via, via);
            LocateError (&x, &y);
            BuildObjectList (&object_count, &object_id_list, &object_type_list);
            violation = pcb_drc_violation_new (_("Via drill size is too small"),
                                               _("Process rules dictate the minimum drill size which can be used"),
                                               x, y,
                                               0,    /* ANGLE OF ERROR UNKNOWN */
                                               TRUE, /* MEASUREMENT OF ERROR KNOWN */
                                               via->DrillingHole,
                                               PCB->minDrill,
                                               object_count,
                                               object_id_list,
                                               object_type_list);
            append_drc_violation (violation);
            pcb_drc_violation_free (violation);
            free (object_id_list);
            free (object_type_list);
            if (!throw_drc_dialog())
              {
                IsBad = true;
                break;
              }
            IncrementUndoSerialNumber ();
            Undo (false);
          }
      }
      END_LOOP;
    }

  FreeConnectionLookupMemory ();

  /* check silkscreen minimum widths outside of elements */
  /* XXX - need to check text and polygons too! */
  if (!IsBad)
    {
      SILKLINE_LOOP (PCB->Data);
      {
        if (line->Thickness < PCB->minSlk)
          {
            SET_FLAG (SELECTEDFLAG, line);
            DrawLine (layer, line);
            drcerr_count++;
            SetThing (LINE_TYPE, layer, line, line);
            LocateError (&x, &y);
            BuildObjectList (&object_count, &object_id_list, &object_type_list);
            violation = pcb_drc_violation_new (_("Silk line is too thin"),
                                               _("Process specifications dictate a minimum silkscreen\n"
                                               "feature-width that can reliably be reproduced"),
                                               x, y,
                                               0,    /* ANGLE OF ERROR UNKNOWN */
                                               TRUE, /* MEASUREMENT OF ERROR KNOWN */
                                               line->Thickness,
                                               PCB->minSlk,
                                               object_count,
                                               object_id_list,
                                               object_type_list);
            append_drc_violation (violation);
            pcb_drc_violation_free (violation);
            free (object_id_list);
            free (object_type_list);
            if (!throw_drc_dialog())
              {
                IsBad = true;
                break;
              }
          }
      }
      ENDALL_LOOP;
    }

  /* check silkscreen minimum widths inside of elements */
  /* XXX - need to check text and polygons too! */
  if (!IsBad)
    {
      ELEMENT_LOOP (PCB->Data);
      {
        tmpcnt = 0;
        ELEMENTLINE_LOOP (element);
        {
          if (line->Thickness < PCB->minSlk)
            tmpcnt++;
        }
        END_LOOP;
        if (tmpcnt > 0)
          {
            char *title;
            char *name;
            char *buffer;
            int buflen;

            SET_FLAG (SELECTEDFLAG, element);
            DrawElement (element);
            drcerr_count++;
            SetThing (ELEMENT_TYPE, element, element, element);
            LocateError (&x, &y);
            BuildObjectList (&object_count, &object_id_list, &object_type_list);

            title = _("Element %s has %i silk lines which are too thin");
            name = (char *)UNKNOWN (NAMEONPCB_NAME (element));

            /* -4 is for the %s and %i place-holders */
            /* +11 is the max printed length for a 32 bit integer */
            /* +1 is for the \0 termination */
            buflen = strlen (title) - 4 + strlen (name) + 11 + 1;
            buffer = (char *)malloc (buflen);
            snprintf (buffer, buflen, title, name, tmpcnt);

            violation = pcb_drc_violation_new (buffer,
                                               _("Process specifications dictate a minimum silkscreen\n"
                                               "feature-width that can reliably be reproduced"),
                                               x, y,
                                               0,    /* ANGLE OF ERROR UNKNOWN */
                                               TRUE, /* MEASUREMENT OF ERROR KNOWN */
                                               0,    /* MINIMUM OFFENDING WIDTH UNKNOWN */
                                               PCB->minSlk,
                                               object_count,
                                               object_id_list,
                                               object_type_list);
            free (buffer);
            append_drc_violation (violation);
            pcb_drc_violation_free (violation);
            free (object_id_list);
            free (object_type_list);
            if (!throw_drc_dialog())
              {
                IsBad = true;
                break;
              }
          }
      }
      END_LOOP;
    }


  if (IsBad)
    {
      IncrementUndoSerialNumber ();
    }


  RestoreStackAndVisibility ();
  hid_action ("LayersChanged");
  gui->invalidate_all ();

  if (nopastecnt > 0) 
    {
      Message (ngettext ("Warning: %d pad has the nopaste flag set.\n",
                         "Warning: %d pads have the nopaste flag set.\n",
			 nopastecnt), nopastecnt);
    }
  return IsBad ? -drcerr_count : drcerr_count;
}

/*!
 * \brief Locate the coordinatates of offending item (thing).
 */
void
LocateError (Coord *x, Coord *y)
{
  switch (thing_type)
    {
    case LINE_TYPE:
      {
        LineType *line = (LineType *) thing_ptr3;
        *x = (line->Point1.X + line->Point2.X) / 2;
        *y = (line->Point1.Y + line->Point2.Y) / 2;
        break;
      }
    case ARC_TYPE:
      {
        ArcType *arc = (ArcType *) thing_ptr3;
        *x = arc->X;
        *y = arc->Y;
        break;
      }
    case POLYGON_TYPE:
      {
        PolygonType *polygon = (PolygonType *) thing_ptr3;
        *x =
          (polygon->Clipped->contours->xmin +
           polygon->Clipped->contours->xmax) / 2;
        *y =
          (polygon->Clipped->contours->ymin +
           polygon->Clipped->contours->ymax) / 2;
        break;
      }
    case PIN_TYPE:
    case VIA_TYPE:
      {
        PinType *pin = (PinType *) thing_ptr3;
        *x = pin->X;
        *y = pin->Y;
        break;
      }
    case PAD_TYPE:
      {
        PadType *pad = (PadType *) thing_ptr3;
        *x = (pad->Point1.X + pad->Point2.X) / 2;
        *y = (pad->Point1.Y + pad->Point2.Y) / 2;
        break;
      }
    case ELEMENT_TYPE:
      {
        ElementType *element = (ElementType *) thing_ptr3;
        *x = element->MarkX;
        *y = element->MarkY;
        break;
      }
    default:
      return;
    }
}

/*!
 * \brief Center the display to show the offending item (thing).
 */
void
GotoError (void)
{
  Coord X, Y;

  LocateError (&X, &Y);

  switch (thing_type)
    {
    case LINE_TYPE:
    case ARC_TYPE:
    case POLYGON_TYPE:
      ChangeGroupVisibility (
          GetLayerNumber (PCB->Data, (LayerType *) thing_ptr1),
          true, true);
    }
  CenterDisplay (X, Y, false);
}

/* -------------------------------------------------------------------------- */

static const char drc_syntax[] = N_("DRC()");

static const char drc_help[] = N_("Invoke the DRC check.");

/* %start-doc actions DRC

Note that the design rule check uses the current board rule settings,
not the current style settings.

%end-doc */

static int
ActionDRCheck (int argc, char **argv, Coord x, Coord y)
{
  int count;

  DBG_MSG("Entering...\n");
  if (gui->drc_gui == NULL || gui->drc_gui->log_drc_overview)
    {
      Message (_("%m+Rules are minspace %$mS, minoverlap %$mS "
		 "minwidth %$mS, minsilk %$mS\n"
		 "min drill %$mS, min annular ring %$mS\n"),
               Settings.grid_unit->allow,
	       PCB->Bloat, PCB->Shrink,
	       PCB->minWid, PCB->minSlk,
	       PCB->minDrill, PCB->minRing);
    }
  count = DRCAll ();
  if (gui->drc_gui == NULL || gui->drc_gui->log_drc_overview)
    {
      if (count == 0)
	Message (_("No DRC problems found.\n"));
      else if (count > 0)
	Message (_("Found %d design rule errors.\n"), count);
      else
	Message (_("Aborted DRC after %d design rule errors.\n"), -count);
    }
  return 0;
}

HID_Action drc_action_list[] = {
  {"DRC", 0, ActionDRCheck, drc_help, drc_syntax}
};

REGISTER_ACTIONS (drc_action_list)

