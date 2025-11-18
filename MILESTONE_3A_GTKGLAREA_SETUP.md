# Milestone 3A: GtkGLArea Setup & Basic 3D - Detailed Implementation Plan

**GTK3 Migration - Week 3 (Days 1-3)**
**Duration:** 3 days (24 hours)
**Goal:** Set up GtkGLArea and implement basic 3D rendering infrastructure

---

## Overview

Milestone 3A replaces the deprecated GtkGLExt library with GTK3's built-in GtkGLArea widget and implements basic 3D rendering infrastructure. By the end of this phase, you will have:
- GtkGLArea widget integrated into GTK3 HID
- OpenGL context creation working
- Basic 3D test scene rendering (axes, cube, grid)
- Camera controls (rotate, zoom, pan)
- Mode toggle between 2D and 3D views
- Foundation ready for complete 3D PCB rendering (Milestone 3B)

**Prerequisites:** Milestone 2 complete (2D rendering working in GTK3 HID)

---

## Table of Contents

1. [Day 1: GtkGLArea Integration](#day-1-gtkglarea-integration)
2. [Day 2: Basic 3D Rendering](#day-2-basic-3d-rendering)
3. [Day 3: Testing & Refinement](#day-3-testing--refinement)
4. [Success Criteria](#success-criteria)
5. [Troubleshooting Guide](#troubleshooting-guide)

---

## Day 1: GtkGLArea Integration

**Goal:** Remove GtkGLExt dependency, integrate GtkGLArea, create OpenGL context

**Time Allocation:** 8 hours

### What You'll Accomplish

By end of Day 1, you will:
- Have removed all GtkGLExt code from GTK3 HID
- Have GtkGLArea widget created and integrated
- Have OpenGL context initialization working
- Have basic GL state setup (depth test, clear color, etc.)
- Be able to clear the GL viewport with a test color

### Detailed Todo List

#### Task 1.1: Analyze Current GTK2 OpenGL Code (1 hour)

- [ ] **Review GTK2 HID OpenGL implementation**

  ```bash
  cd src/hid/gtk

  # Find OpenGL-related code
  grep -n "gtkgl\|GdkGL\|gl_config\|gl_context" gtkhid-gl.c | wc -l

  # Understand the structure
  less gtkhid-gl.c
  ```

- [ ] **Identify key components**

  Look for:
  - GL config creation
  - GL context creation
  - Widget setup (making widget GL-capable)
  - Realize callback
  - Expose/render callback
  - Drawing functions

- [ ] **Create migration checklist**

  Create `src/hid/gtk3/GL_MIGRATION.md`:
  ```markdown
  # OpenGL Migration: GtkGLExt → GtkGLArea

  ## GTK2 Components (GtkGLExt) → GTK3 Replacements (GtkGLArea)

  - [ ] GdkGLConfig → Not needed (GtkGLArea handles it)
  - [ ] gtk_widget_set_gl_capability() → gtk_gl_area_new()
  - [ ] GdkGLContext → GdkGLContext (different API)
  - [ ] GdkGLDrawable → Not needed
  - [ ] gdk_gl_drawable_gl_begin() → gtk_gl_area_make_current()
  - [ ] gdk_gl_drawable_gl_end() → Not needed (automatic)
  - [ ] gdk_gl_drawable_swap_buffers() → Not needed (automatic)
  - [ ] expose-event → render signal
  - [ ] realize → realize signal (simpler)

  ## Functions to Migrate

  From gtkhid-gl.c:
  - [ ] ghid_gl_init() - GL initialization
  - [ ] ghid_gl_realize() - Realize callback
  - [ ] ghid_gl_expose() - Render callback
  - [ ] ghid_gl_configure() - Resize callback
  - [ ] ghid_gl_draw_*() - 3D drawing functions

  ## OpenGL State to Preserve

  - [ ] Depth testing
  - [ ] Lighting setup
  - [ ] Material properties
  - [ ] Viewport setup
  - [ ] Projection matrix
  - [ ] Modelview matrix
  ```

#### Task 1.2: Remove GtkGLExt Dependencies (30 minutes)

- [ ] **Update gtkhid-gl.c includes**

  Edit `src/hid/gtk3/gtkhid-gl.c`:

  ```c
  // OLD (GTK2 + GtkGLExt):
  #include <gtk/gtk.h>
  #include <gtk/gtkgl.h>
  #include <gdk/gdkgl.h>
  #include <GL/gl.h>
  #include <GL/glu.h>

  // NEW (GTK3 with built-in GL):
  #include <gtk/gtk.h>
  #include <epoxy/gl.h>  // GTK3 uses libepoxy for GL
  // OR if epoxy not available:
  #include <GL/gl.h>
  #include <GL/glu.h>
  ```

- [ ] **Remove GtkGLExt-specific code**

  Find and remove/comment out:
  ```c
  // Remove these:
  GdkGLConfig *gl_config = NULL;
  gtk_widget_set_gl_capability(...);
  gdk_gl_drawable_gl_begin(...);
  gdk_gl_drawable_gl_end(...);
  gdk_gl_drawable_swap_buffers(...);
  ```

- [ ] **Update Makefile.am**

  Edit `src/hid/gtk3/Makefile.am`:

  ```makefile
  # Remove GtkGLExt flags (no longer needed)
  # OLD:
  # AM_CPPFLAGS += $(GTKGLEXT_CFLAGS)
  # LIBS += $(GTKGLEXT_LIBS)

  # NEW: Just need GL libraries
  LIBS += $(GL_LIBS)  # Should be -lGL -lGLU from configure.ac
  ```

#### Task 1.3: Create GtkGLArea Widget (1.5 hours)

- [ ] **Define GL area widget structure**

  Add to `src/hid/gtk3/gtkhid-gl.c`:

  ```c
  /* GTK3 OpenGL area widget */
  static GtkWidget *gl_area = NULL;

  /* GL state */
  typedef struct {
    gboolean initialized;
    gboolean has_depth_buffer;
    gboolean has_stencil_buffer;

    /* Viewport size */
    int width;
    int height;

    /* Error tracking */
    GError *error;
  } Ghid3GLState;

  static Ghid3GLState gl_state = {
    .initialized = FALSE,
    .has_depth_buffer = TRUE,
    .has_stencil_buffer = FALSE,
    .width = 0,
    .height = 0,
    .error = NULL
  };
  ```

- [ ] **Create GtkGLArea widget**

  ```c
  /* Create and configure GtkGLArea widget */
  GtkWidget *
  ghid3_gl_area_new(void)
  {
    gl_area = gtk_gl_area_new();

    /* Set GL requirements */
    gtk_gl_area_set_has_depth_buffer(GTK_GL_AREA(gl_area), TRUE);
    gtk_gl_area_set_has_stencil_buffer(GTK_GL_AREA(gl_area), FALSE);

    /* Enable auto-render (render on expose) */
    gtk_gl_area_set_auto_render(GTK_GL_AREA(gl_area), TRUE);

    /* Set required OpenGL version (optional) */
    gtk_gl_area_set_required_version(GTK_GL_AREA(gl_area), 2, 1);
    // OpenGL 2.1 - widely available, includes fixed-function pipeline

    /* Connect signals */
    g_signal_connect(gl_area, "realize",
                     G_CALLBACK(ghid3_gl_realize_cb), NULL);
    g_signal_connect(gl_area, "render",
                     G_CALLBACK(ghid3_gl_render_cb), NULL);
    g_signal_connect(gl_area, "resize",
                     G_CALLBACK(ghid3_gl_resize_cb), NULL);
    g_signal_connect(gl_area, "unrealize",
                     G_CALLBACK(ghid3_gl_unrealize_cb), NULL);

    /* Set size request */
    gtk_widget_set_size_request(gl_area, 640, 480);

    /* Enable event handling */
    gtk_widget_set_can_focus(gl_area, TRUE);
    gtk_widget_add_events(gl_area,
                          GDK_BUTTON_PRESS_MASK |
                          GDK_BUTTON_RELEASE_MASK |
                          GDK_POINTER_MOTION_MASK |
                          GDK_SCROLL_MASK |
                          GDK_KEY_PRESS_MASK);

    return gl_area;
  }
  ```

- [ ] **Integrate into window structure**

  Edit `src/hid/gtk3/gui-top-window.c`:

  Find where the drawing area is created and add GL area as alternative:

  ```c
  /* Create rendering area - either 2D (Cairo) or 3D (OpenGL) */
  static GtkWidget *output_widget = NULL;
  static gboolean use_gl = FALSE;  // Toggle between 2D and 3D

  static GtkWidget *
  ghid3_create_output_widget(void)
  {
    if (use_gl) {
      /* Create OpenGL area for 3D rendering */
      output_widget = ghid3_gl_area_new();
    } else {
      /* Create drawing area for 2D rendering */
      output_widget = gtk_drawing_area_new();
      g_signal_connect(output_widget, "draw",
                       G_CALLBACK(ghid3_output_draw_cb), NULL);
    }

    return output_widget;
  }
  ```

#### Task 1.4: Implement Realize Callback (1 hour)

- [ ] **Create realize callback**

  Add to `src/hid/gtk3/gtkhid-gl.c`:

  ```c
  /* Realize callback - called once when widget is realized */
  static void
  ghid3_gl_realize_cb(GtkGLArea *area, gpointer user_data)
  {
    GError *error = NULL;

    /* Make the GL context current */
    gtk_gl_area_make_current(area);

    /* Check for errors */
    error = gtk_gl_area_get_error(area);
    if (error != NULL) {
      g_warning("Failed to realize GL area: %s", error->message);
      gl_state.error = error;
      return;
    }

    /* Initialize OpenGL state */
    ghid3_gl_init();

    gl_state.initialized = TRUE;
  }

  /* Initialize OpenGL state (called from realize) */
  static void
  ghid3_gl_init(void)
  {
    /* Set clear color (background) */
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);  // Dark gray

    /* Enable depth testing */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    /* Enable smooth shading */
    glShadeModel(GL_SMOOTH);

    /* Set up hints */
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    /* Enable face culling */
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    /* Set up lighting (basic setup) */
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat light_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat light_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat light_position[] = { 1.0f, 1.0f, 1.0f, 0.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    /* Material properties */
    GLfloat mat_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat mat_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat mat_specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    GLfloat mat_shininess[] = { 50.0f };

    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

    /* Check for GL errors */
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
      g_warning("OpenGL initialization error: %s",
                gluErrorString(err));
    } else {
      g_message("OpenGL initialized successfully");
      g_message("  Vendor: %s", glGetString(GL_VENDOR));
      g_message("  Renderer: %s", glGetString(GL_RENDERER));
      g_message("  Version: %s", glGetString(GL_VERSION));
    }
  }
  ```

- [ ] **Create unrealize callback**

  ```c
  /* Unrealize callback - cleanup when widget is destroyed */
  static void
  ghid3_gl_unrealize_cb(GtkGLArea *area, gpointer user_data)
  {
    /* Make context current for cleanup */
    gtk_gl_area_make_current(area);

    /* Clean up any GL resources here */
    /* (Display lists, textures, VBOs, etc.) */

    gl_state.initialized = FALSE;
  }
  ```

#### Task 1.5: Implement Render Callback (1 hour)

- [ ] **Create render callback**

  ```c
  /* Render callback - called to draw each frame */
  static gboolean
  ghid3_gl_render_cb(GtkGLArea *area, GdkGLContext *context,
                     gpointer user_data)
  {
    /* Context is automatically current when this is called */

    if (!gl_state.initialized) {
      g_warning("GL not initialized yet");
      return FALSE;
    }

    /* Clear the screen */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* For now, just test with a simple clear color change */
    /* This will be replaced with actual 3D rendering in Day 2 */

    /* Flush to ensure commands are executed */
    glFlush();

    /* Check for errors */
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
      g_warning("OpenGL render error: %s", gluErrorString(err));
      return FALSE;
    }

    /* Return TRUE to indicate successful rendering */
    return TRUE;
  }
  ```

#### Task 1.6: Implement Resize Callback (30 minutes)

- [ ] **Create resize callback**

  ```c
  /* Resize callback - called when widget size changes */
  static void
  ghid3_gl_resize_cb(GtkGLArea *area, int width, int height,
                     gpointer user_data)
  {
    /* Context is automatically current */

    /* Update viewport */
    glViewport(0, 0, width, height);

    /* Save size */
    gl_state.width = width;
    gl_state.height = height;

    /* Set up projection matrix */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    /* Calculate aspect ratio */
    GLfloat aspect = (GLfloat)width / (GLfloat)height;

    /* Set up perspective projection */
    gluPerspective(45.0f,      /* Field of view */
                   aspect,      /* Aspect ratio */
                   0.1f,        /* Near clipping plane */
                   1000.0f);    /* Far clipping plane */

    /* Switch back to modelview */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
  }
  ```

#### Task 1.7: Test Basic GL Setup (1 hour)

- [ ] **Build GTK3 HID with GL**

  ```bash
  cd /home/user/pcb
  make clean
  make 2>&1 | tee build-gl.log

  # Check for GL-related errors
  grep "error.*GL\|error.*gl" build-gl.log
  ```

- [ ] **Test GL area creation**

  ```bash
  cd src
  ./pcb --hid gtk3 2>&1 | tee test-gl.log

  # Check console output for GL info
  grep "OpenGL" test-gl.log
  # Should show vendor, renderer, version
  ```

- [ ] **Verify GL context is created**

  Add debug output to realize callback:
  ```c
  static void
  ghid3_gl_realize_cb(GtkGLArea *area, gpointer user_data)
  {
    gtk_gl_area_make_current(area);

    printf("=== OpenGL Context Created ===\n");
    printf("GL_VENDOR: %s\n", glGetString(GL_VENDOR));
    printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));
    printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
    printf("==============================\n");

    ghid3_gl_init();
  }
  ```

- [ ] **Test with colored clear**

  Modify render callback to test:
  ```c
  static gboolean
  ghid3_gl_render_cb(GtkGLArea *area, GdkGLContext *context,
                     gpointer user_data)
  {
    /* Test clear with green color */
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);  // Green
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glFlush();
    return TRUE;
  }
  ```

  Run test:
  ```bash
  ./pcb --hid gtk3

  # Should see:
  # - Window appears
  # - GL area shows solid green color
  # - No GL errors in console
  ```

#### Task 1.8: Create 2D/3D Mode Toggle (2 hours)

- [ ] **Add menu item for 3D view**

  Edit `src/hid/gtk3/ghid-main-menu.c`:

  ```c
  /* Add View menu item for 3D toggle */
  static GtkWidget *
  ghid3_create_view_menu(void)
  {
    GtkWidget *menu, *item;

    menu = gtk_menu_new();

    /* ... existing view menu items ... */

    /* Add separator */
    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    /* Add 3D View toggle */
    item = gtk_check_menu_item_new_with_label("3D View");
    g_signal_connect(item, "toggled",
                     G_CALLBACK(ghid3_view_3d_toggled_cb), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    return menu;
  }
  ```

- [ ] **Implement toggle callback**

  ```c
  /* Global state for view mode */
  static gboolean view_3d = FALSE;
  static GtkWidget *view_2d_widget = NULL;  /* Cairo drawing area */
  static GtkWidget *view_3d_widget = NULL;  /* GL area */
  static GtkWidget *view_container = NULL;  /* Container for swapping */

  /* Callback for 3D view toggle */
  static void
  ghid3_view_3d_toggled_cb(GtkCheckMenuItem *item, gpointer user_data)
  {
    gboolean active = gtk_check_menu_item_get_active(item);

    ghid3_set_view_mode(active);
  }

  /* Switch between 2D and 3D view */
  static void
  ghid3_set_view_mode(gboolean use_3d)
  {
    GtkWidget *old_widget, *new_widget;

    if (use_3d == view_3d)
      return;  /* No change */

    view_3d = use_3d;

    /* Get widgets */
    old_widget = use_3d ? view_2d_widget : view_3d_widget;
    new_widget = use_3d ? view_3d_widget : view_2d_widget;

    /* Swap widgets in container */
    if (old_widget && gtk_widget_get_parent(old_widget)) {
      gtk_container_remove(GTK_CONTAINER(view_container), old_widget);
    }

    if (new_widget) {
      gtk_container_add(GTK_CONTAINER(view_container), new_widget);
      gtk_widget_show(new_widget);
      gtk_widget_grab_focus(new_widget);
    }

    g_message("Switched to %s view", use_3d ? "3D" : "2D");
  }
  ```

- [ ] **Set up view container in window**

  Edit `src/hid/gtk3/gui-top-window.c`:

  ```c
  /* Create output area with 2D/3D support */
  static GtkWidget *
  ghid3_create_output_area(void)
  {
    /* Create container for view widgets */
    view_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* Create 2D widget (Cairo) */
    view_2d_widget = gtk_drawing_area_new();
    gtk_widget_set_size_request(view_2d_widget, 640, 480);
    g_signal_connect(view_2d_widget, "draw",
                     G_CALLBACK(ghid3_output_draw_cb), NULL);

    /* Create 3D widget (OpenGL) */
    view_3d_widget = ghid3_gl_area_new();

    /* Start in 2D mode */
    gtk_container_add(GTK_CONTAINER(view_container), view_2d_widget);
    gtk_widget_show(view_2d_widget);

    return view_container;
  }
  ```

- [ ] **Test mode toggle**

  ```bash
  cd src
  ./pcb --hid gtk3 tests/inputs/gerberelement.pcb

  # Test:
  # 1. Should start in 2D mode (Cairo rendering)
  # 2. View → 3D View
  # 3. Should switch to 3D mode (green GL area)
  # 4. View → 3D View (uncheck)
  # 5. Should switch back to 2D mode
  ```

### End of Day 1 Checklist

- [ ] **GtkGLExt removed from GTK3 HID**
- [ ] **GtkGLArea widget created and integrated**
- [ ] **OpenGL context initializes successfully**
- [ ] **GL clear works (test color visible)**
- [ ] **2D/3D mode toggle functional**
- [ ] **No GL errors in console**
- [ ] **GL info printed (vendor, renderer, version)**

**Expected State:** Can switch between 2D and 3D modes. 3D mode shows solid color (test clear). OpenGL context is working. Ready for actual 3D rendering on Day 2.

**Next:** Day 2 will implement basic 3D rendering (axes, cube, camera controls).

---

## Day 2: Basic 3D Rendering

**Goal:** Implement basic 3D rendering infrastructure and camera controls

**Time Allocation:** 8 hours

### What You'll Accomplish

By end of Day 2, you will:
- Have 3D coordinate axes rendering
- Have a simple 3D test cube rendering
- Have a 3D grid plane rendering
- Have camera view transformations working
- Have mouse controls (rotate, zoom, pan)
- Have a complete 3D test scene

### Detailed Todo List

#### Task 2.1: Implement 3D Axes (1 hour)

- [ ] **Create axes drawing function**

  Add to `src/hid/gtk3/gtkhid-gl.c`:

  ```c
  /* Draw 3D coordinate axes */
  static void
  ghid3_gl_draw_axes(void)
  {
    GLfloat axis_length = 10.0f;
    GLfloat axis_width = 2.0f;

    glDisable(GL_LIGHTING);  /* Draw without lighting */
    glLineWidth(axis_width);

    glBegin(GL_LINES);

    /* X axis - Red */
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(axis_length, 0.0f, 0.0f);

    /* Y axis - Green */
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, axis_length, 0.0f);

    /* Z axis - Blue */
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, axis_length);

    glEnd();

    glEnable(GL_LIGHTING);
  }
  ```

- [ ] **Update render callback to draw axes**

  ```c
  static gboolean
  ghid3_gl_render_cb(GtkGLArea *area, GdkGLContext *context,
                     gpointer user_data)
  {
    if (!gl_state.initialized)
      return FALSE;

    /* Clear */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Reset modelview matrix */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* Position camera */
    gluLookAt(20.0, 20.0, 20.0,  /* Eye position */
              0.0, 0.0, 0.0,      /* Look at origin */
              0.0, 0.0, 1.0);     /* Up vector (Z is up) */

    /* Draw axes */
    ghid3_gl_draw_axes();

    glFlush();
    return TRUE;
  }
  ```

- [ ] **Test axes rendering**

  ```bash
  cd src
  ./pcb --hid gtk3
  # View → 3D View

  # Should see:
  # - Red line (X axis) to the right
  # - Green line (Y axis) forward/back
  # - Blue line (Z axis) upward
  ```

#### Task 2.2: Implement Grid Plane (1 hour)

- [ ] **Create grid drawing function**

  ```c
  /* Draw 3D grid on XY plane */
  static void
  ghid3_gl_draw_grid(GLfloat size, GLfloat step)
  {
    GLfloat i;

    glDisable(GL_LIGHTING);
    glColor3f(0.3f, 0.3f, 0.3f);  /* Gray grid */
    glLineWidth(1.0f);

    glBegin(GL_LINES);

    /* Grid lines parallel to X axis */
    for (i = -size; i <= size; i += step) {
      glVertex3f(-size, i, 0.0f);
      glVertex3f(size, i, 0.0f);
    }

    /* Grid lines parallel to Y axis */
    for (i = -size; i <= size; i += step) {
      glVertex3f(i, -size, 0.0f);
      glVertex3f(i, size, 0.0f);
    }

    glEnd();

    glEnable(GL_LIGHTING);
  }
  ```

- [ ] **Add grid to render callback**

  ```c
  static gboolean
  ghid3_gl_render_cb(GtkGLArea *area, GdkGLContext *context,
                     gpointer user_data)
  {
    /* ... clear and setup ... */

    /* Draw grid first (behind everything) */
    ghid3_gl_draw_grid(50.0f, 5.0f);  /* 50x50 grid, 5 unit spacing */

    /* Draw axes on top */
    ghid3_gl_draw_axes();

    glFlush();
    return TRUE;
  }
  ```

#### Task 2.3: Implement Test Cube (1 hour)

- [ ] **Create cube drawing function**

  ```c
  /* Draw a simple cube */
  static void
  ghid3_gl_draw_cube(GLfloat size)
  {
    GLfloat half = size / 2.0f;

    glEnable(GL_LIGHTING);

    glBegin(GL_QUADS);

    /* Front face (Z+) - Red */
    glColor3f(1.0f, 0.0f, 0.0f);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-half, -half, half);
    glVertex3f(half, -half, half);
    glVertex3f(half, half, half);
    glVertex3f(-half, half, half);

    /* Back face (Z-) - Cyan */
    glColor3f(0.0f, 1.0f, 1.0f);
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(-half, -half, -half);
    glVertex3f(-half, half, -half);
    glVertex3f(half, half, -half);
    glVertex3f(half, -half, -half);

    /* Top face (Y+) - Green */
    glColor3f(0.0f, 1.0f, 0.0f);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-half, half, -half);
    glVertex3f(-half, half, half);
    glVertex3f(half, half, half);
    glVertex3f(half, half, -half);

    /* Bottom face (Y-) - Magenta */
    glColor3f(1.0f, 0.0f, 1.0f);
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-half, -half, -half);
    glVertex3f(half, -half, -half);
    glVertex3f(half, -half, half);
    glVertex3f(-half, -half, half);

    /* Right face (X+) - Blue */
    glColor3f(0.0f, 0.0f, 1.0f);
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(half, -half, -half);
    glVertex3f(half, half, -half);
    glVertex3f(half, half, half);
    glVertex3f(half, -half, half);

    /* Left face (X-) - Yellow */
    glColor3f(1.0f, 1.0f, 0.0f);
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-half, -half, -half);
    glVertex3f(-half, -half, half);
    glVertex3f(-half, half, half);
    glVertex3f(-half, half, -half);

    glEnd();
  }
  ```

- [ ] **Add cube to scene**

  ```c
  static gboolean
  ghid3_gl_render_cb(GtkGLArea *area, GdkGLContext *context,
                     gpointer user_data)
  {
    /* ... setup ... */

    ghid3_gl_draw_grid(50.0f, 5.0f);
    ghid3_gl_draw_axes();

    /* Draw cube at origin, elevated above grid */
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 5.0f);  /* Lift above grid */
    ghid3_gl_draw_cube(8.0f);         /* 8x8x8 cube */
    glPopMatrix();

    glFlush();
    return TRUE;
  }
  ```

#### Task 2.4: Implement Camera State (1 hour)

- [ ] **Define camera state structure**

  ```c
  /* Camera state for 3D view */
  typedef struct {
    /* Rotation (in degrees) */
    GLfloat rotate_x;
    GLfloat rotate_y;
    GLfloat rotate_z;

    /* Zoom (distance from origin) */
    GLfloat distance;

    /* Pan (translation) */
    GLfloat pan_x;
    GLfloat pan_y;

    /* Target point (what we're looking at) */
    GLfloat target_x;
    GLfloat target_y;
    GLfloat target_z;
  } Ghid3Camera;

  static Ghid3Camera camera = {
    .rotate_x = 30.0f,   /* Initial rotation */
    .rotate_y = 0.0f,
    .rotate_z = 45.0f,
    .distance = 100.0f,  /* Initial distance */
    .pan_x = 0.0f,
    .pan_y = 0.0f,
    .target_x = 0.0f,
    .target_y = 0.0f,
    .target_z = 0.0f
  };
  ```

- [ ] **Create camera setup function**

  ```c
  /* Set up camera view based on current camera state */
  static void
  ghid3_gl_setup_camera(void)
  {
    GLfloat eye_x, eye_y, eye_z;

    /* Calculate camera position based on spherical coordinates */
    GLfloat theta = camera.rotate_z * M_PI / 180.0;  /* Azimuth */
    GLfloat phi = camera.rotate_x * M_PI / 180.0;    /* Elevation */

    eye_x = camera.target_x + camera.distance * cos(phi) * cos(theta);
    eye_y = camera.target_y + camera.distance * cos(phi) * sin(theta);
    eye_z = camera.target_z + camera.distance * sin(phi);

    /* Apply pan */
    eye_x += camera.pan_x;
    eye_y += camera.pan_y;

    /* Set up view matrix */
    gluLookAt(eye_x, eye_y, eye_z,
              camera.target_x, camera.target_y, camera.target_z,
              0.0f, 0.0f, 1.0f);  /* Z is up */
  }
  ```

- [ ] **Update render callback to use camera**

  ```c
  static gboolean
  ghid3_gl_render_cb(GtkGLArea *area, GdkGLContext *context,
                     gpointer user_data)
  {
    if (!gl_state.initialized)
      return FALSE;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* Set up camera view */
    ghid3_gl_setup_camera();

    /* Draw scene */
    ghid3_gl_draw_grid(50.0f, 5.0f);
    ghid3_gl_draw_axes();

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 5.0f);
    ghid3_gl_draw_cube(8.0f);
    glPopMatrix();

    glFlush();
    return TRUE;
  }
  ```

#### Task 2.5: Implement Mouse Controls (2 hours)

- [ ] **Add mouse event tracking**

  ```c
  /* Mouse state */
  static struct {
    gboolean dragging;
    gboolean panning;
    gdouble last_x;
    gdouble last_y;
  } mouse_state = {
    .dragging = FALSE,
    .panning = FALSE,
    .last_x = 0.0,
    .last_y = 0.0
  };
  ```

- [ ] **Implement button press handler**

  ```c
  /* Mouse button press in GL area */
  static gboolean
  ghid3_gl_button_press_cb(GtkWidget *widget, GdkEventButton *event,
                           gpointer user_data)
  {
    /* Left button - rotate */
    if (event->button == 1) {
      mouse_state.dragging = TRUE;
      mouse_state.last_x = event->x;
      mouse_state.last_y = event->y;
      return TRUE;
    }

    /* Middle button - pan */
    if (event->button == 2) {
      mouse_state.panning = TRUE;
      mouse_state.last_x = event->x;
      mouse_state.last_y = event->y;
      return TRUE;
    }

    return FALSE;
  }
  ```

- [ ] **Implement button release handler**

  ```c
  /* Mouse button release */
  static gboolean
  ghid3_gl_button_release_cb(GtkWidget *widget, GdkEventButton *event,
                             gpointer user_data)
  {
    if (event->button == 1) {
      mouse_state.dragging = FALSE;
      return TRUE;
    }

    if (event->button == 2) {
      mouse_state.panning = FALSE;
      return TRUE;
    }

    return FALSE;
  }
  ```

- [ ] **Implement motion handler**

  ```c
  /* Mouse motion in GL area */
  static gboolean
  ghid3_gl_motion_notify_cb(GtkWidget *widget, GdkEventMotion *event,
                            gpointer user_data)
  {
    gdouble dx, dy;

    dx = event->x - mouse_state.last_x;
    dy = event->y - mouse_state.last_y;

    if (mouse_state.dragging) {
      /* Rotate camera */
      camera.rotate_z += dx * 0.5f;  /* Horizontal rotation */
      camera.rotate_x += dy * 0.5f;  /* Vertical rotation */

      /* Clamp vertical rotation */
      if (camera.rotate_x > 89.0f)
        camera.rotate_x = 89.0f;
      if (camera.rotate_x < -89.0f)
        camera.rotate_x = -89.0f;

      /* Normalize horizontal rotation */
      while (camera.rotate_z >= 360.0f)
        camera.rotate_z -= 360.0f;
      while (camera.rotate_z < 0.0f)
        camera.rotate_z += 360.0f;

      /* Request redraw */
      gtk_gl_area_queue_render(GTK_GL_AREA(widget));
    }
    else if (mouse_state.panning) {
      /* Pan camera */
      GLfloat pan_speed = camera.distance * 0.001f;

      camera.pan_x -= dx * pan_speed;
      camera.pan_y += dy * pan_speed;

      gtk_gl_area_queue_render(GTK_GL_AREA(widget));
    }

    mouse_state.last_x = event->x;
    mouse_state.last_y = event->y;

    return TRUE;
  }
  ```

- [ ] **Implement scroll handler (zoom)**

  ```c
  /* Mouse scroll (zoom) */
  static gboolean
  ghid3_gl_scroll_cb(GtkWidget *widget, GdkEventScroll *event,
                     gpointer user_data)
  {
    gdouble delta_x, delta_y;

    if (gdk_event_get_scroll_deltas((GdkEvent *)event, &delta_x, &delta_y)) {
      /* Smooth scrolling */
      camera.distance += delta_y * camera.distance * 0.1f;
    } else {
      /* Traditional scroll wheel */
      switch (event->direction) {
        case GDK_SCROLL_UP:
          camera.distance *= 0.9f;  /* Zoom in */
          break;
        case GDK_SCROLL_DOWN:
          camera.distance *= 1.1f;  /* Zoom out */
          break;
        default:
          break;
      }
    }

    /* Clamp distance */
    if (camera.distance < 10.0f)
      camera.distance = 10.0f;
    if (camera.distance > 1000.0f)
      camera.distance = 1000.0f;

    gtk_gl_area_queue_render(GTK_GL_AREA(widget));
    return TRUE;
  }
  ```

- [ ] **Connect mouse event handlers**

  Update `ghid3_gl_area_new()`:
  ```c
  GtkWidget *
  ghid3_gl_area_new(void)
  {
    gl_area = gtk_gl_area_new();

    /* ... existing setup ... */

    /* Connect mouse event handlers */
    g_signal_connect(gl_area, "button-press-event",
                     G_CALLBACK(ghid3_gl_button_press_cb), NULL);
    g_signal_connect(gl_area, "button-release-event",
                     G_CALLBACK(ghid3_gl_button_release_cb), NULL);
    g_signal_connect(gl_area, "motion-notify-event",
                     G_CALLBACK(ghid3_gl_motion_notify_cb), NULL);
    g_signal_connect(gl_area, "scroll-event",
                     G_CALLBACK(ghid3_gl_scroll_cb), NULL);

    return gl_area;
  }
  ```

#### Task 2.6: Add Camera Reset (30 minutes)

- [ ] **Create camera reset function**

  ```c
  /* Reset camera to default view */
  void
  ghid3_gl_reset_camera(void)
  {
    camera.rotate_x = 30.0f;
    camera.rotate_y = 0.0f;
    camera.rotate_z = 45.0f;
    camera.distance = 100.0f;
    camera.pan_x = 0.0f;
    camera.pan_y = 0.0f;
    camera.target_x = 0.0f;
    camera.target_y = 0.0f;
    camera.target_z = 0.0f;

    if (gl_area)
      gtk_gl_area_queue_render(GTK_GL_AREA(gl_area));
  }
  ```

- [ ] **Add keyboard handler for reset**

  ```c
  /* Keyboard handler for GL area */
  static gboolean
  ghid3_gl_key_press_cb(GtkWidget *widget, GdkEventKey *event,
                        gpointer user_data)
  {
    switch (event->keyval) {
      case GDK_KEY_Home:
      case GDK_KEY_r:
        /* Reset camera */
        ghid3_gl_reset_camera();
        return TRUE;

      case GDK_KEY_Escape:
        /* Exit 3D mode */
        ghid3_set_view_mode(FALSE);
        return TRUE;

      default:
        break;
    }

    return FALSE;
  }

  /* Connect in ghid3_gl_area_new(): */
  g_signal_connect(gl_area, "key-press-event",
                   G_CALLBACK(ghid3_gl_key_press_cb), NULL);
  ```

#### Task 2.7: Test Complete 3D Scene (1.5 hours)

- [ ] **Build and test**

  ```bash
  cd /home/user/pcb
  make clean
  make 2>&1 | tee build-day2.log

  cd src
  ./pcb --hid gtk3
  ```

- [ ] **Test 3D rendering**

  - [ ] Switch to 3D mode (View → 3D View)
  - [ ] Verify grid renders
  - [ ] Verify axes render (red X, green Y, blue Z)
  - [ ] Verify cube renders with colored faces
  - [ ] Test lighting (cube should have shading)

- [ ] **Test camera controls**

  - [ ] Left-drag to rotate
    - Horizontal drag rotates around Z axis
    - Vertical drag changes elevation
  - [ ] Scroll wheel to zoom
    - Scroll up zooms in
    - Scroll down zooms out
  - [ ] Middle-drag to pan
    - Moves view left/right, up/down
  - [ ] Press 'r' or Home to reset camera

- [ ] **Create test checklist**

  Create `tests/3d-rendering-test.md`:
  ```markdown
  # 3D Rendering Test Checklist

  ## Basic Rendering
  - [ ] Grid visible (gray lines on XY plane)
  - [ ] Axes visible (X=red, Y=green, Z=blue)
  - [ ] Cube visible (multicolored faces)
  - [ ] Lighting works (cube has shading)
  - [ ] Depth test works (proper occlusion)

  ## Camera Controls
  - [ ] Left-drag rotates view
  - [ ] Rotation is smooth
  - [ ] Vertical rotation clamped to ±89°
  - [ ] Scroll wheel zooms in/out
  - [ ] Zoom clamped to reasonable range
  - [ ] Middle-drag pans view
  - [ ] 'r' key resets camera

  ## Mode Toggle
  - [ ] Can switch to 3D mode
  - [ ] Can switch back to 2D mode
  - [ ] No crashes during toggle

  ## Performance
  - [ ] Smooth rotation (no lag)
  - [ ] Smooth zoom
  - [ ] Frame rate > 30 FPS

  ## OpenGL
  - [ ] No GL errors in console
  - [ ] GL context info printed on realize
  ```

- [ ] **Screenshot 3D scene**

  ```bash
  # With 3D mode active
  import screenshot-day2-3d-scene.png
  ```

- [ ] **Document results**

  Update `MIGRATION_LOG.md`:
  ```markdown
  ## Day 2: Basic 3D Rendering Complete

  ### Achievements
  - ✅ 3D coordinate axes rendering
  - ✅ 3D grid plane rendering
  - ✅ Test cube with lighting
  - ✅ Camera view transformations
  - ✅ Mouse controls (rotate, zoom, pan)
  - ✅ Keyboard shortcuts (reset, exit)

  ### 3D Scene Elements
  - Grid: 50x50 units, 5 unit spacing
  - Axes: 10 unit length, colored (R/G/B)
  - Cube: 8x8x8 units, multicolored faces
  - Lighting: Single directional light

  ### Camera Controls
  - Left-drag: Rotate view
  - Scroll: Zoom in/out
  - Middle-drag: Pan view
  - 'r' or Home: Reset camera
  - Escape: Exit 3D mode

  ### Screenshots
  See docs/screenshots/milestone3a-day2-3d-scene.png
  ```

### End of Day 2 Checklist

- [ ] **3D axes rendering**
- [ ] **3D grid rendering**
- [ ] **Test cube rendering with lighting**
- [ ] **Camera transformations working**
- [ ] **Mouse controls functional (rotate, zoom, pan)**
- [ ] **Keyboard controls working**
- [ ] **Smooth, responsive interaction**
- [ ] **No GL errors**

**Expected State:** Have a complete 3D test scene with grid, axes, and test cube. Camera controls work smoothly. Can rotate, zoom, and pan the view. Ready for Day 3 testing and refinement.

**Next:** Day 3 will test on multiple platforms, optimize performance, and prepare for Milestone 3B.

---

## Day 3: Testing & Refinement

**Goal:** Test thoroughly, optimize performance, document

**Time Allocation:** 8 hours

### What You'll Accomplish

By end of Day 3, you will:
- Have tested on multiple platforms (Linux, macOS, Windows if available)
- Have performance profiled and optimized
- Have any platform-specific issues fixed
- Have complete documentation
- Have stable, tested foundation for Milestone 3B

### Detailed Todo List

#### Task 3.1: Platform Testing (2 hours)

- [ ] **Test on Linux X11**

  ```bash
  # Ensure using X11
  echo $XDG_SESSION_TYPE
  # Should show "x11"

  cd src
  ./pcb --hid gtk3

  # Test:
  # - 3D mode switch
  # - All camera controls
  # - No GL errors
  ```

- [ ] **Test on Linux Wayland (if available)**

  ```bash
  # Check if Wayland session available
  echo $XDG_SESSION_TYPE
  # If shows "wayland"

  ./pcb --hid gtk3

  # Test same as X11
  # Document any differences
  ```

- [ ] **Test on macOS (if available)**

  ```bash
  # On macOS
  ./pcb --hid gtk3

  # Check:
  # - GL context creates (may use Apple GL)
  # - Rendering works
  # - Performance acceptable
  ```

- [ ] **Test on Windows (if available)**

  ```bash
  # In MSYS2
  ./pcb --hid gtk3

  # Check:
  # - GL context creates (uses Windows GL)
  # - Rendering works
  # - Performance acceptable
  ```

- [ ] **Document platform issues**

  Create `tests/3d-platform-issues.md`:
  ```markdown
  # 3D Rendering Platform-Specific Issues

  ## Linux X11
  - Status: ✅ Working
  - GL Version: X.X
  - Issues: (none)

  ## Linux Wayland
  - Status: ✅ Working / ⚠️ Issues / ❌ Not tested
  - GL Version: X.X
  - Issues: (describe)

  ## macOS
  - Status: ✅ / ⚠️ / ❌
  - GL Version: X.X
  - Issues: (describe)

  ## Windows
  - Status: ✅ / ⚠️ / ❌
  - GL Version: X.X
  - Issues: (describe)
  ```

#### Task 3.2: Performance Testing (1.5 hours)

- [ ] **Measure frame rate**

  Add FPS counter:
  ```c
  /* FPS tracking */
  static struct {
    int frame_count;
    double last_time;
    double fps;
  } fps_tracker = {0, 0.0, 0.0};

  static void
  ghid3_gl_update_fps(void)
  {
    struct timespec now;
    double current_time, elapsed;

    clock_gettime(CLOCK_MONOTONIC, &now);
    current_time = now.tv_sec + now.tv_nsec / 1e9;

    fps_tracker.frame_count++;

    elapsed = current_time - fps_tracker.last_time;
    if (elapsed >= 1.0) {
      fps_tracker.fps = fps_tracker.frame_count / elapsed;
      g_message("FPS: %.1f", fps_tracker.fps);

      fps_tracker.frame_count = 0;
      fps_tracker.last_time = current_time;
    }
  }

  /* Call from render callback */
  static gboolean
  ghid3_gl_render_cb(GtkGLArea *area, GdkGLContext *context,
                     gpointer user_data)
  {
    /* ... rendering ... */

    ghid3_gl_update_fps();

    return TRUE;
  }
  ```

- [ ] **Run performance tests**

  ```bash
  ./pcb --hid gtk3 2>&1 | grep FPS

  # While in 3D mode:
  # - Rotate slowly: Record FPS
  # - Rotate quickly: Record FPS
  # - Zoom in/out: Record FPS
  # - Idle (no interaction): Record FPS
  ```

- [ ] **Profile with perf (Linux)**

  ```bash
  # Profile rendering
  perf record -g ./pcb --hid gtk3
  # (interact with 3D view for ~30 seconds)
  # (quit)

  perf report
  # Look for hotspots in GL code
  ```

- [ ] **Optimize if needed**

  Common optimizations:
  ```c
  /* Use display lists for static geometry */
  static GLuint grid_list = 0;

  static void
  ghid3_gl_create_grid_display_list(void)
  {
    if (grid_list)
      return;

    grid_list = glGenLists(1);
    glNewList(grid_list, GL_COMPILE);

    /* Draw grid geometry */
    ghid3_gl_draw_grid(50.0f, 5.0f);

    glEndList();
  }

  /* Call display list instead of drawing each frame */
  static void
  ghid3_gl_draw_scene(void)
  {
    if (!grid_list)
      ghid3_gl_create_grid_display_list();

    glCallList(grid_list);
  }
  ```

#### Task 3.3: Error Handling (1 hour)

- [ ] **Add comprehensive error checking**

  ```c
  /* Check for GL errors after rendering */
  static void
  ghid3_gl_check_errors(const char *location)
  {
    GLenum err;
    int error_count = 0;

    while ((err = glGetError()) != GL_NO_ERROR) {
      g_warning("OpenGL error at %s: %s",
                location, gluErrorString(err));
      error_count++;

      if (error_count > 10) {
        g_warning("Too many GL errors, stopping check");
        break;
      }
    }
  }

  /* Call after each major operation */
  static gboolean
  ghid3_gl_render_cb(GtkGLArea *area, GdkGLContext *context,
                     gpointer user_data)
  {
    /* ... rendering ... */

    ghid3_gl_check_errors("render");

    return TRUE;
  }
  ```

- [ ] **Handle GL context loss**

  ```c
  /* Check if GL context is valid */
  static gboolean
  ghid3_gl_context_valid(void)
  {
    if (!gl_area)
      return FALSE;

    GError *error = gtk_gl_area_get_error(GTK_GL_AREA(gl_area));
    if (error) {
      g_warning("GL area has error: %s", error->message);
      return FALSE;
    }

    return gl_state.initialized;
  }
  ```

- [ ] **Add fallback for no OpenGL**

  ```c
  /* Check if OpenGL is available */
  static gboolean
  ghid3_gl_available(void)
  {
    /* Try to create a test GL area */
    GtkWidget *test_area = gtk_gl_area_new();

    if (!test_area)
      return FALSE;

    gtk_widget_realize(test_area);

    GError *error = gtk_gl_area_get_error(GTK_GL_AREA(test_area));
    gtk_widget_destroy(test_area);

    if (error) {
      g_message("OpenGL not available: %s", error->message);
      g_error_free(error);
      return FALSE;
    }

    return TRUE;
  }

  /* Disable 3D view if GL not available */
  static void
  ghid3_setup_view_menu(void)
  {
    if (!ghid3_gl_available()) {
      /* Hide or disable 3D view menu item */
      gtk_widget_set_sensitive(view_3d_menuitem, FALSE);
      gtk_widget_set_tooltip_text(view_3d_menuitem,
        "OpenGL not available on this system");
    }
  }
  ```

#### Task 3.4: Code Cleanup (1 hour)

- [ ] **Remove debug output**

  ```bash
  cd src/hid/gtk3

  # Find debug printf/g_message calls
  grep -n "printf\|g_message.*DEBUG" gtkhid-gl.c

  # Remove or comment out debug output
  # Keep important messages (warnings, errors)
  ```

- [ ] **Add documentation comments**

  ```c
  /**
   * ghid3_gl_area_new - Create GtkGLArea widget for 3D rendering
   *
   * Creates and configures a GtkGLArea widget with appropriate
   * OpenGL settings for PCB 3D visualization.
   *
   * Returns: New GtkWidget (GtkGLArea)
   */
  GtkWidget *
  ghid3_gl_area_new(void)
  {
    /* ... */
  }

  /**
   * ghid3_gl_setup_camera - Configure camera view transformation
   *
   * Sets up the GL modelview matrix based on current camera state
   * (rotation, zoom, pan). Uses spherical coordinates for camera
   * position calculation.
   */
  static void
  ghid3_gl_setup_camera(void)
  {
    /* ... */
  }
  ```

- [ ] **Format code consistently**

  ```bash
  # If using clang-format
  clang-format -i gtkhid-gl.c

  # Or manually check indentation, braces, etc.
  ```

#### Task 3.5: Integration Testing (1.5 hours)

- [ ] **Test integration with 2D mode**

  ```bash
  ./pcb --hid gtk3 tests/inputs/gerberelement.pcb

  # Test sequence:
  # 1. Load board (2D mode)
  # 2. Verify 2D rendering works
  # 3. Switch to 3D mode
  # 4. Verify 3D scene appears
  # 5. Interact with 3D (rotate, zoom)
  # 6. Switch back to 2D
  # 7. Verify 2D rendering still works
  # 8. Repeat several times
  ```

- [ ] **Test with multiple boards**

  ```bash
  for board in tests/inputs/*.pcb; do
    echo "Testing: $board"
    ./pcb --hid gtk3 "$board" --quit
    if [ $? -eq 0 ]; then
      echo "  ✓ Load successful"
    else
      echo "  ✗ Load failed"
    fi
  done
  ```

- [ ] **Memory leak testing**

  ```bash
  valgrind --leak-check=full \
           --suppressions=/usr/share/gtk-3.0/valgrind/gtk.supp \
           ./pcb --hid gtk3 --quit \
           2>&1 | tee valgrind-3d.log

  grep "definitely lost" valgrind-3d.log
  ```

#### Task 3.6: Documentation (2 hours)

- [ ] **Update MIGRATION_LOG.md**

  ```markdown
  # Milestone 3A Complete - $(date)

  ## Objectives Achieved
  - ✅ GtkGLArea integrated (no GtkGLExt dependency)
  - ✅ OpenGL context creation working
  - ✅ Basic 3D rendering infrastructure
  - ✅ 3D test scene (grid, axes, cube)
  - ✅ Camera controls (rotate, zoom, pan)
  - ✅ 2D/3D mode toggle

  ## Current State

  ### Working
  - GtkGLArea widget creates GL context
  - Basic 3D scene renders
  - Grid, axes, test cube visible
  - Lighting and depth testing work
  - Camera controls smooth and responsive
  - Can switch between 2D and 3D modes

  ### Not Yet Working (Expected)
  - Real PCB rendering in 3D (Milestone 3B)
  - Layer visualization in 3D (Milestone 3B)
  - Component elevation (Milestone 3B)

  ## Performance
  - FPS (idle): ~XX FPS
  - FPS (rotating): ~XX FPS
  - FPS (zooming): ~XX FPS
  - Performance: ✅ Acceptable

  ## Platform Testing
  - Linux X11: ✅ Working
  - Linux Wayland: ✅ / ⚠️ / ❌
  - macOS: ✅ / ⚠️ / ❌
  - Windows: ✅ / ⚠️ / ❌

  ## Files Modified
  - ✅ src/hid/gtk3/gtkhid-gl.c (complete rewrite for GtkGLArea)
  - ✅ src/hid/gtk3/gui-top-window.c (2D/3D toggle)
  - ✅ src/hid/gtk3/ghid-main-menu.c (3D view menu item)

  ## Next Steps
  - Milestone 3B: Implement 3D PCB rendering
  - Draw layers with thickness
  - Render components in 3D
  - Add via visualization
  ```

- [ ] **Create Milestone 3A summary**

  Create `MILESTONE_3A_SUMMARY.md`:
  ```markdown
  # Milestone 3A: GtkGLArea Setup & Basic 3D - Summary

  **Completed:** $(date)
  **Duration:** 3 days
  **Status:** ✅ COMPLETE

  ## Achievements

  1. **GtkGLArea Integration**
     - Removed deprecated GtkGLExt dependency
     - Integrated GTK3's built-in GtkGLArea
     - Simplified OpenGL context management

  2. **OpenGL Infrastructure**
     - Context creation and initialization
     - GL state setup (depth test, lighting, etc.)
     - Error handling and validation

  3. **Basic 3D Rendering**
     - 3D coordinate axes (RGB = XYZ)
     - Grid plane (50x50 units)
     - Test cube with multicolored faces
     - Lighting with shading

  4. **Camera System**
     - Spherical coordinate camera
     - Rotation (azimuth and elevation)
     - Zoom (distance from target)
     - Pan (view translation)

  5. **Interactive Controls**
     - Left-drag: Rotate view
     - Scroll: Zoom in/out
     - Middle-drag: Pan view
     - Keyboard: Reset camera, exit 3D mode

  ## Usage

  ```bash
  # Launch GTK3 HID
  ./pcb --hid gtk3 myboard.pcb

  # Switch to 3D mode
  View → 3D View

  # Controls
  Left-drag: Rotate
  Scroll: Zoom
  Middle-drag: Pan
  'r' or Home: Reset camera
  Escape: Exit 3D mode
  ```

  ## Technical Details

  **OpenGL Version:** 2.1 (fixed-function pipeline)
  **Context Management:** Automatic via GtkGLArea
  **Rendering:** Immediate mode (glBegin/glEnd)
  **Projection:** Perspective (45° FOV)

  ## Performance

  | Metric | Value |
  |--------|-------|
  | Idle FPS | ~XX |
  | Active FPS | ~XX |
  | GL Errors | 0 |

  ## Platform Support

  - Linux (X11): ✅
  - Linux (Wayland): ✅
  - macOS: ✅/⚠️
  - Windows: ✅/⚠️

  ## Known Limitations

  - Only test geometry (no PCB rendering yet)
  - Basic lighting (single light source)
  - Fixed-function pipeline (OpenGL 2.1)

  These will be addressed in Milestone 3B.

  ## Ready for Milestone 3B

  Foundation is solid:
  - ✅ GL context working on all platforms
  - ✅ Camera controls smooth
  - ✅ Performance acceptable
  - ✅ No memory leaks
  - ✅ Well-tested and documented

  Milestone 3B can build on this to implement complete
  3D PCB visualization.
  ```

- [ ] **Commit Milestone 3A**

  ```bash
  git add -A
  git commit -m "Milestone 3A: GtkGLArea setup and basic 3D rendering

  Replaced deprecated GtkGLExt with GTK3's built-in GtkGLArea.
  Implemented basic 3D rendering infrastructure.

  GtkGLArea Integration:
  - Removed all GtkGLExt dependencies
  - Created GtkGLArea widget with OpenGL 2.1 requirement
  - Automatic context management via GTK3
  - Realize/render/resize/unrealize signal handlers

  OpenGL Initialization:
  - Depth testing and face culling
  - Basic lighting (single directional light)
  - Material properties
  - Smooth shading

  Basic 3D Rendering:
  - 3D coordinate axes (X=red, Y=green, Z=blue)
  - Grid plane on XY (50x50 units, gray lines)
  - Test cube with colored faces
  - Proper lighting and shading

  Camera System:
  - Spherical coordinate camera positioning
  - Rotation (azimuth, elevation)
  - Zoom (distance)
  - Pan (translation)
  - gluLookAt-based view setup

  Interactive Controls:
  - Left mouse drag: Rotate view
  - Scroll wheel: Zoom in/out
  - Middle mouse drag: Pan view
  - 'r' or Home key: Reset camera
  - Escape key: Exit 3D mode

  Mode Toggle:
  - View → 3D View menu item
  - Switch between 2D (Cairo) and 3D (OpenGL)
  - Both modes fully functional

  Testing:
  - Platform testing (Linux X11/Wayland)
  - Performance profiling (FPS tracking)
  - Memory leak testing (valgrind)
  - Error handling

  Performance:
  - > 60 FPS for test scene
  - Smooth camera controls
  - No memory leaks

  Usage:
    ./pcb --hid gtk3
    View → 3D View

  Status:
  - ✅ Basic 3D infrastructure complete
  - ✅ Camera controls working
  - ✅ Performance excellent
  - ⏳ Ready for Milestone 3B (3D PCB rendering)

  GTK2 HID remains fully functional."

  git tag -a milestone-3a -m "Milestone 3A: GtkGLArea & Basic 3D Complete"
  ```

- [ ] **Push to remote**

  ```bash
  git push origin gtk3-migration
  git push origin milestone-3a
  ```

### End of Day 3 / Milestone 3A Checklist

- [ ] **Platform testing complete**
- [ ] **Performance profiled and acceptable**
- [ ] **Error handling implemented**
- [ ] **Code cleaned up and documented**
- [ ] **Integration tests passing**
- [ ] **Documentation complete**
- [ ] **Code committed and tagged**

---

## Success Criteria

### At the end of Milestone 3A, you should be able to:

#### OpenGL Context
- ✅ GtkGLArea creates OpenGL context successfully
- ✅ GL context info printed (vendor, renderer, version)
- ✅ GL version >= 2.1
- ✅ No GL errors during initialization or rendering

#### 3D Rendering
- ✅ Switch to 3D mode: View → 3D View
- ✅ See 3D grid on XY plane (gray lines)
- ✅ See 3D axes (X=red, Y=green, Z=blue)
- ✅ See test cube with colored faces
- ✅ Lighting works (cube has shading)
- ✅ Depth test works (proper occlusion)

#### Camera Controls
- ✅ Left-drag rotates view smoothly
- ✅ Scroll wheel zooms in/out
- ✅ Middle-drag pans view
- ✅ 'r' or Home resets camera
- ✅ Escape exits 3D mode
- ✅ Controls are responsive (no lag)

#### Mode Toggle
- ✅ Can switch to 3D mode (View → 3D View)
- ✅ Can switch back to 2D mode
- ✅ 2D mode still works after toggling
- ✅ No crashes during mode switches

#### Performance
- ✅ FPS > 60 for test scene
- ✅ Smooth rotation
- ✅ Smooth zoom
- ✅ No lag or stuttering

#### Quality
- ✅ No memory leaks (valgrind clean)
- ✅ No GL errors in console
- ✅ Works on tested platforms

### What Should NOT Work Yet (Expected Limitations)

- ❌ Real PCB rendering in 3D - **Milestone 3B**
- ❌ Layer thickness visualization - **Milestone 3B**
- ❌ Component elevation - **Milestone 3B**
- ❌ Via 3D rendering - **Milestone 3B**

This is **expected and normal** for Milestone 3A!

---

## Troubleshooting Guide

### Common Issues and Solutions

#### Issue: GL context creation fails

**Symptoms:**
```
Failed to realize GL area: ...
GL context is NULL
```

**Solutions:**
```bash
# Check OpenGL is available
glxinfo | grep "OpenGL version"

# Check if hardware acceleration works
glxinfo | grep "direct rendering"
# Should show "direct rendering: Yes"

# If software rendering:
export LIBGL_ALWAYS_SOFTWARE=1
./pcb --hid gtk3
```

In code:
```c
// Add better error reporting
GError *error = gtk_gl_area_get_error(area);
if (error) {
  g_warning("GL Error: %s", error->message);
  g_warning("Try installing mesa-libGL or update graphics drivers");
}
```

---

#### Issue: Black screen in 3D mode

**Symptoms:**
- 3D mode activates but shows only black

**Solutions:**
```c
// Check clear color is not black
glClearColor(0.2f, 0.2f, 0.2f, 1.0f);  // Not (0,0,0,1)

// Check viewport is set
glViewport(0, 0, width, height);

// Check projection matrix
glMatrixMode(GL_PROJECTION);
glLoadIdentity();
gluPerspective(45.0, aspect, 0.1, 1000.0);

// Check camera is not at origin
// (if camera is at (0,0,0) looking at (0,0,0), you see nothing)

// Add debug output
printf("Camera: eye(%.1f,%.1f,%.1f) target(%.1f,%.1f,%.1f)\n",
       eye_x, eye_y, eye_z, target_x, target_y, target_z);
```

---

#### Issue: Objects not visible or clipped

**Symptoms:**
- Some objects disappear when camera moves
- Objects disappear when zooming

**Solutions:**
```c
// Check near/far clipping planes
gluPerspective(45.0, aspect,
               0.1,      // Near - too far means objects nearby get clipped
               1000.0);  // Far - too close means distant objects get clipped

// Adjust based on scene size
// For PCB: near=0.01, far=10000.0

// Check depth test is enabled
glEnable(GL_DEPTH_TEST);

// Clear depth buffer
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
```

---

#### Issue: Camera controls don't work

**Symptoms:**
- Mouse drag doesn't rotate
- Scroll doesn't zoom

**Solutions:**
```c
// Ensure events are connected
gtk_widget_add_events(gl_area,
                      GDK_BUTTON_PRESS_MASK |
                      GDK_BUTTON_RELEASE_MASK |
                      GDK_POINTER_MOTION_MASK |
                      GDK_SCROLL_MASK);

g_signal_connect(gl_area, "button-press-event", ...);
g_signal_connect(gl_area, "motion-notify-event", ...);
g_signal_connect(gl_area, "scroll-event", ...);

// Ensure widget can receive focus
gtk_widget_set_can_focus(gl_area, TRUE);
gtk_widget_grab_focus(gl_area);

// Check mouse_state is updated
printf("Dragging: %d, x=%.1f, y=%.1f\n",
       mouse_state.dragging, mouse_state.last_x, mouse_state.last_y);
```

---

#### Issue: Rotation is inverted or wrong direction

**Symptoms:**
- Dragging left rotates right
- Dragging up rotates down

**Solutions:**
```c
// Fix rotation direction
// If inverted, negate the delta:
camera.rotate_z += dx * 0.5f;    // Try: -= dx * 0.5f
camera.rotate_x += dy * 0.5f;    // Try: -= dy * 0.5f

// Or swap X/Y
camera.rotate_z += dy * 0.5f;    // Swap dx with dy
camera.rotate_x += dx * 0.5f;
```

---

#### Issue: Poor performance / low FPS

**Symptoms:**
- Stuttering or lag during rotation
- FPS < 30

**Solutions:**
```c
// Use display lists
GLuint list = glGenLists(1);
glNewList(list, GL_COMPILE);
// ... draw static geometry ...
glEndList();

// Later:
glCallList(list);

// Enable VSync (prevent tearing, may limit FPS)
// This is controlled by compositor/driver

// Reduce complexity
// - Draw less geometry
// - Disable lighting for testing
// - Use simpler shapes

// Check GL errors aren't slowing down
// Comment out ghid3_gl_check_errors() in render loop
```

---

#### Issue: Window doesn't resize properly

**Symptoms:**
- GL viewport stays same size when window resized
- Aspect ratio wrong after resize

**Solutions:**
```c
// Ensure resize callback connected
g_signal_connect(gl_area, "resize",
                 G_CALLBACK(ghid3_gl_resize_cb), NULL);

// Update viewport in resize callback
glViewport(0, 0, width, height);

// Update projection matrix
GLfloat aspect = (GLfloat)width / (GLfloat)height;
glMatrixMode(GL_PROJECTION);
glLoadIdentity();
gluPerspective(45.0, aspect, 0.1, 1000.0);
```

---

## Summary

At the end of Milestone 3A, you will have:

✅ GtkGLArea widget integrated (no GtkGLExt dependency)
✅ OpenGL context creation working
✅ Basic 3D rendering (grid, axes, cube)
✅ Camera system with transformations
✅ Interactive controls (rotate, zoom, pan)
✅ 2D/3D mode toggle functional
✅ Tested and optimized

**Time Investment:** 24 hours (3 days × 8 hours)
**Lines of Code:** ~1,500 new OpenGL code
**Code Quality:** Tested, performant, production-ready

**Ready for:** Milestone 3B - Complete 3D PCB Rendering

---

**Document Version:** 1.0
**Last Updated:** November 17, 2025
**Status:** Ready for Use
