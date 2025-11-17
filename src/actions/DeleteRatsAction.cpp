/*!
 * \file src/actions/DeleteRatsAction.cpp
 *
 * \brief DeleteRats action - deletes rat lines.
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
#include "data.h"    // For Settings
#include "error.h"   // For Message()
#include "remove.h"  // For DeleteRats()
#include "action.h"  // For ClearWarnings()
#include "set.h"     // For SetChangedFlag()
}

#include <cstring>   // For strcmp

namespace pcb {
namespace actions {

/*!
 * \brief DeleteRatsAction - Delete rat lines
 *
 * This action deletes rat lines (airwires). You can delete all rats
 * or only selected rats.
 *
 * Syntax: DeleteRats(AllRats|SelectedRats|Selected)
 */
class DeleteRatsAction : public Action {
public:
    DeleteRatsAction()
        : Action("DeleteRats",
                 "Delete rat lines.",
                 "DeleteRats(AllRats|SelectedRats|Selected)")
    {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        if (argc < 1) {
            Message("Syntax error. Usage:\n%s\n", syntax());
            return 1;
        }

        const char* function = argv[0];

        if (Settings.RatWarn) {
            ClearWarnings();
        }

        if (strcmp(function, "AllRats") == 0) {
            if (DeleteRats(false)) {
                SetChangedFlag(true);
            }
        }
        else if (strcmp(function, "SelectedRats") == 0 || strcmp(function, "Selected") == 0) {
            if (DeleteRats(true)) {
                SetChangedFlag(true);
            }
        }
        else {
            Message("DeleteRats: Unknown argument '%s'. Use AllRats, SelectedRats, or Selected.\n", function);
            return 1;
        }

        return 0;
    }
};

// Auto-register this action
REGISTER_ACTION(DeleteRatsAction);

}} // namespace pcb::actions
