/*!
 * \file src/actions/modes/ViaMode.cpp
 *
 * \brief Via placement mode implementation
 *
 * Allows the user to click to place vias (vertical interconnects between layers).
 * Optionally applies thermal relief if shift key is held during placement.
 *
 * Extracted from action.c lines 890-913 (VIA_MODE case in NotifyMode).
 */

#include "EditorMode.h"

extern "C" {
#include "global.h"
#include "data.h"
#include "create.h"
#include "change.h"
#include "draw.h"
#include "thermal.h"
#include "undo.h"
#include "misc.h"
#include "hid.h"
#include "error.h"
}

namespace pcb {
namespace modes {

/*!
 * \brief Via placement mode
 *
 * This mode allows users to place vias (vertical interconnects) by clicking
 * on the board. Vias connect traces on different layers.
 *
 * Features:
 * - Checks if vias are visible before placing
 * - Creates via at click position
 * - Optionally applies thermal relief if shift key held
 * - Uses settings for via parameters (thickness, drilling hole, etc.)
 *
 * Behavior:
 * - Click: Place via at cursor position
 * - Shift+Click: Place via with thermal relief
 * - Release: No action
 */
class ViaMode : public EditorMode {
public:
    /*!
     * \brief Constructor
     * \param context Shared action context
     */
    explicit ViaMode(ActionContext* context)
        : EditorMode(context)
    {}

    /*!
     * \brief Handle mouse click - place via at cursor
     * \param x X coordinate of click
     * \param y Y coordinate of click
     *
     * Creates a via at the specified position using current settings.
     * If shift key is held, applies thermal relief to the via.
     */
    void onNotify(Coord x, Coord y) override {
        // Check if vias are visible
        if (!PCB->ViaOn) {
            Message(_("You must turn via visibility on before\n"
                     "you can place vias\n"));
            return;
        }

        // Create via at cursor position
        PinType* via = CreateNewVia(
            PCB->Data,
            x, y,
            Settings.ViaThickness,
            2 * Settings.Keepaway,
            Settings.ViaMaskAperture,
            Settings.ViaDrillingHole,
            NULL,
            NoFlags()
        );

        if (via) {
            // Add to undo list
            AddObjectToCreateUndoList(VIA_TYPE, via, via, via);

            // Apply thermal relief if shift key held
            if (gui->shift_is_pressed()) {
                ChangeObjectThermal(VIA_TYPE, via, via, via, PCB->ThermStyle);
            }

            IncrementUndoSerialNumber();
            DrawVia(via);
            Draw();
        }
    }

    /*!
     * \brief Handle mouse release - no action for ViaMode
     */
    void onRelease() override {
        // ViaMode doesn't use release events
        // Via is placed on click, not on release
    }

    /*!
     * \brief Get mode name
     * \return "Via"
     */
    const char* getName() const override {
        return "Via";
    }

    /*!
     * \brief Get mode ID
     * \return VIA_MODE constant
     */
    int getModeId() const override {
        return VIA_MODE;
    }
};

/*!
 * \brief Factory function to create ViaMode instance
 * \param context Shared action context
 * \return Unique pointer to ViaMode instance
 */
std::unique_ptr<EditorMode> createViaMode(ActionContext* context) {
    return std::unique_ptr<EditorMode>(new ViaMode(context));
}

} // namespace modes
} // namespace pcb
