/*!
 * \file src/actions/MoveToCurrentLayerAction.cpp
 *
 * \brief MoveToCurrentLayer action - moves objects to current layer.
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
#include "data.h"         // For Settings, PCB, LayerStack, and layer globals
#include "action.h"       // For GetFunctionID()
#include "search.h"       // For SearchScreen()
#include "move.h"         // For MoveObjectToLayer(), MoveSelectedObjectsToLayer()
#include "set.h"          // For SetChangedFlag()
#include "hid.h"          // For gui
}

namespace pcb {
namespace actions {

/*!
 * \brief MoveToCurrentLayerAction - Move objects to current layer
 *
 * Moves the specified object or selected objects to the current layer.
 * This validates that GetFunctionID export works correctly in C++.
 *
 * Syntax: MoveToCurrentLayer(Object|SelectedObjects|Selected)
 */
class MoveToCurrentLayerAction : public Action {
public:
    MoveToCurrentLayerAction()
        : Action("MoveToCurrentLayer",
                 "Moves objects to current layer.",
                 "MoveToCurrentLayer(Object|SelectedObjects|Selected)")
    {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        if (argc < 1) {
            return 0;
        }

        const char* function = argv[0];

        switch (GetFunctionID(const_cast<char*>(function))) {
            case F_Object: {
                int type;
                void* ptr1;
                void* ptr2;
                void* ptr3;

                gui->get_coords(const_cast<char*>(_("Select an Object")), &x, &y);
                type = SearchScreen(x, y, MOVETOLAYER_TYPES, &ptr1, &ptr2, &ptr3);
                if (type != NO_TYPE) {
                    if (MoveObjectToLayer(type, ptr1, ptr2, ptr3, CURRENT, false)) {
                        SetChangedFlag(true);
                    }
                }
                break;
            }

            case F_SelectedObjects:
            case F_Selected:
                if (MoveSelectedObjectsToLayer(CURRENT)) {
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
REGISTER_ACTION(MoveToCurrentLayerAction);

}} // namespace pcb::actions
