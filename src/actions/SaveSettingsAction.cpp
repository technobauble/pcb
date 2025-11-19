/*!
 * \file src/actions/SaveSettingsAction.cpp
 *
 * \brief SaveSettings action - saves PCB settings to file.
 *
 * Second action in the migration proof of concept. Demonstrates:
 * - Optional argument handling
 * - Calling HID functions
 * - Simple action with minimal dependencies
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
#include "coord_types.h"
#include <string.h> // For strncasecmp()
#include <strings.h> // For strncasecmp() on some systems

// Forward declare hid_save_settings instead of including hid.h
// (hid.h has heavy dependencies we don't need)
void hid_save_settings(int locally);
}

namespace pcb {
namespace actions {

/*!
 * \brief SaveSettingsAction - Save PCB settings to file
 *
 * Saves current settings either locally (project-specific) or
 * globally (user preferences).
 *
 * Syntax: SaveSettings([local])
 * - No args or "local" → save locally
 * - Other args → save globally
 */
class SaveSettingsAction : public Action {
public:
    SaveSettingsAction()
        : Action("SaveSettings",
                 "Saves settings",
                 "SaveSettings([local])")
    {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        // Suppress unused parameter warnings
        (void)x;
        (void)y;

        // Check if "local" argument provided
        int locally = 0;
        if (argc > 0 && argv[0]) {
            locally = (strncasecmp(argv[0], "local", 5) == 0) ? 1 : 0;
        }

        // Call HID function to save settings
        hid_save_settings(locally);

        return 0;
    }
};

// Auto-register this action
REGISTER_ACTION(SaveSettingsAction);

}} // namespace pcb::actions
