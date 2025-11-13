/*!
 * \file src/actions/Action.h
 *
 * \brief Base action class and registry for PCB action system.
 *
 * This file defines the C++ action framework that will replace the
 * monolithic action.c file. All actions inherit from the Action base
 * class and register themselves with the ActionRegistry.
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

#ifndef PCB_ACTIONS_ACTION_H
#define PCB_ACTIONS_ACTION_H

#ifdef __cplusplus

#include <string>
#include <map>
#include <vector>

extern "C" {
#include "global.h"
}

namespace pcb {
namespace actions {

/*!
 * \brief Base class for all PCB actions.
 *
 * All user-invokable actions should inherit from this class and
 * implement the execute() method. Actions are automatically registered
 * with the ActionRegistry upon construction.
 *
 * Example usage:
 * \code
 * class MyAction : public Action {
 * public:
 *     MyAction() : Action("MyAction", "Help text", "MyAction(args)") {}
 *
 *     int execute(int argc, char** argv, Coord x, Coord y) override {
 *         // Implementation here
 *         return 0;
 *     }
 * };
 *
 * // Static instance auto-registers
 * static MyAction g_my_action;
 * \endcode
 */
class Action {
public:
    /*!
     * \brief Construct an action with metadata.
     * \param name The action name (e.g., "Select", "Delete")
     * \param help Help text describing what the action does
     * \param syntax Syntax string showing how to invoke the action
     */
    Action(const char* name, const char* help, const char* syntax);

    /*!
     * \brief Construct an action with no help/syntax (for simple actions).
     * \param name The action name
     */
    explicit Action(const char* name);

    virtual ~Action();

    /*!
     * \brief Execute the action.
     * \param argc Number of arguments
     * \param argv Array of argument strings
     * \param x X coordinate (from mouse or crosshair)
     * \param y Y coordinate (from mouse or crosshair)
     * \return 0 on success, non-zero on error
     *
     * Subclasses must implement this method with their action logic.
     */
    virtual int execute(int argc, char** argv, Coord x, Coord y) = 0;

    // Accessors
    const char* name() const { return name_; }
    const char* help() const { return help_; }
    const char* syntax() const { return syntax_; }

protected:
    /*!
     * \brief Get argument at index (safe accessor).
     * \param n Argument index
     * \param argv Argument array
     * \return Argument string, or empty string if out of bounds
     */
    const char* arg(int n, char** argv) const {
        return (argv && argv[n]) ? argv[n] : "";
    }

    /*!
     * \brief Check if argument exists.
     * \param n Argument index
     * \param argc Argument count
     * \return true if argument exists
     */
    bool hasArg(int n, int argc) const {
        return n < argc;
    }

private:
    const char* name_;
    const char* help_;
    const char* syntax_;
};

/*!
 * \brief Singleton registry for all actions.
 *
 * The ActionRegistry maintains a mapping from action names to
 * Action instances. Actions register themselves upon construction.
 * The registry provides lookup functionality for action execution.
 */
class ActionRegistry {
public:
    /*!
     * \brief Get the singleton instance.
     * \return Reference to the global ActionRegistry
     */
    static ActionRegistry& instance();

    /*!
     * \brief Register an action.
     * \param action Pointer to action to register (must remain valid)
     *
     * Called automatically by Action constructor. Actions are not
     * owned by the registry - they're typically static instances.
     */
    void registerAction(Action* action);

    /*!
     * \brief Look up an action by name.
     * \param name Action name (case-sensitive)
     * \return Pointer to action, or nullptr if not found
     */
    Action* lookup(const std::string& name);

    /*!
     * \brief Get all registered actions.
     * \return Vector of all action pointers
     */
    std::vector<Action*> allActions() const;

    /*!
     * \brief Get number of registered actions.
     * \return Count of registered actions
     */
    size_t count() const { return actions_.size(); }

private:
    ActionRegistry() = default;
    ~ActionRegistry() = default;

    // Prevent copying
    ActionRegistry(const ActionRegistry&) = delete;
    ActionRegistry& operator=(const ActionRegistry&) = delete;

    std::map<std::string, Action*> actions_;
};

/*!
 * \brief Helper macro for registering actions.
 *
 * Use this macro to create a static instance that auto-registers.
 *
 * Example:
 * \code
 * REGISTER_ACTION(MyAction)
 * \endcode
 *
 * This creates a static instance called g_MyAction_instance.
 */
#define REGISTER_ACTION(ActionClass) \
    static ActionClass g_##ActionClass##_instance

} // namespace actions
} // namespace pcb

#endif // __cplusplus

#endif // PCB_ACTIONS_ACTION_H
