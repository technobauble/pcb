/*!
 * \file src/actions/modes/ArrowMode.cpp
 *
 * \brief Arrow (selection) mode implementation
 *
 * The primary selection and manipulation mode. Handles:
 * - Single click to select/deselect objects
 * - Drag to move objects (delegates to MoveMode/CopyMode)
 * - Drag selected objects (delegates to PasteBufferMode)
 * - Box selection for multiple objects
 *
 * Uses modern distance-based drag detection instead of timer-based.
 * This provides more responsive feedback and cleaner code architecture.
 *
 * Extracted from action.c ARROW_MODE case in NotifyMode(), click_cb(),
 * and ReleaseMode().
 */

#include "EditorMode.h"
#include "ModesCommon.h"

extern "C" {
#include "action.h"
#include "buffer.h"
#include "crosshair.h"
#include "draw.h"
#include "error.h"
#include "misc.h"
#include "move.h"
#include "remove.h"
#include "select.h"
#include "set.h"
#include "undo.h"

// Forward declarations for mode management (defined in ModeManager.cpp)
void SaveMode(void);
void RestoreMode(void);
}

namespace pcb {
namespace modes {

/*!
 * \brief Arrow (selection) mode
 *
 * The default mode for selecting and manipulating objects.
 *
 * Features:
 * - Click to select/deselect objects
 * - Shift+click to add to selection
 * - Drag object to move it (Ctrl+drag to copy)
 * - Drag selected objects to move selection
 * - Drag on empty area for box selection
 *
 * Uses distance-based drag detection: if the mouse moves more than
 * DRAG_THRESHOLD from the initial click point, it's a drag operation.
 * Otherwise, it's a click.
 */
class ArrowMode : public EditorMode {
public:
    explicit ArrowMode(ActionContext* context)
        : EditorMode(context)
    {}

    void onNotify(Coord x, Coord y) override {
        // Record start position for drag detection
        startX_ = x;
        startY_ = y;
        isDragging_ = false;
        dragStarted_ = false;

        // Search for objects under cursor
        hitType_ = NO_TYPE;
        hasSelectedUnderCursor_ = false;

        // Search for both movable objects and selected objects
        int test = (SELECT_TYPES | MOVE_TYPES) & ~RATLINE_TYPE;
        int type;
        void *ptr1, *ptr2, *ptr3;

        while (test) {
            type = SearchScreen(x, y, test, &ptr1, &ptr2, &ptr3);

            // Found a movable object that's not locked
            if (!hitType_ && (type & MOVE_TYPES) &&
                !TEST_FLAG(LOCKFLAG, (PinType*)ptr2)) {
                hitType_ = type;
                hitPtr1_ = ptr1;
                hitPtr2_ = ptr2;
                hitPtr3_ = ptr3;
            }

            // Found something that's already selected
            if (!hasSelectedUnderCursor_ && (type & SELECT_TYPES) &&
                TEST_FLAG(SELECTEDFLAG, (PinType*)ptr2)) {
                hasSelectedUnderCursor_ = true;
            }

            // Stop if we found both, or no more matches
            if ((hitType_ && hasSelectedUnderCursor_) || type == NO_TYPE) {
                break;
            }

            test &= ~type;
        }
    }

    void onMotion(Coord x, Coord y) override {
        if (dragStarted_) {
            // Drag already delegated to another mode
            return;
        }

        if (!isDragging_) {
            // Check if we've moved beyond drag threshold
            Coord dx = ABS(x - startX_);
            Coord dy = ABS(y - startY_);
            Coord threshold = getDragThreshold();

            if (dx > threshold || dy > threshold) {
                isDragging_ = true;
                startDragOperation();
            }
        }
    }

    void onRelease() override {
        if (dragStarted_) {
            // Drag was delegated to another mode, which handles its own release
            // We just need to handle restoration if needed
            if (!isDragging_) {
                // Released without actually dragging - shouldn't happen
                // but clean up just in case
                hitType_ = NO_TYPE;
                hasSelectedUnderCursor_ = false;
            }
            return;
        }

        if (!isDragging_) {
            // Click without drag - handle selection
            handleClick();
        } else {
            // Completed box selection
            completeBoxSelection();
        }

        // Reset state
        hitType_ = NO_TYPE;
        hasSelectedUnderCursor_ = false;
        isDragging_ = false;
    }

    void onExit() override {
        // Reset box selection state
        Crosshair.AttachedBox.State = STATE_FIRST;
        hitType_ = NO_TYPE;
        hasSelectedUnderCursor_ = false;
        isDragging_ = false;
        dragStarted_ = false;
    }

    const char* getName() const override { return "Arrow"; }
    int getModeId() const override { return ARROW_MODE; }

private:
    // Drag threshold in screen pixels
    // This is converted to PCB coordinates based on current zoom level
    static const int DRAG_THRESHOLD_PIXELS = 5;

    // Fallback drag threshold in PCB coordinates if zoom info not available
    static const Coord DRAG_THRESHOLD_FALLBACK = MIL_TO_COORD(10);

    /*!
     * \brief Get the drag threshold in PCB coordinates
     *
     * Returns a threshold based on screen pixels (zoom-independent).
     * If zoom info is not available, falls back to a fixed PCB coordinate value.
     */
    Coord getDragThreshold() const {
        if (context_->coord_per_px > 0) {
            // Convert pixel threshold to PCB coordinates based on current zoom
            return static_cast<Coord>(DRAG_THRESHOLD_PIXELS * context_->coord_per_px);
        }
        return DRAG_THRESHOLD_FALLBACK;
    }

    // Start position for drag detection
    Coord startX_ = 0;
    Coord startY_ = 0;

    // Object under cursor at click time
    int hitType_ = NO_TYPE;
    void* hitPtr1_ = nullptr;
    void* hitPtr2_ = nullptr;
    void* hitPtr3_ = nullptr;
    bool hasSelectedUnderCursor_ = false;

    // Drag state
    bool isDragging_ = false;
    bool dragStarted_ = false;  // True if we switched to another mode

    /*!
     * \brief Start a drag operation
     *
     * Determines what kind of drag to perform based on what's under
     * the cursor and switches to the appropriate mode.
     */
    void startDragOperation() {
        notify_crosshair_change(false);

        if (hasSelectedUnderCursor_ && !gui->shift_is_pressed()) {
            // Dragging selected objects - use paste buffer to move them
            startMoveSelected();
            dragStarted_ = true;
        } else if (hitType_ && !gui->shift_is_pressed()) {
            // Dragging single object
            startMoveSingle();
            dragStarted_ = true;
        } else {
            // Box selection
            startBoxSelection();
            // Box selection stays in ArrowMode
        }

        notify_crosshair_change(true);
    }

    /*!
     * \brief Start moving selected objects
     *
     * Copies selected objects to paste buffer, removes originals,
     * and switches to PasteBuffer mode.
     */
    void startMoveSelected() {
        // Save current buffer number
        context_->Note.Buffer = Settings.BufferNumber;

        // Use last buffer for move operation
        SetBufferNumber(MAX_BUFFER - 1);
        ClearBuffer(PASTEBUFFER);

        // Add selected objects to buffer and remove originals
        AddSelectedToBuffer(PASTEBUFFER, startX_, startY_, true);
        SaveUndoSerialNumber();
        RemoveSelected();

        // Switch to paste buffer mode
        SaveMode();
        context_->saved_mode = true;
        SetMode(PASTEBUFFER_MODE);
    }

    /*!
     * \brief Start moving/copying a single object
     *
     * Sets up the attached object and switches to Move or Copy mode.
     */
    void startMoveSingle() {
        // IMPORTANT: Save these values BEFORE calling SetMode(), because
        // SetMode() triggers onExit() which resets the member variables
        int savedType = hitType_;
        void* savedPtr1 = hitPtr1_;
        void* savedPtr2 = hitPtr2_;
        void* savedPtr3 = hitPtr3_;
        Coord savedStartX = startX_;
        Coord savedStartY = startY_;

        SaveMode();
        context_->saved_mode = true;

        // Ctrl+drag = copy, regular drag = move
        int newMode = gui->control_is_pressed() ? COPY_MODE : MOVE_MODE;
        SetMode(newMode);

        // Set up the attached object for move/copy
        // Note: SetMode resets state to STATE_FIRST, but we need STATE_SECOND
        // so that the release triggers the move, not another selection
        Crosshair.AttachedObject.Ptr1 = savedPtr1;
        Crosshair.AttachedObject.Ptr2 = savedPtr2;
        Crosshair.AttachedObject.Ptr3 = savedPtr3;
        Crosshair.AttachedObject.Type = savedType;
        Crosshair.AttachedObject.State = STATE_SECOND;

        AttachForCopy(savedStartX, savedStartY);
    }

    /*!
     * \brief Start box selection
     *
     * Initializes the selection box at the click position.
     */
    void startBoxSelection() {
        // Clear current selection if shift not held
        BoxType box;
        box.X1 = -MAX_COORD;
        box.Y1 = -MAX_COORD;
        box.X2 = MAX_COORD;
        box.Y2 = MAX_COORD;

        SaveUndoSerialNumber();
        if (!gui->shift_is_pressed() && SelectBlock(&box, false)) {
            SetChangedFlag(true);
        }

        // Start the selection box
        NotifyBlock();
        Crosshair.AttachedBox.Point1.X = startX_;
        Crosshair.AttachedBox.Point1.Y = startY_;
    }

    /*!
     * \brief Complete box selection
     *
     * Selects all objects within the drawn box.
     */
    void completeBoxSelection() {
        BoxType box;

        box.X1 = MIN(Crosshair.AttachedBox.Point1.X,
                     Crosshair.AttachedBox.Point2.X);
        box.Y1 = MIN(Crosshair.AttachedBox.Point1.Y,
                     Crosshair.AttachedBox.Point2.Y);
        box.X2 = MAX(Crosshair.AttachedBox.Point1.X,
                     Crosshair.AttachedBox.Point2.X);
        box.Y2 = MAX(Crosshair.AttachedBox.Point1.Y,
                     Crosshair.AttachedBox.Point2.Y);

        RestoreUndoSerialNumber();
        if (SelectBlock(&box, true)) {
            SetChangedFlag(true);
        } else if (Bumped) {
            IncrementUndoSerialNumber();
        }

        Crosshair.AttachedBox.State = STATE_FIRST;
    }

    /*!
     * \brief Handle a single click (no drag)
     *
     * Toggles selection of the object under cursor.
     */
    void handleClick() {
        BoxType box;
        box.X1 = -MAX_COORD;
        box.Y1 = -MAX_COORD;
        box.X2 = MAX_COORD;
        box.Y2 = MAX_COORD;

        SaveUndoSerialNumber();

        // Deselect all first if shift not held
        if (!gui->shift_is_pressed()) {
            if (SelectBlock(&box, false)) {
                SetChangedFlag(true);
            }

            // If we were on a selected item that just got deselected,
            // we're done
            if (hasSelectedUnderCursor_) {
                hitType_ = NO_TYPE;
                hasSelectedUnderCursor_ = false;
                return;
            }
        }

        // Try to select object under cursor
        RestoreUndoSerialNumber();
        if (SelectObject()) {
            SetChangedFlag(true);
        } else {
            // Nothing selected, so deselection gets its own serial number
            IncrementUndoSerialNumber();
        }
    }
};

// Factory function for ModeManager registration
std::unique_ptr<EditorMode> createArrowMode(ActionContext* context) {
    return std::unique_ptr<EditorMode>(new ArrowMode(context));
}

} // namespace modes
} // namespace pcb
