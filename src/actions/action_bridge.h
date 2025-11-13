/*!
 * \file src/actions/action_bridge.h
 *
 * \brief C/C++ bridge interface for action system.
 *
 * This file provides a C-compatible interface to the C++ action system.
 * It allows existing C code to call C++ actions without modification.
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

#ifndef PCB_ACTIONS_ACTION_BRIDGE_H
#define PCB_ACTIONS_ACTION_BRIDGE_H

#include "global.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief Execute a C++ action from C code.
 * \param name Action name (e.g., "Select", "Delete")
 * \param argc Number of arguments
 * \param argv Array of argument strings
 * \param x X coordinate
 * \param y Y coordinate
 * \return 0 on success, non-zero on error, -1 if action not found
 *
 * This is the primary entry point for executing C++ actions from
 * existing C code. It looks up the action in the C++ registry and
 * calls its execute() method.
 *
 * Example usage from C:
 * \code
 * int result = pcb_action_execute("Select", argc, argv, x, y);
 * if (result == -1) {
 *     fprintf(stderr, "Action not found\n");
 * }
 * \endcode
 */
int pcb_action_execute(const char* name, int argc, char** argv, Coord x, Coord y);

/*!
 * \brief Initialize the C++ action system.
 *
 * This should be called early in main() to ensure the C++ runtime
 * is initialized. In practice, C++ static initialization will have
 * already registered all actions, but this provides an explicit
 * initialization point if needed.
 */
void pcb_action_init(void);

/*!
 * \brief Get the number of registered C++ actions.
 * \return Number of registered actions
 *
 * Useful for debugging and diagnostics.
 */
int pcb_action_count(void);

/*!
 * \brief Check if a C++ action is registered.
 * \param name Action name to check
 * \return 1 if registered, 0 if not found
 */
int pcb_action_exists(const char* name);

/*!
 * \brief Get HID_Action array for backwards compatibility.
 * \param count Output parameter for array size
 * \return Array of HID_Action structures, or NULL on error
 *
 * This function builds a HID_Action array from the C++ action registry
 * for compatibility with the existing HID interface. The returned
 * array is valid until the next call to this function.
 *
 * Note: The caller should NOT free the returned array.
 */
HID_Action* pcb_action_get_hid_list(int* count);

/*!
 * \brief Print all registered actions (for debugging).
 *
 * Prints a list of all registered C++ actions to stdout.
 * Useful for verification during development.
 */
void pcb_action_list_all(void);

#ifdef __cplusplus
}
#endif

#endif // PCB_ACTIONS_ACTION_BRIDGE_H
