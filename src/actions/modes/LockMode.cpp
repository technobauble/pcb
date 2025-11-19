/*!
 * \file src/actions/modes/LockMode.cpp
 *
 * \brief Lock/unlock mode implementation
 *
 * Allows the user to click on objects to toggle their lock state.
 * Locked objects cannot be moved, deleted, or selected, preventing
 * accidental modifications.
 *
 * Extracted from action.c lines 1002-1047 (LOCK_MODE case in NotifyMode).
 */

#include "EditorMode.h"

extern "C" {
#include "global.h"
#include "search.h"
#include "draw.h"
#include "set.h"
#include "hid.h"
#include "hid/common/actions.h"
#include "macro.h"
}

namespace pcb {
namespace modes {

/*!
 * \brief Lock/unlock mode
 *
 * This mode allows users to click on objects to toggle their lock flag.
 * Locked objects are protected from modification.
 *
 * Special handling for elements:
 * - Locking an element also locks all its pins and pads
 * - Locked objects are automatically deselected
 *
 * Behavior:
 * - Click on element: Toggle lock for element and all its pins/pads
 * - Click on other object: Toggle lock for that object
 * - Locked objects are deselected
 * - Reports object info after locking
 */
class LockMode : public EditorMode {
public:
    /*!
     * \brief Constructor
     * \param context Shared action context
     */
    explicit LockMode(ActionContext* context)
        : EditorMode(context)
    {}

    /*!
     * \brief Handle mouse click - toggle lock on object
     * \param x X coordinate of click
     * \param y Y coordinate of click
     *
     * Searches for a lockable object at the click position and toggles
     * its lock flag. Elements get special handling.
     */
    void onNotify(Coord x, Coord y) override {
        void *ptr1, *ptr2, *ptr3;

        // Search for lockable object at cursor
        // LOCK_TYPES includes elements, lines, arcs, text, etc.
        int type = SearchScreen(x, y, LOCK_TYPES, &ptr1, &ptr2, &ptr3);

        if (type == ELEMENT_TYPE) {
            // Special handling for elements
            ElementType* element = (ElementType*)ptr2;

            // Toggle lock flag on element
            TOGGLE_FLAG(LOCKFLAG, element);

            // Also toggle lock on all pins
            PIN_LOOP(element);
            {
                TOGGLE_FLAG(LOCKFLAG, pin);
                CLEAR_FLAG(SELECTEDFLAG, pin);  // Locked items can't be selected
            }
            END_LOOP;

            // And toggle lock on all pads
            PAD_LOOP(element);
            {
                TOGGLE_FLAG(LOCKFLAG, pad);
                CLEAR_FLAG(SELECTEDFLAG, pad);
            }
            END_LOOP;

            // Clear selection from element itself
            CLEAR_FLAG(SELECTEDFLAG, element);

            // Redraw the element
            // (Always redraw since selection flags may have changed)
            DrawElement(element);
            Draw();

            SetChangedFlag(true);
            hid_actionl("Report", "Object", NULL);
        }
        else if (type != NO_TYPE) {
            // Other object types (text, lines, arcs, etc.)
            TextType* thing = (TextType*)ptr3;

            // Toggle lock flag
            TOGGLE_FLAG(LOCKFLAG, thing);

            // If now locked and was selected, deselect it
            // (Locked items can't be selected)
            if (TEST_FLAG(LOCKFLAG, thing) && TEST_FLAG(SELECTEDFLAG, thing)) {
                // Note: This is not undoable since LOCK flag changes aren't recorded in undo
                CLEAR_FLAG(SELECTEDFLAG, thing);
                DrawObject(type, ptr1, ptr2);
                Draw();
            }

            SetChangedFlag(true);
            hid_actionl("Report", "Object", NULL);
        }
    }

    /*!
     * \brief Handle mouse release - no action for LockMode
     */
    void onRelease() override {
        // LockMode doesn't use release events
        // Lock toggle happens on click
    }

    /*!
     * \brief Get mode name
     * \return "Lock"
     */
    const char* getName() const override {
        return "Lock";
    }

    /*!
     * \brief Get mode ID
     * \return LOCK_MODE constant
     */
    int getModeId() const override {
        return LOCK_MODE;
    }
};

/*!
 * \brief Factory function to create LockMode instance
 * \param context Shared action context
 * \return Unique pointer to LockMode instance
 */
std::unique_ptr<EditorMode> createLockMode(ActionContext* context) {
    return std::make_unique<LockMode>(context);
}

} // namespace modes
} // namespace pcb
