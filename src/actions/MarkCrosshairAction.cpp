/*!
 * \file src/actions/MarkCrosshairAction.cpp
 *
 * \brief MarkCrosshair action - sets or resets the crosshair mark.
 *
 * The "mark" is a small X-shaped target on the display which is
 * treated like a second origin for measurements.
 *
 * <hr>
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

#include "Action.h"

// Only include stable C interfaces
extern "C" {
#include "global.h"
#include "error.h"       // For Message()
#include "data.h"        // For Marked
#include "crosshair.h"   // For Crosshair, notify_mark_change()
}

#include <cstring>       // For strcmp

namespace pcb {
namespace actions {

/*!
 * \brief MarkCrosshairAction - Set/Reset the Crosshair mark
 *
 * The "mark" is a small X-shaped target on the display which is
 * treated like a second origin (the normal origin is the upper left
 * corner of the board). The GUI will display a second set of
 * coordinates for this mark, which tells you how far you are from it.
 *
 * If no argument is given, the mark is toggled - disabled if it was
 * enabled, or enabled at the current cursor position if disabled. If
 * the Center argument is given, the mark is moved to the current
 * cursor location.
 *
 * Syntax: MarkCrosshair() or MarkCrosshair(Center)
 */
class MarkCrosshairAction : public Action {
public:
    MarkCrosshairAction()
        : Action("MarkCrosshair",
                 "Set/Reset the Crosshair mark.",
                 "MarkCrosshair()\nMarkCrosshair(Center)")
    {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        // No argument: toggle mark at current position
        if (argc == 0 || !argv[0] || argv[0][0] == '\0') {
            if (Marked.status) {
                // Mark is currently on, turn it off
                notify_mark_change(false);
                Marked.status = false;
                notify_mark_change(true);
            }
            else {
                // Mark is currently off, turn it on at cursor position
                notify_mark_change(false);
                Marked.status = true;
                Marked.X = Crosshair.X;
                Marked.Y = Crosshair.Y;
                notify_mark_change(true);
            }
        }
        // Argument "Center": enable mark at cursor position
        else if (strcmp(argv[0], "Center") == 0) {
            notify_mark_change(false);
            Marked.status = true;
            Marked.X = Crosshair.X;
            Marked.Y = Crosshair.Y;
            notify_mark_change(true);
        }
        else {
            Message("MarkCrosshair: Unknown argument '%s'. Use no argument to toggle, or 'Center' to set.\n", argv[0]);
            return 1;
        }

        return 0;
    }
};

// Auto-register this action
REGISTER_ACTION(MarkCrosshairAction);

}} // namespace pcb::actions
