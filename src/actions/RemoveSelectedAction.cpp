/*!
 * \file src/actions/RemoveSelectedAction.cpp
 *
 * \brief RemoveSelected action - removes any selected objects.
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
#include "remove.h"  // For RemoveSelected()
#include "set.h"     // For SetChangedFlag()
}

namespace pcb {
namespace actions {

/*!
 * \brief RemoveSelectedAction - Remove any selected objects
 *
 * This action removes all currently selected objects from the board.
 * The operation can be undone.
 *
 * Syntax: RemoveSelected()
 */
class RemoveSelectedAction : public Action {
public:
    RemoveSelectedAction()
        : Action("RemoveSelected",
                 "Removes any selected objects.",
                 "RemoveSelected()")
    {}

    int execute(int /*argc*/, char** /*argv*/, Coord /*x*/, Coord /*y*/) override {
        if (RemoveSelected()) {
            SetChangedFlag(true);
        }
        return 0;
    }
};

// Auto-register this action
REGISTER_ACTION(RemoveSelectedAction);

}} // namespace pcb::actions
