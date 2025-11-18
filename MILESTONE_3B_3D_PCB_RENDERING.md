# Milestone 3B: Complete 3D PCB Rendering - Detailed Implementation Plan

**Duration**: 2 days (16 hours)
**Days**: Day 4-5 of Milestone 3
**Prerequisites**: Milestone 3A completed (GtkGLArea setup, basic 3D rendering)
**Goal**: Implement complete 3D visualization of PCB boards with proper layer stacking, component elevation, and optimized performance

---

## Overview

Milestone 3B builds upon the OpenGL infrastructure established in Milestone 3A to create a complete 3D PCB visualization system. This milestone transforms the basic 3D rendering capabilities into a full-featured 3D PCB viewer that can display:

- PCB layers with proper z-axis separation and thickness
- Traces, pads, and vias rendered as 3D geometry
- Components elevated above the board surface
- Copper layers with realistic thickness
- Silkscreen and soldermask layers
- Drilled holes and vias through multiple layers
- Proper lighting and material properties for realistic appearance

The implementation focuses on translating 2D PCB elements into 3D geometry while maintaining visual clarity and performance. This includes using OpenGL display lists or VBOs for efficient rendering, implementing proper depth sorting, and ensuring the 3D view provides useful information for PCB design and inspection.

**Critical Reminder**: This is parallel HID development. All code goes in `src/hid/gtk3/`, and all functions use the `ghid3_` prefix to avoid conflicts with the existing GTK2 HID in `src/hid/gtk/`.

---

## Day 4: 3D Layer Rendering with Thickness (8 hours)

### Day 4 Overview

Transform the 2D Cairo-based PCB rendering into true 3D geometry with proper layer stacking, thickness, and depth. The goal is to render a PCB board where you can clearly see the separation between layers, the thickness of copper traces, and how vias connect through multiple layers.

**Key concept**: In 2D mode, everything is flat on the z=0 plane. In 3D mode, each layer gets a z-coordinate based on the board stackup, and PCB elements (traces, pads, vias) are rendered as 3D solids with actual thickness.

### Hour 1-2: Layer Z-Coordinate System & 3D State Management

**File**: `src/hid/gtk3/gui-gl-3d.c` (new file)

Create the infrastructure for managing 3D layer positions and rendering state.

#### Create 3D layer coordinate system

```c
/* gui-gl-3d.c - 3D PCB rendering implementation */

#include "gtkhid.h"
#include "gui.h"
#include <GL/gl.h>
#include <GL/glu.h>

/* 3D rendering constants */
#define COPPER_THICKNESS_MIL 1.4    /* Standard 1 oz copper = 1.4 mil */
#define BOARD_THICKNESS_MIL 62.0    /* Standard FR4 = 62 mil (1.57mm) */
#define SOLDERMASK_THICKNESS_MIL 0.8
#define SILKSCREEN_THICKNESS_MIL 0.4
#define COMPONENT_ELEVATION_MIL 10.0 /* How far components sit above board */

/* Layer depth configuration for standard 2-layer board */
typedef struct {
  gboolean is_3d_mode;           /* TRUE = 3D rendering, FALSE = 2D */

  /* Layer z-coordinates in PCB units */
  Coord bottom_soldermask_z;     /* Bottom of board */
  Coord bottom_copper_z;
  Coord bottom_silk_z;
  Coord board_center_z;          /* Middle of FR4 substrate */
  Coord top_silk_z;
  Coord top_copper_z;
  Coord top_soldermask_z;        /* Top of board */
  Coord component_z;             /* Top of components */

  /* Thickness values in PCB units */
  Coord copper_thickness;
  Coord soldermask_thickness;
  Coord silkscreen_thickness;

  /* Rendering quality settings */
  gint cylinder_slices;          /* Number of sides for cylinders (pads, vias) */
  gboolean use_display_lists;    /* Use OpenGL display lists for performance */
  gboolean use_lighting;         /* Enable OpenGL lighting */

} Ghid3DRenderState;

static Ghid3DRenderState render_state_3d;

/* Initialize 3D rendering state for a 2-layer board */
void
ghid3_gl_init_3d_state(void)
{
  Coord copper_thick = MIL_TO_COORD(COPPER_THICKNESS_MIL);
  Coord board_thick = MIL_TO_COORD(BOARD_THICKNESS_MIL);
  Coord mask_thick = MIL_TO_COORD(SOLDERMASK_THICKNESS_MIL);
  Coord silk_thick = MIL_TO_COORD(SILKSCREEN_THICKNESS_MIL);

  render_state_3d.is_3d_mode = FALSE;  /* Start in 2D mode */

  /* Calculate layer z-coordinates from bottom to top */
  /* Bottom of board at z = 0 */
  render_state_3d.bottom_soldermask_z = 0;
  render_state_3d.bottom_copper_z = mask_thick;
  render_state_3d.bottom_silk_z = mask_thick + copper_thick;

  render_state_3d.board_center_z = board_thick / 2;

  /* Top layers */
  render_state_3d.top_silk_z = board_thick - mask_thick - copper_thick;
  render_state_3d.top_copper_z = board_thick - mask_thick;
  render_state_3d.top_soldermask_z = board_thick;
  render_state_3d.component_z = board_thick + MIL_TO_COORD(COMPONENT_ELEVATION_MIL);

  /* Store thicknesses */
  render_state_3d.copper_thickness = copper_thick;
  render_state_3d.soldermask_thickness = mask_thick;
  render_state_3d.silkscreen_thickness = silk_thick;

  /* Quality settings */
  render_state_3d.cylinder_slices = 16;  /* 16-sided cylinders */
  render_state_3d.use_display_lists = TRUE;
  render_state_3d.use_lighting = TRUE;
}

/* Get z-coordinate for a given layer in 3D mode */
Coord
ghid3_gl_get_layer_z(int layer_idx, gboolean is_top)
{
  if (!render_state_3d.is_3d_mode)
    return 0;  /* In 2D mode, everything at z=0 */

  /* For now, simple 2-layer board */
  if (is_top)
    return render_state_3d.top_copper_z;
  else
    return render_state_3d.bottom_copper_z;
}

/* Toggle between 2D and 3D rendering modes */
void
ghid3_gl_toggle_3d_mode(void)
{
  render_state_3d.is_3d_mode = !render_state_3d.is_3d_mode;

  if (render_state_3d.is_3d_mode)
    {
      /* Switching to 3D - adjust camera to good viewing angle */
      ghid3_gl_camera_set_3d_view();
    }
  else
    {
      /* Switching to 2D - reset to top-down view */
      ghid3_gl_camera_set_2d_view();
    }

  ghid3_gl_invalidate();  /* Request redraw */
}

gboolean
ghid3_gl_is_3d_mode(void)
{
  return render_state_3d.is_3d_mode;
}
```

**Testing checkpoint**: Verify that the layer z-coordinates are calculated correctly and that toggling between 2D/3D modes works. Add a keyboard shortcut (e.g., '3' key) to toggle 3D mode.

#### Add layer information to PCB data structures

```c
/* Update layer metadata with 3D information */
typedef struct {
  LayerType *pcb_layer;   /* Pointer to PCB layer data */
  gchar *name;            /* Layer name */
  Coord z_position;       /* Z coordinate in 3D space */
  Coord thickness;        /* Layer thickness */
  gboolean visible;       /* Layer visibility */
  GdkRGBA color;          /* Layer color */
  gboolean is_copper;     /* TRUE for copper layers */
} Ghid3LayerInfo;

static GArray *layer_info_3d = NULL;  /* Array of Ghid3LayerInfo */

/* Initialize layer information from PCB data */
void
ghid3_gl_init_layer_info(void)
{
  int i;

  if (layer_info_3d != NULL)
    g_array_free(layer_info_3d, TRUE);

  layer_info_3d = g_array_new(FALSE, FALSE, sizeof(Ghid3LayerInfo));

  /* Iterate through PCB layers */
  for (i = 0; i < max_copper_layer; i++)
    {
      Ghid3LayerInfo info;
      LayerType *layer = &PCB->Data->Layer[i];

      info.pcb_layer = layer;
      info.name = g_strdup(layer->Name);
      info.visible = layer->On;
      info.is_copper = TRUE;

      /* Determine if top or bottom layer */
      /* PCB convention: component side is top, solder side is bottom */
      gboolean is_top = (i % 2 == 0);  /* Simplified for 2-layer */

      if (is_top)
        {
          info.z_position = render_state_3d.top_copper_z;
          info.color = (GdkRGBA){0.8, 0.3, 0.2, 1.0};  /* Red for top */
        }
      else
        {
          info.z_position = render_state_3d.bottom_copper_z;
          info.color = (GdkRGBA){0.2, 0.3, 0.8, 1.0};  /* Blue for bottom */
        }

      info.thickness = render_state_3d.copper_thickness;

      g_array_append_val(layer_info_3d, info);
    }
}

/* Get layer info by index */
Ghid3LayerInfo *
ghid3_gl_get_layer_info(int layer_idx)
{
  if (layer_idx < 0 || layer_idx >= layer_info_3d->len)
    return NULL;

  return &g_array_index(layer_info_3d, Ghid3LayerInfo, layer_idx);
}
```

**Testing checkpoint**: Verify that layer information is correctly initialized and that each layer has the correct z-position and color.

### Hour 3-4: 3D Trace Rendering (Lines as Rectangular Prisms)

Transform 2D line drawing into 3D rectangular prisms with proper thickness.

```c
/* Draw a trace line as a 3D rectangular prism */
void
ghid3_gl_draw_line_3d(Coord x1, Coord y1, Coord x2, Coord y2,
                      Coord thickness, Coord z_position, Coord z_thickness,
                      const GdkRGBA *color)
{
  if (!render_state_3d.is_3d_mode)
    {
      /* In 2D mode, use Cairo rendering */
      ghid3_draw_line(x1, y1, x2, y2, thickness);
      return;
    }

  /* Convert to OpenGL coordinates */
  GLfloat x1_gl = COORD_TO_GL(x1);
  GLfloat y1_gl = COORD_TO_GL(y1);
  GLfloat x2_gl = COORD_TO_GL(x2);
  GLfloat y2_gl = COORD_TO_GL(y2);
  GLfloat z_gl = COORD_TO_GL(z_position);
  GLfloat z_top_gl = COORD_TO_GL(z_position + z_thickness);
  GLfloat half_width = COORD_TO_GL(thickness / 2);

  /* Calculate perpendicular vector for line width */
  GLfloat dx = x2_gl - x1_gl;
  GLfloat dy = y2_gl - y1_gl;
  GLfloat len = sqrtf(dx * dx + dy * dy);

  if (len < 0.001f)
    return;  /* Degenerate line */

  /* Normalize and rotate 90 degrees */
  GLfloat perp_x = -dy / len * half_width;
  GLfloat perp_y = dx / len * half_width;

  /* Set material color */
  glColor4f(color->red, color->green, color->blue, color->alpha);

  /* Draw rectangular prism (8 vertices, 6 faces) */
  glBegin(GL_QUADS);

  /* Bottom face (z_gl) */
  glNormal3f(0.0f, 0.0f, -1.0f);
  glVertex3f(x1_gl - perp_x, y1_gl - perp_y, z_gl);
  glVertex3f(x2_gl - perp_x, y2_gl - perp_y, z_gl);
  glVertex3f(x2_gl + perp_x, y2_gl + perp_y, z_gl);
  glVertex3f(x1_gl + perp_x, y1_gl + perp_y, z_gl);

  /* Top face (z_top_gl) */
  glNormal3f(0.0f, 0.0f, 1.0f);
  glVertex3f(x1_gl - perp_x, y1_gl - perp_y, z_top_gl);
  glVertex3f(x1_gl + perp_x, y1_gl + perp_y, z_top_gl);
  glVertex3f(x2_gl + perp_x, y2_gl + perp_y, z_top_gl);
  glVertex3f(x2_gl - perp_x, y2_gl - perp_y, z_top_gl);

  /* Side face 1 */
  GLfloat nx1 = perp_x / half_width;
  GLfloat ny1 = perp_y / half_width;
  glNormal3f(-nx1, -ny1, 0.0f);
  glVertex3f(x1_gl - perp_x, y1_gl - perp_y, z_gl);
  glVertex3f(x1_gl - perp_x, y1_gl - perp_y, z_top_gl);
  glVertex3f(x2_gl - perp_x, y2_gl - perp_y, z_top_gl);
  glVertex3f(x2_gl - perp_x, y2_gl - perp_y, z_gl);

  /* Side face 2 */
  glNormal3f(nx1, ny1, 0.0f);
  glVertex3f(x1_gl + perp_x, y1_gl + perp_y, z_gl);
  glVertex3f(x2_gl + perp_x, y2_gl + perp_y, z_gl);
  glVertex3f(x2_gl + perp_x, y2_gl + perp_y, z_top_gl);
  glVertex3f(x1_gl + perp_x, y1_gl + perp_y, z_top_gl);

  /* End face 1 */
  glNormal3f(-dx / len, -dy / len, 0.0f);
  glVertex3f(x1_gl - perp_x, y1_gl - perp_y, z_gl);
  glVertex3f(x1_gl + perp_x, y1_gl + perp_y, z_gl);
  glVertex3f(x1_gl + perp_x, y1_gl + perp_y, z_top_gl);
  glVertex3f(x1_gl - perp_x, y1_gl - perp_y, z_top_gl);

  /* End face 2 */
  glNormal3f(dx / len, dy / len, 0.0f);
  glVertex3f(x2_gl - perp_x, y2_gl - perp_y, z_gl);
  glVertex3f(x2_gl - perp_x, y2_gl - perp_y, z_top_gl);
  glVertex3f(x2_gl + perp_x, y2_gl + perp_y, z_top_gl);
  glVertex3f(x2_gl + perp_x, y2_gl + perp_y, z_gl);

  glEnd();
}

/* Render all lines on a layer as 3D geometry */
void
ghid3_gl_render_layer_lines_3d(int layer_idx)
{
  Ghid3LayerInfo *layer_info = ghid3_gl_get_layer_info(layer_idx);
  if (!layer_info || !layer_info->visible)
    return;

  LayerType *layer = layer_info->pcb_layer;

  /* Iterate through all lines on the layer */
  LINE_LOOP(layer);
  {
    ghid3_gl_draw_line_3d(line->Point1.X, line->Point1.Y,
                          line->Point2.X, line->Point2.Y,
                          line->Thickness,
                          layer_info->z_position,
                          layer_info->thickness,
                          &layer_info->color);
  }
  END_LOOP;
}
```

**Testing checkpoint**: Load a simple PCB file with traces and verify that lines are rendered as 3D rectangular prisms in 3D mode. Rotate the camera to view from different angles and verify that the geometry looks correct.

### Hour 5-6: 3D Pad & Via Rendering (Cylinders)

Render pads and vias as 3D cylinders using GLU quadrics.

```c
/* Draw a circular pad as a 3D cylinder */
void
ghid3_gl_draw_pad_3d(Coord x, Coord y, Coord diameter,
                     Coord z_position, Coord z_thickness,
                     const GdkRGBA *color)
{
  if (!render_state_3d.is_3d_mode)
    {
      /* In 2D mode, use Cairo rendering */
      ghid3_draw_circle(x, y, diameter);
      return;
    }

  GLfloat x_gl = COORD_TO_GL(x);
  GLfloat y_gl = COORD_TO_GL(y);
  GLfloat z_gl = COORD_TO_GL(z_position);
  GLfloat radius_gl = COORD_TO_GL(diameter / 2);
  GLfloat height_gl = COORD_TO_GL(z_thickness);

  /* Set material color */
  glColor4f(color->red, color->green, color->blue, color->alpha);

  glPushMatrix();

  /* Move to pad position */
  glTranslatef(x_gl, y_gl, z_gl);

  /* Create GLU quadric for cylinder */
  GLUquadric *quad = gluNewQuadric();
  gluQuadricDrawStyle(quad, GLU_FILL);
  gluQuadricNormals(quad, GLU_SMOOTH);

  /* Draw cylinder (bottom disk, sides, top disk) */

  /* Bottom disk */
  glPushMatrix();
  glRotatef(180.0f, 1.0f, 0.0f, 0.0f);  /* Flip normal to point down */
  gluDisk(quad, 0.0, radius_gl, render_state_3d.cylinder_slices, 1);
  glPopMatrix();

  /* Cylinder sides */
  gluCylinder(quad, radius_gl, radius_gl, height_gl,
              render_state_3d.cylinder_slices, 1);

  /* Top disk */
  glPushMatrix();
  glTranslatef(0.0f, 0.0f, height_gl);
  gluDisk(quad, 0.0, radius_gl, render_state_3d.cylinder_slices, 1);
  glPopMatrix();

  gluDeleteQuadric(quad);
  glPopMatrix();
}

/* Draw a via as a 3D cylinder that spans multiple layers */
void
ghid3_gl_draw_via_3d(Coord x, Coord y, Coord diameter,
                     Coord drill_diameter,
                     const GdkRGBA *color)
{
  if (!render_state_3d.is_3d_mode)
    {
      ghid3_draw_circle(x, y, diameter);
      return;
    }

  /* Vias go through entire board */
  Coord z_bottom = render_state_3d.bottom_copper_z;
  Coord z_top = render_state_3d.top_copper_z + render_state_3d.copper_thickness;
  Coord via_height = z_top - z_bottom;

  GLfloat x_gl = COORD_TO_GL(x);
  GLfloat y_gl = COORD_TO_GL(y);
  GLfloat z_gl = COORD_TO_GL(z_bottom);
  GLfloat outer_radius_gl = COORD_TO_GL(diameter / 2);
  GLfloat inner_radius_gl = COORD_TO_GL(drill_diameter / 2);
  GLfloat height_gl = COORD_TO_GL(via_height);

  glColor4f(color->red, color->green, color->blue, color->alpha);

  glPushMatrix();
  glTranslatef(x_gl, y_gl, z_gl);

  GLUquadric *quad = gluNewQuadric();
  gluQuadricDrawStyle(quad, GLU_FILL);
  gluQuadricNormals(quad, GLU_SMOOTH);

  /* Via is a hollow cylinder - use gluCylinder for outer and inner */

  /* Outer cylinder */
  gluCylinder(quad, outer_radius_gl, outer_radius_gl, height_gl,
              render_state_3d.cylinder_slices, 1);

  /* Top annular ring */
  glPushMatrix();
  glTranslatef(0.0f, 0.0f, height_gl);
  gluDisk(quad, inner_radius_gl, outer_radius_gl,
          render_state_3d.cylinder_slices, 1);
  glPopMatrix();

  /* Bottom annular ring */
  glPushMatrix();
  glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
  gluDisk(quad, inner_radius_gl, outer_radius_gl,
          render_state_3d.cylinder_slices, 1);
  glPopMatrix();

  /* Draw drilled hole as darker cylinder */
  if (inner_radius_gl > 0.001f)
    {
      glColor4f(0.1, 0.1, 0.1, 1.0);  /* Dark gray for hole */

      /* Inner cylinder for hole */
      gluCylinder(quad, inner_radius_gl, inner_radius_gl, height_gl,
                  render_state_3d.cylinder_slices, 1);
    }

  gluDeleteQuadric(quad);
  glPopMatrix();
}

/* Render all pads on a layer as 3D geometry */
void
ghid3_gl_render_layer_pads_3d(int layer_idx)
{
  Ghid3LayerInfo *layer_info = ghid3_gl_get_layer_info(layer_idx);
  if (!layer_info || !layer_info->visible)
    return;

  /* Iterate through all elements (components) */
  ELEMENT_LOOP(PCB->Data);
  {
    /* Render pads */
    PAD_LOOP(element);
    {
      /* Determine if pad is on this layer */
      gboolean on_layer = FALSE;

      if (TEST_FLAG(ONSOLDERFLAG, pad) && !layer_info->is_copper)
        on_layer = FALSE;  /* Pad is on bottom, checking top */
      else if (!TEST_FLAG(ONSOLDERFLAG, pad) && layer_info->is_copper)
        on_layer = TRUE;   /* Pad is on top */

      if (!on_layer)
        continue;

      /* For simplicity, treat rectangular pads as circles */
      /* In production, would need proper rectangle rendering */
      Coord x = (pad->Point1.X + pad->Point2.X) / 2;
      Coord y = (pad->Point1.Y + pad->Point2.Y) / 2;

      ghid3_gl_draw_pad_3d(x, y, pad->Thickness,
                           layer_info->z_position,
                           layer_info->thickness,
                           &layer_info->color);
    }
    END_LOOP;

    /* Render pins (through-hole pads - treat like vias) */
    PIN_LOOP(element);
    {
      ghid3_gl_draw_via_3d(pin->X, pin->Y,
                          pin->Thickness,
                          pin->DrillingHole,
                          &layer_info->color);
    }
    END_LOOP;
  }
  END_LOOP;
}

/* Render all vias as 3D geometry */
void
ghid3_gl_render_vias_3d(void)
{
  GdkRGBA via_color = {0.6, 0.6, 0.6, 1.0};  /* Gray for vias */

  VIA_LOOP(PCB->Data);
  {
    ghid3_gl_draw_via_3d(via->X, via->Y,
                        via->Thickness,
                        via->DrillingHole,
                        &via_color);
  }
  END_LOOP;
}
```

**Testing checkpoint**: Load a PCB with pads and vias. Verify that they render as proper 3D cylinders with correct height. Check that vias span from bottom to top copper layers.

### Hour 7-8: Complete Layer Rendering & Testing

Integrate all 3D rendering functions and test with real PCB files.

```c
/* Main 3D rendering function - called from render callback */
void
ghid3_gl_render_pcb_3d(void)
{
  int i;

  if (!render_state_3d.is_3d_mode)
    {
      /* Fall back to 2D Cairo rendering */
      ghid3_render_pcb_2d();
      return;
    }

  /* Enable depth testing for proper 3D rendering */
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  /* Enable lighting if requested */
  if (render_state_3d.use_lighting)
    {
      glEnable(GL_LIGHTING);
      glEnable(GL_LIGHT0);

      /* Set up light position (from above and to the side) */
      GLfloat light_pos[] = {1.0f, 1.0f, 2.0f, 0.0f};  /* Directional */
      GLfloat light_ambient[] = {0.3f, 0.3f, 0.3f, 1.0f};
      GLfloat light_diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};

      glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
      glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
      glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);

      /* Enable color material so glColor affects lighting */
      glEnable(GL_COLOR_MATERIAL);
      glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    }

  /* Draw FR4 substrate (the board itself) */
  ghid3_gl_draw_board_substrate();

  /* Render layers from bottom to top */
  for (i = 0; i < layer_info_3d->len; i++)
    {
      Ghid3LayerInfo *layer_info = ghid3_gl_get_layer_info(i);

      if (!layer_info->visible)
        continue;

      /* Render all primitives on this layer */
      ghid3_gl_render_layer_lines_3d(i);
      ghid3_gl_render_layer_pads_3d(i);
      ghid3_gl_render_layer_arcs_3d(i);      /* TODO: Hour 9 */
      ghid3_gl_render_layer_polygons_3d(i);  /* TODO: Hour 10 */
    }

  /* Render vias (they span multiple layers) */
  ghid3_gl_render_vias_3d();

  /* Draw reference grid and axes */
  if (render_state_3d.use_lighting)
    glDisable(GL_LIGHTING);  /* Grid doesn't need lighting */

  ghid3_gl_draw_grid(50.0f, 5.0f);
  ghid3_gl_draw_axes();

  glDisable(GL_DEPTH_TEST);
}

/* Draw the FR4 substrate board */
void
ghid3_gl_draw_board_substrate(void)
{
  GLfloat board_color[] = {0.2f, 0.35f, 0.2f, 0.9f};  /* Green FR4 */

  Coord x1 = 0;
  Coord y1 = 0;
  Coord x2 = PCB->MaxWidth;
  Coord y2 = PCB->MaxHeight;

  GLfloat x1_gl = COORD_TO_GL(x1);
  GLfloat y1_gl = COORD_TO_GL(y1);
  GLfloat x2_gl = COORD_TO_GL(x2);
  GLfloat y2_gl = COORD_TO_GL(y2);

  GLfloat z_bottom = COORD_TO_GL(render_state_3d.bottom_copper_z);
  GLfloat z_top = COORD_TO_GL(render_state_3d.top_copper_z);

  glColor4fv(board_color);

  /* Draw board as a rectangular prism */
  glBegin(GL_QUADS);

  /* Bottom face */
  glNormal3f(0.0f, 0.0f, -1.0f);
  glVertex3f(x1_gl, y1_gl, z_bottom);
  glVertex3f(x2_gl, y1_gl, z_bottom);
  glVertex3f(x2_gl, y2_gl, z_bottom);
  glVertex3f(x1_gl, y2_gl, z_bottom);

  /* Top face */
  glNormal3f(0.0f, 0.0f, 1.0f);
  glVertex3f(x1_gl, y1_gl, z_top);
  glVertex3f(x1_gl, y2_gl, z_top);
  glVertex3f(x2_gl, y2_gl, z_top);
  glVertex3f(x2_gl, y1_gl, z_top);

  /* Side faces */
  glNormal3f(0.0f, -1.0f, 0.0f);
  glVertex3f(x1_gl, y1_gl, z_bottom);
  glVertex3f(x1_gl, y1_gl, z_top);
  glVertex3f(x2_gl, y1_gl, z_top);
  glVertex3f(x2_gl, y1_gl, z_bottom);

  glNormal3f(1.0f, 0.0f, 0.0f);
  glVertex3f(x2_gl, y1_gl, z_bottom);
  glVertex3f(x2_gl, y1_gl, z_top);
  glVertex3f(x2_gl, y2_gl, z_top);
  glVertex3f(x2_gl, y2_gl, z_bottom);

  glNormal3f(0.0f, 1.0f, 0.0f);
  glVertex3f(x2_gl, y2_gl, z_bottom);
  glVertex3f(x2_gl, y2_gl, z_top);
  glVertex3f(x1_gl, y2_gl, z_top);
  glVertex3f(x1_gl, y2_gl, z_bottom);

  glNormal3f(-1.0f, 0.0f, 0.0f);
  glVertex3f(x1_gl, y2_gl, z_bottom);
  glVertex3f(x1_gl, y2_gl, z_top);
  glVertex3f(x1_gl, y1_gl, z_top);
  glVertex3f(x1_gl, y1_gl, z_bottom);

  glEnd();
}
```

**Day 4 Testing Protocol**:

1. **Simple board test**:
   - Load a simple 2-layer board with traces, pads, and vias
   - Toggle to 3D mode (press '3' key)
   - Verify layers are separated in z-axis
   - Rotate camera to view from side angle
   - Check that traces have rectangular cross-section
   - Check that pads and vias are cylindrical
   - Verify vias connect through board

2. **Complex board test**:
   - Load a complex board (e.g., tutorial1.pcb from examples)
   - Verify all layers render correctly
   - Check for z-fighting or rendering artifacts
   - Test layer visibility toggling in 3D mode

3. **Performance test**:
   - Load a large board (>1000 elements)
   - Measure frame rate in 3D mode
   - Should maintain >30 FPS on modern hardware

4. **Visual inspection**:
   - Board substrate should be visible as green FR4
   - Copper layers should have proper color (red top, blue bottom)
   - Lighting should provide depth cues
   - No gaps or holes in geometry

**Expected results after Day 4**:
- ✓ PCB renders in true 3D with layer separation
- ✓ Traces appear as 3D rectangular prisms
- ✓ Pads and vias appear as 3D cylinders
- ✓ Vias properly span from bottom to top layer
- ✓ Can toggle between 2D and 3D rendering modes
- ✓ Board substrate visible as green FR4 material
- ✓ Lighting provides realistic depth perception

---

## Day 5: Polish, Performance & Complete Testing (8 hours)

### Day 5 Overview

Complete the 3D rendering system by adding arcs and polygons, implementing performance optimizations, and conducting comprehensive testing. The goal is to achieve visual parity with the GTK2 3D mode while providing better performance and maintainability.

### Hour 9: 3D Arc & Polygon Rendering

Add support for arcs and filled polygons in 3D mode.

```c
/* Draw an arc as a 3D curved rectangular prism */
void
ghid3_gl_draw_arc_3d(Coord cx, Coord cy, Coord radius,
                     Coord thickness, int start_angle, int delta_angle,
                     Coord z_position, Coord z_thickness,
                     const GdkRGBA *color)
{
  if (!render_state_3d.is_3d_mode)
    {
      ghid3_draw_arc(cx, cy, radius, thickness, start_angle, delta_angle);
      return;
    }

  /* Approximate arc as series of short line segments */
  int num_segments = abs(delta_angle) / 10;  /* 10 degrees per segment */
  if (num_segments < 4)
    num_segments = 4;

  GLfloat cx_gl = COORD_TO_GL(cx);
  GLfloat cy_gl = COORD_TO_GL(cy);
  GLfloat rad_gl = COORD_TO_GL(radius);
  GLfloat half_thick_gl = COORD_TO_GL(thickness / 2);

  /* Generate arc points */
  int i;
  for (i = 0; i < num_segments; i++)
    {
      float angle1 = (start_angle + (delta_angle * i) / num_segments) * M_PI / 180.0;
      float angle2 = (start_angle + (delta_angle * (i + 1)) / num_segments) * M_PI / 180.0;

      /* Inner and outer points */
      GLfloat x1_outer = cx_gl + (rad_gl + half_thick_gl) * cosf(angle1);
      GLfloat y1_outer = cy_gl + (rad_gl + half_thick_gl) * sinf(angle1);
      GLfloat x1_inner = cx_gl + (rad_gl - half_thick_gl) * cosf(angle1);
      GLfloat y1_inner = cy_gl + (rad_gl - half_thick_gl) * sinf(angle1);

      GLfloat x2_outer = cx_gl + (rad_gl + half_thick_gl) * cosf(angle2);
      GLfloat y2_outer = cy_gl + (rad_gl + half_thick_gl) * sinf(angle2);
      GLfloat x2_inner = cx_gl + (rad_gl - half_thick_gl) * cosf(angle2);
      GLfloat y2_inner = cy_gl + (rad_gl - half_thick_gl) * sinf(angle2);

      /* Draw this arc segment as a rectangular prism */
      /* Simplified - draw outer edge and inner edge */
      Coord x1_out_pcb = GL_TO_COORD(x1_outer);
      Coord y1_out_pcb = GL_TO_COORD(y1_outer);
      Coord x2_out_pcb = GL_TO_COORD(x2_outer);
      Coord y2_out_pcb = GL_TO_COORD(y2_outer);

      ghid3_gl_draw_line_3d(x1_out_pcb, y1_out_pcb,
                           x2_out_pcb, y2_out_pcb,
                           thickness, z_position, z_thickness, color);
    }
}

/* Render all arcs on a layer */
void
ghid3_gl_render_layer_arcs_3d(int layer_idx)
{
  Ghid3LayerInfo *layer_info = ghid3_gl_get_layer_info(layer_idx);
  if (!layer_info || !layer_info->visible)
    return;

  LayerType *layer = layer_info->pcb_layer;

  ARC_LOOP(layer);
  {
    ghid3_gl_draw_arc_3d(arc->X, arc->Y, arc->Width,
                        arc->Thickness,
                        arc->StartAngle, arc->Delta,
                        layer_info->z_position,
                        layer_info->thickness,
                        &layer_info->color);
  }
  END_LOOP;
}

/* Draw a filled polygon as a 3D extruded shape */
void
ghid3_gl_draw_polygon_3d(PolygonType *polygon,
                        Coord z_position, Coord z_thickness,
                        const GdkRGBA *color)
{
  if (!render_state_3d.is_3d_mode)
    {
      ghid3_draw_polygon(polygon);
      return;
    }

  GLfloat z_gl = COORD_TO_GL(z_position);
  GLfloat z_top_gl = COORD_TO_GL(z_position + z_thickness);

  glColor4f(color->red, color->green, color->blue, color->alpha);

  /* Use GLU tesselator for complex polygons */
  GLUtesselator *tess = gluNewTess();

  /* Set tesselator callbacks */
  gluTessCallback(tess, GLU_TESS_BEGIN, (void (*)())glBegin);
  gluTessCallback(tess, GLU_TESS_VERTEX, (void (*)())glVertex3dv);
  gluTessCallback(tess, GLU_TESS_END, (void (*)())glEnd);

  /* Tesselate bottom face */
  glNormal3f(0.0f, 0.0f, -1.0f);
  gluTessBeginPolygon(tess, NULL);
  gluTessBeginContour(tess);

  POLYGONPOINT_LOOP(polygon);
  {
    GLdouble vertex[3];
    vertex[0] = COORD_TO_GL(point->X);
    vertex[1] = COORD_TO_GL(point->Y);
    vertex[2] = z_gl;
    gluTessVertex(tess, vertex, vertex);
  }
  END_LOOP;

  gluTessEndContour(tess);
  gluTessEndPolygon(tess);

  /* Tesselate top face */
  glNormal3f(0.0f, 0.0f, 1.0f);
  gluTessBeginPolygon(tess, NULL);
  gluTessBeginContour(tess);

  POLYGONPOINT_LOOP(polygon);
  {
    GLdouble vertex[3];
    vertex[0] = COORD_TO_GL(point->X);
    vertex[1] = COORD_TO_GL(point->Y);
    vertex[2] = z_top_gl;
    gluTessVertex(tess, vertex, vertex);
  }
  END_LOOP;

  gluTessEndContour(tess);
  gluTessEndPolygon(tess);

  /* Draw side walls */
  glBegin(GL_QUAD_STRIP);
  POLYGONPOINT_LOOP(polygon);
  {
    GLfloat x_gl = COORD_TO_GL(point->X);
    GLfloat y_gl = COORD_TO_GL(point->Y);

    /* Calculate outward normal for this edge */
    /* Simplified - would need proper normal calculation */
    glVertex3f(x_gl, y_gl, z_gl);
    glVertex3f(x_gl, y_gl, z_top_gl);
  }
  END_LOOP;

  /* Close the loop */
  PointType *first_point = &polygon->Points[0];
  GLfloat x_gl = COORD_TO_GL(first_point->X);
  GLfloat y_gl = COORD_TO_GL(first_point->Y);
  glVertex3f(x_gl, y_gl, z_gl);
  glVertex3f(x_gl, y_gl, z_top_gl);

  glEnd();

  gluDeleteTess(tess);
}

/* Render all polygons on a layer */
void
ghid3_gl_render_layer_polygons_3d(int layer_idx)
{
  Ghid3LayerInfo *layer_info = ghid3_gl_get_layer_info(layer_idx);
  if (!layer_info || !layer_info->visible)
    return;

  LayerType *layer = layer_info->pcb_layer;

  POLYGON_LOOP(layer);
  {
    ghid3_gl_draw_polygon_3d(polygon,
                            layer_info->z_position,
                            layer_info->thickness,
                            &layer_info->color);
  }
  END_LOOP;
}
```

**Testing checkpoint**: Load a PCB with arcs and polygons. Verify that arcs render smoothly and polygons are properly filled and extruded.

### Hour 10-11: Performance Optimization with Display Lists

Implement OpenGL display lists to cache geometry and improve rendering performance.

```c
/* Display list management */
typedef struct {
  GLuint display_list_id;
  gboolean needs_rebuild;
  int layer_idx;
} Ghid3DisplayList;

static GArray *display_lists = NULL;  /* Array of Ghid3DisplayList */

/* Initialize display list system */
void
ghid3_gl_init_display_lists(void)
{
  int i;

  if (display_lists != NULL)
    {
      /* Free existing display lists */
      for (i = 0; i < display_lists->len; i++)
        {
          Ghid3DisplayList *dl = &g_array_index(display_lists, Ghid3DisplayList, i);
          if (dl->display_list_id != 0)
            glDeleteLists(dl->display_list_id, 1);
        }
      g_array_free(display_lists, TRUE);
    }

  display_lists = g_array_new(FALSE, FALSE, sizeof(Ghid3DisplayList));

  /* Create display list for each layer */
  for (i = 0; i < max_copper_layer; i++)
    {
      Ghid3DisplayList dl;
      dl.display_list_id = glGenLists(1);
      dl.needs_rebuild = TRUE;
      dl.layer_idx = i;

      g_array_append_val(display_lists, dl);
    }
}

/* Build display list for a layer */
void
ghid3_gl_build_layer_display_list(int layer_idx)
{
  if (!render_state_3d.use_display_lists)
    return;

  Ghid3DisplayList *dl = &g_array_index(display_lists, Ghid3DisplayList, layer_idx);

  glNewList(dl->display_list_id, GL_COMPILE);

  /* Render all geometry for this layer */
  ghid3_gl_render_layer_lines_3d(layer_idx);
  ghid3_gl_render_layer_pads_3d(layer_idx);
  ghid3_gl_render_layer_arcs_3d(layer_idx);
  ghid3_gl_render_layer_polygons_3d(layer_idx);

  glEndList();

  dl->needs_rebuild = FALSE;
}

/* Mark all display lists as needing rebuild (call when PCB changes) */
void
ghid3_gl_invalidate_display_lists(void)
{
  int i;

  if (display_lists == NULL)
    return;

  for (i = 0; i < display_lists->len; i++)
    {
      Ghid3DisplayList *dl = &g_array_index(display_lists, Ghid3DisplayList, i);
      dl->needs_rebuild = TRUE;
    }
}

/* Optimized rendering using display lists */
void
ghid3_gl_render_pcb_3d_optimized(void)
{
  int i;

  if (!render_state_3d.is_3d_mode)
    {
      ghid3_render_pcb_2d();
      return;
    }

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  if (render_state_3d.use_lighting)
    {
      glEnable(GL_LIGHTING);
      glEnable(GL_LIGHT0);

      GLfloat light_pos[] = {1.0f, 1.0f, 2.0f, 0.0f};
      GLfloat light_ambient[] = {0.3f, 0.3f, 0.3f, 1.0f};
      GLfloat light_diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};

      glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
      glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
      glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);

      glEnable(GL_COLOR_MATERIAL);
      glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    }

  /* Draw board substrate */
  ghid3_gl_draw_board_substrate();

  /* Render layers using display lists */
  for (i = 0; i < display_lists->len; i++)
    {
      Ghid3DisplayList *dl = &g_array_index(display_lists, Ghid3DisplayList, i);
      Ghid3LayerInfo *layer_info = ghid3_gl_get_layer_info(dl->layer_idx);

      if (!layer_info || !layer_info->visible)
        continue;

      /* Rebuild display list if needed */
      if (dl->needs_rebuild)
        ghid3_gl_build_layer_display_list(dl->layer_idx);

      /* Call display list */
      glCallList(dl->display_list_id);
    }

  /* Render vias */
  ghid3_gl_render_vias_3d();

  /* Draw reference geometry */
  if (render_state_3d.use_lighting)
    glDisable(GL_LIGHTING);

  ghid3_gl_draw_grid(50.0f, 5.0f);
  ghid3_gl_draw_axes();

  glDisable(GL_DEPTH_TEST);
}
```

**Testing checkpoint**: Compare rendering performance with and without display lists. Should see 2-5x performance improvement on complex boards.

### Hour 12: UI Integration & Menu Items

Add UI controls for 3D rendering options.

**File**: `src/hid/gtk3/gui-top-window.c`

```c
/* Add menu items for 3D view */
static void
ghid3_view_3d_toggle_cb(GtkToggleAction *action, gpointer user_data)
{
  ghid3_gl_toggle_3d_mode();
}

static void
ghid3_view_3d_lighting_toggle_cb(GtkToggleAction *action, gpointer user_data)
{
  gboolean active = gtk_toggle_action_get_active(action);
  ghid3_gl_set_lighting(active);
  ghid3_gl_invalidate();
}

static const GtkToggleActionEntry view_3d_toggle_actions[] = {
  {"View3DMode", NULL, "_3D View", "F3",
   "Toggle 3D view mode", G_CALLBACK(ghid3_view_3d_toggle_cb), FALSE},

  {"View3DLighting", NULL, "3D _Lighting", NULL,
   "Toggle lighting in 3D view", G_CALLBACK(ghid3_view_3d_lighting_toggle_cb), TRUE},
};

/* Add to view menu */
static const char *view_menu_3d_items =
  "    <separator/>\n"
  "    <menuitem action='View3DMode'/>\n"
  "    <menuitem action='View3DLighting'/>\n";

/* Initialize 3D view menu items */
void
ghid3_init_3d_view_menu(GtkUIManager *ui_manager)
{
  GtkActionGroup *action_group;

  action_group = gtk_action_group_new("View3D");
  gtk_action_group_add_toggle_actions(action_group,
                                     view_3d_toggle_actions,
                                     G_N_ELEMENTS(view_3d_toggle_actions),
                                     NULL);

  gtk_ui_manager_insert_action_group(ui_manager, action_group, 0);

  /* Merge menu items */
  gtk_ui_manager_add_ui_from_string(ui_manager, view_menu_3d_items, -1, NULL);
}

/* Add keyboard shortcut handler */
static gboolean
ghid3_key_press_cb(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  /* '3' key toggles 3D mode */
  if (event->keyval == GDK_KEY_3 || event->keyval == GDK_KEY_KP_3)
    {
      ghid3_gl_toggle_3d_mode();
      return TRUE;
    }

  return FALSE;
}
```

**Testing checkpoint**: Verify that View menu shows 3D options, F3 key and '3' key toggle 3D mode, and lighting can be toggled.

### Hour 13-14: Comprehensive Testing & Bug Fixes

Conduct systematic testing across different PCB files and platforms.

#### Test Suite

```bash
#!/bin/bash
# test_3d_rendering.sh - Test script for 3D rendering

TEST_FILES=(
  "tutorial1.pcb"
  "tutorial2.pcb"
  "example_arduino.pcb"
  "complex_6layer.pcb"
)

echo "=== PCB GTK3 3D Rendering Test Suite ==="
echo ""

for pcb_file in "${TEST_FILES[@]}"; do
  echo "Testing: $pcb_file"

  # Test 2D mode
  echo "  - 2D rendering..."
  ./src/pcb --hid gtk3 "$pcb_file" &
  PCB_PID=$!
  sleep 3
  kill $PCB_PID 2>/dev/null

  # Test 3D mode
  echo "  - 3D rendering..."
  ./src/pcb --hid gtk3 "$pcb_file" &
  PCB_PID=$!
  sleep 3
  # Send '3' key to toggle 3D
  xdotool key --window $(xdotool search --pid $PCB_PID) 3
  sleep 3
  kill $PCB_PID 2>/dev/null

  echo "  ✓ Complete"
  echo ""
done

echo "=== Performance Test ==="
echo "Loading large board and measuring FPS..."
# Performance testing code here

echo "=== Test Complete ==="
```

#### Common Issues & Fixes

**Issue 1: Z-fighting on layers**
```c
/* Fix: Add small offset between layers */
#define LAYER_Z_OFFSET MIL_TO_COORD(0.1)  /* 0.1 mil separation */

Coord
ghid3_gl_get_layer_z(int layer_idx, gboolean is_top)
{
  Coord base_z = is_top ? render_state_3d.top_copper_z
                        : render_state_3d.bottom_copper_z;

  /* Add small offset per layer to prevent z-fighting */
  return base_z + (layer_idx * LAYER_Z_OFFSET);
}
```

**Issue 2: Performance degradation on large boards**
```c
/* Fix: Frustum culling - don't render off-screen elements */
gboolean
ghid3_gl_is_in_view_frustum(Coord x, Coord y, Coord radius)
{
  /* Get current view frustum from camera */
  /* Return TRUE if element is visible, FALSE otherwise */
  /* Skip rendering for FALSE */
  return TRUE;  /* Simplified - implement proper frustum culling */
}
```

**Issue 3: Lighting too dark/bright**
```c
/* Fix: Adjust light and material properties */
GLfloat light_ambient[] = {0.4f, 0.4f, 0.4f, 1.0f};   /* Brighter ambient */
GLfloat light_diffuse[] = {0.7f, 0.7f, 0.7f, 1.0f};   /* Less harsh diffuse */
GLfloat specular[] = {0.2f, 0.2f, 0.2f, 1.0f};        /* Subtle specular */

glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
```

### Hour 15-16: Documentation & Final Integration

Create documentation and ensure clean integration with build system.

#### Update `src/hid/gtk3/README.md`

```markdown
# GTK3 HID 3D Rendering

## Overview

The GTK3 HID includes complete 3D PCB visualization using OpenGL via GtkGLArea.

## Features

- True 3D layer stacking with configurable separation
- Realistic copper thickness rendering
- 3D vias that span through board layers
- OpenGL lighting for depth perception
- Smooth camera controls (rotate, zoom, pan)
- Performance-optimized with display lists
- Toggle between 2D and 3D views

## Usage

### Keyboard Shortcuts

- `3` or `F3` - Toggle 3D view mode
- Mouse drag - Rotate camera (in 3D mode)
- Mouse wheel - Zoom in/out
- Middle mouse drag - Pan view
- `R` - Reset camera to default position

### Menu Options

**View → 3D View** - Toggle 3D rendering mode
**View → 3D Lighting** - Enable/disable OpenGL lighting

## Architecture

### 3D Rendering Pipeline

1. **Layer z-coordinate calculation** (`ghid3_gl_init_3d_state`)
   - Each layer assigned z-position based on board stackup
   - Copper thickness: 1.4 mil (1 oz copper)
   - Board thickness: 62 mil (standard FR4)

2. **Geometry generation** (`ghid3_gl_render_pcb_3d`)
   - 2D primitives extruded into 3D geometry
   - Lines → rectangular prisms
   - Pads → cylinders
   - Vias → hollow cylinders
   - Polygons → extruded polygons with GLU tesselator

3. **Display list caching** (`ghid3_gl_build_layer_display_list`)
   - Each layer's geometry compiled into OpenGL display list
   - Rebuilt only when layer changes
   - 2-5x performance improvement on complex boards

4. **Lighting and materials** (`ghid3_gl_render_pcb_3d_optimized`)
   - Single directional light from above
   - Ambient + diffuse lighting model
   - Color material mode for layer colors

## Performance

- Display lists provide significant speedup
- Typical frame rates:
  - Simple board (< 100 elements): 60+ FPS
  - Medium board (100-1000 elements): 45-60 FPS
  - Complex board (> 1000 elements): 30-45 FPS

## Customization

### Adjusting Layer Thickness

Edit `gui-gl-3d.c`:

```c
#define COPPER_THICKNESS_MIL 1.4    /* Change to 0.7 for 0.5 oz */
#define BOARD_THICKNESS_MIL 62.0    /* Change for different PCB thickness */
```

### Adjusting Cylinder Quality

```c
render_state_3d.cylinder_slices = 16;  /* Higher = smoother, slower */
```

### Disabling Display Lists

```c
render_state_3d.use_display_lists = FALSE;  /* Use immediate mode */
```

## Known Limitations

- Currently supports 2-layer boards (4-layer support planned)
- Rectangular SMD pads rendered as circles (proper rectangles planned)
- No component 3D models (simple elevation only)
- Silkscreen and soldermask not rendered separately
```

#### Update `Makefile.am`

```makefile
# Add new 3D rendering source files
gtk3_SOURCES = \
  gui-top-window.c \
  gui-dialog.c \
  gui-output-events.c \
  gui-gl.c \
  gui-gl-3d.c \
  gui-gl-camera.c \
  gtkhid-main.c \
  gtkhid-gdk.c
```

#### Update `configure.ac`

```bash
# Ensure OpenGL and GLU are detected
PKG_CHECK_MODULES(GL, [gl glu], [have_gl=yes], [have_gl=no])
if test "x$have_gl" = "xyes"; then
  AC_DEFINE([HAVE_OPENGL], [1], [Define if OpenGL is available])
fi
```

**Day 5 Final Testing Protocol**:

1. **Feature completeness**:
   - ✓ All PCB primitives render in 3D (lines, arcs, pads, vias, polygons)
   - ✓ Layer stacking correct with proper z-separation
   - ✓ Lighting provides good depth perception
   - ✓ Camera controls work smoothly
   - ✓ Toggle between 2D/3D modes works
   - ✓ UI menus and keyboard shortcuts functional

2. **Visual parity test**:
   - Load same PCB in GTK2 HID (3D mode) and GTK3 HID (3D mode)
   - Compare visually - should look similar or better
   - Check that all elements visible in GTK2 are visible in GTK3

3. **Performance validation**:
   - Load tutorial1.pcb - should be 60 FPS
   - Load complex board - should be >30 FPS
   - Verify no memory leaks with valgrind
   - Check CPU usage is reasonable

4. **Platform testing**:
   - **Linux**: Test on Ubuntu 20.04+, Fedora 35+
   - **macOS**: Test on macOS 11+ (if accessible)
   - **Windows**: Test on Windows 10+ with MSYS2 (if accessible)

5. **Regression testing**:
   - Verify 2D mode still works correctly
   - Verify existing GTK2 HID unaffected
   - Run `make check` to ensure unit tests pass
   - Test PCB editing operations (add trace, move component)

**Expected results after Day 5**:
- ✓ Complete 3D PCB visualization working
- ✓ All primitives (lines, arcs, pads, vias, polygons) render correctly
- ✓ Performance optimized with display lists
- ✓ UI integration complete (menus, keyboard shortcuts)
- ✓ Documentation written
- ✓ Visual parity with GTK2 3D mode achieved
- ✓ All tests passing
- ✓ Ready for integration with rest of GTK3 HID

---

## What You Can Do After Completing Milestone 3B

After successfully completing this milestone, you will have:

### Core 3D Functionality

1. **Complete 3D PCB Visualization**
   - View your PCB designs in true 3D with proper layer separation
   - See copper traces rendered as 3D rectangular prisms with realistic thickness
   - View pads and vias as 3D cylinders
   - Inspect how vias connect through multiple layers
   - Visualize the FR4 substrate board with realistic green color
   - Toggle between 2D flat view and 3D layered view instantly

2. **Interactive 3D Viewing**
   - Rotate the PCB board in 3D space using mouse drag
   - Zoom in/out to inspect details or see the whole board
   - Pan the view to focus on specific areas
   - Reset camera to default comfortable viewing angle
   - View from any angle to inspect layer stacking

3. **Professional Rendering Quality**
   - OpenGL lighting provides realistic depth perception
   - Proper z-ordering prevents rendering artifacts
   - Smooth camera movements without jitter
   - Configurable rendering quality (cylinder smoothness)

### Performance & Usability

4. **Optimized Performance**
   - Display lists cache geometry for fast rendering
   - 60 FPS on simple boards, 30+ FPS on complex boards
   - Efficient handling of large PCB files (1000+ elements)
   - No lag or stuttering during camera manipulation

5. **User-Friendly Controls**
   - Simple keyboard shortcut ('3' or F3) to toggle 3D mode
   - Intuitive mouse controls for camera manipulation
   - Menu items for 3D options (View → 3D View, View → 3D Lighting)
   - Easy switching between 2D editing and 3D inspection

### Technical Capabilities

6. **Parallel HID Architecture**
   - GTK3 3D rendering works independently of GTK2 HID
   - Choose between HIDs with `--hid gtk3` flag
   - Both HIDs available in same build
   - No interference with existing GTK2 functionality

7. **Extensibility Foundation**
   - Clean separation between 2D Cairo and 3D OpenGL rendering
   - Modular design allows easy addition of new 3D features
   - Display list system ready for more complex geometry
   - Layer management system ready for multi-layer boards

### Validation & Quality

8. **Tested & Reliable**
   - Comprehensive test suite covering all PCB primitives
   - Visual parity with GTK2 3D mode verified
   - Performance benchmarks met
   - Cross-platform compatibility (Linux, macOS, Windows)
   - No memory leaks or crashes

### What's Still Missing (Future Work)

After Milestone 3B, these features would still need implementation:

- **Dialogs and widgets** (Milestone 4) - Preferences, layer selector, etc.
- **4+ layer board support** - Currently optimized for 2-layer
- **Component 3D models** - Currently just elevation, no actual 3D shapes
- **Soldermask & silkscreen layers** - Not rendered separately yet
- **Advanced lighting** - Multiple lights, shadows, ambient occlusion
- **Anti-aliasing** - Smooth edges in 3D mode
- **Texture mapping** - Realistic PCB surface textures

But the core 3D visualization system will be complete and usable!

---

## Integration Points

### Files Modified

**New files created**:
- `src/hid/gtk3/gui-gl-3d.c` - 3D rendering implementation (~800 lines)
- `src/hid/gtk3/gui-gl-3d.h` - 3D rendering API (~100 lines)

**Files modified**:
- `src/hid/gtk3/gui-gl.c` - Add 3D mode toggle logic
- `src/hid/gtk3/gui-top-window.c` - Add 3D menu items and keyboard shortcuts
- `src/hid/gtk3/gui-gl-camera.c` - Extend camera for 3D positioning
- `src/hid/gtk3/Makefile.am` - Add new source files
- `configure.ac` - Verify GL/GLU availability

### API Changes

**New public functions**:
```c
/* 3D mode management */
void ghid3_gl_init_3d_state(void);
void ghid3_gl_toggle_3d_mode(void);
gboolean ghid3_gl_is_3d_mode(void);
void ghid3_gl_set_lighting(gboolean enabled);

/* Layer management */
void ghid3_gl_init_layer_info(void);
Ghid3LayerInfo *ghid3_gl_get_layer_info(int layer_idx);
Coord ghid3_gl_get_layer_z(int layer_idx, gboolean is_top);

/* 3D primitive rendering */
void ghid3_gl_draw_line_3d(Coord x1, Coord y1, Coord x2, Coord y2,
                           Coord thickness, Coord z_position, Coord z_thickness,
                           const GdkRGBA *color);
void ghid3_gl_draw_pad_3d(Coord x, Coord y, Coord diameter,
                          Coord z_position, Coord z_thickness,
                          const GdkRGBA *color);
void ghid3_gl_draw_via_3d(Coord x, Coord y, Coord diameter,
                          Coord drill_diameter, const GdkRGBA *color);
void ghid3_gl_draw_arc_3d(Coord cx, Coord cy, Coord radius,
                          Coord thickness, int start_angle, int delta_angle,
                          Coord z_position, Coord z_thickness,
                          const GdkRGBA *color);
void ghid3_gl_draw_polygon_3d(PolygonType *polygon,
                             Coord z_position, Coord z_thickness,
                             const GdkRGBA *color);

/* Complete rendering */
void ghid3_gl_render_pcb_3d(void);
void ghid3_gl_render_pcb_3d_optimized(void);

/* Display list management */
void ghid3_gl_init_display_lists(void);
void ghid3_gl_invalidate_display_lists(void);
void ghid3_gl_build_layer_display_list(int layer_idx);
```

---

## Risk Mitigation

### Identified Risks

1. **OpenGL compatibility issues**
   - Some older systems may not support required OpenGL version
   - **Mitigation**: Check OpenGL version at runtime, fall back to 2D if unsupported
   - Test on variety of graphics cards and drivers

2. **Performance on large boards**
   - Rendering thousands of 3D primitives could be slow
   - **Mitigation**: Display lists, frustum culling, level-of-detail reduction
   - Profile and optimize hot paths

3. **Z-fighting artifacts**
   - Overlapping geometry at same z-coordinate causes flickering
   - **Mitigation**: Small z-offsets between layers, proper depth buffer precision
   - Adjust depth range if needed

4. **Memory usage**
   - Display lists and geometry data could use significant RAM
   - **Mitigation**: Monitor memory usage, free unused display lists
   - Option to disable display lists on low-memory systems

### Fallback Strategy

If 3D rendering encounters critical issues:
1. Display error message to user
2. Automatically fall back to 2D Cairo rendering
3. Disable 3D menu items
4. Log error details for debugging

```c
gboolean
ghid3_gl_init_3d_rendering(GError **error)
{
  /* Check OpenGL version */
  const GLubyte *version_string = glGetString(GL_VERSION);
  if (!version_string)
    {
      g_set_error(error, GHID_ERROR, GHID_ERROR_GL,
                  "Failed to get OpenGL version");
      return FALSE;
    }

  /* Require OpenGL 2.1 or later */
  int major, minor;
  sscanf((const char *)version_string, "%d.%d", &major, &minor);

  if (major < 2 || (major == 2 && minor < 1))
    {
      g_set_error(error, GHID_ERROR, GHID_ERROR_GL,
                  "OpenGL 2.1 or later required (found %d.%d)", major, minor);
      return FALSE;
    }

  /* Initialize 3D state */
  ghid3_gl_init_3d_state();
  ghid3_gl_init_layer_info();
  ghid3_gl_init_display_lists();

  return TRUE;
}
```

---

## Troubleshooting Guide

### Problem: 3D view is completely black

**Diagnosis**: Lighting is enabled but no lights are configured, or all colors are (0,0,0,1).

**Solution**:
```c
/* Verify light is enabled and positioned */
glEnable(GL_LIGHTING);
glEnable(GL_LIGHT0);

GLfloat light_pos[] = {1.0f, 1.0f, 2.0f, 0.0f};
glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

/* Verify colors are not black */
glColor4f(0.8, 0.3, 0.2, 1.0);  /* Red, not black */
```

### Problem: Geometry appears inside-out or transparent

**Diagnosis**: Face culling is enabled and winding order is wrong, or alpha blending issue.

**Solution**:
```c
/* Disable face culling during development */
glDisable(GL_CULL_FACE);

/* Or fix winding order - counter-clockwise for front faces */
glFrontFace(GL_CCW);

/* Ensure alpha is 1.0 for opaque */
glColor4f(r, g, b, 1.0);
```

### Problem: Performance is very slow (< 10 FPS)

**Diagnosis**: Display lists not enabled, or geometry too complex.

**Solution**:
```c
/* Enable display lists */
render_state_3d.use_display_lists = TRUE;

/* Reduce cylinder detail for pads/vias */
render_state_3d.cylinder_slices = 8;  /* Lower from 16 */

/* Implement frustum culling to skip off-screen elements */
```

### Problem: Layers appear at wrong heights

**Diagnosis**: Z-coordinate calculation incorrect.

**Solution**:
```c
/* Verify layer z-coordinates after init */
void ghid3_gl_debug_print_layer_z(void)
{
  printf("Bottom soldermask: %d\n", render_state_3d.bottom_soldermask_z);
  printf("Bottom copper: %d\n", render_state_3d.bottom_copper_z);
  printf("Top copper: %d\n", render_state_3d.top_copper_z);
  printf("Top soldermask: %d\n", render_state_3d.top_soldermask_z);
}
```

### Problem: Vias don't appear to connect layers

**Diagnosis**: Via height calculation wrong, or not spanning full board thickness.

**Solution**:
```c
/* Verify via spans from bottom copper to top copper + thickness */
Coord z_bottom = render_state_3d.bottom_copper_z;
Coord z_top = render_state_3d.top_copper_z + render_state_3d.copper_thickness;
Coord via_height = z_top - z_bottom;  /* Must be > 0 */
```

### Problem: Camera won't rotate/zoom

**Diagnosis**: Camera matrix not being applied, or mouse events not connected.

**Solution**:
```c
/* Verify camera setup is called before rendering */
void ghid3_gl_render_cb(...)
{
  glClear(...);

  ghid3_gl_setup_camera();  /* MUST call this first */

  ghid3_gl_render_pcb_3d();
}

/* Verify mouse signal is connected */
g_signal_connect(gl_area, "button-press-event",
                 G_CALLBACK(ghid3_gl_button_press_cb), NULL);
```

---

## Success Criteria

Milestone 3B is complete when:

- [ ] All PCB primitives render correctly in 3D (lines, arcs, pads, vias, polygons)
- [ ] Layer stacking is correct with visible z-separation
- [ ] Vias properly span from bottom to top layer
- [ ] FR4 substrate board is visible
- [ ] Lighting provides good depth perception
- [ ] Camera can rotate, zoom, and pan smoothly
- [ ] Toggle between 2D and 3D modes works (F3 or '3' key)
- [ ] UI menus for 3D options functional
- [ ] Display lists provide 2-5x performance improvement
- [ ] Visual parity with GTK2 3D mode achieved
- [ ] No rendering artifacts (z-fighting, flickering, gaps)
- [ ] Performance meets targets (30+ FPS on complex boards)
- [ ] No memory leaks detected with valgrind
- [ ] Documentation complete (README.md, code comments)
- [ ] All tests passing on Linux (primary platform)

---

## Estimated Completion Time

**Total**: 16 hours (2 days)

- Day 4 (8 hours): 3D layer rendering with thickness
  - Hour 1-2: Layer z-coordinate system & state management
  - Hour 3-4: 3D trace rendering (rectangular prisms)
  - Hour 5-6: 3D pad & via rendering (cylinders)
  - Hour 7-8: Complete layer rendering & testing

- Day 5 (8 hours): Polish, performance & complete testing
  - Hour 9: Arc & polygon rendering
  - Hour 10-11: Performance optimization (display lists)
  - Hour 12: UI integration (menus, keyboard shortcuts)
  - Hour 13-14: Comprehensive testing & bug fixes
  - Hour 15-16: Documentation & final integration

**Dependencies**: Milestone 3A must be complete (GtkGLArea setup, camera controls)

**Next Milestone**: Milestone 4 (if needed) - Dialogs, custom widgets, and final polish

---

## Notes

- **Parallel development**: All code in `src/hid/gtk3/`, GTK2 HID untouched
- **Function naming**: Use `ghid3_` prefix for all functions
- **Testing**: Test frequently after each major feature
- **Performance**: Profile early, optimize display lists
- **Visual quality**: Compare with GTK2 3D mode for reference
- **Documentation**: Update as you go, don't leave for end

This milestone completes the core 3D visualization capability of the GTK3 HID!
