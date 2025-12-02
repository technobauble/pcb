/*!
 * \file src/actions/modes/ModeManager.cpp
 *
 * \brief ModeManager implementation for editor mode management
 *
 * Implements the State Pattern context class that manages mode transitions
 * and event dispatch.
 */

#include "EditorMode.h"

extern "C" {
#include "global.h"
#include "data.h"
#include "action.h"
#include "set.h"
#include "crosshair.h"
#include "misc.h"
#include "error.h"
}

// Forward declarations of mode classes
// We'll implement these incrementally
namespace pcb {
namespace modes {

// P1 modes (simple - Days 1-3)
class RemoveMode;
class ViaMode;
class ThermalMode;
class LockMode;

// P2 modes (medium - Days 4-10)
class TextMode;
class RectangleMode;
class PolygonHoleMode;
class CopyMode;
class MoveMode;
class PasteBufferMode;
class InsertPointMode;
class RotateMode;

// P3 modes (complex - Days 11-18)
class PolygonMode;
class ArcMode;
class LineMode;

// P4 modes (most complex - Days 19-21)
class ArrowMode;

// Factory functions (defined in respective mode .cpp files)
// P1 modes (simple)
extern std::unique_ptr<EditorMode> createRemoveMode(ActionContext* context);
extern std::unique_ptr<EditorMode> createViaMode(ActionContext* context);
extern std::unique_ptr<EditorMode> createThermalMode(ActionContext* context);
extern std::unique_ptr<EditorMode> createLockMode(ActionContext* context);
// P2 modes (medium complexity)
extern std::unique_ptr<EditorMode> createRectangleMode(ActionContext* context);
extern std::unique_ptr<EditorMode> createTextMode(ActionContext* context);
extern std::unique_ptr<EditorMode> createCopyMode(ActionContext* context);
extern std::unique_ptr<EditorMode> createMoveMode(ActionContext* context);
extern std::unique_ptr<EditorMode> createRotateMode(ActionContext* context);
extern std::unique_ptr<EditorMode> createPasteBufferMode(ActionContext* context);
extern std::unique_ptr<EditorMode> createInsertPointMode(ActionContext* context);
// P3 modes (complex)
extern std::unique_ptr<EditorMode> createLineMode(ActionContext* context);
extern std::unique_ptr<EditorMode> createArcMode(ActionContext* context);
extern std::unique_ptr<EditorMode> createPolygonMode(ActionContext* context);
extern std::unique_ptr<EditorMode> createPolygonHoleMode(ActionContext* context);
// P4 modes (most complex):
extern std::unique_ptr<EditorMode> createArrowMode(ActionContext* context);

} // namespace modes
} // namespace pcb

namespace pcb {
namespace modes {

ModeManager::ModeManager(ActionContext* context)
    : context_(context)
    , current_mode_(nullptr)
    , saved_mode_id_(NO_MODE)
{
    initializeModes();
}

ModeManager::~ModeManager() = default;

void ModeManager::registerMode(int id, std::unique_ptr<EditorMode> mode) {
    modes_[id] = std::move(mode);
}

void ModeManager::initializeModes() {
    // Register modes as we implement them

    // Phase 1: Simple modes (Days 1-3) ✅ COMPLETE
    registerMode(REMOVE_MODE, createRemoveMode(context_));
    registerMode(VIA_MODE, createViaMode(context_));
    registerMode(THERMAL_MODE, createThermalMode(context_));
    registerMode(LOCK_MODE, createLockMode(context_));

    // Phase 2: Medium complexity modes (Days 4-10)
    registerMode(RECTANGLE_MODE, createRectangleMode(context_));
    registerMode(TEXT_MODE, createTextMode(context_));
    registerMode(COPY_MODE, createCopyMode(context_));
    registerMode(MOVE_MODE, createMoveMode(context_));
    registerMode(ROTATE_MODE, createRotateMode(context_));
    registerMode(PASTEBUFFER_MODE, createPasteBufferMode(context_));
    registerMode(INSERTPOINT_MODE, createInsertPointMode(context_));

    // Phase 3: Complex modes (Days 11-18)
    registerMode(LINE_MODE, createLineMode(context_));
    registerMode(ARC_MODE, createArcMode(context_));
    registerMode(POLYGON_MODE, createPolygonMode(context_));
    registerMode(POLYGONHOLE_MODE, createPolygonHoleMode(context_));

    // Phase 4: ArrowMode - uses modern distance-based drag detection
    registerMode(ARROW_MODE, createArrowMode(context_));

    // Note: Until a mode is registered, setMode() for that mode will do nothing
    // and the old NotifyMode() implementation will continue to handle it
}

void ModeManager::setMode(int mode_id) {
    // Exit current mode
    if (current_mode_) {
        current_mode_->onExit();
    }

    // Find new mode
    auto it = modes_.find(mode_id);
    if (it != modes_.end()) {
        current_mode_ = it->second.get();
        current_mode_->onEnter();
    } else {
        // Mode not implemented yet
        // This is expected during incremental development
        current_mode_ = nullptr;
    }

    // Update global Settings.Mode for backward compatibility
    // This ensures existing C code that reads Settings.Mode still works
    Settings.Mode = mode_id;
}

void ModeManager::saveMode() {
    saved_mode_id_ = Settings.Mode;
}

void ModeManager::restoreMode() {
    if (saved_mode_id_ != NO_MODE) {
        setMode(saved_mode_id_);
        saved_mode_id_ = NO_MODE;  // Clear saved mode after restore
    }
}

void ModeManager::notifyClick(Coord x, Coord y) {
    if (current_mode_) {
        // Update context with click position
        context_->Note.X = x;
        context_->Note.Y = y;

        // Clear rat warnings if enabled (from original NotifyMode)
        if (Settings.RatWarn) {
            ClearWarnings();
        }

        // Dispatch to current mode
        current_mode_->onNotify(x, y);
    }
}

void ModeManager::notifyRelease() {
    if (current_mode_) {
        current_mode_->onRelease();
    }
}

void ModeManager::notifyMotion(Coord x, Coord y) {
    if (current_mode_) {
        current_mode_->onMotion(x, y);
    }
}

void ModeManager::notifyCancel() {
    if (current_mode_) {
        current_mode_->onCancel();
    }
}

bool ModeManager::isCurrentModeIdle() const {
    if (current_mode_) {
        return current_mode_->isIdle();
    }
    return true;  // No mode = idle
}

int ModeManager::getCurrentModeId() const {
    return Settings.Mode;
}

} // namespace modes
} // namespace pcb

// Global instance (initially null, will be created in action.c initialization)
pcb::modes::ModeManager* pcb_mode_manager = nullptr;

// C wrapper functions for backward compatibility
// These allow existing C code to call mode manager functions
extern "C" {

/*!
 * \brief Initialize the mode manager
 *
 * Called once during PCB initialization to create the global mode manager.
 * Must be called before any mode operations.
 */
void initializeModeManager(void) {
    if (!pcb_mode_manager && pcb_action_context) {
        pcb_mode_manager = new pcb::modes::ModeManager(pcb_action_context);
    }
}

/*!
 * \brief Clean up the mode manager
 *
 * Called during PCB shutdown to destroy the global mode manager.
 */
void destroyModeManager(void) {
    if (pcb_mode_manager) {
        delete pcb_mode_manager;
        pcb_mode_manager = nullptr;
    }
}

/*!
 * \brief C wrapper for NotifyMode()
 *
 * Dispatches mouse click events to the current mode's onNotify() handler.
 * All modes are now implemented in C++.
 */
void NotifyMode(void) {
    if (pcb_mode_manager && pcb_action_context) {
        pcb_mode_manager->notifyClick(
            pcb_action_context->Note.X,
            pcb_action_context->Note.Y
        );
    }
}

/*!
 * \brief C wrapper for ReleaseMode()
 *
 * Dispatches mouse release events to the current mode's onRelease() handler.
 * Also handles completing drag operations started from ArrowMode (when
 * saved_mode is set, triggers the delegated mode and restores).
 */
void ReleaseMode(void) {
    if (pcb_mode_manager && pcb_action_context) {
        // Check if we're handling a drag operation from ArrowMode
        // (indicated by saved_mode being set)
        if (pcb_action_context->saved_mode) {
            // First, let the current mode handle the release
            pcb_mode_manager->notifyRelease();

            // For MOVE_MODE/COPY_MODE: trigger the move/copy by calling notify
            // The attached object has state STATE_SECOND which will trigger the action
            if (Settings.Mode == MOVE_MODE || Settings.Mode == COPY_MODE) {
                pcb_mode_manager->notifyClick(Crosshair.X, Crosshair.Y);
            }

            // Restore back to the original mode (ARROW_MODE)
            pcb_mode_manager->restoreMode();
            pcb_action_context->saved_mode = false;
        } else {
            // Normal release - dispatch to current mode
            pcb_mode_manager->notifyRelease();
        }
    }
}

/*!
 * \brief C wrapper for SaveMode()
 *
 * Saves the current mode so it can be restored later.
 * Used by ArrowMode when delegating to Move/Copy/PasteBuffer modes.
 */
void SaveMode(void) {
    if (pcb_mode_manager) {
        pcb_mode_manager->saveMode();
    }
}

/*!
 * \brief C wrapper for RestoreMode()
 *
 * Restores the previously saved mode.
 * Used to return to ArrowMode after completing a drag operation.
 */
void RestoreMode(void) {
    if (pcb_mode_manager) {
        pcb_mode_manager->restoreMode();
    }
}

/*!
 * \brief Notify mode manager of mode change from set.c
 *
 * Called by SetMode() in set.c to update the mode manager's current_mode_
 * pointer. This ensures the C++ mode handlers are used after mode changes.
 *
 * Note: This only updates current_mode_, it doesn't call onEnter/onExit
 * or set Settings.Mode (set.c handles those aspects).
 */
void NotifyModeManagerOfChange(int mode_id) {
    if (pcb_mode_manager) {
        // Find the mode and update current_mode_ directly
        // We use setMode which handles onExit/onEnter
        pcb_mode_manager->setMode(mode_id);
    }
}

/*!
 * \brief Notify mode manager of mouse motion
 *
 * Called from GUI layer when mouse moves. This allows modes to
 * implement distance-based drag detection.
 */
void NotifyModeMotion(Coord x, Coord y) {
    if (pcb_mode_manager && pcb_mode_manager->getCurrentMode()) {
        pcb_mode_manager->notifyMotion(x, y);
    }
}

/*!
 * \brief Cancel current mode operation
 *
 * Called when user presses Escape or requests cancel.
 * Dispatches to current mode's onCancel() handler.
 */
void CancelMode(void) {
    if (pcb_mode_manager) {
        pcb_mode_manager->notifyCancel();
    }
}

/*!
 * \brief Check if current mode is idle (no operation in progress)
 *
 * Used by Escape handling to decide whether to switch to ARROW_MODE.
 * \return true if mode is idle, false if operation in progress
 */
int IsModeIdle(void) {
    if (pcb_mode_manager) {
        return pcb_mode_manager->isCurrentModeIdle() ? 1 : 0;
    }
    return 1;  // No mode manager = idle
}

} // extern "C"
