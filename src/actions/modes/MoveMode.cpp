/*!
 * \file src/actions/modes/MoveMode.cpp
 *
 * \brief Move object mode implementation
 *
 * Allows the user to move PCB objects by clicking to select an object,
 * then clicking to place it at a new location.
 *
 * State machine:
 * - STATE_FIRST: Click to select object to move
 * - STATE_SECOND: Click to place object at new location
 *
 * Uses AttachedObject to track the selected object.
 * Respects object lock flags - locked objects cannot be moved.
 *
 * Extracted from action.c MOVE_MODE case in NotifyMode().
 */

#include "StatefulMode.h"
#include "ModesCommon.h"

extern "C" {
#include "move.h"
#include "crosshair.h"
#include "rubberband.h"
#include "error.h"
}

namespace pcb {
namespace modes {

/*!
 * \brief Move object mode
 *
 * Moves PCB objects (elements, vias, lines, etc.) to a new location.
 * Unlike copy, move removes the object from its original position.
 *
 * Features:
 * - Click on object to select for moving
 * - Locked objects cannot be moved (shows message)
 * - Object follows cursor with rubber-band connections
 * - Click to place at new location
 * - Returns to STATE_FIRST after move for another operation
 */
class MoveMode : public TwoClickMode {
public:
    explicit MoveMode(ActionContext* context)
        : TwoClickMode(context)
        , attachment_(Crosshair.AttachedObject)
    {}

    void onNotify(Coord x, Coord y) override {
        int state = getState();
        Message("MoveMode::onNotify state=%d (FIRST=%d, SECOND=%d)\n",
                state, STATE_FIRST, STATE_SECOND);
        switch (state) {
        case STATE_FIRST:
            Message("  -> selectObject\n");
            selectObject();
            break;

        case STATE_SECOND:
            Message("  -> moveObject\n");
            moveObject();
            break;
        }
    }

    void onRelease() override {
        // Move mode doesn't use release
    }

    const char* getName() const override { return "Move"; }
    int getModeId() const override { return MOVE_MODE; }

protected:
    IAttachedState& getAttachment() override {
        return attachment_;
    }

private:
    AttachedObjectAdapter attachment_;

    /*!
     * \brief Select object to move
     *
     * Searches for a movable object at the click location.
     * Checks lock flag - locked objects cannot be moved.
     * If found and unlocked, attaches for moving and advances to STATE_SECOND.
     */
    void selectObject() {
        // Search for movable object
        Crosshair.AttachedObject.Type = SearchScreen(
            context_->Note.X, context_->Note.Y,
            MOVE_TYPES,
            &Crosshair.AttachedObject.Ptr1,
            &Crosshair.AttachedObject.Ptr2,
            &Crosshair.AttachedObject.Ptr3);

        if (Crosshair.AttachedObject.Type != NO_TYPE) {
            // Check if object is locked
            if (TEST_FLAG(LOCKFLAG, (PinType*)Crosshair.AttachedObject.Ptr2)) {
                Message(_("Sorry, the object is locked\n"));
                Crosshair.AttachedObject.Type = NO_TYPE;
            } else {
                // Attach the object for moving (sets up rubber-band)
                AttachForCopy(context_->Note.X, context_->Note.Y);
                // State is advanced by AttachForCopy
            }
        }
    }

    /*!
     * \brief Move the object to new location
     *
     * Moves the attached object to the current cursor position.
     * Also moves rubber-band connections.
     * Resets state for next move operation.
     */
    void moveObject() {
        // Calculate delta from original position
        Coord dx = context_->Note.X - Crosshair.AttachedObject.X;
        Coord dy = context_->Note.Y - Crosshair.AttachedObject.Y;

        // Move the object with its rubber-band connections
        MoveObjectAndRubberband(
            Crosshair.AttachedObject.Type,
            Crosshair.AttachedObject.Ptr1,
            Crosshair.AttachedObject.Ptr2,
            Crosshair.AttachedObject.Ptr3,
            dx, dy);

        // Clear local reference point
        SetLocalRef(0, 0, false);

        SetChangedFlag(true);

        // Reset for next move
        Crosshair.AttachedObject.Type = NO_TYPE;
        resetToFirst();
    }
};

// Factory function for ModeManager registration
std::unique_ptr<EditorMode> createMoveMode(ActionContext* context) {
    return std::unique_ptr<EditorMode>(new MoveMode(context));
}

} // namespace modes
} // namespace pcb
