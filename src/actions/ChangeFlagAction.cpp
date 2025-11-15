/*!
 * \file src/actions/ChangeFlagAction.cpp
 *
 * \brief Implementation of ChangeFlag action.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2025 PCB Contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "Action.h"

extern "C" {
#include "coord_types.h"
#include "macro.h"       // For ARG macro
#include "error.h"       // For Message()
#include <stdlib.h>      // For atoi()

// Forward declare ChangeFlag helper (defined in action.c)
void ChangeFlag(char *what, char *flag_name, int value, char *cmd_name);
}

namespace pcb {
namespace actions {

class ChangeFlagAction : public Action {
public:
    ChangeFlagAction()
        : Action("ChangeFlag",
                 "Sets or clears flags on objects.",
                 "ChangeFlag(Object|Selected|SelectedObjects, flag, value)\n"
                 "ChangeFlag(SelectedLines|SelectedPins|SelectedVias, flag, value)\n"
                 "ChangeFlag(SelectedPads|SelectedTexts|SelectedNames, flag, value)\n"
                 "ChangeFlag(SelectedElements, flag, value)\n"
                 "flag = square | octagon | thermal | join\n"
                 "value = 0 or 1")
    {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        // Suppress unused parameter warnings
        (void)x;
        (void)y;

        char *function = ARG (0);
        char *flag = ARG (1);
        int value = argc > 2 ? atoi (argv[2]) : -1;

        if (value != 0 && value != 1)
        {
            Message("Syntax error. Usage:\n%s\n", syntax());
            return 1;
        }

        ChangeFlag (function, flag, value, (char*)"ChangeFlag");
        return 0;
    }
};

REGISTER_ACTION(ChangeFlagAction);

}} // namespace pcb::actions
