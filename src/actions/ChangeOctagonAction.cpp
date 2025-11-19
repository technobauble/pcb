/*!
 * \file src/actions/ChangeOctagonAction.cpp
 *
 * \brief ChangeOctagon action - toggles octagon flag of pins/vias.
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
#include "change.h"      // For ChangeObjectOctagon(), ChangeSelectedOctagon()
#include "set.h"         // For SetChangedFlag()
#include "hid.h"         // For gui
}

namespace pcb {
namespace actions {

/*!
 * \brief ChangeOctagonAction - Toggle octagon flag
 *
 * Changes the octagon flag of pins and vias.
 *
 * Syntax: ChangeOctagon(ToggleObject|Object|SelectedElements|SelectedPins|SelectedVias|Selected|SelectedObjects)
 */
class ChangeOctagonAction : public Action {
public:
    ChangeOctagonAction()
        : Action("ChangeOctagon",
                 "Changes the octagon-flag of objects.",
                 "ChangeOctagon(ToggleObject|SelectedElements|SelectedPins|SelectedVias|Selected)")
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
                type = SearchScreen(x, y, CHANGEOCTAGON_TYPES, &ptr1, &ptr2, &ptr3);
                if (type != NO_TYPE && ChangeObjectOctagon(type, ptr1, ptr2, ptr3)) {
                    SetChangedFlag(true);
                }
                break;
            }

            case F_SelectedElements:
                if (ChangeSelectedOctagon(ELEMENT_TYPE)) {
                    SetChangedFlag(true);
                }
                break;

            case F_SelectedPins:
                if (ChangeSelectedOctagon(PIN_TYPE)) {
                    SetChangedFlag(true);
                }
                break;

            case F_SelectedVias:
                if (ChangeSelectedOctagon(VIA_TYPE)) {
                    SetChangedFlag(true);
                }
                break;

            case F_Selected:
            case F_SelectedObjects:
                if (ChangeSelectedOctagon(PIN_TYPES)) {
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
REGISTER_ACTION(ChangeOctagonAction);

}} // namespace pcb::actions
