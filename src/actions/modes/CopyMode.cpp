/*!
 * \file src/actions/modes/CopyMode.cpp
 *
 * \brief Copy object mode implementation
 *
 * Allows the user to copy PCB objects by clicking to select an object,
 * then clicking to place the copy.
 *
 * State machine:
 * - STATE_FIRST: Click to select object to copy
 * - STATE_SECOND: Click to place the copy
 *
 * Uses AttachedObject to track the selected object.
 *
 * Extracted from action.c COPY_MODE case in NotifyMode().
 */

#include "StatefulMode.h"
#include "ModesCommon.h"

extern "C" {
#include "copy.h"
#include "crosshair.h"
}

namespace pcb {
namespace modes {

/*!
 * \brief Copy object mode
 *
 * Copies PCB objects (elements, vias, lines, etc.) to a new location.
 *
 * Features:
 * - Click on object to select for copying
 * - Object follows cursor (rubber-band)
 * - Click to place copy
 * - Returns to STATE_FIRST after copy for another operation
 */
class CopyMode : public TwoClickMode {
public:
    explicit CopyMode(ActionContext* context)
        : TwoClickMode(context)
        , attachment_(Crosshair.AttachedObject)
    {}

    void onNotify(Coord x, Coord y) override {
        switch (getState()) {
        case STATE_FIRST:
            selectObject();
            break;

        case STATE_SECOND:
            copyObject();
            break;
        }
    }

    void onRelease() override {
        // Copy mode doesn't use release
    }

    const char* getName() const override { return "Copy"; }
    int getModeId() const override { return COPY_MODE; }

protected:
    IAttachedState& getAttachment() override {
        return attachment_;
    }

private:
    AttachedObjectAdapter attachment_;

    /*!
     * \brief Select object to copy
     *
     * Searches for a copyable object at the click location.
     * If found, attaches it for copying and advances to STATE_SECOND.
     */
    void selectObject() {
        // Search for copyable object
        Crosshair.AttachedObject.Type = SearchScreen(
            context_->Note.X, context_->Note.Y,
            COPY_TYPES,
            &Crosshair.AttachedObject.Ptr1,
            &Crosshair.AttachedObject.Ptr2,
            &Crosshair.AttachedObject.Ptr3);

        if (Crosshair.AttachedObject.Type != NO_TYPE) {
            // Attach the object for copying (sets up rubber-band)
            AttachForCopy(context_->Note.X, context_->Note.Y);
            // State is advanced by AttachForCopy
        }
    }

    /*!
     * \brief Place the copy
     *
     * Copies the attached object to the current cursor position.
     * Resets state for next copy operation.
     */
    void copyObject() {
        // Calculate delta from original position
        Coord dx = context_->Note.X - Crosshair.AttachedObject.X;
        Coord dy = context_->Note.Y - Crosshair.AttachedObject.Y;

        // Copy the object
        CopyObject(
            Crosshair.AttachedObject.Type,
            Crosshair.AttachedObject.Ptr1,
            Crosshair.AttachedObject.Ptr2,
            Crosshair.AttachedObject.Ptr3,
            dx, dy);

        SetChangedFlag(true);

        // Reset for next copy
        Crosshair.AttachedObject.Type = NO_TYPE;
        resetToFirst();
    }
};

// Factory function for ModeManager registration
std::unique_ptr<EditorMode> createCopyMode(ActionContext* context) {
    return std::unique_ptr<EditorMode>(new CopyMode(context));
}

} // namespace modes
} // namespace pcb
