/*!
 * \file src/actions/SelectionActions.cpp
 *
 * \brief Selection-related actions (Select, Unselect, RemoveSelected).
 *
 * This module implements the three core selection actions:
 * - Select: Select objects on the PCB
 * - Unselect: Unselect objects on the PCB
 * - RemoveSelected: Remove all selected objects
 *
 * These are the first actions extracted from the monolithic action.c file
 * as part of the C++ refactoring effort.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2025 PCB Contributors
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
 */

#include "SelectionActions.h"

#include <cstring>
#include <algorithm>

extern "C" {
#include "global.h"
#include "data.h"
#include "select.h"
#include "remove.h"
#include "search.h"
#include "set.h"
#include "undo.h"
#include "draw.h"
#include "crosshair.h"
#include "misc.h"
#include "error.h"
#include "flags.h"
}

namespace pcb {
namespace actions {

//=============================================================================
// SelectAction
//=============================================================================

static const char select_syntax[] =
    "Select(Object|ToggleObject)\n"
    "Select(Block)\n"
    "Select(All)\n"
    "Select(Found|Connection|BuriedVias)\n"
    "Select(ElementByName|ObjectByName|PadByName|PinByName|TextByName|ViaByName|NetByName, pattern)";

static const char select_help[] =
    "Toggles or sets the selection flag on objects.\n"
    "\n"
    "Object/ToggleObject - selects the object under the crosshair\n"
    "Block - selects all objects in the selection rectangle\n"
    "All - selects all visible objects\n"
    "Found - selects objects marked as 'found'\n"
    "Connection - selects physically connected objects\n"
    "BuriedVias - selects buried vias\n"
    "*ByName - selects objects matching a pattern";

SelectAction::SelectAction()
    : Action("Select", select_help, select_syntax)
{}

int SelectAction::execute(int /*argc*/, char** argv, Coord /*x*/, Coord /*y*/)
{
    const char* function = arg(0, argv);

    if (!function || !*function) {
        // No argument - try to select object at cursor
        if (SelectObject()) {
            SetChangedFlag(true);
        }
        return 0;
    }

    // Handle different selection modes
    if (strcmp(function, "Object") == 0 || strcmp(function, "ToggleObject") == 0) {
        if (SelectObject()) {
            SetChangedFlag(true);
        }
    }
    else if (strcmp(function, "Block") == 0) {
        BoxType box;
        box.X1 = std::min(Crosshair.AttachedBox.Point1.X, Crosshair.AttachedBox.Point2.X);
        box.Y1 = std::min(Crosshair.AttachedBox.Point1.Y, Crosshair.AttachedBox.Point2.Y);
        box.X2 = std::max(Crosshair.AttachedBox.Point1.X, Crosshair.AttachedBox.Point2.X);
        box.Y2 = std::max(Crosshair.AttachedBox.Point1.Y, Crosshair.AttachedBox.Point2.Y);

        notify_crosshair_change(false);

        // TODO: NotifyBlock() is a static function in action.c that manages
        // AttachedBox state transitions (STATE_FIRST -> STATE_SECOND -> STATE_THIRD).
        // This UI state management needs to be refactored and exposed properly.
        // For now, we handle the STATE_THIRD check directly below.

        if (Crosshair.AttachedBox.State == STATE_THIRD && SelectBlock(&box, true)) {
            SetChangedFlag(true);
            Crosshair.AttachedBox.State = STATE_FIRST;
        }

        notify_crosshair_change(true);
    }
    else if (strcmp(function, "All") == 0) {
        BoxType box;
        box.X1 = -MAX_COORD;
        box.Y1 = -MAX_COORD;
        box.X2 = MAX_COORD;
        box.Y2 = MAX_COORD;

        if (SelectBlock(&box, true)) {
            SetChangedFlag(true);
        }
    }
    else if (strcmp(function, "Found") == 0) {
        if (SelectByFlag(FOUNDFLAG, true)) {
            Draw();
            IncrementUndoSerialNumber();
            SetChangedFlag(true);
        }
    }
    else if (strcmp(function, "Connection") == 0) {
        if (SelectByFlag(CONNECTEDFLAG, true)) {
            Draw();
            IncrementUndoSerialNumber();
            SetChangedFlag(true);
        }
    }
    else if (strcmp(function, "BuriedVias") == 0) {
        if (SelectBuriedVias(true)) {
            Draw();
            IncrementUndoSerialNumber();
            SetChangedFlag(true);
        }
    }
#if defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP)
    // Handle selection by name pattern
    else if (strcmp(function, "ElementByName") == 0) {
        selectByName(ELEMENT_TYPE, arg(1, argv));
    }
    else if (strcmp(function, "ObjectByName") == 0) {
        selectByName(ALL_TYPES, arg(1, argv));
    }
    else if (strcmp(function, "PadByName") == 0) {
        selectByName(PAD_TYPE, arg(1, argv));
    }
    else if (strcmp(function, "PinByName") == 0) {
        selectByName(PIN_TYPE, arg(1, argv));
    }
    else if (strcmp(function, "TextByName") == 0) {
        selectByName(TEXT_TYPE, arg(1, argv));
    }
    else if (strcmp(function, "ViaByName") == 0) {
        selectByName(VIA_TYPE, arg(1, argv));
    }
    else if (strcmp(function, "NetByName") == 0) {
        selectByName(NET_TYPE, arg(1, argv));
    }
#endif
    else {
        Message("Select: Unknown mode '%s'\n", function);
        return 1;
    }

    return 0;
}

void SelectAction::selectByName(int type, const char* pattern)
{
#if defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP)
    if (pattern && *pattern) {
        if (SelectObjectByName(type, const_cast<char*>(pattern), true)) {
            SetChangedFlag(true);
        }
    } else {
        char* prompted = gui->prompt_for(_("Enter pattern:"), "");
        if (prompted) {
            if (SelectObjectByName(type, prompted, true)) {
                SetChangedFlag(true);
            }
            free(prompted);
        }
    }
#endif
}

//=============================================================================
// UnselectAction
//=============================================================================

static const char unselect_syntax[] =
    "Unselect(All)\n"
    "Unselect(Block)\n"
    "Unselect(Found|Connection)\n"
    "Unselect(ElementByName|ObjectByName|PadByName|PinByName|TextByName|ViaByName|NetByName, pattern)";

static const char unselect_help[] =
    "Clears the selection flag on objects.\n"
    "\n"
    "All - unselects all visible objects\n"
    "Block - unselects all objects in the selection rectangle\n"
    "Found - unselects objects marked as 'found'\n"
    "Connection - unselects physically connected objects\n"
    "*ByName - unselects objects matching a pattern";

UnselectAction::UnselectAction()
    : Action("Unselect", unselect_help, unselect_syntax)
{}

int UnselectAction::execute(int /*argc*/, char** argv, Coord /*x*/, Coord /*y*/)
{
    const char* function = arg(0, argv);

    if (!function || !*function) {
        // No argument - unselect all
        BoxType box;
        box.X1 = -MAX_COORD;
        box.Y1 = -MAX_COORD;
        box.X2 = MAX_COORD;
        box.Y2 = MAX_COORD;

        if (SelectBlock(&box, false)) {
            SetChangedFlag(true);
        }
        return 0;
    }

    // Handle different unselection modes
    if (strcmp(function, "Block") == 0) {
        BoxType box;
        box.X1 = std::min(Crosshair.AttachedBox.Point1.X, Crosshair.AttachedBox.Point2.X);
        box.Y1 = std::min(Crosshair.AttachedBox.Point1.Y, Crosshair.AttachedBox.Point2.Y);
        box.X2 = std::max(Crosshair.AttachedBox.Point1.X, Crosshair.AttachedBox.Point2.X);
        box.Y2 = std::max(Crosshair.AttachedBox.Point1.Y, Crosshair.AttachedBox.Point2.Y);

        notify_crosshair_change(false);

        // TODO: Same as SelectAction - NotifyBlock() needs to be refactored.
        // See comment in SelectAction::execute().

        if (Crosshair.AttachedBox.State == STATE_THIRD && SelectBlock(&box, false)) {
            SetChangedFlag(true);
            Crosshair.AttachedBox.State = STATE_FIRST;
        }

        notify_crosshair_change(true);
    }
    else if (strcmp(function, "All") == 0) {
        BoxType box;
        box.X1 = -MAX_COORD;
        box.Y1 = -MAX_COORD;
        box.X2 = MAX_COORD;
        box.Y2 = MAX_COORD;

        if (SelectBlock(&box, false)) {
            SetChangedFlag(true);
        }
    }
    else if (strcmp(function, "Found") == 0) {
        if (SelectByFlag(FOUNDFLAG, false)) {
            Draw();
            IncrementUndoSerialNumber();
            SetChangedFlag(true);
        }
    }
    else if (strcmp(function, "Connection") == 0) {
        if (SelectByFlag(CONNECTEDFLAG, false)) {
            Draw();
            IncrementUndoSerialNumber();
            SetChangedFlag(true);
        }
    }
#if defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP)
    // Handle unselection by name pattern
    else if (strcmp(function, "ElementByName") == 0) {
        unselectByName(ELEMENT_TYPE, arg(1, argv));
    }
    else if (strcmp(function, "ObjectByName") == 0) {
        unselectByName(ALL_TYPES, arg(1, argv));
    }
    else if (strcmp(function, "PadByName") == 0) {
        unselectByName(PAD_TYPE, arg(1, argv));
    }
    else if (strcmp(function, "PinByName") == 0) {
        unselectByName(PIN_TYPE, arg(1, argv));
    }
    else if (strcmp(function, "TextByName") == 0) {
        unselectByName(TEXT_TYPE, arg(1, argv));
    }
    else if (strcmp(function, "ViaByName") == 0) {
        unselectByName(VIA_TYPE, arg(1, argv));
    }
    else if (strcmp(function, "NetByName") == 0) {
        unselectByName(NET_TYPE, arg(1, argv));
    }
#endif
    else {
        Message("Unselect: Unknown mode '%s'\n", function);
        return 1;
    }

    return 0;
}

void UnselectAction::unselectByName(int type, const char* pattern)
{
#if defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP)
    if (pattern && *pattern) {
        if (SelectObjectByName(type, const_cast<char*>(pattern), false)) {
            SetChangedFlag(true);
        }
    } else {
        char* prompted = gui->prompt_for(_("Enter pattern:"), "");
        if (prompted) {
            if (SelectObjectByName(type, prompted, false)) {
                SetChangedFlag(true);
            }
            free(prompted);
        }
    }
#endif
}

//=============================================================================
// RemoveSelectedAction
//=============================================================================

static const char removeselected_syntax[] = "RemoveSelected()";

static const char removeselected_help[] =
    "Removes all selected objects from the board.";

RemoveSelectedAction::RemoveSelectedAction()
    : Action("RemoveSelected", removeselected_help, removeselected_syntax)
{}

int RemoveSelectedAction::execute(int /*argc*/, char** /*argv*/, Coord /*x*/, Coord /*y*/)
{
    if (RemoveSelected()) {
        SetChangedFlag(true);
    }
    return 0;
}

//=============================================================================
// Registration
//=============================================================================

REGISTER_ACTION(SelectAction);
REGISTER_ACTION(UnselectAction);
REGISTER_ACTION(RemoveSelectedAction);

} // namespace actions
} // namespace pcb
