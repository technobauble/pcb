/*!
 * \file src/actions/modes/ThermalMode.cpp
 *
 * \brief Thermal relief mode implementation
 *
 * Allows the user to toggle thermal relief patterns on pins and vias.
 * Thermal relief creates small air gaps around pins to make soldering easier
 * while still maintaining electrical connection through the remaining copper.
 *
 * Extracted from action.c lines 1048-1070 (THERMAL_MODE case in NotifyMode).
 */

#include "EditorMode.h"
#include "ModesCommon.h"

extern "C" {
#include "thermal.h"
#include "macro.h"
}

namespace pcb {
namespace modes {

/*!
 * \brief Thermal relief toggle mode
 *
 * This mode allows users to click on pins/vias to toggle thermal relief patterns.
 * Thermal relief creates air gaps around pins for easier soldering while maintaining
 * electrical connection.
 *
 * Behavior:
 * - Click: Toggle thermal relief on/off using board's default style
 * - Shift+Click: Cycle through thermal relief styles (1-5)
 * - Only works on pins/vias, not on holes
 *
 * Thermal styles:
 * - 0: No thermal (solid connection)
 * - 1-5: Different thermal relief patterns
 */
class ThermalMode : public EditorMode {
public:
    /*!
     * \brief Constructor
     * \param context Shared action context
     */
    explicit ThermalMode(ActionContext* context)
        : EditorMode(context)
    {}

    /*!
     * \brief Handle mouse click - toggle thermal relief on pin/via
     * \param x X coordinate of click
     * \param y Y coordinate of click
     *
     * Searches for a pin or via at the click position and toggles or cycles
     * its thermal relief pattern.
     */
    void onNotify(Coord x, Coord y) override {
        void *ptr1, *ptr2, *ptr3;

        // Search for pin or via at cursor
        // PIN_TYPES includes pins, pads, and vias
        int type = SearchScreen(x, y, PIN_TYPES, &ptr1, &ptr2, &ptr3);

        // Only process if we found something and it's not a hole
        if (type != NO_TYPE && !TEST_FLAG(HOLEFLAG, (PinType*)ptr3)) {
            if (gui->shift_is_pressed()) {
                // Shift+Click: Cycle through thermal styles 1-5
                int tstyle = GET_THERM(INDEXOFCURRENT, (PinType*)ptr3);
                tstyle++;
                if (tstyle > 5) {
                    tstyle = 1;  // Wrap back to style 1
                }
                ChangeObjectThermal(type, ptr1, ptr2, ptr3, tstyle);
            }
            else if (GET_THERM(INDEXOFCURRENT, (PinType*)ptr3)) {
                // Click on object with thermal: Turn it off (set to 0)
                ChangeObjectThermal(type, ptr1, ptr2, ptr3, 0);
            }
            else {
                // Click on object without thermal: Turn it on using board's default style
                ChangeObjectThermal(type, ptr1, ptr2, ptr3, PCB->ThermStyle);
            }
        }
    }

    /*!
     * \brief Handle mouse release - no action for ThermalMode
     */
    void onRelease() override {
        // ThermalMode doesn't use release events
        // Thermal toggle happens on click
    }

    /*!
     * \brief Get mode name
     * \return "Thermal"
     */
    const char* getName() const override {
        return "Thermal";
    }

    /*!
     * \brief Get mode ID
     * \return THERMAL_MODE constant
     */
    int getModeId() const override {
        return THERMAL_MODE;
    }
};

/*!
 * \brief Factory function to create ThermalMode instance
 * \param context Shared action context
 * \return Unique pointer to ThermalMode instance
 */
std::unique_ptr<EditorMode> createThermalMode(ActionContext* context) {
    return std::unique_ptr<EditorMode>(new ThermalMode(context));
}

} // namespace modes
} // namespace pcb
