/*!
 * \file src/actions/ExecCommandAction.cpp
 *
 * \brief ExecCommand action - runs a system command.
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
#include "error.h"   // For Message()
}

#include <cstdlib>   // For system()

namespace pcb {
namespace actions {

/*!
 * \brief ExecCommandAction - Run a system command
 *
 * Runs the given command, which is a system executable.
 *
 * Syntax: ExecCommand(command)
 */
class ExecCommandAction : public Action {
public:
    ExecCommandAction()
        : Action("ExecCommand",
                 "Runs a command.",
                 "ExecCommand(command)")
    {}

    int execute(int argc, char** argv, Coord /*x*/, Coord /*y*/) override {
        // Argument validation
        if (argc < 1) {
            Message("Syntax error. Usage:\n%s\n", syntax());
            return 1;
        }

        const char* command = argv[0];

        if (system(command) != 0) {
            return 1;
        }

        return 0;
    }
};

// Auto-register this action
REGISTER_ACTION(ExecCommandAction);

}} // namespace pcb::actions
