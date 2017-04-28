/*!
 * \file src/snap.h
 *
 * \brief Snapping functions.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2017 Charles Parker
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

#ifndef snap_h
#define snap_h

#include <stdio.h>
#include <stdbool.h>
#include "global.h"  /* type definitions */

/*
 * Snap Type and functions
 */

typedef struct SnapStruct
{
  char * name;
  bool (*search)(struct SnapStruct * snap, Coord x, Coord y);
  bool enabled;
  int priority;
  Coord radius;
  unsigned obj_type;
  PointType location;
  Coord distsq;
} SnapType;

/* how many parameters ought to be specified in this constructor? 
   For insertion into a snap list it needs at least a name and a priority,
   so I'll go with that for now. */
/* constructor */
SnapType * snap_new(char * name, int priority);
/* destructor */
void snap_delete(SnapType * snap);

/*
 * Snap List Type and functions
 */

typedef struct {
  int n, max;
  SnapType * snaps;
} SnapListType;

/* constructor */
SnapListType * snap_list_new();
/* destructor */
void snap_list_delete(SnapListType * list);

SnapType * snap_list_add_snap(SnapListType * list, SnapType * snap);
int snap_list_remove_snap_by_name(SnapListType * list, char * name);
SnapType * snap_list_find_snap_by_name(SnapListType * list, char * name);
int snap_list_list_snaps(SnapListType * list);
SnapType * snap_list_search_snaps(SnapListType * list, Coord x, Coord y);



#endif /* snap_h */
