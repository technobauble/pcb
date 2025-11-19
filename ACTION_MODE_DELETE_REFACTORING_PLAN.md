# ActionMode and ActionDelete Refactoring Plan

**Date:** November 19, 2025
**Branch:** claude/refactor-action-c-01YXGswdA4r3spfjFQcru8om
**Status:** Planning Phase

---

## Executive Summary

This document outlines a comprehensive plan for refactoring the two most complex remaining actions in action.c: **ActionMode** (212 lines) and **ActionDelete** (39 lines). These actions represent a unique architectural challenge as they both depend heavily on the **NotifyMode()** state machine (820 lines, 16 mode cases), which is the largest and most complex function in the PCB codebase.

**Key Finding:** These two actions cannot be meaningfully refactored in isolation without first addressing the NotifyMode architectural dependency. We present three strategic approaches with varying levels of effort and architectural improvement.

**Updated Recommendation (Nov 19, 2025):** **Full State Pattern Refactoring (Option 1)** - Based on stakeholder feedback prioritizing code quality and modern best practices over quick delivery, we will implement the complete State Pattern solution. This eliminates the 820-line NotifyMode() technical debt and creates a maintainable, testable architecture.

**See [STATE_PATTERN_IMPLEMENTATION_PLAN.md](STATE_PATTERN_IMPLEMENTATION_PLAN.md) for the detailed 28-30 day implementation guide.**

**Original Recommendation:** Hybrid Strategy (Option 2) - This was the pragmatic 6-day low-effort approach, but doesn't address the root architectural problems.

---

## Table of Contents

1. [Problem Analysis](#problem-analysis)
2. [Dependency Analysis](#dependency-analysis)
3. [Strategic Options](#strategic-options)
4. [Recommended Approach](#recommended-approach)
5. [Implementation Plan](#implementation-plan)
6. [Risk Assessment](#risk-assessment)
7. [Future Work](#future-work)

---

## Problem Analysis

### ActionMode Overview

**Location:** `src/action.c:3034-3243` (212 lines)
**Purpose:** Handle editor mode switching (ARC, LINE, POLYGON, ARROW, etc.)

**Functionality:**
- Sets `Note.X` and `Note.Y` to current crosshair position
- Calls `SetMode()` to switch between 17 different editor modes
- Handles special mode operations (Cancel, Escape, Notify, Release, Stroke, Save, Restore)
- Contains complex mode-specific logic for the Escape sub-command

**Key Dependencies:**
```c
Note.X = Crosshair.X;      // Static variable (needs ActionContext)
Note.Y = Crosshair.Y;      // Static variable (needs ActionContext)
SetMode(ARC_MODE);         // Calls NotifyMode() internally
NotifyMode();              // Direct call (line 3155)
ReleaseMode();             // Related mode function
SaveMode();                // Saves current mode
RestoreMode();             // Restores saved mode
```

**Complexity Factors:**
- 17 mode constants (ARC_MODE, LINE_MODE, POLYGON_MODE, etc.)
- 8 function sub-commands (Arc, Arrow, Copy, InsertPoint, Line, Lock, Move, None, PasteBuffer, Polygon, Rectangle, Remove, Rotate, Text, Thermal, Via)
- 6 control sub-commands (Notify, Release, Cancel, Escape, Save, Restore)
- Special Escape logic with nested mode-specific conditionals (80 lines)
- Libstroke integration (#ifdef HAVE_LIBSTROKE)

---

### ActionDelete Overview

**Location:** `src/action.c:3843-3879` (39 lines)
**Purpose:** Delete objects at cursor position or selected objects

**Functionality:**
- If no argument provided and no selection, defaults to F_Object mode
- Handles four sub-commands: Object, Selected, AllRats, SelectedRats
- Saves/restores mode when deleting object at cursor

**Key Dependencies:**
```c
Note.X = Crosshair.X;      // Static variable (needs ActionContext)
Note.Y = Crosshair.Y;      // Static variable (needs ActionContext)
SaveMode();                // Save current mode
SetMode(REMOVE_MODE);      // Enter remove mode
NotifyMode();              // Process click in REMOVE_MODE
RestoreMode();             // Restore previous mode
RemoveSelected();          // Delete selected objects
DeleteRats();              // Delete rat lines
```

**Complexity Factors:**
- Simpler than ActionMode (39 lines vs 212 lines)
- Critical dependency on NotifyMode() for F_Object case
- Mode save/restore pattern
- Multiple deletion strategies

---

## Dependency Analysis

### The NotifyMode Problem

**Location:** `src/action.c:985-1805` (~820 lines)
**Purpose:** Massive state machine handling mouse clicks in different editor modes

**Structure:**
```c
NotifyMode (void)
{
  void *ptr1, *ptr2, *ptr3;
  int type;

  if (Settings.RatWarn)
    ClearWarnings ();

  switch (Settings.Mode)
    {
    case ARROW_MODE:     // ~32 lines - Object selection logic
    case VIA_MODE:       // ~22 lines - Place via
    case ARC_MODE:       // ~84 lines - Multi-state arc creation
    case LOCK_MODE:      // ~44 lines - Toggle lock flag
    case THERMAL_MODE:   // ~22 lines - Toggle thermal relief
    case LINE_MODE:      // ~147 lines - Multi-segment line drawing
    case RECTANGLE_MODE: // ~52 lines - Two-click rectangle
    case TEXT_MODE:      // ~48 lines - Place text
    case POLYGON_MODE:   // ~71 lines - Multi-point polygon
    case POLYGONHOLE_MODE: // ~55 lines - Polygon hole creation
    case PASTEBUFFER_MODE: // ~65 lines - Paste buffer placement
    case REMOVE_MODE:    // ~12 lines - Delete object at cursor **(USED BY ActionDelete)**
    case ROTATE_MODE:    // ~78 lines - Rotate object
    case COPY_MODE:      // ~61 lines - Copy object
    case MOVE_MODE:      // ~61 lines - Move object
    case INSERTPOINT_MODE: // ~66 lines - Insert polygon point
    }
}
```

**Key Characteristics:**
- **16 mode cases**, each 10-150 lines of complex logic
- **Heavy state dependencies:** Note struct, Crosshair, Settings, PCB global
- **Multi-step operations:** Arc, Line, Rectangle, Polygon modes have STATE_FIRST, STATE_SECOND, STATE_THIRD
- **Side effects:** Creates objects, updates undo list, triggers redraws, modifies crosshair state
- **GUI integration:** Direct HID calls, timer callbacks, keyboard modifier checks

**Why It's a Blocker:**
1. **ActionMode calls NotifyMode directly** (line 3155) for the F_Notify sub-command
2. **ActionDelete depends on REMOVE_MODE case** in NotifyMode for object deletion
3. **SetMode() calls NotifyMode** to initialize mode state
4. **Can't be migrated incrementally** - it's an all-or-nothing 820-line function

---

### Related Functions

**SetMode()** - Located in `src/set.c`
- Changes current editor mode (Settings.Mode)
- Calls `NotifyMode()` on mode entry to initialize state
- Resets crosshair, attached objects, drawing state
- Updates UI indicators

**ReleaseMode()** - Located in `src/action.c`
- Handles mouse button release events
- Called when user completes a mode operation
- Complements NotifyMode (click) with release behavior

**SaveMode() / RestoreMode()** - Located in `src/action.c`
- Simple mode stack (single level)
- Used by ActionDelete and stroke operations
- No NotifyMode dependency

---

## Strategic Options

### Option 1: Full NotifyMode Refactoring (State Pattern)

**Description:** Refactor the 820-line NotifyMode function using the State Pattern, then migrate ActionMode and ActionDelete.

#### Architecture

```cpp
// Base class for mode handlers
class EditorMode {
public:
    virtual ~EditorMode() = default;

    // Core mode operations
    virtual void onNotify(Coord x, Coord y) = 0;
    virtual void onRelease() = 0;
    virtual void onEnter() {}
    virtual void onExit() {}

    // Query methods
    virtual const char* getName() const = 0;
    virtual int getModeId() const = 0;

protected:
    ActionContext* context_;  // Shared state
};

// Concrete mode implementations
class ArrowMode : public EditorMode {
public:
    void onNotify(Coord x, Coord y) override {
        // Extract ARROW_MODE case logic from NotifyMode
        // Lines 994-1026 of current action.c
    }
    const char* getName() const override { return "Arrow"; }
    int getModeId() const override { return ARROW_MODE; }
};

class LineMode : public EditorMode {
public:
    void onNotify(Coord x, Coord y) override {
        // Extract LINE_MODE case logic from NotifyMode
        // Lines 1210-1372 of current action.c
        // Handle STATE_FIRST, STATE_SECOND, STATE_THIRD
    }
    const char* getName() const override { return "Line"; }
    int getModeId() const override { return LINE_MODE; }

private:
    // Line-specific state
    enum State { STATE_FIRST, STATE_SECOND, STATE_THIRD };
    State state_;
};

class RemoveMode : public EditorMode {
public:
    void onNotify(Coord x, Coord y) override {
        // Extract REMOVE_MODE case logic from NotifyMode
        // Lines 1590-1602 of current action.c (only ~12 lines!)
        void *ptr1, *ptr2, *ptr3;
        int type = SearchScreen(x, y, REMOVE_TYPES, &ptr1, &ptr2, &ptr3);

        if (type != NO_TYPE && RemoveObject(type, ptr1, ptr2, ptr3)) {
            IncrementUndoSerialNumber();
            SetChangedFlag(true);
        }
    }
    const char* getName() const override { return "Remove"; }
    int getModeId() const override { return REMOVE_MODE; }
};

// ... 13 more mode classes (ArcMode, ViaMode, PolygonMode, etc.)

// Mode manager
class ModeManager {
public:
    void setMode(int mode_id) {
        if (current_mode_) {
            current_mode_->onExit();
        }
        current_mode_ = modes_[mode_id].get();
        current_mode_->onEnter();
    }

    void notifyClick(Coord x, Coord y) {
        if (current_mode_) {
            current_mode_->onNotify(x, y);
        }
    }

    void notifyRelease() {
        if (current_mode_) {
            current_mode_->onRelease();
        }
    }

private:
    std::map<int, std::unique_ptr<EditorMode>> modes_;
    EditorMode* current_mode_ = nullptr;
};

// C wrapper for backward compatibility
extern "C" {
    void NotifyMode(void) {
        mode_manager->notifyClick(
            pcb_action_context->Note.X,
            pcb_action_context->Note.Y
        );
    }
}
```

#### Then Migrate ActionMode/ActionDelete

```cpp
// ActionMode becomes straightforward
class ModeAction : public Action {
public:
    int execute(int argc, char** argv, Coord x, Coord y) override {
        const char* function = argv[0];

        pcb_action_context->Note.X = Crosshair.X;
        pcb_action_context->Note.Y = Crosshair.Y;

        notify_crosshair_change(false);

        if (strcmp(function, "Arc") == 0) {
            mode_manager->setMode(ARC_MODE);
        } else if (strcmp(function, "Line") == 0) {
            mode_manager->setMode(LINE_MODE);
        } else if (strcmp(function, "Notify") == 0) {
            mode_manager->notifyClick(x, y);
        }
        // ... 20+ more mode commands

        notify_crosshair_change(true);
        return 0;
    }
};

// ActionDelete becomes much simpler
class DeleteAction : public Action {
public:
    int execute(int argc, char** argv, Coord x, Coord y) override {
        int id = get_function_id(argv[0]);

        pcb_action_context->Note.X = Crosshair.X;
        pcb_action_context->Note.Y = Crosshair.Y;

        if (id == F_Object) {
            SaveMode();
            mode_manager->setMode(REMOVE_MODE);
            mode_manager->notifyClick(x, y);  // Calls RemoveMode::onNotify()
            RestoreMode();
        } else if (id == F_Selected) {
            RemoveSelected();
        }
        // ... handle AllRats, SelectedRats

        return 0;
    }
};
```

#### Effort Estimate

| Phase | Task | Lines Changed | Duration | Risk |
|-------|------|---------------|----------|------|
| 1 | Design Mode class hierarchy | - | 3 days | Medium |
| 2 | Create ActionContext infrastructure | ~200 | 2 days | Low |
| 3 | Implement 16 mode classes | ~1200 | 10 days | High |
| 4 | Create ModeManager | ~150 | 2 days | Medium |
| 5 | Migrate ActionMode | ~250 | 2 days | Low |
| 6 | Migrate ActionDelete | ~80 | 1 day | Low |
| 7 | Integration testing | - | 5 days | High |
| 8 | Fix discovered issues | ~300 | 3-5 days | Very High |
| **Total** | | **~2180** | **28-30 days** | **High** |

#### Benefits
✅ **Clean architecture** - State Pattern is textbook solution for mode state machines
✅ **Testability** - Each mode class can be unit tested in isolation
✅ **Extensibility** - New modes easy to add (just implement EditorMode interface)
✅ **Reduced complexity** - 16 small classes instead of 1 giant function
✅ **Type safety** - C++ classes vs C function pointers
✅ **Complete migration** - Both ActionMode and ActionDelete fully in C++

#### Drawbacks
⚠️ **Huge effort** - 28-30 days of solid work (4-6 weeks)
⚠️ **High risk** - Touching core UI state machine, many edge cases
⚠️ **Testing burden** - Must test all 16 modes × multiple states × interactions
⚠️ **Regression potential** - NotifyMode is called hundreds of times throughout the codebase
⚠️ **Integration complexity** - SetMode, ReleaseMode, mouse handlers all need updates

---

### Option 2: Minimal Wrapper (Hybrid Strategy) ⭐ **RECOMMENDED**

**Description:** Create minimal C++ wrapper around NotifyMode, migrate ActionMode/ActionDelete to C++, defer comprehensive refactoring.

#### Architecture

```cpp
// Minimal wrapper in ActionContext
extern "C" {
    // Export NotifyMode as-is for now
    void NotifyMode(void);
}

// Migrate ActionMode with minimal changes
class ModeAction : public Action {
public:
    ModeAction() : Action(
        "Mode",
        "Mode(Arc|Arrow|Copy|InsertPoint|Line|Lock|Move|None|PasteBuffer)\n"
        "Mode(Polygon|Rectangle|Remove|Rotate|Text|Thermal|Via)\n"
        "Mode(Notify|Release|Cancel|Stroke)\n"
        "Mode(Save|Restore)",
        "Change or use the tool mode."
    ) {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        const char* function = argv[0];
        if (!function) {
            return 1;  // AFAIL
        }

        // Update action context (shared state)
        pcb_action_context->Note.X = Crosshair.X;
        pcb_action_context->Note.Y = Crosshair.Y;

        notify_crosshair_change(false);

        // Use function ID lookup (existing C code)
        switch (GetFunctionID(function)) {
        case F_Arc:
            SetMode(ARC_MODE);
            break;
        case F_Arrow:
            SetMode(ARROW_MODE);
            break;
        case F_Copy:
            SetMode(COPY_MODE);
            break;
        // ... all other mode cases
        case F_Notify:
            NotifyMode();  // Call C function directly
            break;
        case F_Release:
#ifdef HAVE_LIBSTROKE
            if (pcb_action_context->mid_stroke)
                FinishStroke();
            else
                ReleaseMode();
#else
            ReleaseMode();
#endif
            break;
        case F_Cancel:
            {
                int saved_mode = Settings.Mode;
                SetMode(NO_MODE);
                SetMode(saved_mode);
            }
            break;
        case F_Escape:
            handle_escape_mode();  // Extract to helper method
            break;
        case F_Save:
            SaveMode();
            break;
        case F_Restore:
            RestoreMode();
            break;
        }

        notify_crosshair_change(true);
        return 0;
    }

private:
    void handle_escape_mode() {
        // Extract Escape logic (lines 3076-3151) to private method
        switch (Settings.Mode) {
        case VIA_MODE:
        case PASTEBUFFER_MODE:
        case TEXT_MODE:
        // ... etc (copy existing logic)
        }
    }
};

REGISTER_ACTION(ModeAction);
```

```cpp
// Migrate ActionDelete similarly
class DeleteAction : public Action {
public:
    DeleteAction() : Action(
        "Delete",
        "Delete(Object|Selected)\nDelete(AllRats|SelectedRats)",
        "Delete stuff."
    ) {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        const char* function = argv[0];
        int id = GetFunctionID(function);

        pcb_action_context->Note.X = Crosshair.X;
        pcb_action_context->Note.Y = Crosshair.Y;

        if (id == -1) {  // No argument
            if (RemoveSelected() == false) {
                id = F_Object;
            }
        }

        switch (id) {
        case F_Object:
            SaveMode();
            SetMode(REMOVE_MODE);
            NotifyMode();  // Call C function directly
            RestoreMode();
            break;
        case F_Selected:
            RemoveSelected();
            break;
        case F_AllRats:
            if (DeleteRats(false))
                SetChangedFlag(true);
            break;
        case F_SelectedRats:
            if (DeleteRats(true))
                SetChangedFlag(true);
            break;
        }

        return 0;
    }
};

REGISTER_ACTION(DeleteAction);
```

#### Required Infrastructure

1. **ActionContext** (from other branch) - ~150 lines
   - Centralizes Note struct and other shared state
   - Accessible from both C and C++ code

2. **Export Helper Functions** from action.c:
   ```c
   // Add to action.h
   void NotifyMode(void);
   void ReleaseMode(void);
   void SaveMode(void);
   void RestoreMode(void);
   int GetFunctionID(const char* function);
   #ifdef HAVE_LIBSTROKE
   void FinishStroke(void);
   #endif
   ```

3. **Remove static** from Note and related variables:
   ```c
   // In action.c - integrate ActionContext
   static ActionContext global_action_context = {0};
   ActionContext *pcb_action_context = &global_action_context;

   // Replace all Note.X → pcb_action_context->Note.X
   // Replace all addedLines → pcb_action_context->addedLines
   ```

#### Effort Estimate

| Phase | Task | Lines Changed | Duration | Risk |
|-------|------|---------------|----------|------|
| 1 | Port ActionContext from other branch | ~150 | 0.5 days | Low |
| 2 | Update action.c static vars | ~50 | 0.5 days | Low |
| 3 | Export helper functions | ~20 | 0.5 days | Low |
| 4 | Create ModeAction.cpp | ~250 | 1 day | Low |
| 5 | Create DeleteAction.cpp | ~80 | 0.5 days | Low |
| 6 | Integration testing | - | 2 days | Medium |
| 7 | Fix issues | ~50 | 1 day | Low |
| **Total** | | **~600** | **6 days** | **Low-Medium** |

#### Benefits
✅ **Low effort** - Can complete in 6 days (1 week)
✅ **Low risk** - NotifyMode stays in C, just called from C++
✅ **Incremental** - Migrate actions without refactoring NotifyMode
✅ **Proven approach** - Already successful in other branch (97% migration)
✅ **Testable** - ActionMode/ActionDelete can have C++ unit tests
✅ **Foundation** - Sets up ActionContext for future work

#### Drawbacks
⚠️ **NotifyMode remains in C** - Still 820 lines of monolithic code
⚠️ **Partial improvement** - Actions migrated but core state machine unchanged
⚠️ **Future debt** - NotifyMode refactoring still needed eventually

---

### Option 3: Defer Everything (Status Quo)

**Description:** Leave both ActionMode and ActionDelete in C, document blockers, focus efforts elsewhere.

#### Rationale

1. **NotifyMode is stable** - It works, hasn't changed much in years
2. **High complexity / low benefit** - Refactoring NotifyMode is risky for unclear gains
3. **Other priorities** - Focus on actions with better ROI
4. **97% is enough** - If we migrate everything else, 2/65 actions (3%) remaining is acceptable

#### Documentation Approach

Create `ACTION_MIGRATION_STATUS.md`:
```markdown
## Deferred Actions (2 remaining, 3%)

### ActionMode (212 lines)
- **Status:** Deferred indefinitely
- **Blocker:** Requires NotifyMode refactoring (820 lines, 16 mode cases)
- **Effort:** 4-6 weeks
- **Risk:** Very High
- **Decision:** Not worth the effort/risk at this time

### ActionDelete (39 lines)
- **Status:** Deferred indefinitely
- **Blocker:** Requires NotifyMode refactoring
- **Dependencies:** Same as ActionMode
- **Decision:** Will be migrated when NotifyMode is refactored

## NotifyMode Refactoring (Future Milestone)

**Estimated Effort:** 4-6 weeks
**Priority:** Low
**Approach:** State Pattern (see NOTIFYMODE_REFACTORING_PLAN.md)
```

#### Effort Estimate
- **Total:** 0 days (just documentation)

#### Benefits
✅ **Zero effort** - No code changes
✅ **Zero risk** - Nothing breaks
✅ **Focus time** - Team can work on higher-value tasks

#### Drawbacks
⚠️ **Incomplete migration** - 3% of actions remain in C
⚠️ **Missed opportunity** - ActionContext benefits not realized
⚠️ **Technical debt** - NotifyMode continues to be unmaintainable

---

## Recommended Approach

### ⭐ **UPDATED: Option 1: Full State Pattern Refactoring**

Based on stakeholder feedback prioritizing **code quality and modern best practices**, we now recommend **Option 1** for the following reasons:

#### Architectural Excellence
1. **Eliminates technical debt** - Removes the 820-line NotifyMode() monolith permanently
2. **Modern best practices** - State Pattern is the textbook solution for mode state machines
3. **Long-term maintainability** - 16 small, focused classes instead of one giant switch statement
4. **Foundation for future** - Clean architecture supports future UI development

#### Code Quality
- **Testability** - Each mode class can be unit tested in isolation
- **Separation of concerns** - Each mode encapsulates its own behavior
- **Type safety** - C++ virtual functions vs function pointers
- **RAII** - Automatic resource management, no memory leaks
- **Readability** - Clear mode transitions, explicit state management

#### Technical Benefits
1. **Extensibility** - New modes easy to add (just implement EditorMode interface)
2. **Debugging** - Clear call stacks, mode-specific breakpoints
3. **Documentation** - Each mode class is self-documenting
4. **Refactoring safety** - Changes to one mode don't affect others

#### Long-Term Value
- **One-time investment** - 28-30 days now saves years of maintenance
- **Reduces bugs** - Isolated mode logic reduces interaction bugs
- **Enables innovation** - Clean architecture makes new features easier
- **Team productivity** - Easier onboarding, clearer code ownership

**See [STATE_PATTERN_IMPLEMENTATION_PLAN.md](STATE_PATTERN_IMPLEMENTATION_PLAN.md) for the complete 28-30 day implementation roadmap with phase-by-phase details.**

---

### Why Not Option 2 (Hybrid)?

While Option 2 is faster (6 days), it:
- ❌ Leaves the 820-line NotifyMode() technical debt in place
- ❌ Doesn't address the root architectural problem
- ❌ Creates a wrapper around bad code instead of fixing it
- ❌ Defers the inevitable refactoring to an uncertain future

**Decision:** Invest the time now to do it right, rather than accumulating more technical debt.

---

## Implementation Plan

### Phase 1: Infrastructure (Days 1-2)

#### 1.1 Port ActionContext from Other Branch
```bash
# Copy ActionContext.h from other branch
git show origin/claude/fix-savebufferelements-error-01Sqseb6JL3wUpGkFNQjHSko:src/actions/ActionContext.h > src/actions/ActionContext.h
```

**Verify:**
- Header compiles in both C and C++
- Struct layout matches static variables in action.c

#### 1.2 Update action.c to Use ActionContext
```c
// In action.c
#include "actions/ActionContext.h"

// Allocate global context
static ActionContext global_action_context = {0};
ActionContext *pcb_action_context = &global_action_context;

// Replace static variable references (estimated ~50 locations)
// OLD: Note.X = Crosshair.X;
// NEW: pcb_action_context->Note.X = Crosshair.X;
```

**Script to find all references:**
```bash
grep -n "Note\." src/action.c | wc -l          # ~84 references
grep -n "addedLines" src/action.c | wc -l      # ~8 references
grep -n "lastLayer" src/action.c | wc -l       # ~6 references
grep -n "defer_updates" src/action.c | wc -l   # ~6 references
```

**Estimated changes:** ~100 lines

#### 1.3 Export Helper Functions
```c
// In action.h
#ifdef __cplusplus
extern "C" {
#endif

// Mode operations (already in set.c/set.h)
void SetMode(int mode);

// Export from action.c
void NotifyMode(void);
void ReleaseMode(void);
void SaveMode(void);
void RestoreMode(void);

// Function ID lookup
int GetFunctionID(const char* function);

#ifdef HAVE_LIBSTROKE
void FinishStroke(void);
extern bool mid_stroke;
extern BoxType StrokeBox;
#endif

#ifdef __cplusplus
}
#endif
```

**Remove static from:**
- `NotifyMode()`
- `ReleaseMode()`
- `SaveMode()`
- `RestoreMode()`
- `GetFunctionID()` (if static)
- `FinishStroke()` (if libstroke enabled)

#### 1.4 Testing
```bash
# Rebuild
make clean && make

# Run existing tests
make check

# Manual testing
./pcb --gui gtk
# Test: Mode switching, object deletion, all 16 modes
```

---

### Phase 2: Migrate ActionMode (Days 3-4)

#### 2.1 Create ModeAction.cpp

**File:** `src/actions/ModeAction.cpp`

```cpp
#include "Action.h"

extern "C" {
#include "global.h"
#include "actions/ActionContext.h"
#include "action.h"
#include "set.h"
#include "crosshair.h"

// Mode functions
void NotifyMode(void);
void ReleaseMode(void);
void SaveMode(void);
void RestoreMode(void);

#ifdef HAVE_LIBSTROKE
void FinishStroke(void);
#endif

// Helper functions
int GetFunctionID(const char* function);
void notify_crosshair_change(bool);
}

namespace pcb {
namespace actions {

class ModeAction : public Action {
public:
    ModeAction() : Action(
        "Mode",
        "Mode(Arc|Arrow|Copy|InsertPoint|Line|Lock|Move|None|PasteBuffer)\n"
        "Mode(Polygon|Rectangle|Remove|Rotate|Text|Thermal|Via)\n"
        "Mode(Notify|Release|Cancel|Stroke)\n"
        "Mode(Save|Restore)",
        "Change or use the tool mode."
    ) {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        const char* function = (argc > 0) ? argv[0] : nullptr;

        if (!function) {
            return 1;
        }

        pcb_action_context->Note.X = Crosshair.X;
        pcb_action_context->Note.Y = Crosshair.Y;

        notify_crosshair_change(false);

        int result = handle_mode_function(function);

        notify_crosshair_change(true);

        return result;
    }

private:
    int handle_mode_function(const char* function) {
        switch (GetFunctionID(function)) {
        case F_Arc:
            SetMode(ARC_MODE);
            break;
        case F_Arrow:
            SetMode(ARROW_MODE);
            break;
        case F_Copy:
            SetMode(COPY_MODE);
            break;
        case F_InsertPoint:
            SetMode(INSERTPOINT_MODE);
            break;
        case F_Line:
            SetMode(LINE_MODE);
            break;
        case F_Lock:
            SetMode(LOCK_MODE);
            break;
        case F_Move:
            SetMode(MOVE_MODE);
            break;
        case F_None:
            SetMode(NO_MODE);
            break;
        case F_Cancel:
            handle_cancel();
            break;
        case F_Escape:
            handle_escape();
            break;
        case F_Notify:
            NotifyMode();
            break;
        case F_PasteBuffer:
            SetMode(PASTEBUFFER_MODE);
            break;
        case F_Polygon:
            SetMode(POLYGON_MODE);
            break;
        case F_PolygonHole:
            SetMode(POLYGONHOLE_MODE);
            break;
#ifndef HAVE_LIBSTROKE
        case F_Release:
            ReleaseMode();
            break;
#else
        case F_Release:
            if (pcb_action_context->mid_stroke)
                FinishStroke();
            else
                ReleaseMode();
            break;
#endif
        case F_Remove:
            SetMode(REMOVE_MODE);
            break;
        case F_Rectangle:
            SetMode(RECTANGLE_MODE);
            break;
        case F_Rotate:
            SetMode(ROTATE_MODE);
            break;
        case F_Stroke:
            handle_stroke();
            break;
        case F_Text:
            SetMode(TEXT_MODE);
            break;
        case F_Thermal:
            SetMode(THERMAL_MODE);
            break;
        case F_Via:
            SetMode(VIA_MODE);
            break;
        case F_Restore:
            RestoreMode();
            break;
        case F_Save:
            SaveMode();
            break;
        default:
            return 1;
        }
        return 0;
    }

    void handle_cancel() {
        int saved_mode = Settings.Mode;
        SetMode(NO_MODE);
        SetMode(saved_mode);
    }

    void handle_escape() {
        // Extract lines 3078-3151 from ActionMode
        switch (Settings.Mode) {
        case VIA_MODE:
        case PASTEBUFFER_MODE:
        case TEXT_MODE:
        case ROTATE_MODE:
        case REMOVE_MODE:
        case MOVE_MODE:
        case COPY_MODE:
        case INSERTPOINT_MODE:
        case RUBBERBANDMOVE_MODE:
        case THERMAL_MODE:
        case LOCK_MODE:
            SetMode(NO_MODE);
            SetMode(ARROW_MODE);
            break;
        case LINE_MODE:
            if (Crosshair.AttachedLine.State == STATE_FIRST)
                SetMode(ARROW_MODE);
            else {
                SetMode(NO_MODE);
                SetMode(LINE_MODE);
            }
            break;
        // ... other mode-specific Escape logic
        }
    }

    void handle_stroke() {
#ifdef HAVE_LIBSTROKE
        pcb_action_context->mid_stroke = true;
        pcb_action_context->StrokeBox.X1 = Crosshair.X;
        pcb_action_context->StrokeBox.Y1 = Crosshair.Y;
#else
        // Extract lines 3194-3218 from ActionMode
        if (Settings.Mode == LINE_MODE &&
            Crosshair.AttachedLine.State != STATE_FIRST) {
            SetMode(LINE_MODE);
        } else if (Settings.Mode == ARC_MODE &&
                   Crosshair.AttachedBox.State != STATE_FIRST) {
            SetMode(ARC_MODE);
        }
        // ... other stroke logic
#endif
    }
};

REGISTER_ACTION(ModeAction);

} // namespace actions
} // namespace pcb
```

**Lines:** ~250

#### 2.2 Update Makefile.am
```makefile
# Add to SOURCES
libactions_la_SOURCES += \
    actions/ModeAction.cpp
```

#### 2.3 Remove ActionMode from action.c
```bash
# Comment out or delete lines 2973-3243
```

#### 2.4 Testing
```bash
# Rebuild
make

# Test all mode switches
./pcb --gui gtk
# Test: Arc, Line, Polygon, Arrow, Via, Text, etc.
# Test: Mode(Notify), Mode(Release), Mode(Cancel), Mode(Escape)
# Test: Mode(Save), Mode(Restore)
```

---

### Phase 3: Migrate ActionDelete (Day 5)

#### 3.1 Create DeleteAction.cpp

**File:** `src/actions/DeleteAction.cpp`

```cpp
#include "Action.h"

extern "C" {
#include "global.h"
#include "actions/ActionContext.h"
#include "action.h"
#include "set.h"
#include "crosshair.h"
#include "remove.h"
#include "rats.h"

void NotifyMode(void);
void SaveMode(void);
void RestoreMode(void);
int GetFunctionID(const char* function);
bool RemoveSelected(void);
bool DeleteRats(bool);
void SetChangedFlag(bool);
}

namespace pcb {
namespace actions {

class DeleteAction : public Action {
public:
    DeleteAction() : Action(
        "Delete",
        "Delete(Object|Selected)\nDelete(AllRats|SelectedRats)",
        "Delete stuff."
    ) {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        const char* function = (argc > 0) ? argv[0] : nullptr;
        int id = GetFunctionID(function);

        pcb_action_context->Note.X = Crosshair.X;
        pcb_action_context->Note.Y = Crosshair.Y;

        // If no argument and nothing selected, default to F_Object
        if (id == -1) {
            if (RemoveSelected() == false) {
                id = F_Object;
            }
        }

        switch (id) {
        case F_Object:
            SaveMode();
            SetMode(REMOVE_MODE);
            NotifyMode();
            RestoreMode();
            break;
        case F_Selected:
            RemoveSelected();
            break;
        case F_AllRats:
            if (DeleteRats(false)) {
                SetChangedFlag(true);
            }
            break;
        case F_SelectedRats:
            if (DeleteRats(true)) {
                SetChangedFlag(true);
            }
            break;
        }

        return 0;
    }
};

REGISTER_ACTION(DeleteAction);

} // namespace actions
} // namespace pcb
```

**Lines:** ~80

#### 3.2 Update Makefile.am
```makefile
# Add to SOURCES
libactions_la_SOURCES += \
    actions/DeleteAction.cpp
```

#### 3.3 Remove ActionDelete from action.c
```bash
# Comment out or delete lines 3832-3879
```

#### 3.4 Testing
```bash
# Rebuild
make

# Test deletion
./pcb --gui gtk
# Test: Delete() with no selection
# Test: Delete(Object)
# Test: Delete(Selected)
# Test: Delete(AllRats)
# Test: Delete(SelectedRats)
```

---

### Phase 4: Integration Testing (Day 6)

#### 4.1 Functional Testing Checklist

**Mode Switching:**
- [ ] Arc mode - create arcs
- [ ] Line mode - create multi-segment lines
- [ ] Polygon mode - create polygons
- [ ] Rectangle mode - create rectangles
- [ ] Via mode - place vias
- [ ] Text mode - place text
- [ ] Arrow mode (selection)
- [ ] Copy mode
- [ ] Move mode
- [ ] Rotate mode
- [ ] Remove mode
- [ ] Lock mode
- [ ] Thermal mode
- [ ] InsertPoint mode (polygon editing)

**Mode Control:**
- [ ] Mode(Notify) - click handling
- [ ] Mode(Release) - release handling
- [ ] Mode(Cancel) - cancel operation
- [ ] Mode(Escape) - escape to arrow mode
- [ ] Mode(Save) / Mode(Restore) - mode stack

**Deletion:**
- [ ] Delete() - delete selected or object at cursor
- [ ] Delete(Object) - explicit object at cursor
- [ ] Delete(Selected) - delete selection
- [ ] Delete(AllRats) - delete all rats
- [ ] Delete(SelectedRats) - delete selected rats

#### 4.2 Regression Testing
```bash
# Run existing test suite
make check

# Run with AddressSanitizer
make clean
./configure --enable-asan
make
./pcb --gui gtk
# Perform mode switches and deletions
# Check for memory leaks
```

#### 4.3 Edge Cases
- [ ] Mode switch during multi-state operation (Line STATE_SECOND)
- [ ] Delete with empty selection
- [ ] Delete with nothing at cursor
- [ ] Stroke gestures (if HAVE_LIBSTROKE)
- [ ] Mode save/restore nesting

---

## Risk Assessment

### Low Risk Areas ✅
- **ActionContext pattern** - Proven in other branch
- **Exporting functions** - Simple changes to action.h
- **ActionDelete migration** - Only 39 lines, straightforward logic
- **Compilation** - C++ wrapping C code is well-understood

### Medium Risk Areas ⚠️
- **Note struct references** - ~84 locations need updating in action.c
- **ActionMode Escape logic** - Complex nested conditionals (80 lines)
- **Libstroke integration** - Conditional compilation may have issues
- **Integration testing** - Need to test 16 modes × multiple operations

### Mitigation Strategies
1. **Incremental commits** - Commit after each phase completes
2. **Automated testing** - Run `make check` after every change
3. **Manual testing matrix** - Checklist of all mode/delete combinations
4. **Rollback plan** - Keep action.c original in git history
5. **Code review** - Review diff of action.c changes carefully

---

## Success Criteria

### Functional
- [ ] All 16 editor modes work correctly
- [ ] Mode(Notify), Mode(Release), Mode(Cancel), Mode(Escape) work
- [ ] Mode(Save) / Mode(Restore) maintain mode stack
- [ ] Delete() deletes object at cursor or selected objects
- [ ] Delete(AllRats) / Delete(SelectedRats) work correctly
- [ ] No regressions in existing actions

### Technical
- [ ] ModeAction.cpp compiles and links
- [ ] DeleteAction.cpp compiles and links
- [ ] ActionContext accessible from both C and C++
- [ ] All existing tests pass (`make check`)
- [ ] No memory leaks (AddressSanitizer clean)
- [ ] No compiler warnings

### Documentation
- [ ] ActionContext.h documented
- [ ] ModeAction.cpp commented
- [ ] DeleteAction.cpp commented
- [ ] Migration progress updated (100% complete)

---

## Future Work

### NotifyMode Refactoring (Deferred)

Once ActionMode and ActionDelete are in C++, the NotifyMode function remains as the largest piece of technical debt in the codebase.

**Future Milestone: State Pattern Refactoring**
- **Effort:** 4-6 weeks
- **Priority:** Medium
- **Approach:** Option 1 (Full State Pattern)
- **Benefits:** Clean architecture, testability, extensibility
- **Plan:** See separate document `NOTIFYMODE_REFACTORING_PLAN.md`

**Trigger Conditions for NotifyMode Refactoring:**
1. Adding new editor modes becomes necessary
2. NotifyMode bugs become frequent
3. UI framework migration (GTK3 → GTK4) requires mode system changes
4. Team has bandwidth for 4-6 week architectural project

---

## Conclusion

The **Hybrid Strategy (Option 2)** provides the best balance of effort, risk, and value:

- **6 days effort** vs 28-30 days for full State Pattern refactoring
- **Low risk** - NotifyMode stays in C, proven approach from other branch
- **Complete migration** - ActionMode and ActionDelete in C++, 100% action migration
- **Foundation** - ActionContext sets up infrastructure for future work

**Recommendation:** Proceed with Option 2 implementation plan.

---

**Next Steps:**
1. Review and approve this plan
2. Begin Phase 1 (Infrastructure setup)
3. Migrate ActionMode (Phase 2)
4. Migrate ActionDelete (Phase 3)
5. Comprehensive testing (Phase 4)

**Estimated Completion:** 6 days (1 sprint)

---

**Document Version:** 1.0
**Last Updated:** November 19, 2025
**Author:** Claude Code Agent
**Status:** Awaiting Approval
