/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 *  RCS: $Id$
 */

/* prototypes for polygon editing routines
 */

#ifndef	__POLYGON_INCLUDED__
#define	__POLYGON_INCLUDED__

#include "global.h"

Cardinal GetLowestDistancePolygonPoint (PolygonTypePtr,
					LocationType, LocationType);
Boolean RemoveExcessPolygonPoints (LayerTypePtr, PolygonTypePtr);
void GoToPreviousPoint (void);
void ClosePolygon (void);
void CopyAttachedPolygonToLayer (void);
int PolygonHoles (int group, const BoxType * range,
		  int (*callback) (PLINE *, LayerTypePtr, PolygonTypePtr));
int PlowsPolygon (DataType *, ObjectArgType *,
		  int (*callback) (LayerTypePtr, PolygonTypePtr, ObjectArgType *));

POLYAREA * CirclePoly(LocationType x, LocationType y, BDimension radius);
POLYAREA * LinePoly(LineType *l, BDimension thick);
POLYAREA * PinPoly(PinType *l, BDimension thick);
int InitClip(LayerType *l, PolygonType *p);
void RestoreToPolygon(DataType *, ObjectArgType *);
void ClearFromPolygon(DataType *, ObjectArgType *);

Boolean IsPointInPolygon (LocationType, LocationType, BDimension, PolygonTypePtr);
Boolean IsRectangleInPolygon (LocationType, LocationType, LocationType,
			      LocationType, PolygonTypePtr);
Boolean isects (POLYAREA *, PolygonTypePtr);
#endif
