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
#include "action.h"
#include "set.h"
#include "crosshair.h"
#include "misc.h"
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
// More will be added as we implement P2, P3, P4 modes:
// extern std::unique_ptr<EditorMode> createTextMode(ActionContext* context);
// extern std::unique_ptr<EditorMode> createRectangleMode(ActionContext* context);
// ... etc

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
    // TODO: registerMode(TEXT_MODE, createTextMode(context_));
    // TODO: registerMode(RECTANGLE_MODE, createRectangleMode(context_));
    // TODO: registerMode(COPY_MODE, createCopyMode(context_));
    // TODO: registerMode(MOVE_MODE, createMoveMode(context_));
    // TODO: registerMode(ROTATE_MODE, createRotateMode(context_));
    // TODO: registerMode(PASTEBUFFER_MODE, createPasteBufferMode(context_));
    // TODO: registerMode(INSERTPOINT_MODE, createInsertPointMode(context_));
    // TODO: registerMode(POLYGONHOLE_MODE, createPolygonHoleMode(context_));

    // Phase 3: Complex modes (Days 11-18)
    // TODO: registerMode(LINE_MODE, createLineMode(context_));
    // TODO: registerMode(ARC_MODE, createArcMode(context_));
    // TODO: registerMode(POLYGON_MODE, createPolygonMode(context_));

    // Phase 4: ArrowMode (Days 19-21)
    // TODO: registerMode(ARROW_MODE, createArrowMode(context_));

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
 * This maintains backward compatibility with existing C code that calls
 * NotifyMode(). It dispatches to the C++ mode manager.
 *
 * During transition, this will fall back to the old C implementation
 * if the mode manager isn't initialized or the mode isn't implemented yet.
 */
void NotifyMode(void) {
    if (pcb_mode_manager && pcb_action_context) {
        // Check if current mode is implemented in C++
        if (pcb_mode_manager->getCurrentMode()) {
            // New C++ path - dispatch through mode manager
            pcb_mode_manager->notifyClick(
                pcb_action_context->Note.X,
                pcb_action_context->Note.Y
            );
        } else {
            // Fallback: Mode not yet migrated to C++
            // Use legacy implementation
            NotifyMode_Legacy();
        }
    } else {
        // Fallback: Mode manager not initialized yet
        // This shouldn't happen in normal operation, but provides safety
        // during transition
        NotifyMode_Legacy();
    }
}

/*!
 * \brief C wrapper for ReleaseMode()
 *
 * Backward compatibility wrapper for mouse release events.
 */
void ReleaseMode(void) {
    if (pcb_mode_manager) {
        pcb_mode_manager->notifyRelease();
    }
}

/*!
 * \brief C wrapper for SaveMode()
 *
 * Backward compatibility wrapper for mode save.
 */
void SaveMode(void) {
    if (pcb_mode_manager) {
        pcb_mode_manager->saveMode();
    }
}

/*!
 * \brief C wrapper for RestoreMode()
 *
 * Backward compatibility wrapper for mode restore.
 */
void RestoreMode(void) {
    if (pcb_mode_manager) {
        pcb_mode_manager->restoreMode();
    }
}

} // extern "C"
