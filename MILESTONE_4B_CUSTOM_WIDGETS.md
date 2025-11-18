# Milestone 4B: Custom Widgets - Detailed Implementation Plan

**Duration**: 2 days (16 hours)
**Days**: Day 3-4 of Milestone 4
**Prerequisites**: Milestones 1, 2, 3, and 4A completed
**Goal**: Migrate PCB-specific custom widgets to GTK3, ensuring layer selector, route style selector, and other custom UI components work correctly with proper Cairo drawing

---

## Overview

Milestone 4B focuses on migrating PCB's custom widgets—specialized UI components that provide PCB-specific functionality beyond standard GTK widgets. These widgets have:

- **Custom drawing code** (using GDK in GTK2, Cairo in GTK3)
- **Complex internal state** (layer visibility, route styles, etc.)
- **Custom signals** (layer-changed, style-selected, etc.)
- **Integration with PCB core** (read/write PCB data structures)

This milestone migrates two major custom widgets:

1. **`ghid-layer-selector.c`** (1,500 lines, 10 hours): Layer visibility and selection widget
   - Tree view with checkboxes for each layer
   - Color swatches showing layer colors
   - Custom drawing for layer preview
   - Layer reordering via drag-and-drop

2. **`ghid-route-style-selector.c`** (1,000 lines, 6 hours): Route style chooser widget
   - Combo box with route style names
   - Custom rendering showing line widths
   - Edit route style parameters
   - Quick access to common routing settings

The migration primarily involves:
- **expose-event → draw signal** (GDK → Cairo)
- **Direct widget access → accessor functions**
- **GdkColor → GdkRGBA** (RGBA color support)
- **Custom cell renderers** for tree views
- **GtkStyleContext** for theming

**Critical Reminder**: Parallel HID development—all code in `src/hid/gtk3/` with `ghid3_` prefix.

---

## Day 1: Layer Selector Widget (8 hours)

### Day 1 Overview

Migrate `ghid-layer-selector.c` to GTK3. This widget is critical for PCB workflow—it allows users to:
- Toggle layer visibility (show/hide layers)
- Select active layer for drawing
- See layer colors at a glance
- Reorder layers via drag-and-drop

**Architecture**:
- GtkTreeView with custom cell renderers
- Columns: checkbox (visible), color swatch, layer name
- Custom drawing for color swatch cells
- Signal emissions when layer visibility/selection changes

### Hour 1-2: File Setup and Basic Widget Structure

**File**: `src/hid/gtk3/ghid-layer-selector.c`

#### Step 1: Copy and rename

```bash
cp src/hid/gtk/ghid-layer-selector.c src/hid/gtk3/ghid-layer-selector.c
cp src/hid/gtk/ghid-layer-selector.h src/hid/gtk3/ghid-layer-selector.h

# Rename functions
sed -i 's/ghid_layer_selector/ghid3_layer_selector/g' src/hid/gtk3/ghid-layer-selector.*
sed -i 's/GHID_LAYER_SELECTOR/GHID3_LAYER_SELECTOR/g' src/hid/gtk3/ghid-layer-selector.*
```

#### Step 2: Update basic widget structure

```c
/* ghid-layer-selector.c - Layer selector custom widget */

#include "config.h"
#include "gtkhid.h"
#include "gui.h"
#include "ghid-layer-selector.h"

#include <gtk/gtk.h>

/* GObject type definition */
#define GHID3_TYPE_LAYER_SELECTOR            (ghid3_layer_selector_get_type())
#define GHID3_LAYER_SELECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GHID3_TYPE_LAYER_SELECTOR, Ghid3LayerSelector))
#define GHID3_LAYER_SELECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GHID3_TYPE_LAYER_SELECTOR, Ghid3LayerSelectorClass))
#define GHID3_IS_LAYER_SELECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GHID3_TYPE_LAYER_SELECTOR))

/* Widget structure */
typedef struct _Ghid3LayerSelector Ghid3LayerSelector;
typedef struct _Ghid3LayerSelectorClass Ghid3LayerSelectorClass;

struct _Ghid3LayerSelector
{
  GtkBox parent;  /* GTK3: Inherit from GtkBox */

  /* Child widgets */
  GtkWidget *tree_view;
  GtkListStore *list_store;

  /* State */
  gint active_layer;
  gboolean callback_blocked;  /* Prevent recursion during updates */
};

struct _Ghid3LayerSelectorClass
{
  GtkBoxClass parent_class;

  /* Signals */
  void (*layer_visibility_changed) (Ghid3LayerSelector *selector, gint layer);
  void (*active_layer_changed) (Ghid3LayerSelector *selector, gint layer);
};

/* Columns in tree view */
enum
{
  COL_VISIBLE,       /* gboolean - checkbox state */
  COL_COLOR,         /* GdkRGBA - layer color */
  COL_NAME,          /* gchar* - layer name */
  COL_LAYER_INDEX,   /* gint - PCB layer index */
  NUM_COLS
};

/* Signals */
enum
{
  LAYER_VISIBILITY_CHANGED,
  ACTIVE_LAYER_CHANGED,
  LAST_SIGNAL
};

static guint ghid3_layer_selector_signals[LAST_SIGNAL] = { 0 };

/* Forward declarations */
static void ghid3_layer_selector_class_init(Ghid3LayerSelectorClass *klass);
static void ghid3_layer_selector_init(Ghid3LayerSelector *selector);

/* Register GObject type */
G_DEFINE_TYPE(Ghid3LayerSelector, ghid3_layer_selector, GTK_TYPE_BOX)

/* Class initialization */
static void
ghid3_layer_selector_class_init(Ghid3LayerSelectorClass *klass)
{
  /* Register signals */
  ghid3_layer_selector_signals[LAYER_VISIBILITY_CHANGED] =
    g_signal_new("layer-visibility-changed",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(Ghid3LayerSelectorClass, layer_visibility_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__INT,
                 G_TYPE_NONE, 1, G_TYPE_INT);

  ghid3_layer_selector_signals[ACTIVE_LAYER_CHANGED] =
    g_signal_new("active-layer-changed",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(Ghid3LayerSelectorClass, active_layer_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__INT,
                 G_TYPE_NONE, 1, G_TYPE_INT);
}

/* Instance initialization */
static void
ghid3_layer_selector_init(Ghid3LayerSelector *selector)
{
  /* GTK3: Set box orientation */
  gtk_orientable_set_orientation(GTK_ORIENTABLE(selector),
                                GTK_ORIENTATION_VERTICAL);

  selector->active_layer = 0;
  selector->callback_blocked = FALSE;
}
```

**Testing checkpoint**: Verify file compiles. Create test program that instantiates widget (even if it's empty).

### Hour 3-4: Tree View and List Store Setup

Create the tree view with columns for visibility, color, and name.

```c
/* Create tree view and list store */
static void
ghid3_layer_selector_create_tree_view(Ghid3LayerSelector *selector)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* Create list store */
  /* Columns: VISIBLE (bool), COLOR (RGBA), NAME (string), LAYER_INDEX (int) */
  selector->list_store = gtk_list_store_new(NUM_COLS,
                                            G_TYPE_BOOLEAN,  /* COL_VISIBLE */
                                            GDK_TYPE_RGBA,   /* COL_COLOR */
                                            G_TYPE_STRING,   /* COL_NAME */
                                            G_TYPE_INT);     /* COL_LAYER_INDEX */

  /* Create tree view */
  selector->tree_view = gtk_tree_view_new_with_model(
    GTK_TREE_MODEL(selector->list_store));

  /* Unref list store (tree view holds reference now) */
  g_object_unref(selector->list_store);

  /* Configure tree view */
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(selector->tree_view), TRUE);
  gtk_tree_view_set_enable_search(GTK_TREE_VIEW(selector->tree_view), FALSE);

  /* Column 1: Visibility checkbox */
  renderer = gtk_cell_renderer_toggle_new();
  g_signal_connect(renderer, "toggled",
                  G_CALLBACK(ghid3_layer_selector_visibility_toggled_cb), selector);

  column = gtk_tree_view_column_new_with_attributes("Vis",
                                                    renderer,
                                                    "active", COL_VISIBLE,
                                                    NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(selector->tree_view), column);

  /* Column 2: Color swatch (custom cell renderer) */
  renderer = ghid3_layer_selector_color_cell_renderer_new();  /* Custom renderer */
  column = gtk_tree_view_column_new_with_attributes("Color",
                                                    renderer,
                                                    "color", COL_COLOR,
                                                    NULL);
  gtk_tree_view_column_set_min_width(column, 40);
  gtk_tree_view_append_column(GTK_TREE_VIEW(selector->tree_view), column);

  /* Column 3: Layer name */
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Layer",
                                                    renderer,
                                                    "text", COL_NAME,
                                                    NULL);
  gtk_tree_view_column_set_expand(column, TRUE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(selector->tree_view), column);

  /* Connect row activation (double-click to select layer) */
  g_signal_connect(selector->tree_view, "row-activated",
                  G_CALLBACK(ghid3_layer_selector_row_activated_cb), selector);

  /* Add tree view to scrolled window */
  GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                GTK_POLICY_NEVER,
                                GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled),
                                     GTK_SHADOW_IN);
  gtk_container_add(GTK_CONTAINER(scrolled), selector->tree_view);

  /* Add scrolled window to selector widget */
  gtk_box_pack_start(GTK_BOX(selector), scrolled, TRUE, TRUE, 0);
}

/* Populate list store with PCB layers */
static void
ghid3_layer_selector_populate(Ghid3LayerSelector *selector)
{
  GtkTreeIter iter;
  gint i;

  /* Clear existing rows */
  gtk_list_store_clear(selector->list_store);

  /* Add each PCB layer */
  for (i = 0; i < max_copper_layer; i++)
    {
      LayerType *layer = &PCB->Data->Layer[i];
      GdkRGBA color;

      /* Convert PCB color to GdkRGBA */
      ghid3_color_from_layer(&color, layer);

      /* Add row to list store */
      gtk_list_store_append(selector->list_store, &iter);
      gtk_list_store_set(selector->list_store, &iter,
                        COL_VISIBLE, layer->On,           /* Visible checkbox */
                        COL_COLOR, &color,                /* Layer color */
                        COL_NAME, layer->Name,            /* Layer name */
                        COL_LAYER_INDEX, i,               /* PCB layer index */
                        -1);
    }
}

/* Helper: Convert PCB layer color to GdkRGBA */
static void
ghid3_color_from_layer(GdkRGBA *rgba, LayerType *layer)
{
  /* PCB stores colors as strings like "#FF0000" or named colors */
  /* Parse to RGBA */
  if (!gdk_rgba_parse(rgba, layer->Color))
    {
      /* Fallback to default color if parse fails */
      rgba->red = 0.8;
      rgba->green = 0.8;
      rgba->blue = 0.8;
      rgba->alpha = 1.0;
    }
}
```

**Key GTK3 Changes**:
- `GdkColor` → `GdkRGBA`: Use `GDK_TYPE_RGBA` in list store, `gdk_rgba_parse()` for parsing
- Tree view API: Mostly unchanged from GTK2
- Cell renderers: Standard renderers unchanged, custom renderer needs Cairo update

**Testing checkpoint**: Populate widget with layers from a test PCB. Verify tree view displays with three columns.

### Hour 5-6: Custom Color Cell Renderer

Create a custom cell renderer to draw color swatches using Cairo.

```c
/* Custom cell renderer for color swatch */

#define GHID3_TYPE_COLOR_CELL_RENDERER            (ghid3_color_cell_renderer_get_type())
#define GHID3_COLOR_CELL_RENDERER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GHID3_TYPE_COLOR_CELL_RENDERER, Ghid3ColorCellRenderer))

typedef struct _Ghid3ColorCellRenderer Ghid3ColorCellRenderer;
typedef struct _Ghid3ColorCellRendererClass Ghid3ColorCellRendererClass;

struct _Ghid3ColorCellRenderer
{
  GtkCellRenderer parent;
  GdkRGBA color;  /* The color to render */
};

struct _Ghid3ColorCellRendererClass
{
  GtkCellRendererClass parent_class;
};

/* Property IDs */
enum
{
  PROP_0,
  PROP_COLOR
};

G_DEFINE_TYPE(Ghid3ColorCellRenderer, ghid3_color_cell_renderer, GTK_TYPE_CELL_RENDERER)

/* Get preferred width (GTK3) */
static void
ghid3_color_cell_renderer_get_preferred_width(GtkCellRenderer *cell,
                                              GtkWidget *widget,
                                              gint *minimum_width,
                                              gint *natural_width)
{
  *minimum_width = 32;
  *natural_width = 32;
}

/* Get preferred height (GTK3) */
static void
ghid3_color_cell_renderer_get_preferred_height(GtkCellRenderer *cell,
                                               GtkWidget *widget,
                                               gint *minimum_height,
                                               gint *natural_height)
{
  *minimum_height = 16;
  *natural_height = 16;
}

/* Render cell using Cairo (GTK3) */
static void
ghid3_color_cell_renderer_render(GtkCellRenderer *cell,
                                 cairo_t *cr,
                                 GtkWidget *widget,
                                 const GdkRectangle *background_area,
                                 const GdkRectangle *cell_area,
                                 GtkCellRendererState flags)
{
  Ghid3ColorCellRenderer *color_cell = GHID3_COLOR_CELL_RENDERER(cell);
  gint x, y, width, height;

  /* Calculate render area (with padding) */
  x = cell_area->x + 2;
  y = cell_area->y + 2;
  width = cell_area->width - 4;
  height = cell_area->height - 4;

  /* Draw color rectangle using Cairo */
  cairo_rectangle(cr, x, y, width, height);
  cairo_set_source_rgba(cr,
                       color_cell->color.red,
                       color_cell->color.green,
                       color_cell->color.blue,
                       color_cell->color.alpha);
  cairo_fill(cr);

  /* Draw border around color swatch */
  cairo_rectangle(cr, x, y, width, height);
  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);  /* Black border */
  cairo_set_line_width(cr, 1.0);
  cairo_stroke(cr);
}

/* Property setter */
static void
ghid3_color_cell_renderer_set_property(GObject *object,
                                       guint param_id,
                                       const GValue *value,
                                       GParamSpec *pspec)
{
  Ghid3ColorCellRenderer *cell = GHID3_COLOR_CELL_RENDERER(object);

  switch (param_id)
    {
    case PROP_COLOR:
      {
        GdkRGBA *color = g_value_get_boxed(value);
        if (color)
          cell->color = *color;
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    }
}

/* Property getter */
static void
ghid3_color_cell_renderer_get_property(GObject *object,
                                       guint param_id,
                                       GValue *value,
                                       GParamSpec *pspec)
{
  Ghid3ColorCellRenderer *cell = GHID3_COLOR_CELL_RENDERER(object);

  switch (param_id)
    {
    case PROP_COLOR:
      g_value_set_boxed(value, &cell->color);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    }
}

/* Class initialization */
static void
ghid3_color_cell_renderer_class_init(Ghid3ColorCellRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS(klass);

  object_class->set_property = ghid3_color_cell_renderer_set_property;
  object_class->get_property = ghid3_color_cell_renderer_get_property;

  /* GTK3: Use new render method signature */
  cell_class->render = ghid3_color_cell_renderer_render;
  cell_class->get_preferred_width = ghid3_color_cell_renderer_get_preferred_width;
  cell_class->get_preferred_height = ghid3_color_cell_renderer_get_preferred_height;

  /* Install "color" property */
  g_object_class_install_property(object_class, PROP_COLOR,
    g_param_spec_boxed("color",
                      "Color",
                      "The color to display",
                      GDK_TYPE_RGBA,
                      G_PARAM_READWRITE));
}

/* Instance initialization */
static void
ghid3_color_cell_renderer_init(Ghid3ColorCellRenderer *cell)
{
  /* Default color: gray */
  cell->color.red = 0.5;
  cell->color.green = 0.5;
  cell->color.blue = 0.5;
  cell->color.alpha = 1.0;
}

/* Constructor */
GtkCellRenderer *
ghid3_layer_selector_color_cell_renderer_new(void)
{
  return g_object_new(GHID3_TYPE_COLOR_CELL_RENDERER, NULL);
}
```

**Key GTK3 Changes**:
- **render method**: GTK2 used `GdkRectangle` and `GdkDrawable`, GTK3 uses `cairo_t` directly
- **Size methods**: GTK2 used `get_size()`, GTK3 uses `get_preferred_width/height()`
- **Cairo drawing**: Direct use of `cairo_t` instead of creating Cairo context from GdkDrawable
- **GdkRGBA**: Use `GDK_TYPE_RGBA` for color property instead of `GDK_TYPE_COLOR`

**Testing checkpoint**: Verify color swatches appear in tree view with correct colors.

### Hour 7-8: Signal Handlers and Integration

Implement signal handlers for user interactions and integrate with PCB core.

```c
/* Visibility checkbox toggled */
static void
ghid3_layer_selector_visibility_toggled_cb(GtkCellRendererToggle *cell_renderer,
                                           gchar *path_str,
                                           gpointer user_data)
{
  Ghid3LayerSelector *selector = GHID3_LAYER_SELECTOR(user_data);
  GtkTreeModel *model = GTK_TREE_MODEL(selector->list_store);
  GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
  GtkTreeIter iter;
  gboolean visible;
  gint layer_index;

  if (!gtk_tree_model_get_iter(model, &iter, path))
    {
      gtk_tree_path_free(path);
      return;
    }

  /* Get current state */
  gtk_tree_model_get(model, &iter,
                    COL_VISIBLE, &visible,
                    COL_LAYER_INDEX, &layer_index,
                    -1);

  /* Toggle visibility */
  visible = !visible;

  /* Update list store */
  gtk_list_store_set(selector->list_store, &iter,
                    COL_VISIBLE, visible,
                    -1);

  /* Update PCB layer */
  if (!selector->callback_blocked)
    {
      LayerType *layer = &PCB->Data->Layer[layer_index];
      layer->On = visible;

      /* Emit signal */
      g_signal_emit(selector,
                   ghid3_layer_selector_signals[LAYER_VISIBILITY_CHANGED],
                   0, layer_index);

      /* Redraw canvas */
      ghid3_invalidate_all();
    }

  gtk_tree_path_free(path);
}

/* Row activated (double-click to select active layer) */
static void
ghid3_layer_selector_row_activated_cb(GtkTreeView *tree_view,
                                      GtkTreePath *path,
                                      GtkTreeViewColumn *column,
                                      gpointer user_data)
{
  Ghid3LayerSelector *selector = GHID3_LAYER_SELECTOR(user_data);
  GtkTreeModel *model = GTK_TREE_MODEL(selector->list_store);
  GtkTreeIter iter;
  gint layer_index;

  if (!gtk_tree_model_get_iter(model, &iter, path))
    return;

  gtk_tree_model_get(model, &iter,
                    COL_LAYER_INDEX, &layer_index,
                    -1);

  /* Set as active layer */
  ghid3_layer_selector_set_active_layer(selector, layer_index);
}

/* Public API: Set active layer */
void
ghid3_layer_selector_set_active_layer(Ghid3LayerSelector *selector, gint layer_index)
{
  if (selector->active_layer == layer_index)
    return;  /* No change */

  selector->active_layer = layer_index;

  /* Update PCB core */
  if (!selector->callback_blocked)
    {
      ChangeGroupVisibility(layer_index, TRUE, TRUE);

      /* Emit signal */
      g_signal_emit(selector,
                   ghid3_layer_selector_signals[ACTIVE_LAYER_CHANGED],
                   0, layer_index);
    }

  /* Visual feedback: highlight active layer row */
  ghid3_layer_selector_update_active_highlight(selector);
}

/* Update visual highlight for active layer */
static void
ghid3_layer_selector_update_active_highlight(Ghid3LayerSelector *selector)
{
  GtkTreeModel *model = GTK_TREE_MODEL(selector->list_store);
  GtkTreeIter iter;
  gboolean valid;

  /* Iterate through rows to find active layer */
  valid = gtk_tree_model_get_iter_first(model, &iter);
  while (valid)
    {
      gint layer_index;
      gtk_tree_model_get(model, &iter, COL_LAYER_INDEX, &layer_index, -1);

      if (layer_index == selector->active_layer)
        {
          /* Select this row */
          GtkTreeSelection *selection;
          selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(selector->tree_view));
          gtk_tree_selection_select_iter(selection, &iter);
          break;
        }

      valid = gtk_tree_model_iter_next(model, &iter);
    }
}

/* Public API: Update from PCB data */
void
ghid3_layer_selector_update_from_pcb(Ghid3LayerSelector *selector)
{
  /* Block callbacks to prevent recursion */
  selector->callback_blocked = TRUE;

  /* Repopulate list store */
  ghid3_layer_selector_populate(selector);

  /* Update active layer highlight */
  ghid3_layer_selector_update_active_highlight(selector);

  selector->callback_blocked = FALSE;
}

/* Constructor */
GtkWidget *
ghid3_layer_selector_new(void)
{
  Ghid3LayerSelector *selector;

  selector = g_object_new(GHID3_TYPE_LAYER_SELECTOR, NULL);

  /* Create UI */
  ghid3_layer_selector_create_tree_view(selector);

  /* Populate with initial data */
  if (PCB != NULL)
    ghid3_layer_selector_populate(selector);

  gtk_widget_show_all(GTK_WIDGET(selector));

  return GTK_WIDGET(selector);
}
```

**Day 1 Testing Protocol**:

1. **Create test program**:
```c
/* test_layer_selector.c */
#include <gtk/gtk.h>
#include "ghid-layer-selector.h"

static void
layer_visibility_changed_cb(Ghid3LayerSelector *selector, gint layer, gpointer data)
{
  g_print("Layer %d visibility changed\n", layer);
}

static void
active_layer_changed_cb(Ghid3LayerSelector *selector, gint layer, gpointer data)
{
  g_print("Active layer changed to %d\n", layer);
}

int
main(int argc, char **argv)
{
  GtkWidget *window, *selector;

  gtk_init(&argc, &argv);

  /* Initialize PCB (load test file or create dummy PCB) */
  init_test_pcb();

  /* Create window */
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "Layer Selector Test");
  gtk_window_set_default_size(GTK_WINDOW(window), 250, 400);

  /* Create layer selector */
  selector = ghid3_layer_selector_new();
  g_signal_connect(selector, "layer-visibility-changed",
                  G_CALLBACK(layer_visibility_changed_cb), NULL);
  g_signal_connect(selector, "active-layer-changed",
                  G_CALLBACK(active_layer_changed_cb), NULL);

  gtk_container_add(GTK_CONTAINER(window), selector);

  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  gtk_widget_show_all(window);
  gtk_main();

  return 0;
}
```

2. **Functional tests**:
   - Click visibility checkboxes, verify layers show/hide
   - Double-click layer name, verify active layer changes
   - Verify color swatches display correct colors
   - Verify all layers from PCB appear in list

3. **Visual tests**:
   - Color swatches should be rectangular with black border
   - Active layer should be highlighted/selected
   - Checkboxes should toggle on/off
   - Widget should respect GTK theme

**Expected results after Day 1**:
- ✓ Layer selector widget displays with tree view
- ✓ All PCB layers listed with visibility checkbox, color, and name
- ✓ Color swatches render correctly with Cairo
- ✓ Clicking visibility checkbox toggles layer on/off
- ✓ Double-clicking layer name sets it as active layer
- ✓ Signals emitted when visibility or active layer changes
- ✓ Widget updates when PCB data changes

---

## Day 2: Route Style Selector Widget (8 hours)

### Day 2 Overview

Migrate `ghid-route-style-selector.c` to GTK3. This widget allows users to:
- Select active route style (line width, via size, clearance)
- See visual preview of route style (line thickness)
- Quickly switch between predefined styles (Signal, Power, Fat, Skinny)
- Edit route style parameters

**Architecture**:
- GtkComboBox with custom cell renderer
- Custom rendering showing line width preview
- Integration with PCB route style data structures

### Hour 1-2: Basic Combo Box Structure

**File**: `src/hid/gtk3/ghid-route-style-selector.c`

```c
/* ghid-route-style-selector.c - Route style selector widget */

#include "config.h"
#include "gtkhid.h"
#include "gui.h"
#include "ghid-route-style-selector.h"

#include <gtk/gtk.h>

/* Widget structure */
typedef struct _Ghid3RouteStyleSelector Ghid3RouteStyleSelector;
typedef struct _Ghid3RouteStyleSelectorClass Ghid3RouteStyleSelectorClass;

struct _Ghid3RouteStyleSelector
{
  GtkBox parent;

  /* Child widgets */
  GtkWidget *combo_box;
  GtkListStore *list_store;

  /* State */
  gint active_style;
  gboolean callback_blocked;
};

struct _Ghid3RouteStyleSelectorClass
{
  GtkBoxClass parent_class;

  /* Signals */
  void (*style_changed) (Ghid3RouteStyleSelector *selector, gint style);
};

/* List store columns */
enum
{
  COL_STYLE_NAME,        /* gchar* - style name */
  COL_STYLE_THICK,       /* gint - line thickness in PCB units */
  COL_STYLE_DIAMETER,    /* gint - via diameter */
  COL_STYLE_DRILL,       /* gint - drill size */
  COL_STYLE_CLEARANCE,   /* gint - clearance */
  COL_STYLE_INDEX,       /* gint - route style index */
  NUM_STYLE_COLS
};

/* Signals */
enum
{
  STYLE_CHANGED,
  LAST_STYLE_SIGNAL
};

static guint ghid3_route_style_selector_signals[LAST_STYLE_SIGNAL] = { 0 };

G_DEFINE_TYPE(Ghid3RouteStyleSelector, ghid3_route_style_selector, GTK_TYPE_BOX)

static void
ghid3_route_style_selector_class_init(Ghid3RouteStyleSelectorClass *klass)
{
  /* Register signals */
  ghid3_route_style_selector_signals[STYLE_CHANGED] =
    g_signal_new("style-changed",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(Ghid3RouteStyleSelectorClass, style_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__INT,
                 G_TYPE_NONE, 1, G_TYPE_INT);
}

static void
ghid3_route_style_selector_init(Ghid3RouteStyleSelector *selector)
{
  gtk_orientable_set_orientation(GTK_ORIENTABLE(selector),
                                GTK_ORIENTATION_HORIZONTAL);

  selector->active_style = 0;
  selector->callback_blocked = FALSE;
}

/* Create combo box */
static void
ghid3_route_style_selector_create_combo(Ghid3RouteStyleSelector *selector)
{
  GtkCellRenderer *renderer;

  /* Create list store */
  selector->list_store = gtk_list_store_new(NUM_STYLE_COLS,
                                            G_TYPE_STRING,  /* Name */
                                            G_TYPE_INT,     /* Thickness */
                                            G_TYPE_INT,     /* Via diameter */
                                            G_TYPE_INT,     /* Drill */
                                            G_TYPE_INT,     /* Clearance */
                                            G_TYPE_INT);    /* Index */

  /* Create combo box */
  selector->combo_box = gtk_combo_box_new_with_model(
    GTK_TREE_MODEL(selector->list_store));
  g_object_unref(selector->list_store);

  /* Add cell renderer for text (style name) */
  renderer = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(selector->combo_box),
                             renderer, TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(selector->combo_box),
                                renderer,
                                "text", COL_STYLE_NAME,
                                NULL);

  /* Add custom cell renderer for line preview */
  renderer = ghid3_route_style_line_renderer_new();  /* Custom renderer */
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(selector->combo_box),
                             renderer, FALSE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(selector->combo_box),
                                renderer,
                                "thickness", COL_STYLE_THICK,
                                NULL);

  /* Connect changed signal */
  g_signal_connect(selector->combo_box, "changed",
                  G_CALLBACK(ghid3_route_style_selector_changed_cb),
                  selector);

  /* Add combo box to selector */
  gtk_box_pack_start(GTK_BOX(selector), selector->combo_box, TRUE, TRUE, 0);
}

/* Populate combo box with route styles */
static void
ghid3_route_style_selector_populate(Ghid3RouteStyleSelector *selector)
{
  GtkTreeIter iter;
  gint i;

  gtk_list_store_clear(selector->list_store);

  /* Add each route style from PCB */
  for (i = 0; i < NUM_STYLES; i++)
    {
      RouteStyleType *style = &PCB->RouteStyle[i];

      gtk_list_store_append(selector->list_store, &iter);
      gtk_list_store_set(selector->list_store, &iter,
                        COL_STYLE_NAME, style->Name,
                        COL_STYLE_THICK, style->Thick,
                        COL_STYLE_DIAMETER, style->Diameter,
                        COL_STYLE_DRILL, style->Drill,
                        COL_STYLE_CLEARANCE, style->Keepaway,
                        COL_STYLE_INDEX, i,
                        -1);
    }

  /* Set active style */
  gtk_combo_box_set_active(GTK_COMBO_BOX(selector->combo_box),
                           selector->active_style);
}
```

**Testing checkpoint**: Verify combo box displays with route style names.

### Hour 3-5: Custom Line Thickness Cell Renderer

Create custom cell renderer to show visual preview of line thickness.

```c
/* Custom cell renderer for line thickness preview */

#define GHID3_TYPE_ROUTE_STYLE_LINE_RENDERER  (ghid3_route_style_line_renderer_get_type())
#define GHID3_ROUTE_STYLE_LINE_RENDERER(obj)  (G_TYPE_CHECK_INSTANCE_CAST((obj), GHID3_TYPE_ROUTE_STYLE_LINE_RENDERER, Ghid3RouteStyleLineRenderer))

typedef struct _Ghid3RouteStyleLineRenderer Ghid3RouteStyleLineRenderer;
typedef struct _Ghid3RouteStyleLineRendererClass Ghid3RouteStyleLineRendererClass;

struct _Ghid3RouteStyleLineRenderer
{
  GtkCellRenderer parent;
  gint thickness;  /* Line thickness in PCB units */
};

struct _Ghid3RouteStyleLineRendererClass
{
  GtkCellRendererClass parent_class;
};

enum
{
  PROP_LINE_0,
  PROP_LINE_THICKNESS
};

G_DEFINE_TYPE(Ghid3RouteStyleLineRenderer, ghid3_route_style_line_renderer, GTK_TYPE_CELL_RENDERER)

/* Get preferred width */
static void
ghid3_route_style_line_renderer_get_preferred_width(GtkCellRenderer *cell,
                                                    GtkWidget *widget,
                                                    gint *minimum_width,
                                                    gint *natural_width)
{
  *minimum_width = 60;
  *natural_width = 60;
}

/* Get preferred height */
static void
ghid3_route_style_line_renderer_get_preferred_height(GtkCellRenderer *cell,
                                                     GtkWidget *widget,
                                                     gint *minimum_height,
                                                     gint *natural_height)
{
  *minimum_height = 24;
  *natural_height = 24;
}

/* Render line thickness preview using Cairo */
static void
ghid3_route_style_line_renderer_render(GtkCellRenderer *cell,
                                       cairo_t *cr,
                                       GtkWidget *widget,
                                       const GdkRectangle *background_area,
                                       const GdkRectangle *cell_area,
                                       GtkCellRendererState flags)
{
  Ghid3RouteStyleLineRenderer *line_cell = GHID3_ROUTE_STYLE_LINE_RENDERER(cell);
  gdouble line_width;
  gint x1, x2, y;

  /* Convert PCB units to pixels */
  /* Assume 100 PCB units = 1 pixel for preview (adjust as needed) */
  line_width = line_cell->thickness / 100.0;

  /* Clamp line width for preview (1-10 pixels) */
  if (line_width < 1.0)
    line_width = 1.0;
  if (line_width > 10.0)
    line_width = 10.0;

  /* Draw horizontal line across cell */
  x1 = cell_area->x + 4;
  x2 = cell_area->x + cell_area->width - 4;
  y = cell_area->y + cell_area->height / 2;

  cairo_set_line_width(cr, line_width);
  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);  /* Black line */
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

  cairo_move_to(cr, x1, y);
  cairo_line_to(cr, x2, y);
  cairo_stroke(cr);
}

/* Property setter */
static void
ghid3_route_style_line_renderer_set_property(GObject *object,
                                             guint param_id,
                                             const GValue *value,
                                             GParamSpec *pspec)
{
  Ghid3RouteStyleLineRenderer *cell = GHID3_ROUTE_STYLE_LINE_RENDERER(object);

  switch (param_id)
    {
    case PROP_LINE_THICKNESS:
      cell->thickness = g_value_get_int(value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    }
}

/* Property getter */
static void
ghid3_route_style_line_renderer_get_property(GObject *object,
                                             guint param_id,
                                             GValue *value,
                                             GParamSpec *pspec)
{
  Ghid3RouteStyleLineRenderer *cell = GHID3_ROUTE_STYLE_LINE_RENDERER(object);

  switch (param_id)
    {
    case PROP_LINE_THICKNESS:
      g_value_set_int(value, cell->thickness);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    }
}

/* Class initialization */
static void
ghid3_route_style_line_renderer_class_init(Ghid3RouteStyleLineRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS(klass);

  object_class->set_property = ghid3_route_style_line_renderer_set_property;
  object_class->get_property = ghid3_route_style_line_renderer_get_property;

  cell_class->render = ghid3_route_style_line_renderer_render;
  cell_class->get_preferred_width = ghid3_route_style_line_renderer_get_preferred_width;
  cell_class->get_preferred_height = ghid3_route_style_line_renderer_get_preferred_height;

  /* Install "thickness" property */
  g_object_class_install_property(object_class, PROP_LINE_THICKNESS,
    g_param_spec_int("thickness",
                    "Thickness",
                    "Line thickness in PCB units",
                    0, G_MAXINT, 1000,
                    G_PARAM_READWRITE));
}

/* Instance initialization */
static void
ghid3_route_style_line_renderer_init(Ghid3RouteStyleLineRenderer *cell)
{
  cell->thickness = 1000;  /* Default thickness */
}

/* Constructor */
GtkCellRenderer *
ghid3_route_style_line_renderer_new(void)
{
  return g_object_new(GHID3_TYPE_ROUTE_STYLE_LINE_RENDERER, NULL);
}
```

**Testing checkpoint**: Verify combo box dropdown shows line thickness previews next to route style names.

### Hour 6-8: Signal Handlers, Edit Dialog, and Integration

```c
/* Combo box selection changed */
static void
ghid3_route_style_selector_changed_cb(GtkComboBox *combo_box, gpointer user_data)
{
  Ghid3RouteStyleSelector *selector = GHID3_ROUTE_STYLE_SELECTOR(user_data);
  GtkTreeIter iter;
  gint style_index;

  if (selector->callback_blocked)
    return;

  if (!gtk_combo_box_get_active_iter(combo_box, &iter))
    return;

  gtk_tree_model_get(GTK_TREE_MODEL(selector->list_store), &iter,
                    COL_STYLE_INDEX, &style_index,
                    -1);

  selector->active_style = style_index;

  /* Update PCB core */
  SetRouteStyle(style_index);

  /* Emit signal */
  g_signal_emit(selector,
               ghid3_route_style_selector_signals[STYLE_CHANGED],
               0, style_index);
}

/* Add "Edit Styles..." button */
static void
ghid3_route_style_selector_add_edit_button(Ghid3RouteStyleSelector *selector)
{
  GtkWidget *button;

  button = gtk_button_new_with_label("Edit...");
  g_signal_connect(button, "clicked",
                  G_CALLBACK(ghid3_route_style_selector_edit_cb),
                  selector);

  gtk_box_pack_start(GTK_BOX(selector), button, FALSE, FALSE, 4);
}

/* Edit route styles dialog */
static void
ghid3_route_style_selector_edit_cb(GtkButton *button, gpointer user_data)
{
  Ghid3RouteStyleSelector *selector = GHID3_ROUTE_STYLE_SELECTOR(user_data);

  /* Show route style edit dialog */
  ghid3_route_style_edit_dialog_show(selector);
}

/* Public API: Update from PCB data */
void
ghid3_route_style_selector_update_from_pcb(Ghid3RouteStyleSelector *selector)
{
  selector->callback_blocked = TRUE;
  ghid3_route_style_selector_populate(selector);
  selector->callback_blocked = FALSE;
}

/* Constructor */
GtkWidget *
ghid3_route_style_selector_new(void)
{
  Ghid3RouteStyleSelector *selector;

  selector = g_object_new(GHID3_TYPE_ROUTE_STYLE_SELECTOR, NULL);

  /* Create UI */
  ghid3_route_style_selector_create_combo(selector);
  ghid3_route_style_selector_add_edit_button(selector);

  /* Populate with initial data */
  if (PCB != NULL)
    ghid3_route_style_selector_populate(selector);

  gtk_widget_show_all(GTK_WIDGET(selector));

  return GTK_WIDGET(selector);
}
```

**Day 2 Testing Protocol**:

1. **Visual tests**:
   - Combo box displays route style names
   - Dropdown shows line thickness previews
   - Thicker lines render wider than thin lines
   - Edit button appears next to combo box

2. **Functional tests**:
   - Select different route style, verify PCB updates
   - Click Edit button, verify edit dialog appears
   - Change route style parameters, verify combo updates

3. **Integration tests**:
   - Use route style to draw trace, verify correct thickness
   - Save and reload PCB, verify route styles persist

**Expected results after Day 2**:
- ✓ Route style selector displays with combo box and edit button
- ✓ Combo box lists all route styles from PCB
- ✓ Line thickness preview renders correctly
- ✓ Selecting route style updates PCB active style
- ✓ Edit button opens route style edit dialog
- ✓ Widget updates when route styles change

---

## Integration with Main Window

After both custom widgets are complete, integrate into main window:

```c
/* gui-top-window.c - Add widgets to sidebar */

static void
ghid3_create_sidebar(void)
{
  GtkWidget *sidebar;
  GtkWidget *layer_selector;
  GtkWidget *route_style_selector;

  sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width(GTK_CONTAINER(sidebar), 6);

  /* Add layer selector */
  layer_selector = ghid3_layer_selector_new();
  gtk_box_pack_start(GTK_BOX(sidebar), layer_selector, TRUE, TRUE, 0);

  /* Add separator */
  gtk_box_pack_start(GTK_BOX(sidebar), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                    FALSE, FALSE, 0);

  /* Add route style selector */
  route_style_selector = ghid3_route_style_selector_new();
  gtk_box_pack_start(GTK_BOX(sidebar), route_style_selector, FALSE, FALSE, 0);

  /* Store references for later updates */
  ghid3_port.layer_selector = layer_selector;
  ghid3_port.route_style_selector = route_style_selector;

  gtk_widget_show_all(sidebar);

  return sidebar;
}
```

---

## Common Issues and Solutions

### Issue 1: Custom cell renderer not drawing

**Solution**: Verify `get_preferred_width/height` methods implemented, ensure `render` method uses correct GTK3 signature.

### Issue 2: Colors look wrong

**Solution**: Use `GdkRGBA` instead of `GdkColor`, parse colors with `gdk_rgba_parse()`.

### Issue 3: Tree view performance slow with many layers

**Solution**: Use fixed-height mode: `gtk_tree_view_set_fixed_height_mode(tree_view, TRUE)`.

---

## Success Criteria

Milestone 4B is complete when:

- [ ] Layer selector widget functional
- [ ] Route style selector widget functional
- [ ] Custom cell renderers draw correctly with Cairo
- [ ] All signals emit properly
- [ ] Widgets integrate with main window
- [ ] Visual parity with GTK2 HID
- [ ] No crashes or memory leaks

---

## Estimated Completion Time

**Total**: 16 hours (2 days)

- Day 1 (8 hours): Layer selector widget
- Day 2 (8 hours): Route style selector widget

**Next Milestone**: 4C - Specialized Windows (library, netlist, DRC windows)
