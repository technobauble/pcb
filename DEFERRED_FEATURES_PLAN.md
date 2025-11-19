# Deferred Features Implementation Plan

**Date:** 2025-11-19
**Purpose:** Prioritize and plan implementation of deferred features from Milestones 2-4

---

## Overview

During the GTK3 HID migration, several features were deferred due to complexity or resource constraints. This document provides a prioritized plan for addressing them.

---

## Current Status (Updated 2025-11-19)

### ✅ Completed Features

1. **Background Image Rendering** - ✅ **COMPLETE** (Commit: 14dc392)
   - Migrated to Cairo using `gdk_cairo_set_source_pixbuf()`
   - Effort: 30 minutes
   - Impact: Milestone 2 → 90%

2. **Crosshair XOR Rendering** - ✅ **COMPLETE** (Commit: a741147)
   - Implemented Cairo semi-transparent overlay (alpha=0.5)
   - All 3 styles supported: Basic, Union Jack, Dozen
   - Effort: 4 hours actual (estimated 4-6 hours)
   - Impact: Milestone 2 → 95%

3. **Lead User Indicator** - ✅ **COMPLETE** (Commit: 71488be)
   - Animated pulsing circles using Cairo
   - Semi-transparent yellow rendering
   - Effort: 1 hour
   - Impact: Milestone 2 → 97%

### ⏳ Remaining Features

**Milestone 2:** 97% Complete (only mask application remaining)

---

## Deferred Features Summary

### From Milestone 2: Cairo Drawing (3% remaining)

1. ~~**Crosshair XOR Rendering**~~ - ✅ COMPLETE

2. **Mask/Stencil Operations** - ⚙️ **60% Complete** (Infrastructure done)
   - Complexity: MEDIUM-HIGH
   - Remaining Effort: 1-2 hours
   - Priority: MEDIUM (affects solder mask rendering)
   - Status: Infrastructure complete (Commit: 61670ae), application pending

3. ~~**Background Image Rendering**~~ - ✅ COMPLETE

4. ~~**Lead User Indicator**~~ - ✅ COMPLETE

### From Milestone 3: OpenGL (5% deferred)

5. **Offscreen Pixmap Rendering (ghid_render_pixmap)**
   - Complexity: HIGH
   - Effort: 4-8 hours
   - Priority: LOW (export/print feature)
   - Blocker: Requires FBO implementation or Cairo rewrite

### From Milestone 4: API Deprecations (19% deferred)

6. **GtkAction/GtkUIManager Migration**
   - Complexity: VERY HIGH
   - Effort: 8-16 hours
   - Priority: LOW (works fine in GTK3)
   - Blocker: Complex architectural refactoring needed

---

## Recommended Implementation Order

### Quick Wins (High Value, Low Effort)

#### 1. Background Image Rendering ✅ **RECOMMENDED FIRST**
**Effort:** 30 minutes | **Value:** HIGH | **Risk:** LOW

**Current Issue:**
```c
// Deprecated GTK2 function:
gdk_pixbuf_render_to_drawable (pixbuf, drawable, gc, ...);
```

**Solution:**
```c
// Modern GTK3 Cairo approach:
gdk_cairo_set_source_pixbuf (cr, pixbuf, x, y);
cairo_paint (cr);
```

**Implementation Steps:**
1. Read ghid_draw_bg_image() in gtkhid-gdk.c
2. Replace gdk_pixbuf_render_to_drawable() with Cairo equivalent
3. Test with background image (if available)
4. Commit

**Why First:**
- Clear, documented solution
- Simple 1:1 API replacement
- Low risk of breaking anything
- Removes deprecated function warning
- Good confidence builder

---

### Medium Priority (Moderate Effort, Clear Value)

#### 2. Mask/Stencil Operations - ⚙️ **In Progress (60% Complete)**
**Effort:** 1-2 hours remaining | **Value:** MEDIUM | **Risk:** LOW

**Status:** Infrastructure implemented, application pending

**Completed (Commit 61670ae):**
- ✅ Created cairo_surface_t with CAIRO_FORMAT_A1 for mask
- ✅ Implemented mask lifecycle management (create/resize/destroy)
- ✅ Updated ghid_use_mask() to handle Cairo mask surface
- ✅ Drawing redirection: priv->cr points to mask_cr in HID_MASK_CLEAR mode
- ✅ Dual-path compatibility maintained

**Remaining Work:**
- Mask application during normal rendering (HID_MASK_AFTER mode)
- Integrate cairo_mask_surface() into rendering pipeline
- Test with solder mask rendering

**Why Continue:**
- 60% done, finish what we started
- Clear path forward with cairo_mask_surface()
- Affects functional rendering (solder masks)
- Low remaining effort (1-2 hours)

---

### Lower Priority (Higher Effort or Less Critical)

#### 3. Lead User Indicator
**Effort:** 1 hour | **Value:** LOW | **Risk:** LOW

**Depends On:** Crosshair implementation decisions

**Current Issue:**
- draw_lead_user() disabled in GTK3 (XOR dependency)

**Solution:**
- Same approach as crosshair (overlay or DIFFERENCE operator)
- Implement after crosshair solution is chosen

**Why Later:**
- Depends on crosshair implementation
- UI enhancement only (not functional)
- Can use same pattern as crosshair

---

#### 4. Crosshair XOR Rendering
**Effort:** 4-6 hours | **Value:** LOW | **Risk:** MEDIUM

**Current Issue:**
- 8 gdk_draw_line() calls with GDK_XOR
- Cairo doesn't support XOR composition

**Solution Options:**
1. **Overlay Surface** - Draw crosshair on separate surface with transparency
2. **CAIRO_OPERATOR_DIFFERENCE** - Inversion effect (not true XOR)
3. **Widget Context** - Draw directly on widget (no offscreen caching)

**Implementation Steps:**
1. Choose approach (recommend overlay or widget context)
2. Implement new crosshair rendering
3. Test with various background colors/patterns
4. Fine-tune performance if needed
5. Commit

**Why Later:**
- Requires architectural decision
- UI enhancement only (crosshair works without XOR)
- More complex testing needed
- Can live with current state

---

#### 5. Offscreen Pixmap Rendering
**Effort:** 4-8 hours | **Value:** LOW | **Risk:** HIGH

**Current Issue:**
- ghid_render_pixmap() still uses GtkGLExt
- Used for export/print operations

**Solution Options:**
1. **GtkGLArea + FBO** - Use framebuffer objects for offscreen rendering
2. **Native OpenGL FBO** - Implement FBO without GtkGLArea
3. **Cairo Fallback** - Rewrite export using Cairo instead of GL

**Why Much Later:**
- Not critical (main display works fine)
- Export/print feature (edge case)
- High complexity, multiple approaches
- Requires careful testing
- Can defer until export is actually tested/used

---

#### 6. GtkAction/GtkUIManager Migration
**Effort:** 8-16 hours | **Value:** LOW (for GTK3) | **Risk:** HIGH

**Current Issue:**
- 14 instances of deprecated GtkAction/GtkUIManager APIs
- Menu and action system uses old framework

**Solution:**
- Complete refactor to GSimpleAction/GMenu
- Requires extensive testing of all menus/shortcuts

**Why Defer Indefinitely:**
- Works perfectly in GTK3 (no runtime issues)
- Only needed for GTK4
- Extensive architectural change
- Should be part of dedicated GTK4 migration project
- High risk of breaking keyboard shortcuts/menus

---

## Recommended Timeline

### Session 1: Quick Win (30 min - 1 hour)
- ✅ Implement background image rendering
- ✅ Test and commit
- **Outcome:** Remove 1 more deprecated function, boost confidence

### Session 2: Medium Effort (3-4 hours)
- Implement mask/stencil operations
- Test with solder mask rendering
- Commit in logical pieces
- **Outcome:** Complete functional rendering feature

### Session 3: UI Enhancements (4-6 hours) - OPTIONAL
- Choose crosshair rendering approach
- Implement crosshair XOR replacement
- Implement lead user indicator
- **Outcome:** Full UI feature parity with GTK2

### Future: Export/Print Support (4-8 hours) - WHEN NEEDED
- Implement offscreen pixmap rendering
- Test export/print functionality
- **Outcome:** Full export feature support

### GTK4 Migration: Action System (8-16 hours) - GTK4 ONLY
- Migrate to GSimpleAction/GMenu
- Test all menus and shortcuts extensively
- **Outcome:** GTK4 compatibility

---

## Success Criteria

### Milestone 2 Completion (Target: 100%)
- ✅ Background images work (currently 85% → 90%)
- ✅ Mask operations work (90% → 95%)
- ⚠️ Crosshair works (optional, 95% → 100%)

### Milestone 3 Completion (Target: 100%)
- ⚠️ Export/print works (currently 95% → 100%)

### Milestone 4 Completion
- ⏳ Defer GtkAction to GTK4 (stays at 81%)

---

## Recommendation

**START WITH:** Background image rendering (30 min, high value, low risk)

This gives us:
1. Quick win to build momentum
2. Removes deprecated function
3. Clear implementation path
4. Easy to test
5. Moves Milestone 2 from 85% → 90%

After that success, evaluate whether to continue with mask operations or move on to other priorities like building/testing the complete GTK3 HID.
