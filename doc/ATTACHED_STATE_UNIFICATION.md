# Attached State Unification Design

**Date:** November 27, 2025
**Branch:** `claude/refactor-action-c-01YXGswdA4r3spfjFQcru8om`
**Status:** Design Phase
**Parent Effort:** Mode Migration / action.c Refactoring

---

## Overview

This document describes the unification of PCB's attached state structures used during interactive editing. This is a foundational change that will enable cleaner mode implementations and prepare for broader data structure modernization.

---

## Current State Analysis

### Attached State Structures (global.h)

```c
// AttachedLineType (lines 652-658) - for line drawing
typedef struct {
    PointType Point1;     // Start position
    PointType Point2;     // End position
    long int State;       // STATE_FIRST/SECOND/THIRD
    bool draw;            // Whether to draw preview
} AttachedLineType;

// AttachedBoxType (lines 663-669) - for rectangle/arc drawing
typedef struct {
    PointType Point1;     // First corner
    PointType Point2;     // Second corner
    long int State;       // STATE_FIRST/SECOND/THIRD
    bool otherway;        // Arc direction flag
} AttachedBoxType;

// AttachedObjectType (lines 674-687) - for object manipulation
typedef struct {
    Coord X, Y;           // Saved position (move origin)
    BoxType BoundingBox;  // Object bounds
    long int Type;        // Object type (LINE_TYPE, etc.)
    long int State;       // STATE_FIRST/SECOND
    void *Ptr1, *Ptr2, *Ptr3;  // Object pointers
    Cardinal RubberbandN, RubberbandMax;
    RubberbandType *Rubberband;
} AttachedObjectType;

// AttachedPolygon - uses full PolygonType (line 712)
// This is a full PCB object, NOT a simple state holder
// Should be handled as part of PCB object unification, not here
```

### Crosshair Structure (lines 700-715)

```c
typedef struct {
    hidGC GC, AttachGC;
    Coord X, Y;
    Coord MinX, MinY, MaxX, MaxY;
    AttachedLineType AttachedLine;
    AttachedBoxType AttachedBox;
    PolygonType AttachedPolygon;      // Note: Full object type
    AttachedObjectType AttachedObject;
    enum crosshair_shape shape;
} CrosshairType;
```

### Usage Distribution (~456 total references)

| File | AttachedLine | AttachedBox | AttachedObject | AttachedPolygon |
|------|-------------|-------------|----------------|-----------------|
| action.c | 110 | 82 | 69 | 10 |
| crosshair.c | 20 | 14 | 40 | 3 |
| set.c | 8 | 8 | 6 | 1 |
| misc.c | 0 | 0 | 20 | 0 |
| polygon.c | 4 | 0 | 0 | 12 |
| line.c | 5 | 0 | 0 | 0 |
| rats.c | 10 | 0 | 0 | 0 |
| Other | ~10 | ~4 | ~19 | 0 |

---

## Commonalities Analysis

### Shared Features

1. **State field** - All have `long int State` for state machine tracking
2. **Two-point geometry** - AttachedLine and AttachedBox both have Point1/Point2

### Differences

| Feature | AttachedLine | AttachedBox | AttachedObject |
|---------|-------------|-------------|----------------|
| Geometry | Point1, Point2 | Point1, Point2 | X, Y + BoundingBox |
| Extra bool | draw | otherway | - |
| Object refs | - | - | Type, Ptr1/2/3 |
| Rubberband | - | - | Yes |

### AttachedPolygon is Different

`AttachedPolygon` uses `PolygonType` which includes `ANYOBJECTFIELDS`:
- BoundingBox, ID, Flags
- Points array (dynamic)
- Clipping/hole data

This is a fundamentally different pattern - it's holding a full PCB object during creation, not just tracking state. **AttachedPolygon should be deferred to the PCB object unification effort.**

---

## Proposed Design

### Scope

Unify **AttachedLine**, **AttachedBox**, and **AttachedObject** under a common interface. Defer AttachedPolygon.

### Interface Hierarchy

```cpp
// Base interface for all attached states
class IAttachedState {
public:
    virtual ~IAttachedState() = default;

    // State machine
    virtual int getState() const = 0;
    virtual void setState(int state) = 0;
    virtual void reset() = 0;

    // State constants
    static constexpr int STATE_FIRST = 0;
    static constexpr int STATE_SECOND = 1;
    static constexpr int STATE_THIRD = 2;
};

// For line/box drawing with two points
class ITwoPointAttachment : public IAttachedState {
public:
    virtual PointType getPoint1() const = 0;
    virtual void setPoint1(PointType p) = 0;
    virtual PointType getPoint2() const = 0;
    virtual void setPoint2(PointType p) = 0;

    // Convenience
    void setPoints(PointType p1, PointType p2) {
        setPoint1(p1);
        setPoint2(p2);
    }
};

// For object manipulation (copy/move/etc.)
class IObjectAttachment : public IAttachedState {
public:
    virtual int getObjectType() const = 0;
    virtual void setObjectType(int type) = 0;

    virtual void* getPtr1() const = 0;
    virtual void* getPtr2() const = 0;
    virtual void* getPtr3() const = 0;
    virtual void setPointers(void* p1, void* p2, void* p3) = 0;

    virtual Coord getX() const = 0;
    virtual Coord getY() const = 0;
    virtual void setPosition(Coord x, Coord y) = 0;

    // Rubberband access
    virtual int getRubberbandCount() const = 0;
    virtual RubberbandType* getRubberband() = 0;
};
```

### Adapter Classes (Wrap Existing Structs)

```cpp
// Adapter for AttachedLineType
class AttachedLineAdapter : public ITwoPointAttachment {
    AttachedLineType& line_;
public:
    explicit AttachedLineAdapter(AttachedLineType& line) : line_(line) {}

    int getState() const override { return line_.State; }
    void setState(int s) override { line_.State = s; }
    void reset() override {
        line_.State = STATE_FIRST;
        line_.Point1 = line_.Point2 = {0, 0};
        line_.draw = false;
    }

    PointType getPoint1() const override { return line_.Point1; }
    void setPoint1(PointType p) override { line_.Point1 = p; }
    PointType getPoint2() const override { return line_.Point2; }
    void setPoint2(PointType p) override { line_.Point2 = p; }

    // Line-specific
    bool getDraw() const { return line_.draw; }
    void setDraw(bool d) { line_.draw = d; }
};

// Adapter for AttachedBoxType
class AttachedBoxAdapter : public ITwoPointAttachment {
    AttachedBoxType& box_;
public:
    explicit AttachedBoxAdapter(AttachedBoxType& box) : box_(box) {}

    int getState() const override { return box_.State; }
    void setState(int s) override { box_.State = s; }
    void reset() override {
        box_.State = STATE_FIRST;
        box_.Point1 = box_.Point2 = {0, 0};
        box_.otherway = false;
    }

    PointType getPoint1() const override { return box_.Point1; }
    void setPoint1(PointType p) override { box_.Point1 = p; }
    PointType getPoint2() const override { return box_.Point2; }
    void setPoint2(PointType p) override { box_.Point2 = p; }

    // Box-specific
    bool getOtherway() const { return box_.otherway; }
    void setOtherway(bool o) { box_.otherway = o; }
};

// Adapter for AttachedObjectType
class AttachedObjectAdapter : public IObjectAttachment {
    AttachedObjectType& obj_;
public:
    explicit AttachedObjectAdapter(AttachedObjectType& obj) : obj_(obj) {}

    int getState() const override { return obj_.State; }
    void setState(int s) override { obj_.State = s; }
    void reset() override {
        obj_.State = STATE_FIRST;
        obj_.Type = NO_TYPE;
        obj_.Ptr1 = obj_.Ptr2 = obj_.Ptr3 = nullptr;
    }

    int getObjectType() const override { return obj_.Type; }
    void setObjectType(int t) override { obj_.Type = t; }

    void* getPtr1() const override { return obj_.Ptr1; }
    void* getPtr2() const override { return obj_.Ptr2; }
    void* getPtr3() const override { return obj_.Ptr3; }
    void setPointers(void* p1, void* p2, void* p3) override {
        obj_.Ptr1 = p1; obj_.Ptr2 = p2; obj_.Ptr3 = p3;
    }

    Coord getX() const override { return obj_.X; }
    Coord getY() const override { return obj_.Y; }
    void setPosition(Coord x, Coord y) override { obj_.X = x; obj_.Y = y; }

    int getRubberbandCount() const override { return obj_.RubberbandN; }
    RubberbandType* getRubberband() override { return obj_.Rubberband; }
};
```

### Crosshair Accessor

```cpp
// Provides typed access to Crosshair attachments
class CrosshairAttachments {
public:
    static AttachedLineAdapter line() {
        return AttachedLineAdapter(Crosshair.AttachedLine);
    }

    static AttachedBoxAdapter box() {
        return AttachedBoxAdapter(Crosshair.AttachedBox);
    }

    static AttachedObjectAdapter object() {
        return AttachedObjectAdapter(Crosshair.AttachedObject);
    }

    // AttachedPolygon stays as direct access for now
    static PolygonType& polygon() {
        return Crosshair.AttachedPolygon;
    }
};
```

---

## Usage in Modes

### Before (direct access)

```cpp
class LineMode : public StatefulMode {
    int getState() const override {
        return Crosshair.AttachedLine.State;
    }
    void setState(int s) override {
        Crosshair.AttachedLine.State = s;
    }

    void onNotify(Coord x, Coord y) override {
        if (getState() == STATE_FIRST) {
            Crosshair.AttachedLine.Point1.X = x;
            Crosshair.AttachedLine.Point1.Y = y;
            // ... more direct access
        }
    }
};
```

### After (via interface)

```cpp
class LineMode : public StatefulMode {
    AttachedLineAdapter attachment_{Crosshair.AttachedLine};

    int getState() const override { return attachment_.getState(); }
    void setState(int s) override { attachment_.setState(s); }

    void onNotify(Coord x, Coord y) override {
        if (getState() == STATE_FIRST) {
            attachment_.setPoint1({x, y});
            // ...
        }
    }
};
```

### Even Better (with CrosshairAttachments)

```cpp
class LineMode : public StatefulMode {
protected:
    ITwoPointAttachment& getAttachment() override {
        static auto adapter = CrosshairAttachments::line();
        return adapter;
    }

    // StatefulMode base provides getState()/setState() via getAttachment()
};
```

---

## Migration Strategy

### Phase 1: Create Infrastructure (no behavior change)
1. Create `IAttachedState`, `ITwoPointAttachment`, `IObjectAttachment` interfaces
2. Create adapter classes for existing structs
3. Create `CrosshairAttachments` accessor
4. Add to build system
5. Verify compilation

### Phase 2: Update Modes Incrementally
1. Update StatefulMode base class to use interfaces
2. Migrate modes one at a time to use adapters
3. Test each mode after migration

### Phase 3: Update Other Code
1. Update crosshair.c drawing code to use adapters
2. Update set.c mode setup code
3. Update misc.c helper functions

### Phase 4: Future - Replace Structs
When ready for data structure modernization:
1. Create new unified structs that implement interfaces directly
2. Replace adapters with direct implementations
3. Remove old typedef structs

---

## File Structure

```
src/
├── attached_state.h          # Interfaces: IAttachedState, ITwoPointAttachment, IObjectAttachment
├── attached_state_adapters.h # Adapters: AttachedLineAdapter, AttachedBoxAdapter, AttachedObjectAdapter
├── crosshair_attachments.h   # CrosshairAttachments accessor class
└── actions/modes/
    └── StatefulMode.h        # Updated to use IAttachedState
```

---

## Benefits

1. **Clean mode implementations** - Modes work with interfaces, not raw structs
2. **Encapsulated access** - All Crosshair.AttachedX access goes through adapters
3. **Future-proof** - When we unify data structures, only adapters change
4. **Testable** - Can mock IAttachedState for unit testing
5. **Type-safe** - Interfaces prevent mixing up Line vs Box vs Object

---

## Risks and Mitigations

| Risk | Mitigation |
|------|------------|
| Performance overhead from virtual calls | Adapters are thin; compiler can inline. Profile if concerned. |
| Increased complexity | Clear naming, good documentation, phased rollout |
| Breaking existing code | Adapters wrap existing structs; direct access still works during transition |

---

## Questions to Resolve

1. **Static vs instance adapters** - Should adapters be singletons (since there's one Crosshair) or created per-mode?

2. **Header organization** - One header with all interfaces, or split by concern?

3. **Backward compatibility** - Should we provide C wrappers for the interfaces?

4. **AttachedPolygon** - Confirm deferral to PCB object unification effort?

---

## Next Steps

1. Review and approve this design
2. Create the interface headers
3. Create adapter implementations
4. Update StatefulMode to use interfaces
5. Migrate existing modes
6. Update support code (crosshair.c, set.c)

---

**End of Design Document**
