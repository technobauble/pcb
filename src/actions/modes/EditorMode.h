/*!
 * \file src/actions/modes/EditorMode.h
 *
 * \brief Base class for all editor modes (State Pattern)
 *
 * This header defines the EditorMode base class and ModeManager that together
 * implement the State Pattern for PCB's editor modes. This replaces the
 * 820-line NotifyMode() switch statement with clean, testable mode classes.
 *
 * Architecture:
 * - EditorMode: Abstract base class defining the mode interface
 * - ModeManager: Context class managing mode transitions and event dispatch
 * - 16 concrete mode classes: ArrowMode, LineMode, ArcMode, etc.
 *
 * Benefits:
 * - Each mode is isolated and testable
 * - Clear separation of concerns
 * - Easy to add new modes
 * - Type-safe C++ vs procedural switch statement
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef PCB_EDITOR_MODE_H
#define PCB_EDITOR_MODE_H

#ifdef __cplusplus

#include <memory>
#include <map>
#include <string>

extern "C" {
#include "global.h"
#include "actions/ActionContext.h"
}

namespace pcb {
namespace modes {

/*!
 * \brief Base class for all editor modes
 *
 * This class defines the interface that all editor modes must implement.
 * Each mode handles user interactions (clicks, releases, motion) differently.
 *
 * The State Pattern allows us to encapsulate mode-specific behavior in
 * separate classes instead of a giant switch statement.
 *
 * Example usage:
 * \code
 * class MyMode : public EditorMode {
 * public:
 *     MyMode(ActionContext* context) : EditorMode(context) {}
 *
 *     void onNotify(Coord x, Coord y) override {
 *         // Handle mouse click
 *     }
 *
 *     void onRelease() override {
 *         // Handle mouse release
 *     }
 *
 *     const char* getName() const override { return "MyMode"; }
 *     int getModeId() const override { return MY_MODE; }
 * };
 * \endcode
 */
class EditorMode {
public:
    virtual ~EditorMode() = default;

    /*!
     * \brief Handle mouse click (button press) event
     * \param x X coordinate of click
     * \param y Y coordinate of click
     *
     * This is the core operation - called when the user clicks in the PCB.
     * Different modes respond differently:
     * - LineMode creates line segments
     * - ViaMode places a via
     * - ArrowMode selects objects
     * - RemoveMode deletes objects
     */
    virtual void onNotify(Coord x, Coord y) = 0;

    /*!
     * \brief Handle mouse button release event
     *
     * Called when the user releases the mouse button. Some modes use this
     * (e.g., ArrowMode for selection, to complete drag operations),
     * others ignore it.
     */
    virtual void onRelease() = 0;

    /*!
     * \brief Called when this mode is activated
     *
     * Override to perform mode initialization:
     * - Set cursor shape
     * - Initialize mode-specific state
     * - Reset crosshair/attached objects
     * - Clear previous mode's state
     */
    virtual void onEnter() {}

    /*!
     * \brief Called when this mode is deactivated
     *
     * Override to perform cleanup:
     * - Reset crosshair
     * - Clear attached objects
     * - Release mode-specific resources
     */
    virtual void onExit() {}

    /*!
     * \brief Handle mouse motion event
     * \param x Current X coordinate
     * \param y Current Y coordinate
     *
     * Called when mouse moves. Used for:
     * - Rubberbanding during line/polygon creation
     * - Cursor updates
     * - Preview of pending operations
     */
    virtual void onMotion(Coord x, Coord y) {}

    /*!
     * \brief Handle cancel/escape request
     *
     * Called when user presses Escape or requests cancel. Modes should:
     * - Reset any in-progress operation
     * - Clear attached objects/state
     * - Return to initial state (STATE_FIRST)
     *
     * The default implementation does nothing (for simple modes).
     */
    virtual void onCancel() {}

    /*!
     * \brief Check if mode is idle (not in middle of operation)
     * \return true if no operation in progress, false otherwise
     *
     * Used by Escape handling to decide whether to:
     * - Switch to ARROW_MODE (if idle)
     * - Cancel current operation and stay in mode (if not idle)
     *
     * Default returns true (simple modes are always "idle").
     */
    virtual bool isIdle() const { return true; }

    /*!
     * \brief Get mode name for display/debugging
     * \return Human-readable mode name (e.g., "Line", "Arc", "Arrow")
     */
    virtual const char* getName() const = 0;

    /*!
     * \brief Get mode ID constant
     * \return Mode ID (e.g., LINE_MODE, ARC_MODE, ARROW_MODE)
     */
    virtual int getModeId() const = 0;

    /*!
     * \brief Does this mode allow object selection?
     * \return true if objects can be selected in this mode, false otherwise
     *
     * Most modes don't allow selection (return false).
     * ArrowMode returns true.
     */
    virtual bool allowsSelection() const { return false; }

protected:
    /*!
     * \brief Constructor (protected - only subclasses can instantiate)
     * \param context Shared action context for state access
     */
    explicit EditorMode(ActionContext* context) : context_(context) {}

    ActionContext* context_;  ///< Shared state (Note, crosshair position, etc.)
};

/*!
 * \brief Manages editor mode transitions and event dispatch
 *
 * This class maintains the collection of all mode instances and handles
 * switching between them. It's the "Context" in the State Pattern.
 *
 * Responsibilities:
 * - Owns all mode instances
 * - Dispatches events (notify, release, motion) to current mode
 * - Handles mode transitions (calls onExit/onEnter)
 * - Maintains mode save/restore stack
 *
 * Example usage:
 * \code
 * ModeManager manager(pcb_action_context);
 *
 * // Switch modes
 * manager.setMode(LINE_MODE);
 *
 * // Dispatch events
 * manager.notifyClick(100, 200);  // Calls LineMode::onNotify()
 * manager.notifyRelease();         // Calls LineMode::onRelease()
 *
 * // Save/restore for temporary mode changes
 * manager.saveMode();
 * manager.setMode(REMOVE_MODE);
 * // ... do something ...
 * manager.restoreMode();  // Back to LINE_MODE
 * \endcode
 */
class ModeManager {
public:
    /*!
     * \brief Constructor
     * \param context Shared action context
     *
     * Creates the mode manager and initializes all mode instances.
     */
    explicit ModeManager(ActionContext* context);

    /*!
     * \brief Destructor
     */
    ~ModeManager();

    /*!
     * \brief Switch to a different mode
     * \param mode_id Mode constant (e.g., LINE_MODE, ARC_MODE)
     *
     * Sequence:
     * 1. Call current_mode->onExit() if there is a current mode
     * 2. Look up new mode by ID
     * 3. Set it as current mode
     * 4. Call new_mode->onEnter()
     * 5. Update Settings.Mode for backward compatibility
     */
    void setMode(int mode_id);

    /*!
     * \brief Save current mode ID for later restoration
     *
     * Used by ActionDelete and stroke operations to temporarily
     * switch modes then switch back.
     *
     * Example:
     * \code
     * manager.saveMode();              // Save LINE_MODE
     * manager.setMode(REMOVE_MODE);    // Switch to REMOVE_MODE
     * manager.notifyClick(x, y);       // Delete object
     * manager.restoreMode();           // Back to LINE_MODE
     * \endcode
     */
    void saveMode();

    /*!
     * \brief Restore previously saved mode
     *
     * Switches back to the mode saved by saveMode().
     * Does nothing if no mode was saved.
     */
    void restoreMode();

    /*!
     * \brief Dispatch click event to current mode
     * \param x X coordinate
     * \param y Y coordinate
     *
     * This is called when the user clicks the mouse button.
     * The event is dispatched to current_mode->onNotify(x, y).
     *
     * Also:
     * - Updates context->Note.X and context->Note.Y
     * - Clears rat warnings if enabled
     */
    void notifyClick(Coord x, Coord y);

    /*!
     * \brief Dispatch release event to current mode
     *
     * Called when the user releases the mouse button.
     * Dispatched to current_mode->onRelease().
     */
    void notifyRelease();

    /*!
     * \brief Dispatch motion event to current mode
     * \param x X coordinate
     * \param y Y coordinate
     *
     * Called when the mouse moves.
     * Dispatched to current_mode->onMotion(x, y).
     */
    void notifyMotion(Coord x, Coord y);

    /*!
     * \brief Dispatch cancel event to current mode
     *
     * Called when user presses Escape or requests cancel.
     * Dispatched to current_mode->onCancel().
     */
    void notifyCancel();

    /*!
     * \brief Check if current mode is idle
     * \return true if mode has no operation in progress
     *
     * Used by Escape handling to decide whether to switch to ARROW_MODE.
     */
    bool isCurrentModeIdle() const;

    /*!
     * \brief Get current mode instance
     * \return Pointer to current mode, or nullptr if no mode active
     */
    EditorMode* getCurrentMode() const { return current_mode_; }

    /*!
     * \brief Get current mode ID
     * \return Current mode ID (e.g., LINE_MODE), or NO_MODE if none active
     */
    int getCurrentModeId() const;

private:
    /*!
     * \brief Register a mode instance
     * \param id Mode ID constant
     * \param mode Unique pointer to mode instance
     *
     * Called during initialization to register all mode instances.
     */
    void registerMode(int id, std::unique_ptr<EditorMode> mode);

    /*!
     * \brief Create and register all mode instances
     *
     * Called by constructor to create all 16 mode instances.
     * As we implement modes, they'll be registered here.
     */
    void initializeModes();

    ActionContext* context_;                              ///< Shared state
    std::map<int, std::unique_ptr<EditorMode>> modes_;   ///< All mode instances (by ID)
    EditorMode* current_mode_;                           ///< Current active mode
    int saved_mode_id_;                                  ///< Saved mode for restore
};

} // namespace modes
} // namespace pcb

extern "C" {
    /*!
     * \brief Global mode manager instance
     *
     * This is the single global instance used throughout the application.
     * Initialized by initializeModeManager() in action.c.
     */
    extern pcb::modes::ModeManager* pcb_mode_manager;
}

#endif // __cplusplus

#endif // PCB_EDITOR_MODE_H
