# Action Migration Progress

**Date:** November 19, 2025
**Branch:** claude/fix-savebufferelements-error-01Sqseb6JL3wUpGkFNQjHSko

---

## Summary

**Total Actions:** 65
**Migrated:** 63 (97%) - **HYBRID APPROACH COMPLETE!** 🎉
**Remaining:** 2 (3%) - Deferred pending NotifyMode() refactoring

---

## Major Milestone: ActionContext Pattern

### Phase 1: Infrastructure (November 19, 2025)

Created **ActionContext** pattern to centralize shared static state, enabling the final action migrations:

**File:** `src/actions/ActionContext.h`

**Purpose:** Replaces file-scoped static variables in action.c with a centralized context accessible to both C and C++ code.

**Shared State Managed:**
- `Note` struct - Mouse/selection state (84 references)
- `lastLayer` - Undo tracking for layer operations (6 references)
- `addedLines` - Line creation tracking (8 references, moved from global)
- `defer_updates`/`defer_needs_update` - Update batching (6 references)
- `InsertedPoint`, `polyIndex`, `saved_mode` - Polygon editing state (19 references)
- `fake` struct - Temporary geometry for operations (6 references)
- `mid_stroke`, `StrokeBox` - Libstroke gesture state (11 references)

**Usage:**
```c
// In C code (action.c)
pcb_action_context->lastLayer = CURRENT;
pcb_action_context->addedLines++;

// In C++ code (UndoAction.cpp)
extern "C" {
#include "actions/ActionContext.h"
}
pcb_action_context->addedLines--;
```

**Benefits:**
- ✅ Enables C++ actions to access shared state
- ✅ Documents all shared state in one location
- ✅ Improves testability (can create mock contexts)
- ✅ Foundation for future multi-document support
- ✅ Unblocked migration of ActionUndo, ActionRenumber, ActionImport

---

## Migrated Actions

### Batch 1-10: Previous Sessions (52 actions)
See previous documentation for actions 1-52.

### Batch 11: Quick Wins Enabled by ActionContext (November 19, 2025)
53. ✅ **DisperseElements** - Scatter selected elements (~126 lines, grid layout algorithm)
54. ✅ **ElementList** - Library element management (~254 lines, footprint cache)
55. ✅ **ExecuteFile** - Execute action script file (~84 lines, uses defer_updates from ActionContext)
56. ✅ **Select** - Select objects by pattern/type (~169 lines, uses NotifyBlock)
57. ✅ **SetViaLayers** - Set via layer range (~132 lines, layer identification)
58. ✅ **Unselect** - Unselect objects (~151 lines, mirrors Select logic)

**Key Achievement:** Exported `NotifyBlock()` from action.c for Select/Unselect

### Batch 12: Phase 2 - Actions Unblocked by ActionContext (November 19, 2025)
59. ✅ **Undo** - Undo recent changes (~204 lines)
   - Uses `pcb_action_context->addedLines` for line creation tracking
   - Uses `pcb_action_context->lastLayer` for undo tracking
   - Mode-specific undo logic (POLYGON, LINE, ARC modes)
   - Multi-segment line undo with intermediate point removal
   - ClearList sub-command for releasing undo memory

60. ✅ **Renumber** - Renumber elements by board position (~365 lines)
   - Self-contained element renumbering algorithm
   - Sorts elements by Y then X coordinates
   - Handles locked elements (preserves their refdes)
   - Writes annotation file for schematic backannotation
   - Updates netlist with renamed element references
   - Sequential numbering per prefix (U1, U2, R1, R2, etc.)

61. ✅ **Import** - Import schematics via gnetlist/make (~343 lines)
   - Schematic import workflow (gnetlist or make mode)
   - setdisperse sub-command for element placement dispersion
   - setnewpoint sub-command for placement origin
   - Spawns external processes (gnetlist/make)
   - Calls ActionExecuteFile to process imported data
   - Updates rats nest after import

### Batch 13: Phase 3 - Display Action (November 19, 2025)
62. ✅ **Display** - Comprehensive display toggles and settings (~428 lines)
   - **Element name modes:** NameOnPCB, Description, Value
   - **Grid controls:** Grid, ToggleGrid (with offset support)
   - **Redraw:** Redraw, ClearAndRedraw
   - **Crosshair:** CycleCrosshair, CycleClip
   - **Direction/movement:** ToggleAllDirections, ToggleStartDirection, ToggleOrthoMove, ToggleRubberBandMode, ToggleSnapPin
   - **Display:** ToggleMask, ToggleName, ToggleHideNames, ToggleThindraw, ToggleThindrawPoly, ToggleCheckPlanes
   - **DRC:** ToggleShowDRC, ToggleAutoDRC (with connection lookup)
   - **Drawing modes:** ToggleClearLine, ToggleFullPoly
   - **Name management:** ToggleLockNames, ToggleOnlyNames, ToggleUniqueNames, ToggleLocalRef
   - **Interactive:** Pinout, PinOrPadName
   - **Other:** ToggleAutoBuriedVias, ToggleLiveRoute

63. ✅ **PasteBuffer** - Buffer operations (add, clear, rotate, convert, save, paste) (~165 lines)

---

## Remaining Actions (Deferred)

### Blocked by NotifyMode() Refactoring (2 actions, 3%)

These actions require the 816-line `NotifyMode()` state machine to be refactored:

1. **ActionMode** (212 lines) - Editor mode switching
   - Calls `NotifyMode()` for mode transitions
   - Requires NotifyMode to be refactored using State Pattern
   - Estimated effort: 2-3 weeks as separate project

2. **ActionDelete** (39 lines) - Delete objects at cursor
   - Calls `NotifyMode()` for delete operations
   - Simple wrapper, but tightly coupled to NotifyMode
   - Will be easy to migrate once NotifyMode is refactored

**Strategic Decision:** Per `FINAL_ACTIONS_MIGRATION_STRATEGY.md`, these 2 actions are explicitly deferred. The Hybrid Approach (Option 4) targets 97% migration, leaving NotifyMode refactoring as a future architectural improvement project.

---

## Migration Patterns Established

### 1. File Structure
```cpp
#include "Action.h"

extern "C" {
#include "global.h"
#include "actions/ActionContext.h"  // For shared state access
#include "specific_headers.h"
}

namespace pcb {
namespace actions {

class MyAction : public Action {
public:
    MyAction() : Action("Name", "Help", "Syntax") {}
    int execute(int argc, char** argv, Coord x, Coord y) override {
        // Can access pcb_action_context->field for shared state
        // Implementation
    }
};

REGISTER_ACTION(MyAction);

}} // namespace
```

### 2. ActionContext Usage Pattern
```cpp
// Accessing shared state from C++ actions
extern "C" {
#include "actions/ActionContext.h"
}

int execute(...) {
    // Read shared state
    if (pcb_action_context->addedLines > 0) {
        // ...
    }

    // Modify shared state
    pcb_action_context->lastLayer = CURRENT;
    pcb_action_context->addedLines++;
}
```

### 3. Dependency Management
- ✅ Use forward declarations when possible
- ✅ Include only stable C interfaces
- ✅ Use ActionContext for shared state
- ✅ Export minimal helper functions from action.c
- ✅ Keep implementations isolated

### 4. Testing Strategy
- Layer 1: Isolated C++ implementation
- Layer 2: Standalone C++ unit tests
- Layer 3: Integration tests with C code
- All tests passing for migrated actions

---

## Helper Function Exports

Successfully exported from action.c for use by C++ actions:
- ✅ **ChangeFlag** - Flag modification (used by SetFlag, ClrFlag, ChangeFlag)
- ✅ **ClearWarnings** - Clear warning flags (used by DeleteRats)
- ✅ **GetFunctionID** - String to enum lookup (used by ~40 actions)
- ✅ **NotifyBlock** - Block selection notification (used by Select, Unselect)

---

## Build Integration

All migrated actions are:
- ✅ Added to `src/Makefile.am` in alphabetical order
- ✅ Auto-registered via `REGISTER_ACTION` macro
- ✅ Dispatched through `hid_actionv()` with C fallback
- ✅ Include migration notes in original action.c

---

## File Statistics

**Original action.c:** 8,466 lines
**Migrated to C++:** ~5,800 lines (63 actions)
**Reduction:** ~68%

**New Infrastructure:**
- `src/actions/ActionContext.h` - 169 lines (shared state management)
- 63 action implementation files

---

## Hybrid Approach Summary

Successfully completed all 3 phases:

### Phase 1: Action Context Pattern ✅
- Created ActionContext infrastructure
- Migrated ~140 static variable references
- Enabled C++ actions to access shared state

### Phase 2: Unblocked Actions ✅
- Migrated Undo (uses ActionContext)
- Migrated Renumber (self-contained)
- Migrated Import (schematic workflow)

### Phase 3: Display Action ✅
- Migrated comprehensive Display action (30+ toggles)
- Preserved cohesive organization
- Single maintainable file vs 40+ individual files

**Result:** 97% migration achieved as planned! 🎯

---

## Documentation

All migrated actions include:
- Comprehensive Doxygen comments
- Help text preserved from original
- Syntax examples
- Migration notes in original action.c
- ActionContext usage documented

Additional documentation:
- ✅ `FINAL_ACTIONS_MIGRATION_STRATEGY.md` - Strategic analysis
- ✅ `ACTION_REFACTORING_PROPOSAL.md` - Original proposal
- ✅ `MIGRATION_DEPENDENCY_STRATEGY.md` - Dependency management
- ✅ `STRATEGIC_REFACTORING_OPPORTUNITIES.md` - Future work

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
- ActionContext integration verified

---

## Future Work

### Deferred Items (NotifyMode Refactoring Project)
1. **Refactor NotifyMode()** (816 lines)
   - Apply State Pattern for mode state machine
   - Extract mode-specific logic into separate classes
   - Estimated effort: 2-3 weeks

2. **Migrate final 2 actions** (once NotifyMode is refactored)
   - ActionMode (212 lines)
   - ActionDelete (39 lines)
   - Estimated effort: 1-2 days after NotifyMode refactoring

### Potential Enhancements
- Multi-document support using ActionContext
- Unit tests for complex actions (Undo, Import, Renumber)
- Further modularization of large actions
- Performance profiling of C++ vs C implementations

---

## Known Issues

None currently. All 63 migrated actions compile and integrate correctly.

---

## References

- [FINAL_ACTIONS_MIGRATION_STRATEGY.md](FINAL_ACTIONS_MIGRATION_STRATEGY.md) - Hybrid Approach strategy
- [ACTION_REFACTORING_PROPOSAL.md](ACTION_REFACTORING_PROPOSAL.md) - Full refactoring strategy
- [MIGRATION_DEPENDENCY_STRATEGY.md](MIGRATION_DEPENDENCY_STRATEGY.md) - Dependency management
- [STRATEGIC_REFACTORING_OPPORTUNITIES.md](STRATEGIC_REFACTORING_OPPORTUNITIES.md) - Future opportunities
- [src/actions/ActionContext.h](src/actions/ActionContext.h) - Shared state infrastructure
