/*!
 * \file src/actions/Action.cpp
 *
 * \brief Implementation of Action base class and registry.
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
#include <iostream>

namespace pcb {
namespace actions {

//-----------------------------------------------------------------------------
// Action implementation
//-----------------------------------------------------------------------------

Action::Action(const char* name, const char* help, const char* syntax)
    : name_(name)
    , help_(help)
    , syntax_(syntax)
{
    // Auto-register with the registry
    ActionRegistry::instance().registerAction(this);
}

Action::Action(const char* name)
    : name_(name)
    , help_("")
    , syntax_("")
{
    // Auto-register with the registry
    ActionRegistry::instance().registerAction(this);
}

Action::~Action()
{
    // Note: We don't unregister because actions are typically static
    // instances that live for the entire program lifetime.
}

//-----------------------------------------------------------------------------
// ActionRegistry implementation
//-----------------------------------------------------------------------------

ActionRegistry& ActionRegistry::instance()
{
    static ActionRegistry registry;
    return registry;
}

void ActionRegistry::registerAction(Action* action)
{
    if (!action) {
        std::cerr << "ActionRegistry: Attempted to register null action" << std::endl;
        return;
    }

    if (!action->name() || !action->name()[0]) {
        std::cerr << "ActionRegistry: Attempted to register action with empty name" << std::endl;
        return;
    }

    std::string name = action->name();

    // Check for duplicate registration
    if (actions_.find(name) != actions_.end()) {
        std::cerr << "ActionRegistry: Warning - action '" << name
                  << "' registered multiple times" << std::endl;
        return;
    }

    actions_[name] = action;

    #ifdef DEBUG_ACTIONS
    std::cout << "ActionRegistry: Registered action '" << name << "'" << std::endl;
    #endif
}

Action* ActionRegistry::lookup(const std::string& name)
{
    auto it = actions_.find(name);
    return (it != actions_.end()) ? it->second : nullptr;
}

std::vector<Action*> ActionRegistry::allActions() const
{
    std::vector<Action*> result;
    result.reserve(actions_.size());

    for (const auto& pair : actions_) {
        result.push_back(pair.second);
    }

    return result;
}

} // namespace actions
} // namespace pcb
