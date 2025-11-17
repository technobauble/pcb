/*!
 * \file src/actions/SetOctagonAction.cpp
 *
 * \brief SetOctagon action - sets octagon flag of pins/vias.
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
#include "change.h"      // For SetObjectOctagon(), SetSelectedOctagon()
#include "set.h"         // For SetChangedFlag()
#include "hid.h"         // For gui
}

namespace pcb {
namespace actions {

/*!
 * \brief SetOctagonAction - Set octagon flag
 *
 * Sets the octagon flag of pins and vias.
 *
 * Syntax: SetOctagon(Object|ToggleObject|SelectedElements|Selected)
 */
class SetOctagonAction : public Action {
public:
    SetOctagonAction()
        : Action("SetOctagon",
                 "Sets the octagon-flag of objects.",
                 "SetOctagon(Object|ToggleObject|SelectedElements|Selected)")
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
                if (type != NO_TYPE && SetObjectOctagon(type, ptr1, ptr2, ptr3)) {
                    SetChangedFlag(true);
                }
                break;
            }

            case F_SelectedElements:
                if (SetSelectedOctagon(ELEMENT_TYPE)) {
                    SetChangedFlag(true);
                }
                break;

            case F_SelectedPins:
                if (SetSelectedOctagon(PIN_TYPE)) {
                    SetChangedFlag(true);
                }
                break;

            case F_SelectedVias:
                if (SetSelectedOctagon(VIA_TYPE)) {
                    SetChangedFlag(true);
                }
                break;

            case F_Selected:
            case F_SelectedObjects:
                if (SetSelectedOctagon(PIN_TYPES)) {
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
REGISTER_ACTION(SetOctagonAction);

}} // namespace pcb::actions
