/*!
 * \file src/actions/AtomicAction.cpp
 *
 * \brief Atomic action - groups multiple actions into a single undo operation.
 *
 * This action allows making multiple-action bindings into an atomic
 * operation that will be undone by a single Undo command.
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
#include "undo.h"    // For SaveUndoSerialNumber(), etc.
#include "data.h"    // For Bumped
}

#include <cstring>   // For strcmp

namespace pcb {
namespace actions {

/*!
 * \brief AtomicAction - Group multiple actions into one undo operation
 *
 * This action allows making multiple-action bindings into an atomic
 * operation that will be undone by a single Undo command. For example,
 * to optimize rats and then select all rats:
 *
 *   Action(Atomic(Save) OptimizeRats() Atomic(Restore) SelectRats() Atomic(Block))
 *
 * This will be undone by a single Undo, even though it involves two actions.
 *
 * Syntax: Atomic(Save|Restore|Close|Block)
 *
 * - Save: Save the current undo serial number
 * - Restore: Restore the previously saved serial number
 * - Close: Restore and increment (group as one undo)
 * - Block: Restore and conditionally increment (if changes were made)
 */
class AtomicAction : public Action {
public:
    AtomicAction()
        : Action("Atomic",
                 "Save or restore the undo serial number.",
                 "Atomic(Save|Restore|Close|Block)")
    {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        // Argument validation
        if (argc != 1) {
            Message("Syntax error. Usage:\n%s\n", syntax());
            return 1;
        }

        const char* operation = argv[0];

        if (strcmp(operation, "Save") == 0) {
            SaveUndoSerialNumber();
        }
        else if (strcmp(operation, "Restore") == 0) {
            RestoreUndoSerialNumber();
        }
        else if (strcmp(operation, "Close") == 0) {
            RestoreUndoSerialNumber();
            IncrementUndoSerialNumber();
        }
        else if (strcmp(operation, "Block") == 0) {
            RestoreUndoSerialNumber();
            if (Bumped) {
                IncrementUndoSerialNumber();
            }
        }
        else {
            Message("Atomic: Unknown operation '%s'. Use Save, Restore, Close, or Block.\n", operation);
            return 1;
        }

        return 0;
    }
};

// Auto-register this action
REGISTER_ACTION(AtomicAction);

}} // namespace pcb::actions
