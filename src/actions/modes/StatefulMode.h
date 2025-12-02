/*!
 * \file src/actions/modes/StatefulMode.h
 *
 * \brief Base class for multi-click editor modes with state machines
 *
 * StatefulMode provides a foundation for modes that track state across
 * multiple clicks (e.g., LINE_MODE needs first click for start point,
 * second click for end point).
 *
 * Subclasses specify which attachment type they use by overriding
 * getAttachment(). State is managed through the IAttachedState interface.
 *
 * Class Hierarchy:
 *   EditorMode (abstract base)
 *   └── StatefulMode (state machine via IAttachedState)
 *       └── TwoClickMode (convenience for exactly 2 states)
 *           ├── ObjectTransferMode → CopyMode, MoveMode
 *           └── InsertPointMode
 *       └── LineMode, ArcMode, RectangleMode (multi-state)
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

#ifndef PCB_STATEFUL_MODE_H
#define PCB_STATEFUL_MODE_H

#include "EditorMode.h"
#include "attached_state.h"
#include "crosshair_attachments.h"

namespace pcb {
namespace modes {

/*!
 * \brief Base class for modes with multi-click state machines
 *
 * Many editor modes track state across multiple clicks:
 * - LINE_MODE: First click sets start, second sets end
 * - ARC_MODE: First click sets center, second sets endpoint
 * - MOVE_MODE: First click picks object, second places it
 *
 * StatefulMode provides:
 * - State access via IAttachedState interface (getState/setState/reset)
 * - Convenience methods isFirstState(), isSecondState(), etc.
 * - Default onExit() that resets state
 * - Template method pattern: subclasses override getAttachment()
 *
 * Example subclass:
 * \code
 * class LineMode : public StatefulMode {
 * protected:
 *     IAttachedState& getAttachment() override {
 *         return attachment_;
 *     }
 *
 * public:
 *     LineMode(ActionContext* ctx)
 *         : StatefulMode(ctx)
 *         , attachment_(Crosshair.AttachedLine)
 *     {}
 *
 *     void onNotify(Coord x, Coord y) override {
 *         if (isFirstState()) {
 *             // Set start point
 *             setState(STATE_SECOND);
 *         } else {
 *             // Create line, stay in STATE_SECOND for next line
 *         }
 *     }
 *
 * private:
 *     AttachedLineAdapter attachment_;
 * };
 * \endcode
 */
class StatefulMode : public EditorMode {
public:
    /*!
     * \brief Get current state
     * \return Current state (STATE_FIRST, STATE_SECOND, or STATE_THIRD)
     */
    int getState() const {
        return const_cast<StatefulMode*>(this)->getAttachment().getState();
    }

    /*!
     * \brief Set current state
     * \param state New state value
     */
    void setState(int state) {
        getAttachment().setState(state);
    }

    /*!
     * \brief Reset attachment to initial state
     *
     * Calls the attachment's reset() which sets state to STATE_FIRST
     * and clears associated data (points, object references, etc.)
     */
    void resetState() {
        getAttachment().reset();
    }

    /*!
     * \brief Check if in first (initial) state
     * \return true if state is STATE_FIRST
     */
    bool isFirstState() const { return getState() == STATE_FIRST; }

    /*!
     * \brief Check if in second state
     * \return true if state is STATE_SECOND
     */
    bool isSecondState() const { return getState() == STATE_SECOND; }

    /*!
     * \brief Check if in third state
     * \return true if state is STATE_THIRD
     */
    bool isThirdState() const { return getState() == STATE_THIRD; }

    /*!
     * \brief Called when mode is exited
     *
     * Default implementation resets the attachment state.
     * Subclasses can override to add additional cleanup.
     */
    void onExit() override {
        resetState();
    }

    /*!
     * \brief Handle cancel/escape request
     *
     * Default implementation resets to STATE_FIRST.
     * Subclasses can override for additional cleanup.
     */
    void onCancel() override {
        resetState();
    }

    /*!
     * \brief Check if mode is idle (no operation in progress)
     * \return true if in STATE_FIRST (nothing started)
     *
     * For stateful modes, "idle" means we're waiting for the first click.
     * If in STATE_SECOND or STATE_THIRD, an operation is in progress.
     */
    bool isIdle() const override {
        return isFirstState();
    }

protected:
    /*!
     * \brief Constructor
     * \param context Shared action context
     */
    explicit StatefulMode(ActionContext* context) : EditorMode(context) {}

    /*!
     * \brief Get the attachment for this mode (Template Method)
     * \return Reference to the mode's IAttachedState
     *
     * Subclasses MUST override this to return their specific attachment.
     * For example:
     * - LineMode returns an AttachedLineAdapter
     * - ArcMode returns an AttachedBoxAdapter
     * - MoveMode returns an AttachedObjectAdapter
     */
    virtual IAttachedState& getAttachment() = 0;
};

/*!
 * \brief Convenience base for modes with exactly two states
 *
 * Many modes only use STATE_FIRST and STATE_SECOND:
 * - Click 1: Select/pick something (STATE_FIRST → STATE_SECOND)
 * - Click 2: Complete action (STATE_SECOND → done or back to FIRST)
 *
 * TwoClickMode provides:
 * - advanceState(): Move from FIRST to SECOND
 * - completeAction(): Called when second click happens
 * - resetToFirst(): Return to initial state
 *
 * Example:
 * \code
 * class CopyMode : public TwoClickMode {
 *     void onNotify(Coord x, Coord y) override {
 *         if (isFirstState()) {
 *             if (findAndAttachObject(x, y)) {
 *                 advanceState();
 *             }
 *         } else {
 *             copyObjectToLocation(x, y);
 *             // Stay in SECOND for more copies, or:
 *             // resetToFirst();
 *         }
 *     }
 * };
 * \endcode
 */
class TwoClickMode : public StatefulMode {
public:
    /*!
     * \brief Advance from STATE_FIRST to STATE_SECOND
     *
     * Call this after the first click successfully picks/selects
     * something and the mode should wait for the second click.
     */
    void advanceState() {
        if (isFirstState()) {
            setState(STATE_SECOND);
        }
    }

    /*!
     * \brief Reset to initial state
     *
     * Call this after completing an action or to cancel.
     */
    void resetToFirst() {
        resetState();
    }

protected:
    /*!
     * \brief Constructor
     * \param context Shared action context
     */
    explicit TwoClickMode(ActionContext* context) : StatefulMode(context) {}
};

} // namespace modes
} // namespace pcb

#endif // PCB_STATEFUL_MODE_H
