/*!
 * \file src/actions/ClearSquareAction.cpp
 *
 * \brief ClearSquare action - clears square flag of pins/pads.
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
#include "change.h"      // For ClrObjectSquare(), ClrSelectedSquare()
#include "set.h"         // For SetChangedFlag()
#include "hid.h"         // For gui
}

namespace pcb {
namespace actions {

/*!
 * \brief ClearSquareAction - Clear square flag
 *
 * Clears the square flag of pins and pads.
 *
 * Syntax: ClearSquare(ToggleObject|SelectedElements|SelectedPins)
 */
class ClearSquareAction : public Action {
public:
    ClearSquareAction()
        : Action("ClearSquare",
                 "Clears the square-flag of pins and pads.",
                 "ClearSquare(ToggleObject|SelectedElements|SelectedPins)")
    {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        if (argc < 1 || !argv[0] || !argv[0][0]) {
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
                type = SearchScreen(x, y, CHANGESQUARE_TYPES, &ptr1, &ptr2, &ptr3);
                if (type != NO_TYPE && ClrObjectSquare(type, ptr1, ptr2, ptr3)) {
                    SetChangedFlag(true);
                }
                break;
            }

            case F_SelectedElements:
                if (ClrSelectedSquare(ELEMENT_TYPE)) {
                    SetChangedFlag(true);
                }
                break;

            case F_SelectedPins:
                if (ClrSelectedSquare(PIN_TYPE | PAD_TYPE)) {
                    SetChangedFlag(true);
                }
                break;

            case F_Selected:
            case F_SelectedObjects:
                if (ClrSelectedSquare(PIN_TYPE | PAD_TYPE)) {
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
REGISTER_ACTION(ClearSquareAction);

}} // namespace pcb::actions
