# Action Refactoring Strategy Analysis

**Date:** November 17, 2025
**Purpose:** Strategic assessment before continuing migration

---

## Current Status

**Progress:** 14/78 actions migrated (18%)
**Simple actions remaining:** 14 with < 50 lines
**Complex actions:** 10 with > 150 lines

---

## Key Findings

### 1. Common Patterns Requiring Support

| Pattern | Usage Count | Status |
|---------|-------------|--------|
| `GetFunctionID` switch/case | 36 actions | ⚠️ Static function - needs export |
| `SearchScreen` (object search) | 38 uses | ⚠️ Needs investigation |
| `notify_crosshair_change` | 52 uses | ✅ Should be in crosshair.h |
| `SetChangedFlag` | 125 uses | ✅ Available from set.h |
| `IncrementUndoSerialNumber` | 43 uses | ✅ Available from undo.h |
| `Settings` global | 73 uses | ✅ Available from data.h |
| `PCB` global | 121 uses | ✅ Available from global.h |

### 2. Critical Blocker: GetFunctionID

**Problem:** `GetFunctionID()` is a static function that converts string arguments like "Object", "Selected", "AllRats" to enum values (F_Object, F_Selected, F_AllRats).

**Impact:** 36 actions use this function for their switch/case dispatch logic.

**Current Implementation:**
```c
static int GetFunctionID (String Ident) {
    // Lookup string in FunctionID array
    // Return enum value or -1
}
```

**Options:**
1. **Export GetFunctionID** - Make it non-static, add to action.h
2. **Rewrite without GetFunctionID** - Use strcmp chains (current approach)
3. **Create C++ enum helper** - Map strings to enums in C++

**Recommendation:** Export GetFunctionID for now, consider C++ wrapper later.

### 3. Simple Actions Ready for Migration

These have < 50 lines and simple logic:

| Lines | Action | Notes |
|-------|--------|-------|
| 45 | ActionNew | File operation |
| 46 | ActionMoveToCurrentLayer | Layer operation |
| 49 | ActionAutoRoute | Calls AutoRoute() |
| 49 | ActionChangeHole | Object modification |
| 42 | ActionAutoPlaceSelected | Calls AutoPlace() |

All migrated actions: Atomic, MarkCrosshair, ExecCommand, RemoveSelected, DeleteRats, Polygon, RouteStyle (and previous 7)

### 4. Complex Actions to Defer

These require substantial refactoring:

| Lines | Action | Complexity |
|-------|--------|------------|
| 789 | ActionSetViaLayers | Massive, complex layer logic |
| 398 | ActionDisplay | Multiple display toggles, 30+ cases |
| 371 | ActionRenumber | Element renumbering with file I/O |
| 311 | ActionImport | Schematic import, external process |
| 267 | ActionDisperseElements | Element auto-placement |
| 226 | ActionMode | Mode switching state machine |
| 205 | ActionSelect | Selection logic, multiple types |
| 178 | ActionUndo | Complex undo state machine |

### 5. Helper Functions Needing Export

Currently static, need to be exported for C++ actions:

1. **GetFunctionID** (critical) - String to enum lookup
2. **AdjustAttachedBox** - Crosshair adjustment
3. **NotifyLine** - Line drawing notification
4. **NotifyBlock** - Block notification
5. **NotifyMode** - Mode notification (400+ lines itself!)

### 6. Already Exported Helpers

✅ **ChangeFlag** - Flag modification (for SetFlag, ClrFlag, ChangeFlag)
✅ **ClearWarnings** - Clear warning flags (for rats actions)

---

## Recommended Next Steps

### Phase 1: Export Critical Helpers (This Session)

1. **Export GetFunctionID**
   - Make it non-static
   - Add declaration to action.h
   - Document enum values in comments
   - Test with existing C code

2. **Verify other common functions**
   - Check if SearchScreen, GetValue are already exported
   - Document where they live

### Phase 2: Migrate Simple Actions (Next Session)

Prioritized list based on simplicity and dependencies:

**Batch 1: File/Buffer Actions (simple)**
- ActionNew (45 lines)
- ActionMoveToCurrentLayer (46 lines)

**Batch 2: Auto Actions (wrapper actions)**
- ActionAutoRoute (49 lines)
- ActionAutoPlaceSelected (42 lines)

**Batch 3: Object Modification (uses GetFunctionID)**
- ActionChangeHole (49 lines)
- ActionChangePaste (similar pattern)

### Phase 3: Medium Complexity Actions

After GetFunctionID is exported:
- ActionConnection
- ActionFlip
- ActionSetThermal
- ActionAddRats

### Phase 4: Complex Actions (Future)

Defer until we have more patterns established:
- ActionDisplay
- ActionMode
- ActionSelect/Unselect
- ActionUndo/Redo

---

## Immediate Action Items

### To Do Before Next Migration:

1. ✅ Export GetFunctionID from action.c
2. ✅ Test that existing C actions still work
3. ✅ Document the FunctionID enum values
4. ✅ Verify SearchScreen, GetValue availability

### Questions to Answer:

1. Is GetFunctionID worth keeping, or should we use C++11 string comparisons?
2. Should we create a C++ helper class for common action patterns?
3. Can we create base classes for different action types (ObjectAction, FileAction, etc.)?

---

## Risk Assessment

### Low Risk (Export Helpers)
- GetFunctionID export is low risk - already used everywhere
- No behavior change, just visibility

### Medium Risk (Object Modification Actions)
- These touch PCB data structures extensively
- Need careful testing
- Good candidates after helper export

### High Risk (State Machine Actions)
- Mode, Display, Undo have complex state
- 200-400+ lines each
- Save for later when patterns are proven

---

## Success Metrics

After this strategic phase:
- GetFunctionID exported and tested
- 4-6 more simple actions migrated
- Clear path for medium-complexity actions
- Total: ~20 actions (25-30% complete)

---

## Notes

- The 789-line ActionSetViaLayers is an outlier - may need to be refactored in place
- NotifyMode is 400+ lines itself - could be a separate refactoring project
- Consider creating action base classes for common patterns after we have ~20 examples
