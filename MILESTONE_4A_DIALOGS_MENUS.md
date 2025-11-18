# Milestone 4A: Dialogs and Menus - Detailed Implementation Plan

**Duration**: 2 days (16 hours)
**Days**: Day 1-2 of Milestone 4
**Prerequisites**: Milestones 1, 2, and 3 completed
**Goal**: Migrate core dialog infrastructure and menu system to GTK3, ensuring all dialogs display correctly and all menu items function properly

---

## Overview

Milestone 4A focuses on the core user interface infrastructure: dialogs and menus. These are the primary ways users interact with PCB beyond the drawing canvas:

- **Dialogs** provide forms for user input (file selection, preferences, printing options)
- **Menus** organize commands and provide keyboard shortcuts for efficient workflows

This milestone migrates two critical files:

1. **`gui-dialog.c`** (800 lines): Core dialog infrastructure including file dialogs, message dialogs, and utility dialog functions
2. **`ghid-main-menu.c`** (900 lines): Main menu bar with File, Edit, View, Settings, and other menus

The migration involves:
- Converting GtkTable layouts to GtkGrid
- Replacing deprecated widgets (GtkCombo → GtkComboBoxText, etc.)
- Updating signal signatures (especially for draw events)
- Fixing keyboard shortcut constants (GDK_* → GDK_KEY_*)
- Ensuring visual parity with GTK2 version

**Critical Reminder**: This is parallel HID development. All code goes in `src/hid/gtk3/`, and all functions use the `ghid3_` prefix to avoid conflicts with the existing GTK2 HID in `src/hid/gtk/`.

---

## Day 1: Core Dialog Infrastructure (8 hours)

### Day 1 Overview

Migrate `gui-dialog.c` from GTK2 to GTK3. This file contains essential dialog functionality used throughout the application, including:
- File chooser dialogs (open, save, export)
- Message dialogs (info, warning, error, confirm)
- Input dialogs (text entry, number entry)
- Common dialog utilities

**Strategy**: Copy from GTK2 HID, rename functions, update deprecated APIs, test each dialog type.

### Hour 1-2: File Setup and Function Renaming

**File**: `src/hid/gtk3/gui-dialog.c` (copy from `src/hid/gtk/gui-dialog.c`)

#### Step 1: Copy and rename

```bash
# Copy GTK2 version to GTK3 directory
cp src/hid/gtk/gui-dialog.c src/hid/gtk3/gui-dialog.c
cp src/hid/gtk/gui-dialog.h src/hid/gtk3/gui-dialog.h
```

#### Step 2: Global function renaming

Use editor or sed to rename all functions:

```bash
# In gui-dialog.c and gui-dialog.h
# Replace ghid_ prefix with ghid3_
sed -i 's/ghid_/ghid3_/g' src/hid/gtk3/gui-dialog.c
sed -i 's/ghid_/ghid3_/g' src/hid/gtk3/gui-dialog.h

# Update include guards
sed -i 's/GHID_DIALOG_H/GHID3_DIALOG_H/g' src/hid/gtk3/gui-dialog.h
```

#### Step 3: Update includes

```c
/* gui-dialog.c - Update header includes */

#include "gtkhid.h"      /* Use GTK3 version */
#include "gui.h"          /* Use GTK3 version */
#include "gui-dialog.h"   /* Our header */

#include <gtk/gtk.h>      /* GTK3 */
#include <glib.h>
#include <string.h>
```

**Testing checkpoint**: Verify file compiles (even if functions don't work yet). Run `make` and fix any syntax errors.

### Hour 3-4: File Chooser Dialogs

Migrate file chooser dialog functions (open, save, export).

```c
/* File chooser dialog for opening PCB files */
gchar *
ghid3_dialog_file_select_open(const gchar *title,
                              const gchar *current_folder,
                              const gchar *current_name)
{
  GtkWidget *dialog;
  gchar *filename = NULL;

  /* Create file chooser dialog */
  /* GTK2 and GTK3: API is mostly the same */
  dialog = gtk_file_chooser_dialog_new(
    title,
    GTK_WINDOW(ghid3_port.top_window),
    GTK_FILE_CHOOSER_ACTION_OPEN,
    "_Cancel", GTK_RESPONSE_CANCEL,  /* GTK3: use stock IDs or custom labels */
    "_Open", GTK_RESPONSE_ACCEPT,
    NULL);

  /* Set current folder if specified */
  if (current_folder != NULL)
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), current_folder);

  /* Set default file name if specified */
  if (current_name != NULL)
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), current_name);

  /* Add file filters */
  GtkFileFilter *filter;

  /* PCB files filter */
  filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, "PCB Files");
  gtk_file_filter_add_pattern(filter, "*.pcb");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

  /* All files filter */
  filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, "All Files");
  gtk_file_filter_add_pattern(filter, "*");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

  /* Run dialog */
  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
      filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    }

  gtk_widget_destroy(dialog);

  return filename;
}

/* File chooser dialog for saving PCB files */
gchar *
ghid3_dialog_file_select_save(const gchar *title,
                              const gchar *current_folder,
                              const gchar *current_name)
{
  GtkWidget *dialog;
  gchar *filename = NULL;

  dialog = gtk_file_chooser_dialog_new(
    title,
    GTK_WINDOW(ghid3_port.top_window),
    GTK_FILE_CHOOSER_ACTION_SAVE,
    "_Cancel", GTK_RESPONSE_CANCEL,
    "_Save", GTK_RESPONSE_ACCEPT,
    NULL);

  /* Enable overwrite confirmation */
  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

  if (current_folder != NULL)
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), current_folder);

  if (current_name != NULL)
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), current_name);

  /* Add file filter */
  GtkFileFilter *filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, "PCB Files");
  gtk_file_filter_add_pattern(filter, "*.pcb");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
      filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    }

  gtk_widget_destroy(dialog);

  return filename;
}
```

**Key GTK3 Changes**:
- Button labels: Use `_Cancel`, `_Open`, `_Save` instead of `GTK_STOCK_*` constants (deprecated in GTK3)
- File chooser API: Mostly unchanged from GTK2
- Dialog parenting: Use `GTK_WINDOW(ghid3_port.top_window)` for proper modal behavior

**Testing checkpoint**: Test file open and save dialogs. Verify they display correctly, filters work, and return correct filenames.

### Hour 5-6: Message Dialogs

Migrate message dialog functions (info, warning, error, confirmation).

```c
/* Show information message dialog */
void
ghid3_dialog_message(const gchar *title, const gchar *message)
{
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new(
    GTK_WINDOW(ghid3_port.top_window),
    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
    GTK_MESSAGE_INFO,
    GTK_BUTTONS_OK,
    "%s", message);

  if (title)
    gtk_window_set_title(GTK_WINDOW(dialog), title);

  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

/* Show warning message dialog */
void
ghid3_dialog_warning(const gchar *title, const gchar *message)
{
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new(
    GTK_WINDOW(ghid3_port.top_window),
    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
    GTK_MESSAGE_WARNING,
    GTK_BUTTONS_OK,
    "%s", message);

  if (title)
    gtk_window_set_title(GTK_WINDOW(dialog), title);

  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

/* Show error message dialog */
void
ghid3_dialog_error(const gchar *title, const gchar *message)
{
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new(
    GTK_WINDOW(ghid3_port.top_window),
    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
    GTK_MESSAGE_ERROR,
    GTK_BUTTONS_OK,
    "%s", message);

  if (title)
    gtk_window_set_title(GTK_WINDOW(dialog), title);

  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

/* Show confirmation dialog (Yes/No) */
gboolean
ghid3_dialog_confirm(const gchar *title, const gchar *message)
{
  GtkWidget *dialog;
  gint response;

  dialog = gtk_message_dialog_new(
    GTK_WINDOW(ghid3_port.top_window),
    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
    GTK_MESSAGE_QUESTION,
    GTK_BUTTONS_YES_NO,
    "%s", message);

  if (title)
    gtk_window_set_title(GTK_WINDOW(dialog), title);

  response = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);

  return (response == GTK_RESPONSE_YES);
}

/* Show confirmation dialog with custom buttons */
gint
ghid3_dialog_confirm_all(const gchar *title, const gchar *message)
{
  GtkWidget *dialog;
  gint response;

  /* Create custom dialog with three buttons: Yes, No, Cancel */
  dialog = gtk_message_dialog_new(
    GTK_WINDOW(ghid3_port.top_window),
    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
    GTK_MESSAGE_QUESTION,
    GTK_BUTTONS_NONE,  /* No default buttons */
    "%s", message);

  if (title)
    gtk_window_set_title(GTK_WINDOW(dialog), title);

  /* Add custom buttons */
  gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                        "_Yes", GTK_RESPONSE_YES,
                        "_No", GTK_RESPONSE_NO,
                        "_Cancel", GTK_RESPONSE_CANCEL,
                        NULL);

  response = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);

  return response;
}
```

**Key GTK3 Changes**:
- Message dialog API: Unchanged from GTK2
- Button constants: `GTK_BUTTONS_OK`, `GTK_BUTTONS_YES_NO` still work
- Response codes: `GTK_RESPONSE_*` unchanged
- **Important**: Use `%s` format string to prevent format string vulnerabilities

**Testing checkpoint**: Test each message dialog type (info, warning, error, confirm). Verify icons display correctly and buttons work.

### Hour 7-8: Input Dialogs and Testing

Create input dialogs and comprehensive testing.

```c
/* Text input dialog */
gchar *
ghid3_dialog_input(const gchar *title, const gchar *prompt, const gchar *default_value)
{
  GtkWidget *dialog;
  GtkWidget *content_area;
  GtkWidget *grid;
  GtkWidget *label;
  GtkWidget *entry;
  gchar *result = NULL;

  /* Create dialog */
  dialog = gtk_dialog_new_with_buttons(
    title,
    GTK_WINDOW(ghid3_port.top_window),
    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
    "_Cancel", GTK_RESPONSE_CANCEL,
    "_OK", GTK_RESPONSE_OK,
    NULL);

  /* Get content area */
  content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

  /* Create grid layout (GTK3 - was GtkTable in GTK2) */
  grid = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
  gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
  gtk_container_set_border_width(GTK_CONTAINER(grid), 12);
  gtk_container_add(GTK_CONTAINER(content_area), grid);

  /* Add prompt label */
  label = gtk_label_new(prompt);
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

  /* Add text entry */
  entry = gtk_entry_new();
  if (default_value != NULL)
    gtk_entry_set_text(GTK_ENTRY(entry), default_value);
  gtk_widget_set_hexpand(entry, TRUE);
  gtk_grid_attach(GTK_GRID(grid), entry, 0, 1, 1, 1);

  /* Entry activates default response (press Enter to submit) */
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

  /* Show all widgets */
  gtk_widget_show_all(dialog);

  /* Run dialog */
  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
    {
      const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
      result = g_strdup(text);
    }

  gtk_widget_destroy(dialog);

  return result;
}

/* Numeric input dialog */
gdouble
ghid3_dialog_input_number(const gchar *title, const gchar *prompt,
                         gdouble default_value, gdouble min_value, gdouble max_value)
{
  GtkWidget *dialog;
  GtkWidget *content_area;
  GtkWidget *grid;
  GtkWidget *label;
  GtkWidget *spin_button;
  gdouble result = default_value;

  dialog = gtk_dialog_new_with_buttons(
    title,
    GTK_WINDOW(ghid3_port.top_window),
    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
    "_Cancel", GTK_RESPONSE_CANCEL,
    "_OK", GTK_RESPONSE_OK,
    NULL);

  content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

  grid = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
  gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
  gtk_container_set_border_width(GTK_CONTAINER(grid), 12);
  gtk_container_add(GTK_CONTAINER(content_area), grid);

  label = gtk_label_new(prompt);
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

  /* Create spin button for numeric input */
  /* GTK2 and GTK3: API unchanged */
  spin_button = gtk_spin_button_new_with_range(min_value, max_value, 1.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), default_value);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_button), 2);
  gtk_widget_set_hexpand(spin_button, TRUE);
  gtk_grid_attach(GTK_GRID(grid), spin_button, 0, 1, 1, 1);

  gtk_widget_show_all(dialog);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
    {
      result = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button));
    }

  gtk_widget_destroy(dialog);

  return result;
}
```

**Key GTK3 Changes**:
- **GtkTable → GtkGrid**: This is the biggest change
  - Use `gtk_grid_new()` instead of `gtk_table_new()`
  - Use `gtk_grid_attach()` with column, row, width, height
  - Use `gtk_widget_set_hexpand()` / `gtk_widget_set_vexpand()` for expansion
  - Use `gtk_widget_set_halign()` / `gtk_widget_set_valign()` for alignment
- GtkSpinButton: API unchanged
- GtkEntry: API unchanged

**Day 1 Testing Protocol**:

```c
/* test_dialogs.c - Test program for dialogs */

#include <gtk/gtk.h>
#include "gui-dialog.h"

static void
test_file_dialogs(void)
{
  gchar *filename;

  /* Test file open */
  filename = ghid3_dialog_file_select_open("Open PCB File", NULL, NULL);
  if (filename)
    {
      g_print("Selected file: %s\n", filename);
      g_free(filename);
    }

  /* Test file save */
  filename = ghid3_dialog_file_select_save("Save PCB File", NULL, "untitled.pcb");
  if (filename)
    {
      g_print("Save to file: %s\n", filename);
      g_free(filename);
    }
}

static void
test_message_dialogs(void)
{
  ghid3_dialog_message("Information", "This is an information message.");
  ghid3_dialog_warning("Warning", "This is a warning message.");
  ghid3_dialog_error("Error", "This is an error message.");

  if (ghid3_dialog_confirm("Confirm", "Do you want to continue?"))
    g_print("User confirmed\n");
  else
    g_print("User declined\n");
}

static void
test_input_dialogs(void)
{
  gchar *text;
  gdouble number;

  text = ghid3_dialog_input("Text Input", "Enter your name:", "John Doe");
  if (text)
    {
      g_print("Entered: %s\n", text);
      g_free(text);
    }

  number = ghid3_dialog_input_number("Number Input", "Enter a value:",
                                    10.0, 0.0, 100.0);
  g_print("Entered number: %.2f\n", number);
}

int
main(int argc, char **argv)
{
  gtk_init(&argc, &argv);

  /* Create dummy top window for dialog parenting */
  ghid3_port.top_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  test_file_dialogs();
  test_message_dialogs();
  test_input_dialogs();

  gtk_widget_destroy(ghid3_port.top_window);

  return 0;
}
```

Compile and run:
```bash
gcc -o test_dialogs test_dialogs.c src/hid/gtk3/gui-dialog.c \
    `pkg-config --cflags --libs gtk+-3.0` -I src/hid/gtk3
./test_dialogs
```

**Expected results after Day 1**:
- ✓ File open dialog works (can browse and select files)
- ✓ File save dialog works (can specify filename, confirms overwrite)
- ✓ Info/warning/error dialogs display with correct icons
- ✓ Confirmation dialogs return correct response
- ✓ Text input dialog accepts and returns text
- ✓ Number input dialog accepts numeric values with spin button
- ✓ All dialogs are modal and properly centered on parent window
- ✓ All dialogs respect theme (colors, fonts)

---

## Day 2: Main Menu System (8 hours)

### Day 2 Overview

Migrate `ghid-main-menu.c` from GTK2 to GTK3. This file defines the main menu bar (File, Edit, View, Settings, etc.) and all menu items, including:
- Menu structure (menu bar, submenus)
- Menu item actions (callbacks)
- Keyboard shortcuts (accelerators)
- Toggle and radio menu items
- Recent files menu

**Strategy**: Copy from GTK2, rename functions, update deprecated APIs, update keyboard shortcuts, test each menu.

### Hour 1-2: File Setup and Menu Structure

**File**: `src/hid/gtk3/ghid-main-menu.c` (copy from `src/hid/gtk/ghid-main-menu.c`)

#### Step 1: Copy and rename

```bash
cp src/hid/gtk/ghid-main-menu.c src/hid/gtk3/ghid-main-menu.c
cp src/hid/gtk/ghid-main-menu.h src/hid/gtk3/ghid-main-menu.h

# Rename functions
sed -i 's/ghid_/ghid3_/g' src/hid/gtk3/ghid-main-menu.c
sed -i 's/ghid_/ghid3_/g' src/hid/gtk3/ghid-main-menu.h
```

#### Step 2: Update menu creation approach

GTK3 has two approaches for menus:
1. **GtkUIManager** (deprecated in GTK 3.10, but still works)
2. **GtkBuilder** with XML (modern approach)

For faster migration, we'll use GtkUIManager initially (same as GTK2), then optionally migrate to GtkBuilder later.

```c
/* ghid-main-menu.c - Main menu creation */

#include "gtkhid.h"
#include "gui.h"
#include "ghid-main-menu.h"

#include <gtk/gtk.h>

/* Suppress GtkUIManager deprecation warnings for now */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static GtkUIManager *ui_manager = NULL;
static GtkActionGroup *action_group = NULL;

/* Menu item callback signatures */
/* GTK2 and GTK3: Same signature for GtkAction callbacks */
static void file_open_cb(GtkAction *action, gpointer user_data);
static void file_save_cb(GtkAction *action, gpointer user_data);
static void file_save_as_cb(GtkAction *action, gpointer user_data);
static void file_quit_cb(GtkAction *action, gpointer user_data);
/* ... more callbacks ... */

/* Define menu actions */
static const GtkActionEntry file_menu_actions[] = {
  /* name, stock_id, label, accelerator, tooltip, callback */
  {"FileMenu", NULL, "_File", NULL, NULL, NULL},

  {"FileNew", NULL, "_New", "<Control>n",
   "Create new PCB", G_CALLBACK(file_new_cb)},

  {"FileOpen", NULL, "_Open...", "<Control>o",
   "Open existing PCB", G_CALLBACK(file_open_cb)},

  {"FileSave", NULL, "_Save", "<Control>s",
   "Save PCB", G_CALLBACK(file_save_cb)},

  {"FileSaveAs", NULL, "Save _As...", "<Shift><Control>s",
   "Save PCB with new name", G_CALLBACK(file_save_as_cb)},

  /* Separator (use NULL callback) */
  {"FileSep1", NULL, NULL, NULL, NULL, NULL},

  {"FileQuit", NULL, "_Quit", "<Control>q",
   "Quit PCB", G_CALLBACK(file_quit_cb)},
};

/* Toggle actions (checkboxes) */
static const GtkToggleActionEntry view_toggle_actions[] = {
  {"ViewGrid", NULL, "Show _Grid", NULL,
   "Toggle grid visibility", G_CALLBACK(view_grid_toggle_cb), TRUE},

  {"ViewCrosshair", NULL, "Show _Crosshair", NULL,
   "Toggle crosshair cursor", G_CALLBACK(view_crosshair_toggle_cb), TRUE},
};

/* Create main menu */
GtkWidget *
ghid3_main_menu_create(GtkWidget *parent_window)
{
  GError *error = NULL;

  /* Create UI manager */
  ui_manager = gtk_ui_manager_new();

  /* Create action group */
  action_group = gtk_action_group_new("MainActions");

  /* Add actions to action group */
  gtk_action_group_add_actions(action_group,
                               file_menu_actions,
                               G_N_ELEMENTS(file_menu_actions),
                               NULL);

  gtk_action_group_add_toggle_actions(action_group,
                                     view_toggle_actions,
                                     G_N_ELEMENTS(view_toggle_actions),
                                     NULL);

  /* Add action group to UI manager */
  gtk_ui_manager_insert_action_group(ui_manager, action_group, 0);

  /* Add accelerator group to window */
  GtkAccelGroup *accel_group = gtk_ui_manager_get_accel_group(ui_manager);
  gtk_window_add_accel_group(GTK_WINDOW(parent_window), accel_group);

  /* Define UI layout (XML) */
  const gchar *ui_description =
    "<ui>"
    "  <menubar name='MainMenu'>"
    "    <menu action='FileMenu'>"
    "      <menuitem action='FileNew'/>"
    "      <menuitem action='FileOpen'/>"
    "      <menuitem action='FileSave'/>"
    "      <menuitem action='FileSaveAs'/>"
    "      <separator/>"
    "      <menuitem action='FileQuit'/>"
    "    </menu>"
    "    <menu action='ViewMenu'>"
    "      <menuitem action='ViewGrid'/>"
    "      <menuitem action='ViewCrosshair'/>"
    "    </menu>"
    "  </menubar>"
    "</ui>";

  /* Load UI from XML */
  if (!gtk_ui_manager_add_ui_from_string(ui_manager, ui_description, -1, &error))
    {
      g_warning("Failed to build menus: %s", error->message);
      g_error_free(error);
      return NULL;
    }

  /* Get menu bar widget */
  GtkWidget *menu_bar = gtk_ui_manager_get_widget(ui_manager, "/MainMenu");

  return menu_bar;
}

G_GNUC_END_IGNORE_DEPRECATIONS
```

**Key GTK3 Changes**:
- GtkUIManager: Deprecated but still works (marked with `G_GNUC_BEGIN/END_IGNORE_DEPRECATIONS`)
- Stock items: Removed (use `NULL` for stock_id, provide custom labels)
- Accelerators: Format unchanged (`<Control>s`, `<Shift><Alt>f`, etc.)
- **Note**: For GTK3.10+, consider migrating to GtkBuilder, but GtkUIManager is simpler for initial port

**Testing checkpoint**: Verify menu bar appears in window. Check that File and View menus open.

### Hour 3-4: Keyboard Shortcuts (Accelerator Updates)

Update keyboard shortcut constants from GTK2 to GTK3 format.

**GTK2 format**: `GDK_Escape`, `GDK_Return`, `GDK_space`
**GTK3 format**: `GDK_KEY_Escape`, `GDK_KEY_Return`, `GDK_KEY_space`

```c
/* Update keyboard event handlers */

static gboolean
ghid3_key_press_cb(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  /* GTK2 used GDK_* constants */
  /* GTK3 uses GDK_KEY_* constants */

  switch (event->keyval)
    {
    case GDK_KEY_Escape:  /* Was GDK_Escape in GTK2 */
      /* Cancel current operation */
      return TRUE;

    case GDK_KEY_Return:  /* Was GDK_Return in GTK2 */
    case GDK_KEY_KP_Enter:
      /* Finish current operation */
      return TRUE;

    case GDK_KEY_space:  /* Was GDK_space in GTK2 */
      /* Toggle mode */
      return TRUE;

    case GDK_KEY_Delete:  /* Was GDK_Delete in GTK2 */
    case GDK_KEY_BackSpace:
      /* Delete selected object */
      return TRUE;

    case GDK_KEY_Tab:  /* Was GDK_Tab in GTK2 */
      /* Cycle through objects */
      return TRUE;

    /* Arrow keys */
    case GDK_KEY_Left:
    case GDK_KEY_Right:
    case GDK_KEY_Up:
    case GDK_KEY_Down:
      /* Move cursor */
      return TRUE;

    /* Function keys */
    case GDK_KEY_F1:
    case GDK_KEY_F2:
    case GDK_KEY_F3:
      /* Function key actions */
      return TRUE;
    }

  return FALSE;  /* Not handled, propagate event */
}
```

**Automated conversion**:
```bash
# Use sed to convert GDK_ constants to GDK_KEY_* in key event handlers
sed -i 's/GDK_Escape/GDK_KEY_Escape/g' src/hid/gtk3/ghid-main-menu.c
sed -i 's/GDK_Return/GDK_KEY_Return/g' src/hid/gtk3/ghid-main-menu.c
sed -i 's/GDK_space/GDK_KEY_space/g' src/hid/gtk3/ghid-main-menu.c
sed -i 's/GDK_Delete/GDK_KEY_Delete/g' src/hid/gtk3/ghid-main-menu.c
sed -i 's/GDK_BackSpace/GDK_KEY_BackSpace/g' src/hid/gtk3/ghid-main-menu.c
sed -i 's/GDK_Tab/GDK_KEY_Tab/g' src/hid/gtk3/ghid-main-menu.c
# ... repeat for all GDK_ key constants ...

# Or use a comprehensive script:
for key in Escape Return space Delete BackSpace Tab \
           Left Right Up Down \
           F1 F2 F3 F4 F5 F6 F7 F8 F9 F10 F11 F12 \
           a b c d e f g h i j k l m n o p q r s t u v w x y z \
           A B C D E F G H I J K L M N O P Q R S T U V W X Y Z \
           0 1 2 3 4 5 6 7 8 9; do
  sed -i "s/GDK_${key}\b/GDK_KEY_${key}/g" src/hid/gtk3/ghid-main-menu.c
done
```

**Testing checkpoint**: Test keyboard shortcuts. Press Ctrl+O (should open file dialog), Ctrl+Q (should quit), etc.

### Hour 5-6: Menu Item Callbacks

Implement menu item callback functions.

```c
/* File menu callbacks */

static void
file_new_cb(GtkAction *action, gpointer user_data)
{
  /* Confirm if current file has unsaved changes */
  if (ghid3_has_unsaved_changes())
    {
      if (!ghid3_dialog_confirm("Unsaved Changes",
                               "Current PCB has unsaved changes. Create new PCB anyway?"))
        return;
    }

  /* Call PCB core function to create new board */
  /* This depends on PCB core API, not GTK-specific */
  hid_action("New");
}

static void
file_open_cb(GtkAction *action, gpointer user_data)
{
  gchar *filename;

  /* Show file open dialog */
  filename = ghid3_dialog_file_select_open("Open PCB File",
                                          ghid3_get_last_directory(),
                                          NULL);

  if (filename == NULL)
    return;  /* User cancelled */

  /* Load PCB file */
  if (!ghid3_load_pcb(filename))
    {
      ghid3_dialog_error("Error", "Failed to load PCB file.");
    }

  g_free(filename);
}

static void
file_save_cb(GtkAction *action, gpointer user_data)
{
  /* If file has name, save directly */
  if (PCB->Filename != NULL)
    {
      if (!ghid3_save_pcb(PCB->Filename))
        ghid3_dialog_error("Error", "Failed to save PCB file.");
    }
  else
    {
      /* No filename, show save as dialog */
      file_save_as_cb(action, user_data);
    }
}

static void
file_save_as_cb(GtkAction *action, gpointer user_data)
{
  gchar *filename;

  filename = ghid3_dialog_file_select_save("Save PCB File As",
                                          ghid3_get_last_directory(),
                                          PCB->Name);

  if (filename == NULL)
    return;

  /* Ensure .pcb extension */
  if (!g_str_has_suffix(filename, ".pcb"))
    {
      gchar *tmp = g_strdup_printf("%s.pcb", filename);
      g_free(filename);
      filename = tmp;
    }

  if (!ghid3_save_pcb(filename))
    ghid3_dialog_error("Error", "Failed to save PCB file.");

  g_free(filename);
}

static void
file_quit_cb(GtkAction *action, gpointer user_data)
{
  /* Confirm if unsaved changes */
  if (ghid3_has_unsaved_changes())
    {
      gint response = ghid3_dialog_confirm_all("Unsaved Changes",
                                              "Save changes before quitting?");

      if (response == GTK_RESPONSE_CANCEL)
        return;  /* Don't quit */

      if (response == GTK_RESPONSE_YES)
        file_save_cb(action, user_data);  /* Save then quit */
    }

  /* Quit application */
  gtk_main_quit();
}

/* View menu callbacks */

static void
view_grid_toggle_cb(GtkToggleAction *action, gpointer user_data)
{
  gboolean active = gtk_toggle_action_get_active(action);

  /* Toggle grid visibility */
  ghid3_set_grid_visible(active);
  ghid3_invalidate_all();  /* Redraw canvas */
}

static void
view_crosshair_toggle_cb(GtkToggleAction *action, gpointer user_data)
{
  gboolean active = gtk_toggle_action_get_active(action);

  /* Toggle crosshair cursor */
  ghid3_set_crosshair_visible(active);
  ghid3_invalidate_all();
}

/* Edit menu callbacks */

static void
edit_undo_cb(GtkAction *action, gpointer user_data)
{
  hid_action("Undo");
  ghid3_invalidate_all();
}

static void
edit_redo_cb(GtkAction *action, gpointer user_data)
{
  hid_action("Redo");
  ghid3_invalidate_all();
}

static void
edit_cut_cb(GtkAction *action, gpointer user_data)
{
  hid_action("Cut");
  ghid3_invalidate_all();
}

static void
edit_copy_cb(GtkAction *action, gpointer user_data)
{
  hid_action("Copy");
}

static void
edit_paste_cb(GtkAction *action, gpointer user_data)
{
  hid_action("Paste");
  ghid3_invalidate_all();
}
```

**Key GTK3 Changes**:
- Callback signatures: Unchanged from GTK2
- `GtkAction` vs `GtkToggleAction`: Still supported (part of GtkUIManager)
- Dialog calls: Use GTK3 dialog functions created in Day 1

**Testing checkpoint**: Test each menu item callback. Verify File→Open shows dialog, File→Quit confirms exit, View→Grid toggles grid, etc.

### Hour 7-8: Recent Files Menu and Testing

Implement recent files submenu using GtkRecentManager (GTK3 built-in).

```c
/* Recent files management */

#define MAX_RECENT_FILES 10

static GtkRecentManager *recent_manager = NULL;
static GtkWidget *recent_menu = NULL;

/* Initialize recent files manager */
void
ghid3_recent_files_init(void)
{
  /* Get default recent manager (GTK3 provides this) */
  recent_manager = gtk_recent_manager_get_default();
}

/* Add file to recent files list */
void
ghid3_recent_files_add(const gchar *filename)
{
  gchar *uri;

  if (recent_manager == NULL)
    return;

  /* Convert filename to URI */
  uri = g_filename_to_uri(filename, NULL, NULL);
  if (uri == NULL)
    return;

  /* Add to recent manager */
  gtk_recent_manager_add_item(recent_manager, uri);

  g_free(uri);

  /* Update recent files menu */
  ghid3_recent_files_menu_update();
}

/* Callback for recent file selection */
static void
recent_file_activated_cb(GtkRecentChooser *chooser, gpointer user_data)
{
  gchar *uri;
  gchar *filename;

  uri = gtk_recent_chooser_get_current_uri(chooser);
  if (uri == NULL)
    return;

  filename = g_filename_from_uri(uri, NULL, NULL);
  g_free(uri);

  if (filename == NULL)
    return;

  /* Load the file */
  if (!ghid3_load_pcb(filename))
    {
      ghid3_dialog_error("Error", "Failed to load PCB file.");
    }

  g_free(filename);
}

/* Create recent files submenu */
GtkWidget *
ghid3_recent_files_menu_create(void)
{
  GtkWidget *menu_item;
  GtkWidget *menu;
  GtkRecentFilter *filter;

  /* Create "Open Recent" menu item */
  menu_item = gtk_menu_item_new_with_mnemonic("Open _Recent");

  /* Create recent chooser menu */
  menu = gtk_recent_chooser_menu_new_for_manager(recent_manager);
  gtk_recent_chooser_set_show_not_found(GTK_RECENT_CHOOSER(menu), FALSE);
  gtk_recent_chooser_set_sort_type(GTK_RECENT_CHOOSER(menu),
                                   GTK_RECENT_SORT_MRU);  /* Most recently used */
  gtk_recent_chooser_set_limit(GTK_RECENT_CHOOSER(menu), MAX_RECENT_FILES);

  /* Create filter for PCB files only */
  filter = gtk_recent_filter_new();
  gtk_recent_filter_add_pattern(filter, "*.pcb");
  gtk_recent_chooser_add_filter(GTK_RECENT_CHOOSER(menu), filter);

  /* Connect activation signal */
  g_signal_connect(menu, "item-activated",
                  G_CALLBACK(recent_file_activated_cb), NULL);

  /* Attach submenu */
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

  recent_menu = menu;

  return menu_item;
}

/* Update recent files menu */
void
ghid3_recent_files_menu_update(void)
{
  /* GTK3 GtkRecentChooserMenu updates automatically */
  /* No manual update needed (unlike GTK2) */
}
```

**Complete menu structure**:

```c
/* Complete menu definition with all submenus */

static const GtkActionEntry menu_actions[] = {
  /* File menu */
  {"FileMenu", NULL, "_File"},
  {"FileNew", NULL, "_New", "<Control>n", "Create new PCB", G_CALLBACK(file_new_cb)},
  {"FileOpen", NULL, "_Open...", "<Control>o", "Open PCB", G_CALLBACK(file_open_cb)},
  {"FileSave", NULL, "_Save", "<Control>s", "Save PCB", G_CALLBACK(file_save_cb)},
  {"FileSaveAs", NULL, "Save _As...", "<Shift><Control>s", "Save PCB as", G_CALLBACK(file_save_as_cb)},
  {"FileQuit", NULL, "_Quit", "<Control>q", "Quit", G_CALLBACK(file_quit_cb)},

  /* Edit menu */
  {"EditMenu", NULL, "_Edit"},
  {"EditUndo", NULL, "_Undo", "<Control>z", "Undo", G_CALLBACK(edit_undo_cb)},
  {"EditRedo", NULL, "_Redo", "<Shift><Control>z", "Redo", G_CALLBACK(edit_redo_cb)},
  {"EditCut", NULL, "Cu_t", "<Control>x", "Cut", G_CALLBACK(edit_cut_cb)},
  {"EditCopy", NULL, "_Copy", "<Control>c", "Copy", G_CALLBACK(edit_copy_cb)},
  {"EditPaste", NULL, "_Paste", "<Control>v", "Paste", G_CALLBACK(edit_paste_cb)},

  /* View menu */
  {"ViewMenu", NULL, "_View"},
  {"ViewZoomIn", NULL, "Zoom _In", "<Control>plus", "Zoom in", G_CALLBACK(view_zoom_in_cb)},
  {"ViewZoomOut", NULL, "Zoom _Out", "<Control>minus", "Zoom out", G_CALLBACK(view_zoom_out_cb)},
  {"ViewZoomFit", NULL, "Zoom to _Fit", "<Control>0", "Fit board in window", G_CALLBACK(view_zoom_fit_cb)},

  /* Settings menu */
  {"SettingsMenu", NULL, "_Settings"},
  {"SettingsPreferences", NULL, "_Preferences...", NULL, "Edit preferences", G_CALLBACK(settings_prefs_cb)},
};

static const GtkToggleActionEntry toggle_actions[] = {
  {"ViewGrid", NULL, "Show _Grid", NULL, "Toggle grid", G_CALLBACK(view_grid_toggle_cb), TRUE},
  {"ViewCrosshair", NULL, "Show _Crosshair", NULL, "Toggle crosshair", G_CALLBACK(view_crosshair_toggle_cb), TRUE},
};

static const gchar *ui_description =
  "<ui>"
  "  <menubar name='MainMenu'>"
  "    <menu action='FileMenu'>"
  "      <menuitem action='FileNew'/>"
  "      <menuitem action='FileOpen'/>"
  "      <separator/>"
  "      <menuitem action='FileSave'/>"
  "      <menuitem action='FileSaveAs'/>"
  "      <separator/>"
  "      <menuitem action='FileQuit'/>"
  "    </menu>"
  "    <menu action='EditMenu'>"
  "      <menuitem action='EditUndo'/>"
  "      <menuitem action='EditRedo'/>"
  "      <separator/>"
  "      <menuitem action='EditCut'/>"
  "      <menuitem action='EditCopy'/>"
  "      <menuitem action='EditPaste'/>"
  "    </menu>"
  "    <menu action='ViewMenu'>"
  "      <menuitem action='ViewZoomIn'/>"
  "      <menuitem action='ViewZoomOut'/>"
  "      <menuitem action='ViewZoomFit'/>"
  "      <separator/>"
  "      <menuitem action='ViewGrid'/>"
  "      <menuitem action='ViewCrosshair'/>"
  "    </menu>"
  "    <menu action='SettingsMenu'>"
  "      <menuitem action='SettingsPreferences'/>"
  "    </menu>"
  "  </menubar>"
  "</ui>";
```

**Day 2 Testing Protocol**:

1. **Visual inspection**:
   - Menu bar displays at top of window
   - All menus open when clicked
   - Menu items have correct labels
   - Keyboard shortcuts shown next to menu items
   - Checkboxes appear for toggle items

2. **Functional testing**:
   - File→New creates new PCB
   - File→Open shows file dialog
   - File→Save saves (or shows save as if no filename)
   - File→Save As shows save dialog
   - File→Quit confirms and exits
   - Edit→Undo/Redo work
   - Edit→Cut/Copy/Paste work
   - View→Zoom In/Out/Fit work
   - View→Grid toggles grid visibility
   - Settings→Preferences opens preferences dialog

3. **Keyboard shortcut testing**:
   - Ctrl+N (new)
   - Ctrl+O (open)
   - Ctrl+S (save)
   - Ctrl+Shift+S (save as)
   - Ctrl+Q (quit)
   - Ctrl+Z (undo)
   - Ctrl+Shift+Z (redo)
   - Ctrl+X/C/V (cut/copy/paste)
   - Ctrl++/- (zoom in/out)

4. **Recent files testing**:
   - Open a file, verify it appears in File→Open Recent
   - Select file from recent list, verify it loads
   - Verify recent list limited to 10 files

**Expected results after Day 2**:
- ✓ Menu bar displays correctly
- ✓ All menu items functional
- ✓ Keyboard shortcuts work
- ✓ Toggle menu items work (checkboxes)
- ✓ Recent files menu updates automatically
- ✓ Menus respect theme
- ✓ No crashes when using menus

---

## Integration Testing

After completing both days, perform integration testing:

```bash
# Build GTK3 HID
cd /path/to/pcb
./autogen.sh
./configure --enable-gtk3
make

# Run GTK3 HID
./src/pcb --hid gtk3

# Test workflow:
# 1. File→New (creates new PCB)
# 2. Draw some traces
# 3. File→Save As (save to test.pcb)
# 4. File→Open (open test.pcb)
# 5. Edit→Undo (undo last action)
# 6. View→Grid (toggle grid)
# 7. File→Quit (confirm and exit)
```

**Regression testing**:
```bash
# Compare with GTK2 HID
./src/pcb --hid gtk  # GTK2 version

# Visual comparison:
# - Do menus look similar?
# - Do dialogs look similar?
# - Do keyboard shortcuts work the same?
```

---

## Common Issues and Solutions

### Issue 1: Menu items don't appear

**Symptom**: Menu bar shows but menus are empty

**Diagnosis**: UI XML not loaded correctly

**Solution**:
```c
/* Check for errors when loading UI */
GError *error = NULL;
if (!gtk_ui_manager_add_ui_from_string(ui_manager, ui_description, -1, &error))
  {
    g_warning("Failed to build menus: %s", error->message);
    g_error_free(error);
  }

/* Verify action names in XML match action group */
/* Check for typos in action names */
```

### Issue 2: Keyboard shortcuts don't work

**Symptom**: Menu items work when clicked, but Ctrl+S, etc. don't work

**Diagnosis**: Accelerator group not attached to window

**Solution**:
```c
/* Ensure accelerator group is added to window */
GtkAccelGroup *accel_group = gtk_ui_manager_get_accel_group(ui_manager);
gtk_window_add_accel_group(GTK_WINDOW(top_window), accel_group);

/* Check accelerator strings are correct: "<Control>s" not "Ctrl+S" */
```

### Issue 3: Recent files menu empty

**Symptom**: File→Open Recent is empty even after opening files

**Diagnosis**: Files not being added to recent manager

**Solution**:
```c
/* Ensure ghid3_recent_files_add() is called after loading file */
void
ghid3_load_pcb(const gchar *filename)
{
  /* Load file... */

  /* Add to recent files */
  ghid3_recent_files_add(filename);
}

/* Verify URI conversion works */
gchar *uri = g_filename_to_uri(filename, NULL, NULL);
g_assert(uri != NULL);
```

### Issue 4: Dialogs not modal

**Symptom**: Can interact with main window while dialog is open

**Diagnosis**: Dialog not set as modal or not properly parented

**Solution**:
```c
/* Ensure dialog is modal and parented */
dialog = gtk_dialog_new_with_buttons(
  title,
  GTK_WINDOW(ghid3_port.top_window),  /* Must have parent */
  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,  /* Must be modal */
  ...);

/* Verify ghid3_port.top_window is set */
g_assert(ghid3_port.top_window != NULL);
```

### Issue 5: GtkUIManager deprecation warnings

**Symptom**: Compiler warnings about deprecated GtkUIManager

**Solution**:
```c
/* Suppress warnings (acceptable for initial port) */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
/* GtkUIManager code here */
G_GNUC_END_IGNORE_DEPRECATIONS

/* Or migrate to GtkBuilder (more work): */
GtkBuilder *builder = gtk_builder_new();
gtk_builder_add_from_file(builder, "menu.ui", NULL);
```

---

## Success Criteria

Milestone 4A is complete when:

- [ ] All dialog functions work (file, message, input)
- [ ] File open/save dialogs display correctly
- [ ] Message dialogs (info, warning, error, confirm) work
- [ ] Input dialogs accept text and numeric input
- [ ] Menu bar displays with all menus
- [ ] All menu items are functional
- [ ] Keyboard shortcuts work correctly
- [ ] Toggle menu items show checkboxes and toggle state
- [ ] Recent files menu displays and works
- [ ] No crashes when using dialogs or menus
- [ ] Visual parity with GTK2 HID (looks similar or better)
- [ ] No memory leaks detected with valgrind

---

## What You Can Do After Completing Milestone 4A

After successfully completing this milestone:

### Dialog Functionality
- ✓ Open PCB files using File→Open dialog
- ✓ Save PCB files using File→Save As dialog
- ✓ See informational, warning, and error messages
- ✓ Confirm actions before proceeding (e.g., quit with unsaved changes)
- ✓ Enter text and numeric values in dialogs

### Menu Functionality
- ✓ Access all PCB functions through menus
- ✓ Use keyboard shortcuts for common operations (Ctrl+S, Ctrl+O, etc.)
- ✓ Toggle settings with checkboxes (grid visibility, crosshair, etc.)
- ✓ Access recently opened files quickly

### User Experience
- ✓ Professional-looking interface with standard GTK3 theming
- ✓ Efficient workflows with keyboard shortcuts
- ✓ Clear feedback through message dialogs
- ✓ Familiar GTK3 behavior (file choosers, etc.)

---

## Estimated Completion Time

**Total**: 16 hours (2 days)

- Day 1 (8 hours): Core dialog infrastructure
  - Hour 1-2: File setup and function renaming
  - Hour 3-4: File chooser dialogs
  - Hour 5-6: Message dialogs
  - Hour 7-8: Input dialogs and testing

- Day 2 (8 hours): Main menu system
  - Hour 1-2: File setup and menu structure
  - Hour 3-4: Keyboard shortcuts
  - Hour 5-6: Menu item callbacks
  - Hour 7-8: Recent files menu and testing

**Dependencies**: Milestone 1, 2, and 3 must be complete

**Next Milestone**: Milestone 4B - Custom Widgets (ghid-layer-selector.c, ghid-route-style-selector.c)

---

## Notes

- **Parallel development**: All code in `src/hid/gtk3/`, use `ghid3_` prefix
- **Test incrementally**: Test each dialog and menu as you implement it
- **Visual comparison**: Keep GTK2 HID open for reference
- **GtkUIManager**: Using deprecated API is acceptable for initial port; can migrate to GtkBuilder later
- **Keyboard shortcuts**: Test all shortcuts to ensure they work correctly
- **Platform testing**: Test on Linux primarily; macOS/Windows if available

This milestone provides the core UI infrastructure needed for the rest of Milestone 4!
