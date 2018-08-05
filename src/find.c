/*!
 * \file src/find.c
 *
 * \brief Routines to find connections between pins, vias, lines ...
 *
 * Short description:\n
 * <ul>
 * <li> Lists for pins and vias, lines, arcs, pads and for polygons are
 *   created.\n
 *   Every object that has to be checked is added to its list.\n
 *   Coarse searching is accomplished with the data rtrees.</li>
 * <li> There's no 'speed-up' mechanism for polygons because they are
 *   not used as often as other objects.</li> 
 * <li> The maximum distance between line and pin ... would depend on
 *   the angle between them. To speed up computation the limit is set
 *   to one half of the thickness of the objects (cause of square
 *   pins).</li>
 * </ul>
 *
 * PV:  means pin or via (objects that connect layers).\n
 * LO:  all non PV objects (layer objects like lines, arcs, polygons,
 * pads).
 *
 * <ol>
 * <li> First, the LO or PV at the given coordinates is looked
 *   up.</li>
 * <li> All LO connections to that PV are looked up next.</li>
 * <li> Lookup of all LOs connected to LOs from (2).\n
 *   This step is repeated until no more new connections are found.</li>
 * <li> Lookup all PVs connected to the LOs from (2) and (3).</li>
 * <li> Start again with (1) for all new PVs from (4).</li>
 * </ol>
 *
 * Intersection of line <--> circle:\n
 * <ul>
 * <li> Calculate the signed distance from the line to the center,
 *   return false if abs(distance) > R.</li>
 * <li> Get the distance from the line <--> distancevector intersection
 *   to (X1,Y1) in range [0,1], return true if 0 <= distance <= 1.</li>
 * <li> Depending on (r > 1.0 or r < 0.0) check the distance of X2,Y2 or
 *   X1,Y1 to X,Y.</li>
 * </ul>
 *
 * Intersection of line <--> line:\n
 * <ul>
 * <li> See the description of 'LineLineIntersect()'.</li>
 * </ul>
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996, 2005 Thomas Nau
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <setjmp.h>
#include <assert.h>

#include "global.h"

#include "data.h"
#include "draw.h"
#include "error.h"
#include "find.h"
#include "misc.h"
#include "rtree.h"
#include "polygon.h"
#include "pcb_geometry.h"
#include "pcb-printf.h"
#include "search.h"
#include "set.h"
#include "undo.h"
#include "rats.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#undef DEBUG

/* ---------------------------------------------------------------------------
 * some local macros
 */

#define	SEPARATE(FP)							\
	{											\
		int	i;									\
		fputc('#', (FP));						\
		for (i = Settings.CharPerLine; i; i--)	\
			fputc('=', (FP));					\
		fputc('\n', (FP));						\
	}

#define LIST_ENTRY(list,I)      (((AnyObjectType **)list->Data)[(I)])
#define PADLIST_ENTRY(L,I)      (((PadType **)PadList[(L)].Data)[(I)])
#define LINELIST_ENTRY(L,I)     (((LineType **)LineList[(L)].Data)[(I)])
#define ARCLIST_ENTRY(L,I)      (((ArcType **)ArcList[(L)].Data)[(I)])
#define RATLIST_ENTRY(I)        (((RatType **)RatList.Data)[(I)])
#define POLYGONLIST_ENTRY(L,I)  (((PolygonType **)PolygonList[(L)].Data)[(I)])
#define PVLIST_ENTRY(I)         (((PinType **)PVList.Data)[(I)])

/*!
 * \brief Some local types.
 *
 * The two 'dummy' structs for PVs and Pads are necessary for creating
 * connection lists which include the element's name.
 */
typedef struct
{
  void **Data;                  /*!< Pointer to index data. */
  Cardinal Location,            /*!< Currently used position. */
    DrawLocation, Number,       /*!< Number of objects in list. */
    Size;
} ListType;

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static Coord Bloat = 0;

static bool User = false;    /*!< User action causing this. */
static bool drc = false;     /*!< Whether to stop if finding something not found. */
static Cardinal TotalP, TotalV;
static ListType LineList[MAX_LAYER],    /*!< List of objects to. */
  PolygonList[MAX_LAYER], ArcList[MAX_LAYER], PadList[2], RatList, PVList;

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static bool LookupLOConnectionsToLine (LineType *, Cardinal, int, bool, bool);
static bool LookupLOConnectionsToPad (PadType *, Cardinal, int, bool);
static bool LookupLOConnectionsToPolygon (PolygonType *, Cardinal, int, bool);
static bool LookupLOConnectionsToArc (ArcType *, Cardinal, int, bool);
static bool LookupLOConnectionsToRatEnd (PointType *, Cardinal, int);
static bool PrepareNextLoop (FILE *);
static void DrawNewConnections (void);
static void DumpList (void);

static bool
add_object_to_list (ListType *list, int type, void *ptr1, void *ptr2, void *ptr3, int flag)
{
  AnyObjectType *object = (AnyObjectType *)ptr2;

  if (User)
    AddObjectToFlagUndoList (type, ptr1, ptr2, ptr3);

  SET_FLAG (flag, object);
  LIST_ENTRY (list, list->Number) = object;
  list->Number++;

#ifdef DEBUG
  if (list->Number > list->Size)
    printf ("add_object_to_list overflow! type=%i num=%d size=%d\n", type, list->Number, list->Size);
#endif

  if (drc && !TEST_FLAG (SELECTEDFLAG, object))
    return (SetThing (type, ptr1, ptr2, ptr3));
  return false;
}

static bool
ADD_PV_TO_LIST (PinType *Pin, int flag)
{
  return add_object_to_list (&PVList, Pin->Element ? PIN_TYPE : VIA_TYPE,
                             Pin->Element ? Pin->Element : Pin, Pin, Pin, flag);
}

static bool
ADD_PAD_TO_LIST (Cardinal L, PadType *Pad, int flag)
{
  return add_object_to_list (&PadList[L], PAD_TYPE, Pad->Element, Pad, Pad, flag);
}

static bool
ADD_LINE_TO_LIST (Cardinal L, LineType *Ptr, int flag)
{
  return add_object_to_list (&LineList[L], LINE_TYPE, LAYER_PTR (L), Ptr, Ptr, flag);
}

static bool
ADD_ARC_TO_LIST (Cardinal L, ArcType *Ptr, int flag)
{
  return add_object_to_list (&ArcList[L], ARC_TYPE, LAYER_PTR (L), Ptr, Ptr, flag);
}

static bool
ADD_RAT_TO_LIST (RatType *Ptr, int flag)
{
  return add_object_to_list (&RatList, RATLINE_TYPE, Ptr, Ptr, Ptr, flag);
}

static bool
ADD_POLYGON_TO_LIST (Cardinal L, PolygonType *Ptr, int flag)
{
  return add_object_to_list (&PolygonList[L], POLYGON_TYPE, LAYER_PTR (L), Ptr, Ptr, flag);
}

static BoxType
expand_bounds (BoxType *box_in)
{
  BoxType box_out = *box_in;

  if (Bloat > 0)
    {
      box_out.X1 -= Bloat;
      box_out.X2 += Bloat;
      box_out.Y1 -= Bloat;
      box_out.Y2 += Bloat;
    }

  return box_out;
}


/*!
 * \brief Releases all allocated memory.
 */
static void
FreeLayoutLookupMemory (void)
{
  Cardinal i;

  for (i = 0; i < max_copper_layer; i++)
    {
      free (LineList[i].Data);
      LineList[i].Data = NULL;
      free (ArcList[i].Data);
      ArcList[i].Data = NULL;
      free (PolygonList[i].Data);
      PolygonList[i].Data = NULL;
    }
  free (PVList.Data);
  PVList.Data = NULL;
  free (RatList.Data);
  RatList.Data = NULL;
}

static void
FreeComponentLookupMemory (void)
{
  free (PadList[0].Data);
  PadList[0].Data = NULL;
  free (PadList[1].Data);
  PadList[1].Data = NULL;
}

/*!
 * \brief Allocates memory for component related stacks ...
 *
 * Initializes index and sorts it by X1 and X2.
 */
static void
InitComponentLookup (void)
{
  Cardinal NumberOfPads[2];
  Cardinal i;

  /* initialize pad data; start by counting the total number
   * on each of the two possible layers
   */
  NumberOfPads[TOP_SIDE] = NumberOfPads[BOTTOM_SIDE] = 0;
  ALLPAD_LOOP (PCB->Data);
  {
    if (TEST_FLAG (ONSOLDERFLAG, pad))
      NumberOfPads[BOTTOM_SIDE]++;
    else
      NumberOfPads[TOP_SIDE]++;
  }
  ENDALL_LOOP;
  for (i = 0; i < 2; i++)
    {
      /* allocate memory for working list */
      PadList[i].Data = (void **)calloc (NumberOfPads[i], sizeof (PadType *));

      /* clear some struct members */
      PadList[i].Location = 0;
      PadList[i].DrawLocation = 0;
      PadList[i].Number = 0;
      PadList[i].Size = NumberOfPads[i];
    }
}

/*!
 * \brief Allocates memory for layout related stacks ...
 *
 * Initializes index and sorts it by X1 and X2.
 */
static void
InitLayoutLookup (void)
{
  Cardinal i;

  /* initialize line arc and polygon data */
  for (i = 0; i < max_copper_layer; i++)
    {
      LayerType *layer = LAYER_PTR (i);

      if (layer->LineN)
        {
          /* allocate memory for line pointer lists */
          LineList[i].Data = (void **)calloc (layer->LineN, sizeof (LineType *));
          LineList[i].Size = layer->LineN;
        }
      if (layer->ArcN)
        {
          ArcList[i].Data = (void **)calloc (layer->ArcN, sizeof (ArcType *));
          ArcList[i].Size = layer->ArcN;
        }


      /* allocate memory for polygon list */
      if (layer->PolygonN)
        {
          PolygonList[i].Data = (void **)calloc (layer->PolygonN, sizeof (PolygonType *));
          PolygonList[i].Size = layer->PolygonN;
        }

      /* clear some struct members */
      LineList[i].Location = 0;
      LineList[i].DrawLocation = 0;
      LineList[i].Number = 0;
      ArcList[i].Location = 0;
      ArcList[i].DrawLocation = 0;
      ArcList[i].Number = 0;
      PolygonList[i].Location = 0;
      PolygonList[i].DrawLocation = 0;
      PolygonList[i].Number = 0;
    }

  if (PCB->Data->pin_tree)
    TotalP = PCB->Data->pin_tree->size;
  else
    TotalP = 0;
  if (PCB->Data->via_tree)
    TotalV = PCB->Data->via_tree->size;
  else
    TotalV = 0;
  /* allocate memory for 'new PV to check' list and clear struct */
  PVList.Data = (void **)calloc (TotalP + TotalV, sizeof (PinType *));
  PVList.Size = TotalP + TotalV;
  PVList.Location = 0;
  PVList.DrawLocation = 0;
  PVList.Number = 0;
  /* Initialize ratline data */
  RatList.Data = (void **)calloc (PCB->Data->RatN, sizeof (RatType *));
  RatList.Size = PCB->Data->RatN;
  RatList.Location = 0;
  RatList.DrawLocation = 0;
  RatList.Number = 0;
}

struct pv_info
{
  Cardinal layer;
  PinType *pv;
  int flag;
  jmp_buf env;
};

static int
LOCtoPVline_callback (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  struct pv_info *i = (struct pv_info *) cl;

  if (!ViaIsOnLayerGroup (i->pv, GetLayerGroupNumberByNumber (i->layer)))
    return 0;

  if (!TEST_FLAG (i->flag, line) && PinLineIntersect (i->pv, line, Bloat) &&
      !TEST_FLAG (HOLEFLAG, i->pv))
    {
      if (ADD_LINE_TO_LIST (i->layer, line, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPVarc_callback (const BoxType * b, void *cl)
{
  ArcType *arc = (ArcType *) b;
  struct pv_info *i = (struct pv_info *) cl;

  if (!ViaIsOnLayerGroup (i->pv, GetLayerGroupNumberByNumber (i->layer)))
    return 0;

  if (!TEST_FLAG (i->flag, arc) && IsPinOnArc (i->pv, arc, Bloat) &&
      !TEST_FLAG (HOLEFLAG, i->pv))
    {
      if (ADD_ARC_TO_LIST (i->layer, arc, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPVpad_callback (const BoxType * b, void *cl)
{
  PadType *pad = (PadType *) b;
  struct pv_info *i = (struct pv_info *) cl;

  if (!ViaIsOnLayerGroup (i->pv, GetLayerGroupNumberBySide (TEST_FLAG (ONSOLDERFLAG, pad) ? BOTTOM_SIDE : TOP_SIDE)))
    return 0;

  if (!TEST_FLAG (i->flag, pad) && IsPinOnPad (i->pv, pad, Bloat) &&
      !TEST_FLAG (HOLEFLAG, i->pv) &&
      ADD_PAD_TO_LIST (TEST_FLAG (ONSOLDERFLAG, pad) ? BOTTOM_SIDE :
                       TOP_SIDE, pad, i->flag))
    longjmp (i->env, 1);
  return 0;
}

static int
LOCtoPVrat_callback (const BoxType * b, void *cl)
{
  RatType *rat = (RatType *) b;
  struct pv_info *i = (struct pv_info *) cl;

  if (!TEST_FLAG (i->flag, rat) && IsPinOnRat (i->pv, rat) &&
      ADD_RAT_TO_LIST (rat, i->flag))
    longjmp (i->env, 1);
  return 0;
}

static int
LOCtoPVpoly_callback (const BoxType * b, void *cl)
{
  PolygonType *polygon = (PolygonType *) b;
  struct pv_info *i = (struct pv_info *) cl;

  if (!ViaIsOnLayerGroup (i->pv, GetLayerGroupNumberByNumber (i->layer)))
    return 0;

  /* if the pin doesn't have a therm and polygon is clearing
   * then it can't touch due to clearance, so skip the expensive
   * test. If it does have a therm, you still need to test
   * because it might not be inside the polygon, or it could
   * be on an edge such that it doesn't actually touch.
   */
  if (!TEST_FLAG (i->flag, polygon) && !TEST_FLAG (HOLEFLAG, i->pv) &&
                                       (TEST_THERM (i->layer, i->pv) ||
                                        !TEST_FLAG (CLEARPOLYFLAG,
                                                    polygon)
                                        || !i->pv->Clearance))
    {
      double wide = MAX (0.5 * i->pv->Thickness + Bloat, 0);
      if (TEST_FLAG (SQUAREFLAG, i->pv))
        {
          Coord x1 = i->pv->X - (i->pv->Thickness + 1 + Bloat) / 2;
          Coord x2 = i->pv->X + (i->pv->Thickness + 1 + Bloat) / 2;
          Coord y1 = i->pv->Y - (i->pv->Thickness + 1 + Bloat) / 2;
          Coord y2 = i->pv->Y + (i->pv->Thickness + 1 + Bloat) / 2;
          if (IsRectangleInPolygon (x1, y1, x2, y2, polygon)
              && ADD_POLYGON_TO_LIST (i->layer, polygon, i->flag))
            longjmp (i->env, 1);
        }
      else if (TEST_FLAG (OCTAGONFLAG, i->pv))
        {
          POLYAREA *oct = OctagonPoly (i->pv->X, i->pv->Y, i->pv->Thickness / 2);
          if (isects (oct, polygon, true)
              && ADD_POLYGON_TO_LIST (i->layer, polygon, i->flag))
            longjmp (i->env, 1);
        }
      else if (IsPointInPolygon (i->pv->X, i->pv->Y, wide,
                                 polygon)
               && ADD_POLYGON_TO_LIST (i->layer, polygon, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

/*!
 * \brief Checks if a PV is connected to LOs, if it is, the LO is added
 * to the appropriate list and the 'used' flag is set.
 */
static bool
LookupLOConnectionsToPVList (int flag, bool AndRats)
{
  Cardinal layer_no;
  struct pv_info info;

  info.flag = flag;

  /* loop over all PVs currently on list */
  while (PVList.Location < PVList.Number)
    {
      BoxType search_box;

      /* get pointer to data */
      info.pv = PVLIST_ENTRY (PVList.Location);
      search_box = expand_bounds (&info.pv->BoundingBox);

      /* check pads */
      if (setjmp (info.env) == 0)
        r_search (PCB->Data->pad_tree, &search_box, NULL,
                  LOCtoPVpad_callback, &info);
      else
        return true;

      /* now all lines, arcs and polygons of the several layers */
      for (layer_no = 0; layer_no < max_copper_layer; layer_no++)
        {
          LayerType *layer = LAYER_PTR (layer_no);

          if (layer->no_drc)
             continue;

          info.layer = layer_no;

          /* add touching lines */
          if (setjmp (info.env) == 0)
            r_search (layer->line_tree, &search_box,
                      NULL, LOCtoPVline_callback, &info);
          else
            return true;
          /* add touching arcs */
          if (setjmp (info.env) == 0)
            r_search (layer->arc_tree, &search_box,
                      NULL, LOCtoPVarc_callback, &info);
          else
            return true;
          /* check all polygons */
          if (setjmp (info.env) == 0)
            r_search (layer->polygon_tree, &search_box,
                      NULL, LOCtoPVpoly_callback, &info);
          else
            return true;
        }
      /* Check for rat-lines that may intersect the PV */
      if (AndRats)
        {
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->rat_tree, &search_box, NULL,
                      LOCtoPVrat_callback, &info);
          else
            return true;
        }
      PVList.Location++;
    }
  return false;
}

/*!
 * \brief Find all connections between LO at the current list position
 * and new LOs.
 */
static bool
LookupLOConnectionsToLOList (int flag, bool AndRats)
{
  bool done;
  Cardinal i, group, layer, ratposition,
    lineposition[MAX_LAYER],
    polyposition[MAX_LAYER], arcposition[MAX_LAYER], padposition[2];

  /* copy the current LO list positions; the original data is changed
   * by 'LookupPVConnectionsToLOList()' which has to check the same
   * list entries plus the new ones
   */
  for (i = 0; i < max_copper_layer; i++)
    {
      lineposition[i] = LineList[i].Location;
      polyposition[i] = PolygonList[i].Location;
      arcposition[i]  = ArcList[i].Location;
    }
  for (i = 0; i < 2; i++)
    padposition[i] = PadList[i].Location;
  ratposition = RatList.Location;

  /* loop over all new LOs in the list; recurse until no
   * more new connections in the layergroup were found
   */
  do
    {
      Cardinal *position;

      if (AndRats)
        {
          position = &ratposition;
          for (; *position < RatList.Number; (*position)++)
            {
              group = RATLIST_ENTRY (*position)->group1;
              if (LookupLOConnectionsToRatEnd
                  (&(RATLIST_ENTRY (*position)->Point1), group, flag))
                return (true);
              group = RATLIST_ENTRY (*position)->group2;
              if (LookupLOConnectionsToRatEnd
                  (&(RATLIST_ENTRY (*position)->Point2), group, flag))
                return (true);
            }
        }
      /* loop over all layergroups */
      for (group = 0; group < max_group; group++)
        {
          Cardinal entry;

          for (entry = 0; entry < PCB->LayerGroups.Number[group]; entry++)
            {
              layer = PCB->LayerGroups.Entries[group][entry];

              /* be aware that the layer number equal max_copper_layer
               * and max_copper_layer+1 have a special meaning for pads
               */
              if (layer < max_copper_layer)
                {
                  /* try all new lines */
                  position = &lineposition[layer];
                  for (; *position < LineList[layer].Number; (*position)++)
                    if (LookupLOConnectionsToLine
                        (LINELIST_ENTRY (layer, *position), group, flag, true, AndRats))
                      return (true);

                  /* try all new arcs */
                  position = &arcposition[layer];
                  for (; *position < ArcList[layer].Number; (*position)++)
                    if (LookupLOConnectionsToArc
                        (ARCLIST_ENTRY (layer, *position), group, flag, AndRats))
                      return (true);

                  /* try all new polygons */
                  position = &polyposition[layer];
                  for (; *position < PolygonList[layer].Number; (*position)++)
                    if (LookupLOConnectionsToPolygon
                        (POLYGONLIST_ENTRY (layer, *position), group, flag, AndRats))
                      return (true);
                }
              else
                {
                  /* try all new pads */
                  layer -= max_copper_layer;
                  if (layer > 1)
                    {
                      Message (_("bad layer number %d max_copper_layer=%d in find.c\n"),
                               layer, max_copper_layer);
                      return false;
                    }
                  position = &padposition[layer];
                  for (; *position < PadList[layer].Number; (*position)++)
                    if (LookupLOConnectionsToPad
                        (PADLIST_ENTRY (layer, *position), group, flag, AndRats))
                      return (true);
                }
            }
        }

      /* check if all lists are done; Later for-loops
       * may have changed the prior lists
       */
      done = !AndRats || ratposition >= RatList.Number;
      done = done && padposition[0] >= PadList[0].Number &&
                     padposition[1] >= PadList[1].Number;
      for (layer = 0; layer < max_copper_layer; layer++)
        done = done &&
               lineposition[layer] >= LineList[layer].Number &&
               arcposition[layer]  >= ArcList[layer].Number &&
               polyposition[layer] >= PolygonList[layer].Number;
    }
  while (!done);
  return (false);
}

static int
pv_pv_callback (const BoxType * b, void *cl)
{
  PinType *pin = (PinType *) b;
  struct pv_info *i = (struct pv_info *) cl;
  bool pv_overlap = false;
  Cardinal l;

  if (VIA_IS_BURIED (pin) && VIA_IS_BURIED (i->pv))
    {
      for (l = pin->BuriedFrom ; l <= pin->BuriedTo; l++)
        {
	   if (ViaIsOnLayerGroup (i->pv, GetLayerGroupNumberByNumber (l)))
	     {
	       pv_overlap = true;
	       break;
	     }
	}
      if (!pv_overlap)
	return 0;
    }

  if (!TEST_FLAG (i->flag, pin) && PinPinIntersect (i->pv, pin, Bloat))
    {
      if (TEST_FLAG (HOLEFLAG, pin) || TEST_FLAG (HOLEFLAG, i->pv))
        {
          SET_FLAG (WARNFLAG, pin);
          Settings.RatWarn = true;
          if (pin->Element)
            Message (_("WARNING: Hole too close to pin.\n"));
          else
            Message (_("WARNING: Hole too close to via.\n"));
        }
      else if (ADD_PV_TO_LIST (pin, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

/*!
 * \brief Searches for new PVs that are connected to PVs on the list.
 */
static bool
LookupPVConnectionsToPVList (int flag)
{
  Cardinal save_place;
  struct pv_info info;

  info.flag = flag;

  /* loop over all PVs on list */
  save_place = PVList.Location;
  while (PVList.Location < PVList.Number)
    {
      BoxType search_box;

      /* get pointer to data */
      info.pv = PVLIST_ENTRY (PVList.Location);
      search_box = expand_bounds ((BoxType *)info.pv);

      if (setjmp (info.env) == 0)
        r_search (PCB->Data->via_tree, &search_box, NULL,
                  pv_pv_callback, &info);
      else
        return true;
      if (setjmp (info.env) == 0)
        r_search (PCB->Data->pin_tree, &search_box, NULL,
                  pv_pv_callback, &info);
      else
        return true;
      PVList.Location++;
    }
  PVList.Location = save_place;
  return (false);
}

struct lo_info
{
  Cardinal layer;
  LineType *line;
  PadType *pad;
  ArcType *arc;
  PolygonType *polygon;
  RatType *rat;
  int flag;
  jmp_buf env;
};

static int
pv_line_callback (const BoxType * b, void *cl)
{
  PinType *pv = (PinType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!ViaIsOnLayerGroup (pv, GetLayerGroupNumberByNumber (i->layer)))
    return 0;

  if (!TEST_FLAG (i->flag, pv) && PinLineIntersect (pv, i->line, Bloat))
    {
      if (TEST_FLAG (HOLEFLAG, pv))
        {
          SET_FLAG (WARNFLAG, pv);
          Settings.RatWarn = true;
          Message (_("WARNING: Hole too close to line.\n"));
        }
      else if (ADD_PV_TO_LIST (pv, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
pv_pad_callback (const BoxType * b, void *cl)
{
  PinType *pv = (PinType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!ViaIsOnLayerGroup (pv, GetLayerGroupNumberBySide (i->layer)))
    return 0;

  if (!TEST_FLAG (i->flag, pv) && IsPinOnPad(pv, i->pad, Bloat))
    {
      if (TEST_FLAG (HOLEFLAG, pv))
        {
          SET_FLAG (WARNFLAG, pv);
          Settings.RatWarn = true;
          Message (_("WARNING: Hole too close to pad.\n"));
        }
      else if (ADD_PV_TO_LIST (pv, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
pv_arc_callback (const BoxType * b, void *cl)
{
  PinType *pv = (PinType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!ViaIsOnLayerGroup (pv, GetLayerGroupNumberByNumber (i->layer)))
    return 0;

  if (!TEST_FLAG (i->flag, pv) && IsPinOnArc(pv, i->arc, Bloat))
    {
      if (TEST_FLAG (HOLEFLAG, pv))
        {
          SET_FLAG (WARNFLAG, pv);
          Settings.RatWarn = true;
          Message (_("WARNING: Hole touches arc.\n"));
        }
      else if (ADD_PV_TO_LIST (pv, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
pv_poly_callback (const BoxType * b, void *cl)
{
  PinType *pv = (PinType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!ViaIsOnLayerGroup (pv, GetLayerGroupNumberByNumber (i->layer)))
    return 0;

  /* note that holes in polygons are ok, so they don't generate warnings. */
  if (!TEST_FLAG (i->flag, pv) && !TEST_FLAG (HOLEFLAG, pv) &&
                                  (TEST_THERM (i->layer, pv) ||
                                   !TEST_FLAG (CLEARPOLYFLAG, i->polygon) ||
                                   !pv->Clearance))
    {
      if (TEST_FLAG (SQUAREFLAG, pv))
        {
          Coord x1, x2, y1, y2;
          x1 = pv->X - (PIN_SIZE (pv) + 1 + Bloat) / 2;
          x2 = pv->X + (PIN_SIZE (pv) + 1 + Bloat) / 2;
          y1 = pv->Y - (PIN_SIZE (pv) + 1 + Bloat) / 2;
          y2 = pv->Y + (PIN_SIZE (pv) + 1 + Bloat) / 2;
          if (IsRectangleInPolygon (x1, y1, x2, y2, i->polygon)
              && ADD_PV_TO_LIST (pv, i->flag))
            longjmp (i->env, 1);
        }
      else if (TEST_FLAG (OCTAGONFLAG, pv))
        {
          POLYAREA *oct = OctagonPoly (pv->X, pv->Y, PIN_SIZE (pv) / 2);
          if (isects (oct, i->polygon, true) && ADD_PV_TO_LIST (pv, i->flag))
            longjmp (i->env, 1);
        }
      else
        {
          if (IsPointInPolygon
              (pv->X, pv->Y, PIN_SIZE (pv) * 0.5 + Bloat, i->polygon)
              && ADD_PV_TO_LIST (pv, i->flag))
            longjmp (i->env, 1);
        }
    }
  return 0;
}

static int
pv_rat_callback (const BoxType * b, void *cl)
{
  PinType *pv = (PinType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  /* rats can't cause DRC so there is no early exit */
  if (!TEST_FLAG (i->flag, pv) && IsPinOnRat(pv, i->rat))
    ADD_PV_TO_LIST (pv, i->flag);
  return 0;
}

/*!
 * \brief Searches for new PVs that are connected to NEW LOs on the list.
 *
 * This routine updates the position counter of the lists too.
 */
static bool
LookupPVConnectionsToLOList (int flag, bool AndRats)
{
  Cardinal layer_no;
  struct lo_info info;

  info.flag = flag;

  /* loop over all layers */
  for (layer_no = 0; layer_no < max_copper_layer; layer_no++)
    {
      LayerType *layer = LAYER_PTR (layer_no);

      if (layer->no_drc)
                       continue;
      /* do nothing if there are no PV's */
      if (TotalP + TotalV == 0)
        {
          LineList[layer_no].Location = LineList[layer_no].Number;
          ArcList[layer_no].Location = ArcList[layer_no].Number;
          PolygonList[layer_no].Location = PolygonList[layer_no].Number;
          continue;
        }

      info.layer = layer_no;
      /* check all lines */
      while (LineList[layer_no].Location < LineList[layer_no].Number)
        {
          BoxType search_box;

          info.line = LINELIST_ENTRY (layer_no, LineList[layer_no].Location);
          search_box = expand_bounds ((BoxType *)info.line);

          if (setjmp (info.env) == 0)
            r_search (PCB->Data->via_tree, &search_box, NULL,
                      pv_line_callback, &info);
          else
            return true;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pin_tree, &search_box, NULL,
                      pv_line_callback, &info);
          else
            return true;
          LineList[layer_no].Location++;
        }

      /* check all arcs */
      while (ArcList[layer_no].Location < ArcList[layer_no].Number)
        {
          BoxType search_box;

          info.arc = ARCLIST_ENTRY (layer_no, ArcList[layer_no].Location);
          search_box = expand_bounds ((BoxType *)info.arc);

          if (setjmp (info.env) == 0)
            r_search (PCB->Data->via_tree, &search_box, NULL,
                      pv_arc_callback, &info);
          else
            return true;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pin_tree, &search_box, NULL,
                      pv_arc_callback, &info);
          else
            return true;
          ArcList[layer_no].Location++;
        }

      /* now all polygons */
      info.layer = layer_no;
      while (PolygonList[layer_no].Location < PolygonList[layer_no].Number)
        {
          BoxType search_box;

          info.polygon = POLYGONLIST_ENTRY (layer_no, PolygonList[layer_no].Location);
          search_box = expand_bounds ((BoxType *)info.polygon);

          if (setjmp (info.env) == 0)
            r_search (PCB->Data->via_tree, &search_box, NULL,
                      pv_poly_callback, &info);
          else
            return true;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pin_tree, &search_box, NULL,
                      pv_poly_callback, &info);
          else
            return true;
          PolygonList[layer_no].Location++;
        }
    }

  /* loop over all pad-layers */
  for (layer_no = 0; layer_no < 2; layer_no++)
    {
      /* do nothing if there are no PV's */
      if (TotalP + TotalV == 0)
        {
          PadList[layer_no].Location = PadList[layer_no].Number;
          continue;
        }

      /* check all pads; for a detailed description see
       * the handling of lines in this subroutine
       */
      while (PadList[layer_no].Location < PadList[layer_no].Number)
        {
          BoxType search_box;

          info.layer = layer_no;
          info.pad = PADLIST_ENTRY (layer_no, PadList[layer_no].Location);
          search_box = expand_bounds ((BoxType *)info.pad);

          if (setjmp (info.env) == 0)
            r_search (PCB->Data->via_tree, &search_box, NULL,
                      pv_pad_callback, &info);
          else
            return true;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pin_tree, &search_box, NULL,
                      pv_pad_callback, &info);
          else
            return true;
          PadList[layer_no].Location++;
        }
    }

  /* do nothing if there are no PV's */
  if (TotalP + TotalV == 0)
    RatList.Location = RatList.Number;

  /* check all rat-lines */
  if (AndRats)
    {
      while (RatList.Location < RatList.Number)
        {
          info.rat = RATLIST_ENTRY (RatList.Location);
          r_search_pt (PCB->Data->via_tree, & info.rat->Point1, 1, NULL,
                    pv_rat_callback, &info);
          r_search_pt (PCB->Data->via_tree, & info.rat->Point2, 1, NULL,
                    pv_rat_callback, &info);
          r_search_pt (PCB->Data->pin_tree, & info.rat->Point1, 1, NULL,
                    pv_rat_callback, &info);
          r_search_pt (PCB->Data->pin_tree, & info.rat->Point2, 1, NULL,
                    pv_rat_callback, &info);

          RatList.Location++;
        }
    }
  return (false);
}


static int
LOCtoArcLine_callback (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (i->flag, line) && LineArcIntersect (line, i->arc, Bloat))
    {
      if (ADD_LINE_TO_LIST (i->layer, line, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoArcArc_callback (const BoxType * b, void *cl)
{
  ArcType *arc = (ArcType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!arc->Thickness)
    return 0;
  if (!TEST_FLAG (i->flag, arc) && ArcArcIntersect (i->arc, arc, Bloat))
    {
      if (ADD_ARC_TO_LIST (i->layer, arc, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoArcPad_callback (const BoxType * b, void *cl)
{
  PadType *pad = (PadType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (i->flag, pad) && i->layer ==
      (TEST_FLAG (ONSOLDERFLAG, pad) ? BOTTOM_SIDE : TOP_SIDE)
      && ArcPadIntersect (i->arc, pad, Bloat) && ADD_PAD_TO_LIST (i->layer, pad, i->flag))
    longjmp (i->env, 1);
  return 0;
}

/*!
 * \brief Searches all LOs that are connected to the given arc on the
 * given layergroup.
 *
 * All found connections are added to the list.
 *
 * The notation that is used is:\n
 * Xij means Xj at arc i.
 */
static bool
LookupLOConnectionsToArc (ArcType *Arc, Cardinal LayerGroup, int flag, bool AndRats)
{
  Cardinal entry;
  struct lo_info info;
  BoxType search_box;

  info.flag = flag;
  info.arc = Arc;
  search_box = expand_bounds ((BoxType *)info.arc);

  /* loop over all layers of the group */
  for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++)
    {
      Cardinal layer_no;
      LayerType *layer;
      GList *i;

      layer_no = PCB->LayerGroups.Entries[LayerGroup][entry];
      layer = LAYER_PTR (layer_no);

      /* handle normal layers */
      if (layer_no < max_copper_layer)
        {
          info.layer = layer_no;
          /* add arcs */
          if (setjmp (info.env) == 0)
            r_search (layer->line_tree, &search_box,
                      NULL, LOCtoArcLine_callback, &info);
          else
            return true;

          if (setjmp (info.env) == 0)
            r_search (layer->arc_tree, &search_box,
                      NULL, LOCtoArcArc_callback, &info);
          else
            return true;

          /* now check all polygons */
          for (i = layer->Polygon; i != NULL; i = g_list_next (i))
            {
              PolygonType *polygon = i->data;
              if (!TEST_FLAG (flag, polygon) && IsArcInPolygon (Arc, polygon, Bloat)
                  && ADD_POLYGON_TO_LIST (layer_no, polygon, flag))
                return true;
            }
        }
      else
        {
          info.layer = layer_no - max_copper_layer;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pad_tree, &search_box, NULL,
                      LOCtoArcPad_callback, &info);
          else
            return true;
        }
    }
  return (false);
}

static int
LOCtoLineLine_callback (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (i->flag, line) && LineLineIntersect (i->line, line, Bloat))
    {
      if (ADD_LINE_TO_LIST (i->layer, line, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoLineArc_callback (const BoxType * b, void *cl)
{
  ArcType *arc = (ArcType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!arc->Thickness)
    return 0;
  if (!TEST_FLAG (i->flag, arc) && LineArcIntersect (i->line, arc, Bloat))
    {
      if (ADD_ARC_TO_LIST (i->layer, arc, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoLineRat_callback (const BoxType * b, void *cl)
{
  RatType *rat = (RatType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (i->flag, rat))
    {
      if ((rat->group1 == i->layer)
          && IsRatPointOnLineEnd (&rat->Point1, i->line))
        {
          if (ADD_RAT_TO_LIST (rat, i->flag))
            longjmp (i->env, 1);
        }
      else if ((rat->group2 == i->layer)
               && IsRatPointOnLineEnd (&rat->Point2, i->line))
        {
          if (ADD_RAT_TO_LIST (rat, i->flag))
            longjmp (i->env, 1);
        }
    }
  return 0;
}

static int
LOCtoLinePad_callback (const BoxType * b, void *cl)
{
  PadType *pad = (PadType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (i->flag, pad) && i->layer ==
      (TEST_FLAG (ONSOLDERFLAG, pad) ? BOTTOM_SIDE : TOP_SIDE)
      && LinePadIntersect (i->line, pad, Bloat) && ADD_PAD_TO_LIST (i->layer, pad, i->flag))
    longjmp (i->env, 1);
  return 0;
}

/*!
 * \brief Searches all LOs that are connected to the given line on the
 * given layergroup.
 *
 * All found connections are added to the list.
 *
 * The notation that is used is:
 * Xij means Xj at line i.
 */
static bool
LookupLOConnectionsToLine (LineType *Line, Cardinal LayerGroup,
                           int flag, bool PolysTo, bool AndRats)
{
  Cardinal entry;
  struct lo_info info;
  BoxType search_box;

  info.flag = flag;
  info.layer = LayerGroup;
  info.line = Line;
  search_box = expand_bounds ((BoxType *)info.line);

  if (AndRats)
    {
      /* add the new rat lines */
      if (setjmp (info.env) == 0)
        r_search (PCB->Data->rat_tree, &search_box, NULL,
                  LOCtoLineRat_callback, &info);
      else
        return true;
    }

  /* loop over all layers of the group */
  for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++)
    {
      Cardinal layer_no;
      LayerType *layer;

      layer_no = PCB->LayerGroups.Entries[LayerGroup][entry];
      layer = LAYER_PTR (layer_no);

      /* handle normal layers */
      if (layer_no < max_copper_layer)
        {
          info.layer = layer_no;
          /* add lines */
          if (setjmp (info.env) == 0)
            r_search (layer->line_tree, &search_box,
                      NULL, LOCtoLineLine_callback, &info);
          else
            return true;
          /* add arcs */
          if (setjmp (info.env) == 0)
            r_search (layer->arc_tree, &search_box,
                      NULL, LOCtoLineArc_callback, &info);
          else
            return true;
          /* now check all polygons */
          if (PolysTo)
            {
              GList *i;
              for (i = layer->Polygon; i != NULL; i = g_list_next (i))
                {
                  PolygonType *polygon = i->data;
                  if (!TEST_FLAG (flag, polygon) && IsLineInPolygon (Line, polygon, Bloat)
                      && ADD_POLYGON_TO_LIST (layer_no, polygon, flag))
                    return true;
                }
            }
        }
      else
        {
          /* handle special 'pad' layers */
          info.layer = layer_no - max_copper_layer;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pad_tree, &search_box, NULL,
                      LOCtoLinePad_callback, &info);
          else
            return true;
        }
    }
  return (false);
}

struct rat_info
{
  Cardinal layer;
  PointType *Point;
  int flag;
  jmp_buf env;
};

static int
LOCtoRat_callback (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  struct rat_info *i = (struct rat_info *) cl;

  if (!TEST_FLAG (i->flag, line) &&
      ((line->Point1.X == i->Point->X &&
        line->Point1.Y == i->Point->Y) ||
       (line->Point2.X == i->Point->X && line->Point2.Y == i->Point->Y)))
    {
      if (ADD_LINE_TO_LIST (i->layer, line, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}
static int
PolygonToRat_callback (const BoxType * b, void *cl)
{
  PolygonType *polygon = (PolygonType *) b;
  struct rat_info *i = (struct rat_info *) cl;

  if (!TEST_FLAG (i->flag, polygon) && polygon->Clipped &&
      (i->Point->X == polygon->Clipped->contours->head.point[0]) &&
      (i->Point->Y == polygon->Clipped->contours->head.point[1]))
    {
      if (ADD_POLYGON_TO_LIST (i->layer, polygon, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPad_callback (const BoxType * b, void *cl)
{
  PadType *pad = (PadType *) b;
  struct rat_info *i = (struct rat_info *) cl;

  if (!TEST_FLAG (i->flag, pad) && i->layer ==
	(TEST_FLAG (ONSOLDERFLAG, pad) ? BOTTOM_SIDE : TOP_SIDE) &&
      ((pad->Point1.X == i->Point->X && pad->Point1.Y == i->Point->Y) ||
       (pad->Point2.X == i->Point->X && pad->Point2.Y == i->Point->Y) ||
       ((pad->Point1.X + pad->Point2.X) / 2 == i->Point->X &&
        (pad->Point1.Y + pad->Point2.Y) / 2 == i->Point->Y)) &&
      ADD_PAD_TO_LIST (i->layer, pad, i->flag))
    longjmp (i->env, 1);
  return 0;
}

/*!
 * \brief Searches all LOs that are connected to the given rat-line on
 * the given layergroup.
 *
 * All found connections are added to the list.
 *
 * The notation that is used is:
 * Xij means Xj at line i.
 */
static bool
LookupLOConnectionsToRatEnd (PointType *Point, Cardinal LayerGroup, int flag)
{
  Cardinal entry;
  struct rat_info info;

  info.flag = flag;
  info.Point = Point;
  /* loop over all layers of this group */
  for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++)
    {
      Cardinal layer_no;
      LayerType *layer;

      layer_no = PCB->LayerGroups.Entries[LayerGroup][entry];
      layer = LAYER_PTR (layer_no);
      /* handle normal layers 
         rats don't ever touch
         arcs by definition
       */

      if (layer_no < max_copper_layer)
        {
          info.layer = layer_no;
          if (setjmp (info.env) == 0)
            r_search_pt (layer->line_tree, Point, 1, NULL,
                      LOCtoRat_callback, &info);
          else
            return true;
          if (setjmp (info.env) == 0)
            r_search_pt (layer->polygon_tree, Point, 1,
                      NULL, PolygonToRat_callback, &info);
        }
      else
        {
          /* handle special 'pad' layers */
          info.layer = layer_no - max_copper_layer;
          if (setjmp (info.env) == 0)
            r_search_pt (PCB->Data->pad_tree, Point, 1, NULL,
                      LOCtoPad_callback, &info);
          else
            return true;
        }
    }
  return (false);
}

static int
LOCtoPadLine_callback (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (i->flag, line) && LinePadIntersect (line, i->pad, Bloat))
    {
      if (ADD_LINE_TO_LIST (i->layer, line, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPadArc_callback (const BoxType * b, void *cl)
{
  ArcType *arc = (ArcType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!arc->Thickness)
    return 0;
  if (!TEST_FLAG (i->flag, arc) && ArcPadIntersect (arc, i->pad, Bloat))
    {
      if (ADD_ARC_TO_LIST (i->layer, arc, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPadPoly_callback (const BoxType * b, void *cl)
{
  PolygonType *polygon = (PolygonType *) b;
  struct lo_info *i = (struct lo_info *) cl;


  if (!TEST_FLAG (i->flag, polygon) &&
      (!TEST_FLAG (CLEARPOLYFLAG, polygon) || !i->pad->Clearance))
    {
      if (IsPadInPolygon (i->pad, polygon, Bloat) &&
          ADD_POLYGON_TO_LIST (i->layer, polygon, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPadRat_callback (const BoxType * b, void *cl)
{
  RatType *rat = (RatType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (i->flag, rat))
    {
      if (rat->group1 == i->layer &&
	  ((rat->Point1.X == i->pad->Point1.X && rat->Point1.Y == i->pad->Point1.Y) ||
	   (rat->Point1.X == i->pad->Point2.X && rat->Point1.Y == i->pad->Point2.Y) ||
	   (rat->Point1.X == (i->pad->Point1.X + i->pad->Point2.X) / 2 &&
	    rat->Point1.Y == (i->pad->Point1.Y + i->pad->Point2.Y) / 2)))
        {
          if (ADD_RAT_TO_LIST (rat, i->flag))
            longjmp (i->env, 1);
        }
      else if (rat->group2 == i->layer &&
	       ((rat->Point2.X == i->pad->Point1.X && rat->Point2.Y == i->pad->Point1.Y) ||
		(rat->Point2.X == i->pad->Point2.X && rat->Point2.Y == i->pad->Point2.Y) ||
		(rat->Point2.X == (i->pad->Point1.X + i->pad->Point2.X) / 2 &&
		 rat->Point2.Y == (i->pad->Point1.Y + i->pad->Point2.Y) / 2)))
        {
          if (ADD_RAT_TO_LIST (rat, i->flag))
            longjmp (i->env, 1);
        }
    }
  return 0;
}

static int
LOCtoPadPad_callback (const BoxType * b, void *cl)
{
  PadType *pad = (PadType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (i->flag, pad) && i->layer ==
      (TEST_FLAG (ONSOLDERFLAG, pad) ? BOTTOM_SIDE : TOP_SIDE)
      && PadPadIntersect (pad, i->pad, Bloat) && ADD_PAD_TO_LIST (i->layer, pad, i->flag))
    longjmp (i->env, 1);
  return 0;
}

/*!
 * \brief Searches all LOs that are connected to the given pad on the
 * given layergroup.
 *
 * All found connections are added to the list.
 */
static bool
LookupLOConnectionsToPad (PadType *Pad, Cardinal LayerGroup, int flag, bool AndRats)
{
  Cardinal entry;
  struct lo_info info;
  BoxType search_box;

  if (!TEST_FLAG (SQUAREFLAG, Pad))
    return (LookupLOConnectionsToLine ((LineType *) Pad, LayerGroup, flag, false, AndRats));

  info.flag = flag;
  info.pad = Pad;
  search_box = expand_bounds ((BoxType *)info.pad);

  /* add the new rat lines */
  info.layer = LayerGroup;

  if (AndRats)
    {
      if (setjmp (info.env) == 0)
        r_search (PCB->Data->rat_tree, &search_box, NULL,
                  LOCtoPadRat_callback, &info);
      else
        return true;
    }

  /* loop over all layers of the group */
  for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++)
    {
      Cardinal layer_no;
      LayerType *layer;

      layer_no = PCB->LayerGroups.Entries[LayerGroup][entry];
      layer = LAYER_PTR (layer_no);
      /* handle normal layers */
      if (layer_no < max_copper_layer)
        {
          info.layer = layer_no;
          /* add lines */
          if (setjmp (info.env) == 0)
            r_search (layer->line_tree, &search_box,
                      NULL, LOCtoPadLine_callback, &info);
          else
            return true;
          /* add arcs */
          if (setjmp (info.env) == 0)
            r_search (layer->arc_tree, &search_box,
                      NULL, LOCtoPadArc_callback, &info);
          else
            return true;
          /* add polygons */
          if (setjmp (info.env) == 0)
            r_search (layer->polygon_tree, &search_box,
                      NULL, LOCtoPadPoly_callback, &info);
          else
            return true;
        }
      else
        {
          /* handle special 'pad' layers */
          info.layer = layer_no - max_copper_layer;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pad_tree, &search_box, NULL,
                      LOCtoPadPad_callback, &info);
          else
            return true;
        }

    }
  return (false);
}

static int
LOCtoPolyLine_callback (const BoxType * b, void *cl)
{
  LineType *line = (LineType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (i->flag, line) && IsLineInPolygon (line, i->polygon, Bloat))
    {
      if (ADD_LINE_TO_LIST (i->layer, line, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPolyArc_callback (const BoxType * b, void *cl)
{
  ArcType *arc = (ArcType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!arc->Thickness)
    return 0;
  if (!TEST_FLAG (i->flag, arc) && IsArcInPolygon (arc, i->polygon, Bloat))
    {
      if (ADD_ARC_TO_LIST (i->layer, arc, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPolyPad_callback (const BoxType * b, void *cl)
{
  PadType *pad = (PadType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (i->flag, pad) && i->layer ==
      (TEST_FLAG (ONSOLDERFLAG, pad) ? BOTTOM_SIDE : TOP_SIDE)
      && IsPadInPolygon (pad, i->polygon, Bloat))
    {
      if (ADD_PAD_TO_LIST (i->layer, pad, i->flag))
        longjmp (i->env, 1);
    }
  return 0;
}

static int
LOCtoPolyRat_callback (const BoxType * b, void *cl)
{
  RatType *rat = (RatType *) b;
  struct lo_info *i = (struct lo_info *) cl;

  if (!TEST_FLAG (i->flag, rat))
    {
      if ((rat->Point1.X == (i->polygon->Clipped->contours->head.point[0]) &&
           rat->Point1.Y == (i->polygon->Clipped->contours->head.point[1]) &&
           rat->group1 == i->layer) ||
          (rat->Point2.X == (i->polygon->Clipped->contours->head.point[0]) &&
           rat->Point2.Y == (i->polygon->Clipped->contours->head.point[1]) &&
           rat->group2 == i->layer))
        if (ADD_RAT_TO_LIST (rat, i->flag))
          longjmp (i->env, 1);
    }
  return 0;
}


/*!
 * \brief Looks up LOs that are connected to the given polygon on the
 * given layergroup.
 *
 * All found connections are added to the list.
 */
static bool
LookupLOConnectionsToPolygon (PolygonType *Polygon, Cardinal LayerGroup, int flag, bool AndRats)
{
  Cardinal entry;
  struct lo_info info;
  BoxType search_box;

  if (!Polygon->Clipped)
    return false;

  info.flag = flag;
  info.polygon = Polygon;
  search_box = expand_bounds ((BoxType *)info.polygon);

  info.layer = LayerGroup;

  /* check rats */
  if (AndRats)
    {
      if (setjmp (info.env) == 0)
        r_search (PCB->Data->rat_tree, &search_box, NULL,
                  LOCtoPolyRat_callback, &info);
      else
        return true;
    }

/* loop over all layers of the group */
  for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++)
    {
      Cardinal layer_no;
      LayerType *layer;

      layer_no = PCB->LayerGroups.Entries[LayerGroup][entry];
      layer = LAYER_PTR (layer_no);

      /* handle normal layers */
      if (layer_no < max_copper_layer)
        {
          GList *i;

          /* check all polygons */
          for (i = layer->Polygon; i != NULL; i = g_list_next (i))
            {
              PolygonType *polygon = i->data;
              if (!TEST_FLAG (flag, polygon)
                  && IsPolygonInPolygon (polygon, Polygon, Bloat)
                  && ADD_POLYGON_TO_LIST (layer_no, polygon, flag))
                return true;
            }

          info.layer = layer_no;
          /* check all lines */
          if (setjmp (info.env) == 0)
            r_search (layer->line_tree, &search_box,
                      NULL, LOCtoPolyLine_callback, &info);
          else
            return true;
          /* check all arcs */
          if (setjmp (info.env) == 0)
            r_search (layer->arc_tree, &search_box,
                      NULL, LOCtoPolyArc_callback, &info);
          else
            return true;
        }
      else
        {
          info.layer = layer_no - max_copper_layer;
          if (setjmp (info.env) == 0)
            r_search (PCB->Data->pad_tree, &search_box,
                      NULL, LOCtoPolyPad_callback, &info);
          else
            return true;
        }
    }
  return (false);
}
/*!
 * \brief Writes the several names of an element to a file.
 */
static void
PrintElementNameList (ElementType *Element, FILE * FP)
{
  static DynamicStringType cname, pname, vname;

  CreateQuotedString (&cname, (char *)EMPTY (DESCRIPTION_NAME (Element)));
  CreateQuotedString (&pname, (char *)EMPTY (NAMEONPCB_NAME (Element)));
  CreateQuotedString (&vname, (char *)EMPTY (VALUE_NAME (Element)));
  fprintf (FP, "(%s %s %s)\n", cname.Data, pname.Data, vname.Data);
}

/*!
 * \brief Writes the several names of an element to a file.
 */
static void
PrintConnectionElementName (ElementType *Element, FILE * FP)
{
  fputs ("Element", FP);
  PrintElementNameList (Element, FP);
  fputs ("{\n", FP);
}

/*!
 * \brief Prints one {pin,pad,via}/element entry of connection lists.
 */
static void
PrintConnectionListEntry (char *ObjName, ElementType *Element,
                          bool FirstOne, FILE * FP)
{
  static DynamicStringType oname;

  CreateQuotedString (&oname, ObjName);
  if (FirstOne)
    fprintf (FP, "\t%s\n\t{\n", oname.Data);
  else
    {
      fprintf (FP, "\t\t%s ", oname.Data);
      if (Element)
        PrintElementNameList (Element, FP);
      else
        fputs ("(__VIA__)\n", FP);
    }
}

/*!
 * \brief Prints all found connections of a pads to file FP
 * the connections are stacked in 'PadList'.
 */
static void
PrintPadConnections (Cardinal Layer, FILE * FP, bool IsFirst)
{
  Cardinal i;
  PadType *ptr;

  if (!PadList[Layer].Number)
    return;

  /* the starting pad */
  if (IsFirst)
    {
      ptr = PADLIST_ENTRY (Layer, 0);
      if (ptr != NULL)
        PrintConnectionListEntry ((char *)UNKNOWN (ptr->Name), NULL, true, FP);
      else
        printf ("Skipping NULL ptr in 1st part of PrintPadConnections\n");
    }

  /* we maybe have to start with i=1 if we are handling the
   * starting-pad itself
   */
  for (i = IsFirst ? 1 : 0; i < PadList[Layer].Number; i++)
    {
      ptr = PADLIST_ENTRY (Layer, i);
      if (ptr != NULL)
        PrintConnectionListEntry ((char *)EMPTY (ptr->Name), (ElementType *)ptr->Element, false, FP);
      else
        printf ("Skipping NULL ptr in 2nd part of PrintPadConnections\n");
    }
}

/*!
 * \brief Prints all found connections of a pin to file FP
 * the connections are stacked in 'PVList'.
 */
static void
PrintPinConnections (FILE * FP, bool IsFirst)
{
  Cardinal i;
  PinType *pv;

  if (!PVList.Number)
    return;

  if (IsFirst)
    {
      /* the starting pin */
      pv = PVLIST_ENTRY (0);
      PrintConnectionListEntry ((char *)EMPTY (pv->Name), NULL, true, FP);
    }

  /* we maybe have to start with i=1 if we are handling the
   * starting-pin itself
   */
  for (i = IsFirst ? 1 : 0; i < PVList.Number; i++)
    {
      /* get the elements name or assume that its a via */
      pv = PVLIST_ENTRY (i);
      PrintConnectionListEntry ((char *)EMPTY (pv->Name), (ElementType *)pv->Element, false, FP);
    }
}

/*!
 * \brief Checks if all lists of new objects are handled.
 */
static bool
ListsEmpty (bool AndRats)
{
  bool empty;
  int i;

  empty = (PVList.Location >= PVList.Number);
  if (AndRats)
    empty = empty && (RatList.Location >= RatList.Number);
  for (i = 0; i < max_copper_layer && empty; i++)
    if (!LAYER_PTR (i)->no_drc)
      empty = empty && LineList[i].Location >= LineList[i].Number
        && ArcList[i].Location >= ArcList[i].Number
        && PolygonList[i].Location >= PolygonList[i].Number;
  return (empty);
}

static void
reassign_no_drc_flags (void)
{
  int layer;

  for (layer = 0; layer < max_copper_layer; layer++)
    {
      LayerType *l = LAYER_PTR (layer);
      l->no_drc = AttributeGet (l, "PCB::skip-drc") != NULL;
    }
}




/*!
 * \brief Loops till no more connections are found.
 */
bool
DoIt (int flag, bool AndRats, bool AndDraw)
{
  bool newone = false;
  reassign_no_drc_flags ();
  do
    {
      /* lookup connections; these are the steps (2) to (4)
       * from the description
       */
      newone = LookupPVConnectionsToPVList (flag) ||
               LookupLOConnectionsToPVList (flag, AndRats) ||
               LookupLOConnectionsToLOList (flag, AndRats) ||
               LookupPVConnectionsToLOList (flag, AndRats);
      if (AndDraw)
        DrawNewConnections ();
    }
  while (!newone && !ListsEmpty (AndRats));
  if (AndDraw)
    Draw ();
  return (newone);
}

/*!
 * \brief Prints all unused pins of an element to file FP.
 */
static bool
PrintAndSelectUnusedPinsAndPadsOfElement (ElementType *Element, FILE * FP, int flag)
{
  bool first = true;
  Cardinal number;
  static DynamicStringType oname;

  /* check all pins in element */

  PIN_LOOP (Element);
  {
    if (!TEST_FLAG (HOLEFLAG, pin))
      {
        /* pin might have bee checked before, add to list if not */
        if (!TEST_FLAG (flag, pin) && FP)
          {
            int i;
            if (ADD_PV_TO_LIST (pin, flag))
              return true;
            DoIt (flag, true, true);
            number = PadList[TOP_SIDE].Number
              + PadList[BOTTOM_SIDE].Number + PVList.Number;
            /* the pin has no connection if it's the only
             * list entry; don't count vias
             */
            for (i = 0; i < PVList.Number; i++)
              if (!PVLIST_ENTRY (i)->Element)
                number--;
            if (number == 1)
              {
                /* output of element name if not already done */
                if (first)
                  {
                    PrintConnectionElementName (Element, FP);
                    first = false;
                  }

                /* write name to list and draw selected object */
                CreateQuotedString (&oname, (char *)EMPTY (pin->Name));
                fprintf (FP, "\t%s\n", oname.Data);
                SET_FLAG (SELECTEDFLAG, pin);
                DrawPin (pin);
              }

            /* reset found objects for the next pin */
            if (PrepareNextLoop (FP))
              return (true);
          }
      }
  }
  END_LOOP;

  /* check all pads in element */
  PAD_LOOP (Element);
  {
    /* lookup pad in list */
    /* pad might has bee checked before, add to list if not */
    if (!TEST_FLAG (flag, pad) && FP)
      {
        int i;
        if (ADD_PAD_TO_LIST (TEST_FLAG (ONSOLDERFLAG, pad)
                             ? BOTTOM_SIDE : TOP_SIDE, pad, flag))
          return true;
        DoIt (flag, true, true);
        number = PadList[TOP_SIDE].Number
          + PadList[BOTTOM_SIDE].Number + PVList.Number;
        /* the pin has no connection if it's the only
         * list entry; don't count vias
         */
        for (i = 0; i < PVList.Number; i++)
          if (!PVLIST_ENTRY (i)->Element)
            number--;
        if (number == 1)
          {
            /* output of element name if not already done */
            if (first)
              {
                PrintConnectionElementName (Element, FP);
                first = false;
              }

            /* write name to list and draw selected object */
            CreateQuotedString (&oname, (char *)EMPTY (pad->Name));
            fprintf (FP, "\t%s\n", oname.Data);
            SET_FLAG (SELECTEDFLAG, pad);
            DrawPad (pad);
          }

        /* reset found objects for the next pin */
        if (PrepareNextLoop (FP))
          return (true);
      }
  }
  END_LOOP;

  /* print separator if element has unused pins or pads */
  if (!first)
    {
      fputs ("}\n\n", FP);
      SEPARATE (FP);
    }
  return (false);
}

/*!
 * \brief Resets some flags for looking up the next pin/pad.
 */
static bool
PrepareNextLoop (FILE * FP)
{
  Cardinal layer;

  /* reset found LOs for the next pin */
  for (layer = 0; layer < max_copper_layer; layer++)
    {
      LineList[layer].Location = LineList[layer].Number = 0;
      ArcList[layer].Location = ArcList[layer].Number = 0;
      PolygonList[layer].Location = PolygonList[layer].Number = 0;
    }

  /* reset found pads */
  for (layer = 0; layer < 2; layer++)
    PadList[layer].Location = PadList[layer].Number = 0;

  /* reset PVs */
  PVList.Number = PVList.Location = 0;
  RatList.Number = RatList.Location = 0;

  return (false);
}

/*!
 * \brief Finds all connections to the pins of the passed element.
 *
 * The result is written to file FP.
 *
 * \return true if operation was aborted.
 */
static bool
PrintElementConnections (ElementType *Element, FILE * FP, int flag, bool AndDraw)
{
  PrintConnectionElementName (Element, FP);

  /* check all pins in element */
  PIN_LOOP (Element);
  {
    /* pin might have been checked before, add to list if not */
    if (TEST_FLAG (flag, pin))
      {
        PrintConnectionListEntry ((char *)EMPTY (pin->Name), NULL, true, FP);
        fputs ("\t\t__CHECKED_BEFORE__\n\t}\n", FP);
        continue;
      }
    if (ADD_PV_TO_LIST (pin, flag))
      return true;
    DoIt (flag, true, AndDraw);
    /* printout all found connections */
    PrintPinConnections (FP, true);
    PrintPadConnections (TOP_SIDE, FP, false);
    PrintPadConnections (BOTTOM_SIDE, FP, false);
    fputs ("\t}\n", FP);
    if (PrepareNextLoop (FP))
      return (true);
  }
  END_LOOP;

  /* check all pads in element */
  PAD_LOOP (Element);
  {
    Cardinal layer;
    /* pad might have been checked before, add to list if not */
    if (TEST_FLAG (flag, pad))
      {
        PrintConnectionListEntry ((char *)EMPTY (pad->Name), NULL, true, FP);
        fputs ("\t\t__CHECKED_BEFORE__\n\t}\n", FP);
        continue;
      }
    layer = TEST_FLAG (ONSOLDERFLAG, pad) ? BOTTOM_SIDE : TOP_SIDE;
    if (ADD_PAD_TO_LIST (layer, pad, flag))
      return true;
    DoIt (flag, true, AndDraw);
    /* print all found connections */
    PrintPadConnections (layer, FP, true);
    PrintPadConnections (layer ==
                         (TOP_SIDE ? BOTTOM_SIDE : TOP_SIDE),
                         FP, false);
    PrintPinConnections (FP, false);
    fputs ("\t}\n", FP);
    if (PrepareNextLoop (FP))
      return (true);
  }
  END_LOOP;
  fputs ("}\n\n", FP);
  return (false);
}

/*!
 * \brief Draws all new connections which have been found since the
 * routine was called the last time.
 */
static void
DrawNewConnections (void)
{
  int i;
  Cardinal position;

  /* decrement 'i' to keep layerstack order */
  for (i = max_copper_layer - 1; i != -1; i--)
    {
      Cardinal layer = LayerStack[i];

      if (PCB->Data->Layer[layer].On)
        {
          /* draw all new lines */
          position = LineList[layer].DrawLocation;
          for (; position < LineList[layer].Number; position++)
            DrawLine (LAYER_PTR (layer), LINELIST_ENTRY (layer, position));
          LineList[layer].DrawLocation = LineList[layer].Number;

          /* draw all new arcs */
          position = ArcList[layer].DrawLocation;
          for (; position < ArcList[layer].Number; position++)
            DrawArc (LAYER_PTR (layer), ARCLIST_ENTRY (layer, position));
          ArcList[layer].DrawLocation = ArcList[layer].Number;

          /* draw all new polygons */
          position = PolygonList[layer].DrawLocation;
          for (; position < PolygonList[layer].Number; position++)
            DrawPolygon (LAYER_PTR (layer), POLYGONLIST_ENTRY (layer, position));
          PolygonList[layer].DrawLocation = PolygonList[layer].Number;
        }
    }

  /* draw all new pads */
  if (PCB->PinOn)
    for (i = 0; i < 2; i++)
      {
        position = PadList[i].DrawLocation;

        for (; position < PadList[i].Number; position++)
          DrawPad (PADLIST_ENTRY (i, position));
        PadList[i].DrawLocation = PadList[i].Number;
      }

  /* draw all new PVs; 'PVList' holds a list of pointers to the
   * sorted array pointers to PV data
   */
  while (PVList.DrawLocation < PVList.Number)
    {
      PinType *pv = PVLIST_ENTRY (PVList.DrawLocation);

      if (TEST_FLAG (PINFLAG, pv))
        {
          if (PCB->PinOn)
            DrawPin (pv);
        }
      else if (PCB->ViaOn)
        DrawVia (pv);
      PVList.DrawLocation++;
    }
  /* draw the new rat-lines */
  if (PCB->RatOn)
    {
      position = RatList.DrawLocation;
      for (; position < RatList.Number; position++)
        DrawRat (RATLIST_ENTRY (position));
      RatList.DrawLocation = RatList.Number;
    }
}

/*!
 * \brief Find all connections to pins within one element.
 */
void
LookupElementConnections (ElementType *Element, FILE * FP)
{
  /* reset all currently marked connections */
  User = true;
  ClearFlagOnAllObjects (true, FOUNDFLAG);
  InitConnectionLookup ();
  PrintElementConnections (Element, FP, FOUNDFLAG, true);
  SetChangedFlag (true);
  if (Settings.RingBellWhenFinished)
    gui->beep ();
  FreeConnectionLookupMemory ();
  IncrementUndoSerialNumber ();
  User = false;
  Draw ();
}

/*!
 * \brief Find all connections to pins of all element.
 */
void
LookupConnectionsToAllElements (FILE * FP)
{
  /* reset all currently marked connections */
  User = false;
  ClearFlagOnAllObjects (false, FOUNDFLAG);
  InitConnectionLookup ();

  ELEMENT_LOOP (PCB->Data);
  {
    /* break if abort dialog returned true */
    if (PrintElementConnections (element, FP, FOUNDFLAG, false))
      break;
    SEPARATE (FP);
    if (Settings.ResetAfterElement && n != 1)
      ClearFlagOnAllObjects (false, FOUNDFLAG);
  }
  END_LOOP;
  if (Settings.RingBellWhenFinished)
    gui->beep ();
  ClearFlagOnAllObjects (false, FOUNDFLAG);
  FreeConnectionLookupMemory ();
  Redraw ();
}

/*!
 * \brief Add the starting object to the list of found objects.
 */
bool
ListStart (int type, void *ptr1, void *ptr2, void *ptr3, int flag)
{
  DumpList ();
  switch (type)
    {
    case PIN_TYPE:
    case VIA_TYPE:
      {
        if (ADD_PV_TO_LIST ((PinType *) ptr2, flag))
          return true;
        break;
      }

    case RATLINE_TYPE:
      {
        if (ADD_RAT_TO_LIST ((RatType *) ptr1, flag))
          return true;
        break;
      }

    case LINE_TYPE:
      {
        int layer = GetLayerNumber (PCB->Data,
                                    (LayerType *) ptr1);

        if (ADD_LINE_TO_LIST (layer, (LineType *) ptr2, flag))
          return true;
        break;
      }

    case ARC_TYPE:
      {
        int layer = GetLayerNumber (PCB->Data,
                                    (LayerType *) ptr1);

        if (ADD_ARC_TO_LIST (layer, (ArcType *) ptr2, flag))
          return true;
        break;
      }

    case POLYGON_TYPE:
      {
        int layer = GetLayerNumber (PCB->Data,
                                    (LayerType *) ptr1);

        if (ADD_POLYGON_TO_LIST (layer, (PolygonType *) ptr2, flag))
          return true;
        break;
      }

    case PAD_TYPE:
      {
        PadType *pad = (PadType *) ptr2;
        if (ADD_PAD_TO_LIST
            (TEST_FLAG
             (ONSOLDERFLAG, pad) ? BOTTOM_SIDE : TOP_SIDE, pad, flag))
          return true;
        break;
      }
    }
  return (false);
}


/*!
 * \brief Looks up all connections from the object at the given
 * coordinates the TheFlag (normally 'FOUNDFLAG') is set for all objects
 * found.
 *
 * The objects are re-drawn if AndDraw is true, also the action is
 * marked as undoable if AndDraw is true.
 */
void
LookupConnection (Coord X, Coord Y, bool AndDraw, Coord Range, int flag,
                  bool AndRats)
{
  void *ptr1, *ptr2, *ptr3;
  char *name;
  int type;

  /* check if there are any pins or pads at that position */

	reassign_no_drc_flags ();

  type
    = SearchObjectByLocation (LOOKUP_FIRST, &ptr1, &ptr2, &ptr3, X, Y, Range);
  if (type == NO_TYPE)
    {
      type = SearchObjectByLocation (
        LOOKUP_MORE & ~(AndRats ? 0 : RATLINE_TYPE),
        &ptr1, &ptr2, &ptr3, X, Y, Range);
      if (type == NO_TYPE)
        return;
      if (type & SILK_TYPE)
        {
          int laynum = GetLayerNumber (PCB->Data,
                                       (LayerType *) ptr1);

          /* don't mess with non-conducting objects! */
          if (laynum >= max_copper_layer || ((LayerType *)ptr1)->no_drc)
            return;
        }
    }

  name = ConnectionName (type, ptr1, ptr2);
  hid_actionl ("NetlistShow", name, NULL);

  User = AndDraw;
  InitConnectionLookup ();

  /* now add the object to the appropriate list and start scanning
   * This is step (1) from the description
   */
  ListStart (type, ptr1, ptr2, ptr3, flag);
  DoIt (flag, AndRats, AndDraw);
  if (AndDraw)
    IncrementUndoSerialNumber ();
  User = false;

  /* we are done */
  if (AndDraw)
    Draw ();
  if (AndDraw && Settings.RingBellWhenFinished)
    gui->beep ();
  FreeConnectionLookupMemory ();
}

void 
LookupConnectionByPin (int type, void *ptr1)
{
/*  int TheFlag = FOUNDFLAG; */

  User = 0;
  InitConnectionLookup ();
  ListStart (type, NULL, ptr1, NULL, FOUNDFLAG);

  DoIt (FOUNDFLAG, true, false);

  FreeConnectionLookupMemory ();
}

/*!
 * \brief Find connections for rats nesting.
 *
 * Assumes InitConnectionLookup() has already been done.
 */
void
RatFindHook (int type, void *ptr1, void *ptr2, void *ptr3,
             bool undo, int flag, bool AndRats)
{
  User = undo;
  DumpList ();
  ListStart (type, ptr1, ptr2, ptr3, flag);
  DoIt (flag, AndRats, false);
  User = false;
}

/*!
 * \brief Find all unused pins of all elements.
 */
void
LookupUnusedPins (FILE * FP)
{
  /* reset all currently marked connections */
  User = true;
  ClearFlagOnAllObjects (true, FOUNDFLAG);
  InitConnectionLookup ();

  ELEMENT_LOOP (PCB->Data);
  {
    /* break if abort dialog returned true;
     * passing NULL as filedescriptor discards the normal output
     */
    if (PrintAndSelectUnusedPinsAndPadsOfElement (element, FP, FOUNDFLAG))
      break;
  }
  END_LOOP;

  if (Settings.RingBellWhenFinished)
    gui->beep ();
  FreeConnectionLookupMemory ();
  IncrementUndoSerialNumber ();
  User = false;
  Draw ();
}

/*!
 * \brief Resets all used flags of pins and vias.
 */
bool
ClearFlagOnPinsViasAndPads (bool AndDraw, int flag)
{
  bool change = false;

  VIA_LOOP (PCB->Data);
  {
    if (TEST_FLAG (flag, via))
      {
        if (AndDraw)
          AddObjectToFlagUndoList (VIA_TYPE, via, via, via);
        CLEAR_FLAG (flag, via);
        if (AndDraw)
          DrawVia (via);
        change = true;
      }
  }
  END_LOOP;
  ELEMENT_LOOP (PCB->Data);
  {
    PIN_LOOP (element);
    {
      if (TEST_FLAG (flag, pin))
        {
          if (AndDraw)
            AddObjectToFlagUndoList (PIN_TYPE, element, pin, pin);
          CLEAR_FLAG (flag, pin);
          if (AndDraw)
            DrawPin (pin);
          change = true;
        }
    }
    END_LOOP;
    PAD_LOOP (element);
    {
      if (TEST_FLAG (flag, pad))
        {
          if (AndDraw)
            AddObjectToFlagUndoList (PAD_TYPE, element, pad, pad);
          CLEAR_FLAG (flag, pad);
          if (AndDraw)
            DrawPad (pad);
          change = true;
        }
    }
    END_LOOP;
  }
  END_LOOP;
  if (change)
    SetChangedFlag (true);
  return change;
}

/*!
 * \brief Resets all used flags of LOs.
 */
bool
ClearFlagOnLinesAndPolygons (bool AndDraw, int flag)
{
  bool change = false;

  RAT_LOOP (PCB->Data);
  {
    if (TEST_FLAG (flag, line))
      {
        if (AndDraw)
          AddObjectToFlagUndoList (RATLINE_TYPE, line, line, line);
        CLEAR_FLAG (flag, line);
        if (AndDraw)
          DrawRat (line);
        change = true;
      }
  }
  END_LOOP;
  COPPERLINE_LOOP (PCB->Data);
  {
    if (TEST_FLAG (flag, line))
      {
        if (AndDraw)
          AddObjectToFlagUndoList (LINE_TYPE, layer, line, line);
        CLEAR_FLAG (flag, line);
        if (AndDraw)
          DrawLine (layer, line);
        change = true;
      }
  }
  ENDALL_LOOP;
  COPPERARC_LOOP (PCB->Data);
  {
    if (TEST_FLAG (flag, arc))
      {
        if (AndDraw)
          AddObjectToFlagUndoList (ARC_TYPE, layer, arc, arc);
        CLEAR_FLAG (flag, arc);
        if (AndDraw)
          DrawArc (layer, arc);
        change = true;
      }
  }
  ENDALL_LOOP;
  COPPERPOLYGON_LOOP (PCB->Data);
  {
    if (TEST_FLAG (flag, polygon))
      {
        if (AndDraw)
          AddObjectToFlagUndoList (POLYGON_TYPE, layer, polygon, polygon);
        CLEAR_FLAG (flag, polygon);
        if (AndDraw)
          DrawPolygon (layer, polygon);
        change = true;
      }
  }
  ENDALL_LOOP;
  if (change)
    SetChangedFlag (true);
  return change;
}

/*!
 * \brief Resets all found connections.
 */
bool
ClearFlagOnAllObjects (bool AndDraw, int flag)
{
  bool change = false;

  change = ClearFlagOnPinsViasAndPads  (AndDraw, flag) || change;
  change = ClearFlagOnLinesAndPolygons (AndDraw, flag) || change;

  return change;
}

/*!
 * \brief Dumps the list contents.
 */
void
DumpList (void)
{
  Cardinal i;

  for (i = 0; i < 2; i++)
    {
      PadList[i].Number = 0;
      PadList[i].Location = 0;
      PadList[i].DrawLocation = 0;
    }

  PVList.Number = 0;
  PVList.Location = 0;

  for (i = 0; i < max_copper_layer; i++)
    {
      LineList[i].Location = 0;
      LineList[i].DrawLocation = 0;
      LineList[i].Number = 0;
      ArcList[i].Location = 0;
      ArcList[i].DrawLocation = 0;
      ArcList[i].Number = 0;
      PolygonList[i].Location = 0;
      PolygonList[i].DrawLocation = 0;
      PolygonList[i].Number = 0;
    }
  RatList.Number = 0;
  RatList.Location = 0;
  RatList.DrawLocation = 0;
}

void
start_do_it_and_dump (int type, void *ptr1, void *ptr2, void *ptr3,
                      int flag, bool AndDraw,
                      Coord bloat, bool is_drc)
{
  Bloat = bloat;
  drc = is_drc;
  ListStart (type, ptr1, ptr2, ptr3, flag);
  DoIt (flag, true, AndDraw);
  DumpList ();
}

void
InitConnectionLookup (void)
{
  InitComponentLookup ();
  InitLayoutLookup ();
}

void
FreeConnectionLookupMemory (void)
{
  FreeComponentLookupMemory ();
  FreeLayoutLookupMemory ();
}
