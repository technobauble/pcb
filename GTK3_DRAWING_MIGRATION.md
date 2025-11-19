# GTK3 Drawing Migration Analysis

## Current State (GTK2/GDK)

### Key Drawing Functions
- `ghid_draw_line()` - Draw lines
- `ghid_draw_arc()` - Draw arcs
- `ghid_draw_rect()` - Draw rectangles
- `ghid_fill_circle()` - Fill circles
- `ghid_fill_polygon()` - Fill polygons
- `ghid_fill_rect()` - Fill rectangles
- `ghid_draw_grid()` - Draw grid

### GDK API Usage
- **40 instances** of GDK drawing APIs:
  - `GdkGC` - Graphics context
  - `GdkDrawable` - Drawing surface
  - `gdk_draw_*()` - Drawing primitives

### Data Structures to Migrate
```c
// GTK2 (Current)
typedef struct render_priv {
  GdkGC *bg_gc;         // Background GC
  GdkGC *offlimits_gc;  // Off-limits GC
  GdkGC *mask_gc;       // Mask GC
  GdkGC *u_gc;          // Universal GC
  GdkGC *grid_gc;       // Grid GC
  ...
} render_priv;

typedef struct hid_gc_struct {
  GdkGC *gc;
  gchar *colorname;
  Coord width;
  gint cap, join;
  ...
} hid_gc_struct;
```

## Target State (GTK3/Cairo)

### Migration Strategy

```c
// GTK3 (Target)
typedef struct render_priv {
  cairo_t *cr;          // Cairo context
  cairo_surface_t *bg_surface;
  // Cairo doesn't use GCs - state is in context
  ...
} render_priv;

typedef struct hid_gc_struct {
  // No more GdkGC!
  gchar *colorname;
  Coord width;
  cairo_line_cap_t cap;
  cairo_line_join_t join;
  ...
} hid_gc_struct;
```

### API Mapping

| GTK2/GDK | GTK3/Cairo |
|----------|------------|
| `GdkGC` | `cairo_t` (context state) |
| `gdk_draw_line()` | `cairo_move_to()` + `cairo_line_to()` + `cairo_stroke()` |
| `gdk_draw_arc()` | `cairo_arc()` + `cairo_stroke()` |
| `gdk_draw_rectangle()` | `cairo_rectangle()` + `cairo_stroke()` or `cairo_fill()` |
| `gdk_draw_polygon()` | `cairo_move_to()` + `cairo_line_to()` loop + `cairo_close_path()` |
| `gdk_gc_set_foreground()` | `cairo_set_source_rgb()` |
| `gdk_gc_set_line_attributes()` | `cairo_set_line_width()`, `cairo_set_line_cap()`, etc. |

### Drawing Widget Changes

**GTK2:**
```c
gtk_widget_set_double_buffered(widget, FALSE);
expose-event signal → gdk_draw_*
```

**GTK3:**
```c
// No more double-buffering flag (automatic)
draw signal → cairo_* with provided cairo_t
```

## Migration Plan

### Phase 1: Infrastructure (Day 2)
1. Change `expose-event` → `draw` signal
2. Add Cairo context to render_priv
3. Create Cairo surface management
4. Update widget initialization

### Phase 2: Basic Primitives (Day 2-3)
5. Migrate `ghid_draw_line()`
6. Migrate `ghid_draw_arc()`
7. Migrate `ghid_draw_rect()`
8. Migrate `ghid_fill_circle()`

### Phase 3: Complete Drawing (Day 3-4)
9. Migrate `ghid_fill_polygon()`
10. Migrate `ghid_fill_rect()`
11. Migrate grid drawing
12. Migrate color/line style management

### Phase 4: Events & Testing (Day 4-5)
13. Migrate gui-output-events.c
14. Test with real PCB files
15. Performance optimization

## Expected Changes
- ~40 GDK API calls → Cairo equivalents
- Remove GdkGC usage (state moves to Cairo context)
- Change expose-event → draw signal
- Simplify double-buffering (automatic in GTK3)

## Testing Strategy
1. Test basic shapes first (lines, rectangles)
2. Test complex shapes (polygons, arcs)
3. Test grid rendering
4. Load real PCB files
5. Compare GTK2 vs GTK3 rendering side-by-side

---

## IMPLEMENTATION STATUS - COMPLETE (100%)

**Last Updated:** November 19, 2025
**Completed Date:** November 19, 2025
**Commits:**
- 97cf2ae, 306b02a: Core Cairo infrastructure
- 14dc392: Background image rendering
- 61670ae: Mask infrastructure
- 71488be: Lead user indicator
- a741147: Crosshair XOR replacement
- 53804e8: Mask application (final feature)

### ✅ Completed Features

**Core Infrastructure:**
- ✅ Cairo surface management (cairo_image_surface_t)
- ✅ Graphics context setup (use_gc with Cairo state)
- ✅ Color parsing (GdkColor → GdkRGBA)
- ✅ Signal migration (expose-event → draw)
- ✅ Screen update integration (gtk_widget_queue_draw)

**Drawing Primitives:**
- ✅ ghid_draw_line() - Cairo lines
- ✅ ghid_draw_arc() - Cairo arcs with transforms
- ✅ ghid_draw_rect() - Cairo rectangles (outline)
- ✅ ghid_fill_rect() - Cairo rectangles (filled)
- ✅ ghid_fill_circle() - Cairo circles
- ✅ ghid_fill_polygon() - Cairo polygons
- ✅ ghid_draw_grid() - Semi-transparent grid (XOR replacement)

**Supporting Functions:**
- ✅ Background rendering (PCB canvas)
- ✅ Background image rendering (Cairo pixbuf rendering)
- ✅ Offlimits areas (dead space around PCB)
- ✅ Dual-path rendering (GTK2 + GTK3 compatibility)
- ✅ Mask/stencil operations (complete Cairo group-based masking)

**UI Features:**
- ✅ Crosshair rendering (all 3 styles: Basic, Union Jack, Dozen)
- ✅ Lead user indicator (animated pulsing circles)

### Mask/Stencil Operations - ✅ **COMPLETE**

**Implementation:** Cairo group-based masking (Commit: 53804e8)

**How It Works:**
1. **HID_MASK_CLEAR:** Draw shapes to 1-bit mask surface (white = opaque)
2. **HID_MASK_AFTER:** Activate masking mode
3. **Masked Drawing:** Each primitive wrapped in cairo_push_group() / cairo_pop_group_to_source() / cairo_mask_surface()
4. **HID_MASK_OFF:** Flush final group, return to normal rendering

**Technical Details:**
- Added `in_masked_group` flag to track group state
- In `use_gc()`: Push group when mask_seq is active
- Each drawing operation renders to temporary group surface
- On next `use_gc()` call: Pop group and composite through mask
- Automatic cleanup on mode transitions

**Result:** Solder mask rendering fully migrated to Cairo

### Migration to Milestone 3

**Milestone 2 is now 100% complete!** All core drawing infrastructure has been
successfully migrated to Cairo. Recent accomplishments:
- ✅ Background image rendering migrated to Cairo
- ✅ Crosshair rendering (all 3 styles with Cairo semi-transparency)
- ✅ Lead user indicator (animated pulsing circles)
- ✅ Mask/stencil operations (complete group-based masking)

The implementation provides complete functionality for:
- ✅ Basic PCB viewing
- ✅ Trace/pad/via rendering
- ✅ Element display
- ✅ Grid visualization
- ✅ All primary drawing operations
- ✅ Solder mask rendering
- ✅ Interactive crosshair and lead user features

**Status:** Ready to proceed to Milestone 3 (OpenGL migration). All Cairo
drawing features are complete and fully functional.
