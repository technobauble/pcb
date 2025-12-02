/*!
 * \file src/actions/modes/RotateMode.cpp
 *
 * \brief Rotate object mode implementation
 *
 * Allows the user to rotate PCB objects by clicking on them.
 * The rotation direction depends on whether Shift is held.
 *
 * This is a simple single-click mode - no state machine needed.
 *
 * Extracted from action.c ROTATE_MODE case in NotifyMode().
 */

#include "EditorMode.h"
#include "ModesCommon.h"

extern "C" {
#include "rotate.h"
}

namespace pcb {
namespace modes {

/*!
 * \brief Rotate object mode
 *
 * Rotates PCB objects at the click location.
 *
 * Features:
 * - Click on object to rotate it
 * - Normal click: rotate 90° counter-clockwise (or clockwise with SWAP_IDENT)
 * - Shift+click: rotate 90° clockwise (or counter-clockwise with SWAP_IDENT)
 */
class RotateMode : public EditorMode {
public:
    explicit RotateMode(ActionContext* context)
        : EditorMode(context)
    {}

    void onNotify(Coord x, Coord y) override {
        // Determine rotation direction based on shift key and SWAP_IDENT
        // SWAP_IDENT swaps left/right mouse button meanings
        int steps;

        if (gui->shift_is_pressed()) {
            // Shift held - rotate opposite direction
            steps = SWAP_IDENT ? 1 : 3;
        } else {
            // Normal click
            steps = SWAP_IDENT ? 3 : 1;
        }

        // Rotate object at click location
        // Steps: 1 = 90° CCW, 3 = 90° CW (270° CCW)
        RotateScreenObject(context_->Note.X, context_->Note.Y, steps);
    }

    void onRelease() override {
        // Rotate mode doesn't use release
    }

    const char* getName() const override { return "Rotate"; }
    int getModeId() const override { return ROTATE_MODE; }
};

// Factory function for ModeManager registration
std::unique_ptr<EditorMode> createRotateMode(ActionContext* context) {
    return std::unique_ptr<EditorMode>(new RotateMode(context));
}

} // namespace modes
} // namespace pcb
