# Milestone 4D: Configuration & Remaining Files - Detailed Implementation Plan

**Duration**: 2 days (16 hours)
**Days**: Day 7-8 of Milestone 4
**Prerequisites**: Milestones 1, 2, 3, 4A, 4B, and 4C completed
**Goal**: Migrate the preferences dialog and all remaining GUI files to complete the GTK3 HID implementation

---

## Overview

Milestone 4D completes the GTK3 HID file migration by handling:

1. **`gui-config.c`** (1,800 lines, 10 hours): Multi-tab preferences dialog
   - General settings tab
   - Layer colors tab
   - Library paths tab
   - Window settings tab
   - Backup settings tab

2. **Remaining GUI files** (6 hours): Miscellaneous dialogs and utility files
   - `gui-utils.c`: Utility functions
   - `gui-pinout.c`: Pinout preview dialog
   - `gui-log.c`: Message log window
   - Other small helper files

After this milestone, all GUI code will be migrated to GTK3, leaving only final integration testing (Milestone 4E).

---

## Day 1: Preferences Dialog (8 hours)

### Hour 1-2: Dialog Structure and Notebook Setup

**File**: `src/hid/gtk3/gui-config.c`

```bash
# Copy and rename
cp src/hid/gtk/gui-config.c src/hid/gtk3/gui-config.c
sed -i 's/ghid_/ghid3_/g' src/hid/gtk3/gui-config.c
```

**Create multi-tab dialog**:

```c
/* gui-config.c - Preferences dialog */

#include "config.h"
#include "gtkhid.h"
#include "gui.h"
#include "gui-config.h"

#include <gtk/gtk.h>

static GtkWidget *config_dialog = NULL;
static GtkWidget *notebook = NULL;

/* Create preferences dialog */
GtkWidget *
ghid3_config_dialog_create(void)
{
  GtkWidget *dialog;
  GtkWidget *content_area;

  /* Create dialog */
  dialog = gtk_dialog_new_with_buttons(
    "Preferences",
    GTK_WINDOW(ghid3_port.top_window),
    GTK_DIALOG_MODAL,
    "_Cancel", GTK_RESPONSE_CANCEL,
    "_Apply", GTK_RESPONSE_APPLY,
    "_OK", GTK_RESPONSE_OK,
    NULL);

  gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 500);

  /* Get content area */
  content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

  /* Create notebook (tabbed interface) */
  /* GTK3: GtkNotebook API unchanged from GTK2 */
  notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
  gtk_container_set_border_width(GTK_CONTAINER(notebook), 6);
  gtk_box_pack_start(GTK_BOX(content_area), notebook, TRUE, TRUE, 0);

  /* Add tabs */
  ghid3_config_add_general_tab(notebook);
  ghid3_config_add_colors_tab(notebook);
  ghid3_config_add_library_tab(notebook);
  ghid3_config_add_window_tab(notebook);
  ghid3_config_add_backup_tab(notebook);

  /* Connect response signal */
  g_signal_connect(dialog, "response",
                  G_CALLBACK(ghid3_config_dialog_response_cb), NULL);

  config_dialog = dialog;

  return dialog;
}

/* Show preferences dialog */
void
ghid3_config_dialog_show(void)
{
  if (config_dialog == NULL)
    config_dialog = ghid3_config_dialog_create();

  /* Load current settings into widgets */
  ghid3_config_load_settings();

  gtk_widget_show_all(config_dialog);
  gtk_window_present(GTK_WINDOW(config_dialog));
}

/* Dialog response (OK, Cancel, Apply) */
static void
ghid3_config_dialog_response_cb(GtkDialog *dialog, gint response_id, gpointer user_data)
{
  switch (response_id)
    {
    case GTK_RESPONSE_OK:
      /* Save settings and close */
      ghid3_config_save_settings();
      gtk_widget_hide(GTK_WIDGET(dialog));
      break;

    case GTK_RESPONSE_APPLY:
      /* Save settings but keep dialog open */
      ghid3_config_save_settings();
      break;

    case GTK_RESPONSE_CANCEL:
      /* Discard changes and close */
      gtk_widget_hide(GTK_WIDGET(dialog));
      break;
    }
}
```

**GTK3 changes**:
- `GtkNotebook`: API unchanged (still works in GTK3)
- Dialog layout: Using `gtk_box_pack_start()` still works
- Response handling: Same as GTK2

### Hour 3-4: General Settings Tab

```c
/* General settings tab */
static void
ghid3_config_add_general_tab(GtkNotebook *notebook)
{
  GtkWidget *grid;
  GtkWidget *label;
  GtkWidget *widget;
  gint row = 0;

  /* Create grid for form layout (replaces GtkTable) */
  grid = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
  gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
  gtk_container_set_border_width(GTK_CONTAINER(grid), 12);

  /* Grid size setting */
  label = gtk_label_new("Grid spacing:");
  gtk_widget_set_halign(label, GTK_ALIGN_END);
  gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

  widget = gtk_spin_button_new_with_range(1.0, 1000.0, 1.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), Settings.Grid);
  gtk_widget_set_hexpand(widget, TRUE);
  gtk_grid_attach(GTK_GRID(grid), widget, 1, row, 1, 1);
  g_object_set_data(G_OBJECT(notebook), "grid-spacing-spin", widget);

  row++;

  /* Grid units (mil/mm) */
  label = gtk_label_new("Grid units:");
  gtk_widget_set_halign(label, GTK_ALIGN_END);
  gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

  widget = gtk_combo_box_text_new();
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), "mil");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), "mm");
  gtk_combo_box_set_active(GTK_COMBO_BOX(widget),
                           Settings.grid_unit == MIL_TO_COORD(1) ? 0 : 1);
  gtk_widget_set_hexpand(widget, TRUE);
  gtk_grid_attach(GTK_GRID(grid), widget, 1, row, 1, 1);
  g_object_set_data(G_OBJECT(notebook), "grid-units-combo", widget);

  row++;

  /* Line thickness */
  label = gtk_label_new("Default line thickness:");
  gtk_widget_set_halign(label, GTK_ALIGN_END);
  gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

  widget = gtk_spin_button_new_with_range(1.0, 500.0, 1.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), Settings.LineThickness);
  gtk_grid_attach(GTK_GRID(grid), widget, 1, row, 1, 1);
  g_object_set_data(G_OBJECT(notebook), "line-thickness-spin", widget);

  row++;

  /* Via size */
  label = gtk_label_new("Default via size:");
  gtk_widget_set_halign(label, GTK_ALIGN_END);
  gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

  widget = gtk_spin_button_new_with_range(10.0, 500.0, 1.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), Settings.ViaThickness);
  gtk_grid_attach(GTK_GRID(grid), widget, 1, row, 1, 1);
  g_object_set_data(G_OBJECT(notebook), "via-size-spin", widget);

  row++;

  /* Checkboxes for boolean options */
  widget = gtk_check_button_new_with_label("Show DRC errors during design");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), Settings.ShowDRC);
  gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 2, 1);
  g_object_set_data(G_OBJECT(notebook), "show-drc-check", widget);

  row++;

  widget = gtk_check_button_new_with_label("Auto-save backup");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), Settings.BackupInterval > 0);
  gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 2, 1);
  g_object_set_data(G_OBJECT(notebook), "auto-save-check", widget);

  /* Add tab to notebook */
  label = gtk_label_new("General");
  gtk_notebook_append_page(notebook, grid, label);
}
```

**GTK3 changes**:
- **GtkTable → GtkGrid**: Major change
  - `gtk_grid_attach(grid, widget, left, top, width, height)`
  - Use `gtk_widget_set_hexpand()` for expansion instead of GtkAttachOptions
  - Use `gtk_widget_set_halign()` for alignment
- `GtkSpinButton`: API unchanged
- `GtkCheckButton`: API unchanged
- `GtkComboBoxText`: Replaces deprecated `GtkCombo`

### Hour 5-6: Layer Colors Tab

```c
/* Layer colors tab with color choosers */
static void
ghid3_config_add_colors_tab(GtkNotebook *notebook)
{
  GtkWidget *scrolled;
  GtkWidget *grid;
  GtkWidget *label;
  GtkWidget *color_button;
  gint row = 0;
  int i;

  /* Scrolled window for many layers */
  scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                GTK_POLICY_NEVER,
                                GTK_POLICY_AUTOMATIC);

  grid = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
  gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
  gtk_container_set_border_width(GTK_CONTAINER(grid), 12);
  gtk_container_add(GTK_CONTAINER(scrolled), grid);

  /* Add color chooser for each layer */
  for (i = 0; i < max_copper_layer; i++)
    {
      LayerType *layer = &PCB->Data->Layer[i];
      GdkRGBA rgba;

      label = gtk_label_new(layer->Name);
      gtk_widget_set_halign(label, GTK_ALIGN_END);
      gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

      /* GtkColorButton for color selection (GTK3) */
      /* GTK3: Uses GdkRGBA instead of GdkColor */
      gdk_rgba_parse(&rgba, layer->Color);

      color_button = gtk_color_button_new_with_rgba(&rgba);
      gtk_color_button_set_title(GTK_COLOR_BUTTON(color_button), layer->Name);
      gtk_grid_attach(GTK_GRID(grid), color_button, 1, row, 1, 1);

      /* Store reference for later retrieval */
      gchar *key = g_strdup_printf("layer-color-%d", i);
      g_object_set_data_full(G_OBJECT(notebook), key, color_button,
                            (GDestroyNotify)g_free);

      row++;
    }

  /* Add tab */
  label = gtk_label_new("Layer Colors");
  gtk_notebook_append_page(notebook, scrolled, label);
}
```

**GTK3 changes**:
- `GtkColorButton`: Uses `GdkRGBA` instead of `GdkColor`
  - `gtk_color_button_new_with_rgba()` instead of `gtk_color_button_new_with_color()`
  - `gtk_color_chooser_get_rgba()` instead of `gtk_color_button_get_color()`

### Hour 7-8: Library Paths Tab and Settings Persistence

```c
/* Library paths tab */
static void
ghid3_config_add_library_tab(GtkNotebook *notebook)
{
  GtkWidget *vbox;
  GtkWidget *scrolled;
  GtkWidget *tree_view;
  GtkWidget *button_box;
  GtkListStore *store;

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);

  /* Scrolled window with tree view */
  scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                GTK_POLICY_AUTOMATIC,
                                GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled),
                                     GTK_SHADOW_IN);
  gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

  /* List store for library paths */
  store = gtk_list_store_new(1, G_TYPE_STRING);

  tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  g_object_unref(store);

  GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
  GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
    "Library Path", renderer, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

  gtk_container_add(GTK_CONTAINER(scrolled), tree_view);

  /* Button box for add/remove */
  button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_START);
  gtk_box_set_spacing(GTK_BOX(button_box), 6);
  gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 0);

  GtkWidget *add_button = gtk_button_new_with_label("Add...");
  g_signal_connect(add_button, "clicked",
                  G_CALLBACK(ghid3_config_library_add_cb), store);
  gtk_container_add(GTK_CONTAINER(button_box), add_button);

  GtkWidget *remove_button = gtk_button_new_with_label("Remove");
  g_signal_connect(remove_button, "clicked",
                  G_CALLBACK(ghid3_config_library_remove_cb), tree_view);
  gtk_container_add(GTK_CONTAINER(button_box), remove_button);

  g_object_set_data(G_OBJECT(notebook), "library-paths-store", store);

  /* Add tab */
  GtkWidget *label = gtk_label_new("Library Paths");
  gtk_notebook_append_page(notebook, vbox, label);
}

/* Load settings from PCB into dialog widgets */
static void
ghid3_config_load_settings(void)
{
  GtkWidget *widget;

  /* Load general settings */
  widget = g_object_get_data(G_OBJECT(notebook), "grid-spacing-spin");
  if (widget)
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), Settings.Grid);

  widget = g_object_get_data(G_OBJECT(notebook), "line-thickness-spin");
  if (widget)
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), Settings.LineThickness);

  /* Load layer colors */
  for (int i = 0; i < max_copper_layer; i++)
    {
      gchar *key = g_strdup_printf("layer-color-%d", i);
      widget = g_object_get_data(G_OBJECT(notebook), key);
      if (widget)
        {
          GdkRGBA rgba;
          gdk_rgba_parse(&rgba, PCB->Data->Layer[i].Color);
          gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(widget), &rgba);
        }
      g_free(key);
    }

  /* Load library paths */
  /* ... */
}

/* Save settings from dialog widgets to PCB */
static void
ghid3_config_save_settings(void)
{
  GtkWidget *widget;

  /* Save general settings */
  widget = g_object_get_data(G_OBJECT(notebook), "grid-spacing-spin");
  if (widget)
    Settings.Grid = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));

  widget = g_object_get_data(G_OBJECT(notebook), "line-thickness-spin");
  if (widget)
    Settings.LineThickness = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));

  /* Save layer colors */
  for (int i = 0; i < max_copper_layer; i++)
    {
      gchar *key = g_strdup_printf("layer-color-%d", i);
      widget = g_object_get_data(G_OBJECT(notebook), key);
      if (widget)
        {
          GdkRGBA rgba;
          gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(widget), &rgba);

          /* Convert RGBA to string and store */
          gchar *color_str = gdk_rgba_to_string(&rgba);
          g_free(PCB->Data->Layer[i].Color);
          PCB->Data->Layer[i].Color = color_str;
        }
      g_free(key);
    }

  /* Apply settings and redraw */
  ghid3_invalidate_all();
}
```

---

## Day 2: Remaining Files (8 hours)

### Hour 1-3: Utility Files Migration

**Files**: `gui-utils.c`, `gui-pinout.c`, `gui-log.c`

```c
/* gui-utils.c - Utility functions */

/* Convert coordinates for display */
gchar *
ghid3_coord_to_string(Coord coord, gboolean use_units)
{
  gdouble value;
  const gchar *units;

  if (Settings.grid_unit == MIL_TO_COORD(1))
    {
      value = COORD_TO_MIL(coord);
      units = "mil";
    }
  else
    {
      value = COORD_TO_MM(coord);
      units = "mm";
    }

  if (use_units)
    return g_strdup_printf("%.2f %s", value, units);
  else
    return g_strdup_printf("%.2f", value);
}

/* Create standard message dialog */
void
ghid3_show_message(GtkMessageType type, const gchar *message)
{
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new(
    GTK_WINDOW(ghid3_port.top_window),
    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
    type,
    GTK_BUTTONS_OK,
    "%s", message);

  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}
```

```c
/* gui-pinout.c - Pinout preview dialog */

GtkWidget *
ghid3_pinout_dialog_create(ElementType *element)
{
  GtkWidget *dialog;
  GtkWidget *content_area;
  GtkWidget *drawing_area;
  GtkWidget *label;
  GtkWidget *vbox;

  dialog = gtk_dialog_new_with_buttons(
    "Pinout",
    GTK_WINDOW(ghid3_port.top_window),
    GTK_DIALOG_DESTROY_WITH_PARENT,
    "_Close", GTK_RESPONSE_CLOSE,
    NULL);

  content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
  gtk_box_pack_start(GTK_BOX(content_area), vbox, TRUE, TRUE, 0);

  /* Element name label */
  label = gtk_label_new(NAMEONPCB_NAME(element));
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  /* Drawing area for pinout preview */
  drawing_area = gtk_drawing_area_new();
  gtk_widget_set_size_request(drawing_area, 300, 300);

  /* GTK3: Connect draw signal */
  g_signal_connect(drawing_area, "draw",
                  G_CALLBACK(ghid3_pinout_draw_cb), element);

  gtk_box_pack_start(GTK_BOX(vbox), drawing_area, TRUE, TRUE, 0);

  return dialog;
}

/* Draw pinout using Cairo */
static gboolean
ghid3_pinout_draw_cb(GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
  ElementType *element = (ElementType *)user_data;

  /* Draw element footprint */
  /* Use Cairo drawing functions to render pads, pins, silkscreen */

  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);  /* White background */
  cairo_paint(cr);

  /* Draw pads */
  PAD_LOOP(element);
  {
    cairo_set_source_rgb(cr, 0.8, 0.8, 0.0);  /* Gold for pads */
    /* Draw pad geometry */
  }
  END_LOOP;

  /* Draw pins */
  PIN_LOOP(element);
  {
    cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);  /* Silver for pins */
    cairo_arc(cr, pin->X, pin->Y, pin->Thickness / 2, 0, 2 * M_PI);
    cairo_fill(cr);
  }
  END_LOOP;

  return TRUE;
}
```

```c
/* gui-log.c - Message log window */

GtkWidget *
ghid3_log_window_create(void)
{
  GtkWidget *window;
  GtkWidget *scrolled;
  GtkWidget *text_view;
  GtkTextBuffer *buffer;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "Message Log");
  gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

  scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                GTK_POLICY_AUTOMATIC,
                                GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(window), scrolled);

  /* Text view for log messages */
  /* GTK3: GtkTextView API unchanged */
  buffer = gtk_text_buffer_new(NULL);
  text_view = gtk_text_view_new_with_buffer(buffer);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);

  gtk_container_add(GTK_CONTAINER(scrolled), text_view);

  return window;
}

/* Append message to log */
void
ghid3_log_append(const gchar *message)
{
  GtkTextBuffer *buffer;
  GtkTextIter iter;

  if (ghid3_port.log_window == NULL)
    return;

  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ghid3_port.log_text_view));

  gtk_text_buffer_get_end_iter(buffer, &iter);
  gtk_text_buffer_insert(buffer, &iter, message, -1);
  gtk_text_buffer_insert(buffer, &iter, "\n", -1);

  /* Auto-scroll to bottom */
  gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(ghid3_port.log_text_view),
                               &iter, 0.0, FALSE, 0.0, 0.0);
}
```

### Hour 4-6: Final File Review and Cleanup

Review all migrated files for:
- Consistent naming (`ghid3_` prefix)
- Proper header includes
- No deprecated API usage
- Memory leak prevention
- Error handling

```bash
# Check for deprecated GTK2 APIs
grep -r "gtk_widget->window" src/hid/gtk3/  # Should use gtk_widget_get_window()
grep -r "gtk_table" src/hid/gtk3/  # Should use gtk_grid
grep -r "gtk_hbox" src/hid/gtk3/  # Should use gtk_box_new(HORIZONTAL)
grep -r "GdkColor\b" src/hid/gtk3/  # Should use GdkRGBA
grep -r "expose-event" src/hid/gtk3/  # Should use "draw"
```

### Hour 7-8: Build System Integration

Update build files to include all new source files.

**File**: `src/hid/gtk3/Makefile.am`

```makefile
# GTK3 HID source files

gtk3_SOURCES = \
	gtkhid-main.c \
	gtkhid-gdk.c \
	gtkhid-gl.c \
	gui-top-window.c \
	gui-output-events.c \
	gui-dialog.c \
	gui-config.c \
	gui-library-window.c \
	gui-netlist-window.c \
	gui-drc-window.c \
	gui-utils.c \
	gui-pinout.c \
	gui-log.c \
	gui-gl.c \
	gui-gl-3d.c \
	gui-gl-camera.c \
	ghid-main-menu.c \
	ghid-layer-selector.c \
	ghid-route-style-selector.c

gtk3_HEADERS = \
	gtkhid.h \
	gui.h \
	gui-dialog.h \
	gui-config.h \
	ghid-layer-selector.h \
	ghid-route-style-selector.h

# Compiler flags
AM_CFLAGS = @GTK3_CFLAGS@ @GL_CFLAGS@
AM_LDFLAGS = @GTK3_LIBS@ @GL_LIBS@
```

**Verify build**:

```bash
cd /path/to/pcb
./autogen.sh
./configure --enable-gtk3
make clean
make

# Check for warnings
make 2>&1 | grep -i "warning\|error"

# Should compile with no errors
```

---

## Testing Protocol

### Functional Tests

1. **Preferences dialog**:
   - Open Settings → Preferences
   - Change grid spacing, verify updates
   - Change layer color, verify updates
   - Add/remove library paths
   - Click OK, verify settings saved
   - Reopen, verify settings persisted

2. **Utility functions**:
   - View pinout (right-click component)
   - Check message log (View → Message Log)
   - Verify coordinate display in status bar

3. **Build verification**:
   - Clean build completes
   - No warnings about deprecated APIs
   - All symbols resolve (no linker errors)

### Regression Tests

Compare with GTK2 HID:

```bash
# Run both HIDs side-by-side
./src/pcb --hid gtk &  # GTK2
./src/pcb --hid gtk3 & # GTK3

# Open same PCB file in both
# Navigate to Settings → Preferences
# Compare all tabs - should look similar
```

---

## Success Criteria

- [ ] Preferences dialog opens with all tabs
- [ ] All settings can be changed and saved
- [ ] Layer colors update immediately
- [ ] Library paths can be added/removed
- [ ] Build completes without warnings
- [ ] All utility dialogs work
- [ ] Visual parity with GTK2 HID
- [ ] No memory leaks or crashes

---

## Estimated Time

**Total**: 16 hours (2 days)
- Day 1: Preferences dialog (8 hours)
- Day 2: Remaining files and integration (8 hours)

**Next**: Milestone 4E - Final integration testing and release preparation
