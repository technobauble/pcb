# Milestone 3: OpenGL 3D Rendering - Overview

**GTK3 Migration - Week 3**
**Duration:** 5 days (40 hours)
**Goal:** Implement OpenGL-based 3D rendering for GTK3 HID using GtkGLArea

---

## Overview

Milestone 3 adds OpenGL 3D visualization capability to the GTK3 HID. The GTK2 HID uses the deprecated GtkGLExt library, which is no longer available in GTK3. GTK3 provides built-in OpenGL support through GtkGLArea, which simplifies context management and provides better integration.

By the end of this milestone, you will have:
- GtkGLArea-based OpenGL rendering working
- 3D visualization of PCB boards
- Camera controls (rotate, zoom, pan)
- Layer visibility in 3D
- Complete feature parity with GTK2 HID's 3D mode

---

## Breaking Down Milestone 3

Milestone 3 is complex because it involves:
- Replacing an external library (GtkGLExt) with built-in GTK3 API
- Understanding OpenGL context management
- 3D rendering pipeline
- Camera/view transformations

To make this manageable, it's broken into two sub-milestones:

### Milestone 3A: GtkGLArea Setup & Basic 3D
**Duration:** 3 days (24 hours)
**Focus:** OpenGL infrastructure, basic 3D rendering

**Deliverables:**
- GtkGLArea widget integrated
- OpenGL context creation working
- Basic 3D test rendering (cube, axes)
- Camera controls (rotate, zoom)
- GL mode toggle (2D ↔ 3D)

**Success Criteria:**
- Can switch to 3D mode: View → 3D View
- Simple 3D shapes render
- Can rotate view with mouse
- Can zoom with scroll wheel
- No GL errors

---

### Milestone 3B: Complete 3D PCB Rendering
**Duration:** 2 days (16 hours)
**Focus:** Full 3D PCB visualization, layer rendering

**Deliverables:**
- Complete 3D PCB rendering
- All layers rendered in 3D with thickness
- Proper z-ordering (layer stacking)
- Component 3D visualization
- Via rendering in 3D
- Performance optimization

**Success Criteria:**
- Real PCB files display in 3D
- Layers show proper thickness/separation
- Visual quality matches GTK2 3D mode
- Performance acceptable (> 30 FPS)
- All 3D features functional

---

## Technical Background

### GTK2 vs GTK3 OpenGL

**GTK2 HID (Current):**
```c
// Uses external GtkGLExt library
#include <gtk/gtkgl.h>
#include <gdk/gdkgl.h>

GdkGLConfig *gl_config;
GdkGLContext *gl_context;
GdkGLDrawable *gl_drawable;

// Manual context management
gdk_gl_drawable_gl_begin(gl_drawable, gl_context);
// ... OpenGL calls ...
gdk_gl_drawable_gl_end(gl_drawable);
gdk_gl_drawable_swap_buffers(gl_drawable);
```

**GTK3 HID (Target):**
```c
// Built-in GTK3 support
#include <gtk/gtk.h>  // That's it!

GtkWidget *gl_area;

// GTK3 manages context automatically
gtk_gl_area_make_current(GTK_GL_AREA(gl_area));
// ... OpenGL calls ...
// GTK3 swaps buffers automatically
```

**Key Differences:**
1. **No external dependency** - GtkGLArea is built into GTK3
2. **Simpler API** - Context management automatic
3. **Signal-based** - Use "realize" and "render" signals
4. **Better integration** - Works with GTK3 event system

### OpenGL Context in GTK3

GTK3 manages the OpenGL context lifecycle through signals:

```c
// Realize: Called when widget is realized (one-time setup)
g_signal_connect(gl_area, "realize",
                 G_CALLBACK(on_realize), NULL);

// Render: Called when widget needs to redraw
g_signal_connect(gl_area, "render",
                 G_CALLBACK(on_render), NULL);

// Resize: Called when widget size changes
g_signal_connect(gl_area, "resize",
                 G_CALLBACK(on_resize), NULL);
```

**Realize callback** - Initialize OpenGL:
```c
static void
on_realize(GtkGLArea *area)
{
  gtk_gl_area_make_current(area);  // Context is current now

  // Initialize OpenGL state
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  // ... other setup ...
}
```

**Render callback** - Draw frame:
```c
static gboolean
on_render(GtkGLArea *area, GdkGLContext *context)
{
  // Context is automatically current

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Draw 3D scene
  draw_pcb_3d();

  glFlush();  // GTK3 handles buffer swap

  return TRUE;
}
```

### 3D Rendering Strategy

**PCB Layers in 3D:**
```
Top View (2D):          Side View (3D):
  Component Side         ┌─ Component layer (silk)
  Copper Layer 1         ├─ Copper layer 1 (traces)
  Copper Layer 2         │  (0.035mm thick)
  Solder Side            ├─ Substrate (FR-4)
                         │  (1.6mm thick)
                         ├─ Copper layer 2 (traces)
                         └─ Solder side (silk)
```

**Rendering approach:**
- Each layer at different Z coordinate
- Traces rendered as 3D rectangles with thickness
- Pads rendered as cylinders
- Vias rendered as cylinders through all layers
- Board substrate rendered as filled rectangle

---

## Parallel HID Development (Continued)

Just like Milestone 2, OpenGL work happens in the parallel `src/hid/gtk3/` directory. The GTK2 HID remains untouched and functional.

**File Focus:**
- `src/hid/gtk3/gtkhid-gl.c` - Main OpenGL implementation
- `src/hid/gtk3/gui-output-events.c` - 3D event handling
- `src/hid/gtk3/gui-top-window.c` - GL widget integration

---

## Dependencies and Prerequisites

### Prerequisites from Previous Milestones

**Required:**
- ✅ Milestone 1 complete - Build system, GTK3 structure
- ✅ Milestone 2 complete - 2D rendering working

**Why these are required:**
- Need working GTK3 HID structure
- Need 2D drawing functions for comparison
- Need event handling infrastructure

### System Dependencies

**OpenGL Libraries:**
```bash
# Ubuntu/Debian
sudo apt-get install libgl1-mesa-dev libglu1-mesa-dev

# Fedora
sudo dnf install mesa-libGL-devel mesa-libGLU-devel

# macOS
# OpenGL included with Xcode

# Windows (MSYS2)
pacman -S mingw-w64-x86_64-mesa
```

**Verify OpenGL:**
```bash
# Check OpenGL version
glxinfo | grep "OpenGL version"
# Should show OpenGL 2.1 or later

# Check if hardware acceleration is available
glxinfo | grep "direct rendering"
# Should show "direct rendering: Yes"
```

---

## Testing Strategy

### Incremental Testing

**After Milestone 3A:**
```bash
# Switch to 3D mode
./pcb --hid gtk3 myboard.pcb
# Then: View → 3D View

# Should see:
# - 3D coordinate axes
# - Simple 3D test geometry
# - Can rotate view with mouse drag
# - Can zoom with scroll wheel
```

**After Milestone 3B:**
```bash
# Full 3D visualization
./pcb --hid gtk3 myboard.pcb
# View → 3D View

# Should see:
# - Complete PCB in 3D
# - Multiple layers with thickness
# - Components elevated above board
# - Smooth rotation and zoom
```

### Comparison Testing

**GTK2 vs GTK3 3D comparison:**
```bash
# Launch both with same board
./pcb --hid gtk tests/inputs/555.pcb &
./pcb --hid gtk3 tests/inputs/555.pcb &

# In both:
# - Switch to 3D mode (View → 3D View)
# - Compare visual appearance
# - Compare camera controls
# - Compare performance
```

---

## Success Metrics

### Milestone 3A Success
- ✅ GtkGLArea widget creates OpenGL context
- ✅ Can switch between 2D and 3D modes
- ✅ Basic 3D rendering works (test geometry)
- ✅ Camera controls functional (rotate, zoom)
- ✅ No OpenGL errors
- ✅ Performance acceptable for test scene

### Milestone 3B Success
- ✅ Real PCB boards render in 3D
- ✅ All layers visible with proper z-ordering
- ✅ Traces have thickness in 3D
- ✅ Components show elevation
- ✅ Vias render through layers
- ✅ Visual parity with GTK2 3D mode
- ✅ Performance > 30 FPS for typical boards

### Combined Milestone 3 Success
- ✅ Complete 3D visualization working
- ✅ Feature parity with GTK2 HID 3D mode
- ✅ No dependency on deprecated GtkGLExt
- ✅ Clean GTK3 integration via GtkGLArea
- ✅ Production-ready 3D rendering

---

## Risk Assessment

### Lower Risk Areas
- **GtkGLArea API** - Well-documented, stable
- **OpenGL basics** - Standard API, portable
- **Context management** - GTK3 handles it

### Medium Risk Areas
- **Performance** - May need optimization
- **Platform differences** - OpenGL drivers vary
- **Shader compatibility** - If using modern GL

### Risk Mitigation
1. **Start simple** - Basic 3D first, optimize later
2. **Test early** - Check on multiple platforms early
3. **Fallback** - Software rendering if GL unavailable
4. **Profile** - Use profiling tools for performance

---

## Performance Targets

**Target Performance:**
- Simple boards: > 60 FPS
- Medium complexity: > 45 FPS
- Complex boards: > 30 FPS

**Comparable to GTK2 HID or better**

**Optimization strategies:**
- Display lists for static geometry
- Frustum culling for off-screen objects
- Level-of-detail (LOD) for distant objects
- VBO (Vertex Buffer Objects) for large data

---

## Detailed Plans

### Milestone 3A: GtkGLArea Setup & Basic 3D
**Document:** `MILESTONE_3A_GTKGLAREA_SETUP.md`
**Duration:** 3 days (24 hours)
**Detail Level:** Hour-by-hour task list

### Milestone 3B: Complete 3D PCB Rendering
**Document:** `MILESTONE_3B_3D_PCB_RENDERING.md`
**Duration:** 2 days (16 hours)
**Detail Level:** Hour-by-hour task list

---

## Timeline Summary

```
Day 1 (M3A): GtkGLArea Integration
├─ Remove GtkGLExt dependencies
├─ Create GtkGLArea widget
├─ Basic OpenGL initialization
└─ Test context creation

Day 2 (M3A): Basic 3D Rendering
├─ Implement simple 3D test scene
├─ Camera/view transformations
├─ Mouse controls (rotate, zoom)
└─ Mode toggle (2D ↔ 3D)

Day 3 (M3A): Testing & Refinement
├─ Test on multiple platforms
├─ Fix OpenGL issues
├─ Performance baseline
└─ Documentation

Day 4 (M3B): 3D PCB Rendering
├─ Layer rendering in 3D
├─ Trace rendering with thickness
├─ Pad and via 3D rendering
└─ Component elevation

Day 5 (M3B): Polish & Testing
├─ Performance optimization
├─ Visual parity verification
├─ Integration testing
└─ Final documentation
```

---

## Alternative Approaches (Not Recommended)

### Using SDL2 for OpenGL Context
**Pros:** More control, cross-platform
**Cons:** Separate window management, complex integration
**Verdict:** Stick with GtkGLArea (better GTK3 integration)

### Using QOpenGLWidget (if Qt)
**Pros:** Similar to GtkGLArea
**Cons:** Would require full Qt port
**Verdict:** Not applicable (using GTK3)

### Software Rendering (Mesa3D)
**Pros:** Works without hardware GL
**Cons:** Very slow
**Verdict:** Use as fallback only

---

## Future Enhancements (Post-Milestone 3)

After Milestone 3 is complete, potential enhancements:

1. **Modern OpenGL** (3.3+ with shaders)
   - Better performance
   - Advanced lighting
   - Post-processing effects

2. **Shadows and Lighting**
   - Better depth perception
   - More realistic visualization

3. **Advanced Camera**
   - Perspective vs orthographic
   - Camera presets (top, bottom, side views)
   - Animation/fly-through

4. **Export 3D**
   - Export to STL, OBJ formats
   - 3D printing support
   - STEP file generation

---

## Next Steps

1. **Review this overview** - Understand the approach
2. **Read Milestone 3A plan** - Detailed 3-day guide for GtkGLArea setup
3. **Read Milestone 3B plan** - Detailed 2-day guide for 3D PCB rendering
4. **Start implementation** - Begin with Milestone 3A Day 1

---

**Document Version:** 1.0
**Last Updated:** November 17, 2025
**Status:** Ready for Implementation
