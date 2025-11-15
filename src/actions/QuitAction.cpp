/*!
 * \file src/actions/QuitAction.cpp
 *
 * \brief Implementation of Quit action.
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
#include "data.h"        // For PCB extern
#include "hid.h"         // For gui extern and HID_CLOSE_CONFIRM_OK
#include "misc.h"        // For QuitApplication()
#include <stdlib.h>      // For exit()
#include <strings.h>     // For strcasecmp()
}

namespace pcb {
namespace actions {

class QuitAction : public Action {
public:
    QuitAction()
        : Action("Quit",
                 "Quits the program.",
                 "Quit([force])")
    {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        // Suppress unused parameter warnings
        (void)x;
        (void)y;

        char *force = ARG (0);
        if (force && strcasecmp (force, "force") == 0)
        {
            PCB->Changed = 0;
            exit (0);
        }
        if (!PCB->Changed || gui->close_confirm_dialog () == HID_CLOSE_CONFIRM_OK)
            QuitApplication ();
        return 1;
    }
};

REGISTER_ACTION(QuitAction);

}} // namespace pcb::actions
