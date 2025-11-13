/*!
 * \file src/actions/action_bridge.cpp
 *
 * \brief Implementation of C/C++ bridge for action system.
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

#include "action_bridge.h"
#include "Action.h"

#include <iostream>
#include <vector>
#include <cstring>

// Static storage for HID_Action array (for backwards compatibility)
static std::vector<HID_Action> g_hid_action_list;

extern "C" {

//-----------------------------------------------------------------------------
// C-callable bridge functions
//-----------------------------------------------------------------------------

int pcb_action_execute(const char* name, int argc, char** argv, Coord x, Coord y)
{
    if (!name) {
        std::cerr << "pcb_action_execute: NULL action name" << std::endl;
        return -1;
    }

    pcb::actions::ActionRegistry& registry = pcb::actions::ActionRegistry::instance();
    pcb::actions::Action* action = registry.lookup(name);

    if (!action) {
        // Action not found - this is not necessarily an error, as the
        // caller might want to try other action sources (e.g., old C actions)
        return -1;
    }

    try {
        return action->execute(argc, argv, x, y);
    }
    catch (const std::exception& e) {
        std::cerr << "pcb_action_execute: Exception in action '" << name
                  << "': " << e.what() << std::endl;
        return -1;
    }
    catch (...) {
        std::cerr << "pcb_action_execute: Unknown exception in action '"
                  << name << "'" << std::endl;
        return -1;
    }
}

void pcb_action_init(void)
{
    // C++ static initialization will have already occurred, but we can
    // use this as an explicit initialization point if needed in the future.

    #ifdef DEBUG_ACTIONS
    std::cout << "pcb_action_init: C++ action system initialized with "
              << pcb::actions::ActionRegistry::instance().count()
              << " actions" << std::endl;
    #endif
}

int pcb_action_count(void)
{
    return static_cast<int>(pcb::actions::ActionRegistry::instance().count());
}

int pcb_action_exists(const char* name)
{
    if (!name) {
        return 0;
    }

    pcb::actions::Action* action = pcb::actions::ActionRegistry::instance().lookup(name);
    return (action != nullptr) ? 1 : 0;
}

HID_Action* pcb_action_get_hid_list(int* count)
{
    if (!count) {
        return nullptr;
    }

    pcb::actions::ActionRegistry& registry = pcb::actions::ActionRegistry::instance();
    std::vector<pcb::actions::Action*> actions = registry.allActions();

    // Rebuild the HID_Action list
    g_hid_action_list.clear();
    g_hid_action_list.reserve(actions.size());

    for (auto* action : actions) {
        HID_Action hid_action;
        hid_action.name = action->name();
        hid_action.need_coord_msg = nullptr;  // Most actions don't need this

        // We need to create a wrapper function that bridges to the C++ action
        // For now, we'll leave this as nullptr and rely on pcb_action_execute()
        hid_action.trigger_cb = nullptr;

        hid_action.description = action->help();
        hid_action.syntax = action->syntax();

        g_hid_action_list.push_back(hid_action);
    }

    *count = static_cast<int>(g_hid_action_list.size());
    return g_hid_action_list.empty() ? nullptr : g_hid_action_list.data();
}

void pcb_action_list_all(void)
{
    pcb::actions::ActionRegistry& registry = pcb::actions::ActionRegistry::instance();
    std::vector<pcb::actions::Action*> actions = registry.allActions();

    std::cout << "Registered C++ Actions (" << actions.size() << "):" << std::endl;
    std::cout << "========================================" << std::endl;

    for (auto* action : actions) {
        std::cout << "  " << action->name();

        if (action->syntax() && action->syntax()[0]) {
            std::cout << " - " << action->syntax();
        }

        std::cout << std::endl;

        if (action->help() && action->help()[0]) {
            std::cout << "      " << action->help() << std::endl;
        }
    }

    if (actions.empty()) {
        std::cout << "  (none)" << std::endl;
    }
}

} // extern "C"
