/*!
 * \file src/actions/MessageAction.cpp
 *
 * \brief Message action - displays messages to the log window.
 *
 * This is a proof-of-concept for the action.c refactoring strategy.
 * It demonstrates isolated implementation with minimal dependencies.
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

// Only include stable C interfaces (Tier 1 dependencies)
extern "C" {
#include "global.h"
#include "error.h"  // For Message()
}

namespace pcb {
namespace actions {

/*!
 * \brief MessageAction - Display messages to the log window
 *
 * This action is primarily provided for use by other programs which
 * may interface with PCB. If multiple arguments are given, each one
 * is sent to the log window followed by a newline.
 *
 * Syntax: Message(text1, text2, ...)
 */
class MessageAction : public Action {
public:
    MessageAction()
        : Action("Message",
                 "Writes a message to the log window.",
                 "Message(message)")
    {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        // Argument validation
        if (argc < 1) {
            Message("Syntax error. Usage:\n%s\n", syntax());
            return 1;
        }

        // Display each argument on a separate line
        for (int i = 0; i < argc; i++) {
            if (argv[i]) {
                Message(argv[i]);
                Message("\n");
            }
        }

        return 0;
    }
};

// Auto-register this action
REGISTER_ACTION(MessageAction);

}} // namespace pcb::actions
