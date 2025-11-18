# Milestone 4: Dialogs, Widgets & Final Polish - Overview

**Total Duration**: 8 days (64 hours)
**Prerequisites**: Milestones 1, 2, and 3 completed
**Goal**: Complete the GTK3 HID by migrating all dialogs, custom widgets, specialized windows, and conducting final integration testing

---

## Overview

Milestone 4 represents the final phase of the GTK3 HID implementation. While Milestones 1-3 established the foundation (initialization, drawing, and OpenGL rendering), Milestone 4 completes the user-facing functionality by migrating:

- **Dialogs**: Configuration dialogs, print dialogs, and various popup windows
- **Custom Widgets**: PCB-specific widgets like layer selector, route style selector, coordinate entry
- **Specialized Windows**: Library window, netlist window, DRC window (all tree-view based)
- **Final Polish**: Integration testing, bug fixes, documentation, and release preparation

After completing Milestone 4, the GTK3 HID will be feature-complete and ready for production use.

---

## Why Break Into Sub-Milestones?

Milestone 4 covers **~126 hours of work** across many different files (dialogs, widgets, windows). Breaking this into smaller milestones provides:

1. **Manageable chunks**: Each sub-milestone is 2 days (16 hours), easier to plan and execute
2. **Clear deliverables**: Each sub-milestone has specific, testable outcomes
3. **Early wins**: Can test and use functionality as each sub-milestone completes
4. **Risk reduction**: Issues discovered early in one area don't block progress in others
5. **Better tracking**: Easier to monitor progress and adjust plans

---

## Sub-Milestone Breakdown

### Milestone 4A: Dialogs and Menus (2 days, 16 hours)

**Focus**: Migrate core dialog infrastructure and menu system

**Files**:
- `gui-dialog.c` (800 lines, 8 hours) - Core dialog infrastructure
- `ghid-main-menu.c` (900 lines, 8 hours) - Main menu system

**Key Changes**:
- GtkTable → GtkGrid in dialog layouts
- Deprecated widget replacements (GtkCombo → GtkComboBoxText)
- Menu accelerator updates (GDK_KEY_* constants)
- Signal signature updates

**Deliverable**: All dialogs display correctly with proper layouts, menus work with correct keyboard shortcuts

---

### Milestone 4B: Custom Widgets (2 days, 16 hours)

**Focus**: Migrate PCB-specific custom widgets

**Files**:
- `ghid-layer-selector.c` (1,500 lines, 10 hours) - Layer visibility/selection widget
- `ghid-route-style-selector.c` (1,000 lines, 6 hours) - Route style chooser widget

**Key Changes**:
- Custom drawing with Cairo (expose-event → draw signal)
- Tree view cell renderer updates
- Custom widget state management with GtkStyleContext
- Color picker widget updates (GtkColorButton compatibility)

**Deliverable**: Layer selector works (toggle visibility, select layers), route style selector works (choose/edit styles)

---

### Milestone 4C: Specialized Windows (2 days, 16 hours)

**Focus**: Migrate tree-view based specialized windows

**Files**:
- `gui-library-window.c` (1,200 lines, 8 hours) - Component library browser
- `gui-netlist-window.c` (1,300 lines, 6 hours) - Netlist viewer/editor
- `gui-drc-window.c` (1,000 lines, 4 hours) - Design rule check results

**Key Changes**:
- GtkTreeView updates (cell renderers, column setup)
- GtkTreeModel compatibility (GtkListStore, GtkTreeStore)
- Window layout updates (GtkTable → GtkGrid)
- Search/filter functionality updates

**Deliverable**: Library window browses components, netlist window displays/edits netlist, DRC window shows violations

---

### Milestone 4D: Configuration & Remaining Files (2 days, 16 hours)

**Focus**: Preferences dialog and remaining utility files

**Files**:
- `gui-config.c` (1,800 lines, 10 hours) - Preferences dialog with multiple tabs
- Remaining `gui-*.c` files (~2-3 small files, 4 hours)
- Remaining `ghid-*.c` files (~2-3 small files, 2 hours)

**Key Changes**:
- Multi-tab preference dialog (GtkNotebook still works, but verify)
- Configuration value widgets (spin buttons, check boxes, color pickers)
- Font selection dialog updates
- Keyboard shortcut configuration

**Deliverable**: Preferences dialog works with all tabs functional, all configuration options accessible

---

### Milestone 4E: Final Integration & Testing (2 days, 16 hours)

**Focus**: Comprehensive testing, bug fixes, and documentation

**Activities**:
- **Integration testing** (6 hours): Test all features together, find integration issues
- **Bug fixing** (6 hours): Fix issues found during testing
- **Platform testing** (2 hours): Test on Linux, macOS (if available), Windows (if available)
- **Documentation** (2 hours): Update README, user guide, release notes

**Key Testing Areas**:
- All dialogs open and close correctly
- All custom widgets render and respond to user input
- Menu items trigger correct actions
- Keyboard shortcuts work
- No memory leaks (valgrind)
- No crashes during normal use

**Deliverable**: GTK3 HID is stable, tested, documented, and ready for release

---

## Architecture: Dialogs vs Widgets vs Windows

Understanding the distinction helps with migration planning:

### Dialogs (`gui-dialog*.c`)
- **Purpose**: Modal or non-modal popup windows for user interaction
- **Characteristics**:
  - Usually GtkDialog subclass
  - Buttons (OK, Cancel, Apply)
  - Form-based layouts (labels + entry fields)
- **Examples**: Preferences, Print setup, Export options
- **GTK3 Changes**: Mostly layout (GtkTable → GtkGrid), deprecated widget replacements

### Custom Widgets (`ghid-*.c`)
- **Purpose**: PCB-specific reusable UI components
- **Characteristics**:
  - Subclass of GtkWidget or GtkBox
  - Custom drawing (Cairo in GTK3)
  - Complex internal state
  - Emit custom signals
- **Examples**: Layer selector, Route style selector, Coordinate entry
- **GTK3 Changes**: Drawing code (GDK → Cairo), style context (theming), signal updates

### Specialized Windows (`gui-*-window.c`)
- **Purpose**: Non-modal windows with complex data display
- **Characteristics**:
  - Usually GtkWindow subclass
  - Often contain GtkTreeView for data
  - May have toolbar, search bar
  - Complex data models
- **Examples**: Library browser, Netlist window, DRC results
- **GTK3 Changes**: Tree view updates, layout changes, window management

---

## Critical Success Factors

For Milestone 4 to succeed:

1. **Test incrementally**: Don't wait until end to test dialogs/widgets
2. **Maintain parallel development**: All code in `src/hid/gtk3/`, use `ghid3_` prefix
3. **Visual parity**: Compare with GTK2 HID to ensure nothing is missing
4. **User workflows**: Test actual user workflows (e.g., "add component from library")
5. **Keyboard shortcuts**: Verify all shortcuts work (critical for power users)

---

## Timeline Visualization

```
Milestone 4: Dialogs, Widgets & Final Polish (8 days)

Week 1 (Days 1-5):
Day 1-2: Milestone 4A - Dialogs and Menus
├─ gui-dialog.c      ████████ (8 hours)
└─ ghid-main-menu.c  ████████ (8 hours)

Day 3-4: Milestone 4B - Custom Widgets
├─ ghid-layer-selector.c       █████████ (10 hours)
└─ ghid-route-style-selector.c ██████    (6 hours)

Day 5: Milestone 4C - Specialized Windows (Part 1)
└─ gui-library-window.c        ████████  (8 hours)

Week 2 (Days 6-10):
Day 6: Milestone 4C - Specialized Windows (Part 2)
├─ gui-netlist-window.c  ██████ (6 hours)
└─ gui-drc-window.c      ████   (4 hours)

Day 7-8: Milestone 4D - Configuration & Remaining Files
├─ gui-config.c          █████████ (10 hours)
└─ Remaining files       ██████    (6 hours)

Day 9-10: Milestone 4E - Final Integration & Testing
├─ Integration testing   ██████  (6 hours)
├─ Bug fixing           ██████  (6 hours)
├─ Platform testing     ██      (2 hours)
└─ Documentation        ██      (2 hours)
```

**Total**: 10 days (80 hours) for complete Milestone 4 with testing

---

## Risk Assessment

### Risk 1: Dialog Layout Issues

**Probability**: Medium (40%)
**Impact**: Low
**Description**: GtkTable → GtkGrid conversion may cause layout glitches

**Mitigation**:
- Test each dialog immediately after migration
- Use `gtk_grid_set_row_homogeneous()` and column properties to match GtkTable behavior
- Create helper function for common table→grid patterns
- Keep GTK2 version open for visual comparison

### Risk 2: Custom Widget Drawing Bugs

**Probability**: Medium (35%)
**Impact**: Medium
**Description**: Cairo drawing may look different than GDK

**Mitigation**:
- Compare pixel-by-pixel with GTK2 version
- Test with different themes
- Verify colors, line widths, and anti-aliasing
- Use Cairo save/restore for clean state management

### Risk 3: Tree View Performance

**Probability**: Low (15%)
**Impact**: Low
**Description**: Large tree views (library, netlist) may be slow

**Mitigation**:
- Profile with large data sets
- Use lazy loading if needed
- Enable GtkTreeView optimizations (fixed-height mode)
- Consider virtual scrolling for huge lists

### Risk 4: Missing Functionality

**Probability**: Low (20%)
**Impact**: High
**Description**: Some feature works in GTK2 but not GTK3

**Mitigation**:
- Create comprehensive feature checklist
- Test every menu item, button, and widget
- Compare feature-by-feature with GTK2 HID
- Get user feedback early (beta testing)

---

## Dependencies Between Sub-Milestones

```
Milestone 1 (Foundation)
    ↓
Milestone 2 (Drawing)
    ↓
Milestone 3 (OpenGL)
    ↓
Milestone 4A (Dialogs) ───→ Milestone 4B (Widgets)
                        ↘              ↓
                         Milestone 4C (Windows)
                                      ↓
                         Milestone 4D (Config)
                                      ↓
                         Milestone 4E (Testing)
                                      ↓
                              GTK3 HID Complete!
```

**Key Dependency Notes**:
- 4A must complete before 4B (dialogs use basic infrastructure)
- 4B and 4C can partially overlap (widgets don't depend on windows)
- 4D depends on 4A-4C (config uses all components)
- 4E depends on everything (testing requires complete system)

---

## What You'll Be Able to Do After Milestone 4

After completing all of Milestone 4, the GTK3 HID will be **feature-complete**:

### User-Facing Features
- ✅ Open/save PCB files (dialogs work)
- ✅ Edit board properties (preferences dialog works)
- ✅ Toggle layer visibility (layer selector works)
- ✅ Change route styles (route style selector works)
- ✅ Browse and add components (library window works)
- ✅ View and edit netlist (netlist window works)
- ✅ Run DRC and view violations (DRC window works)
- ✅ Print and export (print dialog works)
- ✅ Configure all preferences (config dialog works)
- ✅ Use all menu items and keyboard shortcuts

### Technical Capabilities
- ✅ Parallel HID development complete (both GTK2 and GTK3 work)
- ✅ All GTK3 APIs used correctly (no deprecated functions)
- ✅ Clean, maintainable code with proper separation
- ✅ Comprehensive test coverage
- ✅ Cross-platform support (Linux, macOS, Windows)
- ✅ Full documentation for users and developers

### Quality Attributes
- ✅ Visual parity with GTK2 HID (looks the same or better)
- ✅ Functional parity with GTK2 HID (does everything GTK2 does)
- ✅ Performance parity or better (Cairo often faster than GDK)
- ✅ No memory leaks or crashes
- ✅ Professional user experience

---

## Next Steps

After reviewing this overview:

1. **Milestone 4A**: Detailed plan for dialogs and menus (2 days)
2. **Milestone 4B**: Detailed plan for custom widgets (2 days)
3. **Milestone 4C**: Detailed plan for specialized windows (2 days)
4. **Milestone 4D**: Detailed plan for configuration and remaining files (2 days)
5. **Milestone 4E**: Detailed plan for final integration and testing (2 days)

Each detailed plan will include:
- Hour-by-hour breakdown
- Specific code examples for each file
- Testing protocols
- Common issues and solutions
- Success criteria

**Total Milestone 4 Duration**: 10 days (80 hours) including testing

---

## Notes

- **Parallel Development**: All code goes in `src/hid/gtk3/` with `ghid3_` prefix
- **Incremental Testing**: Test each dialog/widget immediately after migration
- **Visual Comparison**: Keep GTK2 HID running for side-by-side comparison
- **User Feedback**: Consider beta release after 4D, before 4E
- **Documentation**: Update as you go, don't save for the end

This is the final milestone before the GTK3 HID is production-ready!
