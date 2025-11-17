/*!
 * \file src/actions/ChangePasteAction.cpp
 *
 * \brief ChangePaste action - changes paste flag of pads.
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
#include "action.h"      // For GetFunctionID()
#include "search.h"      // For SearchScreen()
#include "change.h"      // For ChangePaste(), ChangeSelectedPaste()
#include "set.h"         // For SetChangedFlag()
#include "undo.h"        // For IncrementUndoSerialNumber()
#include "hid.h"         // For gui
}

namespace pcb {
namespace actions {

/*!
 * \brief ChangePasteAction - Toggle paste flag
 *
 * Changes the "no paste" flag of pads. This controls whether solder paste
 * stencil openings are created for the pad.
 *
 * Syntax: ChangePaste(ToggleObject|Object|SelectedPads|Selected)
 */
class ChangePasteAction : public Action {
public:
    ChangePasteAction()
        : Action("ChangePaste",
                 "Changes the no paste flag of objects.",
                 "ChangePaste(ToggleObject|Object|SelectedPads|Selected)")
    {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        if (argc < 1) {
            return 0;
        }

        const char* function = argv[0];

        switch (GetFunctionID(const_cast<char*>(function))) {
            case F_ToggleObject:
            case F_Object: {
                int type;
                void* ptr1;
                void* ptr2;
                void* ptr3;

                gui->get_coords(const_cast<char*>(_("Select an Object")), &x, &y);
                type = SearchScreen(x, y, PAD_TYPE, &ptr1, &ptr2, &ptr3);
                if (type != NO_TYPE && ChangePaste(static_cast<PadType*>(ptr3))) {
                    IncrementUndoSerialNumber();
                }
                break;
            }

            case F_SelectedPads:
            case F_Selected:
                if (ChangeSelectedPaste()) {
                    SetChangedFlag(true);
                }
                break;

            default:
                break;
        }

        return 0;
    }
};

// Auto-register this action
REGISTER_ACTION(ChangePasteAction);

}} // namespace pcb::actions
