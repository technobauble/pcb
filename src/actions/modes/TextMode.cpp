/*!
 * \file src/actions/modes/TextMode.cpp
 *
 * \brief Text placement mode implementation
 *
 * Allows the user to place text on the PCB by clicking a location
 * and entering text in a dialog.
 *
 * This is a simple single-click mode - no state machine needed.
 *
 * Extracted from action.c TEXT_MODE case in NotifyMode().
 */

#include "EditorMode.h"
#include "ModesCommon.h"

extern "C" {
#include "create.h"
#include "draw.h"
}

namespace pcb {
namespace modes {

/*!
 * \brief Text placement mode
 *
 * Places text on the current layer at the click location.
 * Opens a dialog to prompt for text content.
 *
 * Features:
 * - Click to set text location
 * - Dialog prompts for text string
 * - Automatically sets ONSOLDERFLAG for bottom layer text
 * - Text uses current Settings.TextScale
 */
class TextMode : public EditorMode {
public:
    explicit TextMode(ActionContext* context)
        : EditorMode(context)
    {}

    void onNotify(Coord x, Coord y) override {
        // Prompt user for text
        char* string = gui->prompt_for(_("Enter text:"), "");

        if (string == NULL) {
            return;
        }

        if (strlen(string) > 0) {
            // Determine text flags
            int flag = CLEARLINEFLAG;

            // Set ONSOLDERFLAG if on bottom layer
            if (GetLayerGroupNumberByNumber(INDEXOFCURRENT) ==
                GetLayerGroupNumberBySide(BOTTOM_SIDE)) {
                flag |= ONSOLDERFLAG;
            }

            // Create the text
            TextType* text = CreateNewText(
                CURRENT,
                &PCB->Font,
                context_->Note.X,
                context_->Note.Y,
                0,  // rotation angle
                Settings.TextScale,
                string,
                MakeFlags(flag));

            if (text != NULL) {
                AddObjectToCreateUndoList(TEXT_TYPE, CURRENT, text, text);
                IncrementUndoSerialNumber();
                DrawText(CURRENT, text);
                Draw();
            }
        }

        free(string);
    }

    void onRelease() override {
        // Text mode doesn't use release
    }

    const char* getName() const override { return "Text"; }
    int getModeId() const override { return TEXT_MODE; }
};

// Factory function for ModeManager registration
std::unique_ptr<EditorMode> createTextMode(ActionContext* context) {
    return std::unique_ptr<EditorMode>(new TextMode(context));
}

} // namespace modes
} // namespace pcb
