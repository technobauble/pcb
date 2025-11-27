# Mode Migration Architecture Review

**Date:** November 27, 2025 (Revised)
**Branch:** `claude/refactor-action-c-01YXGswdA4r3spfjFQcru8om`
**Reviewer:** Claude Code
**Status:** Simplified design - no intermediate AttachedXxxMode classes needed

---

## Executive Summary

The State Pattern foundation for editor modes is **solid and well-designed**. However, before continuing with Phase 2, we should consider refactoring to introduce intermediate base classes that capture common patterns. This will reduce code duplication across the 16 mode implementations and improve long-term maintainability.

**Key Recommendation:** Create intermediate base classes for mode families before implementing more modes.

---

## Current Implementation Status

### What's Done (Phase 1)
- `EditorMode` base class with clean interface
- `ModeManager` for mode transitions and event dispatch
- 4 modes implemented: RemoveMode, ViaMode, ThermalMode, LockMode
- Fallback to legacy `NotifyMode_Legacy()` for unimplemented modes
- C wrapper functions for backward compatibility

### Current File Structure
```
src/actions/modes/
├── EditorMode.h        # Base class (334 lines)
├── ModeManager.cpp     # Context manager (281 lines)
├── ModesCommon.h       # Common includes (58 lines)
├── RemoveMode.cpp      # Phase 1 (105 lines)
├── ViaMode.cpp         # Phase 1 (130 lines)
├── ThermalMode.cpp     # Phase 1 (122 lines)
└── LockMode.cpp        # Phase 1 (156 lines)
```

---

## Identified Patterns in Legacy Code

### Pattern Analysis from action.c

After reviewing the legacy `NotifyMode_Legacy()` implementation (action.c:850-1662), I identified these distinct mode families:

#### Family A: "Search-and-Act" Modes
**Modes:** RemoveMode, LockMode, ThermalMode, RotateMode

**Common Pattern:**
```c
// 1. Search for object at cursor
type = SearchScreen(x, y, SEARCH_TYPES, &ptr1, &ptr2, &ptr3);

// 2. Check if found and not locked
if (type != NO_TYPE && !TEST_FLAG(LOCKFLAG, ptr2)) {
    // 3. Perform action on object
    ActOnObject(type, ptr1, ptr2, ptr3);

    // 4. Update state and redraw
    SetChangedFlag(true);
    DrawObject(type, ptr1, ptr2);
    Draw();
}
```

**Lines in action.c:**
- LOCK_MODE: 1002-1047 (45 lines)
- THERMAL_MODE: 1048-1070 (22 lines)
- REMOVE_MODE: 1494-1529 (35 lines)
- ROTATE_MODE: 1531-1536 (5 lines)

#### Family B: "Stateful/Multi-Click" Modes
**Modes:** CopyMode, MoveMode, RectangleMode, InsertPointMode, LineMode, ArcMode, PolygonMode, PolygonHoleMode

**Key Insight:** All of these modes use a state machine pattern with STATE_FIRST, STATE_SECOND, and sometimes STATE_THIRD. The differences are:
1. **Number of states** (2 vs 3 vs N)
2. **What Crosshair attachment they use** (AttachedObject, AttachedBox, AttachedLine, AttachedPolygon)
3. **Whether they loop/continue** or reset after completion

| Mode | Attachment | States | Behavior |
|------|-----------|--------|----------|
| CopyMode | AttachedObject | FIRST → SECOND | Reset after copy |
| MoveMode | AttachedObject | FIRST → SECOND | Reset after move |
| InsertPointMode | AttachedObject | FIRST → SECOND | Reset after insert |
| RectangleMode | AttachedBox | FIRST → SECOND → THIRD | Reset after create |
| ArcMode | AttachedBox | FIRST → SECOND → THIRD | Continues (loops) |
| LineMode | AttachedLine | FIRST → SECOND → THIRD | Continues (loops) |
| PolygonMode | AttachedPolygon | Collects N points | Reset when closed |
| PolygonHoleMode | AttachedPolygon | Collects N points | Reset when closed |

**Common Pattern:**
```c
switch (Crosshair.AttachedXxx.State) {
    case STATE_FIRST:
        // Set initial anchor/search for object
        // Transition to STATE_SECOND
        break;
    case STATE_SECOND:
        // Create object or continue
        // May go to STATE_THIRD or reset to STATE_FIRST
        break;
    case STATE_THIRD:
        // Continue creating (for multi-segment modes)
        // May loop back or continue
        break;
}
```

**Lines in action.c:**
- COPY_MODE + MOVE_MODE: 1539-1595 (56 lines, shared!)
- RECTANGLE_MODE: 1232-1271 (39 lines)
- INSERTPOINT_MODE: 1598-1660 (62 lines)
- LINE_MODE: 1072-1230 (158 lines - most complex!)
- ARC_MODE: 915-1000 (85 lines)
- POLYGON_MODE: 1302-1337 (35 lines)
- POLYGONHOLE_MODE: 1338-1435 (97 lines)

**Critical Note:** COPY_MODE and MOVE_MODE share the same switch statement in action.c (lines 1539-1595). The code comment says: *"both are almost the same"*. Only the inner operation differs.

#### Family D: "Simple Single-Click" Modes
**Modes:** ViaMode, TextMode

**Pattern:** Single click creates object, no state machine.

**Lines in action.c:**
- VIA_MODE: 890-913 (23 lines)
- TEXT_MODE: 1273-1300 (27 lines)

#### Special Cases
- **ARROW_MODE** (856-888): Complex selection with timer callbacks
- **PASTEBUFFER_MODE** (1436-1493): Buffer paste operation

---

## Recommended Refactoring

### 1. Create Intermediate Base Classes

#### SearchAndActMode (High Priority)

For modes that search for an object and perform an action on it.

```cpp
// src/actions/modes/SearchAndActMode.h

class SearchAndActMode : public EditorMode {
public:
    void onNotify(Coord x, Coord y) override {
        void *ptr1, *ptr2, *ptr3;
        int type = SearchScreen(x, y, getSearchTypes(), &ptr1, &ptr2, &ptr3);

        if (type == NO_TYPE) {
            onNothingFound(x, y);
            return;
        }

        if (checkLocked() && TEST_FLAG(LOCKFLAG, (PinType*)ptr2)) {
            onLockedObject(type, ptr1, ptr2, ptr3);
            return;
        }

        if (actOnObject(type, ptr1, ptr2, ptr3)) {
            postAction(type, ptr1, ptr2, ptr3);
        }
    }

    void onRelease() override {
        // Most search-and-act modes don't use release
    }

protected:
    // Subclasses implement these
    virtual int getSearchTypes() const = 0;
    virtual bool actOnObject(int type, void* ptr1, void* ptr2, void* ptr3) = 0;

    // Optional overrides
    virtual bool checkLocked() const { return false; }
    virtual void onNothingFound(Coord x, Coord y) {}
    virtual void onLockedObject(int type, void* ptr1, void* ptr2, void* ptr3) {
        Message(_("Sorry, the object is locked\n"));
    }
    virtual void postAction(int type, void* ptr1, void* ptr2, void* ptr3) {
        SetChangedFlag(true);
    }
};
```

**Refactored LockMode would become:**
```cpp
class LockMode : public SearchAndActMode {
protected:
    int getSearchTypes() const override { return LOCK_TYPES; }

    bool actOnObject(int type, void* ptr1, void* ptr2, void* ptr3) override {
        if (type == ELEMENT_TYPE) {
            toggleElementLock((ElementType*)ptr2);
        } else {
            toggleObjectLock((TextType*)ptr3);
        }
        return true;
    }

    void postAction(int type, void* ptr1, void* ptr2, void* ptr3) override {
        SearchAndActMode::postAction(type, ptr1, ptr2, ptr3);
        hid_actionl("Report", "Object", NULL);
    }

private:
    void toggleElementLock(ElementType* element);
    void toggleObjectLock(TextType* thing);
};
```

#### StatefulMode (Medium-High Priority)

**Key Insight:** All multi-click modes are state machines using `STATE_FIRST`, `STATE_SECOND`, `STATE_THIRD`. Looking at the Crosshair attachment structures in `global.h`:

```c
// All have a State field, but different additional data:
AttachedLineType   { Point1, Point2, State, draw }
AttachedBoxType    { Point1, Point2, State, otherway }
AttachedObjectType { X, Y, BoundingBox, Type, State, Ptr1-3, Rubberband... }
AttachedPolygon    // Uses PolygonType directly
```

They're **not** casts of the same structure, but they **all share `State`**. This means:
1. State machine logic can be unified
2. Concrete modes access their specific attachment's other fields directly
3. **No need for intermediate `AttachedXxxMode` classes** - just have concrete modes implement `getState()`/`setState()`

```cpp
// src/actions/modes/StatefulMode.h

/*!
 * \brief Base class for modes that use a click-state machine
 *
 * Most PCB modes use STATE_FIRST, STATE_SECOND, STATE_THIRD to track
 * progress through multi-click operations. Concrete modes implement
 * getState()/setState() to access their specific Crosshair attachment.
 */
class StatefulMode : public EditorMode {
public:
    void onEnter() override {
        resetState();
    }

    void onExit() override {
        resetState();
    }

    virtual void onCancel() {
        resetState();
    }

protected:
    // Subclasses implement to access their specific Crosshair attachment
    virtual int getState() const = 0;
    virtual void setState(int state) = 0;

    // Common reset - subclasses may override to clear additional state
    virtual void resetState() {
        setState(STATE_FIRST);
    }

    // State constants (match existing PCB defines)
    static constexpr int STATE_FIRST = 0;
    static constexpr int STATE_SECOND = 1;
    static constexpr int STATE_THIRD = 2;
};
```

#### TwoClickMode (Convenience subclass)

For the common case of exactly two states (copy, move, insert point):

```cpp
// src/actions/modes/TwoClickMode.h

/*!
 * \brief Convenience base for modes with exactly two click states
 *
 * Provides onFirstClick()/onSecondClick() instead of manual switch.
 */
class TwoClickMode : public StatefulMode {
public:
    void onNotify(Coord x, Coord y) override {
        switch (getState()) {
            case STATE_FIRST:
                if (onFirstClick(x, y)) {
                    setState(STATE_SECOND);
                }
                break;
            case STATE_SECOND:
                onSecondClick(x, y);
                resetState();
                break;
        }
    }

protected:
    virtual bool onFirstClick(Coord x, Coord y) = 0;  // return true to advance
    virtual void onSecondClick(Coord x, Coord y) = 0;
};
```

#### ObjectTransferMode (for Copy/Move)

CopyMode and MoveMode are nearly identical - factor out the common logic:

```cpp
// src/actions/modes/ObjectTransferMode.h

class ObjectTransferMode : public TwoClickMode {
protected:
    // Use Crosshair.AttachedObject for state
    int getState() const override { return Crosshair.AttachedObject.State; }
    void setState(int state) override { Crosshair.AttachedObject.State = state; }

    void resetState() override {
        TwoClickMode::resetState();
        Crosshair.AttachedObject.Type = NO_TYPE;
    }

    bool onFirstClick(Coord x, Coord y) override {
        int type = SearchScreen(x, y, getTransferTypes(),
                               &Crosshair.AttachedObject.Ptr1,
                               &Crosshair.AttachedObject.Ptr2,
                               &Crosshair.AttachedObject.Ptr3);

        if (type == NO_TYPE) return false;

        if (TEST_FLAG(LOCKFLAG, (PinType*)Crosshair.AttachedObject.Ptr2)) {
            Message(_("Sorry, the object is locked\n"));
            return false;
        }

        Crosshair.AttachedObject.Type = type;
        AttachForCopy(x, y);
        return true;
    }

    void onSecondClick(Coord x, Coord y) override {
        Coord dx = x - Crosshair.AttachedObject.X;
        Coord dy = y - Crosshair.AttachedObject.Y;

        performTransfer(Crosshair.AttachedObject.Type,
                       Crosshair.AttachedObject.Ptr1,
                       Crosshair.AttachedObject.Ptr2,
                       Crosshair.AttachedObject.Ptr3, dx, dy);
        SetChangedFlag(true);
    }

    // Subclasses implement
    virtual int getTransferTypes() const = 0;
    virtual void performTransfer(int type, void* p1, void* p2, void* p3,
                                  Coord dx, Coord dy) = 0;
};

class CopyMode : public ObjectTransferMode {
protected:
    int getTransferTypes() const override { return COPY_TYPES; }
    void performTransfer(...) override { CopyObject(...); }
public:
    const char* getName() const override { return "Copy"; }
    int getModeId() const override { return COPY_MODE; }
};

class MoveMode : public ObjectTransferMode {
protected:
    int getTransferTypes() const override { return MOVE_TYPES; }
    void performTransfer(...) override { MoveObjectAndRubberband(...); }
public:
    const char* getName() const override { return "Move"; }
    int getModeId() const override { return MOVE_MODE; }
};
```

#### Other StatefulMode Examples

Modes using other attachments implement `getState()`/`setState()` directly:

```cpp
// RectangleMode uses AttachedBox
class RectangleMode : public StatefulMode {
protected:
    int getState() const override { return Crosshair.AttachedBox.State; }
    void setState(int state) override { Crosshair.AttachedBox.State = state; }

    void onNotify(Coord x, Coord y) override {
        // Access Crosshair.AttachedBox.Point1, Point2 directly
        // ... rectangle creation logic
    }
};

// LineMode uses AttachedLine
class LineMode : public StatefulMode {
protected:
    int getState() const override { return Crosshair.AttachedLine.State; }
    void setState(int state) override { Crosshair.AttachedLine.State = state; }

    void onNotify(Coord x, Coord y) override {
        // Access Crosshair.AttachedLine.Point1, Point2 directly
        // ... line drawing logic
    }
};
```

### 2. Add Helper Methods to EditorMode

```cpp
class EditorMode {
protected:
    // Undo helpers
    void commitCreation(int type, void* obj) {
        AddObjectToCreateUndoList(type, obj, obj, obj);
        IncrementUndoSerialNumber();
    }

    void commitChange() {
        IncrementUndoSerialNumber();
        SetChangedFlag(true);
    }

    // Drawing helpers
    void redraw() {
        Draw();
    }

    // Shift key helper
    bool isShiftPressed() const {
        return gui->shift_is_pressed();
    }

    // Lock check helper
    bool isLocked(void* obj) const {
        return TEST_FLAG(LOCKFLAG, (PinType*)obj);
    }
};
```

### 3. Consider Adding Lifecycle Methods

```cpp
class EditorMode {
public:
    // Existing
    virtual void onNotify(Coord x, Coord y) = 0;
    virtual void onRelease() = 0;
    virtual void onEnter() {}
    virtual void onExit() {}
    virtual void onMotion(Coord x, Coord y) {}

    // New - for Escape key handling
    virtual void onCancel() {
        // Default: reset to initial state
    }

    // New - for better state management
    virtual void resetState() {
        // Called by onEnter and onCancel
    }
};
```

---

## Proposed Mode Hierarchy

```
EditorMode (abstract base)
│
├── SearchAndActMode (stateless: search → act on object)
│   ├── RemoveMode
│   ├── LockMode
│   ├── ThermalMode
│   └── RotateMode
│
├── StatefulMode (click-state machine, subclasses implement getState/setState)
│   │
│   ├── TwoClickMode (convenience: exactly 2 states)
│   │   │
│   │   ├── ObjectTransferMode (uses AttachedObject)
│   │   │   ├── CopyMode
│   │   │   └── MoveMode
│   │   │
│   │   └── InsertPointMode (uses AttachedObject)
│   │
│   ├── RectangleMode (uses AttachedBox, 2-3 states)
│   ├── ArcMode (uses AttachedBox, 3 states, continues)
│   ├── LineMode (uses AttachedLine, 3 states, continues)
│   ├── PolygonMode (uses AttachedPolygon, N points)
│   └── PolygonHoleMode (uses AttachedPolygon, N points)
│
├── SimpleClickMode (single-click, no state)
│   ├── ViaMode
│   └── TextMode
│
├── PasteBufferMode (special case)
└── ArrowMode (complex selection with timer callbacks)
```

### Hierarchy Rationale

1. **StatefulMode** is the common base for ALL multi-click modes
   - Defines abstract `getState()`/`setState()` interface
   - Provides `resetState()`, `onEnter()`/`onExit()` state cleanup
   - Adds `onCancel()` for Escape key handling
   - **Concrete modes implement which Crosshair attachment to use**

2. **TwoClickMode** is a convenience subclass of StatefulMode
   - Provides `onFirstClick()`/`onSecondClick()` instead of manual switch
   - Used when exactly 2 states are needed
   - Still requires subclass to implement `getState()`/`setState()`

3. **No intermediate AttachedXxxMode classes needed**
   - All Crosshair attachments have a `State` field
   - Concrete modes just implement 2-line `getState()`/`setState()`
   - Avoids unnecessary class hierarchy depth

4. **Concrete modes** directly access their Crosshair attachment
   - CopyMode/MoveMode → `Crosshair.AttachedObject`
   - RectangleMode/ArcMode → `Crosshair.AttachedBox`
   - LineMode → `Crosshair.AttachedLine`
   - PolygonMode → `Crosshair.AttachedPolygon`

---

## ActionContext Concerns

The current `ActionContext` struct mixes different concerns:

```cpp
typedef struct {
    // Mouse/selection state (used by many modes)
    struct { Coord X, Y; ... } Note;

    // Line-mode specific (should be in LineMode)
    LayerType* lastLayer;
    int addedLines;

    // Polygon-mode specific (should be in PolygonMode/InsertPointMode)
    PointType InsertedPoint;
    Cardinal polyIndex;
    struct { PolygonType *poly; LineType line; } fake;

    // General operation state
    int defer_updates;
    int defer_needs_update;
    bool saved_mode;
} ActionContext;
```

**Recommendation:** Mode-specific state should live in the mode classes, not in a global context. This:
- Makes modes self-contained
- Reduces global state coupling
- Makes testing easier
- Prevents state leakage between modes

**Example - LineMode with internal state:**
```cpp
class LineMode : public MultiSegmentMode {
private:
    LayerType* lastLayer_ = nullptr;
    int addedLines_ = 0;

public:
    void onEnter() override {
        lastLayer_ = CURRENT;
        addedLines_ = 0;
    }

    // ... rest of implementation
};
```

---

## Implementation Priority

### High Priority (Before Phase 2)
1. Create `SearchAndActMode` base class
2. Refactor existing RemoveMode, LockMode, ThermalMode to use it
3. Add helper methods to EditorMode base class
4. Verify build and test

### Medium Priority (Phase 2 - Foundation)
5. Create `StatefulMode` base class with abstract `getState()`/`setState()`
6. Create `TwoClickMode` convenience subclass
7. Create `ObjectTransferMode` → CopyMode, MoveMode
8. Implement InsertPointMode (also uses TwoClickMode + AttachedObject)

### Medium Priority (Phase 2 - Continued)
9. Implement RectangleMode (StatefulMode + AttachedBox)
10. Implement TextMode (SimpleClickMode or direct EditorMode)
11. Implement RotateMode (SearchAndActMode)

### Phase 3 - Complex Modes
12. Implement ArcMode (StatefulMode + AttachedBox, 3-state continuous)
13. Implement LineMode (StatefulMode + AttachedLine, 3-state continuous)
14. Implement PolygonMode, PolygonHoleMode (StatefulMode + AttachedPolygon)

### Final Phase
15. Implement ArrowMode (most complex - timer callbacks, selection)
16. Implement PasteBufferMode
17. Add `onCancel()` integration with ActionMode Escape handling
18. Move mode-specific state from ActionContext into mode classes
19. Clean up and remove legacy NotifyMode_Legacy()

---

## Future Considerations: Data Structure Unification

A future refactoring effort will likely unify PCB's data structures under common base classes:

1. **Attached State Unification:** `AttachedLineType`, `AttachedBoxType`, `AttachedObjectType` could inherit from a common `AttachedState` base
2. **PCB Object Unification:** `LineType`, `ArcType`, `PolygonType`, `ElementType`, etc. could inherit from a common `PcbObject` base

### Design Now to Accommodate This

**Current approach (tightly coupled):**
```cpp
// Hardcoded to specific Crosshair member
int getState() const override { return Crosshair.AttachedBox.State; }
void setState(int s) override { Crosshair.AttachedBox.State = s; }
```

**Future-proof approach (abstract interface):**
```cpp
// Abstract interface for attached state
class IAttachedState {
public:
    virtual ~IAttachedState() = default;
    virtual int getState() const = 0;
    virtual void setState(int state) = 0;
    virtual void reset() = 0;
    virtual PointType getPoint1() const = 0;  // Common to Line/Box
    virtual PointType getPoint2() const = 0;  // Common to Line/Box
};

// Adapters for current structs (temporary, until data unification)
class AttachedBoxAdapter : public IAttachedState {
    AttachedBoxType& box_;
public:
    AttachedBoxAdapter(AttachedBoxType& box) : box_(box) {}
    int getState() const override { return box_.State; }
    void setState(int s) override { box_.State = s; }
    // ...
};

// StatefulMode works with interface
class StatefulMode : public EditorMode {
protected:
    virtual IAttachedState& getAttachment() = 0;

    int getState() const { return getAttachment().getState(); }
    void setState(int s) { getAttachment().setState(s); }
};
```

### Similarly for PCB Objects

**Current approach (void pointers + type flags):**
```cpp
int type = SearchScreen(x, y, SEARCH_TYPES, &ptr1, &ptr2, &ptr3);
if (type == LINE_TYPE) {
    LineType* line = (LineType*)ptr2;
    // ...
}
```

**Future-proof approach (polymorphic objects):**
```cpp
// Future: SearchScreen returns polymorphic object
PcbObject* obj = SearchScreen(x, y, SEARCH_TYPES);
if (obj) {
    obj->toggleLock();  // Virtual method
    obj->draw();        // Virtual method
}
```

### Recommendation

For now, we can use the simpler direct-access approach (`Crosshair.AttachedBox.State`) but:

1. **Encapsulate access in methods** - don't scatter `Crosshair.AttachedBox` throughout mode code
2. **Keep mode logic focused** - modes should express intent ("set first point"), not implementation details
3. **Document the coupling** - note where future refactoring will be needed
4. **Consider thin adapter layer** - if a mode needs multiple attachment fields, consider a helper struct/class that groups access

This way, when data structure unification happens, changes will be localized to:
- The `getState()`/`setState()` implementations
- Any adapter classes we create
- Not scattered throughout mode logic

---

## Questions for Discussion

1. **State Location:** Should mode-specific state (like `addedLines` for LineMode) live in the mode class or remain in ActionContext for backward compatibility during transition?

2. **Base Class Files:** Should intermediate base classes be:
   - In separate files (`SearchAndActMode.h`, `StatefulMode.h`, etc.)
   - All in `EditorMode.h`
   - In a new `ModeBase.h`

3. **Refactoring Existing Modes:** Should we refactor the 4 existing Phase 1 modes to use `SearchAndActMode` now, or leave them as-is and only use the pattern for new modes?

4. **Return Values:** Should `onNotify()` return a result enum to indicate success/failure/consumed?

5. **Future-proofing vs Pragmatism:** Should we introduce the `IAttachedState` interface now (more work, but cleaner future migration), or use direct access with good encapsulation (simpler now, more refactoring later)?

---

## Next Steps

1. **Discuss this review** and decide on approach
2. If approved, implement `SearchAndActMode` base class
3. Refactor existing modes or proceed with Phase 2 using new patterns
4. Update STATE_PATTERN_PROGRESS.md with architectural decisions

---

## Appendix: Code References

| File | Lines | Description |
|------|-------|-------------|
| `action.c` | 850-1662 | Legacy NotifyMode_Legacy() |
| `action.c` | 1539-1595 | Copy/Move shared code |
| `EditorMode.h` | 1-334 | Current base class |
| `ModeManager.cpp` | 1-281 | Current manager |
| `LockMode.cpp` | 1-156 | Example Phase 1 mode |

---

**End of Review**
