/*!
 * \file src/actions/EditorActions.cpp
 *
 * \brief Editor utility actions (Atomic, MarkCrosshair).
 *
 * This module implements editor-level utility actions:
 * - Atomic: Manage undo serial numbers for atomic operations
 * - MarkCrosshair: Set/reset the crosshair mark
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
 */

#include "EditorActions.h"

#include <cstring>

extern "C" {
#include "global.h"
#include "crosshair.h"
#include "undo.h"
#include "macro.h"
}

namespace pcb {
namespace actions {

//=============================================================================
// AtomicAction
//=============================================================================

static const char atomic_syntax[] = "Atomic(Save|Restore|Close|Block)";

static const char atomic_help[] = "Save or restore the undo serial number.";

AtomicAction::AtomicAction()
    : Action("Atomic", atomic_help, atomic_syntax)
{}

int AtomicAction::execute(int argc, char** argv, Coord /*x*/, Coord /*y*/)
{
    if (argc != 1) {
        Message("ERROR: Atomic() requires exactly one argument\n");
        return 1;
    }

    switch (GetFunctionID(argv[0])) {
        case F_Save:
            SaveUndoSerialNumber();
            break;
        case F_Restore:
            RestoreUndoSerialNumber();
            break;
        case F_Close:
            RestoreUndoSerialNumber();
            IncrementUndoSerialNumber();
            break;
        case F_Block:
            RestoreUndoSerialNumber();
            if (Bumped) {
                IncrementUndoSerialNumber();
            }
            break;
        default:
            Message("ERROR: Unknown Atomic operation: %s\n", argv[0]);
            return 1;
    }

    return 0;
}

//=============================================================================
// MarkCrosshairAction
//=============================================================================

static const char markcrosshair_syntax[] =
    "MarkCrosshair()\n"
    "MarkCrosshair(Center)";

static const char markcrosshair_help[] = "Set/Reset the Crosshair mark.";

MarkCrosshairAction::MarkCrosshairAction()
    : Action("MarkCrosshair", markcrosshair_help, markcrosshair_syntax)
{}

int MarkCrosshairAction::execute(int /*argc*/, char** argv, Coord /*x*/, Coord /*y*/)
{
    const char* function = arg(0, argv);

    if (!function || !*function) {
        // Toggle mark
        if (Marked.status) {
            notify_mark_change(false);
            Marked.status = false;
            notify_mark_change(true);
        } else {
            notify_mark_change(false);
            Marked.status = false;
            Marked.status = true;
            Marked.X = Crosshair.X;
            Marked.Y = Crosshair.Y;
            notify_mark_change(true);
        }
    } else if (GetFunctionID(function) == F_Center) {
        // Center mark at current crosshair position
        notify_mark_change(false);
        Marked.status = true;
        Marked.X = Crosshair.X;
        Marked.Y = Crosshair.Y;
        notify_mark_change(true);
    }

    return 0;
}

//=============================================================================
// Registration
//=============================================================================

REGISTER_ACTION(AtomicAction);
REGISTER_ACTION(MarkCrosshairAction);

} // namespace actions
} // namespace pcb
