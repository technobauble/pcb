/*!
 * \file src/actions/ClrFlagAction.cpp
 *
 * \brief Implementation of ClrFlag action.
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

// Forward declare ChangeFlag helper (defined in action.c)
void ChangeFlag(char *what, char *flag_name, int value, char *cmd_name);
}

namespace pcb {
namespace actions {

class ClrFlagAction : public Action {
public:
    ClrFlagAction()
        : Action("ClrFlag",
                 "Clears flags on objects.",
                 "ClrFlag(Object|Selected|SelectedObjects, flag)\n"
                 "ClrFlag(SelectedLines|SelectedPins|SelectedVias, flag)\n"
                 "ClrFlag(SelectedPads|SelectedTexts|SelectedNames, flag)\n"
                 "ClrFlag(SelectedElements, flag)\n"
                 "flag = square | octagon | thermal | join")
    {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        // Suppress unused parameter warnings
        (void)argc;
        (void)x;
        (void)y;

        char *function = ARG (0);
        char *flag = ARG (1);
        ChangeFlag (function, flag, 0, (char*)"ClrFlag");
        return 0;
    }
};

REGISTER_ACTION(ClrFlagAction);

}} // namespace pcb::actions
