/*!
 * \file src/actions/ChangeHoleAction.cpp
 *
 * \brief ChangeHole action - changes drill hole diameter of vias.
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
#include "change.h"      // For ChangeHole(), ChangeSelectedHole()
#include "set.h"         // For SetChangedFlag()
#include "undo.h"        // For IncrementUndoSerialNumber()
#include "hid.h"         // For gui
}

namespace pcb {
namespace actions {

/*!
 * \brief ChangeHoleAction - Change drill hole diameter
 *
 * Changes the drill hole diameter of vias, either for a selected object
 * or for all selected vias.
 *
 * Syntax: ChangeHole(ToggleObject|Object|SelectedVias|Selected)
 */
class ChangeHoleAction : public Action {
public:
    ChangeHoleAction()
        : Action("ChangeHole",
                 "Changes the drill hole diameter of objects.",
                 "ChangeHole(ToggleObject|Object|SelectedVias|Selected)")
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
                type = SearchScreen(x, y, VIA_TYPE, &ptr1, &ptr2, &ptr3);
                if (type != NO_TYPE && ChangeHole(static_cast<PinType*>(ptr3))) {
                    IncrementUndoSerialNumber();
                }
                break;
            }

            case F_SelectedVias:
            case F_Selected:
                if (ChangeSelectedHole()) {
                    SetChangedFlag(true);
                }
                break;

            default:
                // Unknown function - just ignore
                break;
        }

        return 0;
    }
};

// Auto-register this action
REGISTER_ACTION(ChangeHoleAction);

}} // namespace pcb::actions
