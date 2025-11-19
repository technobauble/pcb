# GTK3 API Deprecations Migration

**Milestone 4: Fix Deprecated GTK3 APIs**

This document tracks the migration of deprecated GTK3 APIs to their modern replacements.

---

## Overview

Several GTK3 APIs used in the gtk3 HID are deprecated and will be removed in GTK4. While they still work in GTK3, migrating to modern APIs improves future compatibility and follows best practices.

---

## Deprecation Summary

### Analysis Results

Total deprecated API usage found:
- **39 instances** of `gtk_stock_*` / `GTK_STOCK_*` (14 files)
- **19 instances** of `gtk_misc_*` / `gtk_alignment_new` (6 files)
- **14 instances** of `GtkAction*` / `GtkUIManager` (4 files)

**Total:** 72 deprecated API calls across 16 files

---

## Priority 1: Stock Items (39 instances)

### What's Deprecated

```c
// GTK2/Early GTK3 (DEPRECATED):
GTK_STOCK_OK
GTK_STOCK_CANCEL
GTK_STOCK_SAVE
gtk_button_new_from_stock (GTK_STOCK_OK)
```

### Modern Replacement

```c
// GTK3+ (MODERN):
"_OK"         // Underscore indicates mnemonic
"_Cancel"
"_Save"
gtk_button_new_with_mnemonic ("_OK")
```

### Affected Files (14)

1. `gui-dialog.c` - 14 instances (dialog buttons)
2. `gui-config.c` - 3 instances
3. `gui-library-window.c` - 3 instances
4. `gui-top-window.c` - 3 instances
5. `gtkhid-main.c` - 3 instances
6. `gui-dialog-print.c` - 3 instances
7. `ghid-route-style-selector.c` - 2 instances
8. `gui-drc-window.c` - 2 instances
9. `gui-command-window.c` - 1 instance
10. `gui-keyref-window.c` - 1 instance
11. `gui-log-window.c` - 1 instance
12. `gui-netlist-window.c` - 1 instance
13. `gui-pinout-window.c` - 1 instance
14. `gui-utils.c` - 1 instance

### Common Stock Item Mappings

| Deprecated Stock ID | Modern String |
|---------------------|---------------|
| `GTK_STOCK_OK` | `"_OK"` |
| `GTK_STOCK_CANCEL` | `"_Cancel"` |
| `GTK_STOCK_SAVE` | `"_Save"` |
| `GTK_STOCK_OPEN` | `"_Open"` |
| `GTK_STOCK_CLOSE` | `"_Close"` |
| `GTK_STOCK_ADD` | `"_Add"` |
| `GTK_STOCK_REMOVE` | `"_Remove"` |
| `GTK_STOCK_APPLY` | `"_Apply"` |
| `GTK_STOCK_REFRESH` | `"_Refresh"` |

### Implementation Strategy

**Phase 1: Simple Replacements**
- Most stock items in dialogs can be replaced with string literals
- `gtk_dialog_new_with_buttons()` accepts strings directly

**Example:**
```c
// Before:
dialog = gtk_dialog_new_with_buttons ("Title",
                                       parent,
                                       GTK_DIALOG_MODAL,
                                       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                       GTK_STOCK_OK, GTK_RESPONSE_OK,
                                       NULL);

// After:
dialog = gtk_dialog_new_with_buttons ("Title",
                                       parent,
                                       GTK_DIALOG_MODAL,
                                       "_Cancel", GTK_RESPONSE_CANCEL,
                                       "_OK", GTK_RESPONSE_OK,
                                       NULL);
```

**Phase 2: Button Creation**
- Replace `gtk_button_new_from_stock()` with `gtk_button_new_with_mnemonic()`
- May need to add icons separately if visual appearance is important

---

## Priority 2: Misc/Alignment (19 instances)

### What's Deprecated

```c
// GTK2/Early GTK3 (DEPRECATED):
gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
gtk_alignment_new (xalign, yalign, xscale, yscale);
```

### Modern Replacement

```c
// GTK3+ (MODERN):
// For labels specifically:
gtk_label_set_xalign (GTK_LABEL (label), 1.0);
gtk_label_set_yalign (GTK_LABEL (label), 0.5);

// For general widgets:
gtk_widget_set_halign (widget, GTK_ALIGN_END);
gtk_widget_set_valign (widget, GTK_ALIGN_CENTER);

// Instead of GtkAlignment container:
// Use widget margins and halign/valign properties
gtk_widget_set_margin_start (widget, 10);
gtk_widget_set_halign (widget, GTK_ALIGN_START);
```

### Affected Files (6)

1. `gui-utils.c` - 9 instances (label alignment)
2. `ghid-route-style-selector.c` - 3 instances
3. `gui-config.c` - 3 instances
4. `gtkhid-main.c` - 2 instances
5. `gui-netlist-window.c` - 1 instance
6. `gui-top-window.c` - 1 instance

### Alignment Value Mappings

| Old `gtk_misc_set_alignment()` | New `gtk_label_set_xalign()` | Or `gtk_widget_set_halign()` |
|-------------------------------|------------------------------|------------------------------|
| `(label, 0.0, 0.5)` | `set_xalign(0.0)` | `GTK_ALIGN_START` |
| `(label, 1.0, 0.5)` | `set_xalign(1.0)` | `GTK_ALIGN_END` |
| `(label, 0.5, 0.5)` | `set_xalign(0.5)` | `GTK_ALIGN_CENTER` |

### Implementation Strategy

**Phase 1: Label Alignments**
- Most instances are `gtk_misc_set_alignment()` on labels
- Direct replacement with `gtk_label_set_xalign()` / `gtk_label_set_yalign()`

**Example:**
```c
// Before:
label = gtk_label_new ("Text");
gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);

// After:
label = gtk_label_new ("Text");
gtk_label_set_xalign (GTK_LABEL (label), 1.0);
gtk_label_set_yalign (GTK_LABEL (label), 0.5);
```

**Phase 2: Alignment Containers**
- Replace `gtk_alignment_new()` with widget properties
- May need to restructure container hierarchy slightly

---

## Priority 3: Action/UIManager (14 instances)

### What's Deprecated

```c
// GTK2/Early GTK3 (DEPRECATED):
GtkActionGroup *action_group = gtk_action_group_new ("name");
GtkAction *action = gtk_action_new (...);
gtk_action_group_add_action (action_group, action);
```

### Modern Replacement

```c
// GTK3.10+ (MODERN):
GSimpleActionGroup *action_group = g_simple_action_group_new ();
GSimpleAction *action = g_simple_action_new ("name", NULL);
g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));

// Or use GAction with GMenu for menus
```

### Affected Files (4)

1. `ghid-layer-selector.c` - 6 instances
2. `ghid-route-style-selector.c` - 4 instances
3. `ghid-main-menu.c` - 3 instances
4. `gui.h` - 1 instance (declaration)

### Implementation Strategy

**Complexity:** HIGH - This is a significant architectural change

**Options:**
1. **Full Migration to GAction/GMenu**
   - Modern GTK3 pattern
   - Better GTK4 compatibility
   - Requires significant refactoring
   - Estimated effort: 8-16 hours

2. **Minimal Fix (Defer)**
   - Keep GtkAction/GtkUIManager for now
   - They still work in GTK3 (deprecated but functional)
   - Add TODO comments for future GTK4 migration
   - Estimated effort: 30 minutes (documentation only)

**Recommendation:** **Defer to future milestone** or post-GTK4 planning
- GtkAction/GtkUIManager still work in all GTK3 versions
- This migration doesn't block GTK3 functionality
- Focus resources on Priority 1 and 2 items first

---

## Implementation Plan

### Phase 1: Stock Items (Highest Priority)
**Effort:** 2-3 hours
**Risk:** Low
**Impact:** Removes 39 deprecated API calls

**Steps:**
1. Create stock item mapping reference
2. Fix gui-dialog.c (14 instances) - commit
3. Fix gui-config.c, gui-library-window.c, gui-top-window.c - commit
4. Fix gtkhid-main.c, gui-dialog-print.c - commit
5. Fix remaining files (1-2 instances each) - commit
6. Test dialogs and file choosers

### Phase 2: Misc/Alignment (Medium Priority)
**Effort:** 1-2 hours
**Risk:** Low
**Impact:** Removes 19 deprecated API calls

**Steps:**
1. Fix gui-utils.c (9 instances) - commit
2. Fix ghid-route-style-selector.c, gui-config.c - commit
3. Fix remaining files - commit
4. Test label alignments visually

### Phase 3: Action/UIManager (Lower Priority)
**Effort:** Document only (30 min) OR Full migration (8-16 hours)
**Risk:** Medium-High (if doing full migration)
**Impact:** 14 deprecated API calls (still functional in GTK3)

**Recommended Approach:**
- Document as deferred feature
- Add TODO comments in code
- Leave for post-GTK3 work or GTK4 migration

---

## Testing Strategy

### Stock Items
- Open each dialog type (file choosers, confirmations, etc.)
- Verify buttons appear correctly
- Check button mnemonics work (Alt+key)
- Verify icons if applicable

### Misc/Alignment
- Visual inspection of all forms/dialogs
- Check labels are aligned correctly (left/right/center)
- Verify no layout regressions

### Action/UIManager
- If deferred: No testing needed
- If migrated: Extensive menu/toolbar testing required

---

## Success Criteria

### Minimum Success (Phase 1-2 Complete)
- ✅ All 39 stock item usages replaced
- ✅ All 19 misc/alignment usages replaced
- ✅ 58/72 deprecated APIs eliminated (81%)
- ✅ All dialogs and forms still functional
- ✅ Visual appearance unchanged

### Full Success (All Phases)
- ✅ All 72 deprecated APIs eliminated (100%)
- ✅ GAction/GMenu fully implemented
- ✅ Code ready for GTK4 transition

---

## Risk Assessment

### Stock Items Migration
**Risk Level:** LOW
- Simple string replacements
- Straightforward 1:1 mappings
- Easy to test visually

### Misc/Alignment Migration
**Risk Level:** LOW
- Direct API replacements
- Same functionality, different API
- Visual testing validates correctness

### Action/UIManager Migration
**Risk Level:** MEDIUM-HIGH
- Complex architectural change
- Affects menu/action system
- Requires extensive testing
- Risk of breaking keyboard shortcuts/accelerators

---

## Timeline Estimate

| Phase | Effort | Risk | Priority |
|-------|--------|------|----------|
| Phase 1: Stock Items | 2-3 hours | Low | High |
| Phase 2: Misc/Alignment | 1-2 hours | Low | Medium |
| Phase 3: Document Deferral | 30 min | None | Low |
| **Total (Recommended)** | **4-6 hours** | **Low** | - |
| Phase 3: Full Migration | 8-16 hours | Medium | Low |
| **Total (Full)** | **11-21 hours** | **Medium** | - |

---

## Recommendation

**Start with Phases 1-2 and defer Phase 3:**

1. ✅ Migrate all stock items (39 instances) - HIGH VALUE, LOW RISK
2. ✅ Migrate all misc/alignment (19 instances) - HIGH VALUE, LOW RISK
3. ⏳ Document GtkAction deferral (14 instances) - DEFER FOR NOW

This approach:
- Eliminates 81% of deprecated APIs (58/72)
- Takes 4-6 hours of focused work
- Low risk of breaking existing functionality
- Leaves the complex action system for later when GTK4 migration is planned

The GtkAction/GtkUIManager system still works fine in GTK3 and can be addressed during a future GTK4 migration milestone.

---

## Completion Status

**Date Completed:** 2025-11-19
**Overall Progress:** 81% Complete (58/72 deprecated APIs eliminated)
**Status:** High-priority migrations complete, GtkAction deferred

### ✅ Completed Migrations (58 instances, 81%)

#### Phase 1: Stock Items (39 instances) - **COMPLETE**

**Files Modified:** 13 files
**Commits:**
- 9b2f9d3: gui-dialog.c (14 instances)
- 5a1d5aa: All remaining files (25 instances)

**Replacements Made:**
- `GTK_STOCK_CANCEL` → `"_Cancel"` (14 instances)
- `GTK_STOCK_OK` → `"_OK"` (14 instances)
- `GTK_STOCK_SAVE` → `"_Save"` (3 instances)
- `GTK_STOCK_CLOSE` → `"_Close"` (5 instances)
- `GTK_STOCK_REFRESH` → `"_Refresh"` (2 instances, buttons)
- `GTK_STOCK_ADD` → `"_Add"` (1 instance)
- `GTK_STOCK_DELETE` → `"_Delete"` (1 instance)

**Function Call Updates:**
- `gtk_button_new_from_stock()` → `gtk_button_new_with_mnemonic()` (9 instances)
- `gtk_image_new_from_icon_name()` (4 instances)
  - `GTK_STOCK_REFRESH` → `"view-refresh"`
  - `GTK_STOCK_DIALOG_WARNING` → `"dialog-warning"`
  - `GTK_STOCK_CLEAR` → `"edit-clear"`

**Impact:** All dialogs, file choosers, and buttons now use modern string-based labels and icon names.

#### Phase 2: Misc/Alignment (19 instances) - **COMPLETE**

**Files Modified:** 6 files
**Commit:** 25a3cfc

**Replacements Made:**
- `gtk_misc_set_alignment(label, x, y)` → `gtk_label_set_xalign(label, x); gtk_label_set_yalign(label, y)` (18 instances)
- `gtk_alignment_new() + padding` → widget margin properties (1 instance)

**File Breakdown:**
- gui-utils.c: 9 label alignment fixes
- ghid-route-style-selector.c: 3 label alignment fixes
- gui-config.c: 3 label alignment fixes
- gtkhid-main.c: 2 label alignment fixes + 1 GtkAlignment container removal
- gui-netlist-window.c: 1 label alignment fix
- gui-top-window.c: 1 label alignment fix

**Impact:** All label alignments use modern xalign/yalign properties. Containers use margins instead of GtkAlignment.

### ⏳ Deferred Features (14 instances, 19%)

#### Phase 3: GtkAction/GtkUIManager (14 instances) - **DEFERRED**

**Files Affected:** 4 files
- ghid-layer-selector.c (6 instances)
- ghid-route-style-selector.c (4 instances)
- ghid-main-menu.c (3 instances)
- gui.h (1 instance)

**Why Deferred:**
1. **Still Functional:** Works fine in all GTK3 versions
2. **High Complexity:** Requires architectural refactoring
3. **GTK4 Blocker:** Required for GTK4, but not GTK3
4. **Resource Allocation:** Estimated 8-16 hours
5. **Testing Burden:** Extensive menu/keyboard testing needed

**Migration Path (Future Work):**
- Replace `GtkActionGroup` with `GSimpleActionGroup`
- Replace `GtkAction` with `GSimpleAction`
- Replace `GtkUIManager` with `GtkBuilder` + `GMenu`

---

## Summary

### Accomplishments

**58 out of 72 deprecated API calls eliminated (81% complete)**

1. ✅ All stock items removed - Modern string labels and icon names
2. ✅ All misc/alignment calls removed - Modern xalign/yalign and margins
3. ⏳ GtkAction system deferred - Functional, will address in GTK4 migration

**Effort Invested:**
- Planning: 1 hour
- Phase 1 (Stock Items): 2 hours
- Phase 2 (Misc/Alignment): 1.5 hours
- **Total: ~4.5 hours**

### Testing Recommendations

- Test all dialogs and file choosers
- Verify button mnemonics work (Alt+key)
- Check label alignments in all forms
- Verify icon buttons display correctly
- Confirm progress dialog margins

### Next Steps

**Recommended:** Build and test GTK3 HID to validate all changes from Milestones 2, 3, and 4 before proceeding further.
