# Action Migration Progress

**Date:** November 17, 2025
**Branch:** claude/refactor-action-dependencies-01A7QiJjB6c7sdmiSbqNkH4k

---

## Summary

**Total Actions:** 62
**Migrated:** 41 (66%) - **TWO-THIRDS COMPLETE!** 🎉
**Remaining:** 21 (34%)

---

## Migrated Actions

### Batch 1: Initial POC (Previous Session)
1. ✅ **Message** - Display messages to log window
2. ✅ **SaveSettings** - Save PCB settings
3. ✅ **DumpLibrary** - Dump library contents
4. ✅ **Quit** - Exit the application
5. ✅ **SetFlag** - Set object flags
6. ✅ **ClrFlag** - Clear object flags
7. ✅ **ChangeFlag** - Change object flags

### Batch 2: Simple Utility Actions (This Session)
8. ✅ **Atomic** - Group actions into single undo operation
9. ✅ **MarkCrosshair** - Set/reset crosshair mark
10. ✅ **ExecCommand** - Run system commands
11. ✅ **RemoveSelected** - Remove selected objects
12. ✅ **DeleteRats** - Delete rat lines (airwires)
13. ✅ **Polygon** - Polygon drawing operations
14. ✅ **RouteStyle** - Select routing styles

### Batch 3: GetFunctionID Validation (This Session)
15. ✅ **New** - Create new layout (file operation, 45 lines)
16. ✅ **MoveToCurrentLayer** - Move objects to current layer (validates GetFunctionID export, 29 lines)

### Batch 4: More GetFunctionID Actions (This Session)
17. ✅ **AutoPlaceSelected** - Auto-place selected elements (simple wrapper, 10 lines)
18. ✅ **AutoRoute** - Auto-route rat lines (uses GetFunctionID, 20 lines)
19. ✅ **ChangeHole** - Change via drill hole diameter (object modification, 30 lines)

### Batch 5: Object Manipulation and Connection Actions (This Session)
20. ✅ **Flip** - Flip elements to opposite board side (uses GetFunctionID, 35 lines)
21. ✅ **AddRats** - Add rat lines/airwires (uses GetFunctionID, 50 lines)
22. ✅ **MorphPolygon** - Morph polygons to simplify (uses GetFunctionID, 37 lines)
23. ✅ **Connection** - Find and highlight connections (uses GetFunctionID, 45 lines)

### Batch 6: Flag Manipulation Actions (This Session)
24. ✅ **ChangeSquare** - Toggle square flag on pins/pads (~40 lines)
25. ✅ **SetSquare** - Set square flag on pins/pads (~40 lines)
26. ✅ **ClearSquare** - Clear square flag on pins/pads (~40 lines)
27. ✅ **ChangeOctagon** - Toggle octagon flag on pins/vias (~43 lines)
28. ✅ **SetOctagon** - Set octagon flag on pins/vias (~43 lines)
29. ✅ **ClearOctagon** - Clear octagon flag on pins/vias (~43 lines)
30. ✅ **ChangeJoin** - Toggle join/clearance flag on lines/arcs (~42 lines)
31. ✅ **ChangePaste** - Toggle paste flag on pads (~37 lines)
32. ✅ **ToggleHideName** - Toggle element name visibility (~45 lines, uses ELEMENT_LOOP)

### Batch 7: Size/Clearance Actions (This Session)
33. ✅ **ChangeSize** - Change object dimensions (~80 lines, uses GetFunctionID)
34. ✅ **Change2ndSize (ChangeDrillSize)** - Change drill hole sizes (~50 lines, uses GetFunctionID)
35. ✅ **ChangeClearSize** - Change clearance around objects (~60 lines, uses GetFunctionID)
36. ✅ **MinMaskGap** - Ensure minimum solder mask clearance (~80 lines, loops through pins/pads/vias)
37. ✅ **MinClearGap** - Ensure minimum polygon clearance (~90 lines, loops through all objects)
38. ✅ **SetThermal** - Set thermal relief style (~60 lines, uses GetFunctionID)

### Batch 8: Name/Value Actions (This Session)
39. ✅ **ChangePinName** - Change pin names on elements (~70 lines, uses ELEMENT_LOOP, PIN_LOOP, PAD_LOOP)
40. ✅ **SetValue** - Set board-wide values (grid, line size, via size, etc.) (~95 lines, uses GetFunctionID)
41. ✅ **SetSame** - Copy properties from clicked object (~80 lines, uses SearchScreen)

---

## Migration Patterns Established

### 1. File Structure
```cpp
#include "Action.h"

extern "C" {
#include "global.h"
#include "specific_headers.h"  // Only what's needed
}

namespace pcb {
namespace actions {

class MyAction : public Action {
public:
    MyAction() : Action("Name", "Help", "Syntax") {}
    int execute(int argc, char** argv, Coord x, Coord y) override {
        // Implementation
    }
};

REGISTER_ACTION(MyAction);

}} // namespace
```

### 2. Dependency Management
- ✅ Use forward declarations when possible
- ✅ Include only stable C interfaces
- ✅ Avoid pulling in heavy headers
- ✅ Keep implementations isolated

### 3. Testing Strategy
- Layer 2: Standalone C++ unit tests
- Layer 3: Integration tests with C code
- All tests passing for migrated actions

---

## Next Steps - Recommended Actions

### Easy Wins (Simple, Low Dependencies)
These actions are good candidates for the next batch:

**Utility Actions:**
- `Attributes` - Edit object attributes (uses gui->edit_attributes)
- `Import` - Import schematics (complex, skip for now)
  
**Display/UI Actions:**
- `Display` - Toggle display elements
- `Mode` - Change editor mode

**File Actions (Medium complexity):**
- `SaveTo` - Save PCB to file
- `LoadFrom` - Load PCB from file
- ✅ `New` - Create new layout (COMPLETED)

### Complex Actions (Save for Later)
These require more refactoring:
- `Undo`/`Redo` - Complex state machine logic
- `Select`/`Unselect` - Complex object traversal
- ✅ `ChangeSize`/`ChangeClearSize` - Object modification patterns (COMPLETED)
- `ExecuteFile` - Needs defer_updates static variable handling

---

## Build Integration

All migrated actions are:
- ✅ Added to `src/Makefile.am`
- ✅ Listed in alphabetical order
- ✅ Auto-registered via `REGISTER_ACTION` macro
- ✅ Dispatched through `hid_actionv()` with C fallback

---

## File Statistics

**Original action.c:** 8,466 lines
**Migrated to C++:** ~3,582 lines (estimated)
**Reduction:** ~42%

## Helper Function Exports

Successfully exported helper functions from action.c for use by C++ actions:
- ✅ **ChangeFlag** - Flag modification (used by SetFlag, ClrFlag, ChangeFlag)
- ✅ **ClearWarnings** - Clear warning flags (used by DeleteRats)
- ✅ **GetFunctionID** - String to enum lookup (validated by MoveToCurrentLayer, unlocks ~36 actions)

---

## Documentation

All migrated actions include:
- Doxygen comments
- Help text
- Syntax examples
- Migration notes in original action.c

---

## Testing Status

**Layer 2 (Standalone C++):**
- MessageAction: 5/5 tests passing
- SaveSettingsAction: 8/8 tests passing
- Simple tests: All passing

**Layer 3 (Integration):**
- 7/7 integration tests passing
- C code successfully calling C++ actions
- Fallback mechanism verified

---

## Next Session Goals

1. **Migrate 5-10 more simple actions**
   - Focus on actions with <50 lines
   - Minimal global state dependencies
   - Clear, simple logic

2. **Create more tests**
   - Add unit tests for new actions
   - Expand integration test coverage

3. **Document patterns**
   - Update pattern library
   - Add examples for common cases

4. **Consider grouping**
   - File-related actions → FileActions.cpp?
   - Flag-related actions already grouped
   - Display-related actions → DisplayActions.cpp?

---

## Known Issues

None currently. All migrated actions compile and integrate correctly.

---

## References

- [ACTION_REFACTORING_PROPOSAL.md](ACTION_REFACTORING_PROPOSAL.md) - Full refactoring strategy
- [MIGRATION_DEPENDENCY_STRATEGY.md](MIGRATION_DEPENDENCY_STRATEGY.md) - Dependency management
- [SESSION_SUMMARY.md](SESSION_SUMMARY.md) - Previous session summary
