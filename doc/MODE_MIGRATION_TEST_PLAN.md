# Mode Migration Test Plan

**Date:** December 1, 2025
**Branch:** `claude/refactor-action-c-01YXGswdA4r3spfjFQcru8om`
**Status:** Ready for Testing
**Tester:** Manual testing required

---

## Overview

This test plan verifies that all 16 editor modes work correctly after the migration from the legacy `NotifyMode_Legacy()` C switch statement to the new C++ State Pattern implementation.

### What Changed

1. **Legacy code removed:** ~900 lines of switch-case code in `action.c`
2. **New C++ modes:** 16 mode classes in `src/actions/modes/`
3. **Cancel/Escape:** New `onCancel()` and `isIdle()` integration
4. **No fallbacks:** ModeManager no longer falls back to legacy code

### Test Environment Setup

```bash
# Build the project
cd /home/parkecw1/src/pcb
make clean && make -j4

# Run PCB
./src/pcb
```

---

## Test Categories

### A. Simple Modes (No State Machine)

These modes perform an action on click without tracking multi-click state.

---

#### A1. ARROW_MODE (Select/Move)

**Purpose:** Default mode for selecting and moving objects

| Test | Steps | Expected Result |
|------|-------|-----------------|
| A1.1 Single click select | 1. Enter Arrow mode (press 'a' or toolbar)<br>2. Click on a line | Line is selected (highlighted) |
| A1.2 Click deselect | 1. With object selected<br>2. Click on empty area | Object deselected |
| A1.3 Shift-click multi-select | 1. Select one object<br>2. Shift+click another object | Both objects selected |
| A1.4 Box select | 1. Click and drag in empty area<br>2. Release to form box | All objects in box selected |
| A1.5 Drag single object | 1. Click on unselected object<br>2. Drag to new position<br>3. Release | Object moves to new position |
| A1.6 Drag selected objects | 1. Select multiple objects<br>2. Click and drag one<br>3. Release | All selected objects move together |
| A1.7 Ctrl+drag copy | 1. Click on object<br>2. Ctrl+drag to new position<br>3. Release | Object copied to new position |
| A1.8 Escape does nothing | 1. In Arrow mode, nothing selected<br>2. Press Escape | Nothing happens (already in Arrow mode) |

---

#### A2. VIA_MODE

**Purpose:** Place vias at click location

| Test | Steps | Expected Result |
|------|-------|-----------------|
| A2.1 Place via | 1. Enter Via mode (press 'v')<br>2. Click on board | Via placed at click location |
| A2.2 Place multiple | 1. Click multiple locations | Via placed at each location |
| A2.3 Escape to Arrow | 1. In Via mode<br>2. Press Escape | Returns to Arrow mode |
| A2.4 Undo via | 1. Place a via<br>2. Press 'u' (undo) | Via removed |

---

#### A3. REMOVE_MODE

**Purpose:** Delete objects at click location

| Test | Steps | Expected Result |
|------|-------|-----------------|
| A3.1 Remove line | 1. Enter Remove mode (press 'Delete' or 'Backspace')<br>2. Click on a line | Line deleted |
| A3.2 Remove via | 1. Click on a via | Via deleted |
| A3.3 Remove nothing | 1. Click on empty area | Nothing happens (no error) |
| A3.4 Escape to Arrow | 1. Press Escape | Returns to Arrow mode |
| A3.5 Undo remove | 1. Delete an object<br>2. Press 'u' | Object restored |

---

#### A4. ROTATE_MODE

**Purpose:** Rotate objects 90 degrees

| Test | Steps | Expected Result |
|------|-------|-----------------|
| A4.1 Rotate element | 1. Enter Rotate mode<br>2. Click on a component | Component rotates 90° CCW |
| A4.2 Multiple rotations | 1. Click same component 4 times | Returns to original orientation |
| A4.3 Escape to Arrow | 1. Press Escape | Returns to Arrow mode |

---

#### A5. THERMAL_MODE

**Purpose:** Toggle thermal relief on pins/vias in polygons

| Test | Steps | Expected Result |
|------|-------|-----------------|
| A5.1 Toggle thermal | 1. Create polygon over a via<br>2. Enter Thermal mode<br>3. Click on via | Thermal relief toggled |
| A5.2 Escape to Arrow | 1. Press Escape | Returns to Arrow mode |

---

#### A6. LOCK_MODE

**Purpose:** Toggle lock flag on objects

| Test | Steps | Expected Result |
|------|-------|-----------------|
| A6.1 Lock object | 1. Enter Lock mode<br>2. Click on unlocked object | Object locked (can't be moved) |
| A6.2 Unlock object | 1. Click on locked object | Object unlocked |
| A6.3 Verify lock | 1. Lock an object<br>2. Try to move in Arrow mode | Object doesn't move |
| A6.4 Escape to Arrow | 1. Press Escape | Returns to Arrow mode |

---

### B. Two-Point Modes (STATE_FIRST → STATE_SECOND)

These modes require two clicks: first to select/position, second to complete.

---

#### B7. LINE_MODE

**Purpose:** Draw copper traces

| Test | Steps | Expected Result |
|------|-------|-----------------|
| B7.1 Draw single line | 1. Enter Line mode (press 'l')<br>2. Click start point<br>3. Click end point | Line created between points |
| B7.2 Continuous drawing | 1. After first line<br>2. Click another point | Second line from end of first |
| B7.3 Cancel mid-line | 1. Click start point<br>2. Press Escape | Returns to STATE_FIRST, no line |
| B7.4 Escape when idle | 1. In Line mode, STATE_FIRST<br>2. Press Escape | Returns to Arrow mode |
| B7.5 Click on start | 1. Click start point<br>2. Click same point again | Cancels line (resets mode) |
| B7.6 45° mode | 1. Set clipping mode (Ctrl+/)<br>2. Draw line | Line constrained to 45° angles |
| B7.7 Layer change auto-via | 1. Start line on layer 1<br>2. Change to layer 2<br>3. Continue line | Via auto-created at junction |
| B7.8 Undo multi-segment | 1. Draw 3 connected segments<br>2. Press 'u' multiple times | Segments undo one at a time |

---

#### B8. ARC_MODE

**Purpose:** Draw arcs

| Test | Steps | Expected Result |
|------|-------|-----------------|
| B8.1 Draw arc | 1. Enter Arc mode (press 'a' twice or toolbar)<br>2. Click center point<br>3. Move to set radius<br>4. Click endpoint | Arc created |
| B8.2 Toggle direction | 1. Start arc<br>2. Press '/' or 'Tab' | Arc direction flips |
| B8.3 Cancel mid-arc | 1. Click center<br>2. Press Escape | Returns to STATE_FIRST |
| B8.4 Escape when idle | 1. In Arc mode, STATE_FIRST<br>2. Press Escape | Returns to Arrow mode |

---

#### B9. RECTANGLE_MODE

**Purpose:** Draw filled rectangles (on silk layer typically)

| Test | Steps | Expected Result |
|------|-------|-----------------|
| B9.1 Draw rectangle | 1. Enter Rectangle mode<br>2. Click first corner<br>3. Click opposite corner | Rectangle created |
| B9.2 Cancel mid-rect | 1. Click first corner<br>2. Press Escape | Returns to STATE_FIRST |
| B9.3 Escape when idle | 1. In Rectangle mode, STATE_FIRST<br>2. Press Escape | Returns to Arrow mode |

---

#### B10. TEXT_MODE

**Purpose:** Place text labels

| Test | Steps | Expected Result |
|------|-------|-----------------|
| B10.1 Place text | 1. Enter Text mode<br>2. Click location<br>3. Enter text in dialog | Text placed at location |
| B10.2 Escape to Arrow | 1. Press Escape | Returns to Arrow mode |

---

#### B11. COPY_MODE

**Purpose:** Copy objects to new location

| Test | Steps | Expected Result |
|------|-------|-----------------|
| B11.1 Copy object | 1. Enter Copy mode<br>2. Click on object<br>3. Move to new location<br>4. Click to place | Object copied |
| B11.2 Multiple copies | 1. After first copy<br>2. Click again | Another copy placed |
| B11.3 Cancel mid-copy | 1. Click object to select<br>2. Press Escape | Returns to STATE_FIRST |
| B11.4 Escape when idle | 1. Press Escape in STATE_FIRST | Returns to Arrow mode |

---

#### B12. MOVE_MODE

**Purpose:** Move objects to new location

| Test | Steps | Expected Result |
|------|-------|-----------------|
| B12.1 Move object | 1. Enter Move mode<br>2. Click on object<br>3. Move to new location<br>4. Click to place | Object moved |
| B12.2 Rubberband | 1. Move connected line endpoint | Connected lines stretch |
| B12.3 Cancel mid-move | 1. Click object<br>2. Press Escape | Object returns to original position |
| B12.4 Escape when idle | 1. Press Escape in STATE_FIRST | Returns to Arrow mode |

---

#### B13. INSERTPOINT_MODE

**Purpose:** Insert vertices into lines/polygons

| Test | Steps | Expected Result |
|------|-------|-----------------|
| B13.1 Insert in line | 1. Enter InsertPoint mode<br>2. Click on line segment<br>3. Click to place point | New vertex inserted |
| B13.2 Insert in polygon | 1. Click on polygon edge<br>2. Click to place | New vertex in polygon |
| B13.3 Locked object | 1. Click on locked object | Message: "object is locked" |
| B13.4 Cancel mid-insert | 1. Select edge<br>2. Press Escape | Returns to STATE_FIRST |
| B13.5 Escape when idle | 1. Press Escape in STATE_FIRST | Returns to Arrow mode |

---

#### B14. PASTEBUFFER_MODE

**Purpose:** Paste buffer contents

| Test | Steps | Expected Result |
|------|-------|-----------------|
| B14.1 Paste buffer | 1. Copy something to buffer<br>2. Enter PasteBuffer mode<br>3. Click location | Buffer contents pasted |
| B14.2 Multiple pastes | 1. Click multiple locations | Contents pasted at each |
| B14.3 Escape to Arrow | 1. Press Escape | Returns to Arrow mode |

---

### C. Multi-Point Modes (STATE_FIRST → STATE_SECOND → ... )

These modes track multiple points during creation.

---

#### C15. POLYGON_MODE

**Purpose:** Draw filled polygons

| Test | Steps | Expected Result |
|------|-------|-----------------|
| C15.1 Draw triangle | 1. Enter Polygon mode<br>2. Click 3 points<br>3. Press Shift+P to close | Triangle polygon created |
| C15.2 Draw complex | 1. Click 6+ points<br>2. Close polygon | Complex polygon created |
| C15.3 Cancel mid-polygon | 1. Click several points<br>2. Press Escape | Polygon cancelled, returns to STATE_FIRST |
| C15.4 Escape when idle | 1. In STATE_FIRST<br>2. Press Escape | Returns to Arrow mode |

---

#### C16. POLYGONHOLE_MODE

**Purpose:** Cut holes in polygons

| Test | Steps | Expected Result |
|------|-------|-----------------|
| C16.1 Create hole | 1. Enter PolygonHole mode<br>2. Click inside polygon<br>3. Draw hole shape<br>4. Close hole | Hole cut in polygon |
| C16.2 Cancel mid-hole | 1. Start drawing hole<br>2. Press Escape | Returns to STATE_FIRST |
| C16.3 Escape when idle | 1. Press Escape in STATE_FIRST | Returns to Arrow mode |

---

### D. Mode Switching and Integration

---

#### D1. Mode Switching

| Test | Steps | Expected Result |
|------|-------|-----------------|
| D1.1 Toolbar switching | 1. Click each toolbar button | Mode changes correctly |
| D1.2 Keyboard shortcuts | 1. Press mode hotkeys (l, v, a, etc.) | Mode changes correctly |
| D1.3 Menu switching | 1. Use Tools menu | Mode changes correctly |
| D1.4 Mid-operation switch | 1. Start line (click once)<br>2. Switch to Via mode | Line cancelled, Via mode active |

---

#### D2. Cancel/Escape Behavior

| Test | Steps | Expected Result |
|------|-------|-----------------|
| D2.1 Escape in simple mode | 1. Enter Via mode<br>2. Press Escape | Returns to Arrow mode |
| D2.2 Escape mid-operation | 1. Enter Line mode<br>2. Click once (start point)<br>3. Press Escape | Cancels line, stays in Line mode |
| D2.3 Double-Escape | 1. Enter Line mode<br>2. Click once<br>3. Press Escape (cancel)<br>4. Press Escape again | Returns to Arrow mode |
| D2.4 Cancel action | 1. Start operation<br>2. Mode(Cancel) action | Operation cancelled |

---

#### D3. Save/Restore Mode

| Test | Steps | Expected Result |
|------|-------|-----------------|
| D3.1 Arrow drag restore | 1. In Arrow mode, drag object<br>2. Release | Returns to Arrow mode after move |
| D3.2 Ctrl+drag restore | 1. Ctrl+drag object (copy)<br>2. Release | Returns to Arrow mode after copy |

---

#### D4. Undo/Redo Integration

| Test | Steps | Expected Result |
|------|-------|-----------------|
| D4.1 Undo single action | 1. Place via<br>2. Undo | Via removed |
| D4.2 Undo multi-segment | 1. Draw 3-segment line<br>2. Undo 3 times | All segments removed |
| D4.3 Redo | 1. Undo action<br>2. Redo | Action restored |
| D4.4 Undo across modes | 1. Draw line<br>2. Place via<br>3. Undo twice | Via then line removed |

---

### E. Edge Cases and Regression

---

#### E1. Edge Cases

| Test | Steps | Expected Result |
|------|-------|-----------------|
| E1.1 Click locked object | 1. Lock an object<br>2. Try to modify in various modes | Appropriate "locked" message |
| E1.2 Empty board | 1. Create new board<br>2. Test all modes | No crashes on empty board |
| E1.3 Rapid clicking | 1. Click rapidly in any mode | No crashes or state corruption |
| E1.4 Mode during dialog | 1. Open preferences dialog<br>2. Try mode operations | Operations blocked or queued |

---

#### E2. Regression Tests

| Test | Steps | Expected Result |
|------|-------|-----------------|
| E2.1 Load existing file | 1. Load complex .pcb file<br>2. Edit with various modes | All modes work on existing data |
| E2.2 Save after edit | 1. Make edits<br>2. Save file<br>3. Reload | Edits preserved correctly |
| E2.3 DRC after edit | 1. Make edits<br>2. Run DRC | DRC works, finds violations |
| E2.4 Export after edit | 1. Make edits<br>2. Export to Gerber | Export works correctly |

---

## Automated Test Suggestions

For future automation, consider:

1. **Unit tests** for each mode's `onNotify()`, `onRelease()`, `onCancel()`, `isIdle()`
2. **Integration tests** using PCB's action system: `Mode(Line)`, `Mode(Notify)`, etc.
3. **State verification** checking `Crosshair.AttachedLine.State` etc. after operations

Example test script approach:
```bash
# Create test board, run actions, verify results
pcb --action-string "Mode(Line)" \
    --action-string "Mode(Notify)" \
    --action-string "Mode(Escape)" \
    --action-string "Mode(Arrow)"
```

---

## Test Execution Checklist

### Phase 1: Smoke Test (30 min)
- [ ] A1.1-A1.4 (Arrow basic)
- [ ] A2.1 (Via place)
- [ ] A3.1 (Remove)
- [ ] B7.1-B7.3 (Line basic)
- [ ] D1.1-D1.2 (Mode switching)
- [ ] D2.1-D2.3 (Escape behavior)

### Phase 2: Full Mode Test (2 hrs)
- [ ] All A tests (simple modes)
- [ ] All B tests (two-point modes)
- [ ] All C tests (multi-point modes)

### Phase 3: Integration Test (1 hr)
- [ ] All D tests (mode switching, undo/redo)
- [ ] All E tests (edge cases, regression)

---

## Sign-off

| Phase | Tester | Date | Result | Notes |
|-------|--------|------|--------|-------|
| Smoke Test | | | | |
| Full Mode Test | | | | |
| Integration Test | | | | |

---

## Known Issues / Limitations

Document any issues found during testing:

1. _None yet_

---

## References

- `doc/MODE_MIGRATION_ARCHITECTURE_REVIEW.md` - Architecture decisions
- `doc/ATTACHED_STATE_UNIFICATION.md` - Interface design
- `src/actions/modes/` - Mode implementations
