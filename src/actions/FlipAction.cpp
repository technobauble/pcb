/*!
 * \file src/actions/FlipAction.cpp
 *
 * \brief Flip action - flips elements to opposite side of board.
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
#include "data.h"        // For PCB, Crosshair
#include "action.h"      // For GetFunctionID()
#include "search.h"      // For SearchScreen()
#include "change.h"      // For ChangeElementSide(), ChangeSelectedElementSide()
#include "draw.h"        // For Draw()
#include "undo.h"        // For IncrementUndoSerialNumber()
#include "error.h"       // For Message()
}

namespace pcb {
namespace actions {

/*!
 * \brief FlipAction - Flip elements to opposite side
 *
 * Flips elements (components) to the opposite side of the board.
 * The flip is done around the current crosshair Y position.
 *
 * Syntax: Flip(Object|Selected|SelectedElements)
 */
class FlipAction : public Action {
public:
    FlipAction()
        : Action("Flip",
                 "Flips an element to the opposite side of the board.",
                 "Flip(Object|Selected|SelectedElements)")
    {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        if (argc < 1) {
            Message("Syntax error. Usage:\n%s\n", syntax());
            return 1;
        }

        const char* function = argv[0];
        ElementType* element;
        void* ptrtmp;
        int err = 0;

        switch (GetFunctionID(const_cast<char*>(function))) {
            case F_Object:
                if (SearchScreen(x, y, ELEMENT_TYPE, &ptrtmp, &ptrtmp, &ptrtmp) != NO_TYPE) {
                    element = static_cast<ElementType*>(ptrtmp);
                    ChangeElementSide(element, 2 * Crosshair.Y - PCB->MaxHeight);
                    IncrementUndoSerialNumber();
                    Draw();
                }
                break;

            case F_Selected:
            case F_SelectedElements:
                ChangeSelectedElementSide();
                break;

            default:
                err = 1;
                break;
        }

        if (err) {
            Message("Syntax error. Usage:\n%s\n", syntax());
            return 1;
        }

        return 0;
    }
};

// Auto-register this action
REGISTER_ACTION(FlipAction);

}} // namespace pcb::actions
