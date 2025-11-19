/*!
 * \file src/actions/AddRatsAction.cpp
 *
 * \brief AddRats action - adds rat lines (airwires) to board.
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
#include "data.h"        // For Settings, PCB
#include "action.h"      // For GetFunctionID(), ClearWarnings()
#include "rats.h"        // For AddAllRats()
#include "set.h"         // For SetChangedFlag()
#include "draw.h"        // For Draw(), DrawRat()
#include "undo.h"        // For AddObjectToFlagUndoList()
#include "misc.h"        // For CenterDisplay()
#include "macro.h"       // For RAT_LOOP macros
}

namespace pcb {
namespace actions {

/*!
 * \brief AddRatsAction - Add rat lines (airwires)
 *
 * Adds rat lines to connect unconnected pins/pads. Can add all rats,
 * only selected rats, or find and select the closest rat.
 *
 * Syntax: AddRats(AllRats|SelectedRats|Selected|Close)
 */
class AddRatsAction : public Action {
public:
    AddRatsAction()
        : Action("AddRats",
                 "Add one or more rat lines.",
                 "AddRats(AllRats|SelectedRats|Selected|Close)")
    {}

    int execute(int argc, char** argv, Coord /*x*/, Coord /*y*/) override {
        if (argc < 1) {
            return 0;
        }

        const char* function = argv[0];

        if (Settings.RatWarn) {
            ClearWarnings();
        }

        switch (GetFunctionID(const_cast<char*>(function))) {
            case F_AllRats:
                if (AddAllRats(false, nullptr)) {
                    SetChangedFlag(true);
                }
                break;

            case F_SelectedRats:
            case F_Selected:
                if (AddAllRats(true, nullptr)) {
                    SetChangedFlag(true);
                }
                break;

            case F_Close: {
                RatType* shorty = nullptr;
                float small = SQUARE(MAX_COORD);

                RAT_LOOP(PCB->Data);
                {
                    if (TEST_FLAG(SELECTEDFLAG, line))
                        continue;

                    float len = SQUARE(line->Point1.X - line->Point2.X) +
                               SQUARE(line->Point1.Y - line->Point2.Y);
                    if (len < small) {
                        small = len;
                        shorty = line;
                    }
                }
                END_LOOP;

                if (shorty) {
                    AddObjectToFlagUndoList(RATLINE_TYPE, shorty, shorty, shorty);
                    SET_FLAG(SELECTEDFLAG, shorty);
                    DrawRat(shorty);
                    Draw();
                    CenterDisplay((shorty->Point2.X + shorty->Point1.X) / 2,
                                (shorty->Point2.Y + shorty->Point1.Y) / 2,
                                false);
                }
                break;
            }

            default:
                // Unknown function - just ignore
                break;
        }

        return 0;
    }
};

// Auto-register this action
REGISTER_ACTION(AddRatsAction);

}} // namespace pcb::actions
