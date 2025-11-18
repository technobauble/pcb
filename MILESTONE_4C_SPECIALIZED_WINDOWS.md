# Milestone 4C: Specialized Windows - Detailed Implementation Plan

**Duration**: 2 days (16 hours)
**Days**: Day 5-6 of Milestone 4
**Prerequisites**: Milestones 1, 2, 3, 4A, and 4B completed
**Goal**: Migrate specialized tree-view based windows (library browser, netlist window, DRC window) to GTK3

---

## Overview

Milestone 4C migrates three specialized windows that provide complex data visualization and editing capabilities:

1. **`gui-library-window.c`** (1,200 lines, 8 hours): Component library browser
   - Browse footprint library hierarchically
   - Search for components
   - Preview footprint before placement
   - Add components to PCB

2. **`gui-netlist-window.c`** (1,300 lines, 6 hours): Netlist viewer/editor
   - Display nets and connections
   - Highlight nets on PCB
   - Edit net names
   - Find/optimize rats

3. **`gui-drc-window.c`** (1,000 lines, 4 hours): Design Rule Check results
   - List DRC violations
   - Jump to violation location
   - Filter by violation type
   - Export violations report

All three windows use **GtkTreeView** for data display, making migration similar. Main changes: layout updates (GtkTable → GtkGrid), tree view cell renderer updates, and window management.

---

## Day 1: Library Window & Netlist Window (8 hours)

### Hour 1-3: Library Window Migration

**File**: `src/hid/gtk3/gui-library-window.c`

```bash
# Copy and rename
cp src/hid/gtk/gui-library-window.c src/hid/gtk3/gui-library-window.c
sed -i 's/ghid_/ghid3_/g' src/hid/gtk3/gui-library-window.c
```

**Key changes**:

```c
/* Create library window - GTK3 */
GtkWidget *
ghid3_library_window_create(void)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *scrolled;
  GtkWidget *tree_view;
  GtkWidget *search_entry;
  GtkWidget *button_box;
  GtkTreeStore *tree_store;

  /* Create window */
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "Library");
  gtk_window_set_default_size(GTK_WINDOW(window), 400, 500);
  gtk_window_set_transient_for(GTK_WINDOW(window),
                               GTK_WINDOW(ghid3_port.top_window));

  /* Create vertical layout */
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  /* Search entry at top */
  search_entry = gtk_search_entry_new();  /* GTK3: dedicated search entry widget */
  gtk_entry_set_placeholder_text(GTK_ENTRY(search_entry), "Search components...");
  g_signal_connect(search_entry, "search-changed",
                  G_CALLBACK(ghid3_library_search_changed_cb), tree_view);
  gtk_box_pack_start(GTK_BOX(vbox), search_entry, FALSE, FALSE, 0);

  /* Tree view in scrolled window */
  scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                GTK_POLICY_AUTOMATIC,
                                GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled),
                                     GTK_SHADOW_IN);
  gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

  /* Create tree store for hierarchical library structure */
  tree_store = gtk_tree_store_new(3,
                                 G_TYPE_STRING,   /* Component name */
                                 G_TYPE_STRING,   /* Description */
                                 G_TYPE_POINTER); /* Element pointer */

  tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(tree_store));
  g_object_unref(tree_store);

  /* Configure columns */
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Name",
                                                    renderer,
                                                    "text", 0,
                                                    NULL);
  gtk_tree_view_column_set_expand(column, TRUE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Description",
                                                    renderer,
                                                    "text", 1,
                                                    NULL);
  gtk_tree_view_column_set_expand(column, TRUE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

  /* Double-click to add component */
  g_signal_connect(tree_view, "row-activated",
                  G_CALLBACK(ghid3_library_row_activated_cb), NULL);

  gtk_container_add(GTK_CONTAINER(scrolled), tree_view);

  /* Button box at bottom */
  button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
  gtk_box_set_spacing(GTK_BOX(button_box), 6);
  gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 0);

  GtkWidget *add_button = gtk_button_new_with_label("Add to PCB");
  g_signal_connect(add_button, "clicked",
                  G_CALLBACK(ghid3_library_add_clicked_cb), tree_view);
  gtk_container_add(GTK_CONTAINER(button_box), add_button);

  GtkWidget *close_button = gtk_button_new_with_label("Close");
  g_signal_connect_swapped(close_button, "clicked",
                           G_CALLBACK(gtk_widget_hide), window);
  gtk_container_add(GTK_CONTAINER(button_box), close_button);

  /* Populate library tree */
  ghid3_library_window_populate(tree_store);

  return window;
}

/* Populate library from PCB library data */
static void
ghid3_library_window_populate(GtkTreeStore *store)
{
  GtkTreeIter dir_iter, elem_iter;
  LibraryType *lib;
  int i, j;

  gtk_tree_store_clear(store);

  /* Iterate through library directories */
  for (i = 0; i < Library.MenuN; i++)
    {
      lib = &Library.Menu[i];

      /* Add directory as parent node */
      gtk_tree_store_append(store, &dir_iter, NULL);
      gtk_tree_store_set(store, &dir_iter,
                        0, lib->Name,
                        1, lib->directory,
                        2, NULL,
                        -1);

      /* Add elements in this directory */
      for (j = 0; j < lib->EntryN; j++)
        {
          LibraryEntryType *entry = &lib->Entry[j];

          gtk_tree_store_append(store, &elem_iter, &dir_iter);
          gtk_tree_store_set(store, &elem_iter,
                            0, entry->ListEntry,
                            1, entry->Description,
                            2, entry,
                            -1);
        }
    }
}

/* Search functionality using GtkTreeModelFilter */
static void
ghid3_library_search_changed_cb(GtkSearchEntry *entry, gpointer user_data)
{
  GtkTreeView *tree_view = GTK_TREE_VIEW(user_data);
  const gchar *search_text = gtk_entry_get_text(GTK_ENTRY(entry));

  /* Implement search filtering here */
  /* GTK3: Can use gtk_tree_model_filter_set_visible_func() */
}
```

**GTK3 changes**:
- `GtkSearchEntry`: New widget type in GTK3 for search boxes
- Layout: `gtk_box_new()` instead of `gtk_vbox_new()`
- Tree view: API mostly unchanged from GTK2
- Window transient: Ensures library window stays on top of main window

**Testing**: Open library window, verify tree displays, search works, double-click adds component.

### Hour 4-6: Netlist Window Migration

**File**: `src/hid/gtk3/gui-netlist-window.c`

```c
/* Create netlist window - GTK3 */
GtkWidget *
ghid3_netlist_window_create(void)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *paned;
  GtkWidget *scrolled1, *scrolled2;
  GtkWidget *nets_tree_view;
  GtkWidget *connections_tree_view;
  GtkListStore *nets_store, *connections_store;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "Netlist");
  gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  /* Paned widget to split nets list and connections list */
  paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start(GTK_BOX(vbox), paned, TRUE, TRUE, 0);

  /* Left pane: Nets list */
  scrolled1 = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled1),
                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  nets_store = gtk_list_store_new(2,
                                 G_TYPE_STRING,   /* Net name */
                                 G_TYPE_INT);     /* Connection count */

  nets_tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(nets_store));
  g_object_unref(nets_store);

  /* Columns for nets */
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Net Name",
                                                    renderer,
                                                    "text", 0,
                                                    NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(nets_tree_view), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Pins",
                                                    renderer,
                                                    "text", 1,
                                                    NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(nets_tree_view), column);

  /* Selection changed: show connections for selected net */
  GtkTreeSelection *selection;
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(nets_tree_view));
  g_signal_connect(selection, "changed",
                  G_CALLBACK(ghid3_netlist_selection_changed_cb),
                  connections_tree_view);

  gtk_container_add(GTK_CONTAINER(scrolled1), nets_tree_view);
  gtk_paned_pack1(GTK_PANED(paned), scrolled1, TRUE, TRUE);

  /* Right pane: Connections for selected net */
  scrolled2 = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled2),
                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  connections_store = gtk_list_store_new(2,
                                        G_TYPE_STRING,   /* Component ref */
                                        G_TYPE_STRING);  /* Pin number */

  connections_tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(connections_store));
  g_object_unref(connections_store);

  /* Columns for connections */
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Component",
                                                    renderer,
                                                    "text", 0,
                                                    NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(connections_tree_view), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Pin",
                                                    renderer,
                                                    "text", 1,
                                                    NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(connections_tree_view), column);

  gtk_container_add(GTK_CONTAINER(scrolled2), connections_tree_view);
  gtk_paned_pack2(GTK_PANED(paned), scrolled2, TRUE, TRUE);

  /* Populate nets */
  ghid3_netlist_window_populate(nets_store);

  return window;
}

/* Populate nets from PCB netlist */
static void
ghid3_netlist_window_populate(GtkListStore *store)
{
  GtkTreeIter iter;
  NetListType *netlist;
  int i;

  gtk_list_store_clear(store);

  netlist = PCB->NetlistLib;

  for (i = 0; i < netlist->MenuN; i++)
    {
      NetListMenuType *menu = &netlist->Menu[i];

      gtk_list_store_append(store, &iter);
      gtk_list_store_set(store, &iter,
                        0, menu->Name,
                        1, menu->EntryN,  /* Number of connections */
                        -1);
    }
}
```

**Testing**: Open netlist window, verify nets display, selecting net shows connections.

### Hour 7-8: Testing and Integration

Test both windows thoroughly and integrate with main menu.

```c
/* gui-top-window.c - Add menu items for windows */

static const GtkActionEntry window_menu_actions[] = {
  {"WindowsMenu", NULL, "_Windows"},

  {"WindowsLibrary", NULL, "_Library", NULL,
   "Browse component library", G_CALLBACK(windows_library_cb)},

  {"WindowsNetlist", NULL, "_Netlist", NULL,
   "View netlist connections", G_CALLBACK(windows_netlist_cb)},

  {"WindowsDRC", NULL, "_DRC", NULL,
   "View DRC violations", G_CALLBACK(windows_drc_cb)},
};

static void
windows_library_cb(GtkAction *action, gpointer data)
{
  if (ghid3_port.library_window == NULL)
    ghid3_port.library_window = ghid3_library_window_create();

  gtk_widget_show_all(ghid3_port.library_window);
  gtk_window_present(GTK_WINDOW(ghid3_port.library_window));
}
```

---

## Day 2: DRC Window & Final Polish (8 hours)

### Hour 1-4: DRC Window Migration

**File**: `src/hid/gtk3/gui-drc-window.c`

```c
/* DRC violation types */
enum
{
  DRC_COL_TYPE,        /* Violation type */
  DRC_COL_MESSAGE,     /* Description */
  DRC_COL_X,           /* X coordinate */
  DRC_COL_Y,           /* Y coordinate */
  DRC_COL_POINTER,     /* DRC violation pointer */
  DRC_NUM_COLS
};

/* Create DRC window - GTK3 */
GtkWidget *
ghid3_drc_window_create(void)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *scrolled;
  GtkWidget *tree_view;
  GtkWidget *button_box;
  GtkListStore *store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "DRC Violations");
  gtk_window_set_default_size(GTK_WINDOW(window), 600, 300);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  /* Tree view */
  scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled),
                                     GTK_SHADOW_IN);
  gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

  store = gtk_list_store_new(DRC_NUM_COLS,
                            G_TYPE_STRING,   /* Type */
                            G_TYPE_STRING,   /* Message */
                            G_TYPE_INT,      /* X */
                            G_TYPE_INT,      /* Y */
                            G_TYPE_POINTER); /* Pointer */

  tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  g_object_unref(store);

  /* Columns */
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Type",
                                                    renderer,
                                                    "text", DRC_COL_TYPE,
                                                    NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Description",
                                                    renderer,
                                                    "text", DRC_COL_MESSAGE,
                                                    NULL);
  gtk_tree_view_column_set_expand(column, TRUE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("X",
                                                    renderer,
                                                    "text", DRC_COL_X,
                                                    NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Y",
                                                    renderer,
                                                    "text", DRC_COL_Y,
                                                    NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

  /* Double-click to center on violation */
  g_signal_connect(tree_view, "row-activated",
                  G_CALLBACK(ghid3_drc_row_activated_cb), NULL);

  gtk_container_add(GTK_CONTAINER(scrolled), tree_view);

  /* Buttons */
  button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
  gtk_box_set_spacing(GTK_BOX(button_box), 6);
  gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 0);

  GtkWidget *run_button = gtk_button_new_with_label("Run DRC");
  g_signal_connect(run_button, "clicked",
                  G_CALLBACK(ghid3_drc_run_clicked_cb), store);
  gtk_container_add(GTK_CONTAINER(button_box), run_button);

  GtkWidget *close_button = gtk_button_new_with_label("Close");
  g_signal_connect_swapped(close_button, "clicked",
                           G_CALLBACK(gtk_widget_hide), window);
  gtk_container_add(GTK_CONTAINER(button_box), close_button);

  return window;
}

/* Run DRC and populate results */
static void
ghid3_drc_run_clicked_cb(GtkButton *button, gpointer user_data)
{
  GtkListStore *store = GTK_LIST_STORE(user_data);
  GtkTreeIter iter;

  gtk_list_store_clear(store);

  /* Run DRC check (PCB core function) */
  /* This is a simplified example */
  int violations = DRC_Check();

  if (violations == 0)
    {
      ghid3_dialog_message("DRC", "No violations found.");
      return;
    }

  /* Populate violations (actual implementation would iterate DRC results) */
  /* Example: */
  gtk_list_store_append(store, &iter);
  gtk_list_store_set(store, &iter,
                    DRC_COL_TYPE, "Clearance",
                    DRC_COL_MESSAGE, "Pad too close to trace",
                    DRC_COL_X, 1000,
                    DRC_COL_Y, 2000,
                    DRC_COL_POINTER, NULL,
                    -1);
}
```

**Testing**: Run DRC, verify violations appear, double-click centers view.

### Hour 5-8: Final Integration and Testing

Comprehensive testing of all three windows.

**Integration checklist**:
- [ ] Windows → Library menu item works
- [ ] Windows → Netlist menu item works
- [ ] Windows → DRC menu item works
- [ ] All windows can be opened/closed multiple times
- [ ] Window positions saved/restored
- [ ] Tree views handle large datasets (>1000 rows)
- [ ] Search/filter functionality works
- [ ] Double-click actions work correctly
- [ ] No memory leaks when opening/closing windows

---

## Success Criteria

- [ ] Library window browses components
- [ ] Netlist window displays nets and connections
- [ ] DRC window shows violations and allows navigation
- [ ] All windows accessible from menu
- [ ] Tree views perform well
- [ ] Visual parity with GTK2
- [ ] No crashes or memory leaks

---

## Estimated Time

**Total**: 16 hours (2 days)
- Day 1: Library + Netlist (8 hours)
- Day 2: DRC + Testing (8 hours)

**Next**: Milestone 4D - Configuration dialog and remaining files
