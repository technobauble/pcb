# State Pattern Implementation Plan for NotifyMode Refactoring

**Date:** November 19, 2025
**Branch:** claude/refactor-action-c-01YXGswdA4r3spfjFQcru8om
**Status:** Implementation Ready
**Approach:** Full State Pattern Refactoring (Best Practices)

---

## Executive Summary

This document provides a comprehensive implementation plan for refactoring the 820-line NotifyMode() state machine into a clean, modern State Pattern architecture. This is the **correct architectural solution** that will result in maintainable, testable, and extensible code that meets current best practices.

**Decision Rationale:** While this approach requires more effort (28-30 days) than the hybrid wrapper (6 days), it eliminates a major source of technical debt and creates a foundation for future UI development.

---

## Table of Contents

1. [Architecture Design](#architecture-design)
2. [Phase-by-Phase Implementation](#phase-by-phase-implementation)
3. [Mode Complexity Analysis](#mode-complexity-analysis)
4. [Testing Strategy](#testing-strategy)
5. [Migration Path](#migration-path)
6. [Risk Mitigation](#risk-mitigation)

---

## Architecture Design

### State Pattern Overview

The State Pattern allows an object to alter its behavior when its internal state changes. For our editor modes, this means:

```
Current (Anti-pattern):
┌─────────────┐
│ NotifyMode()│  820-line switch statement
│             │  16 case blocks
│  switch(mode)
│   case ARROW_MODE: ...
│   case LINE_MODE: ...
│   case ARC_MODE: ...
│   ... 13 more
└─────────────┘

New (State Pattern):
┌─────────────┐
│ModeManager  │────┐
│             │    │
│current_mode_│◄───┼──┐
└─────────────┘    │  │
                   │  │
        ┌──────────┴──▼───────────┐
        │   EditorMode (base)     │
        │  + onNotify()           │
        │  + onRelease()          │
        │  + onEnter() / onExit() │
        └─────────────────────────┘
                    △
                    │ inherits
        ┌───────────┴───────────┐
        │                       │
   ┌────▼─────┐           ┌────▼─────┐
   │ArrowMode │           │LineMode  │  ... 14 more
   │          │           │          │
   └──────────┘           └──────────┘
```

### Class Hierarchy

```cpp
// Base class for all editor modes
class EditorMode {
public:
    virtual ~EditorMode() = default;

    // Core operations
    virtual void onNotify(Coord x, Coord y) = 0;   // Mouse click
    virtual void onRelease() = 0;                   // Mouse release
    virtual void onEnter() {}                       // Mode activated
    virtual void onExit() {}                        // Mode deactivated
    virtual void onMotion(Coord x, Coord y) {}      // Mouse moved

    // Query methods
    virtual const char* getName() const = 0;
    virtual int getModeId() const = 0;
    virtual bool allowsSelection() const { return false; }

protected:
    EditorMode(ActionContext* context) : context_(context) {}
    ActionContext* context_;  // Shared state
};

// Mode manager handles transitions
class ModeManager {
public:
    explicit ModeManager(ActionContext* context);

    // Mode control
    void setMode(int mode_id);
    void saveMode();
    void restoreMode();

    // Event dispatch
    void notifyClick(Coord x, Coord y);
    void notifyRelease();
    void notifyMotion(Coord x, Coord y);

    // Query
    EditorMode* getCurrentMode() const { return current_mode_; }
    int getCurrentModeId() const;

private:
    void registerMode(int id, std::unique_ptr<EditorMode> mode);

    ActionContext* context_;
    std::map<int, std::unique_ptr<EditorMode>> modes_;
    EditorMode* current_mode_ = nullptr;
    int saved_mode_id_ = NO_MODE;
};
```

### File Structure

```
src/
├── actions/
│   ├── ActionContext.h          # Shared state (from other branch)
│   ├── ActionContext.c          # Context initialization
│   ├── modes/
│   │   ├── EditorMode.h         # Base class + ModeManager
│   │   ├── ModeManager.cpp      # Mode manager implementation
│   │   ├── ArrowMode.cpp        # Selection mode (~50 lines)
│   │   ├── ViaMode.cpp          # Via placement (~40 lines)
│   │   ├── ArcMode.cpp          # Arc creation (~120 lines)
│   │   ├── LineMode.cpp         # Line drawing (~180 lines)
│   │   ├── PolygonMode.cpp      # Polygon creation (~100 lines)
│   │   ├── PolygonHoleMode.cpp  # Polygon holes (~80 lines)
│   │   ├── RectangleMode.cpp    # Rectangle drawing (~70 lines)
│   │   ├── TextMode.cpp         # Text placement (~60 lines)
│   │   ├── RemoveMode.cpp       # Object deletion (~30 lines)
│   │   ├── RotateMode.cpp       # Object rotation (~90 lines)
│   │   ├── CopyMode.cpp         # Object copying (~80 lines)
│   │   ├── MoveMode.cpp         # Object moving (~80 lines)
│   │   ├── LockMode.cpp         # Lock/unlock (~60 lines)
│   │   ├── ThermalMode.cpp      # Thermal relief (~40 lines)
│   │   ├── InsertPointMode.cpp  # Polygon point insertion (~80 lines)
│   │   └── PasteBufferMode.cpp  # Paste buffer (~80 lines)
│   ├── ModeAction.cpp           # Action("Mode", ...)
│   └── DeleteAction.cpp         # Action("Delete", ...)
```

---

## Mode Complexity Analysis

I've analyzed each of the 16 mode cases in NotifyMode to understand their complexity:

| Mode | Lines | State Machine | Complexity | Dependencies | Priority |
|------|-------|---------------|------------|--------------|----------|
| **RemoveMode** | ~12 | None | Very Low | SearchScreen, RemoveObject | P1 (start here) |
| **ViaMode** | ~22 | None | Low | CreateNewVia, DrawVia | P1 |
| **ThermalMode** | ~22 | None | Low | SearchScreen, ChangeObjectThermal | P1 |
| **LockMode** | ~44 | None | Low | SearchScreen, flag operations | P1 |
| **TextMode** | ~48 | None | Medium | CreateNewText, DrawText | P2 |
| **RectangleMode** | ~52 | 2 states | Medium | CreateNewPolygonFromRectangle | P2 |
| **PolygonHoleMode** | ~55 | Multi-state | Medium | Polygon operations | P2 |
| **CopyMode** | ~61 | None | Medium | CopyObject, flag operations | P2 |
| **MoveMode** | ~61 | None | Medium | MoveObject, rubberband | P2 |
| **PasteBufferMode** | ~65 | None | Medium | CopyPastebufferToLayout | P2 |
| **InsertPointMode** | ~66 | None | Medium | Polygon point insertion | P2 |
| **PolygonMode** | ~71 | Multi-state | High | Complex polygon logic | P3 |
| **RotateMode** | ~78 | None | Medium | RotateObject, flag operations | P2 |
| **ArcMode** | ~84 | 3 states | High | Complex arc geometry | P3 |
| **ArrowMode** | ~32 | None | High | Complex selection logic, timers | P4 (last) |
| **LineMode** | ~147 | 3 states | Very High | Multi-segment lines, vias, clipping | P3 |

**Priority Strategy:**
- **P1 (Week 1):** Simple modes, no state machines - Build confidence, establish patterns
- **P2 (Week 2-3):** Medium complexity modes - Most of the modes
- **P3 (Week 3-4):** Complex multi-state modes - Line, Arc, Polygon
- **P4 (Week 4):** Arrow mode last - Most complex selection logic, handles timer callbacks

---

## Phase-by-Phase Implementation

### Phase 1: Foundation (Days 1-3)

#### Day 1: ActionContext and Base Classes

**1.1 Port ActionContext**
```bash
# Copy from other branch
git show origin/claude/fix-savebufferelements-error-01Sqseb6JL3wUpGkFNQjHSko:src/actions/ActionContext.h \
  > src/actions/ActionContext.h
```

**1.2 Create EditorMode Base Class**

File: `src/actions/modes/EditorMode.h`

```cpp
#ifndef PCB_EDITOR_MODE_H
#define PCB_EDITOR_MODE_H

#ifdef __cplusplus

#include <memory>
#include <map>
#include <string>

extern "C" {
#include "global.h"
#include "actions/ActionContext.h"
}

namespace pcb {
namespace modes {

/*!
 * \brief Base class for all editor modes
 *
 * This class defines the interface that all editor modes must implement.
 * Each mode handles user interactions (clicks, releases, motion) differently.
 *
 * The State Pattern allows us to encapsulate mode-specific behavior in
 * separate classes instead of a giant switch statement.
 */
class EditorMode {
public:
    virtual ~EditorMode() = default;

    /*!
     * \brief Handle mouse click (button press) event
     * \param x X coordinate of click
     * \param y Y coordinate of click
     *
     * This is the core operation - called when the user clicks in the PCB.
     * Different modes respond differently (e.g., LineMode creates line segments,
     * ViaMode places a via, ArrowMode selects objects).
     */
    virtual void onNotify(Coord x, Coord y) = 0;

    /*!
     * \brief Handle mouse button release event
     *
     * Called when the user releases the mouse button. Some modes use this
     * (e.g., to complete drag operations), others ignore it.
     */
    virtual void onRelease() = 0;

    /*!
     * \brief Called when this mode is activated
     *
     * Override to perform mode initialization (e.g., set cursor shape,
     * initialize mode-specific state).
     */
    virtual void onEnter() {}

    /*!
     * \brief Called when this mode is deactivated
     *
     * Override to perform cleanup (e.g., reset crosshair, clear attached objects).
     */
    virtual void onExit() {}

    /*!
     * \brief Handle mouse motion event
     * \param x Current X coordinate
     * \param y Current Y coordinate
     *
     * Called when mouse moves (used for rubberbanding, cursor updates, etc.)
     */
    virtual void onMotion(Coord x, Coord y) {}

    /*!
     * \brief Get mode name for display/debugging
     */
    virtual const char* getName() const = 0;

    /*!
     * \brief Get mode ID constant (e.g., LINE_MODE, ARC_MODE)
     */
    virtual int getModeId() const = 0;

    /*!
     * \brief Does this mode allow object selection?
     *
     * Most modes don't allow selection (return false). ArrowMode returns true.
     */
    virtual bool allowsSelection() const { return false; }

protected:
    /*!
     * \brief Constructor (protected - only subclasses can instantiate)
     * \param context Shared action context for state access
     */
    explicit EditorMode(ActionContext* context) : context_(context) {}

    ActionContext* context_;  ///< Shared state (Note, crosshair position, etc.)
};

/*!
 * \brief Manages editor mode transitions and event dispatch
 *
 * This class maintains the collection of all mode instances and handles
 * switching between them. It's the "Context" in the State Pattern.
 */
class ModeManager {
public:
    /*!
     * \brief Constructor
     * \param context Shared action context
     */
    explicit ModeManager(ActionContext* context);

    /*!
     * \brief Destructor
     */
    ~ModeManager();

    /*!
     * \brief Switch to a different mode
     * \param mode_id Mode constant (e.g., LINE_MODE, ARC_MODE)
     *
     * Calls onExit() on current mode, then onEnter() on new mode.
     */
    void setMode(int mode_id);

    /*!
     * \brief Save current mode ID for later restoration
     *
     * Used by ActionDelete and stroke operations to temporarily
     * switch modes then switch back.
     */
    void saveMode();

    /*!
     * \brief Restore previously saved mode
     */
    void restoreMode();

    /*!
     * \brief Dispatch click event to current mode
     * \param x X coordinate
     * \param y Y coordinate
     */
    void notifyClick(Coord x, Coord y);

    /*!
     * \brief Dispatch release event to current mode
     */
    void notifyRelease();

    /*!
     * \brief Dispatch motion event to current mode
     * \param x X coordinate
     * \param y Y coordinate
     */
    void notifyMotion(Coord x, Coord y);

    /*!
     * \brief Get current mode instance
     */
    EditorMode* getCurrentMode() const { return current_mode_; }

    /*!
     * \brief Get current mode ID
     */
    int getCurrentModeId() const;

private:
    /*!
     * \brief Register a mode instance
     * \param id Mode ID constant
     * \param mode Unique pointer to mode instance
     */
    void registerMode(int id, std::unique_ptr<EditorMode> mode);

    /*!
     * \brief Create and register all mode instances
     */
    void initializeModes();

    ActionContext* context_;                              ///< Shared state
    std::map<int, std::unique_ptr<EditorMode>> modes_;   ///< All mode instances
    EditorMode* current_mode_;                           ///< Current active mode
    int saved_mode_id_;                                  ///< Saved mode for restore
};

} // namespace modes
} // namespace pcb

extern "C" {
    /*!
     * \brief Global mode manager instance
     */
    extern pcb::modes::ModeManager* pcb_mode_manager;
}

#endif // __cplusplus

#endif // PCB_EDITOR_MODE_H
```

**1.3 Create ModeManager Implementation**

File: `src/actions/modes/ModeManager.cpp`

```cpp
#include "EditorMode.h"

extern "C" {
#include "set.h"
#include "crosshair.h"
}

// Forward declarations of mode classes (will implement later)
namespace pcb {
namespace modes {

// P1 modes (simple)
class RemoveMode;
class ViaMode;
class ThermalMode;
class LockMode;

// P2 modes (medium)
class TextMode;
class RectangleMode;
class PolygonHoleMode;
class CopyMode;
class MoveMode;
class PasteBufferMode;
class InsertPointMode;
class RotateMode;

// P3 modes (complex)
class PolygonMode;
class ArcMode;
class LineMode;

// P4 modes (most complex)
class ArrowMode;

} // namespace modes
} // namespace pcb

namespace pcb {
namespace modes {

ModeManager::ModeManager(ActionContext* context)
    : context_(context)
    , current_mode_(nullptr)
    , saved_mode_id_(NO_MODE)
{
    // Initialize all modes in Phase 2+
    // For now, just set up the infrastructure
}

ModeManager::~ModeManager() = default;

void ModeManager::registerMode(int id, std::unique_ptr<EditorMode> mode) {
    modes_[id] = std::move(mode);
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
        // Mode not implemented yet - log warning
        fprintf(stderr, "Warning: Mode %d not implemented\n", mode_id);
        current_mode_ = nullptr;
    }

    // Update global Settings.Mode for compatibility
    Settings.Mode = mode_id;
}

void ModeManager::saveMode() {
    saved_mode_id_ = Settings.Mode;
}

void ModeManager::restoreMode() {
    if (saved_mode_id_ != NO_MODE) {
        setMode(saved_mode_id_);
    }
}

void ModeManager::notifyClick(Coord x, Coord y) {
    if (current_mode_) {
        // Update context
        context_->Note.X = x;
        context_->Note.Y = y;

        // Clear rat warnings if enabled
        if (Settings.RatWarn) {
            ClearWarnings();
        }

        // Dispatch to mode
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

void ModeManager::initializeModes() {
    // Will register modes as we implement them
    // Example:
    // registerMode(REMOVE_MODE, std::make_unique<RemoveMode>(context_));
    // registerMode(VIA_MODE, std::make_unique<ViaMode>(context_));
    // ... etc
}

} // namespace modes
} // namespace pcb

// Global instance
pcb::modes::ModeManager* pcb_mode_manager = nullptr;

// C wrapper functions for backward compatibility
extern "C" {

void NotifyMode(void) {
    if (pcb_mode_manager) {
        pcb_mode_manager->notifyClick(
            pcb_action_context->Note.X,
            pcb_action_context->Note.Y
        );
    }
}

void ReleaseMode(void) {
    if (pcb_mode_manager) {
        pcb_mode_manager->notifyRelease();
    }
}

} // extern "C"
```

#### Day 2: Implement First Simple Mode (RemoveMode)

This is our "Hello World" to prove the pattern works.

File: `src/actions/modes/RemoveMode.cpp`

```cpp
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
 * This is the simplest mode - just search for object at click position
 * and remove it if found.
 */
class RemoveMode : public EditorMode {
public:
    explicit RemoveMode(ActionContext* context)
        : EditorMode(context)
    {}

    void onNotify(Coord x, Coord y) override {
        void *ptr1, *ptr2, *ptr3;

        // Search for removable object at cursor
        int type = SearchScreen(x, y, REMOVE_TYPES, &ptr1, &ptr2, &ptr3);

        if (type != NO_TYPE) {
            // Remove the object
            if (RemoveObject(type, ptr1, ptr2, ptr3)) {
                IncrementUndoSerialNumber();
                SetChangedFlag(true);
            }
        }
    }

    void onRelease() override {
        // RemoveMode doesn't use release events
    }

    const char* getName() const override {
        return "Remove";
    }

    int getModeId() const override {
        return REMOVE_MODE;
    }
};

// Factory function for registration
std::unique_ptr<EditorMode> createRemoveMode(ActionContext* context) {
    return std::make_unique<RemoveMode>(context);
}

} // namespace modes
} // namespace pcb
```

**Test RemoveMode:**
```cpp
// In ModeManager::initializeModes()
registerMode(REMOVE_MODE, createRemoveMode(context_));

// Test:
// 1. Compile and link
// 2. Switch to Remove mode
// 3. Click on object
// 4. Verify object is deleted
```

#### Day 3: Implement More P1 Modes (Via, Thermal, Lock)

File: `src/actions/modes/ViaMode.cpp`

```cpp
#include "EditorMode.h"

extern "C" {
#include "global.h"
#include "create.h"
#include "draw.h"
#include "thermal.h"
#include "undo.h"
#include "misc.h"
#include "gui.h"
}

namespace pcb {
namespace modes {

class ViaMode : public EditorMode {
public:
    explicit ViaMode(ActionContext* context) : EditorMode(context) {}

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

    void onRelease() override {
        // ViaMode doesn't use release events
    }

    const char* getName() const override { return "Via"; }
    int getModeId() const override { return VIA_MODE; }
};

std::unique_ptr<EditorMode> createViaMode(ActionContext* context) {
    return std::make_unique<ViaMode>(context);
}

} // namespace modes
} // namespace pcb
```

Similar implementations for ThermalMode and LockMode (extract logic from action.c lines 1186-1208 and 1140-1184).

---

### Phase 2: Medium Complexity Modes (Days 4-10)

Implement P2 modes one at a time, testing each:
- TextMode (Day 4)
- RectangleMode (Day 5) - has STATE_FIRST/STATE_SECOND
- CopyMode, MoveMode (Day 6-7)
- RotateMode (Day 8)
- PasteBufferMode, InsertPointMode (Day 9-10)

**Example: RectangleMode with State**

File: `src/actions/modes/RectangleMode.cpp`

```cpp
#include "EditorMode.h"

extern "C" {
#include "global.h"
#include "crosshair.h"
#include "create.h"
#include "draw.h"
#include "undo.h"
#include "polygon.h"
#include "set.h"
}

namespace pcb {
namespace modes {

class RectangleMode : public EditorMode {
public:
    explicit RectangleMode(ActionContext* context) : EditorMode(context) {}

    void onNotify(Coord x, Coord y) override {
        switch (Crosshair.AttachedBox.State) {
        case STATE_FIRST:
            // First click - set corner 1
            Crosshair.AttachedBox.Point1.X = x;
            Crosshair.AttachedBox.Point1.Y = y;
            Crosshair.AttachedBox.Point2.X = x;
            Crosshair.AttachedBox.Point2.Y = y;
            Crosshair.AttachedBox.State = STATE_SECOND;
            break;

        case STATE_SECOND:
            // Second click - create rectangle
            if (Crosshair.AttachedBox.Point1.X != x ||
                Crosshair.AttachedBox.Point1.Y != y) {

                Coord x1 = Crosshair.AttachedBox.Point1.X;
                Coord y1 = Crosshair.AttachedBox.Point1.Y;

                PolygonType* polygon = CreateNewPolygonFromRectangle(
                    CURRENT,
                    x1, y1, x, y,
                    MakeFlags(TEST_FLAG(CLEARNEWFLAG, PCB) ? CLEARPOLYFLAG : 0)
                );

                if (polygon) {
                    AddObjectToCreateUndoList(POLYGON_TYPE, CURRENT, polygon, polygon);
                    IncrementUndoSerialNumber();
                    DrawPolygon(CURRENT, polygon);
                    Draw();
                }
            }

            // Reset for next rectangle
            Crosshair.AttachedBox.State = STATE_FIRST;
            break;
        }
    }

    void onRelease() override {}

    void onEnter() override {
        Crosshair.AttachedBox.State = STATE_FIRST;
    }

    void onExit() override {
        Crosshair.AttachedBox.State = STATE_FIRST;
    }

    const char* getName() const override { return "Rectangle"; }
    int getModeId() const override { return RECTANGLE_MODE; }
};

std::unique_ptr<EditorMode> createRectangleMode(ActionContext* context) {
    return std::make_unique<RectangleMode>(context);
}

} // namespace modes
} // namespace pcb
```

---

### Phase 3: Complex Multi-State Modes (Days 11-18)

These are the hardest modes with complex state machines.

#### LineMode (Days 11-14)

This is the most complex mode - 147 lines with multi-segment line drawing, via creation, clipping, etc.

File: `src/actions/modes/LineMode.cpp`

```cpp
#include "EditorMode.h"

extern "C" {
#include "global.h"
#include "crosshair.h"
#include "create.h"
#include "draw.h"
#include "undo.h"
#include "set.h"
#include "rats.h"
#include "find.h"
}

namespace pcb {
namespace modes {

/*!
 * \brief Line drawing mode with multi-segment support
 *
 * This is the most complex mode:
 * - Three states: STATE_FIRST (no line), STATE_SECOND (one point),
 *   STATE_THIRD (attaching to previous line)
 * - Handles rat lines vs copper lines
 * - Creates vias when switching layers
 * - Supports line clipping
 * - Tracks multiple segments with addedLines counter
 */
class LineMode : public EditorMode {
public:
    explicit LineMode(ActionContext* context) : EditorMode(context) {}

    void onNotify(Coord x, Coord y) override {
        // Update line endpoint
        NotifyLine();

        // Check if we're in multi-segment mode
        if (Crosshair.AttachedLine.State != STATE_THIRD) {
            return;
        }

        // Clicking on start point cancels the line
        if (x == Crosshair.AttachedLine.Point1.X &&
            y == Crosshair.AttachedLine.Point1.Y) {
            SetMode(LINE_MODE);  // Reset mode
            return;
        }

        if (PCB->RatDraw) {
            createRatLine();
        } else {
            createCopperLine(x, y);
        }
    }

    void onRelease() override {
        // Line mode doesn't use release
    }

    void onEnter() override {
        // Initialize line state
        Crosshair.AttachedLine.State = STATE_FIRST;
        context_->addedLines = 0;
    }

    void onExit() override {
        // Clean up attached line
        Crosshair.AttachedLine.State = STATE_FIRST;
    }

    const char* getName() const override { return "Line"; }
    int getModeId() const override { return LINE_MODE; }

private:
    void createRatLine() {
        RatType* line = AddNet();
        if (line) {
            context_->addedLines++;
            AddObjectToCreateUndoList(RATLINE_TYPE, line, line, line);
            IncrementUndoSerialNumber();
            DrawRat(line);

            // Move to next segment
            Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X;
            Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y;
            Draw();
        }
    }

    void createCopperLine(Coord x, Coord y) {
        int line_flags = 0;

        // Set flags based on PCB settings
        if (TEST_FLAG(AUTODRCFLAG, PCB) && !TEST_SILK_LAYER(CURRENT)) {
            line_flags |= CONNECTEDFLAG | FOUNDFLAG;
        }
        if (TEST_FLAG(CLEARNEWFLAG, PCB)) {
            line_flags |= CLEARLINEFLAG;
        }

        // Handle clipping
        if (PCB->Clipping &&
            Crosshair.AttachedLine.Point1.X == Crosshair.AttachedLine.Point2.X &&
            Crosshair.AttachedLine.Point1.Y == Crosshair.AttachedLine.Point2.Y &&
            (Crosshair.AttachedLine.Point2.X != x ||
             Crosshair.AttachedLine.Point2.Y != y)) {
            // Swap segments for via detection
            Crosshair.AttachedLine.Point2.X = x;
            Crosshair.AttachedLine.Point2.Y = y;
        }

        // Create line if endpoints differ
        if (Crosshair.AttachedLine.Point1.X != Crosshair.AttachedLine.Point2.X ||
            Crosshair.AttachedLine.Point1.Y != Crosshair.AttachedLine.Point2.Y) {

            createLineSegment(line_flags);
        }

        // Handle second segment if clipping enabled
        if (PCB->Clipping &&
            (Crosshair.AttachedLine.Point2.X != x ||
             Crosshair.AttachedLine.Point2.Y != y)) {

            Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X;
            Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y;
            Crosshair.AttachedLine.Point2.X = x;
            Crosshair.AttachedLine.Point2.Y = y;

            createLineSegment(line_flags);
        }

        Draw();
    }

    void createLineSegment(int line_flags) {
        LineType* line = CreateDrawnLineOnLayer(
            CURRENT,
            Crosshair.AttachedLine.Point1.X,
            Crosshair.AttachedLine.Point1.Y,
            Crosshair.AttachedLine.Point2.X,
            Crosshair.AttachedLine.Point2.Y,
            Settings.LineThickness,
            2 * Settings.Keepaway,
            MakeFlags(line_flags)
        );

        if (line) {
            context_->addedLines++;
            AddObjectToCreateUndoList(LINE_TYPE, CURRENT, line, line);

            // Check for layer change and create via if needed
            checkAndCreateVia();

            DrawLine(CURRENT, line);
            IncrementUndoSerialNumber();

            // Update for next segment
            Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X;
            Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y;
            context_->lastLayer = CURRENT;
        }
    }

    void checkAndCreateVia() {
        // Via creation logic when switching layers
        // (Extract from lines 1278-1347 of action.c)
        // ... detailed via creation code ...
    }
};

std::unique_ptr<EditorMode> createLineMode(ActionContext* context) {
    return std::make_unique<LineMode>(context);
}

} // namespace modes
} // namespace pcb
```

#### ArcMode (Days 15-16)

Similar complexity to LineMode - multi-state arc creation with geometry calculations.

#### PolygonMode (Days 17-18)

Multi-point polygon creation with hole support.

---

### Phase 4: ArrowMode (Days 19-21)

This is special - handles selection logic, timer callbacks, complex hit detection.

File: `src/actions/modes/ArrowMode.cpp`

```cpp
#include "EditorMode.h"

extern "C" {
#include "global.h"
#include "search.h"
#include "crosshair.h"
#include "gui.h"
}

namespace pcb {
namespace modes {

class ArrowMode : public EditorMode {
public:
    explicit ArrowMode(ActionContext* context) : EditorMode(context) {}

    void onNotify(Coord x, Coord y) override {
        void *ptr1, *ptr2, *ptr3;
        int test;

        context_->Note.Click = true;

        // Set up click timer callback
        hidval hv;
        gui->add_timer(click_cb, CLICK_TIME, hv);

        // Search for selected or movable objects
        for (test = (SELECT_TYPES | MOVE_TYPES) & ~RATLINE_TYPE;
             test; test &= ~type) {

            int type = SearchScreen(x, y, test, &ptr1, &ptr2, &ptr3);

            // Check for movable objects
            if (!context_->Note.Hit && (type & MOVE_TYPES) &&
                !TEST_FLAG(LOCKFLAG, (PinType*)ptr2)) {
                context_->Note.Hit = type;
                context_->Note.ptr1 = ptr1;
                context_->Note.ptr2 = ptr2;
                context_->Note.ptr3 = ptr3;
            }

            // Check for already-selected objects
            if (!context_->Note.Moving && (type & SELECT_TYPES) &&
                TEST_FLAG(SELECTEDFLAG, (PinType*)ptr2)) {
                context_->Note.Moving = true;
            }

            if ((context_->Note.Hit && context_->Note.Moving) ||
                type == NO_TYPE) {
                break;
            }
        }
    }

    void onRelease() override {
        // Handle selection logic on release
        // (Extract from ReleaseMode in action.c)
    }

    bool allowsSelection() const override {
        return true;  // Arrow mode allows selection
    }

    const char* getName() const override { return "Arrow"; }
    int getModeId() const override { return ARROW_MODE; }
};

std::unique_ptr<EditorMode> createArrowMode(ActionContext* context) {
    return std::make_unique<ArrowMode>(context);
}

} // namespace modes
} // namespace pcb
```

---

### Phase 5: Migrate ActionMode and ActionDelete (Days 22-24)

Now that all modes are implemented, migrating the actions is straightforward.

File: `src/actions/ModeAction.cpp`

```cpp
#include "Action.h"
#include "modes/EditorMode.h"

extern "C" {
#include "global.h"
#include "actions/ActionContext.h"
#include "action.h"
int GetFunctionID(const char*);
void notify_crosshair_change(bool);
}

namespace pcb {
namespace actions {

class ModeAction : public Action {
public:
    ModeAction() : Action("Mode", /* syntax */, /* help */) {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        const char* function = (argc > 0) ? argv[0] : nullptr;
        if (!function) return 1;

        pcb_action_context->Note.X = Crosshair.X;
        pcb_action_context->Note.Y = Crosshair.Y;

        notify_crosshair_change(false);

        // Use mode manager instead of SetMode()
        switch (GetFunctionID(function)) {
        case F_Arc:
            pcb_mode_manager->setMode(ARC_MODE);
            break;
        case F_Line:
            pcb_mode_manager->setMode(LINE_MODE);
            break;
        // ... all other modes
        case F_Notify:
            pcb_mode_manager->notifyClick(x, y);
            break;
        case F_Release:
            pcb_mode_manager->notifyRelease();
            break;
        case F_Save:
            pcb_mode_manager->saveMode();
            break;
        case F_Restore:
            pcb_mode_manager->restoreMode();
            break;
        }

        notify_crosshair_change(true);
        return 0;
    }
};

REGISTER_ACTION(ModeAction);

} // namespace actions
} // namespace pcb
```

Similar clean implementation for DeleteAction.

---

### Phase 6: Integration & Testing (Days 25-28)

#### Testing Strategy

**Unit Tests for Each Mode:**
```cpp
// tests/cpp/modes/RemoveMode_test.cpp
#include <gtest/gtest.h>
#include "modes/EditorMode.h"
#include "test_fixtures.h"

TEST(RemoveModeTest, DeleteLine) {
    PCBTestFixture fixture;
    ActionContext context;
    auto mode = createRemoveMode(&context);

    // Create a test line
    LineType* line = createTestLine(0, 0, 100, 100);

    // Click on line
    mode->onNotify(50, 50);

    // Verify line was deleted
    EXPECT_TRUE(lineWasDeleted(line));
}
```

**Integration Tests:**
```cpp
TEST(ModeManagerTest, ModeTransitions) {
    PCBTestFixture fixture;
    ActionContext context;
    ModeManager manager(&context);

    // Test mode switching
    manager.setMode(LINE_MODE);
    EXPECT_EQ(LINE_MODE, manager.getCurrentModeId());

    manager.setMode(ARC_MODE);
    EXPECT_EQ(ARC_MODE, manager.getCurrentModeId());
}

TEST(ModeManagerTest, SaveRestore) {
    ModeManager manager(&context);

    manager.setMode(LINE_MODE);
    manager.saveMode();
    manager.setMode(ARC_MODE);
    manager.restoreMode();

    EXPECT_EQ(LINE_MODE, manager.getCurrentModeId());
}
```

**Manual Testing Checklist:**
- [ ] All 16 modes switch correctly
- [ ] Each mode's onNotify works (create objects, select, delete, etc.)
- [ ] Multi-state modes progress through states correctly
- [ ] Mode save/restore works
- [ ] No memory leaks (run with AddressSanitizer)
- [ ] No crashes with rapid mode switching
- [ ] Undo works correctly for all modes
- [ ] Existing action.c tests still pass

---

## Migration Path

### Incremental Approach

The beauty of this design is we can migrate incrementally:

**Week 1:** RemoveMode, ViaMode, ThermalMode, LockMode
- NotifyMode switch statement has fallback for unimplemented modes
- Can test each mode individually

**Week 2-3:** All P2 modes
- Most modes done
- Application mostly functional

**Week 4:** Complex modes + ArrowMode
- Complete the migration
- Remove old NotifyMode code

### Backward Compatibility

```c
// Keep C wrapper during transition
void NotifyMode(void) {
    if (pcb_mode_manager) {
        // New C++ path
        pcb_mode_manager->notifyClick(
            pcb_action_context->Note.X,
            pcb_action_context->Note.Y
        );
    } else {
        // Old C path (gradually remove mode cases as we migrate)
        switch (Settings.Mode) {
        case REMOVE_MODE:
            // ... old code ...
            break;
        // Other cases removed as modes are migrated
        }
    }
}
```

### Safe Rollback

Each phase is a separate commit. If we discover issues:
```bash
git revert <commit-hash>  # Rollback specific mode
# or
git reset --hard <earlier-commit>  # Rollback entire phase
```

---

## Risk Mitigation

### Technical Risks

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Mode behavior changes | Medium | High | Extensive testing, unit tests per mode |
| Memory leaks | Low | Medium | AddressSanitizer, RAII design |
| Performance regression | Low | Low | Modes are lightweight, no perf impact expected |
| Compilation issues | Low | Medium | Incremental commits, CI on every commit |
| Subtle state bugs | Medium | High | Careful extraction, side-by-side comparison |

### Process Risks

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Takes longer than estimated | Medium | Medium | Incremental delivery, can pause at any week |
| Discover new dependencies | Low | Medium | Thorough analysis completed, unlikely |
| Breaking existing features | Low | High | Comprehensive test suite, manual QA |

---

## Success Criteria

### Technical Milestones

- [ ] **Week 1:** 4 simple modes working (Remove, Via, Thermal, Lock)
- [ ] **Week 2:** 8 more modes added (total 12/16)
- [ ] **Week 3:** 3 complex modes added (Line, Arc, Polygon) - 15/16
- [ ] **Week 4:** ArrowMode complete, ActionMode/Delete migrated - 100%

### Quality Gates

- [ ] All existing tests pass
- [ ] Each mode has unit tests
- [ ] No memory leaks (AddressSanitizer clean)
- [ ] No compiler warnings
- [ ] Code review approved
- [ ] Performance benchmarks unchanged
- [ ] Manual testing checklist complete

### Code Quality

- [ ] Each mode class < 200 lines
- [ ] Well-commented code
- [ ] Follows C++ best practices
- [ ] RAII for resource management
- [ ] Const-correctness

---

## Conclusion

This State Pattern refactoring transforms an 820-line procedural switch statement into 16 clean, testable, maintainable mode classes. While it requires more upfront effort (28-30 days), the result is:

✅ **Modern architecture** - Textbook State Pattern implementation
✅ **Testable** - Each mode can be unit tested in isolation
✅ **Maintainable** - Clear separation of concerns, easy to understand
✅ **Extensible** - New modes are easy to add (just implement interface)
✅ **Type-safe** - C++ classes vs function pointers
✅ **No technical debt** - Eliminates the largest procedural function in the codebase

**This is the right solution.** It may take a month, but the codebase will be dramatically improved and ready for modern development practices.

---

**Next Steps:**
1. Review and approve this plan
2. Begin Phase 1 (Days 1-3): Foundation
3. Implement modes incrementally (P1 → P2 → P3 → P4)
4. Migrate ActionMode and ActionDelete
5. Comprehensive testing

**Estimated Completion:** 28-30 days (4-6 weeks)
**Long-term Value:** Immeasurable

---

**Document Version:** 1.0
**Last Updated:** November 19, 2025
**Status:** Ready for Implementation
