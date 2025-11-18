# Milestone 2A: Cairo Foundation & Basic Drawing - Detailed Implementation Plan

**GTK3 Migration - Week 2 (Days 1-3)**
**Duration:** 3 days (24 hours)
**Goal:** Create parallel GTK3 HID with basic Cairo drawing infrastructure

---

## Overview

Milestone 2A establishes the GTK3 HID as a separate, parallel HID alongside the existing GTK2 HID. By the end of this phase, you will have:
- A complete `src/hid/gtk3/` directory structure
- GTK3 HID registered and selectable with `--hid gtk3`
- Basic Cairo drawing infrastructure working
- Simple geometric shapes rendering (lines, rectangles, arcs, circles)
- Foundation ready for complete PCB rendering (Milestone 2B)

**Key Principle:** The GTK2 HID remains completely untouched and functional. Users can choose which HID to use.

---

## Table of Contents

1. [Day 1: Parallel HID Structure Setup](#day-1-parallel-hid-structure-setup)
2. [Day 2: Cairo Infrastructure & Basic Primitives](#day-2-cairo-infrastructure--basic-primitives)
3. [Day 3: Testing & Integration](#day-3-testing--integration)
4. [Success Criteria](#success-criteria)
5. [Troubleshooting Guide](#troubleshooting-guide)

---

## Day 1: Parallel HID Structure Setup

**Goal:** Create gtk3/ directory, copy and adapt GTK2 files, register new HID

**Time Allocation:** 8 hours

### What You'll Accomplish

By end of Day 1, you will:
- Have complete `src/hid/gtk3/` directory with all files
- Have GTK3 HID registered in build system
- Have basic compilation working (even if functionality incomplete)
- Be able to see `gtk3` in `./pcb --hid-list`
- Have renamed functions to avoid conflicts with GTK2 HID

### Detailed Todo List

#### Task 1.1: Create GTK3 HID Directory Structure (1 hour)

- [ ] **Create gtk3 directory**
  ```bash
  cd /home/user/pcb/src/hid
  mkdir gtk3
  ls -la
  # Should show both gtk/ and gtk3/
  ```

- [ ] **Copy all GTK2 HID files to gtk3**
  ```bash
  cd /home/user/pcb/src/hid
  cp -r gtk/* gtk3/

  # Verify all files copied
  ls -la gtk3/
  # Should show all 24+ files from gtk/
  ```

- [ ] **Create backup of original GTK2 files**
  ```bash
  cd /home/user/pcb/src/hid
  tar -czf gtk2-backup-$(date +%Y%m%d).tar.gz gtk/
  ls -lh gtk2-backup-*.tar.gz
  # Keep this backup safe!
  ```

- [ ] **Document the parallel structure**

  Create `src/hid/gtk3/README.md`:
  ```markdown
  # GTK3 HID - Parallel Implementation

  This directory contains the GTK3-based HID, developed in parallel
  with the existing GTK2 HID in `src/hid/gtk/`.

  ## Status

  - **Milestone 1:** Build system and window structure (COMPLETE)
  - **Milestone 2A:** Cairo drawing infrastructure (IN PROGRESS)
  - **Milestone 2B:** Complete PCB rendering (PLANNED)

  ## Usage

  ```bash
  # Use GTK3 HID
  ./pcb --hid gtk3 myboard.pcb

  # Compare with GTK2
  ./pcb --hid gtk myboard.pcb  # or just ./pcb myboard.pcb
  ```

  ## Files Adapted from GTK2 HID

  All files in this directory were copied from `src/hid/gtk/` and
  adapted for GTK3:
  - Function names prefixed with `ghid3_` to avoid conflicts
  - GTK3 APIs used throughout
  - Cairo-based drawing instead of GDK

  ## Development Status

  See MILESTONE_2A_CAIRO_FOUNDATION.md for current status.
  ```

#### Task 1.2: Rename Functions to Avoid Conflicts (2 hours)

Since we're running both HIDs in parallel, we need unique function names.

- [ ] **Create rename script**

  Create `src/hid/gtk3/rename-functions.sh`:
  ```bash
  #!/bin/bash
  # Rename GTK2 function prefixes to GTK3 to avoid conflicts

  set -e

  echo "Renaming functions in gtk3/ directory..."

  # Rename ghid_ to ghid3_
  find . -type f -name "*.c" -o -name "*.h" | while read file; do
    echo "Processing: $file"
    sed -i 's/\bghid_/ghid3_/g' "$file"
  done

  # Rename HID structure name
  sed -i 's/\bgtk_hid\b/gtk3_hid/g' *.c *.h
  sed -i 's/\bgtk_gui\b/gtk3_gui/g' *.c *.h

  # Rename any other GTK2-specific identifiers
  sed -i 's/\bGTK_HID\b/GTK3_HID/g' *.c *.h

  echo "Done! All ghid_ functions are now ghid3_"
  ```

- [ ] **Run rename script**
  ```bash
  cd /home/user/pcb/src/hid/gtk3
  chmod +x rename-functions.sh
  ./rename-functions.sh 2>&1 | tee rename.log

  # Verify changes
  grep -r "ghid3_" . | wc -l
  # Should show many occurrences

  grep -r "\bghid_" . | grep -v "ghid3_" | wc -l
  # Should show 0 (all renamed)
  ```

- [ ] **Update HID name and description**

  Edit `src/hid/gtk3/gtkhid-main.c`:

  Find the HID structure (around line 50-100):
  ```c
  // OLD:
  HID gtk_hid = {
    .name = "gtk",
    .description = "Gtk+ 2.0 based GUI",
    // ...
  };

  // NEW:
  HID gtk3_hid = {
    .name = "gtk3",
    .description = "GTK+ 3.0 based GUI (experimental)",
    // ...
  };
  ```

- [ ] **Verify no function name conflicts**
  ```bash
  # Build both directories (will fail, but check for conflicts)
  cd /home/user/pcb
  grep -r "^ghid_" src/hid/gtk/*.c | cut -d: -f2 | sort > /tmp/gtk-funcs.txt
  grep -r "^ghid3_" src/hid/gtk3/*.c | cut -d: -f2 | sort > /tmp/gtk3-funcs.txt

  # Should be no overlap
  comm -12 /tmp/gtk-funcs.txt /tmp/gtk3-funcs.txt
  # Should produce no output (no conflicts)
  ```

#### Task 1.3: Update Build System for Parallel HIDs (2 hours)

- [ ] **Create Makefile.am for gtk3/**

  Edit `src/hid/gtk3/Makefile.am`:

  ```makefile
  # GTK3 HID Makefile

  # Note the different library name
  noinst_LIBRARIES = libhid-gtk3.a

  # Use GTK3 flags
  AM_CPPFLAGS = \
      -I$(top_srcdir) \
      -I$(top_srcdir)/src \
      $(GTK_CFLAGS) \
      $(GL_CFLAGS) \
      -DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_3_22 \
      -DGDK_VERSION_MAX_ALLOWED=GDK_VERSION_3_24

  # All source files (same as GTK2, but in gtk3/)
  libhid_gtk3_a_SOURCES = \
      gtkhid-main.c \
      gtkhid-gdk.c \
      gtkhid-gl.c \
      gui-top-window.c \
      gui-output-events.c \
      gui-dialog.c \
      gui-dialog-print.c \
      gui-config.c \
      gui-drc-window.c \
      gui-library-window.c \
      gui-netlist-window.c \
      gui-log-window.c \
      gui-command-window.c \
      gui-keyref-window.c \
      gui-pinout-window.c \
      gui-pinout-preview.c \
      gui-misc.c \
      gui-utils.c \
      gui-trackball.c \
      ghid-main-menu.c \
      ghid-layer-selector.c \
      ghid-route-style-selector.c \
      ghid-coord-entry.c \
      ghid-cell-renderer-visibility.c

  # Headers
  noinst_HEADERS = \
      gui.h \
      gtkhid.h \
      ghid-coord-entry.h \
      ghid-main-menu.h \
      ghid-layer-selector.h \
      ghid-route-style-selector.h \
      ghid-cell-renderer-visibility.h \
      gui-drc-window.h \
      gui-library-window.h \
      gui-pinout-preview.h \
      gui-trackball.h

  # Data files
  EXTRA_DIST = \
      gui-icons-mode-buttons.data \
      gui-icons-misc.data \
      pcb.rc \
      hid.conf
  ```

- [ ] **Update src/hid/Makefile.am to include gtk3**

  Edit `src/hid/Makefile.am`:

  ```makefile
  # Add gtk3 to SUBDIRS
  # Find the line with SUBDIRS and add gtk3

  # OLD:
  SUBDIRS = common gtk lesstif ...

  # NEW:
  SUBDIRS = common gtk gtk3 lesstif ...
  ```

- [ ] **Update configure.ac to build gtk3/**

  Edit `configure.ac`:

  Find the section with `AC_CONFIG_FILES` (near end of file):
  ```bash
  AC_CONFIG_FILES([
    Makefile
    src/Makefile
    src/hid/Makefile
    src/hid/common/Makefile
    src/hid/gtk/Makefile
    src/hid/gtk3/Makefile        # ADD THIS LINE
    src/hid/lesstif/Makefile
    ...
  ])
  ```

- [ ] **Register GTK3 HID in main HID list**

  Edit `src/main.c` (or wherever HIDs are registered):

  Find the HID registration section:
  ```c
  // External HID declarations
  extern HID gtk_hid;
  extern HID gtk3_hid;     // ADD THIS
  extern HID lesstif_hid;

  // HID registration
  static HID *hid_list[] = {
  #ifdef HAVE_GTK
    &gtk_hid,
  #endif
  #ifdef HAVE_GTK3        // ADD THIS BLOCK
    &gtk3_hid,
  #endif
  #ifdef HAVE_LESSTIF
    &lesstif_hid,
  #endif
    // ... other HIDs
    NULL
  };
  ```

  **Note:** We'll define `HAVE_GTK3` in configure.ac

- [ ] **Update configure.ac to define HAVE_GTK3**

  Edit `configure.ac`:

  After the GTK3 detection (added in Milestone 1):
  ```bash
  PKG_CHECK_MODULES(GTK, gtk+-3.0 >= 3.22.0, ,
    [AC_MSG_ERROR([Cannot find gtk+ >= 3.22.0...])])

  GTK_VERSION=`$PKG_CONFIG gtk+-3.0 --modversion`

  # Define HAVE_GTK3 for conditional compilation
  AC_DEFINE([HAVE_GTK3], 1, [Define if GTK3 HID is available])
  AM_CONDITIONAL([HAVE_GTK3], [test "x$GTK_VERSION" != "x"])
  ```

#### Task 1.4: Fix Compilation Issues (2 hours)

- [ ] **Regenerate build system**
  ```bash
  cd /home/user/pcb
  autoreconf -vif

  # Or use autogen if available
  ./autogen.sh
  ```

- [ ] **Configure with both HIDs**
  ```bash
  ./configure 2>&1 | tee configure-parallel.log

  # Verify both detected
  grep "GTK" configure-parallel.log
  # Should show GTK3 version detected
  ```

- [ ] **Attempt initial build**
  ```bash
  cd /home/user/pcb
  make 2>&1 | tee build-parallel.log
  ```

  **Expected:** Many errors in gtk3/ - this is normal! We're fixing them now.

- [ ] **Fix linking issues**

  The main executable needs to link both HIDs.

  Edit `src/Makefile.am`:
  ```makefile
  # OLD:
  pcb_LDADD = \
      hid/common/libhidcommon.a \
      hid/gtk/libhid-gtk.a \
      ...

  # NEW:
  pcb_LDADD = \
      hid/common/libhidcommon.a \
      hid/gtk/libhid-gtk.a \
      hid/gtk3/libhid-gtk3.a \     # ADD THIS
      ...

  # Add GTK3 libs to linking
  pcb_LDADD += $(GTK_LIBS) $(GL_LIBS)
  ```

- [ ] **Build gtk3 directory specifically**
  ```bash
  cd src/hid/gtk3
  make 2>&1 | tee build-gtk3.log

  # Count errors
  grep "error:" build-gtk3.log | wc -l
  ```

- [ ] **Fix most common errors**

  Most errors should be from Milestone 1-style issues. Quick fixes:

  **1. Update all widget access:**
  ```bash
  cd src/hid/gtk3

  # Create quick-fix script
  cat > quick-fix.sh << 'EOF'
  #!/bin/bash
  for file in *.c; do
    echo "Fixing: $file"
    # These are safe automated fixes
    sed -i 's/GTK_WIDGET_VISIBLE(/gtk_widget_get_visible(/g' "$file"
    sed -i 's/GTK_WIDGET_REALIZED(/gtk_widget_get_realized(/g' "$file"
    sed -i 's/GDK_\([A-Z][a-z_]*\)\b/GDK_KEY_\1/g' "$file"
  done
  EOF

  chmod +x quick-fix.sh
  ./quick-fix.sh
  ```

  **2. Apply Milestone 1 fixes to all files:**

  For each .c file, apply the same fixes as Milestone 1:
  - `gtk_hbox_new()` → `gtk_box_new(GTK_ORIENTATION_HORIZONTAL, ...)`
  - `gtk_vbox_new()` → `gtk_box_new(GTK_ORIENTATION_VERTICAL, ...)`
  - `gtk_table_new()` → `gtk_grid_new()`
  - `widget->window` → `gtk_widget_get_window(widget)`
  - `widget->allocation` → `gtk_widget_get_allocation(widget, &allocation)`

  **Note:** You can reuse the work from Milestone 1 (gtkhid-main.c, gui-top-window.c)

#### Task 1.5: Test HID Registration (1 hour)

- [ ] **Build until no critical errors**
  ```bash
  cd /home/user/pcb
  make 2>&1 | tee build-test.log

  # Focus on getting src/pcb to link
  # Some gtk3 files can have warnings, but main binary must build
  ```

- [ ] **Test HID listing**
  ```bash
  cd src
  ./pcb --hid-list
  ```

  **Expected output:**
  ```
  Available HIDs:
    gtk          GTK+ 2.0 based GUI
    gtk3         GTK+ 3.0 based GUI (experimental)
    lesstif      Lesstif-based GUI
    ps           PostScript export
    gerber       Gerber export
    ...
  ```

  ✅ **Success:** Both `gtk` and `gtk3` appear in the list!

- [ ] **Test basic GTK3 HID launch**
  ```bash
  ./pcb --hid gtk3 2>&1 | tee gtk3-launch.log
  ```

  **Expected:**
  - Window appears (may be broken/empty)
  - No immediate crash
  - GTK3 initialized

  **Document issues:**
  ```bash
  echo "=== GTK3 HID First Launch ===" >> /home/user/pcb/MIGRATION_LOG.md
  echo "Date: $(date)" >> /home/user/pcb/MIGRATION_LOG.md
  echo "" >> /home/user/pcb/MIGRATION_LOG.md
  cat gtk3-launch.log >> /home/user/pcb/MIGRATION_LOG.md
  ```

- [ ] **Compare with GTK2 HID**
  ```bash
  # Launch GTK2 (should still work!)
  ./pcb --hid gtk 2>&1 | tee gtk2-verify.log

  # Verify GTK2 is unaffected
  echo "GTK2 HID still works: YES/NO" >> /home/user/pcb/MIGRATION_LOG.md
  ```

### End of Day 1 Checklist

- [ ] **gtk3/ directory created with all files**
- [ ] **All functions renamed to ghid3_ prefix**
- [ ] **Build system configured for parallel HIDs**
- [ ] **Both HIDs compile (even with warnings)**
- [ ] **Both HIDs appear in --hid-list**
- [ ] **GTK2 HID still fully functional**
- [ ] **GTK3 HID launches (even if broken)**

**Expected State:** Both HIDs exist in parallel. GTK2 works perfectly. GTK3 compiles and launches but may be non-functional. Build system supports both.

**Next:** Day 2 will implement Cairo drawing infrastructure.

---

## Day 2: Cairo Infrastructure & Basic Primitives

**Goal:** Implement Cairo drawing infrastructure and basic geometric primitives

**Time Allocation:** 8 hours

### What You'll Accomplish

By end of Day 2, you will:
- Have Cairo drawing infrastructure in place
- Have basic primitive functions implemented (lines, rectangles, arcs)
- Have a test drawing function showing geometric shapes
- Be able to see simple shapes rendered in GTK3 HID window

### Detailed Todo List

#### Task 2.1: Create Cairo Infrastructure (2 hours)

- [ ] **Create Cairo drawing context structure**

  Edit `src/hid/gtk3/gtkhid-gdk.c`:

  Add near top of file:
  ```c
  /* GTK3/Cairo drawing context */
  typedef struct {
    cairo_t *cr;              /* Cairo context */
    GdkWindow *drawable;      /* Target window */
    double scale;             /* Current zoom scale */
    int width, height;        /* Drawable size */

    /* Current drawing state */
    struct {
      double r, g, b, a;      /* Current color */
      double line_width;      /* Current line width */
      cairo_line_cap_t cap;   /* Line cap style */
      cairo_line_join_t join; /* Line join style */
    } state;

    /* Color cache */
    GHashTable *color_cache;  /* GdkColor -> cairo RGB cache */
  } Ghid3DrawingContext;

  static Ghid3DrawingContext *drawing_context = NULL;
  ```

- [ ] **Create context initialization function**

  Add to `gtkhid-gdk.c`:
  ```c
  /* Initialize Cairo drawing context */
  static Ghid3DrawingContext *
  ghid3_drawing_context_new(GdkWindow *window)
  {
    Ghid3DrawingContext *ctx;

    ctx = g_new0(Ghid3DrawingContext, 1);
    ctx->drawable = window;
    ctx->scale = 1.0;

    /* Default drawing state */
    ctx->state.r = 0.0;
    ctx->state.g = 0.0;
    ctx->state.b = 0.0;
    ctx->state.a = 1.0;
    ctx->state.line_width = 1.0;
    ctx->state.cap = CAIRO_LINE_CAP_ROUND;
    ctx->state.join = CAIRO_LINE_JOIN_ROUND;

    /* Create color cache */
    ctx->color_cache = g_hash_table_new_full(
      g_str_hash, g_str_equal, g_free, g_free);

    return ctx;
  }

  /* Free drawing context */
  static void
  ghid3_drawing_context_free(Ghid3DrawingContext *ctx)
  {
    if (!ctx)
      return;

    if (ctx->color_cache)
      g_hash_table_destroy(ctx->color_cache);

    g_free(ctx);
  }

  /* Begin drawing (creates Cairo context) */
  static gboolean
  ghid3_drawing_begin(Ghid3DrawingContext *ctx, cairo_t *cr)
  {
    if (!ctx)
      return FALSE;

    ctx->cr = cr;

    /* Get drawable size */
    ctx->width = gdk_window_get_width(ctx->drawable);
    ctx->height = gdk_window_get_height(ctx->drawable);

    /* Set up initial Cairo state */
    cairo_set_source_rgba(ctx->cr,
                          ctx->state.r,
                          ctx->state.g,
                          ctx->state.b,
                          ctx->state.a);
    cairo_set_line_width(ctx->cr, ctx->state.line_width);
    cairo_set_line_cap(ctx->cr, ctx->state.cap);
    cairo_set_line_join(ctx->cr, ctx->state.join);

    return TRUE;
  }

  /* End drawing */
  static void
  ghid3_drawing_end(Ghid3DrawingContext *ctx)
  {
    if (!ctx)
      return;

    /* Cairo context is managed by GTK3, don't destroy it */
    ctx->cr = NULL;
  }
  ```

- [ ] **Create color management functions**

  Add to `gtkhid-gdk.c`:
  ```c
  /* Set drawing color from GdkColor (GTK2 compatibility) */
  static void
  ghid3_set_color_gdk(Ghid3DrawingContext *ctx, GdkColor *color)
  {
    if (!ctx || !color)
      return;

    /* Convert GdkColor (0-65535) to Cairo RGB (0.0-1.0) */
    ctx->state.r = color->red / 65535.0;
    ctx->state.g = color->green / 65535.0;
    ctx->state.b = color->blue / 65535.0;

    if (ctx->cr) {
      cairo_set_source_rgba(ctx->cr,
                            ctx->state.r,
                            ctx->state.g,
                            ctx->state.b,
                            ctx->state.a);
    }
  }

  /* Set drawing color from RGB values */
  static void
  ghid3_set_color_rgb(Ghid3DrawingContext *ctx,
                      double r, double g, double b)
  {
    if (!ctx)
      return;

    ctx->state.r = r;
    ctx->state.g = g;
    ctx->state.b = b;

    if (ctx->cr) {
      cairo_set_source_rgba(ctx->cr, r, g, b, ctx->state.a);
    }
  }

  /* Set line width */
  static void
  ghid3_set_line_width(Ghid3DrawingContext *ctx, double width)
  {
    if (!ctx)
      return;

    ctx->state.line_width = width;

    if (ctx->cr) {
      cairo_set_line_width(ctx->cr, width);
    }
  }

  /* Set line cap style */
  static void
  ghid3_set_line_cap(Ghid3DrawingContext *ctx, cairo_line_cap_t cap)
  {
    if (!ctx)
      return;

    ctx->state.cap = cap;

    if (ctx->cr) {
      cairo_set_line_cap(ctx->cr, cap);
    }
  }
  ```

#### Task 2.2: Implement Basic Drawing Primitives (3 hours)

- [ ] **Implement line drawing**

  Add to `gtkhid-gdk.c`:
  ```c
  /* Draw a line */
  static void
  ghid3_draw_line(Ghid3DrawingContext *ctx,
                  int x1, int y1, int x2, int y2)
  {
    if (!ctx || !ctx->cr)
      return;

    cairo_move_to(ctx->cr, x1, y1);
    cairo_line_to(ctx->cr, x2, y2);
    cairo_stroke(ctx->cr);
  }
  ```

- [ ] **Implement rectangle drawing**

  ```c
  /* Draw a rectangle */
  static void
  ghid3_draw_rectangle(Ghid3DrawingContext *ctx,
                       gboolean filled,
                       int x, int y, int width, int height)
  {
    if (!ctx || !ctx->cr)
      return;

    cairo_rectangle(ctx->cr, x, y, width, height);

    if (filled)
      cairo_fill(ctx->cr);
    else
      cairo_stroke(ctx->cr);
  }
  ```

- [ ] **Implement arc drawing**

  ```c
  /* Draw an arc (angles in degrees) */
  static void
  ghid3_draw_arc(Ghid3DrawingContext *ctx,
                 gboolean filled,
                 int cx, int cy, int radius,
                 int start_angle, int delta_angle)
  {
    if (!ctx || !ctx->cr)
      return;

    /* Convert degrees to radians */
    double start_rad = start_angle * M_PI / 180.0;
    double end_rad = (start_angle + delta_angle) * M_PI / 180.0;

    /* Draw arc */
    if (delta_angle > 0) {
      cairo_arc(ctx->cr, cx, cy, radius, start_rad, end_rad);
    } else {
      cairo_arc_negative(ctx->cr, cx, cy, radius, start_rad, end_rad);
    }

    if (filled) {
      /* For filled arc, close path to center */
      cairo_line_to(ctx->cr, cx, cy);
      cairo_close_path(ctx->cr);
      cairo_fill(ctx->cr);
    } else {
      cairo_stroke(ctx->cr);
    }
  }
  ```

- [ ] **Implement circle drawing**

  ```c
  /* Draw a circle */
  static void
  ghid3_draw_circle(Ghid3DrawingContext *ctx,
                    gboolean filled,
                    int cx, int cy, int radius)
  {
    if (!ctx || !ctx->cr)
      return;

    cairo_arc(ctx->cr, cx, cy, radius, 0, 2 * M_PI);

    if (filled)
      cairo_fill(ctx->cr);
    else
      cairo_stroke(ctx->cr);
  }
  ```

- [ ] **Implement polygon drawing**

  ```c
  /* Draw a polygon */
  static void
  ghid3_draw_polygon(Ghid3DrawingContext *ctx,
                     gboolean filled,
                     int n_points,
                     int *x, int *y)
  {
    int i;

    if (!ctx || !ctx->cr || n_points < 2)
      return;

    /* Start path at first point */
    cairo_move_to(ctx->cr, x[0], y[0]);

    /* Draw lines to remaining points */
    for (i = 1; i < n_points; i++) {
      cairo_line_to(ctx->cr, x[i], y[i]);
    }

    /* Close path */
    cairo_close_path(ctx->cr);

    if (filled)
      cairo_fill(ctx->cr);
    else
      cairo_stroke(ctx->cr);
  }
  ```

#### Task 2.3: Create Test Drawing Function (1 hour)

- [ ] **Create geometric test pattern**

  Add to `gtkhid-gdk.c`:
  ```c
  /* Test function - draw geometric shapes to verify Cairo works */
  static void
  ghid3_draw_test_pattern(Ghid3DrawingContext *ctx)
  {
    int cx, cy;

    if (!ctx || !ctx->cr)
      return;

    cx = ctx->width / 2;
    cy = ctx->height / 2;

    /* Clear background */
    ghid3_set_color_rgb(ctx, 0.95, 0.95, 0.95);
    ghid3_draw_rectangle(ctx, TRUE, 0, 0, ctx->width, ctx->height);

    /* Draw grid pattern */
    ghid3_set_color_rgb(ctx, 0.8, 0.8, 0.8);
    ghid3_set_line_width(ctx, 1.0);

    for (int x = 0; x < ctx->width; x += 50) {
      ghid3_draw_line(ctx, x, 0, x, ctx->height);
    }
    for (int y = 0; y < ctx->height; y += 50) {
      ghid3_draw_line(ctx, 0, y, ctx->width, y);
    }

    /* Draw test shapes */

    /* Red rectangle */
    ghid3_set_color_rgb(ctx, 1.0, 0.0, 0.0);
    ghid3_set_line_width(ctx, 2.0);
    ghid3_draw_rectangle(ctx, FALSE, 50, 50, 100, 80);

    /* Green filled circle */
    ghid3_set_color_rgb(ctx, 0.0, 0.8, 0.0);
    ghid3_draw_circle(ctx, TRUE, cx, cy, 60);

    /* Blue arc */
    ghid3_set_color_rgb(ctx, 0.0, 0.0, 1.0);
    ghid3_set_line_width(ctx, 3.0);
    ghid3_draw_arc(ctx, FALSE, cx + 100, cy, 50, 0, 270);

    /* Black lines */
    ghid3_set_color_rgb(ctx, 0.0, 0.0, 0.0);
    ghid3_set_line_width(ctx, 1.5);
    ghid3_draw_line(ctx, 10, 10, cx - 10, cy - 10);
    ghid3_draw_line(ctx, ctx->width - 10, 10, cx + 10, cy - 10);

    /* Orange polygon */
    ghid3_set_color_rgb(ctx, 1.0, 0.5, 0.0);
    int poly_x[] = {200, 250, 240, 210, 160};
    int poly_y[] = {200, 220, 270, 280, 240};
    ghid3_draw_polygon(ctx, FALSE, 5, poly_x, poly_y);

    /* Text label */
    cairo_set_source_rgb(ctx->cr, 0.2, 0.2, 0.2);
    cairo_select_font_face(ctx->cr, "Sans",
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(ctx->cr, 16.0);
    cairo_move_to(ctx->cr, 10, ctx->height - 10);
    cairo_show_text(ctx->cr, "GTK3 HID - Cairo Test Pattern");
  }
  ```

#### Task 2.4: Integrate with Draw Signal (1.5 hours)

- [ ] **Update draw callback in gui-output-events.c**

  Edit `src/hid/gtk3/gui-output-events.c`:

  Find the draw callback (from Milestone 1 stub):
  ```c
  static gboolean
  ghid3_output_draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data)
  {
    GdkWindow *window;

    window = gtk_widget_get_window(widget);
    if (!window)
      return FALSE;

    /* Initialize drawing context if needed */
    if (!drawing_context) {
      drawing_context = ghid3_drawing_context_new(window);
    }

    /* Begin drawing */
    if (!ghid3_drawing_begin(drawing_context, cr)) {
      return FALSE;
    }

    /* Draw test pattern */
    ghid3_draw_test_pattern(drawing_context);

    /* End drawing */
    ghid3_drawing_end(drawing_context);

    return FALSE;
  }
  ```

- [ ] **Ensure drawing area widget is set up**

  Edit `src/hid/gtk3/gui-top-window.c`:

  Find the drawing area creation:
  ```c
  /* Create drawing area for rendering */
  drawing_area = gtk_drawing_area_new();
  gtk_widget_set_size_request(drawing_area, 800, 600);

  /* Set up drawing events */
  gtk_widget_set_can_focus(drawing_area, TRUE);
  gtk_widget_add_events(drawing_area,
                        GDK_EXPOSURE_MASK |
                        GDK_BUTTON_PRESS_MASK |
                        GDK_BUTTON_RELEASE_MASK |
                        GDK_POINTER_MOTION_MASK |
                        GDK_SCROLL_MASK |
                        GDK_KEY_PRESS_MASK |
                        GDK_KEY_RELEASE_MASK);

  /* Connect draw signal */
  g_signal_connect(drawing_area, "draw",
                   G_CALLBACK(ghid3_output_draw_cb), NULL);

  gtk_widget_show(drawing_area);
  ```

#### Task 2.5: Compile and Test (30 minutes)

- [ ] **Build gtk3 HID**
  ```bash
  cd /home/user/pcb
  make clean
  make 2>&1 | tee build-day2.log

  # Check for errors in gtk3/
  grep "src/hid/gtk3.*error:" build-day2.log
  ```

- [ ] **Fix compilation errors**

  Common issues:
  - Missing function declarations: Add prototypes
  - Undefined variables: Check context usage
  - Type mismatches: Verify Cairo types

- [ ] **Test draw function**
  ```bash
  cd src
  ./pcb --hid gtk3 2>&1 | tee test-draw.log
  ```

  **Expected result:**
  - Window appears
  - Test pattern visible (grid, shapes, text)
  - Red rectangle, green circle, blue arc visible
  - No crashes

- [ ] **Take screenshot**
  ```bash
  # With PCB running
  import screenshot-day2-test-pattern.png

  # Or use system screenshot tool
  ```

  Save to `docs/screenshots/milestone2a-day2.png`

- [ ] **Document test results**

  Update `MIGRATION_LOG.md`:
  ```markdown
  ## Day 2: Cairo Infrastructure Complete

  ### Achievements
  - ✅ Cairo drawing context created
  - ✅ Basic primitives implemented (line, rect, arc, circle, polygon)
  - ✅ Test pattern renders correctly
  - ✅ Colors working
  - ✅ Line widths working

  ### Test Pattern Results
  - Grid: ✅ Visible
  - Red rectangle: ✅ Visible
  - Green circle: ✅ Visible
  - Blue arc: ✅ Visible
  - Polygon: ✅ Visible
  - Text: ✅ Visible

  ### Screenshots
  See docs/screenshots/milestone2a-day2.png

  ### Known Issues
  - (list any issues)

  ### Next Steps
  - Day 3: Integration testing, performance, bug fixes
  ```

### End of Day 2 Checklist

- [ ] **Cairo drawing context infrastructure complete**
- [ ] **All basic primitives implemented and working**
- [ ] **Test pattern renders correctly**
- [ ] **No crashes in drawing code**
- [ ] **Screenshot captured showing test pattern**
- [ ] **Documentation updated**

**Expected State:** GTK3 HID can draw basic geometric shapes using Cairo. Test pattern displays correctly showing all primitives work.

**Next:** Day 3 will test, optimize, and prepare for Milestone 2B.

---

## Day 3: Testing & Integration

**Goal:** Thorough testing, performance optimization, documentation

**Time Allocation:** 8 hours

### What You'll Accomplish

By end of Day 3, you will:
- Have tested all drawing primitives thoroughly
- Have performance benchmarks established
- Have side-by-side comparison with GTK2 working
- Have documentation complete
- Be ready for Milestone 2B (PCB rendering)

### Detailed Todo List

#### Task 3.1: Comprehensive Primitive Testing (2 hours)

- [ ] **Create systematic test suite**

  Create `src/hid/gtk3/test-primitives.c`:
  ```c
  /* Test all Cairo drawing primitives */

  #include "gui.h"

  /* Test line drawing - various widths and styles */
  static void
  test_lines(Ghid3DrawingContext *ctx, int x, int y)
  {
    /* Horizontal lines with different widths */
    for (int i = 1; i <= 5; i++) {
      ghid3_set_line_width(ctx, i);
      ghid3_draw_line(ctx, x, y + i * 10, x + 100, y + i * 10);
    }

    /* Vertical lines */
    for (int i = 1; i <= 5; i++) {
      ghid3_set_line_width(ctx, i);
      ghid3_draw_line(ctx, x + 120 + i * 10, y, x + 120 + i * 10, y + 50);
    }

    /* Diagonal lines */
    ghid3_set_line_width(ctx, 2.0);
    for (int i = 0; i < 5; i++) {
      ghid3_draw_line(ctx, x + 200, y + i * 10, x + 250, y + 50 - i * 10);
    }
  }

  /* Test rectangles - filled and outlined */
  static void
  test_rectangles(Ghid3DrawingContext *ctx, int x, int y)
  {
    /* Outlined rectangles with different widths */
    for (int i = 1; i <= 3; i++) {
      ghid3_set_line_width(ctx, i);
      ghid3_draw_rectangle(ctx, FALSE, x + i * 40, y, 30, 30);
    }

    /* Filled rectangles with different colors */
    ghid3_set_color_rgb(ctx, 1.0, 0.0, 0.0);
    ghid3_draw_rectangle(ctx, TRUE, x, y + 50, 30, 30);

    ghid3_set_color_rgb(ctx, 0.0, 1.0, 0.0);
    ghid3_draw_rectangle(ctx, TRUE, x + 40, y + 50, 30, 30);

    ghid3_set_color_rgb(ctx, 0.0, 0.0, 1.0);
    ghid3_draw_rectangle(ctx, TRUE, x + 80, y + 50, 30, 30);
  }

  /* Test circles and arcs */
  static void
  test_circles_arcs(Ghid3DrawingContext *ctx, int x, int y)
  {
    /* Outlined circles */
    ghid3_set_color_rgb(ctx, 0.0, 0.0, 0.0);
    ghid3_set_line_width(ctx, 1.0);
    ghid3_draw_circle(ctx, FALSE, x + 20, y + 20, 15);

    ghid3_set_line_width(ctx, 3.0);
    ghid3_draw_circle(ctx, FALSE, x + 60, y + 20, 15);

    /* Filled circles */
    ghid3_set_color_rgb(ctx, 0.8, 0.4, 0.0);
    ghid3_draw_circle(ctx, TRUE, x + 100, y + 20, 15);

    /* Arcs - different angles */
    ghid3_set_color_rgb(ctx, 0.0, 0.0, 0.0);
    ghid3_set_line_width(ctx, 2.0);

    ghid3_draw_arc(ctx, FALSE, x + 20, y + 60, 15, 0, 90);
    ghid3_draw_arc(ctx, FALSE, x + 60, y + 60, 15, 45, 180);
    ghid3_draw_arc(ctx, FALSE, x + 100, y + 60, 15, -45, -270);

    /* Filled arcs */
    ghid3_set_color_rgb(ctx, 0.5, 0.0, 0.5);
    ghid3_draw_arc(ctx, TRUE, x + 20, y + 100, 15, 0, 120);
  }

  /* Test polygons */
  static void
  test_polygons(Ghid3DrawingContext *ctx, int x, int y)
  {
    /* Triangle */
    int tri_x[] = {x, x + 30, x + 15};
    int tri_y[] = {y + 30, y + 30, y};

    ghid3_set_color_rgb(ctx, 0.0, 0.0, 0.0);
    ghid3_set_line_width(ctx, 2.0);
    ghid3_draw_polygon(ctx, FALSE, 3, tri_x, tri_y);

    /* Pentagon */
    int pent_x[] = {x + 60, x + 75, x + 70, x + 50, x + 45};
    int pent_y[] = {y, y + 10, y + 30, y + 30, y + 10};

    ghid3_set_color_rgb(ctx, 0.0, 0.6, 0.6);
    ghid3_draw_polygon(ctx, TRUE, 5, pent_x, pent_y);

    /* Hexagon */
    int hex_x[] = {x + 100, x + 115, x + 115, x + 100, x + 85, x + 85};
    int hex_y[] = {y, y + 8, y + 22, y + 30, y + 22, y + 8};

    ghid3_set_color_rgb(ctx, 0.0, 0.0, 0.0);
    ghid3_set_line_width(ctx, 1.5);
    ghid3_draw_polygon(ctx, FALSE, 6, hex_x, hex_y);
  }

  /* Master test function */
  void
  ghid3_run_primitive_tests(Ghid3DrawingContext *ctx)
  {
    /* Clear background */
    ghid3_set_color_rgb(ctx, 1.0, 1.0, 1.0);
    ghid3_draw_rectangle(ctx, TRUE, 0, 0, ctx->width, ctx->height);

    /* Draw test sections */
    test_lines(ctx, 10, 10);
    test_rectangles(ctx, 10, 100);
    test_circles_arcs(ctx, 10, 200);
    test_polygons(ctx, 10, 350);

    /* Labels */
    cairo_set_source_rgb(ctx->cr, 0.0, 0.0, 0.0);
    cairo_select_font_face(ctx->cr, "Sans",
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(ctx->cr, 12.0);

    cairo_move_to(ctx->cr, 10, 90);
    cairo_show_text(ctx->cr, "Lines (various widths)");

    cairo_move_to(ctx->cr, 10, 190);
    cairo_show_text(ctx->cr, "Rectangles (outlined & filled)");

    cairo_move_to(ctx->cr, 10, 340);
    cairo_show_text(ctx->cr, "Circles & Arcs");

    cairo_move_to(ctx->cr, 10, 450);
    cairo_show_text(ctx->cr, "Polygons");
  }
  ```

- [ ] **Add test mode to GTK3 HID**

  Edit `src/hid/gtk3/gtkhid-main.c`:

  Add command-line option:
  ```c
  static gboolean test_mode = FALSE;

  static GOptionEntry ghid3_option_entries[] = {
    {"test-primitives", 't', 0, G_OPTION_ARG_NONE, &test_mode,
     "Run Cairo primitive tests", NULL},
    // ... other options
    {NULL}
  };
  ```

  In draw callback, check test mode:
  ```c
  if (test_mode) {
    ghid3_run_primitive_tests(drawing_context);
  } else {
    ghid3_draw_test_pattern(drawing_context);
  }
  ```

- [ ] **Run comprehensive tests**
  ```bash
  cd src
  ./pcb --hid gtk3 --test-primitives
  ```

  **Verify all shapes render correctly:**
  - [ ] Lines at different widths
  - [ ] Horizontal, vertical, diagonal lines
  - [ ] Outlined rectangles
  - [ ] Filled rectangles with colors
  - [ ] Outlined circles
  - [ ] Filled circles
  - [ ] Arcs at various angles
  - [ ] Filled arcs
  - [ ] Polygons (triangle, pentagon, hexagon)

- [ ] **Screenshot all tests**
  ```bash
  import screenshot-primitive-tests.png
  ```

#### Task 3.2: Performance Testing (1.5 hours)

- [ ] **Create performance benchmark**

  Create `src/hid/gtk3/benchmark.c`:
  ```c
  /* Performance benchmark for Cairo drawing */

  #include <time.h>
  #include "gui.h"

  /* Benchmark line drawing */
  static double
  benchmark_lines(Ghid3DrawingContext *ctx, int count)
  {
    struct timespec start, end;
    double elapsed;

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < count; i++) {
      int x1 = rand() % ctx->width;
      int y1 = rand() % ctx->height;
      int x2 = rand() % ctx->width;
      int y2 = rand() % ctx->height;

      ghid3_draw_line(ctx, x1, y1, x2, y2);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed = (end.tv_sec - start.tv_sec) +
              (end.tv_nsec - start.tv_nsec) / 1e9;

    return elapsed;
  }

  /* Benchmark rectangle drawing */
  static double
  benchmark_rectangles(Ghid3DrawingContext *ctx, int count)
  {
    struct timespec start, end;
    double elapsed;

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < count; i++) {
      int x = rand() % ctx->width;
      int y = rand() % ctx->height;
      int w = rand() % 100 + 10;
      int h = rand() % 100 + 10;
      gboolean filled = rand() % 2;

      ghid3_draw_rectangle(ctx, filled, x, y, w, h);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed = (end.tv_sec - start.tv_sec) +
              (end.tv_nsec - start.tv_nsec) / 1e9;

    return elapsed;
  }

  /* Benchmark circle drawing */
  static double
  benchmark_circles(Ghid3DrawingContext *ctx, int count)
  {
    struct timespec start, end;
    double elapsed;

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < count; i++) {
      int cx = rand() % ctx->width;
      int cy = rand() % ctx->height;
      int r = rand() % 50 + 5;
      gboolean filled = rand() % 2;

      ghid3_draw_circle(ctx, filled, cx, cy, r);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed = (end.tv_sec - start.tv_sec) +
              (end.tv_nsec - start.tv_nsec) / 1e9;

    return elapsed;
  }

  /* Run all benchmarks */
  void
  ghid3_run_benchmarks(Ghid3DrawingContext *ctx)
  {
    double t_lines, t_rects, t_circles;
    int count = 10000;

    printf("\n=== GTK3 HID Cairo Performance Benchmarks ===\n\n");

    /* Lines */
    t_lines = benchmark_lines(ctx, count);
    printf("Lines:      %d drawn in %.3f seconds (%.0f lines/sec)\n",
           count, t_lines, count / t_lines);

    /* Rectangles */
    t_rects = benchmark_rectangles(ctx, count);
    printf("Rectangles: %d drawn in %.3f seconds (%.0f rects/sec)\n",
           count, t_rects, count / t_rects);

    /* Circles */
    t_circles = benchmark_circles(ctx, count);
    printf("Circles:    %d drawn in %.3f seconds (%.0f circles/sec)\n",
           count, t_circles, count / t_circles);

    printf("\n");
  }
  ```

- [ ] **Add benchmark mode**

  In `gtkhid-main.c`:
  ```c
  static gboolean benchmark_mode = FALSE;

  {"benchmark", 'b', 0, G_OPTION_ARG_NONE, &benchmark_mode,
   "Run performance benchmarks", NULL},
  ```

- [ ] **Run benchmarks**
  ```bash
  cd src
  ./pcb --hid gtk3 --benchmark 2>&1 | tee benchmark-results.txt
  ```

  **Expected performance:**
  - Lines: > 50,000/sec
  - Rectangles: > 30,000/sec
  - Circles: > 20,000/sec

  (Actual numbers depend on hardware)

- [ ] **Compare with GTK2 HID (if possible)**

  If GTK2 HID has benchmarks, run them:
  ```bash
  ./pcb --hid gtk --benchmark 2>&1 | tee benchmark-gtk2.txt

  # Compare
  diff benchmark-gtk2.txt benchmark-results.txt
  ```

  **Goal:** GTK3 should be comparable or faster than GTK2

#### Task 3.3: Side-by-Side Comparison (1 hour)

- [ ] **Create comparison script**

  Create `scripts/compare-hids.sh`:
  ```bash
  #!/bin/bash
  # Launch GTK2 and GTK3 HIDs side-by-side for visual comparison

  BOARD=$1

  if [ -z "$BOARD" ]; then
    echo "Usage: $0 <board.pcb>"
    exit 1
  fi

  cd src

  # Launch GTK2 HID
  ./pcb --hid gtk "$BOARD" &
  GTK2_PID=$!

  sleep 1

  # Launch GTK3 HID
  ./pcb --hid gtk3 "$BOARD" &
  GTK3_PID=$!

  echo "GTK2 HID: PID $GTK2_PID (left)"
  echo "GTK3 HID: PID $GTK3_PID (right)"
  echo ""
  echo "Compare the two windows visually"
  echo "Press Enter to terminate both..."

  read

  kill $GTK2_PID $GTK3_PID 2>/dev/null
  ```

  Make executable:
  ```bash
  chmod +x scripts/compare-hids.sh
  ```

- [ ] **Run side-by-side comparison**
  ```bash
  # With test pattern
  scripts/compare-hids.sh

  # Position windows side-by-side
  # Take screenshot showing both
  ```

- [ ] **Visual comparison checklist**

  Create `tests/visual-comparison.md`:
  ```markdown
  # GTK2 vs GTK3 Visual Comparison

  ## Test Pattern Comparison

  | Feature | GTK2 | GTK3 | Match? |
  |---------|------|------|--------|
  | Grid lines | ☐ | ☐ | ☐ |
  | Red rectangle | ☐ | ☐ | ☐ |
  | Green circle | ☐ | ☐ | ☐ |
  | Blue arc | ☐ | ☐ | ☐ |
  | Polygon | ☐ | ☐ | ☐ |
  | Text rendering | ☐ | ☐ | ☐ |
  | Line widths | ☐ | ☐ | ☐ |
  | Colors accurate | ☐ | ☐ | ☐ |

  ## Differences Noted

  (Document any visual differences)

  ## Acceptable Differences

  - Anti-aliasing: GTK3/Cairo may be smoother
  - Font rendering: May differ slightly
  - Line ends: Cairo uses different algorithm

  ## Screenshots

  GTK2: docs/screenshots/gtk2-test-pattern.png
  GTK3: docs/screenshots/gtk3-test-pattern.png
  ```

#### Task 3.4: Memory Leak Testing (1 hour)

- [ ] **Run valgrind on GTK3 HID**
  ```bash
  cd src
  valgrind --leak-check=full \
           --show-leak-kinds=definite \
           --suppressions=/usr/share/glib-2.0/valgrind/glib.supp \
           --suppressions=/usr/share/gtk-3.0/valgrind/gtk.supp \
           ./pcb --hid gtk3 --test-primitives --quit \
           2>&1 | tee valgrind-gtk3.log
  ```

- [ ] **Analyze leaks**
  ```bash
  grep "definitely lost" valgrind-gtk3.log
  grep "possibly lost" valgrind-gtk3.log

  # Check for Cairo-specific leaks
  grep -A5 "cairo" valgrind-gtk3.log | grep "lost"
  ```

- [ ] **Fix critical leaks**

  Common Cairo leaks:
  ```c
  // Leak: Surface not destroyed
  cairo_surface_t *surface = cairo_image_surface_create(...);
  // ... use surface ...
  // MISSING: cairo_surface_destroy(surface);

  // Leak: Context not unreferenced
  // NOTE: In GTK3 draw callback, DON'T destroy the cairo_t
  // It's managed by GTK3. But in other contexts:
  cairo_t *cr = cairo_create(surface);
  // ... use cr ...
  cairo_destroy(cr);  // Must destroy if you created it
  ```

- [ ] **Document leak status**
  ```markdown
  ## Memory Leak Analysis (Milestone 2A)

  ### Valgrind Summary
  - Definitely lost: X bytes in Y blocks
  - Possibly lost: X bytes in Y blocks

  ### Analysis
  - GTK3 internal allocations: (list)
  - Cairo internal allocations: (list)
  - Our code leaks: (list and fix)

  ### Status
  - ✅ No critical leaks in drawing code
  - ⚠️ Minor leaks: (describe)
  ```

#### Task 3.5: Documentation & Cleanup (1.5 hours)

- [ ] **Update MIGRATION_LOG.md**
  ```markdown
  # Milestone 2A Complete - $(date)

  ## Objectives Achieved
  - ✅ Parallel GTK3 HID structure created
  - ✅ Cairo drawing infrastructure implemented
  - ✅ Basic primitives working (line, rect, circle, arc, polygon)
  - ✅ Test pattern rendering
  - ✅ Performance benchmarks completed

  ## Current State

  ### Working
  - GTK3 HID registers and launches (--hid gtk3)
  - Basic Cairo drawing functions operational
  - All geometric primitives render correctly
  - Colors, line widths, styles working
  - Performance acceptable (see benchmarks)

  ### Not Yet Working (Expected)
  - Real PCB rendering (Milestone 2B)
  - Complex PCB shapes (pads, traces, vias) (Milestone 2B)
  - Full event handling (Milestone 2B)

  ## Files Modified/Created

  ### New Directory
  - ✅ src/hid/gtk3/ (complete parallel HID)

  ### Key Files
  - ✅ src/hid/gtk3/gtkhid-gdk.c (Cairo infrastructure)
  - ✅ src/hid/gtk3/gui-output-events.c (draw callback)
  - ✅ src/hid/gtk3/gtkhid-main.c (HID registration)
  - ✅ src/hid/gtk3/test-primitives.c (test suite)
  - ✅ src/hid/gtk3/benchmark.c (performance tests)

  ## Performance Benchmarks

  (Paste benchmark results)

  ## Visual Comparison

  GTK2 vs GTK3 test patterns: MATCH ✅ / DIFFERENCES (describe)

  See tests/visual-comparison.md

  ## Memory Leaks

  (Paste valgrind summary)

  ## Next Steps
  - Milestone 2B: Implement PCB-specific drawing
  - Complex shapes (pads, traces, vias, arcs)
  - Real PCB file rendering
  - Event handling integration
  ```

- [ ] **Create Milestone 2A summary document**

  Create `MILESTONE_2A_SUMMARY.md`:
  ```markdown
  # Milestone 2A: Cairo Foundation - Summary

  **Completed:** $(date)
  **Duration:** 3 days
  **Status:** ✅ COMPLETE

  ## Achievements

  1. **Parallel HID Structure**
     - Created src/hid/gtk3/ directory
     - All GTK2 files copied and adapted
     - Function names updated (ghid_ → ghid3_)
     - HID registered as "gtk3"

  2. **Cairo Infrastructure**
     - Drawing context structure
     - Color management
     - Line style management
     - Context lifecycle functions

  3. **Basic Primitives**
     - ✅ Lines (various widths, styles)
     - ✅ Rectangles (outlined, filled)
     - ✅ Circles (outlined, filled)
     - ✅ Arcs (various angles, outlined, filled)
     - ✅ Polygons (arbitrary points)

  4. **Testing**
     - Comprehensive primitive tests
     - Performance benchmarks
     - Visual comparison with GTK2
     - Memory leak analysis

  ## Usage

  ```bash
  # Launch GTK3 HID
  ./pcb --hid gtk3 myboard.pcb

  # Run primitive tests
  ./pcb --hid gtk3 --test-primitives

  # Run benchmarks
  ./pcb --hid gtk3 --benchmark

  # Compare with GTK2
  ./scripts/compare-hids.sh myboard.pcb
  ```

  ## Performance

  (Summary of benchmark results)

  ## Known Limitations

  - Cannot render real PCB files yet
  - No complex PCB shapes (pads, traces, etc.)
  - Limited event handling

  These will be addressed in Milestone 2B.

  ## Ready for Milestone 2B

  Foundation is solid:
  - ✅ Drawing infrastructure working
  - ✅ Basic primitives tested
  - ✅ Performance acceptable
  - ✅ Memory management correct

  Milestone 2B can build on this foundation to implement
  complete PCB rendering.
  ```

- [ ] **Clean up code**
  ```bash
  cd src/hid/gtk3

  # Remove commented debug code
  find . -name "*.c" -exec sed -i '/\/\/ DEBUG/d' {} \;

  # Format code consistently
  # (if you have a code formatter like clang-format)
  ```

- [ ] **Commit Milestone 2A**
  ```bash
  git add -A
  git commit -m "Milestone 2A: Cairo foundation and basic drawing complete

  Created parallel GTK3 HID with Cairo drawing infrastructure.

  Structure:
  - src/hid/gtk3/ parallel HID directory
  - All functions renamed (ghid_ → ghid3_)
  - HID registered as 'gtk3'

  Cairo Infrastructure:
  - Drawing context with state management
  - Color conversion (GdkColor → Cairo RGB)
  - Line width and style management

  Primitives Implemented:
  - Lines (various widths)
  - Rectangles (outlined and filled)
  - Circles (outlined and filled)
  - Arcs (various angles, outlined and filled)
  - Polygons (arbitrary points)

  Testing:
  - Comprehensive primitive test suite
  - Performance benchmarks (X lines/sec, Y rects/sec, etc.)
  - Visual comparison with GTK2 HID
  - Memory leak analysis with valgrind

  Usage:
    ./pcb --hid gtk3                  Launch GTK3 HID
    ./pcb --hid gtk3 --test-primitives  Run tests
    ./pcb --hid gtk3 --benchmark        Run benchmarks

  Status:
  - ✅ Basic drawing infrastructure complete
  - ✅ All primitives working correctly
  - ✅ Performance acceptable
  - ⏳ Ready for Milestone 2B (PCB rendering)

  GTK2 HID remains fully functional and unaffected."

  git tag -a milestone-2a -m "Milestone 2A: Cairo Foundation Complete"
  ```

- [ ] **Push to remote**
  ```bash
  git push origin gtk3-migration
  git push origin milestone-2a
  ```

### End of Day 3 / Milestone 2A Checklist

- [ ] **All primitives tested and working**
- [ ] **Performance benchmarks completed**
- [ ] **Visual comparison done**
- [ ] **Memory leaks checked and fixed**
- [ ] **Documentation complete**
- [ ] **Code committed and tagged**
- [ ] **Ready for Milestone 2B**

---

## Success Criteria

### At the end of Milestone 2A, you should be able to:

#### HID Selection
- ✅ Run `./pcb --hid-list` and see both `gtk` and `gtk3`
- ✅ Launch with GTK2: `./pcb --hid gtk`
- ✅ Launch with GTK3: `./pcb --hid gtk3`
- ✅ GTK2 HID completely unaffected by GTK3 changes

#### Basic Drawing
- ✅ See test pattern in GTK3 window
- ✅ Lines render at various widths
- ✅ Rectangles render (outlined and filled)
- ✅ Circles render (outlined and filled)
- ✅ Arcs render at various angles
- ✅ Polygons render correctly
- ✅ Colors display correctly
- ✅ Text renders

#### Testing
- ✅ Run primitive test suite: `./pcb --hid gtk3 --test-primitives`
- ✅ All shapes in test suite display correctly
- ✅ Run benchmarks: `./pcb --hid gtk3 --benchmark`
- ✅ Performance is acceptable (comparable to GTK2)
- ✅ No memory leaks in drawing code (per valgrind)

#### Code Quality
- ✅ No compilation errors
- ✅ No GTK-CRITICAL errors in console
- ✅ GTK warnings documented
- ✅ Code properly formatted
- ✅ Functions properly documented

### What Should NOT Work Yet (Expected Limitations)

- ❌ Real PCB file rendering - **Milestone 2B**
- ❌ PCB-specific shapes (traces, pads, vias) - **Milestone 2B**
- ❌ Complex filled polygons with holes - **Milestone 2B**
- ❌ Full mouse/keyboard event handling - **Milestone 2B**
- ❌ OpenGL rendering - **Milestone 3**

This is **expected and normal** for Milestone 2A!

---

## Troubleshooting Guide

### Common Issues and Solutions

#### Issue: "undefined reference to ghid3_*"

**Symptoms:**
```
undefined reference to `ghid3_draw_line'
undefined reference to `ghid3_drawing_context_new'
```

**Solutions:**
```bash
# Ensure gtkhid-gdk.c is in Makefile.am
grep "gtkhid-gdk.c" src/hid/gtk3/Makefile.am

# Rebuild
cd src/hid/gtk3
make clean
make
```

---

#### Issue: Cairo functions not found

**Symptoms:**
```
error: implicit declaration of function 'cairo_move_to'
```

**Solutions:**
```c
// Ensure cairo.h is included
#include <cairo.h>

// Check pkg-config
pkg-config --cflags --libs cairo

// Add to Makefile if needed
AM_CPPFLAGS += $(shell pkg-config --cflags cairo)
LIBS += $(shell pkg-config --libs cairo)
```

---

#### Issue: Test pattern doesn't appear

**Symptoms:**
- Window launches but is blank
- No shapes visible

**Solutions:**
```c
// Check draw callback is connected
g_signal_connect(drawing_area, "draw",
                 G_CALLBACK(ghid3_output_draw_cb), NULL);

// Ensure widget is shown
gtk_widget_show(drawing_area);
gtk_widget_show_all(main_window);

// Check cairo context
if (!ctx->cr) {
  g_warning("Cairo context is NULL!");
  return FALSE;
}

// Verify drawing is called
printf("Draw callback called\n");  // Debug
```

---

#### Issue: Shapes render but wrong colors

**Symptoms:**
- Shapes visible but all same color
- Colors not matching test pattern

**Solutions:**
```c
// Verify color conversion
GdkColor color = {0, 65535, 0, 0};  // Red
ghid3_set_color_gdk(ctx, &color);

// Should set ctx->state.r = 1.0, g = 0.0, b = 0.0

// Check Cairo source is set
cairo_set_source_rgba(ctx->cr,
                      ctx->state.r,
                      ctx->state.g,
                      ctx->state.b,
                      ctx->state.a);
```

---

#### Issue: Lines too thin or thick

**Symptoms:**
- Lines barely visible or way too thick

**Solutions:**
```c
// Check line width setting
ghid3_set_line_width(ctx, 2.0);  // Reasonable width

// Verify Cairo line width
cairo_set_line_width(ctx->cr, ctx->state.line_width);

// Check coordinate system scaling
// If window is scaled differently, adjust line width accordingly
```

---

#### Issue: Arcs render incorrectly

**Symptoms:**
- Arcs appear as strange shapes
- Arc angles wrong

**Solutions:**
```c
// Check angle conversion (degrees → radians)
double start_rad = start_angle * M_PI / 180.0;
double end_rad = (start_angle + delta_angle) * M_PI / 180.0;

// Check arc direction
if (delta_angle > 0)
  cairo_arc(cr, cx, cy, radius, start_rad, end_rad);  // Counter-clockwise
else
  cairo_arc_negative(cr, cx, cy, radius, start_rad, end_rad);  // Clockwise

// Ensure M_PI is defined
#include <math.h>
```

---

#### Issue: Polygon doesn't close properly

**Symptoms:**
- Polygon has gap at start/end
- Fill doesn't work

**Solutions:**
```c
// Ensure cairo_close_path() is called
cairo_move_to(cr, x[0], y[0]);
for (i = 1; i < n_points; i++) {
  cairo_line_to(cr, x[i], y[i]);
}
cairo_close_path(cr);  // Critical for polygons!

if (filled)
  cairo_fill(cr);
else
  cairo_stroke(cr);
```

---

#### Issue: Poor performance

**Symptoms:**
- Sluggish rendering
- Benchmarks show low numbers
- Window updates slowly

**Solutions:**
```c
// Check for unnecessary operations in draw callback
// Each draw call should be efficient

// Avoid creating/destroying contexts repeatedly
// Reuse drawing_context across draw calls

// Use cairo_save/restore for temporary state
cairo_save(cr);
// ... draw with modified state ...
cairo_restore(cr);

// Consider using cairo_push_group for complex compositing
cairo_push_group(cr);
// ... draw complex shapes ...
cairo_pop_group_to_source(cr);
cairo_paint(cr);
```

---

## Summary

At the end of Milestone 2A, you will have:

✅ Parallel GTK3 HID structure (src/hid/gtk3/)
✅ Cairo drawing infrastructure complete
✅ Basic geometric primitives working
✅ Test suite and benchmarks operational
✅ Foundation for PCB rendering (Milestone 2B)

**Time Investment:** 24 hours (3 days × 8 hours)
**Lines of Code:** ~1,500 new + ~24,000 copied/adapted
**Code Quality:** Tested, benchmarked, no critical leaks

**Ready for:** Milestone 2B - Complete PCB Rendering

---

**Document Version:** 1.0
**Last Updated:** November 17, 2025
**Status:** Ready for Use
