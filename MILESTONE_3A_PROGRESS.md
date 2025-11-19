# Milestone 3A Progress: GtkGLArea Setup & Basic 3D

**Started:** November 19, 2025
**Status:** In Progress - 30% Complete

---

## Overview

Milestone 3A focuses on replacing deprecated GtkGLExt with GTK3's built-in GtkGLArea widget and ensuring basic 3D rendering works.

**Goal:** Complete OpenGL infrastructure migration and verify 3D rendering

---

## Implementation Status

### ✅ Completed Features (30%)

**1. GtkGLArea Widget Integration** - ✅ COMPLETE
- Widget creation using `gtk_gl_area_new()` (gui-top-window.c:1422)
- Depth buffer enabled with `gtk_gl_area_set_has_depth_buffer()`
- OpenGL 2.1 requirement set with `gtk_gl_area_set_required_version()`
- Proper widget initialization

**2. Signal Connections** - ✅ COMPLETE (Commit: 2585ecd)
- Changed from "draw" signal to "render" signal for GtkGLArea
- Conditional compilation (#ifdef ENABLE_GL) for GL vs Cairo paths
- Proper separation of OpenGL and 2D Cairo rendering paths

**3. Render Callback** - ✅ COMPLETE (Commit: 2585ecd)
- Renamed `ghid_drawing_area_draw_cb` → `ghid_drawing_area_render_cb`
- Updated signature for GtkGLArea render signal
- Automatic GL context management in render callback
- Removed deprecated `widget->allocation` usage

**4. GL Context Management** - ✅ COMPLETE (Commit: 2585ecd)
- `ghid_start_drawing()` explicitly calls `gtk_gl_area_make_current()`
- Error checking with `gtk_gl_area_get_error()`
- Context activation for drawing outside render callback
- Proper error reporting

### ⏳ Remaining Work (70%)

**5. Realize Callback Enhancement** - ⏳ PENDING
- Add one-time OpenGL state initialization
- Set up lighting, materials, depth testing
- Initialize any GL resources needed

**6. 2D vs 3D Mode Toggle** - ⏳ PARTIALLY COMPLETE
- Toggle mechanism exists (`global_view_2d` variable)
- Trackball widget with "2D View" button exists
- Need to verify mode switching works correctly
- May need to adjust camera/projection for 3D mode

**7. Camera Controls** - ⏳ PARTIALLY COMPLETE
- Trackball rotation code exists (gui-trackball.c)
- View matrix manipulation present (view_matrix variable)
- Need to verify mouse controls work
- May need zoom/pan refinements

**8. Testing** - ⏳ NOT STARTED
- Test OpenGL context creation
- Verify 3D rendering works
- Check 2D/3D mode toggle
- Test camera controls (rotate, zoom, pan)

---

## Technical Details

### GtkGLArea Render Callback Flow

```
User Action / Timer
        ↓
gtk_widget_queue_draw()
        ↓
GTK3 GtkGLArea "render" signal
        ↓
ghid_drawing_area_render_cb()
   - Context automatically current
   - Set up viewport/matrices
   - Call hidgl_start_render()
   - Draw PCB using OpenGL
   - Call hidgl_finish_render()
   - Return (buffer swap automatic)
```

### GL Context Lifecycle

1. **Widget Creation**: `gtk_gl_area_new()` in gui-top-window.c
2. **Realize**: GtkGLArea creates GL context automatically
3. **Render**: Context made current → render callback → buffer swap
4. **Drawing Outside Callback**: Explicit `gtk_gl_area_make_current()` needed

### Key Files

- `src/hid/gtk3/gtkhid-gl.c` - OpenGL rendering implementation
- `src/hid/gtk3/gui-top-window.c` - Widget creation and signal connections
- `src/hid/gtk3/gui-trackball.c` - Camera controls (trackball widget)
- `src/hid/common/hidgl.c` - Common OpenGL drawing routines

---

## Next Steps

1. **Add Realize Callback Initialization**
   - Initialize GL state (lighting, depth test, blending)
   - Set up any GL resources
   - Report GL version and capabilities

2. **Verify Mode Toggle**
   - Test switching between 2D and 3D views
   - Ensure camera updates correctly
   - Check rendering path selection

3. **Test OpenGL Rendering**
   - Compile with ENABLE_GL
   - Launch pcb with --hid gtk3
   - Load a test PCB file
   - Toggle to 3D view
   - Verify rendering works

4. **Camera Control Testing**
   - Test mouse rotation (trackball)
   - Test zoom (mouse wheel)
   - Test pan
   - Verify smooth interaction

---

## Known Issues

None currently identified - waiting for testing phase

---

## Commits

- **2585ecd** - Fix GtkGLArea rendering integration (signal, callback, context)

---

## Estimated Completion

- **Current**: 30% complete
- **Remaining**: ~5-6 hours
  - Realize callback: 1 hour
  - Mode toggle verification: 1 hour
  - Testing and fixes: 3-4 hours

**Target**: Complete within 1 day
