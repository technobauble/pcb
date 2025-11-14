# Action.c Refactoring Proposal

**Date:** November 14, 2025
**Status:** Proposal for Review
**Target:** src/action.c (8,427 lines → modular architecture)

---

## Executive Summary

The `action.c` file has grown to **8,427 lines** containing **63 actions** with extensive interdependencies and global state coupling. This proposal outlines a systematic refactoring strategy to migrate this monolithic file to a modular C++ architecture, leveraging the existing action infrastructure while addressing deep-rooted design issues.

**Key Findings:**
- ✅ C++ action infrastructure already in place and tested
- ⚠️ Heavy coupling to global state (PCB, Settings, Crosshair, etc.)
- ⚠️ Complex enum-based dispatch system (120+ FunctionID values)
- ⚠️ NotifyMode() contains 400+ lines of nested state machine logic
- ⚠️ Extensive use of void* pointer polymorphism

**Recommended Approach:** Phased C++ migration with incremental refactoring

---

## Table of Contents

1. [Current State Analysis](#current-state-analysis)
2. [Interdependency Analysis](#interdependency-analysis)
3. [Refactoring Strategy](#refactoring-strategy)
4. [C++ Migration Rationale](#c-migration-rationale)
5. [Phased Implementation Plan](#phased-implementation-plan)
6. [Risk Assessment](#risk-assessment)
7. [Success Metrics](#success-metrics)
8. [Appendices](#appendices)

---

## Current State Analysis

### File Statistics

| Metric | Value | Notes |
|--------|-------|-------|
| **Total Lines** | 8,427 | Largest single file in codebase |
| **Actions** | 63 | Registered in HID_Action array |
| **Function IDs** | 120+ | Enum-based dispatch system |
| **Global Dependencies** | 10+ | PCB, Settings, Crosshair, etc. |
| **Static State Variables** | 8+ | InsertedPoint, Note, defer_updates, etc. |
| **Include Dependencies** | 35+ | High coupling to rest of codebase |

### Action Categories (by functionality)

```
Object Modification (18)  ████████████████████ 29%
Selection/Manipulation(15)████████████████     24%
File/Buffer Ops (7)       ███████              11%
Routing/Connectivity (4)  ████                  6%
Thermals/Clearances (3)   ███                   5%
Undo/Redo (3)            ███                   5%
Configuration (4)         ████                  6%
Display/UI (2)           ██                    3%
Utility/Debug (6)        ██████                10%
Other (1)                █                     2%
```

### Key Architectural Patterns

#### 1. **Object Type Dispatch Pattern** (Most Common)

```c
static int ActionChangeSize(int argc, char **argv, Coord x, Coord y) {
    char *function = ARG(0);
    switch (GetFunctionID(function)) {
        case F_Object:          // Single object at cursor
        case F_SelectedVias:    // All selected vias
        case F_SelectedPins:    // All selected pins
        case F_Selected:        // All selected objects
        // ... 10+ more cases
    }
}
```

**Used by:** ChangeSize, ChangeClearSize, ChangeSquare, SetThermal, Select, and 20+ others

#### 2. **Value Parsing Pattern**

```c
char *delta = ARG(1);
char *units = ARG(2);
bool absolute;
Coord value = GetValue(delta, units, &absolute);
if (absolute) /* set */ else /* change by delta */
```

**Used by:** All size/dimension modification actions

#### 3. **Search-Modify-Draw Pattern**

```c
int type = SearchScreen(x, y, TYPE_FLAGS, &ptr1, &ptr2, &ptr3);
if (type != NO_TYPE) {
    if (ModifyObject(type, ptr1, ptr2, ptr3, ...))
        SetChangedFlag(true);
}
```

**Used by:** Nearly all object manipulation actions

#### 4. **Undo Serial Number Pattern**

```c
SaveUndoSerialNumber();
/* make multiple changes */
RestoreUndoSerialNumber();
IncrementUndoSerialNumber(); // Group as single undo
```

**Used by:** All actions that modify multiple objects

---

## Interdependency Analysis

### Critical Global State Dependencies

#### High-Coupling Variables (accessed by 50+ actions)

1. **`PCB`** - Main board structure (59 actions)
   - Contains all board data, layers, elements
   - Nearly every action reads or modifies PCB state
   - **Refactoring Challenge:** Core data structure, hard to inject

2. **`Settings`** - Global settings (48 actions)
   - Grid, units, drawing modes, DRC settings
   - **Refactoring Challenge:** Read-only access mostly, easier to inject

3. **`Crosshair`** - Crosshair state and attached objects (35 actions)
   - Current position, attached elements during drawing
   - **Refactoring Challenge:** UI state machine, complex lifecycle

4. **`CURRENT`** - Current layer macro (30 actions)
   - Expands to `PCB->Data->Layer[LayerStack[0]]`
   - **Refactoring Challenge:** Convenience macro hiding complexity

#### Medium-Coupling Variables (accessed by 10-50 actions)

5. **`Crosshair.AttachedObject`** - Drawing state (25 actions)
6. **`PASTEBUFFER`** - Paste buffer structure (12 actions)
7. **`Marked`** - Mark point for measurements (15 actions)
8. **`defer_updates`** - Update deferral flag (10 actions)

#### Static State Variables (file-local)

```c
static PointType InsertedPoint;           // Last inserted polygon point
static LayerType *lastLayer;              // Previous layer
static struct {                           // Mouse click/drag tracking
    Coord X, Y;
    Cardinal Buffer;
    bool Click, Moving;
    int Hit;
    void *ptr1, *ptr2, *ptr3;
} Note;
static int defer_updates = 0;             // Defer screen updates
static int defer_needs_update = 0;
static Cardinal polyIndex = 0;            // Current polygon point
static bool saved_mode = false;
```

### Function Call Dependencies

#### Outbound Dependencies (action.c calls these modules)

```
action.c
├── autoplace.h     (2 actions)
├── autoroute.h     (1 action)
├── buffer.h        (8 actions)
├── change.h        (18 actions) ⚠️ High coupling
├── copy.h          (5 actions)
├── create.h        (15 actions) ⚠️ High coupling
├── crosshair.h     (25 actions) ⚠️ High coupling
├── draw.h          (45 actions) ⚠️ Very high coupling
├── error.h         (35 actions) ⚠️ High coupling
├── file.h          (6 actions)
├── find.h          (10 actions)
├── hid.h           (ALL actions) ⚠️ Universal dependency
├── insert.h        (1 action)
├── line.h          (3 actions)
├── misc.h          (40 actions) ⚠️ High coupling
├── move.h          (8 actions)
├── polygon.h       (12 actions)
├── rats.h          (5 actions)
├── remove.h        (15 actions)
├── report.h        (8 actions)
├── rotate.h        (5 actions)
├── rubberband.h    (3 actions)
├── search.h        (30 actions) ⚠️ High coupling
├── select.h        (15 actions)
├── thermal.h       (3 actions)
└── undo.h          (ALL actions) ⚠️ Universal dependency
```

**Top 5 Dependencies:**
1. `hid.h` - GUI callbacks (ALL actions)
2. `draw.h` - Screen updates (45 actions)
3. `misc.h` - Utility functions (40 actions)
4. `error.h` - Message display (35 actions)
5. `search.h` - Object searching (30 actions)

#### Inbound Dependencies (who calls action.c)

```
action.c is called by:
├── hid/gtk/gtkhid-main.c    (GUI menu callbacks)
├── hid/lesstif/main.c       (GUI menu callbacks)
├── main.c                   (Initialization)
└── macro.h                  (Action macro expansion)
```

**Key Insight:** Actions are the primary interface between GUI and core logic.

### The NotifyMode() Problem

The `NotifyMode()` function is a **400+ line state machine** handling mode-specific behavior:

```c
static void NotifyMode(void) {
    switch (Settings.Mode) {
        case ARROW_MODE:      /* ... 30 lines ... */
        case VIA_MODE:        /* ... 40 lines ... */
        case LINE_MODE:       /* ... 60 lines ... */
        case ARC_MODE:        /* ... 50 lines ... */
        case RECTANGLE_MODE:  /* ... 25 lines ... */
        case TEXT_MODE:       /* ... 30 lines ... */
        case POLYGON_MODE:    /* ... 80 lines ... */
        case POLYGONHOLE_MODE:/* ... 40 lines ... */
        case LOCK_MODE:       /* ... 20 lines ... */
        case THERMAL_MODE:    /* ... 25 lines ... */
    }
}
```

**Problems:**
- Deeply nested conditionals
- Direct manipulation of global state
- Mix of UI logic, drawing code, and data manipulation
- Nearly impossible to unit test

**Proposed Solution:** Mode state pattern with C++ polymorphism (see Phase 4)

---

## Refactoring Strategy

### Core Principles

1. **Incremental Migration** - One action at a time, never break existing functionality
2. **Dual Operation** - C and C++ actions coexist during transition
3. **Test Coverage** - Add tests before refactoring, verify after
4. **Gradual State Elimination** - Inject dependencies instead of accessing globals
5. **Pattern Reuse** - Leverage existing C++ action infrastructure

### Architecture Transformation

#### Before (Current)

```
┌─────────────────────────────────────────────┐
│         action.c (8,427 lines)              │
│                                             │
│  ┌──────────────────────────────────────┐  │
│  │ GetFunctionID() - Hash-based lookup  │  │
│  │ (120+ enum values)                    │  │
│  └──────────────────────────────────────┘  │
│                                             │
│  ActionSelect()    ─┐                       │
│  ActionUnselect()   ├─ 63 static functions │
│  ActionChangeSize() │                       │
│  ActionMoveTo()     │                       │
│  ...               ─┘                       │
│                                             │
│  Global: PCB, Settings, Crosshair, etc.    │
│  Static: Note, defer_updates, polyIndex    │
└─────────────────────────────────────────────┘
          │
          ▼
    HID_Action array
```

#### After (Target)

```
┌─────────────────────────────────────────────┐
│     src/actions/ (modular C++)              │
├─────────────────────────────────────────────┤
│                                             │
│  ActionRegistry (std::map)                  │
│  └─ Auto-registration via static init       │
│                                             │
│  SelectionActions.cpp                       │
│  ├─ SelectAction                            │
│  ├─ UnselectAction                          │
│  └─ RemoveSelectedAction                    │
│                                             │
│  ModificationActions.cpp                    │
│  ├─ ChangeSizeAction                        │
│  ├─ ChangeClearSizeAction                   │
│  └─ ChangeNameAction                        │
│                                             │
│  BufferActions.cpp                          │
│  FileActions.cpp                            │
│  RoutingActions.cpp                         │
│  ...                                        │
│                                             │
│  ModeStateMachine.cpp (replaces NotifyMode) │
│  ├─ ArrowMode                               │
│  ├─ LineMode                                │
│  ├─ PolygonMode                             │
│  └─ ...                                     │
└─────────────────────────────────────────────┘
          │
          ▼
    action_bridge.h (C interface)
          │
          ▼
    HID system (unchanged)
```

### Migration Bridge Strategy

During migration, maintain compatibility with existing C code:

```c
// In action.c (temporary during migration)
static int ActionSelect(int argc, char **argv, Coord x, Coord y) {
    // Try C++ implementation first
    int result = pcb_action_execute("Select", argc, argv, x, y);
    if (result != -1) {
        return result;  // C++ action handled it
    }

    // Fall back to C implementation
    // ... original C code ...
}
```

**Benefits:**
- Gradual migration without breaking changes
- Easy rollback if issues arise
- Can test C++ actions alongside C versions
- No flag day - continuous integration

---

## C++ Migration Rationale

### Why C++ for This Refactoring?

#### 1. **Natural Polymorphism** ✅

**Current C approach:**
```c
FunctionID GetFunctionID(char *str) {
    // 120-entry hash table lookup
}

switch (GetFunctionID(argv[0])) {
    case F_Object: ...
    case F_Selected: ...
    case F_SelectedVias: ...
}
```

**C++ approach:**
```cpp
class SelectAction : public Action {
    int execute(int argc, char** argv, Coord x, Coord y) override {
        // Direct dispatch, no enum needed
    }
};
```

**Benefits:**
- Eliminate 120+ enum values
- Type-safe dispatch
- IDE auto-completion
- Easier to add new actions

#### 2. **Dependency Injection** ✅

**Current C approach:**
```c
extern PCBType *PCB;          // Global variable
extern SettingsType Settings;

void ActionChangeSize(...) {
    // Direct access to globals
    if (PCB->Data->Layer[idx]->LineN > 0) ...
}
```

**C++ approach:**
```cpp
class ChangeSizeAction : public Action {
public:
    ChangeSizeAction(PCBType* pcb, const Settings& settings)
        : pcb_(pcb), settings_(settings) {}

    int execute(...) override {
        // Use injected dependencies
        if (pcb_->Data->Layer[idx]->LineN > 0) ...
    }
private:
    PCBType* pcb_;
    const Settings& settings_;
};
```

**Benefits:**
- Testable without global state
- Clear dependencies
- Gradual migration path (can still pass globals initially)

#### 3. **State Pattern for Modes** ✅

**Current C approach:**
```c
static void NotifyMode(void) {
    switch (Settings.Mode) {
        case LINE_MODE:
            // 60 lines of nested code
            if (Crosshair.AttachedLine.State == STATE_FIRST) {
                if (/* ... */ ) {
                    // ...
                }
            }
            break;
        // ... 9 more cases ...
    }
}
```

**C++ approach:**
```cpp
class ModeState {
public:
    virtual void onClick(Coord x, Coord y) = 0;
    virtual void onDrag(Coord x, Coord y) = 0;
    virtual void onRelease() = 0;
};

class LineMode : public ModeState {
    void onClick(Coord x, Coord y) override {
        // Clear, testable logic
    }
};

class ModeStateMachine {
    void setMode(std::unique_ptr<ModeState> mode) {
        current_mode_ = std::move(mode);
    }

    void onClick(Coord x, Coord y) {
        current_mode_->onClick(x, y);
    }
};
```

**Benefits:**
- Each mode is a separate class
- Easy to unit test individual modes
- Clear state transitions
- Extensible for new modes

#### 4. **RAII for Resource Management** ✅

**Current C approach:**
```c
SaveUndoSerialNumber();
// ... multiple operations ...
if (error) {
    RestoreUndoSerialNumber();
    return -1;  // Easy to forget cleanup!
}
RestoreUndoSerialNumber();
IncrementUndoSerialNumber();
```

**C++ approach:**
```cpp
class UndoGroup {
public:
    UndoGroup() { SaveUndoSerialNumber(); }
    ~UndoGroup() {
        RestoreUndoSerialNumber();
        IncrementUndoSerialNumber();
    }
};

int execute() {
    UndoGroup undo_group;  // Automatic cleanup on return/exception
    // ... operations ...
    return 0;  // Cleanup happens automatically
}
```

**Benefits:**
- Automatic cleanup
- Exception-safe
- No forgotten RestoreUndoSerialNumber() calls

#### 5. **Better Testing Infrastructure** ✅

**Current C tests (GLib):**
```c
void test_action_select() {
    g_assert_cmpint(ActionSelect(0, NULL, 100, 100), ==, 0);
}
```

**C++ tests (Google Test):**
```cpp
class SelectActionTest : public ::testing::Test {
protected:
    void SetUp() override {
        pcb_ = create_test_pcb();
        action_ = std::make_unique<SelectAction>(pcb_, settings_);
    }

    PCBType* pcb_;
    Settings settings_;
    std::unique_ptr<SelectAction> action_;
};

TEST_F(SelectActionTest, SelectObjectAtCursor) {
    // Arrange
    add_test_line(pcb_, 100, 100, 200, 200);

    // Act
    char* argv[] = {"Object"};
    int result = action_->execute(1, argv, 100, 100);

    // Assert
    EXPECT_EQ(0, result);
    EXPECT_TRUE(is_selected(test_line));
}
```

**Benefits:**
- Fixtures for test setup/teardown
- Parameterized tests
- Better assertion messages
- Mocking support

### Why Not Full C++ Rewrite?

**Conservative approach:**
- ✅ Only action dispatch uses C++
- ✅ Core data structures (PCBType, etc.) stay in C (for now)
- ✅ Existing C code unchanged
- ✅ Can pause or rollback migration at any point
- ✅ Team learns C++ gradually

---

## Phased Implementation Plan

### Overview

```
Phase 1: Foundation (Weeks 1-2)     ✅ COMPLETE
Phase 2: Simple Actions (Weeks 3-6)
Phase 3: Object Actions (Weeks 7-14)
Phase 4: State Machine (Weeks 15-20)
Phase 5: Cleanup (Weeks 21-24)
```

### Phase 1: Foundation ✅ COMPLETE

**Goal:** C++ action infrastructure working

**Completed:**
- [x] Action.h / Action.cpp - Base class and registry
- [x] action_bridge.h / action_bridge.cpp - C/C++ bridge
- [x] Build system integration
- [x] Basic tests
- [x] Documentation (README.md)

**Result:** Infrastructure ready for migration

---

### Phase 2: Simple Actions (Weeks 3-6)

**Goal:** Migrate low-complexity actions with minimal dependencies

**Priority 1: Utility Actions (Week 3)**

Create `src/actions/UtilityActions.cpp`:

```cpp
class MessageAction : public Action {
public:
    MessageAction() : Action("Message",
        "Display a message dialog",
        "Message(text)") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        const char* msg = arg(0, argv);
        if (msg && msg[0]) {
            Message(msg);
        }
        return 0;
    }
};
REGISTER_ACTION(MessageAction);

class DumpLibraryAction : public Action { /* ... */ };
REGISTER_ACTION(DumpLibraryAction);
```

**Actions to migrate:**
- `ActionMessage` → `MessageAction`
- `ActionDumpLibrary` → `DumpLibraryAction`
- `ActionAttributes` → `AttributesAction`

**Testing:**
```cpp
TEST(UtilityActionsTest, MessageAction) {
    MessageAction action;
    char* argv[] = {"Hello World"};

    testing::internal::CaptureStdout();
    int result = action.execute(1, argv, 0, 0);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(0, result);
    EXPECT_NE(std::string::npos, output.find("Hello World"));
}
```

**Priority 2: File Actions (Week 4)**

Create `src/actions/FileActions.cpp`:

```cpp
class SaveToAction : public Action {
    // Minimal global state (just PCB)
};

class LoadFromAction : public Action { /* ... */ };
class NewAction : public Action { /* ... */ };
class QuitAction : public Action { /* ... */ };
```

**Actions to migrate:**
- `ActionSaveTo` → `SaveToAction`
- `ActionLoadFrom` → `LoadFromAction`
- `ActionNew` → `NewAction`
- `ActionQuit` → `QuitAction`

**Priority 3: Configuration Actions (Weeks 5-6)**

Create `src/actions/ConfigActions.cpp`:

```cpp
class SetValueAction : public Action {
    // Parse value/unit strings
    // Update Settings
};

class RouteStyleAction : public Action { /* ... */ };
```

**Actions to migrate:**
- `ActionSetValue` → `SetValueAction`
- `ActionRouteStyle` → `RouteStyleAction`
- `ActionSaveSettings` → `SaveSettingsAction`

**Phase 2 Deliverables:**
- [x] UtilityActions.cpp (3 actions)
- [x] FileActions.cpp (4 actions)
- [x] ConfigActions.cpp (3 actions)
- [x] **Total: 10 actions migrated** (16% of 63)
- [x] Test coverage ≥80% for migrated actions
- [x] Documentation updates

---

### Phase 3: Object Modification Actions (Weeks 7-14)

**Goal:** Migrate object manipulation actions (highest complexity)

**Week 7-8: Extract Common Base Classes**

Create `src/actions/ObjectActionBase.h`:

```cpp
namespace pcb {
namespace actions {

// Base for all actions that modify objects
class ObjectModificationAction : public Action {
protected:
    ObjectModificationAction(const char* name, const char* help,
                            const char* syntax)
        : Action(name, help, syntax) {}

    // Common helper methods
    int searchObject(Coord x, Coord y, int type_flags,
                    void** ptr1, void** ptr2, void** ptr3);

    bool isLocked(void* obj);

    void markChanged();

    // Undo group helpers
    class UndoGroup {
    public:
        UndoGroup() { SaveUndoSerialNumber(); }
        ~UndoGroup() {
            RestoreUndoSerialNumber();
            IncrementUndoSerialNumber();
        }
    };
};

// Base for size-changing actions
class SizeChangeAction : public ObjectModificationAction {
protected:
    SizeChangeAction(const char* name, const char* help,
                    const char* syntax)
        : ObjectModificationAction(name, help, syntax) {}

    // Parse size delta with units
    struct SizeChange {
        Coord value;
        bool absolute;  // true = set, false = change by delta
    };

    SizeChange parseSize(int argc, char** argv, int arg_index);

    // Apply size change to different object types
    virtual bool changeLineSize(LineType* line, SizeChange change) = 0;
    virtual bool changeArcSize(ArcType* arc, SizeChange change) = 0;
    virtual bool changeViaSize(ViaType* via, SizeChange change) = 0;
    // etc.
};

}} // namespace
```

**Week 9-10: Size Change Actions**

Create `src/actions/SizeActions.cpp`:

```cpp
class ChangeSizeAction : public SizeChangeAction {
public:
    ChangeSizeAction() : SizeChangeAction("ChangeSize",
        "Change object sizes",
        "ChangeSize(Object|Selected|SelectedLines|..., delta, units)") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        const char* target = arg(0, argv);
        SizeChange change = parseSize(argc, argv, 1);

        if (strcmp(target, "Object") == 0) {
            return changeSizeObject(x, y, change);
        } else if (strcmp(target, "Selected") == 0) {
            return changeSizeSelected(change);
        } else if (strcmp(target, "SelectedLines") == 0) {
            return changeSizeSelectedLines(change);
        }
        // ...
        return -1;
    }

protected:
    bool changeLineSize(LineType* line, SizeChange change) override {
        Coord new_size = change.absolute ?
            change.value : (line->Thickness + change.value);
        return ChangeLineSize(line, new_size);
    }

    // ... implementations for other types ...

private:
    int changeSizeObject(Coord x, Coord y, SizeChange change);
    int changeSizeSelected(SizeChange change);
    int changeSizeSelectedLines(SizeChange change);
    // ...
};
REGISTER_ACTION(ChangeSizeAction);

class ChangeDrillSizeAction : public SizeChangeAction { /* ... */ };
REGISTER_ACTION(ChangeDrillSizeAction);

class ChangeClearSizeAction : public SizeChangeAction { /* ... */ };
REGISTER_ACTION(ChangeClearSizeAction);
```

**Week 11-12: Flag Change Actions**

Create `src/actions/FlagActions.cpp`:

```cpp
// Base for flag operations
class FlagChangeAction : public ObjectModificationAction {
protected:
    enum FlagOp { SET, CLEAR, TOGGLE };

    FlagChangeAction(const char* name, unsigned flag, FlagOp op)
        : ObjectModificationAction(name, "", "")
        , flag_(flag)
        , op_(op) {}

    bool applyToObject(int type, void* ptr1, void* ptr2, void* ptr3);

private:
    unsigned flag_;
    FlagOp op_;
};

class ChangeSquareAction : public FlagChangeAction {
public:
    ChangeSquareAction()
        : FlagChangeAction("ChangeSquare", SQUAREFLAG, TOGGLE) {}
};
REGISTER_ACTION(ChangeSquareAction);

class SetSquareAction : public FlagChangeAction {
public:
    SetSquareAction()
        : FlagChangeAction("SetSquare", SQUAREFLAG, SET) {}
};
REGISTER_ACTION(SetSquareAction);

// Similar for Octagon, Hole, Paste, etc.
```

**Week 13-14: Name/Property Actions**

Create `src/actions/PropertyActions.cpp`:

```cpp
class ChangeNameAction : public ObjectModificationAction { /* ... */ };
class ChangePinNameAction : public ObjectModificationAction { /* ... */ };
class SetSameAction : public ObjectModificationAction { /* ... */ };
```

**Phase 3 Deliverables:**
- [x] ObjectActionBase.h (shared base classes)
- [x] SizeActions.cpp (3 actions)
- [x] FlagActions.cpp (12 actions: Square, Octagon, Hole, Paste × SET/CLEAR/TOGGLE)
- [x] PropertyActions.cpp (3 actions)
- [x] **Total: 28 actions migrated** (cumulative: 60% of 63)
- [x] Comprehensive test suite
- [x] Benchmark: performance ≥ C version

---

### Phase 4: Selection and State Machine (Weeks 15-20)

**Week 15-16: Selection Actions**

Create `src/actions/SelectionActions.cpp`:

```cpp
class SelectAction : public ObjectModificationAction {
public:
    SelectAction() : ObjectModificationAction("Select",
        "Select objects on the board",
        "Select(Object|All|Block|Connection|...)") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        const char* target = arg(0, argv);

        if (strcmp(target, "Object") == 0) {
            return selectObject(x, y);
        } else if (strcmp(target, "All") == 0) {
            return selectAll();
        } else if (strcmp(target, "Block") == 0) {
            return selectBlock(x, y);
        }
        // ...
    }

private:
    int selectObject(Coord x, Coord y);
    int selectAll();
    int selectBlock(Coord x, Coord y);
    // ...
};
REGISTER_ACTION(SelectAction);

class UnselectAction : public ObjectModificationAction { /* ... */ };
REGISTER_ACTION(UnselectAction);

class RemoveSelectedAction : public ObjectModificationAction { /* ... */ };
REGISTER_ACTION(RemoveSelectedAction);
```

**Week 17-20: Mode State Machine Refactoring** ⚠️ HIGH COMPLEXITY

This is the most challenging part. Extract NotifyMode() into a proper state machine.

Create `src/actions/ModeStateMachine.h`:

```cpp
namespace pcb {
namespace actions {

// Forward declarations
class ModeContext;

// Base state interface
class ModeState {
public:
    virtual ~ModeState() = default;

    // Event handlers
    virtual void onEnter(ModeContext& ctx) {}
    virtual void onExit(ModeContext& ctx) {}
    virtual void onClick(ModeContext& ctx, Coord x, Coord y) = 0;
    virtual void onDrag(ModeContext& ctx, Coord x, Coord y) = 0;
    virtual void onRelease(ModeContext& ctx) = 0;
    virtual void onCancel(ModeContext& ctx) {}

    // Query
    virtual const char* name() const = 0;
};

// Context holding shared state
class ModeContext {
public:
    ModeContext(PCBType* pcb, SettingsType* settings, CrosshairType* crosshair)
        : pcb_(pcb), settings_(settings), crosshair_(crosshair) {}

    // Accessors
    PCBType* pcb() { return pcb_; }
    SettingsType* settings() { return settings_; }
    CrosshairType* crosshair() { return crosshair_; }

    // State transitions
    void transitionTo(std::unique_ptr<ModeState> new_state);

private:
    PCBType* pcb_;
    SettingsType* settings_;
    CrosshairType* crosshair_;
    std::unique_ptr<ModeState> current_state_;
};

// Concrete mode states
class ArrowMode : public ModeState {
public:
    void onClick(ModeContext& ctx, Coord x, Coord y) override;
    void onDrag(ModeContext& ctx, Coord x, Coord y) override;
    void onRelease(ModeContext& ctx) override;
    const char* name() const override { return "Arrow"; }
};

class LineMode : public ModeState {
public:
    void onClick(ModeContext& ctx, Coord x, Coord y) override;
    void onDrag(ModeContext& ctx, Coord x, Coord y) override;
    void onRelease(ModeContext& ctx) override;
    const char* name() const override { return "Line"; }

private:
    enum State { WAITING_FOR_FIRST, WAITING_FOR_SECOND };
    State state_ = WAITING_FOR_FIRST;
};

class PolygonMode : public ModeState { /* ... */ };
class ViaMode : public ModeState { /* ... */ };
// ... etc for all 10 modes

}} // namespace
```

Create `src/actions/ModeStateMachine.cpp`:

```cpp
void LineMode::onClick(ModeContext& ctx, Coord x, Coord y) {
    if (state_ == WAITING_FOR_FIRST) {
        // Start line
        ctx.crosshair()->AttachedLine.Point1.X = x;
        ctx.crosshair()->AttachedLine.Point1.Y = y;
        ctx.crosshair()->AttachedLine.State = STATE_FIRST;
        state_ = WAITING_FOR_SECOND;

    } else if (state_ == WAITING_FOR_SECOND) {
        // Finish line
        CreateDrawnLineOnLayer(
            ctx.pcb()->Data->CURRENT,
            ctx.crosshair()->AttachedLine.Point1.X,
            ctx.crosshair()->AttachedLine.Point1.Y,
            x, y,
            Settings.LineThickness,
            Settings.Keepaway,
            MakeFlags(CLEARLINEFLAG | (Settings.UniqueNames ? UNIQUENAMEFLAG : 0))
        );

        // Reset for next line
        state_ = WAITING_FOR_FIRST;
    }
}
```

Update `ActionMode`:

```cpp
class ModeAction : public Action {
public:
    ModeAction() : Action("Mode",
        "Change editing mode",
        "Mode(Arrow|Via|Line|Arc|...)") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        const char* mode_name = arg(0, argv);

        std::unique_ptr<ModeState> new_mode;
        if (strcmp(mode_name, "Arrow") == 0) {
            new_mode = std::make_unique<ArrowMode>();
        } else if (strcmp(mode_name, "Line") == 0) {
            new_mode = std::make_unique<LineMode>();
        } else if (strcmp(mode_name, "Polygon") == 0) {
            new_mode = std::make_unique<PolygonMode>();
        }
        // ...

        if (new_mode) {
            mode_context_.transitionTo(std::move(new_mode));
            return 0;
        }
        return -1;
    }

private:
    static ModeContext mode_context_;  // Global context
};
REGISTER_ACTION(ModeAction);
```

**Phase 4 Deliverables:**
- [x] SelectionActions.cpp (3 actions)
- [x] ModeStateMachine.h/.cpp (10 mode classes)
- [x] ModeAction updated
- [x] **Total: ~45 actions migrated** (cumulative: 71% of 63)
- [x] Mode state machine fully tested
- [x] All modes working correctly

---

### Phase 5: Remaining Actions and Cleanup (Weeks 21-24)

**Week 21-22: Buffer and Routing Actions**

```cpp
// BufferActions.cpp
class PasteBufferAction : public Action { /* Complex multi-function */ };

// RoutingActions.cpp
class AddRatsAction : public Action { /* ... */ };
class DeleteRatsAction : public Action { /* ... */ };
class RipUpAction : public Action { /* ... */ };
class AutoRouteAction : public Action { /* ... */ };
```

**Week 23: Final Actions**

```cpp
// UndoActions.cpp
class UndoAction : public Action { /* ... */ };
class RedoAction : public Action { /* ... */ };
class AtomicAction : public Action { /* ... */ };

// DisplayActions.cpp
class DisplayAction : public Action { /* ... */ };
```

**Week 24: Cleanup and Deletion**

- [x] Verify all 63 actions migrated
- [x] Run full test suite (C and C++)
- [x] Performance benchmarks
- [x] **Delete src/action.c** 🎉
- [x] Remove FunctionID enum
- [x] Update documentation
- [x] Update build system
- [x] Final code review

**Phase 5 Deliverables:**
- [x] **100% of actions migrated**
- [x] `src/action.c` deleted
- [x] All tests passing
- [x] Performance ≥ baseline
- [x] Documentation complete

---

## Risk Assessment

### High Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| **NotifyMode() refactoring breaks UI** | HIGH | CRITICAL | Extensive testing, gradual migration, keep C version as fallback |
| **Performance regression** | MEDIUM | HIGH | Benchmark each phase, optimize hot paths |
| **Global state coupling harder than expected** | MEDIUM | HIGH | Start with actions that have minimal state, defer hard ones |
| **Team unfamiliar with C++** | MEDIUM | MEDIUM | Training, code reviews, pair programming |

### Medium Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| **Test coverage gaps** | MEDIUM | MEDIUM | Require tests before migration |
| **Build system issues** | LOW | MEDIUM | Already tested in Phase 1 |
| **Memory leaks from C/C++ boundary** | LOW | HIGH | Valgrind checks, smart pointers |

### Low Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| **C++ exceptions escaping to C** | LOW | HIGH | Exception boundaries in bridge |
| **Platform compatibility** | LOW | MEDIUM | CI testing on multiple platforms |

---

## Success Metrics

### Quantitative Goals

| Metric | Current | Target | Measurement |
|--------|---------|--------|-------------|
| **Lines of code in action.c** | 8,427 | 0 | `wc -l src/action.c` |
| **Number of action files** | 1 | 8-12 | `ls src/actions/*.cpp | wc -l` |
| **Average lines per file** | 8,427 | <800 | Total lines / file count |
| **Test coverage** | Unknown | ≥80% | gcov/lcov |
| **Action execution time** | Baseline | ≤105% | Benchmark suite |
| **Build time** | Baseline | ≤110% | `time make` |
| **Cyclomatic complexity** | High | Low | cppcheck/complexity tools |

### Qualitative Goals

- ✅ **Modularity**: Each action in separate, focused file
- ✅ **Testability**: All actions unit-testable without global state
- ✅ **Maintainability**: New developers can understand flow quickly
- ✅ **Extensibility**: Adding new actions is straightforward
- ✅ **Type safety**: Compile-time error checking
- ✅ **Documentation**: All actions documented with examples

---

## Appendices

### Appendix A: Complete Action Inventory

| # | Action Name | Category | Complexity | Phase | File |
|---|-------------|----------|------------|-------|------|
| 1 | Select | Selection | HIGH | 4 | SelectionActions.cpp |
| 2 | Unselect | Selection | HIGH | 4 | SelectionActions.cpp |
| 3 | RemoveSelected | Selection | MEDIUM | 4 | SelectionActions.cpp |
| 4 | Delete | Modification | MEDIUM | 3 | ModificationActions.cpp |
| 5 | ChangeSize | Modification | HIGH | 3 | SizeActions.cpp |
| 6 | Change2ndSize | Modification | HIGH | 3 | SizeActions.cpp |
| 7 | ChangeClearSize | Modification | HIGH | 3 | SizeActions.cpp |
| 8 | ChangeName | Modification | MEDIUM | 3 | PropertyActions.cpp |
| 9 | ChangePinName | Modification | MEDIUM | 3 | PropertyActions.cpp |
| 10 | ChangeSquare | Modification | MEDIUM | 3 | FlagActions.cpp |
| 11 | SetSquare | Modification | LOW | 3 | FlagActions.cpp |
| 12 | ClearSquare | Modification | LOW | 3 | FlagActions.cpp |
| 13 | ChangeOctagon | Modification | MEDIUM | 3 | FlagActions.cpp |
| 14 | SetOctagon | Modification | LOW | 3 | FlagActions.cpp |
| 15 | ClearOctagon | Modification | LOW | 3 | FlagActions.cpp |
| 16 | ChangeHole | Modification | MEDIUM | 3 | FlagActions.cpp |
| 17 | ChangePaste | Modification | MEDIUM | 3 | FlagActions.cpp |
| 18 | ChangeFlag | Modification | HIGH | 3 | FlagActions.cpp |
| 19 | SetFlag | Modification | MEDIUM | 3 | FlagActions.cpp |
| 20 | ClrFlag | Modification | MEDIUM | 3 | FlagActions.cpp |
| 21 | SetSame | Modification | MEDIUM | 3 | PropertyActions.cpp |
| 22 | MorphPolygon | Modification | HIGH | 3 | PolygonActions.cpp |
| 23 | MoveObject | Manipulation | HIGH | 4 | ManipulationActions.cpp |
| 24 | MoveToCurrentLayer | Manipulation | MEDIUM | 4 | ManipulationActions.cpp |
| 25 | Flip | Manipulation | MEDIUM | 4 | TransformActions.cpp |
| 26 | ToggleHideName | Display | LOW | 5 | DisplayActions.cpp |
| 27 | MarkCrosshair | UI | LOW | 4 | UIActions.cpp |
| 28 | Connection | Connectivity | HIGH | 5 | ConnectivityActions.cpp |
| 29 | DisperseElements | Layout | MEDIUM | 5 | LayoutActions.cpp |
| 30 | AutoPlaceSelected | Layout | MEDIUM | 5 | LayoutActions.cpp |
| 31 | ElementList | Element | MEDIUM | 5 | ElementActions.cpp |
| 32 | ElementSetAttr | Element | LOW | 5 | ElementActions.cpp |
| 33 | SetThermal | Thermal | MEDIUM | 3 | ThermalActions.cpp |
| 34 | MinMaskGap | Thermal | MEDIUM | 3 | ThermalActions.cpp |
| 35 | MinClearGap | Thermal | MEDIUM | 3 | ThermalActions.cpp |
| 36 | AddRats | Routing | MEDIUM | 5 | RoutingActions.cpp |
| 37 | DeleteRats | Routing | MEDIUM | 5 | RoutingActions.cpp |
| 38 | RipUp | Routing | MEDIUM | 5 | RoutingActions.cpp |
| 39 | AutoRoute | Routing | HIGH | 5 | RoutingActions.cpp |
| 40 | Display | Display | HIGH | 5 | DisplayActions.cpp |
| 41 | Mode | UI | CRITICAL | 4 | ModeStateMachine.cpp |
| 42 | SaveTo | File | MEDIUM | 2 | FileActions.cpp |
| 43 | SaveSettings | File | LOW | 2 | ConfigActions.cpp |
| 44 | LoadFrom | File | MEDIUM | 2 | FileActions.cpp |
| 45 | New | File | MEDIUM | 2 | FileActions.cpp |
| 46 | Quit | File | LOW | 2 | FileActions.cpp |
| 47 | Import | File | MEDIUM | 5 | FileActions.cpp |
| 48 | PasteBuffer | Buffer | HIGH | 5 | BufferActions.cpp |
| 49 | Undo | Undo | MEDIUM | 5 | UndoActions.cpp |
| 50 | Redo | Undo | MEDIUM | 5 | UndoActions.cpp |
| 51 | Atomic | Undo | MEDIUM | 5 | UndoActions.cpp |
| 52 | SetValue | Config | MEDIUM | 2 | ConfigActions.cpp |
| 53 | RouteStyle | Config | MEDIUM | 2 | ConfigActions.cpp |
| 54 | Polygon | Polygon | HIGH | 5 | PolygonActions.cpp |
| 55 | SetViaLayers | Via | MEDIUM | 3 | ViaActions.cpp |
| 56 | Renumber | Utility | MEDIUM | 5 | UtilityActions.cpp |
| 57 | Message | Utility | LOW | 2 | UtilityActions.cpp |
| 58 | DumpLibrary | Utility | LOW | 2 | UtilityActions.cpp |
| 59 | ExecuteFile | Utility | LOW | 2 | UtilityActions.cpp |
| 60 | ExecCommand | Utility | LOW | 2 | UtilityActions.cpp |
| 61 | Attributes | Utility | LOW | 2 | UtilityActions.cpp |
| 62 | PSCalib | Utility | LOW | 5 | UtilityActions.cpp |
| 63 | ChangeJoin | Modification | LOW | 3 | FlagActions.cpp |

### Appendix B: Dependency Graph

```
High-Level Dependencies (simplified):

Actions Layer (C++)
├─ ObjectModificationAction
│  ├─ SizeChangeAction
│  │  ├─ ChangeSizeAction
│  │  ├─ ChangeDrillSizeAction
│  │  └─ ChangeClearSizeAction
│  ├─ FlagChangeAction
│  │  ├─ ChangeSquareAction
│  │  └─ ... (12 flag actions)
│  └─ PropertyChangeAction
│     ├─ ChangeNameAction
│     └─ ChangePinNameAction
├─ SelectionAction
│  ├─ SelectAction
│  ├─ UnselectAction
│  └─ RemoveSelectedAction
└─ ModeStateMachine
   ├─ ArrowMode
   ├─ LineMode
   ├─ PolygonMode
   └─ ... (10 modes)

Core C Layer (unchanged for now)
├─ PCBType (global.h)
├─ change.h (modification functions)
├─ search.h (object finding)
├─ select.h (selection operations)
├─ draw.h (rendering)
└─ undo.h (undo/redo)
```

### Appendix C: Testing Strategy

#### Unit Tests (Google Test)

```cpp
// tests/cpp/actions/selection_test.cpp
class SelectActionTest : public ::testing::Test {
protected:
    void SetUp() override {
        pcb_ = CreateEmptyPCB();
        line_ = CreateLine(pcb_, 0, 0, 100, 100, 10, 5, 0);
        action_ = std::make_unique<SelectAction>();
    }

    void TearDown() override {
        FreePCB(pcb_);
    }

    PCBType* pcb_;
    LineType* line_;
    std::unique_ptr<SelectAction> action_;
};

TEST_F(SelectActionTest, SelectObjectAtCursor) {
    char* argv[] = {"Object"};
    int result = action_->execute(1, argv, 50, 50);

    EXPECT_EQ(0, result);
    EXPECT_TRUE(TEST_FLAG(SELECTEDFLAG, line_));
}

TEST_F(SelectActionTest, SelectAll) {
    LineType* line2 = CreateLine(pcb_, 200, 200, 300, 300, 10, 5, 0);

    char* argv[] = {"All"};
    int result = action_->execute(1, argv, 0, 0);

    EXPECT_EQ(0, result);
    EXPECT_TRUE(TEST_FLAG(SELECTEDFLAG, line_));
    EXPECT_TRUE(TEST_FLAG(SELECTEDFLAG, line2));
}
```

#### Integration Tests (Shared C/C++ tests)

```bash
# tests/integration/test_select.sh
#!/bin/bash

# Load test PCB
pcb --action 'LoadFrom(test_board.pcb)'

# Select object
pcb --action 'Select(Object)' --click 100,100

# Verify selection
pcb --action 'Report(Selected)' | grep "1 object selected"
```

#### Performance Benchmarks

```cpp
// benchmarks/action_benchmark.cpp
static void BM_SelectAction(benchmark::State& state) {
    PCBType* pcb = CreateTestPCBWithObjects(1000);
    SelectAction action;

    for (auto _ : state) {
        char* argv[] = {"All"};
        action.execute(1, argv, 0, 0);
    }

    FreePCB(pcb);
}
BENCHMARK(BM_SelectAction);
```

### Appendix D: Migration Checklist Template

For each action being migrated:

```markdown
## Action: [ActionName]

**Category:** [Selection/Modification/File/etc.]
**Complexity:** [Low/Medium/High/Critical]
**Original LOC:** [lines in action.c]
**Dependencies:** [list global vars accessed]

### Pre-Migration
- [ ] Document current behavior
- [ ] Identify all call sites
- [ ] List global state dependencies
- [ ] Write characterization tests (C)
- [ ] Measure baseline performance

### Migration
- [ ] Create C++ action class
- [ ] Implement execute() method
- [ ] Handle all argument variations
- [ ] Add unit tests (C++)
- [ ] Verify tests pass
- [ ] Add to action_bridge fallback

### Post-Migration
- [ ] Integration tests pass
- [ ] Performance ≥ baseline
- [ ] Code review complete
- [ ] Documentation updated
- [ ] Remove C implementation (after validation period)

### Validation Period: 1 week
- [ ] No bug reports
- [ ] All automated tests passing
```

---

## Conclusion

This refactoring proposal provides a **systematic, low-risk path** to transform action.c from an 8,427-line monolith into a modular, maintainable C++ architecture. By leveraging the existing C++ action infrastructure and following a phased approach, we can:

✅ **Improve code quality** - Smaller, focused files with clear responsibilities
✅ **Enhance testability** - Unit tests for individual actions
✅ **Reduce coupling** - Dependency injection instead of global state
✅ **Increase type safety** - Compile-time error checking
✅ **Maintain compatibility** - C/C++ bridge ensures no breaking changes
✅ **Enable future growth** - Easy to add new actions and features

**Estimated Timeline:** 24 weeks (6 months) with 1-2 developers
**Risk Level:** Medium (mitigated by phased approach and dual operation)
**Recommended Start:** Phase 2 - Simple Actions (infrastructure complete)

**Next Steps:**
1. Review and approve this proposal
2. Begin Phase 2 with utility actions (low risk)
3. Iterate based on learnings
4. Proceed through phases with continuous validation

---

**Document Version:** 1.0
**Last Updated:** November 14, 2025
**Author:** PCB Development Team
**Reviewed By:** [TBD]
