/*!
 * \file src/actions/modes/RemoveMode.cpp
 *
 * \brief Remove (delete) mode implementation
 *
 * This is the simplest mode - searches for an object at the cursor position
 * and removes it if found. This serves as our "Hello World" to prove the
 * State Pattern works.
 *
 * Extracted from action.c lines 1590-1602 (REMOVE_MODE case in NotifyMode).
 */

#include "EditorMode.h"

extern "C" {
#include "global.h"
#include "search.h"
#include "remove.h"
#include "undo.h"
#include "set.h"
}

namespace pcb {
namespace modes {

/*!
 * \brief Remove (delete) mode - delete object at cursor
 *
 * This mode allows the user to click on objects to delete them.
 * It's the simplest mode in the editor - just search and remove.
 *
 * Behavior:
 * - Click: Search for object at cursor, remove if found
 * - Release: No action
 * - No multi-state behavior
 * - No attached objects or preview
 */
class RemoveMode : public EditorMode {
public:
    /*!
     * \brief Constructor
     * \param context Shared action context
     */
    explicit RemoveMode(ActionContext* context)
        : EditorMode(context)
    {}

    /*!
     * \brief Handle mouse click - remove object at cursor
     * \param x X coordinate of click
     * \param y Y coordinate of click
     *
     * Searches for removable objects at the click position and removes
     * the first one found.
     */
    void onNotify(Coord x, Coord y) override {
        void *ptr1, *ptr2, *ptr3;

        // Search for removable object at cursor
        // REMOVE_TYPES includes all objects that can be deleted
        int type = SearchScreen(x, y, REMOVE_TYPES, &ptr1, &ptr2, &ptr3);

        if (type != NO_TYPE) {
            // Remove the object
            if (RemoveObject(type, ptr1, ptr2, ptr3)) {
                // Successfully removed - record in undo and mark changed
                IncrementUndoSerialNumber();
                SetChangedFlag(true);
            }
        }
    }

    /*!
     * \brief Handle mouse release - no action for RemoveMode
     */
    void onRelease() override {
        // RemoveMode doesn't use release events
        // Deletion happens on click, not release
    }

    /*!
     * \brief Get mode name
     * \return "Remove"
     */
    const char* getName() const override {
        return "Remove";
    }

    /*!
     * \brief Get mode ID
     * \return REMOVE_MODE constant
     */
    int getModeId() const override {
        return REMOVE_MODE;
    }
};

/*!
 * \brief Factory function to create RemoveMode instance
 * \param context Shared action context
 * \return Unique pointer to RemoveMode instance
 */
std::unique_ptr<EditorMode> createRemoveMode(ActionContext* context) {
    return std::unique_ptr<EditorMode>(new RemoveMode(context));
}

} // namespace modes
} // namespace pcb
