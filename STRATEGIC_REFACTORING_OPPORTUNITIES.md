# Strategic Refactoring Opportunities for Remaining Actions

**Date:** November 17, 2025
**Current Progress:** 23/62 actions migrated (37%)
**Remaining:** 39 actions (63%)

---

## Executive Summary

After migrating 23 actions and successfully exporting GetFunctionID, we've identified several strategic refactorings that would significantly ease migration of the remaining 39 actions.

---

## 1. Static Helper Functions Analysis

### Currently Exported (Successfully Used)
- ✅ **GetFunctionID()** - String to FunctionID enum (used by 9 migrated actions)
- ✅ **ChangeFlag()** - Flag modification (used by SetFlag, ClrFlag, ChangeFlag)
- ✅ **ClearWarnings()** - Clear rat warnings (used by DeleteRats, AddRats)

### Static Helpers Still in action.c

#### High Priority - Worth Exporting

**1. NotifyMode() - 816 lines!**
- **Usage:** Called 4 times in action.c
- **Complexity:** Massive function (~816 lines) - essentially a state machine
- **Recommendation:** **DO NOT EXPORT** - This is too complex and stateful
- **Alternative:**
  - Leave it in action.c for now
  - Actions that need it can remain in C
  - Consider refactoring NotifyMode itself as a separate project
- **Actions affected:** ActionMode uses this heavily

**2. NotifyLine() - Unknown size**
- **Usage:** Called ~7 times
- **Used by:** Mode-related operations, polygon hole creation
- **Recommendation:** Analyze size; if < 100 lines, consider exporting
- **Impact:** Medium - affects mode-switching actions

**3. NotifyBlock() - Unknown size**
- **Usage:** Called ~4 times
- **Used by:** Block/area selection operations
- **Recommendation:** Analyze size; if < 100 lines, consider exporting
- **Impact:** Low-Medium

**4. AdjustAttachedBox() - Unknown size**
- **Usage:** Called ~3 times
- **Used by:** Drawing operations with attached objects
- **Recommendation:** Analyze size and dependencies
- **Impact:** Low

### Already Exported in Other Headers
- ✅ **SetMode()** - Change editor mode (set.h)
- ✅ **SaveMode()** / **RestoreMode()** - Mode stack (set.h)
- ✅ **SearchScreen()** - Find objects (search.h) - used by many actions
- ✅ **IncrementUndoSerialNumber()** - Undo management (undo.h)

---

## 2. Remaining Actions by Complexity

### Simple Actions (< 50 lines) - Good Candidates

**Object Property Changes (All use GetFunctionID):**
1. **ActionSetThermal** - Set thermal relief on pads/pins
2. **ActionToggleHideName** - Toggle element name visibility
3. **ActionChangeJoin** - Change line join style
4. **ActionChangeSquare** - Change square flag
5. **ActionSetSquare** - Set square flag
6. **ActionClearSquare** - Clear square flag
7. **ActionChangeOctagon** - Change octagon flag
8. **ActionSetOctagon** - Set octagon flag
9. **ActionClearOctagon** - Clear octagon flag
10. **ActionChangePaste** - Change paste flag

**Utility Actions:**
11. **ActionRipUp** - Rip up auto-routed traces
12. **ActionSetSame** - Copy properties from object
13. **ActionPSCalib** - PostScript calibration
14. **ActionAttributes** - Edit object attributes
15. **ActionElementSetAttr** - Set element attributes

### Medium Actions (50-150 lines)

**Size/Clearance Changes (Use GetFunctionID):**
1. **ActionChangeSize** - Change object size
2. **ActionChange2ndSize** - Change second size parameter
3. **ActionChangeClearSize** - Change clearance
4. **ActionMinMaskGap** - Minimum mask gap
5. **ActionMinClearGap** - Minimum clearance gap

**Name/Value Changes:**
6. **ActionChangePinName** - Change pin names
7. **ActionSetValue** - Set element values

**File Operations:**
8. **ActionSaveTo** - Save to file
9. **ActionLoadFrom** - Load from file
10. **ActionExecuteFile** - Execute action script

**Buffer/Clipboard:**
11. **ActionPasteBuffer** - Paste buffer operations
12. **ActionMoveObject** - Move objects

**Other:**
13. **ActionDelete** - Delete objects (uses GetFunctionID)
14. **ActionElementList** - List elements
15. **ActionDisperseElements** - Scatter elements

### Complex Actions (> 150 lines) - Defer for Later

**The Big Three:**
1. **ActionDisplay** (398 lines) - Toggle display elements (30+ cases)
2. **ActionMode** (226 lines) - Mode switching state machine
3. **ActionSelect** / **ActionUnselect** (205 lines) - Selection logic

**Heavy File Operations:**
4. **ActionImport** (311 lines) - Schematic import
5. **ActionRenumber** (371 lines) - Element renumbering

**The Monster:**
6. **ActionSetViaLayers** (789 lines!) - Via layer selection UI

**Undo/Redo:**
7. **ActionUndo** / **ActionRedo** (178 lines) - Undo state machine

---

## 3. Recommended Refactorings

### Option A: Export Small Helper Functions (Low Effort, Medium Gain)

**Candidates to Analyze:**
```bash
# Check sizes of these functions:
NotifyLine()
NotifyBlock()
AdjustAttachedBox()
```

**If < 100 lines each:**
- Export them like we did with GetFunctionID
- Add to action.h
- Document their purpose
- **Estimated effort:** 1-2 hours
- **Actions unlocked:** 5-10 medium actions

### Option B: Create Action Base Classes (Medium Effort, High Gain)

Based on patterns we've seen, create specialized base classes:

**1. FlagAction** - For actions that set/clear/change flags
```cpp
class FlagAction : public Action {
protected:
    virtual void applyToObject(...) = 0;
    virtual void applyToSelected(...) = 0;
public:
    int execute(int argc, char** argv, Coord x, Coord y) override {
        // Common GetFunctionID dispatch logic
        // Call applyToObject or applyToSelected
    }
};
```
**Actions that benefit:**
- ChangeSquare, SetSquare, ClearSquare
- ChangeOctagon, SetOctagon, ClearOctagon
- ChangeJoin, ChangePaste
- **Count:** ~10 actions

**2. SizeChangeAction** - For size/clearance modifications
```cpp
class SizeChangeAction : public Action {
protected:
    virtual bool changeObject(void* obj, Coord amount) = 0;
    virtual bool changeSelected(Coord amount) = 0;
};
```
**Actions that benefit:**
- ChangeSize, Change2ndSize, ChangeClearSize
- MinMaskGap, MinClearGap
- **Count:** ~5 actions

**3. FileAction** - For save/load operations
**Actions that benefit:**
- SaveTo, LoadFrom, ExecuteFile
- **Count:** ~3 actions

**Estimated effort:** 1-2 days
**Actions simplified:** ~20 actions

### Option C: Do Nothing (Current Approach)

Continue migrating actions one-by-one with current patterns.
- **Pros:** Working well, no risk, steady progress
- **Cons:** Repetitive code, no abstraction benefits
- **Remaining effort:** ~20-30 hours for remaining simple/medium actions

---

## 4. High-Impact, Low-Effort Wins

### Immediate Opportunities (Next 2-3 Hours)

**Batch 6: Flag Actions** (All similar, ~30 lines each)
1. ChangeSquare, SetSquare, ClearSquare
2. ChangeOctagon, SetOctagon, ClearOctagon
3. ChangeJoin
4. ChangePaste
5. ToggleHideName

**Estimated:** 9 actions, ~270 lines of C++ code
**Complexity:** Low - all follow same pattern
**Would bring us to:** 32/62 actions (52%)

### Medium-Term (Next 5-10 Hours)

**Batch 7: Size Change Actions**
1. ChangeSize
2. Change2ndSize
3. ChangeClearSize
4. MinMaskGap
5. MinClearGap
6. SetThermal

**Batch 8: Name/Value Actions**
1. ChangePinName
2. SetValue
3. SetSame

**Estimated:** 9 more actions
**Would bring us to:** 41/62 actions (66%)

---

## 5. Actions to Explicitly Defer

These require substantial work or depend on NotifyMode:

**Definitely Later:**
- ActionDisplay (398 lines, 30+ display toggles)
- ActionMode (226 lines, uses NotifyMode extensively)
- ActionSelect/Unselect (205 lines, complex selection logic)
- ActionImport (311 lines, external process management)
- ActionRenumber (371 lines, complex file I/O)
- ActionSetViaLayers (789 lines! - needs UI refactoring)
- ActionUndo/Redo (178 lines, complex state machine)
- ActionDisperseElements (267 lines, complex layout algorithm)

**Recommended:** Leave these in C for now, migrate later as part of larger refactoring efforts.

---

## 6. Recommended Strategy

### Phase 1: Continue Current Approach (This Session)
- Migrate the 9 flag actions (Batch 6)
- **Effort:** 2-3 hours
- **Progress:** 32/62 (52%)

### Phase 2: Size/Name Actions (Next Session)
- Migrate size change and name/value actions (Batches 7-8)
- **Effort:** 5-10 hours
- **Progress:** 41/62 (66%)

### Phase 3: Medium Complexity (Future Session)
- File operations, buffer operations, element lists
- **Effort:** 10-15 hours
- **Progress:** 50/62 (81%)

### Phase 4: Complex Actions (Future Project)
- Defer Display, Mode, Select, Import, SetViaLayers
- These need architectural decisions
- May benefit from waiting for more C++ infrastructure

---

## 7. Key Insights

### What's Working Well
1. **GetFunctionID export** - Validated by 9 actions, unlocked switch/case patterns
2. **Minimal includes** - Using forward declarations keeps compilation fast
3. **Action auto-registration** - REGISTER_ACTION macro works perfectly
4. **C fallback** - Smooth transition, no breaking changes

### What Would Help Most
1. **Base classes for common patterns** - Would eliminate repetitive code
2. **Export NotifyLine/NotifyBlock** - If they're small enough (<100 lines)
3. **Better documentation** - For complex actions, understanding intent is hardest part

### What to Avoid
1. **Don't export NotifyMode** - It's 816 lines and too complex
2. **Don't force ActionMode migration yet** - Too dependent on NotifyMode
3. **Don't tackle SetViaLayers yet** - 789 lines, needs UI rethinking

---

## 8. Decision Points for User

**Question 1:** Should we export NotifyLine() and NotifyBlock()?
- Requires analyzing their size and dependencies
- If < 100 lines each, probably worth it
- Could unlock several mode-related actions

**Question 2:** Should we create base classes (FlagAction, SizeChangeAction)?
- Would reduce duplication significantly
- Makes pattern more explicit
- Requires 1-2 days of work upfront
- Pays off for ~20 actions

**Question 3:** Continue with current one-by-one approach?
- Working well, proven, low risk
- Good for learning patterns
- But repetitive for similar actions

---

## Recommendation

**For this session:**
Continue current approach and migrate the 9 flag actions (Batch 6). They're simple, similar, and will push us past 50% completion.

**For next session:**
Consider creating FlagAction and SizeChangeAction base classes, then use them to migrate Batches 7-8.

**For later:**
Defer the complex actions (Display, Mode, Select, Import, SetViaLayers) until we have 50-60 actions migrated and can better assess if they need special handling.
