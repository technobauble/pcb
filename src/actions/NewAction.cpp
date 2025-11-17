/*!
 * \file src/actions/NewAction.cpp
 *
 * \brief New action - creates a new layout.
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
#include "data.h"         // For Settings
#include "file.h"         // For SaveInTMP()
#include "remove.h"       // For RemovePCB()
#include "create.h"       // For CreateNewPCB(), CreateNewPCBPost()
#include "misc.h"         // For CenterDisplay(), ResetStackAndVisibility()
#include "draw.h"         // For Redraw()
#include "crosshair.h"    // For notify_crosshair_change()
#include "hid.h"          // For gui, hid_action()
}

#include <cstring>        // For strdup()

namespace pcb {
namespace actions {

/*!
 * \brief NewAction - Create a new layout
 *
 * Starts a new layout, optionally with a given name.
 * Prompts for confirmation if there are unsaved changes.
 *
 * Syntax: New([name])
 */
class NewAction : public Action {
public:
    NewAction()
        : Action("New",
                 "Starts a new layout.",
                 "New([name])")
    {}

    int execute(int argc, char** argv, Coord /*x*/, Coord /*y*/) override {
        char* name = (argc > 0) ? argv[0] : nullptr;

        if (!PCB->Changed || gui->confirm_dialog(_("OK to clear layout data?"), 0)) {
            if (name) {
                name = strdup(name);
            } else {
                name = gui->prompt_for(_("Enter the layout name:"), "");
            }

            if (!name) {
                return 1;
            }

            notify_crosshair_change(false);

            // Do emergency saving and clear the old struct
            if (PCB->Changed && Settings.SaveInTMP) {
                SaveInTMP();
            }

            RemovePCB(PCB);
            PCB = nullptr;
            PCB = CreateNewPCB();
            CreateNewPCBPost(PCB, 1);

            // Setup the new name and reset some values to default
            free(PCB->Name);
            PCB->Name = name;

            ResetStackAndVisibility();
            CenterDisplay(PCB->MaxWidth / 2, PCB->MaxHeight / 2, false);
            Redraw();

            hid_action("PCBChanged");
            notify_crosshair_change(true);

            return 0;
        }

        return 1;
    }
};

// Auto-register this action
REGISTER_ACTION(NewAction);

}} // namespace pcb::actions
