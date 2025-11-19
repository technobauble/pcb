/*!
 * \file src/actions/AutoPlaceSelectedAction.cpp
 *
 * \brief AutoPlaceSelected action - auto-place selected elements.
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
#include "autoplace.h"   // For AutoPlaceSelected()
#include "set.h"         // For SetChangedFlag()
#include "hid.h"         // For gui, hid_action()
}

namespace pcb {
namespace actions {

/*!
 * \brief AutoPlaceSelectedAction - Auto-place selected elements
 *
 * Automatically places selected elements on the board.
 * This operation cannot be undone, so it prompts for confirmation.
 *
 * Syntax: AutoPlaceSelected()
 */
class AutoPlaceSelectedAction : public Action {
public:
    AutoPlaceSelectedAction()
        : Action("AutoPlaceSelected",
                 "Auto-place selected elements.",
                 "AutoPlaceSelected()")
    {}

    int execute(int /*argc*/, char** /*argv*/, Coord /*x*/, Coord /*y*/) override {
        hid_action(const_cast<char*>("Busy"));

        if (gui->confirm_dialog(const_cast<char*>(_("Auto-placement can NOT be undone.\n"
                                                      "Do you want to continue anyway?\n")), 0)) {
            if (AutoPlaceSelected()) {
                SetChangedFlag(true);
            }
        }

        return 0;
    }
};

// Auto-register this action
REGISTER_ACTION(AutoPlaceSelectedAction);

}} // namespace pcb::actions
