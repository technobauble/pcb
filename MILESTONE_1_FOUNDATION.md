# Milestone 1: Foundation - Detailed Implementation Plan

**GTK3 Migration - Week 1**
**Duration:** 5 days (40 hours)
**Goal:** Establish GTK3 build environment and migrate core initialization code

---

## Overview

Milestone 1 establishes the foundation for the GTK3 migration. By the end of this phase, you will have:
- A working GTK3 build environment
- Core GTK initialization code migrated
- Main window structure converted to GTK3
- Application launching successfully with basic UI

This milestone is **critical** because it sets up the infrastructure for all subsequent work. Once complete, you'll have a compiling, launchable application (even if drawing doesn't work yet).

---

## Table of Contents

1. [Day 1: Build System Setup](#day-1-build-system-setup)
2. [Day 2: Core Initialization Migration](#day-2-core-initialization-migration)
3. [Day 3: Main Window Layout - Part 1](#day-3-main-window-layout---part-1)
4. [Day 4: Main Window Layout - Part 2](#day-4-main-window-layout---part-2)
5. [Day 5: Integration Testing & Bug Fixes](#day-5-integration-testing--bug-fixes)
6. [Success Criteria](#success-criteria)
7. [Troubleshooting Guide](#troubleshooting-guide)

---

## Day 1: Build System Setup

**Goal:** Configure build system for GTK3, verify dependencies, create development environment

**Time Allocation:** 8 hours

### What You'll Accomplish

By end of Day 1, you will:
- Have GTK3 libraries installed on your system
- Have updated configure.ac for GTK3 detection
- Have updated Makefiles for GTK3 compilation
- Be able to run `./configure` successfully with GTK3
- Have a clean build environment ready

### Detailed Todo List

#### Task 1.1: Environment Preparation (1 hour)

- [ ] **Create feature branch**
  ```bash
  git checkout main
  git pull origin main
  git checkout -b gtk3-migration
  git push -u origin gtk3-migration
  ```

- [ ] **Install GTK3 development libraries**

  **On Ubuntu/Debian:**
  ```bash
  sudo apt-get update
  sudo apt-get install -y \
    libgtk-3-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    pkg-config \
    autoconf \
    automake \
    libtool
  ```

  **On Fedora/RHEL:**
  ```bash
  sudo dnf install -y \
    gtk3-devel \
    mesa-libGL-devel \
    mesa-libGLU-devel \
    pkgconfig \
    autoconf \
    automake \
    libtool
  ```

  **On macOS:**
  ```bash
  brew install gtk+3 pkg-config autoconf automake libtool
  ```

  **On Windows (MSYS2):**
  ```bash
  pacman -S \
    mingw-w64-x86_64-gtk3 \
    mingw-w64-x86_64-mesa \
    mingw-w64-x86_64-toolchain \
    autoconf \
    automake \
    libtool
  ```

- [ ] **Verify GTK3 installation**
  ```bash
  pkg-config --modversion gtk+-3.0
  # Should output version >= 3.22.0

  pkg-config --cflags gtk+-3.0
  # Should output compile flags

  pkg-config --libs gtk+-3.0
  # Should output link flags
  ```

- [ ] **Create backup of current working GTK2 version**
  ```bash
  # Build and test current GTK2 version
  ./autogen.sh
  ./configure
  make clean
  make

  # Create backup
  cp src/pcb src/pcb-gtk2-backup

  # Test it works
  ./src/pcb-gtk2-backup --version
  ```

#### Task 1.2: Update configure.ac (2 hours)

- [ ] **Backup original configure.ac**
  ```bash
  cp configure.ac configure.ac.gtk2-backup
  ```

- [ ] **Locate GTK2 check in configure.ac**
  ```bash
  grep -n "gtk+-2.0" configure.ac
  # Note the line number (around line 764)
  ```

- [ ] **Update GTK version check**

  Find this section (around line 764):
  ```bash
  PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.18.0, ,
    [AC_MSG_ERROR([Cannot find gtk+ >= 2.18.0, install it and rerun ./configure
  ```

  Replace with:
  ```bash
  PKG_CHECK_MODULES(GTK, gtk+-3.0 >= 3.22.0, ,
    [AC_MSG_ERROR([Cannot find gtk+ >= 3.22.0, install it and rerun ./configure
  Please install GTK3 development packages:
    Debian/Ubuntu: sudo apt-get install libgtk-3-dev
    Fedora/RHEL:   sudo dnf install gtk3-devel
    macOS:         brew install gtk+3
    Windows/MSYS2: pacman -S mingw-w64-x86_64-gtk3
  ])])
  ```

- [ ] **Update GTK version variable**

  Find:
  ```bash
  GTK_VERSION=`$PKG_CONFIG gtk+-2.0 --modversion`
  ```

  Replace with:
  ```bash
  GTK_VERSION=`$PKG_CONFIG gtk+-3.0 --modversion`
  AC_MSG_RESULT([Using GTK+ version $GTK_VERSION])
  ```

- [ ] **Remove GtkGLExt dependency check**

  Find this section (search for "gtkglext"):
  ```bash
  PKG_CHECK_MODULES(GTKGLEXT, gtkglext-1.0 >= 1.0.0,
    [AC_DEFINE([HAVE_GTKGLEXT], 1, [Define if you have gtkglext])],
    [AC_MSG_WARN([Cannot find gtkglext, OpenGL rendering will be disabled])])
  ```

  Replace with:
  ```bash
  # GTK3 has built-in OpenGL support via GtkGLArea
  # No external GtkGLExt dependency needed

  # Still need OpenGL libraries for GL functions
  AC_CHECK_LIB([GL], [glBegin], [HAVE_GL=yes], [HAVE_GL=no])
  if test "x$HAVE_GL" = "xyes"; then
    AC_DEFINE([HAVE_OPENGL], 1, [Define if you have OpenGL])
    GL_LIBS="-lGL -lGLU"
    AC_SUBST(GL_LIBS)
  else
    AC_MSG_WARN([OpenGL not found, OpenGL rendering will be disabled])
  fi
  ```

- [ ] **Add GTK3 deprecation guards (optional but recommended)**

  Add after the GTK_VERSION line:
  ```bash
  # Warn about deprecated GTK3 APIs
  GTK_CFLAGS="$GTK_CFLAGS -DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_3_22"
  GTK_CFLAGS="$GTK_CFLAGS -DGDK_VERSION_MAX_ALLOWED=GDK_VERSION_3_24"
  ```

- [ ] **Save and validate configure.ac**
  ```bash
  # Check for syntax errors
  autoconf --trace=AC_MSG_ERROR configure.ac
  ```

#### Task 1.3: Update Makefiles (1.5 hours)

- [ ] **Update src/hid/gtk/Makefile.am**

  Open `src/hid/gtk/Makefile.am`

  Find the section with GTK flags (look for `AM_CPPFLAGS` or `INCLUDES`):
  ```makefile
  # OLD:
  AM_CPPFLAGS = ... $(GTK_CFLAGS) $(GTKGLEXT_CFLAGS) ...
  ```

  Replace with:
  ```makefile
  # NEW:
  AM_CPPFLAGS = ... $(GTK_CFLAGS) $(GL_CFLAGS) ...
  ```

  Find the section with libraries (look for `LIBS` or `LDADD`):
  ```makefile
  # OLD:
  ... $(GTK_LIBS) $(GTKGLEXT_LIBS) ...
  ```

  Replace with:
  ```makefile
  # NEW:
  ... $(GTK_LIBS) $(GL_LIBS) ...
  ```

- [ ] **Add deprecation warnings to Makefile.am**

  Add to the AM_CPPFLAGS section:
  ```makefile
  AM_CPPFLAGS += -DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_3_22
  AM_CPPFLAGS += -DGDK_VERSION_MAX_ALLOWED=GDK_VERSION_3_24
  AM_CPPFLAGS += -DGTK_DISABLE_DEPRECATED
  ```

- [ ] **Check other Makefiles**
  ```bash
  # Search for GTK2 references in other Makefiles
  find . -name "Makefile.am" -exec grep -l "gtk+-2.0\|gtkglext" {} \;

  # Update any found files similarly
  ```

#### Task 1.4: Regenerate Build System (1 hour)

- [ ] **Clean old build artifacts**
  ```bash
  make distclean 2>/dev/null || true
  rm -rf autom4te.cache
  rm -f aclocal.m4 configure config.h.in
  find . -name "Makefile.in" -delete
  ```

- [ ] **Regenerate autotools files**
  ```bash
  autoreconf -vif

  # OR use autogen.sh if it exists:
  ./autogen.sh
  ```

  Expected output should include:
  - "libtoolize: putting auxiliary files in `.'"
  - "aclocal: installing"
  - "autoconf: tracing"
  - "automake: installing"

- [ ] **Verify configure script was regenerated**
  ```bash
  ls -la configure
  # Should have recent timestamp

  grep "gtk+-3.0" configure
  # Should find GTK3 references
  ```

#### Task 1.5: Test Build Configuration (1.5 hours)

- [ ] **Run configure with GTK3**
  ```bash
  ./configure \
    --enable-debug \
    2>&1 | tee configure.log
  ```

  Watch for:
  - ✅ "checking for GTK... yes"
  - ✅ "Using GTK+ version 3.x.x"
  - ❌ No errors about missing GTK3
  - ⚠️  Note any warnings

- [ ] **Review configure output**
  ```bash
  # Check GTK detection
  grep -A5 "checking for GTK" configure.log

  # Check OpenGL detection
  grep -A5 "checking for GL" configure.log

  # Check for errors
  grep -i "error" configure.log
  ```

- [ ] **Verify config.h was generated correctly**
  ```bash
  grep "HAVE_OPENGL" config.h
  # Should see: #define HAVE_OPENGL 1 (if OpenGL found)

  grep "GTK" config.h
  ```

- [ ] **Document configuration results**
  ```bash
  # Save successful configuration
  ./configure --help > docs/configure-help.txt

  # Document any issues
  echo "Configuration completed on $(date)" >> MIGRATION_LOG.md
  echo "GTK Version: $(pkg-config --modversion gtk+-3.0)" >> MIGRATION_LOG.md
  ```

#### Task 1.6: Create Development Helper Scripts (1 hour)

- [ ] **Create build helper script**

  Create `scripts/build-gtk3.sh`:
  ```bash
  #!/bin/bash
  # Quick build script for GTK3 development

  set -e  # Exit on error

  echo "=== PCB GTK3 Development Build ==="
  echo "GTK Version: $(pkg-config --modversion gtk+-3.0)"
  echo ""

  # Configure if needed
  if [ ! -f Makefile ]; then
    echo "Running configure..."
    ./configure --enable-debug
  fi

  # Build
  echo "Building..."
  make -j$(nproc)

  echo ""
  echo "✅ Build complete!"
  echo "Run: ./src/pcb"
  ```

  Make executable:
  ```bash
  chmod +x scripts/build-gtk3.sh
  mkdir -p scripts
  ```

- [ ] **Create quick test script**

  Create `scripts/test-gtk3.sh`:
  ```bash
  #!/bin/bash
  # Quick test for GTK3 build

  set -e

  echo "=== Testing GTK3 Build ==="

  # Test 1: Version check
  echo "1. Version check..."
  ./src/pcb --version

  # Test 2: HID list
  echo "2. HID list..."
  ./src/pcb --hid-list

  # Test 3: Load test board (if exists)
  if [ -f tests/inputs/test.pcb ]; then
    echo "3. Loading test board..."
    ./src/pcb tests/inputs/test.pcb --quit
  fi

  echo ""
  echo "✅ Basic tests passed!"
  ```

  Make executable:
  ```bash
  chmod +x scripts/test-gtk3.sh
  ```

- [ ] **Create migration tracking document**

  Create `MIGRATION_LOG.md`:
  ```markdown
  # GTK3 Migration Log

  ## Day 1: Build System Setup

  Date: $(date +%Y-%m-%d)

  ### Completed
  - [ ] GTK3 libraries installed
  - [ ] configure.ac updated
  - [ ] Makefile.am updated
  - [ ] Build system regenerated
  - [ ] Configuration successful

  ### Issues Encountered

  (Document any problems here)

  ### Notes

  (Any important observations)
  ```

### End of Day 1 Checklist

- [ ] **Build system configured for GTK3**
- [ ] **No errors in configure output**
- [ ] **Helper scripts created**
- [ ] **Development environment documented**
- [ ] **Can run `./configure` successfully**

**Expected State:** You should be able to run `./configure` successfully with GTK3 detected. The build will fail because source code still has GTK2 references, but the build system is ready.

**Next:** Day 2 will update the source code to use GTK3 APIs.

---

## Day 2: Core Initialization Migration

**Goal:** Migrate gtkhid-main.c to GTK3, get application to launch

**Time Allocation:** 8 hours

### What You'll Accomplish

By end of Day 2, you will:
- Have gtkhid-main.c fully migrated to GTK3
- Have application compile successfully
- Have application launch (even if window is broken)
- Have GTK main loop working

### Detailed Todo List

#### Task 2.1: Audit gtkhid-main.c (1 hour)

- [ ] **Read and understand gtkhid-main.c**
  ```bash
  wc -l src/hid/gtk/gtkhid-main.c
  # Note line count

  # Open in editor
  code src/hid/gtk/gtkhid-main.c  # or vim, emacs, etc.
  ```

- [ ] **Identify all GTK2-specific code**

  Search for these patterns:
  ```bash
  cd src/hid/gtk

  # Find deprecated APIs
  grep -n "gtk_hbox_new\|gtk_vbox_new\|gtk_table_new" gtkhid-main.c
  grep -n "GtkTooltips\|gtk_tooltips" gtkhid-main.c
  grep -n "gdk_threads_" gtkhid-main.c
  grep -n "->window\|->allocation" gtkhid-main.c
  grep -n "GdkGC\|gdk_gc_" gtkhid-main.c
  grep -n "expose-event" gtkhid-main.c
  ```

- [ ] **Create migration checklist for this file**

  Create `src/hid/gtk/MIGRATION_gtkhid-main.md`:
  ```markdown
  # gtkhid-main.c Migration Checklist

  ## Deprecated APIs Found
  - [ ] Line XXX: gtk_xxx_xxx -> Needs: gtk_yyy_yyy
  - [ ] Line XXX: widget->member -> Needs: gtk_widget_get_member()

  ## Functions to Update
  - [ ] ghid_init() - GTK initialization
  - [ ] ghid_do_export() - Main entry
  - [ ] (list all functions)

  ## Testing Plan
  - [ ] Application launches
  - [ ] No GTK warnings
  - [ ] Main loop runs
  ```

- [ ] **Document current behavior**
  ```bash
  # Run current GTK2 version and document
  ./src/pcb-gtk2-backup --version > docs/gtk2-version.txt
  ./src/pcb-gtk2-backup --help > docs/gtk2-help.txt
  ```

#### Task 2.2: Update Includes and Declarations (30 minutes)

- [ ] **Update header includes**

  In `src/hid/gtk/gtkhid-main.c`, find:
  ```c
  #include <gtk/gtk.h>
  ```

  Verify it's the only GTK include needed (no `gtk/gtkgl.h` etc.)

- [ ] **Remove GtkGLExt includes**

  Remove any lines like:
  ```c
  #include <gtk/gtkgl.h>
  #include <gdk/gdkgl.h>
  ```

- [ ] **Add GTK3 version check**

  Add after includes:
  ```c
  /* Verify GTK3 version */
  #if !GTK_CHECK_VERSION(3, 22, 0)
  #error "GTK 3.22 or later required"
  #endif
  ```

- [ ] **Update conditional compilation**

  Find any `#ifdef HAVE_GTKGLEXT` blocks:
  ```c
  #ifdef HAVE_GTKGLEXT
    // OpenGL code
  #endif
  ```

  Change to:
  ```c
  #ifdef HAVE_OPENGL
    // OpenGL code (using GtkGLArea)
  #endif
  ```

#### Task 2.3: Update GTK Initialization (1.5 hours)

- [ ] **Update ghid_init() or ghid_parse_arguments()**

  Find the GTK initialization code (look for `gtk_init`):

  **OLD GTK2 code:**
  ```c
  static void
  ghid_init (int *argc, char ***argv)
  {
    /* Initialize threads */
    gdk_threads_init();
    gdk_threads_enter();

    /* Initialize GTK */
    gtk_init(argc, argv);

    /* ... more initialization ... */
  }
  ```

  **NEW GTK3 code:**
  ```c
  static void
  ghid_init (int *argc, char ***argv)
  {
    /* GTK3 is thread-safe by default, no gdk_threads_init needed */

    /* Initialize GTK */
    gtk_init(argc, argv);

    /* ... more initialization ... */
  }
  ```

- [ ] **Remove all gdk_threads_* calls**

  Search and remove:
  ```c
  gdk_threads_init();
  gdk_threads_enter();
  gdk_threads_leave();
  GDK_THREADS_ENTER();
  GDK_THREADS_LEAVE();
  ```

  **Rationale:** GTK3's default main context is thread-safe. The `gdk_threads_*` functions are deprecated and should not be used.

- [ ] **Update tooltip initialization**

  **OLD GTK2 code:**
  ```c
  GtkTooltips *tooltips;

  tooltips = gtk_tooltips_new();
  gtk_tooltips_set_tip(tooltips, widget, "Tip text", NULL);
  ```

  **NEW GTK3 code:**
  ```c
  /* No GtkTooltips object needed in GTK3 */

  gtk_widget_set_tooltip_text(widget, "Tip text");
  /* or for markup: */
  gtk_widget_set_tooltip_markup(widget, "<b>Bold</b> tip");
  ```

  Find all `GtkTooltips` usage and replace:
  ```bash
  grep -n "GtkTooltips\|gtk_tooltips" gtkhid-main.c
  ```

- [ ] **Update RC file handling (theme/style)**

  **OLD GTK2 code:**
  ```c
  gtk_rc_parse("theme.rc");
  gtk_rc_parse_string("style 'custom' { ... }");
  ```

  **NEW GTK3 code:**
  ```c
  /* GTK3 uses CSS instead of RC files */
  GtkCssProvider *provider;
  GdkDisplay *display;
  GdkScreen *screen;

  provider = gtk_css_provider_new();
  display = gdk_display_get_default();
  screen = gdk_display_get_default_screen(display);

  /* Load CSS file */
  gtk_css_provider_load_from_path(provider, "theme.css", NULL);

  /* Or load CSS string */
  const gchar *css_data =
    ".custom-widget {"
    "  background-color: #ffffff;"
    "  color: #000000;"
    "}";
  gtk_css_provider_load_from_data(provider, css_data, -1, NULL);

  gtk_style_context_add_provider_for_screen(screen,
    GTK_STYLE_PROVIDER(provider),
    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  g_object_unref(provider);
  ```

  **Note:** This is optional - can be done later. For now, just comment out RC file code.

#### Task 2.4: Update Main Loop and Event Handling (1 hour)

- [ ] **Find main loop code**

  Look for:
  ```c
  gtk_main();
  ```

  This stays the same in GTK3! No changes needed.

- [ ] **Update idle/timeout callbacks if needed**

  GTK3 timeout/idle functions are the same:
  ```c
  g_timeout_add(100, callback, data);    // Same
  g_idle_add(callback, data);            // Same
  ```

  No changes needed unless using deprecated variants.

- [ ] **Check main loop exit handling**

  ```c
  gtk_main_quit();  // Same in GTK3
  ```

  No changes needed.

#### Task 2.5: Update Widget Access Patterns (2 hours)

This is the most tedious but important part - replacing direct struct member access with accessor functions.

- [ ] **Replace widget->window**

  **Find all occurrences:**
  ```bash
  grep -n "->window" gtkhid-main.c
  ```

  **OLD:**
  ```c
  GdkWindow *window = widget->window;
  if (widget->window)
    gdk_window_invalidate_rect(widget->window, NULL, FALSE);
  ```

  **NEW:**
  ```c
  GdkWindow *window = gtk_widget_get_window(widget);
  if (gtk_widget_get_window(widget))
    gdk_window_invalidate_rect(gtk_widget_get_window(widget), NULL, FALSE);
  ```

- [ ] **Replace widget->allocation**

  **Find all occurrences:**
  ```bash
  grep -n "->allocation" gtkhid-main.c
  ```

  **OLD:**
  ```c
  int width = widget->allocation.width;
  int height = widget->allocation.height;
  ```

  **NEW:**
  ```c
  GtkAllocation allocation;
  gtk_widget_get_allocation(widget, &allocation);
  int width = allocation.width;
  int height = allocation.height;
  ```

- [ ] **Replace GTK_WIDGET_* macros**

  **OLD:**
  ```c
  if (GTK_WIDGET_VISIBLE(widget))
  if (GTK_WIDGET_REALIZED(widget))
  if (GTK_WIDGET_MAPPED(widget))
  GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_FOCUS);
  ```

  **NEW:**
  ```c
  if (gtk_widget_get_visible(widget))
  if (gtk_widget_get_realized(widget))
  if (gtk_widget_get_mapped(widget))
  gtk_widget_set_can_focus(widget, TRUE);
  ```

- [ ] **Create helper script for bulk replacement**

  Create `scripts/replace-widget-access.sh`:
  ```bash
  #!/bin/bash
  # Helper script to replace common widget access patterns

  FILE=$1

  if [ -z "$FILE" ]; then
    echo "Usage: $0 <file>"
    exit 1
  fi

  # Backup
  cp "$FILE" "$FILE.bak"

  # Replace GTK_WIDGET_VISIBLE
  sed -i 's/GTK_WIDGET_VISIBLE(\([^)]*\))/gtk_widget_get_visible(\1)/g' "$FILE"

  # Replace GTK_WIDGET_REALIZED
  sed -i 's/GTK_WIDGET_REALIZED(\([^)]*\))/gtk_widget_get_realized(\1)/g' "$FILE"

  # Note: widget->window and widget->allocation need manual review

  echo "Replacements done. Review changes in: $FILE"
  echo "Backup saved as: $FILE.bak"
  ```

  **Use with caution** - verify all changes!

#### Task 2.6: Update Signal Connections (1 hour)

- [ ] **Check signal names**

  Most signal names are unchanged in GTK3:
  ```c
  g_signal_connect(widget, "clicked", G_CALLBACK(callback), data);
  // Same in GTK3
  ```

  But check for deprecated signals:
  ```bash
  grep -n "expose-event" gtkhid-main.c
  ```

  **If found in gtkhid-main.c:**
  ```c
  // OLD:
  g_signal_connect(widget, "expose-event", G_CALLBACK(expose_cb), data);

  // NEW:
  g_signal_connect(widget, "draw", G_CALLBACK(draw_cb), data);
  ```

  **Update callback signature:**
  ```c
  // OLD:
  static gboolean
  expose_cb(GtkWidget *widget, GdkEventExpose *event, gpointer data)
  {
    // ...
  }

  // NEW:
  static gboolean
  draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data)
  {
    // Drawing code goes here (will be done in Week 2)
    // For now, just return FALSE
    return FALSE;
  }
  ```

- [ ] **Update key press event handling**

  Check for `GDK_` key constants:
  ```bash
  grep -n "GDK_[A-Z]" gtkhid-main.c | grep -v "GDK_KEY_"
  ```

  **OLD:**
  ```c
  if (event->keyval == GDK_Escape)
  if (event->keyval == GDK_Return)
  ```

  **NEW:**
  ```c
  if (event->keyval == GDK_KEY_Escape)
  if (event->keyval == GDK_KEY_Return)
  ```

  Note the `GDK_KEY_` prefix in GTK3.

#### Task 2.7: Compile and Test (1.5 hours)

- [ ] **Attempt first compilation**
  ```bash
  cd src/hid/gtk
  make gtkhid-main.o 2>&1 | tee compile-errors.log
  ```

  Expect errors! This is normal.

- [ ] **Fix compilation errors iteratively**

  Common errors and fixes:

  **Error: `'GtkWidget' has no member named 'window'`**
  ```
  Fix: Use gtk_widget_get_window(widget)
  ```

  **Error: `'GtkWidget' has no member named 'allocation'`**
  ```
  Fix: Use gtk_widget_get_allocation(widget, &allocation)
  ```

  **Error: `'GDK_Escape' undeclared`**
  ```
  Fix: Change to GDK_KEY_Escape
  ```

  **Error: `gdk_threads_init() is deprecated`**
  ```
  Fix: Remove the call (not needed in GTK3)
  ```

  **Error: `GtkTooltips` undeclared`**
  ```
  Fix: Replace with gtk_widget_set_tooltip_text()
  ```

- [ ] **Fix all errors in gtkhid-main.c**

  Work through errors one by one:
  ```bash
  # Compile
  make gtkhid-main.o

  # Fix first error
  # ... edit file ...

  # Compile again
  make gtkhid-main.o

  # Repeat until clean
  ```

- [ ] **Verify clean compilation**
  ```bash
  make gtkhid-main.o
  # Should complete with no errors

  # Check for warnings
  gcc -Wall -Wextra -c gtkhid-main.c $(pkg-config --cflags gtk+-3.0)
  ```

- [ ] **Document changes made**

  Update `MIGRATION_LOG.md`:
  ```markdown
  ## Day 2: gtkhid-main.c Migration

  ### Changes Made
  - Removed gdk_threads_* calls
  - Replaced GtkTooltips with gtk_widget_set_tooltip_text()
  - Updated widget->window to gtk_widget_get_window()
  - Updated widget->allocation to gtk_widget_get_allocation()
  - Updated GDK_* constants to GDK_KEY_* for keyboard events

  ### Compilation Status
  - ✅ gtkhid-main.o compiles cleanly
  - Warnings: (list any warnings)

  ### Known Issues
  - (list any deferred issues)
  ```

#### Task 2.8: Initial Integration Test (30 minutes)

Note: Full application won't work yet (other files not migrated), but we can test what we can.

- [ ] **Attempt full build**
  ```bash
  cd /home/user/pcb
  make 2>&1 | tee build-errors.log
  ```

  **Expected:** Errors in other GTK files (gui-top-window.c, etc.)
  **Success:** gtkhid-main.o should compile without errors

- [ ] **Count remaining errors**
  ```bash
  grep "error:" build-errors.log | wc -l
  # Note the number for tracking
  ```

- [ ] **Commit progress**
  ```bash
  git add src/hid/gtk/gtkhid-main.c
  git add configure.ac src/hid/gtk/Makefile.am
  git commit -m "WIP: Migrate gtkhid-main.c to GTK3

  - Remove gdk_threads_* (deprecated in GTK3)
  - Replace GtkTooltips with gtk_widget_set_tooltip_text
  - Update widget direct access to accessor functions
  - Update GDK key constants to GDK_KEY_* format

  gtkhid-main.c now compiles cleanly with GTK3.
  Other GTK files still need migration."
  ```

### End of Day 2 Checklist

- [ ] **gtkhid-main.c compiles without errors**
- [ ] **All direct widget access replaced with accessors**
- [ ] **All deprecated APIs removed or updated**
- [ ] **Changes documented in MIGRATION_LOG.md**
- [ ] **Work committed to git**

**Expected State:** gtkhid-main.c should compile cleanly. Other GTK files will still have errors, but the core initialization is GTK3-ready.

**Next:** Day 3 will migrate gui-top-window.c layout code.

---

## Day 3: Main Window Layout - Part 1

**Goal:** Convert main window layout containers from GTK2 to GTK3

**Time Allocation:** 8 hours

### What You'll Accomplish

By end of Day 3, you will:
- Have gui-top-window.c layout code migrated to GTK3
- Have main window structure converted (GtkTable → GtkGrid, boxes)
- Have menu bar and toolbar layout working
- Be able to see main window structure (even if empty)

### Detailed Todo List

#### Task 3.1: Analyze gui-top-window.c Structure (1 hour)

- [ ] **Read and understand gui-top-window.c**
  ```bash
  wc -l src/hid/gtk/gui-top-window.c
  # Around 1200 lines

  # Open in editor
  code src/hid/gtk/gui-top-window.c
  ```

- [ ] **Identify main layout structure**

  Look for the main window creation function (usually `ghid_build_pcb_top_window` or similar):
  ```bash
  grep -n "static.*window\|create.*window\|build.*window" gui-top-window.c
  ```

- [ ] **Map out layout hierarchy**

  Create `src/hid/gtk/gui-top-window-layout.txt`:
  ```
  Main Window (GtkWindow)
  └─ Main VBox (GtkVBox) → Convert to GtkBox(VERTICAL)
      ├─ Menu Bar (GtkMenuBar) → No change
      ├─ Toolbar (GtkToolbar or GtkHBox) → Check type
      ├─ Main Area (GtkHPaned or GtkTable?) → Identify
      │   ├─ Left Panel
      │   └─ Drawing Area
      └─ Status Bar (GtkHBox) → Convert to GtkBox(HORIZONTAL)
  ```

- [ ] **Find all GtkTable usage**
  ```bash
  grep -n "gtk_table_new\|GtkTable\|gtk_table_attach" gui-top-window.c > tables.txt
  cat tables.txt
  # Note each line number and context
  ```

- [ ] **Find all GtkHBox/GtkVBox usage**
  ```bash
  grep -n "gtk_hbox_new\|gtk_vbox_new\|GtkHBox\|GtkVBox" gui-top-window.c > boxes.txt
  cat boxes.txt
  ```

- [ ] **Document widgets that need conversion**

  Create checklist in `src/hid/gtk/MIGRATION_gui-top-window.md`:
  ```markdown
  # gui-top-window.c Migration Checklist

  ## Containers to Convert

  ### Tables (GtkTable → GtkGrid)
  - [ ] Line XXX: Main layout table
  - [ ] Line YYY: Status bar table
  - [ ] (list all)

  ### Boxes (GtkHBox/VBox → GtkBox)
  - [ ] Line XXX: Main vbox
  - [ ] Line YYY: Toolbar hbox
  - [ ] (list all)

  ## Widget Access to Fix
  - [ ] widget->window occurrences
  - [ ] widget->allocation occurrences

  ## Signals to Check
  - [ ] expose-event → draw
  ```

#### Task 3.2: Convert Main VBox/HBox Containers (1.5 hours)

- [ ] **Find main VBox creation**

  Look for pattern like:
  ```c
  main_vbox = gtk_vbox_new(FALSE, 0);
  ```

  **Convert to:**
  ```c
  main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  ```

- [ ] **Update all gtk_vbox_new() calls**

  Search and replace pattern:
  ```bash
  # Find all vbox_new
  grep -n "gtk_vbox_new" gui-top-window.c
  ```

  For each occurrence:
  ```c
  // OLD:
  widget = gtk_vbox_new(homogeneous, spacing);

  // NEW:
  widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, spacing);
  if (homogeneous)
    gtk_box_set_homogeneous(GTK_BOX(widget), TRUE);
  ```

- [ ] **Update all gtk_hbox_new() calls**

  ```bash
  grep -n "gtk_hbox_new" gui-top-window.c
  ```

  For each occurrence:
  ```c
  // OLD:
  widget = gtk_hbox_new(homogeneous, spacing);

  // NEW:
  widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing);
  if (homogeneous)
    gtk_box_set_homogeneous(GTK_BOX(widget), TRUE);
  ```

- [ ] **Update box packing calls**

  Good news: `gtk_box_pack_start()` and `gtk_box_pack_end()` work the same in GTK3!

  ```c
  gtk_box_pack_start(GTK_BOX(box), widget, expand, fill, padding);
  // Same in GTK3 - no changes needed
  ```

  But you can modernize if desired:
  ```c
  // Modern GTK3 approach:
  gtk_container_add(GTK_CONTAINER(box), widget);
  gtk_widget_set_hexpand(widget, TRUE);  // if expand
  gtk_widget_set_halign(widget, GTK_ALIGN_FILL);  // if fill
  ```

  **Recommendation:** Keep `gtk_box_pack_start()` for now (less risky), modernize later.

- [ ] **Verify all VBox/HBox conversions**
  ```bash
  # Should return no results:
  grep "gtk_vbox_new\|gtk_hbox_new" gui-top-window.c

  # Should find new code:
  grep "gtk_box_new" gui-top-window.c
  ```

#### Task 3.3: Convert GtkTable to GtkGrid (2.5 hours)

This is the most complex conversion. Take it slow and methodical.

- [ ] **Identify first GtkTable to convert**

  Find simplest table first (fewer attachments):
  ```bash
  grep -B5 -A20 "gtk_table_new" gui-top-window.c | head -30
  ```

- [ ] **Create helper function for table conversion**

  Add this utility function to gui-top-window.c (near top):
  ```c
  /* Helper function for GTK2-style table attachments in GTK3 */
  static void
  ghid_grid_attach_compat(GtkGrid *grid, GtkWidget *widget,
                          guint left, guint right, guint top, guint bottom,
                          GtkAttachOptions xoptions, GtkAttachOptions yoptions,
                          guint xpad, guint ypad)
  {
    /* Attach widget to grid */
    gtk_grid_attach(grid, widget, left, top, right - left, bottom - top);

    /* Handle expand/fill options */
    if (xoptions & GTK_EXPAND) {
      gtk_widget_set_hexpand(widget, TRUE);
      if (xoptions & GTK_FILL)
        gtk_widget_set_halign(widget, GTK_ALIGN_FILL);
      else
        gtk_widget_set_halign(widget, GTK_ALIGN_CENTER);
    } else {
      gtk_widget_set_hexpand(widget, FALSE);
      gtk_widget_set_halign(widget, GTK_ALIGN_START);
    }

    if (yoptions & GTK_EXPAND) {
      gtk_widget_set_vexpand(widget, TRUE);
      if (yoptions & GTK_FILL)
        gtk_widget_set_valign(widget, GTK_ALIGN_FILL);
      else
        gtk_widget_set_valign(widget, GTK_ALIGN_CENTER);
    } else {
      gtk_widget_set_vexpand(widget, FALSE);
      gtk_widget_set_valign(widget, GTK_ALIGN_START);
    }

    /* Handle padding */
    if (xpad > 0) {
      gtk_widget_set_margin_start(widget, xpad);
      gtk_widget_set_margin_end(widget, xpad);
    }
    if (ypad > 0) {
      gtk_widget_set_margin_top(widget, ypad);
      gtk_widget_set_margin_bottom(widget, ypad);
    }
  }
  ```

- [ ] **Convert each GtkTable systematically**

  For each table in the file:

  **Step 1: Find the table creation**
  ```c
  // OLD:
  table = gtk_table_new(rows, cols, homogeneous);
  ```

  **Step 2: Replace with grid**
  ```c
  // NEW:
  table = gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(table), homogeneous);
  gtk_grid_set_column_homogeneous(GTK_GRID(table), homogeneous);
  ```

  **Step 3: Find all gtk_table_attach calls for this table**
  ```c
  // OLD:
  gtk_table_attach(GTK_TABLE(table), widget,
                   left, right, top, bottom,
                   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
                   2, 2);
  ```

  **Step 4: Replace with grid attach using helper**
  ```c
  // NEW (using helper):
  ghid_grid_attach_compat(GTK_GRID(table), widget,
                          left, right, top, bottom,
                          GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
                          2, 2);

  // Or use direct GTK3 approach:
  gtk_grid_attach(GTK_GRID(table), widget, left, top,
                  right - left, bottom - top);
  gtk_widget_set_hexpand(widget, TRUE);
  gtk_widget_set_vexpand(widget, TRUE);
  gtk_widget_set_halign(widget, GTK_ALIGN_FILL);
  gtk_widget_set_valign(widget, GTK_ALIGN_FILL);
  gtk_widget_set_margin_start(widget, 2);
  gtk_widget_set_margin_end(widget, 2);
  gtk_widget_set_margin_top(widget, 2);
  gtk_widget_set_margin_bottom(widget, 2);
  ```

  **Step 5: Update spacing if needed**
  ```c
  // OLD:
  gtk_table_set_row_spacings(GTK_TABLE(table), 4);
  gtk_table_set_col_spacings(GTK_TABLE(table), 4);

  // NEW:
  gtk_grid_set_row_spacing(GTK_GRID(table), 4);
  gtk_grid_set_column_spacing(GTK_GRID(table), 4);
  ```

- [ ] **Test each table conversion individually**

  After converting each table:
  ```bash
  # Try to compile
  cd src/hid/gtk
  gcc -c gui-top-window.c $(pkg-config --cflags gtk+-3.0) 2>&1 | grep error
  ```

  Fix any errors before moving to next table.

- [ ] **Document table conversions**

  Update `MIGRATION_gui-top-window.md`:
  ```markdown
  ### Tables Converted
  - [x] Line 234: Main layout table (3x3) - Converted to GtkGrid
  - [x] Line 456: Status bar table (2x4) - Converted to GtkGrid
  - [ ] Line XXX: (next table)
  ```

#### Task 3.4: Update Widget Access Patterns (1 hour)

Same as Day 2, but for gui-top-window.c:

- [ ] **Replace widget->window**
  ```bash
  grep -n "->window" gui-top-window.c
  ```

  Replace each occurrence:
  ```c
  // OLD:
  if (widget->window)

  // NEW:
  if (gtk_widget_get_window(widget))
  ```

- [ ] **Replace widget->allocation**
  ```bash
  grep -n "->allocation" gui-top-window.c
  ```

  Replace each:
  ```c
  // OLD:
  width = widget->allocation.width;

  // NEW:
  GtkAllocation allocation;
  gtk_widget_get_allocation(widget, &allocation);
  width = allocation.width;
  ```

- [ ] **Replace GTK_WIDGET_* macros**
  ```bash
  grep -n "GTK_WIDGET_VISIBLE\|GTK_WIDGET_REALIZED" gui-top-window.c
  ```

  Replace:
  ```c
  // OLD:
  if (GTK_WIDGET_VISIBLE(widget))

  // NEW:
  if (gtk_widget_get_visible(widget))
  ```

#### Task 3.5: Update Deprecated Widgets (1 hour)

- [ ] **Find deprecated widget usage**
  ```bash
  grep -n "gtk_combo_new\|GtkCombo[^B]" gui-top-window.c
  grep -n "gtk_option_menu\|GtkOptionMenu" gui-top-window.c
  ```

- [ ] **Replace GtkCombo (if found)**
  ```c
  // OLD:
  combo = gtk_combo_new();

  // NEW:
  combo = gtk_combo_box_text_new();
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Item");
  ```

- [ ] **Replace GtkOptionMenu (if found)**
  ```c
  // OLD:
  option_menu = gtk_option_menu_new();

  // NEW:
  option_menu = gtk_combo_box_text_new();
  ```

#### Task 3.6: Compile and Test (1 hour)

- [ ] **Compile gui-top-window.c**
  ```bash
  cd src/hid/gtk
  make gui-top-window.o 2>&1 | tee compile.log
  ```

- [ ] **Fix all compilation errors**

  Work through errors systematically:
  ```bash
  # Show first 5 errors
  grep "error:" compile.log | head -5

  # Fix them, then recompile
  make gui-top-window.o
  ```

- [ ] **Verify clean compilation**
  ```bash
  make gui-top-window.o
  echo "Exit code: $?"
  # Should be 0
  ```

- [ ] **Check warnings**
  ```bash
  make gui-top-window.o 2>&1 | grep warning
  # Document any warnings for later
  ```

- [ ] **Commit progress**
  ```bash
  git add src/hid/gtk/gui-top-window.c
  git commit -m "WIP: Migrate gui-top-window.c layout to GTK3

  - Convert GtkTable to GtkGrid with helper function
  - Convert GtkHBox/VBox to GtkBox with orientation
  - Update widget direct access to accessor functions
  - Replace deprecated widgets (GtkCombo, etc.)

  gui-top-window.c now compiles with GTK3.
  Layout structure migrated, drawing code still needs work."
  ```

### End of Day 3 Checklist

- [ ] **All GtkTable converted to GtkGrid**
- [ ] **All GtkHBox/VBox converted to GtkBox**
- [ ] **gui-top-window.c compiles without errors**
- [ ] **Layout structure documented**
- [ ] **Changes committed to git**

**Expected State:** gui-top-window.c should compile. The main window layout structure is converted to GTK3. Drawing code (if any in this file) may still need work.

**Next:** Day 4 will complete gui-top-window.c and handle any remaining files needed for window creation.

---

## Day 4: Main Window Layout - Part 2

**Goal:** Complete gui-top-window.c migration, handle signals and final integration

**Time Allocation:** 8 hours

### What You'll Accomplish

By end of Day 4, you will:
- Have gui-top-window.c fully migrated
- Have all window signals updated
- Have menu bar and toolbar functional
- Be able to launch application with visible main window

### Detailed Todo List

#### Task 4.1: Update Signal Handlers (2 hours)

- [ ] **Find all signal connections in gui-top-window.c**
  ```bash
  grep -n "g_signal_connect" gui-top-window.c > signals.txt
  cat signals.txt
  ```

- [ ] **Check for expose-event signals**
  ```bash
  grep -n "expose-event" gui-top-window.c
  ```

  If found, update to "draw" signal:
  ```c
  // OLD:
  g_signal_connect(widget, "expose-event",
                   G_CALLBACK(expose_callback), data);

  static gboolean
  expose_callback(GtkWidget *widget, GdkEventExpose *event, gpointer data)
  {
    // GDK drawing code
    return FALSE;
  }

  // NEW:
  g_signal_connect(widget, "draw",
                   G_CALLBACK(draw_callback), data);

  static gboolean
  draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data)
  {
    // Cairo drawing code (stub for now)
    return FALSE;
  }
  ```

- [ ] **Update configure-event if needed**

  configure-event is still valid in GTK3, but check usage:
  ```bash
  grep -n "configure-event" gui-top-window.c
  ```

  If it's used for size handling, consider using "size-allocate" signal instead (more GTK3-idiomatic).

- [ ] **Update delete-event and destroy signals**

  These are the same in GTK3:
  ```c
  g_signal_connect(window, "delete-event",
                   G_CALLBACK(delete_callback), NULL);
  g_signal_connect(window, "destroy",
                   G_CALLBACK(destroy_callback), NULL);
  // No changes needed
  ```

- [ ] **Update all callback signatures**

  Make sure callback functions match new signal signatures:
  - expose-event → draw: Change `GdkEventExpose *event` to `cairo_t *cr`
  - No changes needed for most other signals

#### Task 4.2: Handle Drawing Code (Stub Implementation) (1.5 hours)

Note: Full drawing migration happens in Week 2. For now, just stub it out so the window displays.

- [ ] **Find drawing code in gui-top-window.c**
  ```bash
  grep -n "gdk_draw_\|GdkGC\|gdk_gc_" gui-top-window.c
  ```

- [ ] **Create stub draw callbacks**

  For any GDK drawing code found:
  ```c
  static gboolean
  some_draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data)
  {
    /* TODO: Migrate drawing code from GDK to Cairo in Week 2 */

    /* For now, just draw a placeholder */
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);

    /* Fill with background color */
    cairo_set_source_rgb(cr, 0.95, 0.95, 0.95);
    cairo_rectangle(cr, 0, 0, allocation.width, allocation.height);
    cairo_fill(cr);

    /* Draw "TODO" text */
    cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12.0);
    cairo_move_to(cr, 10, 20);
    cairo_show_text(cr, "Drawing area (to be implemented)");

    return FALSE;
  }
  ```

- [ ] **Comment out complex GDK code temporarily**

  If there's extensive GDK drawing code:
  ```c
  #if 0  /* TODO: Migrate to Cairo in Week 2 */
    /* Old GDK drawing code here */
    GdkGC *gc = gdk_gc_new(widget->window);
    gdk_draw_line(...);
    g_object_unref(gc);
  #endif
  ```

#### Task 4.3: Update Menu and Toolbar (1.5 hours)

- [ ] **Check menu bar code**

  Menu bars should work mostly unchanged:
  ```c
  menubar = gtk_menu_bar_new();  // Same in GTK3
  menu = gtk_menu_new();         // Same
  menuitem = gtk_menu_item_new_with_label("File");  // Same
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), menu);  // Same
  ```

  But check for deprecated functions:
  ```bash
  grep -n "gtk_image_menu_item_new" gui-top-window.c
  ```

  If found:
  ```c
  // OLD GTK2 (deprecated in GTK3.10):
  menuitem = gtk_image_menu_item_new_with_label("Open");
  image = gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);

  // NEW GTK3 (for now, use simple version):
  menuitem = gtk_menu_item_new_with_label("Open");

  // Or with icon (GTK3.10+):
  menuitem = gtk_menu_item_new();
  box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  image = gtk_image_new_from_icon_name("document-open",
                                        GTK_ICON_SIZE_MENU);
  label = gtk_label_new("Open");
  gtk_container_add(GTK_CONTAINER(box), image);
  gtk_container_add(GTK_CONTAINER(box), label);
  gtk_container_add(GTK_CONTAINER(menuitem), box);
  gtk_widget_show_all(box);
  ```

  **Recommendation:** For Milestone 1, just use simple text menu items. Add icons later.

- [ ] **Check toolbar code**

  Toolbars should work mostly unchanged:
  ```c
  toolbar = gtk_toolbar_new();  // Same

  // OLD:
  button = gtk_toolbar_insert_stock(toolbar, GTK_STOCK_NEW,
                                    "New", "Private", NULL, 0);

  // NEW (stock items deprecated):
  button = gtk_tool_button_new(NULL, "New");
  gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(button), "document-new");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), button, 0);
  ```

- [ ] **Update stock items if used**
  ```bash
  grep -n "GTK_STOCK_" gui-top-window.c
  ```

  Stock items are deprecated in GTK3. Replace with icon names:
  ```c
  // Stock → Icon Name mapping:
  GTK_STOCK_NEW       → "document-new"
  GTK_STOCK_OPEN      → "document-open"
  GTK_STOCK_SAVE      → "document-save"
  GTK_STOCK_QUIT      → "application-exit"
  GTK_STOCK_CUT       → "edit-cut"
  GTK_STOCK_COPY      → "edit-copy"
  GTK_STOCK_PASTE     → "edit-paste"
  // etc.
  ```

  **Note:** Stock items still work in GTK 3.10-3.24, just deprecated. Can update later if time is short.

#### Task 4.4: Update Status Bar (1 hour)

- [ ] **Find status bar code**
  ```bash
  grep -n "statusbar\|status_bar" gui-top-window.c
  ```

- [ ] **Check GtkStatusbar usage**

  GtkStatusbar works the same in GTK3:
  ```c
  statusbar = gtk_statusbar_new();  // Same
  context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar),
                                             "Main");  // Same
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), context_id, "Ready");  // Same
  ```

- [ ] **If custom status bar, check layout**

  If status bar is custom (using HBox, labels):
  ```c
  // Just verify layout was converted in Day 3:
  // GtkHBox → GtkBox(HORIZONTAL)
  ```

#### Task 4.5: Handle Remaining Files (2 hours)

There may be other files needed for the window to launch. Check what's required:

- [ ] **Attempt full build**
  ```bash
  cd /home/user/pcb
  make 2>&1 | tee build-day4.log
  ```

- [ ] **Identify critical files with errors**
  ```bash
  grep "error:" build-day4.log | awk -F: '{print $1}' | sort -u
  ```

  Look for files in `src/hid/gtk/` that are blocking the build.

- [ ] **Quick fixes for critical files**

  For any file blocking compilation, apply quick GTK3 fixes:

  **Priority:** Files needed for window creation:
  - gui.h (header file)
  - ghid-main-menu.c (if menu crashes)
  - gui-misc.c (if utility functions crash)

  For each file:
  ```bash
  # Apply basic conversions
  # 1. widget->window → gtk_widget_get_window(widget)
  # 2. widget->allocation → gtk_widget_get_allocation(widget, &allocation)
  # 3. GDK_Key → GDK_KEY_Key
  # 4. Comment out complex drawing code
  ```

- [ ] **Update gui.h if needed**

  Check `src/hid/gtk/gui.h` for:
  ```bash
  grep -n "GtkTable\|GtkHBox\|GtkVBox" gui.h
  ```

  Update any type declarations:
  ```c
  // In struct definitions:
  // OLD:
  struct {
    GtkWidget *main_vbox;  // Still valid (GtkWidget is generic)
    GtkTable *table;       // Change to GtkGrid
  };

  // NEW:
  struct {
    GtkWidget *main_vbox;
    GtkGrid *table;        // Or just use GtkWidget
  };
  ```

  **OR** keep as `GtkWidget *` (more flexible):
  ```c
  struct {
    GtkWidget *main_vbox;
    GtkWidget *table;      // Works for both GtkTable and GtkGrid
  };
  ```

#### Task 4.6: Compilation and Testing (1 hour)

- [ ] **Build the application**
  ```bash
  cd /home/user/pcb
  make clean
  make 2>&1 | tee build-final.log
  ```

- [ ] **Fix remaining errors**

  Work through any remaining compilation errors:
  ```bash
  grep "error:" build-final.log | head -10
  # Fix top 10 errors
  make 2>&1 | tee build-final2.log
  # Repeat until build succeeds or only non-critical files have errors
  ```

- [ ] **Test application launch**
  ```bash
  cd src
  ./pcb --version
  # Should print version

  ./pcb
  # Should launch GUI (may be broken, but should not crash immediately)
  ```

  **Expected behavior:**
  - Application launches
  - Main window appears (may be empty or broken)
  - No immediate crashes
  - Console may show GTK warnings (document them)

- [ ] **Document launch behavior**

  Update `MIGRATION_LOG.md`:
  ```markdown
  ## Day 4: Application Launch Test

  ### Build Status
  - ✅ Compiles successfully / ⚠️ Compiles with warnings / ❌ Errors in: [files]

  ### Launch Test
  - ✅ Application launches
  - ✅ Main window appears
  - ⚠️ Window contents: [describe what you see]
  - ⚠️ GTK warnings: [list warnings from console]

  ### Known Issues
  - Drawing area is empty/broken (expected - Week 2)
  - (other issues)

  ### Next Steps
  - Fix critical launch issues
  - Day 5: Integration testing
  ```

- [ ] **Take screenshots**
  ```bash
  # Launch app
  ./pcb &

  # Take screenshot (Linux)
  import screenshot-day4.png

  # Or use system screenshot tool
  ```

  Save to `docs/screenshots/milestone1-day4.png`

### End of Day 4 Checklist

- [ ] **Application compiles (even if with some warnings)**
- [ ] **Application launches without crashing**
- [ ] **Main window appears (even if broken/empty)**
- [ ] **All signals updated to GTK3**
- [ ] **Changes committed to git**

**Expected State:** Application should launch and show a window. The window may be empty or broken (drawing area), but it should not crash. Menu and toolbar structure should be visible.

**Next:** Day 5 will test everything, fix critical bugs, and prepare for Week 2.

---

## Day 5: Integration Testing & Bug Fixes

**Goal:** Test Milestone 1 deliverables, fix critical issues, document status

**Time Allocation:** 8 hours

### What You'll Accomplish

By end of Day 5, you will:
- Have tested all Milestone 1 functionality
- Have fixed critical launch issues
- Have documented the state of the migration
- Have a stable foundation for Week 2

### Detailed Todo List

#### Task 5.1: Systematic Testing (2 hours)

- [ ] **Create test checklist**

  Create `tests/milestone1-checklist.md`:
  ```markdown
  # Milestone 1 Test Checklist

  ## Build Tests
  - [ ] Clean build succeeds
  - [ ] No compilation errors
  - [ ] Warnings documented

  ## Launch Tests
  - [ ] Application launches
  - [ ] No immediate crashes
  - [ ] Main window appears
  - [ ] Window is correct size
  - [ ] Window is resizable

  ## Window Structure
  - [ ] Menu bar visible
  - [ ] Menu bar items present
  - [ ] Menus can be opened
  - [ ] Toolbar visible (if applicable)
  - [ ] Status bar visible
  - [ ] Drawing area present (even if empty)

  ## Basic Interaction
  - [ ] Window can be moved
  - [ ] Window can be resized
  - [ ] Window can be closed (without crash)
  - [ ] Application quits cleanly

  ## GTK3 Integration
  - [ ] No GTK-CRITICAL errors
  - [ ] GTK warnings documented
  - [ ] HiDPI scaling works (if on HiDPI display)

  ## Platform Tests (if available)
  - [ ] Linux X11
  - [ ] Linux Wayland (if available)
  - [ ] macOS (if available)
  - [ ] Windows (if available)
  ```

- [ ] **Execute test checklist**

  Go through each item and mark pass/fail:
  ```bash
  # Launch and test
  cd src
  ./pcb 2>&1 | tee ../test-output.log

  # Test each item from checklist
  # Document results
  ```

- [ ] **Document GTK warnings**
  ```bash
  grep "Gtk-WARNING\|Gtk-CRITICAL" test-output.log > gtk-warnings.txt
  cat gtk-warnings.txt
  ```

  Analyze each warning:
  ```
  Common warnings and solutions:

  "Attempting to add a widget with type GtkXXX to a YYY, but as a GtkBin..."
  → Wrong container type, check parent widget

  "Widget not realized"
  → Widget used before gtk_widget_show() or window realization

  "Invalid icon name"
  → Stock icon not found, use different icon name or remove
  ```

- [ ] **Test on different GTK themes**
  ```bash
  # Try different themes to test compatibility
  GTK_THEME=Adwaita ./pcb
  GTK_THEME=Adwaita:dark ./pcb

  # Does it look reasonable?
  # Document any theme-specific issues
  ```

#### Task 5.2: Fix Critical Issues (3 hours)

- [ ] **Prioritize issues**

  Create priority list:
  ```markdown
  # Issue Priority

  ## P0 - Critical (Crashes, won't launch)
  - [ ] Issue: [description]
    Solution: [plan]

  ## P1 - Important (Major functionality broken)
  - [ ] Issue: [description]
    Solution: [plan]

  ## P2 - Medium (Cosmetic, can defer)
  - [ ] Issue: [description]
    Solution: [defer to Week 2]
  ```

- [ ] **Fix P0 (Critical) issues**

  For each critical issue:

  **Example Issue: Crash on startup**
  ```
  Error: Segmentation fault in ghid_main_menu_real_add_menu

  Debugging steps:
  1. Run with gdb:
     gdb ./pcb
     (gdb) run
     (gdb) bt  # Get backtrace

  2. Identify cause (e.g., accessing widget->window before realization)

  3. Fix:
     // OLD:
     gdk_window_invalidate_rect(widget->window, NULL, FALSE);

     // NEW:
     if (gtk_widget_get_realized(widget)) {
       GdkWindow *window = gtk_widget_get_window(widget);
       if (window)
         gdk_window_invalidate_rect(window, NULL, FALSE);
     }

  4. Test fix:
     make && ./pcb

  5. Commit:
     git add ...
     git commit -m "fix: Check widget realization before accessing window"
  ```

- [ ] **Fix P1 (Important) issues**

  For each important issue (non-crash but major functionality):

  **Example: Menu doesn't appear**
  ```
  Issue: Menu bar is invisible

  Debugging:
  1. Check if menu bar widget is created
  2. Check if it's added to parent container
  3. Check if gtk_widget_show() is called

  Common cause: gtk_widget_show_all() not called

  Fix:
  gtk_widget_show_all(main_window);

  Or show each widget explicitly:
  gtk_widget_show(menu_bar);
  ```

- [ ] **Defer P2 (Medium) issues**

  Document medium-priority issues for later:
  ```markdown
  ## Deferred Issues (Week 2+)

  - Drawing area is empty → Drawing migration in Week 2
  - Icons missing from toolbar → Stock icon migration later
  - Status bar text not visible → Styling issue, low priority
  ```

#### Task 5.3: Memory Leak Testing (1 hour)

- [ ] **Install valgrind if not present**
  ```bash
  # Ubuntu/Debian
  sudo apt-get install valgrind

  # Fedora
  sudo dnf install valgrind

  # macOS
  brew install valgrind  # May not be available on newer macOS
  ```

- [ ] **Run basic memory leak test**
  ```bash
  cd src
  valgrind --leak-check=summary --show-leak-kinds=all \
           ./pcb --quit 2>&1 | tee valgrind-quick.log

  # Check summary
  grep "definitely lost" valgrind-quick.log
  grep "possibly lost" valgrind-quick.log
  ```

  **Expected:** Some GTK internal leaks are normal. Look for large leaks or leaks in your code.

- [ ] **Run detailed leak test (if issues found)**
  ```bash
  valgrind --leak-check=full \
           --show-leak-kinds=definite,possible \
           --track-origins=yes \
           ./pcb --quit 2>&1 | tee valgrind-full.log

  # Analyze leaks
  grep -A10 "definitely lost" valgrind-full.log
  ```

- [ ] **Fix critical leaks**

  Look for leaks in your code (not GTK internals):
  ```
  ==12345== 1,200 bytes in 1 blocks are definitely lost in loss record 123 of 456
  ==12345==    at malloc (vg_replace_malloc.c:309)
  ==12345==    by g_malloc (gmem.c:94)
  ==12345==    by ghid_some_function (gui-top-window.c:234)

  Fix: Ensure g_free() or g_object_unref() is called
  ```

  **Note:** Full leak-free operation comes later. For Milestone 1, just fix obvious leaks.

#### Task 5.4: Documentation (1.5 hours)

- [ ] **Update MIGRATION_LOG.md with final status**
  ```markdown
  # Milestone 1 Complete - $(date)

  ## Objectives Achieved
  - ✅ Build system configured for GTK3
  - ✅ gtkhid-main.c migrated
  - ✅ gui-top-window.c migrated
  - ✅ Application launches
  - ✅ Main window structure visible

  ## Current State

  ### Working
  - GTK3 libraries detected and linked
  - Application compiles and launches
  - Main window appears and is interactive
  - Menu bar structure present
  - Status bar visible

  ### Not Yet Working (Expected)
  - Drawing area is empty (Week 2: Drawing migration)
  - OpenGL rendering (Week 3: OpenGL migration)
  - Some dialogs may be broken (Week 4: Dialog migration)

  ### Known Issues
  - GTK Warning: [list warnings with line numbers]
  - Memory leak: [describe if significant]

  ## Files Migrated (✅ Complete, 🚧 Partial, ⏳ Not Started)
  - ✅ configure.ac
  - ✅ src/hid/gtk/Makefile.am
  - ✅ src/hid/gtk/gtkhid-main.c
  - ✅ src/hid/gtk/gui-top-window.c
  - 🚧 src/hid/gtk/gui.h (partial updates)
  - ⏳ src/hid/gtk/gtkhid-gdk.c (Week 2)
  - ⏳ src/hid/gtk/gtkhid-gl.c (Week 3)
  - ⏳ [remaining files]

  ## Statistics
  - Lines of code migrated: ~2,700
  - Files fully migrated: 2
  - Files partially migrated: 1
  - Compilation errors fixed: [count]
  - Compilation warnings: [count]

  ## Platform Test Results
  - Linux (Ubuntu 22.04, GTK 3.24.33): ✅ Pass
  - Linux (Fedora 38, GTK 3.24.37): [status]
  - macOS (Homebrew GTK 3.24.x): [status]
  - Windows (MSYS2 GTK 3.24.x): [status]

  ## Next Steps
  - Week 2: Drawing migration (gtkhid-gdk.c, gui-output-events.c)
  - Fix remaining GTK warnings
  - Test on additional platforms
  ```

- [ ] **Create screenshot documentation**

  Create `docs/MILESTONE_1_SCREENSHOTS.md`:
  ```markdown
  # Milestone 1 Screenshots

  ## Main Window (GTK3)
  ![Main Window](screenshots/milestone1-main-window.png)

  - Window appears correctly
  - Menu bar visible
  - Toolbar visible (if applicable)
  - Status bar visible
  - Drawing area present (empty - expected)

  ## Console Output
  ```
  [paste console output]
  ```

  ## GTK Warnings
  ```
  [paste GTK warnings]
  ```

  ## Comparison with GTK2
  | Feature | GTK2 | GTK3 (Milestone 1) |
  |---------|------|---------------------|
  | Launches | ✅ | ✅ |
  | Window structure | ✅ | ✅ |
  | Menu bar | ✅ | ✅ |
  | Drawing | ✅ | ⏳ Week 2 |
  | OpenGL | ✅ | ⏳ Week 3 |
  ```

- [ ] **Update main proposal document**

  Add Milestone 1 status to `GTK3_IMPLEMENTATION_PROPOSAL.md`:
  ```markdown
  ## Implementation Status

  ### Milestone 1: Foundation ✅ COMPLETE
  - Completed: [date]
  - Status: Application launches with GTK3, window structure migrated
  - Files: gtkhid-main.c, gui-top-window.c
  - Next: Milestone 2 - Drawing migration
  ```

#### Task 5.5: Code Cleanup and Commit (30 minutes)

- [ ] **Remove commented-out code**

  Look for debug comments:
  ```bash
  grep -n "// DEBUG\|// TODO\|// FIXME" src/hid/gtk/*.c
  ```

  Either fix or convert to proper TODO comments:
  ```c
  // TEMPORARY: Remove after drawing migration
  // TODO(Week 2): Migrate this GDK code to Cairo
  ```

- [ ] **Format code consistently**

  PCB uses specific formatting. Match existing style:
  ```c
  // Function definitions
  static void
  function_name(int param)
  {
    // Code
  }

  // Indentation: 2 spaces
  // Braces: K&R style (opening brace on same line for control structures)
  ```

- [ ] **Final commit for Milestone 1**
  ```bash
  git add -A
  git commit -m "Milestone 1: GTK3 Foundation Complete

  Completed build system migration and core window structure.
  Application now compiles and launches with GTK3.

  Changes:
  - configure.ac: Updated for GTK3 detection, removed GtkGLExt
  - Makefiles: Updated for GTK3 compilation flags
  - gtkhid-main.c: Full GTK3 migration
    - Removed deprecated threading code
    - Updated tooltips, widget access patterns
  - gui-top-window.c: Full GTK3 migration
    - Converted all GtkTable to GtkGrid
    - Converted all GtkHBox/VBox to GtkBox
    - Updated signals and widget access

  Current state:
  - Application launches successfully
  - Main window appears with correct structure
  - Menu bar and status bar visible
  - Drawing area present but empty (Week 2)

  Test results:
  - Linux (Ubuntu 22.04, GTK 3.24.33): PASS
  - No critical GTK errors
  - [X] warnings documented in MIGRATION_LOG.md

  Next: Milestone 2 - Drawing migration (gtkhid-gdk.c)"

  # Tag the milestone
  git tag -a milestone-1 -m "Milestone 1: GTK3 Foundation Complete"
  ```

- [ ] **Push to remote**
  ```bash
  git push origin gtk3-migration
  git push origin milestone-1
  ```

#### Task 5.6: Prepare for Week 2 (30 minutes)

- [ ] **Create Week 2 planning document**

  Create `MILESTONE_2_DRAWING.md` (brief outline):
  ```markdown
  # Milestone 2: Drawing Migration - Week 2

  ## Objectives
  - Migrate all GDK drawing to Cairo
  - Update gtkhid-gdk.c completely
  - Update gui-output-events.c
  - Get PCB rendering working

  ## Files to Migrate
  - gtkhid-gdk.c (~2000 lines) - Priority 1
  - gui-output-events.c (~1500 lines) - Priority 2

  ## Prerequisites (from Milestone 1)
  - ✅ GTK3 build system
  - ✅ Window structure
  - ✅ Signal handlers updated

  ## Success Criteria
  - PCB boards render correctly
  - All drawing primitives work (lines, arcs, pads)
  - No visual regressions vs GTK2

  (Detailed plan to be created at start of Week 2)
  ```

- [ ] **Document known issues for Week 2**
  ```markdown
  ## Known Issues to Address in Week 2

  1. Drawing area is empty
     - Cause: GDK drawing code not migrated
     - Fix: Migrate to Cairo in gtkhid-gdk.c

  2. GTK Warning: [specific warning]
     - Cause: [analysis]
     - Fix: [planned solution]

  3. [Other issues]
  ```

- [ ] **Set up Week 2 branch (optional)**
  ```bash
  # If you want separate tracking:
  git checkout -b milestone-2-drawing
  git push -u origin milestone-2-drawing

  # Or continue on gtk3-migration branch
  ```

### End of Day 5 / Milestone 1 Checklist

- [ ] **All tests completed and documented**
- [ ] **Critical issues fixed**
- [ ] **Memory leaks checked**
- [ ] **Documentation complete**
- [ ] **Code committed and tagged**
- [ ] **Ready for Week 2**

---

## Success Criteria

### At the end of Milestone 1, you should be able to:

#### Build & Configuration
- ✅ Run `./configure` successfully with GTK3 detected
- ✅ Build the application with `make` (no compilation errors in migrated files)
- ✅ See GTK 3.22+ version confirmed in configure output

#### Application Launch
- ✅ Launch the application: `./src/pcb`
- ✅ See the main window appear on screen
- ✅ Resize the main window without crashes
- ✅ Close the window cleanly (no crashes)
- ✅ Quit the application normally

#### Window Structure
- ✅ See menu bar at top of window
- ✅ Open menus (File, Edit, etc.) without crashes
- ✅ See toolbar (if applicable) with correct layout
- ✅ See status bar at bottom of window
- ✅ See drawing area (even if empty) in center

#### Code Quality
- ✅ No GTK-CRITICAL errors in console
- ✅ GTK warnings documented and understood
- ✅ No memory leaks in initialization code (per valgrind)
- ✅ Code follows consistent style

#### Documentation
- ✅ MIGRATION_LOG.md updated with complete status
- ✅ All changes committed to git with clear messages
- ✅ Screenshots captured showing current state
- ✅ Known issues documented

#### Platform Support
- ✅ Works on at least one platform (Linux recommended)
- ⚠️ Tested on additional platforms if available

### What Should NOT Work Yet (Expected Limitations)

- ❌ Drawing area will be empty (no PCB rendering) - **Week 2**
- ❌ OpenGL rendering will not work - **Week 3**
- ❌ Some dialogs may be broken - **Week 4**
- ❌ Custom widgets may not display correctly - **Week 4**

This is **expected and normal** for Milestone 1!

---

## Troubleshooting Guide

### Common Issues and Solutions

#### Issue: "Cannot find gtk+-3.0"

**Symptoms:**
```
configure: error: Package requirements (gtk+-3.0 >= 3.22.0) were not met
```

**Solutions:**
```bash
# Verify GTK3 is installed
pkg-config --modversion gtk+-3.0

# If not found, install:
# Ubuntu/Debian:
sudo apt-get install libgtk-3-dev

# Fedora:
sudo dnf install gtk3-devel

# macOS:
brew install gtk+3

# Set PKG_CONFIG_PATH if needed:
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
./configure
```

---

#### Issue: "widget->window" compilation error

**Symptoms:**
```
error: 'GtkWidget' {aka 'struct _GtkWidget'} has no member named 'window'
```

**Solutions:**
```c
// Replace:
widget->window

// With:
gtk_widget_get_window(widget)

// Or with check:
if (gtk_widget_get_realized(widget)) {
  GdkWindow *window = gtk_widget_get_window(widget);
  if (window) {
    // Use window
  }
}
```

---

#### Issue: "widget->allocation" compilation error

**Symptoms:**
```
error: 'GtkWidget' has no member named 'allocation'
```

**Solutions:**
```c
// Replace:
int w = widget->allocation.width;
int h = widget->allocation.height;

// With:
GtkAllocation allocation;
gtk_widget_get_allocation(widget, &allocation);
int w = allocation.width;
int h = allocation.height;
```

---

#### Issue: Application crashes on startup

**Symptoms:**
```
Segmentation fault (core dumped)
```

**Debugging Steps:**
```bash
# Run with gdb
gdb ./src/pcb
(gdb) run
# Wait for crash
(gdb) bt
# Get backtrace showing crash location

# Common causes:
# 1. Accessing widget->window before widget is realized
# 2. NULL pointer dereference
# 3. Calling GTK function with NULL widget

# Solutions:
# - Add NULL checks
# - Add realization checks
# - Ensure widgets are shown before accessing
```

---

#### Issue: Window appears but is completely blank

**Symptoms:**
- Window launches
- No menu bar, no content
- Just empty gray window

**Solutions:**
```c
// Ensure all widgets are shown:
gtk_widget_show_all(main_window);

// Or show each widget individually:
gtk_widget_show(menu_bar);
gtk_widget_show(toolbar);
gtk_widget_show(drawing_area);
gtk_widget_show(status_bar);
gtk_widget_show(main_window);

// Check that widgets are added to containers:
gtk_container_add(GTK_CONTAINER(window), main_vbox);
gtk_box_pack_start(GTK_BOX(main_vbox), menu_bar, FALSE, FALSE, 0);
```

---

#### Issue: GTK-WARNING messages in console

**Symptoms:**
```
(pcb:12345): Gtk-WARNING **: 10:30:45.123: Attempting to add a widget with type GtkMenuItem to a GtkBox, but as a GtkBin subclass a GtkMenuItem can only contain one widget at a time; it already contains a widget of type GtkAccelLabel
```

**Solutions:**

This specific warning means you're trying to add a second child to a GtkMenuItem (which only holds one child).

```c
// Wrong:
gtk_container_add(GTK_CONTAINER(menu_item), label);
gtk_container_add(GTK_CONTAINER(menu_item), icon);  // ERROR

// Right - use a box to hold multiple children:
GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
gtk_container_add(GTK_CONTAINER(box), icon);
gtk_container_add(GTK_CONTAINER(box), label);
gtk_container_add(GTK_CONTAINER(menu_item), box);
```

**General approach to GTK warnings:**
1. Read the warning carefully
2. Note the file and line number (if given)
3. Check GTK3 documentation for that function
4. Verify widget types and containment rules

---

#### Issue: "undefined reference to glBegin"

**Symptoms:**
```
undefined reference to `glBegin'
undefined reference to `glEnd'
```

**Solutions:**
```bash
# Add OpenGL libraries to link flags
# In Makefile.am or configure.ac:
LIBS="$LIBS -lGL -lGLU"

# Or in src/hid/gtk/Makefile.am:
pcb_LDADD = ... $(GL_LIBS)
```

---

#### Issue: Menu items have no icons

**Symptoms:**
- Menus work but items have no icons
- Console shows icon-related warnings

**Solutions:**
```c
// GTK3 deprecated stock items
// For Milestone 1, just use text menu items:

menuitem = gtk_menu_item_new_with_label("Open");

// Icons can be added later in Week 4:
// (This is cosmetic, not critical for Milestone 1)
```

---

#### Issue: Valgrind shows many GTK leaks

**Symptoms:**
```
==12345== 123,456 bytes in 789 blocks are definitely lost
```

**Analysis:**

GTK has some internal "leaks" that are actually cached allocations. Look for:

```bash
# Find YOUR leaks, not GTK's:
valgrind --leak-check=full ./pcb 2>&1 | grep "src/hid/gtk"

# Focus on:
# - definitely lost: Real leaks, should fix
# - possibly lost: May be GTK internals, investigate
# - still reachable: Usually OK (cached)
```

**Common real leaks:**
```c
// Leak: String not freed
char *str = g_strdup("text");
// ... use str ...
// MISSING: g_free(str);

// Leak: Object not unreferenced
GObject *obj = g_object_new(TYPE, NULL);
// ... use obj ...
// MISSING: g_object_unref(obj);

// Leak: Widget created but never destroyed
GtkWidget *widget = gtk_label_new("text");
// MISSING: gtk_widget_destroy(widget);
// OR: Widget should be added to a container that owns it
```

---

## Summary

At the end of Milestone 1, you will have:

✅ A working GTK3 build environment
✅ Core initialization code migrated
✅ Main window structure converted
✅ Application that launches and displays
✅ Foundation for Week 2 drawing migration

**Time Investment:** 40 hours (5 days × 8 hours)
**Files Migrated:** 2 core files (~2,700 lines)
**Code Quality:** Compiling, launching, no critical errors

**Ready for:** Milestone 2 - Drawing migration (Week 2)

---

**Document Version:** 1.0
**Last Updated:** November 17, 2025
**Status:** Ready for Use
