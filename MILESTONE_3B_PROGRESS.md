# Milestone 3B Progress: Complete 3D PCB Rendering

**Started:** November 19, 2025
**Status:** In Progress - 50% Complete
**Prerequisites:** Milestone 3A complete (GtkGLArea setup)

---

## Overview

Milestone 3B implements complete 3D visualization of PCB boards with proper layer stacking, component elevation, and realistic geometry.

**Goal:** Transform flat 2D PCB into true 3D model with thickness, depth, and proper layer separation

---

## Implementation Status

### ✅ Completed Features (50%)

**1. 3D Layer Coordinate System** - ✅ COMPLETE (Commit: 2154072)
- Defined standard 2-layer PCB stackup with realistic dimensions
- Layer z-coordinates calculated from bottom to top:
  - Bottom soldermask: z = 0
  - Bottom copper: z = 0.8 mil
  - Board substrate: 62 mil thick FR4
  - Top copper: z = 61.2 mil
  - Top soldermask: z = 62 mil
  - Components: z = 72 mil (10 mil elevation)
- Thickness values for each layer type:
  - Copper: 1.4 mil (1 oz)
  - Soldermask: 0.8 mil
  - Silkscreen: 0.4 mil

**2. Layer Depth Mapping** - ✅ COMPLETE (Commit: 2154072)
- `ghid_get_layer_depth()`: Maps PCB layer index → z-coordinate
- Handles all layer types (copper, silk, mask, drill, rats)
- Top/bottom side determination for special layers
- Returns 0 in 2D mode for flat rendering
- Integrated into `ghid_set_layer()` via `hidgl_set_depth()`

**3. 3D Geometry Helper Functions** - ✅ COMPLETE (Commit: 501f591)
- `ghid_draw_3d_cylinder()`: For circular pads, vias, pins
  - GLU quadric-based rendering
  - Smooth tessellation with configurable slices
  - Top and bottom caps included
  - Proper normals for lighting
- `ghid_draw_3d_box()`: For rectangular pads
  - 6-face solid box
  - All faces with correct normals
  - Automatic coordinate ordering
- `ghid_draw_3d_line()`: For thick traces
  - Extruded quad geometry
  - Perpendicular width calculation
  - Top, bottom, and 4 side faces
  - Proper normals per face

### ⏳ Remaining Work (50%)

**4. Integration into Drawing Functions** - ⏳ NOT STARTED
- Modify `ghid_draw_line()` to call `ghid_draw_3d_line()` in 3D mode
- Modify `ghid_fill_circle()` to call `ghid_draw_3d_cylinder()` in 3D mode
- Modify `ghid_fill_rect()` to call `ghid_draw_3d_box()` in 3D mode
- Add conditional logic based on `global_view_2d`
- Ensure thickness is applied from `ghid_get_layer_thickness()`

**5. Via Rendering in 3D** - ⏳ NOT STARTED
- Vias need to span multiple layers (plated through-holes)
- Calculate via height from bottom copper to top copper
- Render as cylinder with appropriate height
- Handle via types (through-hole, blind, buried)

**6. Component 3D Rendering** - ⏳ NOT STARTED
- Elevate component elements above board surface
- Use `component_z` for positioning
- Render pin pads with proper height
- Render component body as simple box (placeholder)

**7. Performance Optimization** - ⏳ NOT STARTED
- Implement OpenGL display lists for static geometry
- Cache PCB geometry per layer
- Rebuild only on PCB changes
- Frame rate target: >30 FPS for typical boards

**8. Testing** - ⏳ NOT STARTED
- Load test PCB files
- Verify layer separation visible
- Check 2D/3D mode toggle
- Validate camera controls work with 3D geometry
- Performance testing with complex boards

---

## Technical Architecture

### Layer Stackup (2-Layer Board)

```
z = 72 mil   ┌─────────────────┐  Component elevation
             │   Components    │
z = 62 mil   ├═════════════════┤  Top soldermask
z = 61.2 mil ├─────────────────┤  Top copper (1.4 mil thick)
             │                 │
             │    FR4 Board    │  62 mil FR4 substrate
             │                 │
z = 2.2 mil  ├─────────────────┤  Bottom copper (1.4 mil thick)
z = 0.8 mil  ├═════════════════┤  Bottom soldermask
z = 0        └─────────────────┘  Board bottom
```

### Rendering Flow

```
PCB Element → Layer Selection → Depth Assignment → 3D Geometry → OpenGL
    |              |                   |                |
   Line      ghid_set_layer()   hidgl_set_depth()  ghid_draw_3d_line()
   Pad       (determines idx)   (sets global_depth) ghid_draw_3d_cylinder()
   Via       (silk/copper/etc)  (z-coordinate)      ghid_draw_3d_box()
```

### 2D vs 3D Mode

- **2D Mode (`global_view_2d = 1`):**
  - All layers at z = 0 (flat)
  - No thickness applied
  - 3D geometry functions return early
  - Standard hidgl triangle rendering

- **3D Mode (`global_view_2d = 0`):**
  - Layers at proper z-coordinates
  - Thickness applied to all elements
  - 3D geometry functions render solid shapes
  - Camera can rotate/tilt for 3D view

---

## Key Files

- `src/hid/gtk3/gtkhid-gl.c` - Main OpenGL rendering (3D system added)
- `src/hid/common/hidgl.c` - Common OpenGL drawing routines
- `src/hid/common/hidgl.h` - hidgl interface (global_depth)
- `src/hid/gtk3/gui-trackball.c` - Camera controls

---

## Next Steps

1. **Integrate 3D Geometry into Drawing Functions** (~3 hours)
   - Update ghid_draw_line, ghid_fill_circle, ghid_fill_rect
   - Add 3D/2D conditionals
   - Apply layer thickness

2. **Implement Via 3D Rendering** (~2 hours)
   - Calculate via span (bottom to top copper)
   - Render plated through-holes
   - Handle drill holes

3. **Testing and Refinement** (~2-3 hours)
   - Load complex PCBs
   - Verify visual correctness
   - Performance tuning
   - Bug fixes

---

## Commits

- **2154072** - 3D layer coordinate system and depth mapping
- **501f591** - 3D geometry helper functions (cylinder, box, line)

---

## Estimated Completion

- **Current**: 50% complete
- **Remaining**: ~7-8 hours
  - Integration: 3 hours
  - Vias: 2 hours
  - Testing: 2-3 hours

**Target**: Complete within 1 day

---

## Notes

- All 3D geometry auto-skips in 2D mode (thickness = 0 check)
- Uses existing hidgl infrastructure (global_depth)
- GLU quadrics provide smooth cylinder rendering
- Proper normals enable realistic lighting
- Foundation complete - now just needs integration
