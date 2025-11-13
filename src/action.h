/*!
 * \file src/action.h
 *
 * \brief Prototypes for action routines.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996 Thomas Nau
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
 */

#ifndef	PCB_ACTION_H
#define	PCB_ACTION_H

#include "global.h"

#define CLONE_TYPES LINE_TYPE | ARC_TYPE | VIA_TYPE | POLYGON_TYPE

/* Function IDs for action argument parsing */
typedef enum
{
  F_AddSelected,
  F_All,
  F_AllConnections,
  F_AllRats,
  F_AllUnusedPins,
  F_Arc,
  F_Arrow,
  F_Block,
  F_Description,
  F_Cancel,
  F_Center,
  F_Clear,
  F_ClearAndRedraw,
  F_ClearList,
  F_Close,
  F_Found,
  F_Connection,
  F_Convert,
  F_Copy,
  F_CycleClip,
  F_CycleCrosshair,
  F_DeleteRats,
  F_Drag,
  F_DrillReport,
  F_Element,
  F_ElementByName,
  F_ElementConnections,
  F_ElementToBuffer,
  F_Escape,
  F_Find,
  F_FlipElement,
  F_FoundPins,
  F_Grid,
  F_InsertPoint,
  F_Layer,
  F_Layout,
  F_LayoutAs,
  F_LayoutToBuffer,
  F_Line,
  F_LineSize,
  F_Lock,
  F_Mirror,
  F_Move,
  F_NameOnPCB,
  F_Netlist,
  F_NetByName,
  F_None,
  F_Notify,
  F_Object,
  F_ObjectByName,
  F_PasteBuffer,
  F_PadByName,
  F_PinByName,
  F_PinOrPadName,
  F_Pinout,
  F_Polygon,
  F_PolygonHole,
  F_PreviousPoint,
  F_RatsNest,
  F_Rectangle,
  F_Redraw,
  F_Release,
  F_Revert,
  F_Remove,
  F_RemoveSelected,
  F_Report,
  F_Reset,
  F_ResetLinesAndPolygons,
  F_ResetPinsViasAndPads,
  F_Restore,
  F_Rotate,
  F_Save,
  F_Selected,
  F_SelectedArcs,
  F_SelectedElements,
  F_SelectedLines,
  F_SelectedNames,
  F_SelectedObjects,
  F_SelectedPads,
  F_SelectedPins,
  F_SelectedTexts,
  F_SelectedVias,
  F_SelectedRats,
  F_Stroke,
  F_Text,
  F_TextByName,
  F_TextScale,
  F_Thermal,
  F_ToLayout,
  F_ToggleAllDirections,
  F_ToggleAutoDRC,
  F_ToggleClearLine,
  F_ToggleFullPoly,
  F_ToggleGrid,
  F_ToggleHideNames,
  F_ToggleMask,
  F_ToggleName,
  F_ToggleObject,
  F_ToggleShowDRC,
  F_ToggleLiveRoute,
  F_ToggleRubberBandMode,
  F_ToggleStartDirection,
  F_ToggleThindraw,
  F_ToggleThindrawPoly,
  F_ToggleOrthoMove,
  F_ToggleLocalRef,
  F_ToggleCheckPlanes,
  F_ToggleUniqueNames,
  F_ToggleSnapPin,
  F_ToggleSnapOffGridLine,
  F_Value,
  F_Via,
  F_ViaByName,
  F_ViaSize,
  F_Zoom,
  F_BuriedVias
}
FunctionID;

/* Get function ID from string name */
int GetFunctionID(const char *);

void ActionAdjustStyle (char *);
void EventMoveCrosshair (int, int);

void AdjustAttachedObjects (void);

void warpNoWhere (void);

/* In gui-misc.c */
bool ActionGetLocation (char *);
void ActionGetXY (char *);

#endif
