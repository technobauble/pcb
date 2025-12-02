/*!
 * \file src/actions/modes/PasteBufferMode.cpp
 *
 * \brief Paste buffer mode implementation
 *
 * Allows the user to paste the contents of the paste buffer at the click
 * location. Shift+click replaces an existing element with the buffer contents,
 * preserving the element's name.
 *
 * This is a single-click mode - no state machine needed.
 *
 * Extracted from action.c PASTEBUFFER_MODE case in NotifyMode().
 */

#include "EditorMode.h"
#include "ModesCommon.h"

extern "C" {
#include "buffer.h"
#include "copy.h"
#include "draw.h"
#include "remove.h"
#include "rtree.h"
}

namespace pcb {
namespace modes {

/*!
 * \brief Paste buffer mode
 *
 * Pastes the contents of the current paste buffer at the click location.
 *
 * Features:
 * - Click to paste buffer contents
 * - Shift+click on element: replace element with buffer, keeping element name
 * - Works with elements, vias, lines, etc.
 */
class PasteBufferMode : public EditorMode {
public:
    explicit PasteBufferMode(ActionContext* context)
        : EditorMode(context)
    {}

    void onNotify(Coord x, Coord y) override {
        void *ptr1, *ptr2, *ptr3;
        TextType savedNames[MAX_ELEMENTNAMES];
        ElementType* elementToReplace = NULL;

        // Shift+click: Replace existing element, keeping its name
        if (gui->shift_is_pressed()) {
            int type = SearchScreen(
                context_->Note.X, context_->Note.Y,
                ELEMENT_TYPE,
                &ptr1, &ptr2, &ptr3);

            if (type == ELEMENT_TYPE) {
                elementToReplace = (ElementType*)ptr1;

                if (elementToReplace) {
                    // Save the element names before removing
                    for (int i = 0; i < MAX_ELEMENTNAMES; ++i) {
                        savedNames[i] = elementToReplace->Name[i];
                        // Duplicate the strings since RemoveElement will free them
                        if (savedNames[i].TextString) {
                            savedNames[i].TextString = strdup(savedNames[i].TextString);
                        }
                    }

                    // Remove the old element
                    RemoveElement(elementToReplace);
                }
            }
        }

        // Paste the buffer contents
        if (CopyPastebufferToLayout(context_->Note.X, context_->Note.Y)) {
            SetChangedFlag(true);
        }

        // If we replaced an element, restore its names to the new element
        if (elementToReplace) {
            int type = SearchScreen(
                context_->Note.X, context_->Note.Y,
                ELEMENT_TYPE,
                &ptr1, &ptr2, &ptr3);

            if (type == ELEMENT_TYPE && ptr1) {
                ElementType* newElement = (ElementType*)ptr1;
                int currentNameIndex = NAME_INDEX(PCB);

                for (int i = 0; i < MAX_ELEMENTNAMES; ++i) {
                    // Erase old name display if this is the current name index
                    if (i == currentNameIndex) {
                        EraseElementName(newElement);
                    }

                    // Remove from R-tree
                    r_delete_entry(
                        PCB->Data->name_tree[i],
                        (BoxType*)&(newElement->Name[i]));

                    // Copy saved name data
                    memcpy(&(newElement->Name[i]), &(savedNames[i]), sizeof(TextType));
                    newElement->Name[i].Element = newElement;

                    // Update bounding box and re-add to R-tree
                    SetTextBoundingBox(&PCB->Font, &(newElement->Name[i]));
                    r_insert_entry(
                        PCB->Data->name_tree[i],
                        (BoxType*)&(newElement->Name[i]), 0);

                    // Redraw if this is the current name index
                    if (i == currentNameIndex) {
                        DrawElementName(newElement);
                    }
                }
            } else {
                // Paste didn't create an element - free the saved strings
                for (int i = 0; i < MAX_ELEMENTNAMES; ++i) {
                    if (savedNames[i].TextString) {
                        free(savedNames[i].TextString);
                    }
                }
            }
        }
    }

    void onRelease() override {
        // Paste buffer mode doesn't use release
    }

    const char* getName() const override { return "PasteBuffer"; }
    int getModeId() const override { return PASTEBUFFER_MODE; }
};

// Factory function for ModeManager registration
std::unique_ptr<EditorMode> createPasteBufferMode(ActionContext* context) {
    return std::unique_ptr<EditorMode>(new PasteBufferMode(context));
}

} // namespace modes
} // namespace pcb
