#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include "crosshair.h"
#include "clip.h"
#include "../hidint.h"
#include "gui.h"
#include "gui-pinout-preview.h"

/* The Linux OpenGL ABI 1.0 spec requires that we define
 * GL_GLEXT_PROTOTYPES before including gl.h or glx.h for extensions
 * in order to get prototypes:
 *   http://www.opengl.org/registry/ABI/
 */
#define GL_GLEXT_PROTOTYPES 1

/* This follows autoconf's recommendation for the AX_CHECK_GL macro
   https://www.gnu.org/software/autoconf-archive/ax_check_gl.html */
#if defined HAVE_WINDOWS_H && defined _WIN32
#  include <windows.h>
#endif
#if defined HAVE_GL_GL_H
#  include <GL/gl.h>
#elif defined HAVE_OPENGL_GL_H
#  include <OpenGL/gl.h>
#else
#  error autoconf couldnt find gl.h
#endif

/* GTK3: GtkGLArea is built into GTK3, no external library needed */
#include "hid/common/hidgl.h"

#include "hid/common/draw_helpers.h"
#include "hid/common/trackball.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

extern HID ghid_hid;
extern HID_DRAW ghid_graphics;

static hidGC current_gc = NULL;

/* Sets gport->u_gc to the "right" GC to use (wrt mask or window)
*/
#define USE_GC(gc) if (!use_gc(gc)) return

static enum mask_mode cur_mask = HID_MASK_OFF;
static GLfloat view_matrix[4][4] = {{1.0, 0.0, 0.0, 0.0},
                                    {0.0, 1.0, 0.0, 0.0},
                                    {0.0, 0.0, 1.0, 0.0},
                                    {0.0, 0.0, 0.0, 1.0}};
static GLfloat last_modelview_matrix[4][4] = {{1.0, 0.0, 0.0, 0.0},
                                              {0.0, 1.0, 0.0, 0.0},
                                              {0.0, 0.0, 1.0, 0.0},
                                              {0.0, 0.0, 0.0, 1.0}};
static int global_view_2d = 1;
static int current_layer_idx = 0;  /* Track current layer for 3D thickness */

/* ===== Milestone 3B: 3D Layer Coordinate System ===== */

/* 3D rendering constants for standard PCB stackup */
#define COPPER_THICKNESS_MIL 1.4      /* 1 oz copper = 1.4 mil */
#define BOARD_THICKNESS_MIL 62.0      /* Standard FR4 = 62 mil (1.57mm) */
#define SOLDERMASK_THICKNESS_MIL 0.8  /* Typical solder mask */
#define SILKSCREEN_THICKNESS_MIL 0.4  /* Silkscreen layer */
#define COMPONENT_ELEVATION_MIL 10.0  /* Component height above board */

/* 3D layer depth configuration */
typedef struct {
  /* Layer z-coordinates in PCB units (from bottom to top) */
  Coord bottom_soldermask_z;     /* Bottom of board (z = 0) */
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
  Coord board_thickness;
} Ghid3DLayerStack;

static Ghid3DLayerStack layer_stack_3d;

/* Initialize 3D layer stack for a standard 2-layer board */
static void
ghid_init_3d_layer_stack (void)
{
  Coord copper_thick = MIL_TO_COORD (COPPER_THICKNESS_MIL);
  Coord board_thick = MIL_TO_COORD (BOARD_THICKNESS_MIL);
  Coord mask_thick = MIL_TO_COORD (SOLDERMASK_THICKNESS_MIL);
  Coord silk_thick = MIL_TO_COORD (SILKSCREEN_THICKNESS_MIL);

  /* Calculate layer z-coordinates from bottom to top */
  layer_stack_3d.bottom_soldermask_z = 0;  /* Bottom at z = 0 */
  layer_stack_3d.bottom_copper_z = mask_thick;
  layer_stack_3d.bottom_silk_z = mask_thick + copper_thick;
  layer_stack_3d.board_center_z = board_thick / 2;
  layer_stack_3d.top_silk_z = board_thick - mask_thick - copper_thick;
  layer_stack_3d.top_copper_z = board_thick - mask_thick;
  layer_stack_3d.top_soldermask_z = board_thick;
  layer_stack_3d.component_z = board_thick + MIL_TO_COORD (COMPONENT_ELEVATION_MIL);

  /* Store thicknesses */
  layer_stack_3d.copper_thickness = copper_thick;
  layer_stack_3d.soldermask_thickness = mask_thick;
  layer_stack_3d.silkscreen_thickness = silk_thick;
  layer_stack_3d.board_thickness = board_thick;
}

/* Get z-coordinate for a given layer in 3D mode */
static Coord
ghid_get_layer_depth (int layer_idx)
{
  /* In 2D mode, everything is flat at z = 0 */
  if (global_view_2d)
    return 0;

  /* Map PCB layer indices to z-coordinates */
  if (layer_idx >= 0 && layer_idx < max_copper_layer)
    {
      /* Copper layer - determine if top or bottom side */
      /* For now, assume component side (top) for layers 0-(n/2-1),
       * solder side (bottom) for layers n/2-(n-1) */
      if (layer_idx < max_copper_layer / 2)
        return layer_stack_3d.top_copper_z;
      else
        return layer_stack_3d.bottom_copper_z;
    }

  /* Special layers */
  if (layer_idx < 0)
    {
      switch (SL_TYPE (layer_idx))
        {
        case SL_SILK:
          if (SL_MYSIDE (layer_idx))  /* Component side */
            return layer_stack_3d.top_silk_z + layer_stack_3d.silkscreen_thickness;
          else  /* Solder side */
            return layer_stack_3d.bottom_silk_z;

        case SL_MASK:
          if (SL_MYSIDE (layer_idx))  /* Component side */
            return layer_stack_3d.top_soldermask_z;
          else  /* Solder side */
            return layer_stack_3d.bottom_soldermask_z;

        case SL_INVISIBLE:
        case SL_ASSY:
        case SL_PDRILL:
        case SL_UDRILL:
        case SL_RATS:
          return layer_stack_3d.top_copper_z + layer_stack_3d.copper_thickness;
        }
    }

  /* Default to top copper */
  return layer_stack_3d.top_copper_z;
}

/* Get thickness for a given layer in 3D mode */
static Coord
ghid_get_layer_thickness (int layer_idx)
{
  /* In 2D mode, no thickness */
  if (global_view_2d)
    return 0;

  /* Map PCB layer indices to thickness values */
  if (layer_idx >= 0 && layer_idx < max_copper_layer)
    return layer_stack_3d.copper_thickness;

  /* Special layers */
  if (layer_idx < 0)
    {
      switch (SL_TYPE (layer_idx))
        {
        case SL_SILK:
          return layer_stack_3d.silkscreen_thickness;

        case SL_MASK:
          return layer_stack_3d.soldermask_thickness;

        default:
          return 0;
        }
    }

  return 0;
}

/* ===== End Milestone 3B: 3D Layer Coordinate System ===== */

/* ===== Milestone 3B: 3D Geometry Helpers ===== */

/* Draw a 3D cylinder (for pads, vias, pins) */
static void
ghid_draw_3d_cylinder (Coord x, Coord y, Coord z_bottom, Coord radius, Coord height, int slices)
{
  GLUquadric *quad;

  if (height <= 0)
    return;  /* Skip if no height in 2D mode */

  quad = gluNewQuadric ();
  gluQuadricDrawStyle (quad, GLU_FILL);
  gluQuadricNormals (quad, GLU_SMOOTH);

  glPushMatrix ();
  glTranslatef (x, y, z_bottom);

  /* Draw cylinder body */
  gluCylinder (quad, radius, radius, height, slices, 1);

  /* Draw top cap */
  glPushMatrix ();
  glTranslatef (0, 0, height);
  gluDisk (quad, 0, radius, slices, 1);
  glPopMatrix ();

  /* Draw bottom cap */
  gluDisk (quad, 0, radius, slices, 1);

  glPopMatrix ();
  gluDeleteQuadric (quad);
}

/* Draw a 3D rectangular box (for rectangular pads) */
static void
ghid_draw_3d_box (Coord x1, Coord y1, Coord x2, Coord y2, Coord z_bottom, Coord height)
{
  Coord z_top = z_bottom + height;

  if (height <= 0)
    return;  /* Skip if no height in 2D mode */

  /* Ensure x1 < x2 and y1 < y2 */
  if (x1 > x2) { Coord t = x1; x1 = x2; x2 = t; }
  if (y1 > y2) { Coord t = y1; y1 = y2; y2 = t; }

  glBegin (GL_QUADS);

  /* Bottom face (z = z_bottom) */
  glNormal3f (0, 0, -1);
  glVertex3f (x1, y1, z_bottom);
  glVertex3f (x2, y1, z_bottom);
  glVertex3f (x2, y2, z_bottom);
  glVertex3f (x1, y2, z_bottom);

  /* Top face (z = z_top) */
  glNormal3f (0, 0, 1);
  glVertex3f (x1, y1, z_top);
  glVertex3f (x1, y2, z_top);
  glVertex3f (x2, y2, z_top);
  glVertex3f (x2, y1, z_top);

  /* Front face (y = y1) */
  glNormal3f (0, -1, 0);
  glVertex3f (x1, y1, z_bottom);
  glVertex3f (x1, y1, z_top);
  glVertex3f (x2, y1, z_top);
  glVertex3f (x2, y1, z_bottom);

  /* Back face (y = y2) */
  glNormal3f (0, 1, 0);
  glVertex3f (x1, y2, z_bottom);
  glVertex3f (x2, y2, z_bottom);
  glVertex3f (x2, y2, z_top);
  glVertex3f (x1, y2, z_top);

  /* Left face (x = x1) */
  glNormal3f (-1, 0, 0);
  glVertex3f (x1, y1, z_bottom);
  glVertex3f (x1, y2, z_bottom);
  glVertex3f (x1, y2, z_top);
  glVertex3f (x1, y1, z_top);

  /* Right face (x = x2) */
  glNormal3f (1, 0, 0);
  glVertex3f (x2, y1, z_bottom);
  glVertex3f (x2, y1, z_top);
  glVertex3f (x2, y2, z_top);
  glVertex3f (x2, y2, z_bottom);

  glEnd ();
}

/* Draw a thick 3D line segment (for traces) */
static void
ghid_draw_3d_line (Coord x1, Coord y1, Coord x2, Coord y2, Coord z, Coord width, Coord thickness)
{
  GLfloat dx, dy, len, nx, ny;
  GLfloat half_width;
  GLfloat p1x, p1y, p2x, p2y, p3x, p3y, p4x, p4y;
  Coord z_top = z + thickness;

  if (thickness <= 0)
    return;  /* Skip if no thickness in 2D mode */

  /* Calculate perpendicular vector for line width */
  dx = x2 - x1;
  dy = y2 - y1;
  len = sqrt (dx * dx + dy * dy);

  if (len < 0.001)
    return;  /* Skip zero-length lines */

  half_width = width / 2.0;
  nx = -dy / len * half_width;  /* Perpendicular x */
  ny = dx / len * half_width;   /* Perpendicular y */

  /* Calculate the 4 corners of the line segment */
  p1x = x1 + nx; p1y = y1 + ny;  /* Start, left */
  p2x = x1 - nx; p2y = y1 - ny;  /* Start, right */
  p3x = x2 - nx; p3y = y2 - ny;  /* End, right */
  p4x = x2 + nx; p4y = y2 + ny;  /* End, left */

  glBegin (GL_QUADS);

  /* Bottom face */
  glNormal3f (0, 0, -1);
  glVertex3f (p1x, p1y, z);
  glVertex3f (p2x, p2y, z);
  glVertex3f (p3x, p3y, z);
  glVertex3f (p4x, p4y, z);

  /* Top face */
  glNormal3f (0, 0, 1);
  glVertex3f (p1x, p1y, z_top);
  glVertex3f (p4x, p4y, z_top);
  glVertex3f (p3x, p3y, z_top);
  glVertex3f (p2x, p2y, z_top);

  /* Side faces */
  /* Left side */
  glNormal3f (ny / half_width, -nx / half_width, 0);
  glVertex3f (p1x, p1y, z);
  glVertex3f (p4x, p4y, z);
  glVertex3f (p4x, p4y, z_top);
  glVertex3f (p1x, p1y, z_top);

  /* Right side */
  glNormal3f (-ny / half_width, nx / half_width, 0);
  glVertex3f (p2x, p2y, z);
  glVertex3f (p2x, p2y, z_top);
  glVertex3f (p3x, p3y, z_top);
  glVertex3f (p3x, p3y, z);

  /* End caps would be added here for round_cap style */

  glEnd ();
}

/* ===== End Milestone 3B: 3D Geometry Helpers ===== */


typedef struct render_priv {
  /* GTK3: GdkGLConfig removed - GtkGLArea handles configuration */
  bool trans_lines;
  bool in_context;
  int subcomposite_stencil_bit;
  char *current_colorname;
  double current_alpha_mult;
  GTimer *time_since_expose;

  /* Feature for leading the user to a particular location */
  guint lead_user_timeout;
  GTimer *lead_user_timer;
  bool lead_user;
  Coord lead_user_radius;
  Coord lead_user_x;
  Coord lead_user_y;

  hidGC crosshair_gc;
} render_priv;


typedef struct hid_gc_struct
{
  HID *me_pointer;

  const char *colorname;
  double alpha_mult;
  Coord width;
  gint cap, join;
}
hid_gc_struct;


static void draw_lead_user (render_priv *priv);
static void ghid_unproject_to_z_plane (int ex, int ey, Coord pcb_z, Coord *pcb_x, Coord *pcb_y);


static void
start_subcomposite (void)
{
  render_priv *priv = gport->render_priv;
  int stencil_bit;

  /* Flush out any existing geoemtry to be rendered */
  hidgl_flush_triangles (&buffer);

  glEnable (GL_STENCIL_TEST);                                 /* Enable Stencil test */
  glStencilOp (GL_KEEP, GL_KEEP, GL_REPLACE);                 /* Stencil pass => replace stencil value (with 1) */

  stencil_bit = hidgl_assign_clear_stencil_bit();             /* Get a new (clean) bitplane to stencil with */
  glStencilMask (stencil_bit);                                /* Only write to our subcompositing stencil bitplane */
  glStencilFunc (GL_GREATER, stencil_bit, stencil_bit);       /* Pass stencil test if our assigned bit is clear */

  priv->subcomposite_stencil_bit = stencil_bit;
}

static void
end_subcomposite (void)
{
  render_priv *priv = gport->render_priv;

  /* Flush out any existing geoemtry to be rendered */
  hidgl_flush_triangles (&buffer);

  hidgl_return_stencil_bit (priv->subcomposite_stencil_bit);  /* Relinquish any bitplane we previously used */

  glStencilMask (0);
  glStencilFunc (GL_ALWAYS, 0, 0);                            /* Always pass stencil test */
  glDisable (GL_STENCIL_TEST);                                /* Disable Stencil test */

  priv->subcomposite_stencil_bit = 0;
}


int
ghid_set_layer (const char *name, int group, int empty)
{
  render_priv *priv = gport->render_priv;
  int idx = group;
  if (idx >= 0 && idx < max_group)
    {
      int n = PCB->LayerGroups.Number[group];
      for (idx = 0; idx < n-1; idx ++)
	{
	  int ni = PCB->LayerGroups.Entries[group][idx];
	  if (ni >= 0 && ni < max_copper_layer + SILK_LAYER
	      && PCB->Data->Layer[ni].On)
	    break;
	}
      idx = PCB->LayerGroups.Entries[group][idx];
  }

  end_subcomposite ();
  start_subcomposite ();

  /* Milestone 3B: Set depth for 3D layer rendering */
  current_layer_idx = idx;  /* Track for thickness calculation */
  hidgl_set_depth (ghid_get_layer_depth (idx));

  if (idx >= 0 && idx < max_copper_layer + SILK_LAYER)
    {
      priv->trans_lines = true;
      return PCB->Data->Layer[idx].On;
    }
  if (idx < 0)
    {
      switch (SL_TYPE (idx))
	{
	case SL_INVISIBLE:
	  return PCB->InvisibleObjectsOn;
	case SL_MASK:
	  if (SL_MYSIDE (idx))
	    return TEST_FLAG (SHOWMASKFLAG, PCB);
	  return 0;
	case SL_SILK:
	  priv->trans_lines = true;
	  if (SL_MYSIDE (idx))
	    return PCB->ElementOn;
	  return 0;
	case SL_ASSY:
	  return 0;
	case SL_PDRILL:
	case SL_UDRILL:
	  return 1;
	case SL_RATS:
	  if (PCB->RatOn)
	    priv->trans_lines = true;
	  return PCB->RatOn;
	}
    }
  return 0;
}

static void
ghid_end_layer (void)
{
  end_subcomposite ();
}

void
ghid_destroy_gc (hidGC gc)
{
  g_free (gc);
}

hidGC
ghid_make_gc (void)
{
  hidGC rv;

  rv = g_new0 (hid_gc_struct, 1);
  rv->me_pointer = &ghid_hid;
  rv->colorname = Settings.BackgroundColor;
  rv->alpha_mult = 1.0;
  return rv;
}

void
ghid_draw_grid (BoxType *drawn_area)
{
  if (Vz (PCB->Grid) < MIN_GRID_DISTANCE)
    return;

  if (gdk_color_parse (Settings.GridColor, &gport->grid_color))
    {
      gport->grid_color.red ^= gport->bg_color.red;
      gport->grid_color.green ^= gport->bg_color.green;
      gport->grid_color.blue ^= gport->bg_color.blue;
    }

  glEnable (GL_COLOR_LOGIC_OP);
  glLogicOp (GL_XOR);

  glColor3f (gport->grid_color.red / 65535.,
             gport->grid_color.green / 65535.,
             gport->grid_color.blue / 65535.);

  hidgl_draw_grid (drawn_area);

  glDisable (GL_COLOR_LOGIC_OP);
}

static void
ghid_draw_bg_image (void)
{
  static GLuint texture_handle = 0;

  if (!ghidgui->bg_pixbuf)
    return;

  if (texture_handle == 0)
    {
      int width =             gdk_pixbuf_get_width (ghidgui->bg_pixbuf);
      int height =            gdk_pixbuf_get_height (ghidgui->bg_pixbuf);
      int rowstride =         gdk_pixbuf_get_rowstride (ghidgui->bg_pixbuf);
      int bits_per_sample =   gdk_pixbuf_get_bits_per_sample (ghidgui->bg_pixbuf);
      int n_channels =        gdk_pixbuf_get_n_channels (ghidgui->bg_pixbuf);
      unsigned char *pixels = gdk_pixbuf_get_pixels (ghidgui->bg_pixbuf);

      g_warn_if_fail (bits_per_sample == 8);
      g_warn_if_fail (rowstride == width * n_channels);

      glGenTextures (1, &texture_handle);
      glBindTexture (GL_TEXTURE_2D, texture_handle);

      /* XXX: We should proabbly determine what the maxmimum texture supported is,
       *      and if our image is larger, shrink it down using GDK pixbuf routines
       *      rather than having it fail below.
       */

      glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
                    (n_channels == 4) ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, pixels);
    }

  glBindTexture (GL_TEXTURE_2D, texture_handle);

  glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glEnable (GL_TEXTURE_2D);

  /* Render a quad with the background as a texture */

  glBegin (GL_QUADS);
  glTexCoord2d (0., 0.);
  glVertex3i (0,             0,              0);
  glTexCoord2d (1., 0.);
  glVertex3i (PCB->MaxWidth, 0,              0);
  glTexCoord2d (1., 1.);
  glVertex3i (PCB->MaxWidth, PCB->MaxHeight, 0);
  glTexCoord2d (0., 1.);
  glVertex3i (0,             PCB->MaxHeight, 0);
  glEnd ();

  glDisable (GL_TEXTURE_2D);
}

void
ghid_use_mask (enum mask_mode mode)
{
  static int stencil_bit = 0;

  if (mode == cur_mask)
    return;

  /* Flush out any existing geoemtry to be rendered */
  hidgl_flush_triangles (&buffer);

  switch (mode)
    {
    case HID_MASK_BEFORE:
      /* The HID asks not to receive this mask type, so warn if we get it */
      g_return_if_reached ();

    case HID_MASK_CLEAR:
      /* Write '1' to the stencil buffer where the solder-mask should not be drawn. */
      glColorMask (0, 0, 0, 0);                             /* Disable writting in color buffer */
      glEnable (GL_STENCIL_TEST);                           /* Enable Stencil test */
      stencil_bit = hidgl_assign_clear_stencil_bit();       /* Get a new (clean) bitplane to stencil with */
      glStencilFunc (GL_ALWAYS, stencil_bit, stencil_bit);  /* Always pass stencil test, write stencil_bit */
      glStencilMask (stencil_bit);                          /* Only write to our subcompositing stencil bitplane */
      glStencilOp (GL_KEEP, GL_KEEP, GL_REPLACE);           /* Stencil pass => replace stencil value (with 1) */
      break;

    case HID_MASK_AFTER:
      /* Drawing operations as masked to areas where the stencil buffer is '0' */
      glColorMask (1, 1, 1, 1);                   /* Enable drawing of r, g, b & a */
      glStencilFunc (GL_GEQUAL, 0, stencil_bit);  /* Draw only where our bit of the stencil buffer is clear */
      glStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);    /* Stencil buffer read only */
      break;

    case HID_MASK_OFF:
      /* Disable stenciling */
      hidgl_return_stencil_bit (stencil_bit);     /* Relinquish any bitplane we previously used */
      glDisable (GL_STENCIL_TEST);                /* Disable Stencil test */
      break;
    }
  cur_mask = mode;
}


  /* Config helper functions for when the user changes color preferences.
     |  set_special colors used in the gtkhid.
   */
static void
set_special_grid_color (void)
{
  if (!gport->colormap)
    return;
  gport->grid_color.red ^= gport->bg_color.red;
  gport->grid_color.green ^= gport->bg_color.green;
  gport->grid_color.blue ^= gport->bg_color.blue;
}

void
ghid_set_special_colors (HID_Attribute * ha)
{
  if (!ha->name || !ha->value)
    return;
  if (!strcmp (ha->name, "background-color"))
    {
      ghid_map_color_string (*(char **) ha->value, &gport->bg_color);
      set_special_grid_color ();
    }
  else if (!strcmp (ha->name, "off-limit-color"))
  {
      ghid_map_color_string (*(char **) ha->value, &gport->offlimits_color);
    }
  else if (!strcmp (ha->name, "grid-color"))
    {
      ghid_map_color_string (*(char **) ha->value, &gport->grid_color);
      set_special_grid_color ();
    }
}

typedef struct
{
  int color_set;
  GdkColor color;
  double red;
  double green;
  double blue;
} ColorCache;

static void
set_gl_color_for_gc (hidGC gc)
{
  render_priv *priv = gport->render_priv;
  static void *cache = NULL;
  hidval cval;
  ColorCache *cc;
  double r, g, b, a;

  if (priv->current_colorname != NULL &&
      strcmp (priv->current_colorname, gc->colorname) == 0 &&
      priv->current_alpha_mult == gc->alpha_mult)
    return;

  free (priv->current_colorname);
  priv->current_colorname = strdup (gc->colorname);
  priv->current_alpha_mult = gc->alpha_mult;

  if (gport->colormap == NULL)
    gport->colormap = gtk_widget_get_colormap (gport->top_window);
  if (strcmp (gc->colorname, "erase") == 0)
    {
      r = gport->bg_color.red   / 65535.;
      g = gport->bg_color.green / 65535.;
      b = gport->bg_color.blue  / 65535.;
      a = 1.0;
    }
  else if (strcmp (gc->colorname, "drill") == 0)
    {
      r = gport->offlimits_color.red   / 65535.;
      g = gport->offlimits_color.green / 65535.;
      b = gport->offlimits_color.blue  / 65535.;
      a = 0.85;
    }
  else
    {
      if (hid_cache_color (0, gc->colorname, &cval, &cache))
        cc = (ColorCache *) cval.ptr;
      else
        {
          cc = (ColorCache *) malloc (sizeof (ColorCache));
          memset (cc, 0, sizeof (*cc));
          cval.ptr = cc;
          hid_cache_color (1, gc->colorname, &cval, &cache);
        }

      if (!cc->color_set)
        {
          if (gdk_color_parse (gc->colorname, &cc->color))
            gdk_color_alloc (gport->colormap, &cc->color);
          else
            gdk_color_white (gport->colormap, &cc->color);
          cc->red   = cc->color.red   / 65535.;
          cc->green = cc->color.green / 65535.;
          cc->blue  = cc->color.blue  / 65535.;
          cc->color_set = 1;
        }
      r = cc->red;
      g = cc->green;
      b = cc->blue;
      a = 0.7;
    }
  if (1) {
    double maxi, mult;
    a *= gc->alpha_mult;
    if (!priv->trans_lines)
      a = 1.0;
    maxi = r;
    if (g > maxi) maxi = g;
    if (b > maxi) maxi = b;
    mult = MIN (1 / a, 1 / maxi);
#if 1
    r = r * mult;
    g = g * mult;
    b = b * mult;
#endif
  }

  if(!priv->in_context)
    return;

  hidgl_flush_triangles (&buffer);
  glColor4d (r, g, b, a);
}

void
ghid_set_color (hidGC gc, const char *name)
{
  gc->colorname = name;
  set_gl_color_for_gc (gc);
}

void
ghid_set_alpha_mult (hidGC gc, double alpha_mult)
{
  gc->alpha_mult = alpha_mult;
  set_gl_color_for_gc (gc);
}

void
ghid_set_line_cap (hidGC gc, EndCapStyle style)
{
  gc->cap = style;
}

void
ghid_set_line_width (hidGC gc, Coord width)
{
  gc->width = width;
}


void
ghid_set_draw_xor (hidGC gc, int xor)
{
  /* NOT IMPLEMENTED */

  /* Only presently called when setting up a crosshair GC.
   * We manage our own drawing model for that anyway. */
}

void
ghid_set_draw_faded (hidGC gc, int faded)
{
  printf ("ghid_set_draw_faded(%p,%d) -- not implemented\n", gc, faded);
}

void
ghid_set_line_cap_angle (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  printf ("ghid_set_line_cap_angle() -- not implemented\n");
}

static void
ghid_invalidate_current_gc (void)
{
  current_gc = NULL;
}

static int
use_gc (hidGC gc)
{
  if (gc->me_pointer != &ghid_hid)
    {
      fprintf (stderr, "Fatal: GC from another HID passed to GTK HID\n");
      abort ();
    }

  if (current_gc == gc)
    return 1;

  current_gc = gc;

  set_gl_color_for_gc (gc);
  return 1;
}

void
ghid_draw_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  USE_GC (gc);

  /* Milestone 3B: Use 3D thick line rendering in 3D mode */
  if (!global_view_2d)
    {
      Coord thickness = ghid_get_layer_thickness (current_layer_idx);
      if (thickness > 0)
        {
          hidgl_flush_triangles (&buffer);  /* Flush buffered triangles first */
          ghid_draw_3d_line (x1, y1, x2, y2, global_depth, gc->width, thickness);
          return;
        }
    }

  /* 2D mode or zero thickness: use standard flat rendering */
  hidgl_draw_line (gc->cap, gc->width, x1, y1, x2, y2, gport->view.coord_per_px);
}

void
ghid_draw_arc (hidGC gc, Coord cx, Coord cy, Coord xradius, Coord yradius,
                         Angle start_angle, Angle delta_angle)
{
  USE_GC (gc);

  hidgl_draw_arc (gc->width, cx, cy, xradius, yradius,
                  start_angle, delta_angle, gport->view.coord_per_px);
}

void
ghid_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  USE_GC (gc);

  hidgl_draw_rect (x1, y1, x2, y2);
}


void
ghid_fill_circle (hidGC gc, Coord cx, Coord cy, Coord radius)
{
  USE_GC (gc);

  /* Milestone 3B: Use 3D cylinder rendering in 3D mode */
  if (!global_view_2d)
    {
      Coord thickness = ghid_get_layer_thickness (current_layer_idx);
      if (thickness > 0)
        {
          hidgl_flush_triangles (&buffer);  /* Flush buffered triangles first */
          ghid_draw_3d_cylinder (cx, cy, global_depth, radius, thickness, 16);
          return;
        }
    }

  /* 2D mode or zero thickness: use standard flat rendering */
  hidgl_fill_circle (cx, cy, radius, gport->view.coord_per_px);
}


void
ghid_fill_polygon (hidGC gc, int n_coords, Coord *x, Coord *y)
{
  USE_GC (gc);

  hidgl_fill_polygon (n_coords, x, y);
}

void
ghid_fill_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box)
{
  USE_GC (gc);

  hidgl_fill_pcb_polygon (poly, clip_box, gport->view.coord_per_px);
}

void
ghid_thindraw_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box)
{
  common_thindraw_pcb_polygon (gc, poly, clip_box);
  ghid_set_alpha_mult (gc, 0.25);
  gui->graphics->fill_pcb_polygon (gc, poly, clip_box);
  ghid_set_alpha_mult (gc, 1.0);
}

void
ghid_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  USE_GC (gc);

  /* Milestone 3B: Use 3D box rendering in 3D mode */
  if (!global_view_2d)
    {
      Coord thickness = ghid_get_layer_thickness (current_layer_idx);
      if (thickness > 0)
        {
          hidgl_flush_triangles (&buffer);  /* Flush buffered triangles first */
          ghid_draw_3d_box (x1, y1, x2, y2, global_depth, thickness);
          return;
        }
    }

  /* 2D mode or zero thickness: use standard flat rendering */
  hidgl_fill_rect (x1, y1, x2, y2);
}

void
ghid_invalidate_lr (Coord left, Coord right, Coord top, Coord bottom)
{
  ghid_invalidate_all ();
}

#define MAX_ELAPSED (50. / 1000.) /* 50ms */
void
ghid_invalidate_all ()
{
  render_priv *priv = gport->render_priv;
  double elapsed = g_timer_elapsed (priv->time_since_expose, NULL);

  ghid_draw_area_update (gport, NULL);

  if (elapsed > MAX_ELAPSED)
    gdk_window_process_all_updates ();
}

void
ghid_notify_crosshair_change (bool changes_complete)
{
  /* We sometimes get called before the GUI is up */
  if (gport->drawing_area == NULL)
    return;

  /* FIXME: We could just invalidate the bounds of the crosshair attached objects? */
  if (changes_complete) ghid_invalidate_all ();
}

void
ghid_notify_mark_change (bool changes_complete)
{
  /* We sometimes get called before the GUI is up */
  if (gport->drawing_area == NULL)
    return;

  /* FIXME: We could just invalidate the bounds of the mark? */
  if (changes_complete) ghid_invalidate_all ();
}

static void
draw_right_cross (gint x, gint y, gint z)
{
  glVertex3i (x, 0, z);
  glVertex3i (x, PCB->MaxHeight, z);
  glVertex3i (0, y, z);
  glVertex3i (PCB->MaxWidth, y, z);
}

static void
draw_slanted_cross (gint x, gint y, gint z)
{
  gint x0, y0, x1, y1;

  x0 = x + (PCB->MaxHeight - y);
  x0 = MAX(0, MIN (x0, PCB->MaxWidth));
  x1 = x - y;
  x1 = MAX(0, MIN (x1, PCB->MaxWidth));
  y0 = y + (PCB->MaxWidth - x);
  y0 = MAX(0, MIN (y0, PCB->MaxHeight));
  y1 = y - x;
  y1 = MAX(0, MIN (y1, PCB->MaxHeight));
  glVertex3i (x0, y0, z);
  glVertex3i (x1, y1, z);

  x0 = x - (PCB->MaxHeight - y);
  x0 = MAX(0, MIN (x0, PCB->MaxWidth));
  x1 = x + y;
  x1 = MAX(0, MIN (x1, PCB->MaxWidth));
  y0 = y + x;
  y0 = MAX(0, MIN (y0, PCB->MaxHeight));
  y1 = y - (PCB->MaxWidth - x);
  y1 = MAX(0, MIN (y1, PCB->MaxHeight));
  glVertex3i (x0, y0, z);
  glVertex3i (x1, y1, z);
}

static void
draw_dozen_cross (gint x, gint y, gint z)
{
  gint x0, y0, x1, y1;
  gdouble tan60 = sqrt (3);

  x0 = x + (PCB->MaxHeight - y) / tan60;
  x0 = MAX(0, MIN (x0, PCB->MaxWidth));
  x1 = x - y / tan60;
  x1 = MAX(0, MIN (x1, PCB->MaxWidth));
  y0 = y + (PCB->MaxWidth - x) * tan60;
  y0 = MAX(0, MIN (y0, PCB->MaxHeight));
  y1 = y - x * tan60;
  y1 = MAX(0, MIN (y1, PCB->MaxHeight));
  glVertex3i (x0, y0, z);
  glVertex3i (x1, y1, z);

  x0 = x + (PCB->MaxHeight - y) * tan60;
  x0 = MAX(0, MIN (x0, PCB->MaxWidth));
  x1 = x - y * tan60;
  x1 = MAX(0, MIN (x1, PCB->MaxWidth));
  y0 = y + (PCB->MaxWidth - x) / tan60;
  y0 = MAX(0, MIN (y0, PCB->MaxHeight));
  y1 = y - x / tan60;
  y1 = MAX(0, MIN (y1, PCB->MaxHeight));
  glVertex3i (x0, y0, z);
  glVertex3i (x1, y1, z);

  x0 = x - (PCB->MaxHeight - y) / tan60;
  x0 = MAX(0, MIN (x0, PCB->MaxWidth));
  x1 = x + y / tan60;
  x1 = MAX(0, MIN (x1, PCB->MaxWidth));
  y0 = y + x * tan60;
  y0 = MAX(0, MIN (y0, PCB->MaxHeight));
  y1 = y - (PCB->MaxWidth - x) * tan60;
  y1 = MAX(0, MIN (y1, PCB->MaxHeight));
  glVertex3i (x0, y0, z);
  glVertex3i (x1, y1, z);

  x0 = x - (PCB->MaxHeight - y) * tan60;
  x0 = MAX(0, MIN (x0, PCB->MaxWidth));
  x1 = x + y * tan60;
  x1 = MAX(0, MIN (x1, PCB->MaxWidth));
  y0 = y + x / tan60;
  y0 = MAX(0, MIN (y0, PCB->MaxHeight));
  y1 = y - (PCB->MaxWidth - x) / tan60;
  y1 = MAX(0, MIN (y1, PCB->MaxHeight));
  glVertex3i (x0, y0, z);
  glVertex3i (x1, y1, z);
}

static void
draw_crosshair (render_priv *priv)
{
  gint x, y, z;
  static int done_once = 0;
  static GdkColor cross_color;

  if (!done_once)
    {
      done_once = 1;
      /* FIXME: when CrossColor changed from config */
      ghid_map_color_string (Settings.CrossColor, &cross_color);
    }

  x = gport->crosshair_x;
  y = gport->crosshair_y;
  z = 0;

  glEnable (GL_COLOR_LOGIC_OP);
  glLogicOp (GL_XOR);

  glColor3f (cross_color.red / 65535.,
             cross_color.green / 65535.,
             cross_color.blue / 65535.);

  glBegin (GL_LINES);

  draw_right_cross (x, y, z);
  if (Crosshair.shape == Union_Jack_Crosshair_Shape)
    draw_slanted_cross (x, y, z);
  if (Crosshair.shape == Dozen_Crosshair_Shape)
    draw_dozen_cross (x, y, z);

  glEnd ();

  glDisable (GL_COLOR_LOGIC_OP);
}

void
ghid_init_renderer (int *argc, char ***argv, GHidPort *port)
{
  render_priv *priv;

  port->render_priv = priv = g_new0 (render_priv, 1);
  port->render_priv->crosshair_gc = gui->graphics->make_gc ();

  /* Milestone 3B: Initialize 3D layer stack */
  ghid_init_3d_layer_stack ();

  priv->time_since_expose = g_timer_new ();

  /* GTK3: No manual GL initialization needed - GtkGLArea handles this
   * automatically when the widget is realized. GL context creation,
   * configuration, and management are all done by GTK3.
   */

  /* Setup HID function pointers specific to the GL renderer*/
  ghid_hid.end_layer = ghid_end_layer;
  ghid_graphics.fill_pcb_polygon = ghid_fill_pcb_polygon;
  ghid_graphics.thindraw_pcb_polygon = ghid_thindraw_pcb_polygon;
}

void
ghid_shutdown_renderer (GHidPort *port)
{
  render_priv *priv = port->render_priv;

  gui->graphics->destroy_gc (priv->crosshair_gc);
  ghid_cancel_lead_user ();
  g_free (port->render_priv);
  port->render_priv = NULL;
}

void
ghid_init_drawing_widget (GtkWidget *widget, GHidPort *port)
{
  /* GTK3: When widget is GtkGLArea, no manual GL capability setup needed.
   * GtkGLArea automatically creates GL context with proper configuration
   * (RGBA, depth buffer, double buffering) when the widget is realized.
   *
   * Properties can be set directly on GtkGLArea if needed:
   *   gtk_gl_area_set_has_depth_buffer (GTK_GL_AREA (widget), TRUE);
   *   gtk_gl_area_set_required_version (GTK_GL_AREA (widget), 2, 1);
   *
   * For now, this function is a no-op. It will be removed once widget
   * creation is migrated to gtk_gl_area_new() in gui-top-window.c.
   */
}

void
ghid_drawing_area_configure_hook (GHidPort *port)
{
}

gboolean
ghid_start_drawing (GHidPort *port, GtkWidget *widget)
{
  /* GTK3 Milestone 3: Explicitly make GL context current.
   * This is needed for drawing operations outside the render callback
   * (e.g., in event handlers). Inside the render callback, context is
   * already current, but making it current again is harmless.
   */
  if (GTK_IS_GL_AREA (widget))
    {
      gtk_gl_area_make_current (GTK_GL_AREA (widget));

      /* Check for GL errors after making context current */
      GError *error = gtk_gl_area_get_error (GTK_GL_AREA (widget));
      if (error != NULL)
        {
          g_warning ("GL context error: %s", error->message);
          return FALSE;
        }
    }

  port->render_priv->in_context = true;
  return TRUE;
}

void
ghid_end_drawing (GHidPort *port, GtkWidget *widget)
{
  /* GTK3: GtkGLArea automatically handles buffer swapping when the render
   * callback returns. No manual swap_buffers needed.
   *
   * For explicit flushing (if needed):
   *   glFlush ();
   *
   * Context is automatically released after render callback completes.
   */
  port->render_priv->in_context = false;
}

void
ghid_screen_update (void)
{
}

#define Z_NEAR 3.0
gboolean
ghid_drawing_area_render_cb (GtkGLArea *area,
                              GdkGLContext *context,
                              GHidPort *port)
{
  render_priv *priv = port->render_priv;
  GtkWidget *widget = GTK_WIDGET (area);
  GtkAllocation allocation;
  BoxType region;
  Coord min_x, min_y;
  Coord max_x, max_y;
  Coord new_x, new_y;
  Coord min_depth;
  Coord max_depth;

  /* GTK3 Milestone 3: GtkGLArea render callback.
   * Context is automatically made current before this callback.
   * Buffer swapping is automatic after callback returns.
   */

  gtk_widget_get_allocation (widget, &allocation);

  ghid_start_drawing (port, widget);
  hidgl_start_render ();

  /* If we don't have any stencil bits available,
     we can't use the hidgl polygon drawing routine */
  /* TODO: We could use the GLU tessellator though */
  if (hidgl_stencil_bits() == 0)
    ghid_graphics.fill_pcb_polygon = common_fill_pcb_polygon;

  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glViewport (0, 0, allocation.width, allocation.height);

  /* TODO Milestone 3: Update clipping for GTK3/Cairo integration */
  glEnable (GL_SCISSOR_TEST);
  glScissor (0, 0, allocation.width, allocation.height);

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  glOrtho (0, allocation.width, allocation.height, 0, -100000, 100000);
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
  glTranslatef (allocation.width / 2., allocation.height / 2., 0);
  glMultMatrixf ((GLfloat *)view_matrix);
  glTranslatef (-allocation.width / 2., -allocation.height / 2., 0);
  glScalef ((port->view.flip_x ? -1. : 1.) / port->view.coord_per_px,
            (port->view.flip_y ? -1. : 1.) / port->view.coord_per_px,
            ((port->view.flip_x == port->view.flip_y) ? 1. : -1.) / port->view.coord_per_px);
  glTranslatef (port->view.flip_x ?  port->view.x0 - PCB->MaxWidth  :
                                    -port->view.x0,
                port->view.flip_y ?  port->view.y0 - PCB->MaxHeight :
                                    -port->view.y0, 0);
  glGetFloatv (GL_MODELVIEW_MATRIX, (GLfloat *)last_modelview_matrix);

  glEnable (GL_STENCIL_TEST);
  glClearColor (port->offlimits_color.red / 65535.,
                port->offlimits_color.green / 65535.,
                port->offlimits_color.blue / 65535.,
                1.);
  glStencilMask (~0);
  glClearStencil (0);
  glClear (GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  hidgl_reset_stencil_usage ();

  /* Disable the stencil test until we need it - otherwise it gets dirty */
  glDisable (GL_STENCIL_TEST);
  glStencilMask (0);
  glStencilFunc (GL_ALWAYS, 0, 0);

  /* GTK3: GtkGLArea render callback always renders full widget.
   * Use allocation dimensions instead of ev->area (which doesn't exist).
   * Test the 8 corners of a cube spanning the widget.
   */
  min_depth = -50; /* FIXME */
  max_depth =  0;  /* FIXME */

  ghid_unproject_to_z_plane (0,
                             0,
                             min_depth, &new_x, &new_y);
  max_x = min_x = new_x;
  max_y = min_y = new_y;

  ghid_unproject_to_z_plane (0,
                             0,
                             max_depth, &new_x, &new_y);
  min_x = MIN (min_x, new_x);  max_x = MAX (max_x, new_x);
  min_y = MIN (min_y, new_y);  max_y = MAX (max_y, new_y);

  /* */
  ghid_unproject_to_z_plane (allocation.width,
                             0,
                             min_depth, &new_x, &new_y);
  min_x = MIN (min_x, new_x);  max_x = MAX (max_x, new_x);
  min_y = MIN (min_y, new_y);  max_y = MAX (max_y, new_y);

  ghid_unproject_to_z_plane (allocation.width, 0,
                             max_depth, &new_x, &new_y);
  min_x = MIN (min_x, new_x);  max_x = MAX (max_x, new_x);
  min_y = MIN (min_y, new_y);  max_y = MAX (max_y, new_y);

  /* */
  ghid_unproject_to_z_plane (allocation.width,
                             allocation.height,
                             min_depth, &new_x, &new_y);
  min_x = MIN (min_x, new_x);  max_x = MAX (max_x, new_x);
  min_y = MIN (min_y, new_y);  max_y = MAX (max_y, new_y);

  ghid_unproject_to_z_plane (allocation.width,
                             allocation.height,
                             max_depth, &new_x, &new_y);
  min_x = MIN (min_x, new_x);  max_x = MAX (max_x, new_x);
  min_y = MIN (min_y, new_y);  max_y = MAX (max_y, new_y);

  /* */
  ghid_unproject_to_z_plane (0,
                             allocation.height,
                             min_depth,
                             &new_x, &new_y);
  min_x = MIN (min_x, new_x);  max_x = MAX (max_x, new_x);
  min_y = MIN (min_y, new_y);  max_y = MAX (max_y, new_y);

  ghid_unproject_to_z_plane (0,
                             allocation.height,
                             max_depth,
                             &new_x, &new_y);
  min_x = MIN (min_x, new_x);  max_x = MAX (max_x, new_x);
  min_y = MIN (min_y, new_y);  max_y = MAX (max_y, new_y);

  region.X1 = min_x;  region.X2 = max_x + 1;
  region.Y1 = min_y;  region.Y2 = max_y + 1;

  region.X1 = MAX (0, MIN (PCB->MaxWidth,  region.X1));
  region.X2 = MAX (0, MIN (PCB->MaxWidth,  region.X2));
  region.Y1 = MAX (0, MIN (PCB->MaxHeight, region.Y1));
  region.Y2 = MAX (0, MIN (PCB->MaxHeight, region.Y2));

  glColor3f (port->bg_color.red / 65535.,
             port->bg_color.green / 65535.,
             port->bg_color.blue / 65535.);

  glBegin (GL_QUADS);
  glVertex3i (0,             0,              -50);
  glVertex3i (PCB->MaxWidth, 0,              -50);
  glVertex3i (PCB->MaxWidth, PCB->MaxHeight, -50);
  glVertex3i (0,             PCB->MaxHeight, -50);
  glEnd ();

  ghid_draw_bg_image ();

  ghid_invalidate_current_gc ();
  hid_expose_callback (&ghid_hid, &region, 0);
  hidgl_flush_triangles (&buffer);

  ghid_graphics.draw_grid (&region);

  ghid_invalidate_current_gc ();

  DrawAttached (priv->crosshair_gc);
  DrawMark (priv->crosshair_gc);
  hidgl_flush_triangles (&buffer);

  draw_crosshair (priv);
  hidgl_flush_triangles (&buffer);

  draw_lead_user (priv);

  hidgl_finish_render ();
  ghid_end_drawing (port, widget);

  g_timer_start (priv->time_since_expose);

  return FALSE;
}

/* GTK3 Milestone 3A: Realize callback for one-time OpenGL initialization.
 * Called after GL context is created but before first render.
 */
void
ghid_port_drawing_realize_cb (GtkWidget *widget, gpointer data)
{
  GHidPort *port = (GHidPort *) data;

  /* Make GL context current for initialization */
  if (GTK_IS_GL_AREA (widget))
    {
      gtk_gl_area_make_current (GTK_GL_AREA (widget));

      /* Check for GL errors during context creation */
      GError *error = gtk_gl_area_get_error (GTK_GL_AREA (widget));
      if (error != NULL)
        {
          g_warning ("Failed to initialize GL: %s", error->message);
          return;
        }

      /* One-time GL state initialization */
      glEnable (GL_DEPTH_TEST);
      glDepthFunc (GL_LEQUAL);
      glClearDepth (1.0);

      /* Enable smooth shading */
      glShadeModel (GL_SMOOTH);

      /* Enable blending for transparency */
      glEnable (GL_BLEND);
      glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      /* Line smoothing for better visual quality */
      glEnable (GL_LINE_SMOOTH);
      glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);

      /* Initialize stencil buffer usage tracking */
      hidgl_reset_stencil_usage ();

      /* Report GL capabilities */
      const GLubyte *vendor = glGetString (GL_VENDOR);
      const GLubyte *renderer = glGetString (GL_RENDERER);
      const GLubyte *version = glGetString (GL_VERSION);
      g_message ("OpenGL initialized: %s / %s / %s", vendor, renderer, version);

      /* Check available stencil bits */
      GLint stencil_bits = hidgl_stencil_bits ();
      g_message ("Available stencil bits: %d", stencil_bits);
    }
}

gboolean
ghid_pinout_preview_draw (GtkWidget *widget,
                          cairo_t *cr)
{
  GhidPinoutPreview *pinout = GHID_PINOUT_PREVIEW (widget);
  GtkAllocation allocation;
  view_data save_view;
  int save_width, save_height;
  Coord save_max_width;
  Coord save_max_height;
  double xz, yz;

  /* NOTE: This is the OpenGL rendering path. Full migration to GTK3
   * OpenGL (GtkGLExt -> GtkGLArea) is scheduled for Milestone 3.
   * The cairo_t parameter is not used in OpenGL rendering.
   */

  save_view = gport->view;
  save_width = gport->width;
  save_height = gport->height;
  save_max_width = PCB->MaxWidth;
  save_max_height = PCB->MaxHeight;

  /* Setup zoom factor for drawing routines */

  gtk_widget_get_allocation (widget, &allocation);
  xz = (double) pinout->x_max / allocation.width;
  yz = (double) pinout->y_max / allocation.height;
  if (xz > yz)
    gport->view.coord_per_px = xz;
  else
    gport->view.coord_per_px = yz;

  gport->width = allocation.width;
  gport->height = allocation.height;
  gport->view.width = allocation.width * gport->view.coord_per_px;
  gport->view.height = allocation.height * gport->view.coord_per_px;
  gport->view.x0 = (pinout->x_max - gport->view.width) / 2;
  gport->view.y0 = (pinout->y_max - gport->view.height) / 2;
  PCB->MaxWidth = pinout->x_max;
  PCB->MaxHeight = pinout->y_max;

  ghid_start_drawing (gport, widget);
  hidgl_start_render ();

#if 0  /* We disable alpha blending here, as hid_expose_callback() does not
        * call set_layer() as appropriate for us to sub-composite rendering
        * from each layer when drawing a single element. If we leave alpha-
        * blending on, it means text and overlapping pads are rendered ugly.
        */

  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif

  glViewport (0, 0, allocation.width, allocation.height);

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  glOrtho (0, allocation.width, allocation.height, 0, -100000, 100000);
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
  glTranslatef (0.0f, 0.0f, -Z_NEAR);

  glClearColor (gport->bg_color.red / 65535.,
                gport->bg_color.green / 65535.,
                gport->bg_color.blue / 65535.,
                1.);
  glStencilMask (~0);
  glClearStencil (0);
  glClear (GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  hidgl_reset_stencil_usage ();

  /* Disable the stencil test until we need it - otherwise it gets dirty */
  glDisable (GL_STENCIL_TEST);
  glStencilMask (0);
  glStencilFunc (GL_ALWAYS, 0, 0);

  /* call the drawing routine */
  ghid_invalidate_current_gc ();
  glPushMatrix ();
  glScalef ((gport->view.flip_x ? -1. : 1.) / gport->view.coord_per_px,
            (gport->view.flip_y ? -1. : 1.) / gport->view.coord_per_px,
            ((gport->view.flip_x == gport->view.flip_y) ? 1. : -1.) / gport->view.coord_per_px);
  glTranslatef (gport->view.flip_x ? gport->view.x0 - PCB->MaxWidth  :
                                    -gport->view.x0,
                gport->view.flip_y ? gport->view.y0 - PCB->MaxHeight :
                                    -gport->view.y0, 0);

  hid_expose_callback (&ghid_hid, NULL, pinout->element);
  hidgl_flush_triangles (&buffer);
  glPopMatrix ();

  hidgl_finish_render ();
  ghid_end_drawing (gport, widget);

  gport->view = save_view;
  gport->width = save_width;
  gport->height = save_height;
  PCB->MaxWidth = save_max_width;
  PCB->MaxHeight = save_max_height;

  return FALSE;
}


GdkPixmap *
ghid_render_pixmap (int cx, int cy, double zoom, int width, int height, int depth)
{
  /* TODO GTK3: This function still uses GtkGLExt for offscreen OpenGL rendering.
   * Migration to GTK3 requires one of:
   *   1. Use GtkGLArea offscreen rendering (gtk_gl_area_attach_buffers + FBO)
   *   2. Use native OpenGL FBO (framebuffer object) rendering
   *   3. Rewrite using Cairo for offscreen rendering
   * This is deferred from Milestone 3 as it's not critical for main display.
   * The function is used for export/print operations that render to pixmap.
   */
  GdkGLConfig *glconfig;
  GdkPixmap *pixmap;
  GdkGLPixmap *glpixmap;
  GdkGLContext* glcontext;
  GdkGLDrawable* gldrawable;
  view_data save_view;
  int save_width, save_height;
  BoxType region;

  save_view = gport->view;
  save_width = gport->width;
  save_height = gport->height;

  /* Setup rendering context for drawing routines
   */

  glconfig = gdk_gl_config_new_by_mode (GDK_GL_MODE_RGB     |
                                        GDK_GL_MODE_STENCIL |
                                        GDK_GL_MODE_SINGLE);

  pixmap = gdk_pixmap_new (NULL, width, height, depth);
  glpixmap = gdk_pixmap_set_gl_capability (pixmap, glconfig, NULL);
  gldrawable = GDK_GL_DRAWABLE (glpixmap);
  glcontext = gdk_gl_context_new (gldrawable, NULL, TRUE, GDK_GL_RGBA_TYPE);

  /* Setup zoom factor for drawing routines */

  gport->view.coord_per_px = zoom;
  gport->width = width;
  gport->height = height;
  gport->view.width = width * gport->view.coord_per_px;
  gport->view.height = height * gport->view.coord_per_px;
  gport->view.x0 = gport->view.flip_x ? PCB->MaxWidth - cx : cx;
  gport->view.x0 -= gport->view.height / 2;
  gport->view.y0 = gport->view.flip_y ? PCB->MaxHeight - cy : cy;
  gport->view.y0 -= gport->view.width / 2;

  /* make GL-context "current" */
  if (!gdk_gl_drawable_gl_begin (gldrawable, glcontext)) {
    return NULL;
  }
  hidgl_start_render ();
  gport->render_priv->in_context = true;

  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glViewport (0, 0, width, height);

  glEnable (GL_SCISSOR_TEST);
  glScissor (0, 0, width, height);

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  glOrtho (0, width, height, 0, -100000, 100000);
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
  glTranslatef (0.0f, 0.0f, -Z_NEAR);

  glClearColor (gport->bg_color.red / 65535.,
                gport->bg_color.green / 65535.,
                gport->bg_color.blue / 65535.,
                1.);
  glStencilMask (~0);
  glClearStencil (0);
  glClear (GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  hidgl_reset_stencil_usage ();

  /* Disable the stencil test until we need it - otherwise it gets dirty */
  glDisable (GL_STENCIL_TEST);
  glStencilMask (0);
  glStencilFunc (GL_ALWAYS, 0, 0);

  /* call the drawing routine */
  ghid_invalidate_current_gc ();
  glPushMatrix ();
  glScalef ((gport->view.flip_x ? -1. : 1.) / gport->view.coord_per_px,
            (gport->view.flip_y ? -1. : 1.) / gport->view.coord_per_px,
            ((gport->view.flip_x == gport->view.flip_y) ? 1. : -1.) / gport->view.coord_per_px);
  glTranslatef (gport->view.flip_x ? gport->view.x0 - PCB->MaxWidth  :
                                    -gport->view.x0,
                gport->view.flip_y ? gport->view.y0 - PCB->MaxHeight :
                                    -gport->view.y0, 0);

  region.X1 = MIN(Px(0), Px(gport->width + 1));
  region.Y1 = MIN(Py(0), Py(gport->height + 1));
  region.X2 = MAX(Px(0), Px(gport->width + 1));
  region.Y2 = MAX(Py(0), Py(gport->height + 1));

  region.X1 = MAX (0, MIN (PCB->MaxWidth,  region.X1));
  region.X2 = MAX (0, MIN (PCB->MaxWidth,  region.X2));
  region.Y1 = MAX (0, MIN (PCB->MaxHeight, region.Y1));
  region.Y2 = MAX (0, MIN (PCB->MaxHeight, region.Y2));

  hid_expose_callback (&ghid_hid, &region, NULL);
  hidgl_flush_triangles (&buffer);
  glPopMatrix ();

  glFlush ();

  hidgl_finish_render ();

  /* end drawing to current GL-context */
  gport->render_priv->in_context = false;
  gdk_gl_drawable_gl_end (gldrawable);

  gdk_pixmap_unset_gl_capability (pixmap);

  g_object_unref (glconfig);
  g_object_unref (glcontext);

  gport->view = save_view;
  gport->width = save_width;
  gport->height = save_height;

  return pixmap;
}

HID_DRAW *
ghid_request_debug_draw (void)
{
  GHidPort *port = gport;
  GtkWidget *widget = port->drawing_area;
  GtkAllocation allocation;

  gtk_widget_get_allocation (widget, &allocation);

  ghid_start_drawing (port, widget);
  hidgl_start_render ();

  glViewport (0, 0, allocation.width, allocation.height);

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  glOrtho (0, allocation.width, allocation.height, 0, 0, 100);
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
  glTranslatef (0.0f, 0.0f, -Z_NEAR);

  ghid_invalidate_current_gc ();

  /* Setup stenciling */
  glDisable (GL_STENCIL_TEST);

  glPushMatrix ();
  glScalef ((port->view.flip_x ? -1. : 1.) / port->view.coord_per_px,
            (port->view.flip_y ? -1. : 1.) / port->view.coord_per_px,
            ((gport->view.flip_x == port->view.flip_y) ? 1. : -1.) / gport->view.coord_per_px);
  glTranslatef (port->view.flip_x ? port->view.x0 - PCB->MaxWidth  :
                             -port->view.x0,
                port->view.flip_y ? port->view.y0 - PCB->MaxHeight :
                             -port->view.y0, 0);

  return ghid_hid.graphics;
}

void
ghid_flush_debug_draw (void)
{
  /* GTK3: GtkGLArea automatically swaps buffers when render callback returns.
   * For immediate drawing outside render callback, just flush.
   */
  hidgl_flush_triangles (&buffer);
  glFlush ();
}

void
ghid_finish_debug_draw (void)
{
  hidgl_flush_triangles (&buffer);
  glPopMatrix ();

  hidgl_finish_render ();
  ghid_end_drawing (gport, gport->drawing_area);
}

static float
determinant_2x2 (float m[2][2])
{
  float det;
  det = m[0][0] * m[1][1] -
        m[0][1] * m[1][0];
  return det;
}

#if 0
static float
determinant_4x4 (float m[4][4])
{
  float det;
  det = m[0][3] * m[1][2] * m[2][1] * m[3][0]-m[0][2] * m[1][3] * m[2][1] * m[3][0] -
        m[0][3] * m[1][1] * m[2][2] * m[3][0]+m[0][1] * m[1][3] * m[2][2] * m[3][0] +
        m[0][2] * m[1][1] * m[2][3] * m[3][0]-m[0][1] * m[1][2] * m[2][3] * m[3][0] -
        m[0][3] * m[1][2] * m[2][0] * m[3][1]+m[0][2] * m[1][3] * m[2][0] * m[3][1] +
        m[0][3] * m[1][0] * m[2][2] * m[3][1]-m[0][0] * m[1][3] * m[2][2] * m[3][1] -
        m[0][2] * m[1][0] * m[2][3] * m[3][1]+m[0][0] * m[1][2] * m[2][3] * m[3][1] +
        m[0][3] * m[1][1] * m[2][0] * m[3][2]-m[0][1] * m[1][3] * m[2][0] * m[3][2] -
        m[0][3] * m[1][0] * m[2][1] * m[3][2]+m[0][0] * m[1][3] * m[2][1] * m[3][2] +
        m[0][1] * m[1][0] * m[2][3] * m[3][2]-m[0][0] * m[1][1] * m[2][3] * m[3][2] -
        m[0][2] * m[1][1] * m[2][0] * m[3][3]+m[0][1] * m[1][2] * m[2][0] * m[3][3] +
        m[0][2] * m[1][0] * m[2][1] * m[3][3]-m[0][0] * m[1][2] * m[2][1] * m[3][3] -
        m[0][1] * m[1][0] * m[2][2] * m[3][3]+m[0][0] * m[1][1] * m[2][2] * m[3][3];
   return det;
}
#endif

static void
invert_2x2 (float m[2][2], float out[2][2])
{
  float scale = 1 / determinant_2x2 (m);
  out[0][0] =  m[1][1] * scale;
  out[0][1] = -m[0][1] * scale;
  out[1][0] = -m[1][0] * scale;
  out[1][1] =  m[0][0] * scale;
}

#if 0
static void
invert_4x4 (float m[4][4], float out[4][4])
{
  float scale = 1 / determinant_4x4 (m);

  out[0][0] = (m[1][2] * m[2][3] * m[3][1] - m[1][3] * m[2][2] * m[3][1] +
               m[1][3] * m[2][1] * m[3][2] - m[1][1] * m[2][3] * m[3][2] -
               m[1][2] * m[2][1] * m[3][3] + m[1][1] * m[2][2] * m[3][3]) * scale;
  out[0][1] = (m[0][3] * m[2][2] * m[3][1] - m[0][2] * m[2][3] * m[3][1] -
               m[0][3] * m[2][1] * m[3][2] + m[0][1] * m[2][3] * m[3][2] +
               m[0][2] * m[2][1] * m[3][3] - m[0][1] * m[2][2] * m[3][3]) * scale;
  out[0][2] = (m[0][2] * m[1][3] * m[3][1] - m[0][3] * m[1][2] * m[3][1] +
               m[0][3] * m[1][1] * m[3][2] - m[0][1] * m[1][3] * m[3][2] -
               m[0][2] * m[1][1] * m[3][3] + m[0][1] * m[1][2] * m[3][3]) * scale;
  out[0][3] = (m[0][3] * m[1][2] * m[2][1] - m[0][2] * m[1][3] * m[2][1] -
               m[0][3] * m[1][1] * m[2][2] + m[0][1] * m[1][3] * m[2][2] +
               m[0][2] * m[1][1] * m[2][3] - m[0][1] * m[1][2] * m[2][3]) * scale;
  out[1][0] = (m[1][3] * m[2][2] * m[3][0] - m[1][2] * m[2][3] * m[3][0] -
               m[1][3] * m[2][0] * m[3][2] + m[1][0] * m[2][3] * m[3][2] +
               m[1][2] * m[2][0] * m[3][3] - m[1][0] * m[2][2] * m[3][3]) * scale;
  out[1][1] = (m[0][2] * m[2][3] * m[3][0] - m[0][3] * m[2][2] * m[3][0] +
               m[0][3] * m[2][0] * m[3][2] - m[0][0] * m[2][3] * m[3][2] -
               m[0][2] * m[2][0] * m[3][3] + m[0][0] * m[2][2] * m[3][3]) * scale;
  out[1][2] = (m[0][3] * m[1][2] * m[3][0] - m[0][2] * m[1][3] * m[3][0] -
               m[0][3] * m[1][0] * m[3][2] + m[0][0] * m[1][3] * m[3][2] +
               m[0][2] * m[1][0] * m[3][3] - m[0][0] * m[1][2] * m[3][3]) * scale;
  out[1][3] = (m[0][2] * m[1][3] * m[2][0] - m[0][3] * m[1][2] * m[2][0] +
               m[0][3] * m[1][0] * m[2][2] - m[0][0] * m[1][3] * m[2][2] -
               m[0][2] * m[1][0] * m[2][3] + m[0][0] * m[1][2] * m[2][3]) * scale;
  out[2][0] = (m[1][1] * m[2][3] * m[3][0] - m[1][3] * m[2][1] * m[3][0] +
               m[1][3] * m[2][0] * m[3][1] - m[1][0] * m[2][3] * m[3][1] -
               m[1][1] * m[2][0] * m[3][3] + m[1][0] * m[2][1] * m[3][3]) * scale;
  out[2][1] = (m[0][3] * m[2][1] * m[3][0] - m[0][1] * m[2][3] * m[3][0] -
               m[0][3] * m[2][0] * m[3][1] + m[0][0] * m[2][3] * m[3][1] +
               m[0][1] * m[2][0] * m[3][3] - m[0][0] * m[2][1] * m[3][3]) * scale;
  out[2][2] = (m[0][1] * m[1][3] * m[3][0] - m[0][3] * m[1][1] * m[3][0] +
               m[0][3] * m[1][0] * m[3][1] - m[0][0] * m[1][3] * m[3][1] -
               m[0][1] * m[1][0] * m[3][3] + m[0][0] * m[1][1] * m[3][3]) * scale;
  out[2][3] = (m[0][3] * m[1][1] * m[2][0] - m[0][1] * m[1][3] * m[2][0] -
               m[0][3] * m[1][0] * m[2][1] + m[0][0] * m[1][3] * m[2][1] +
               m[0][1] * m[1][0] * m[2][3] - m[0][0] * m[1][1] * m[2][3]) * scale;
  out[3][0] = (m[1][2] * m[2][1] * m[3][0] - m[1][1] * m[2][2] * m[3][0] -
               m[1][2] * m[2][0] * m[3][1] + m[1][0] * m[2][2] * m[3][1] +
               m[1][1] * m[2][0] * m[3][2] - m[1][0] * m[2][1] * m[3][2]) * scale;
  out[3][1] = (m[0][1] * m[2][2] * m[3][0] - m[0][2] * m[2][1] * m[3][0] +
               m[0][2] * m[2][0] * m[3][1] - m[0][0] * m[2][2] * m[3][1] -
               m[0][1] * m[2][0] * m[3][2] + m[0][0] * m[2][1] * m[3][2]) * scale;
  out[3][2] = (m[0][2] * m[1][1] * m[3][0] - m[0][1] * m[1][2] * m[3][0] -
               m[0][2] * m[1][0] * m[3][1] + m[0][0] * m[1][2] * m[3][1] +
               m[0][1] * m[1][0] * m[3][2] - m[0][0] * m[1][1] * m[3][2]) * scale;
  out[3][3] = (m[0][1] * m[1][2] * m[2][0] - m[0][2] * m[1][1] * m[2][0] +
               m[0][2] * m[1][0] * m[2][1] - m[0][0] * m[1][2] * m[2][1] -
               m[0][1] * m[1][0] * m[2][2] + m[0][0] * m[1][1] * m[2][2]) * scale;
}
#endif


static void
ghid_unproject_to_z_plane (int ex, int ey, Coord pcb_z, Coord *pcb_x, Coord *pcb_y)
{
  float mat[2][2];
  float inv_mat[2][2];
  float x, y;

  /*
    ex = view_matrix[0][0] * vx +
         view_matrix[0][1] * vy +
         view_matrix[0][2] * vz +
         view_matrix[0][3] * 1;
    ey = view_matrix[1][0] * vx +
         view_matrix[1][1] * vy +
         view_matrix[1][2] * vz +
         view_matrix[1][3] * 1;
    UNKNOWN ez = view_matrix[2][0] * vx +
                 view_matrix[2][1] * vy +
                 view_matrix[2][2] * vz +
                 view_matrix[2][3] * 1;

    ex - view_matrix[0][3] * 1
       - view_matrix[0][2] * vz
      = view_matrix[0][0] * vx +
        view_matrix[0][1] * vy;

    ey - view_matrix[1][3] * 1
       - view_matrix[1][2] * vz
      = view_matrix[1][0] * vx +
        view_matrix[1][1] * vy;
  */

  /* NB: last_modelview_matrix is transposed in memory! */
  x = (float)ex - last_modelview_matrix[3][0] * 1
                - last_modelview_matrix[2][0] * pcb_z;

  y = (float)ey - last_modelview_matrix[3][1] * 1
                - last_modelview_matrix[2][1] * pcb_z;

  /*
    x = view_matrix[0][0] * vx +
        view_matrix[0][1] * vy;

    y = view_matrix[1][0] * vx +
        view_matrix[1][1] * vy;

    [view_matrix[0][0] view_matrix[0][1]] [vx] = [x]
    [view_matrix[1][0] view_matrix[1][1]] [vy]   [y]
  */

  mat[0][0] = last_modelview_matrix[0][0];
  mat[0][1] = last_modelview_matrix[1][0];
  mat[1][0] = last_modelview_matrix[0][1];
  mat[1][1] = last_modelview_matrix[1][1];

  /*    if (determinant_2x2 (mat) < 0.00001)       */
  /*      printf ("Determinant is quite small\n"); */

  invert_2x2 (mat, inv_mat);

  *pcb_x = (int)(inv_mat[0][0] * x + inv_mat[0][1] * y);
  *pcb_y = (int)(inv_mat[1][0] * x + inv_mat[1][1] * y);
}


bool
ghid_event_to_pcb_coords (int event_x, int event_y, Coord *pcb_x, Coord *pcb_y)
{
  ghid_unproject_to_z_plane (event_x, event_y, 0, pcb_x, pcb_y);

  return true;
}

bool
ghid_pcb_to_event_coords (Coord pcb_x, Coord pcb_y, int *event_x, int *event_y)
{
  /* NB: last_modelview_matrix is transposed in memory */
  float w = last_modelview_matrix[0][3] * (float)pcb_x +
            last_modelview_matrix[1][3] * (float)pcb_y +
            last_modelview_matrix[2][3] * 0. +
            last_modelview_matrix[3][3] * 1.;

  *event_x = (last_modelview_matrix[0][0] * (float)pcb_x +
              last_modelview_matrix[1][0] * (float)pcb_y +
              last_modelview_matrix[2][0] * 0. +
              last_modelview_matrix[3][0] * 1.) / w;
  *event_y = (last_modelview_matrix[0][1] * (float)pcb_x +
              last_modelview_matrix[1][1] * (float)pcb_y +
              last_modelview_matrix[2][1] * 0. +
              last_modelview_matrix[3][1] * 1.) / w;

  return true;
}

void
ghid_view_2d (void *ball, gboolean view_2d, gpointer userdata)
{
  global_view_2d = view_2d;
  ghid_invalidate_all ();
}

void
ghid_port_rotate (void *ball, float *quarternion, gpointer userdata)
{
#ifdef DEBUG_ROTATE
  int row, column;
#endif

  build_rotmatrix (view_matrix, quarternion);

#ifdef DEBUG_ROTATE
  for (row = 0; row < 4; row++) {
    printf ("[ %f", view_matrix[row][0]);
    for (column = 1; column < 4; column++) {
      printf (",\t%f", view_matrix[row][column]);
    }
    printf ("\t]\n");
  }
  printf ("\n");
#endif

  ghid_invalidate_all ();
}


#define LEAD_USER_WIDTH           0.2          /* millimeters */
#define LEAD_USER_PERIOD          (1000 / 20)  /* 20fps (in ms) */
#define LEAD_USER_VELOCITY        3.           /* millimeters per second */
#define LEAD_USER_ARC_COUNT       3
#define LEAD_USER_ARC_SEPARATION  3.           /* millimeters */
#define LEAD_USER_INITIAL_RADIUS  10.          /* millimetres */
#define LEAD_USER_COLOR_R         1.
#define LEAD_USER_COLOR_G         1.
#define LEAD_USER_COLOR_B         0.

static void
draw_lead_user (render_priv *priv)
{
  int i;
  double radius = priv->lead_user_radius;
  double width = MM_TO_COORD (LEAD_USER_WIDTH);
  double separation = MM_TO_COORD (LEAD_USER_ARC_SEPARATION);

  if (!priv->lead_user)
    return;

  glPushAttrib (GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
  glEnable (GL_COLOR_LOGIC_OP);
  glLogicOp (GL_XOR);
  glColor3f (LEAD_USER_COLOR_R, LEAD_USER_COLOR_G,LEAD_USER_COLOR_B);


  /* arcs at the approrpriate radii */

  for (i = 0; i < LEAD_USER_ARC_COUNT; i++, radius -= separation)
    {
      if (radius < width)
        radius += MM_TO_COORD (LEAD_USER_INITIAL_RADIUS);

      /* Draw an arc at radius */
      hidgl_draw_arc (width, priv->lead_user_x, priv->lead_user_y,
                      radius, radius, 0, 360, gport->view.coord_per_px);
    }

  hidgl_flush_triangles (&buffer);
  glPopAttrib ();
}

gboolean
lead_user_cb (gpointer data)
{
  render_priv *priv = data;
  Coord step;
  double elapsed_time;

  /* Queue a redraw */
  ghid_invalidate_all ();

  /* Update radius */
  elapsed_time = g_timer_elapsed (priv->lead_user_timer, NULL);
  g_timer_start (priv->lead_user_timer);

  step = MM_TO_COORD (LEAD_USER_VELOCITY * elapsed_time);
  if (priv->lead_user_radius > step)
    priv->lead_user_radius -= step;
  else
    priv->lead_user_radius = MM_TO_COORD (LEAD_USER_INITIAL_RADIUS);

  return TRUE;
}

void
ghid_lead_user_to_location (Coord x, Coord y)
{
  render_priv *priv = gport->render_priv;

  ghid_cancel_lead_user ();

  priv->lead_user = true;
  priv->lead_user_x = x;
  priv->lead_user_y = y;
  priv->lead_user_radius = MM_TO_COORD (LEAD_USER_INITIAL_RADIUS);
  priv->lead_user_timeout = g_timeout_add (LEAD_USER_PERIOD, lead_user_cb, priv);
  priv->lead_user_timer = g_timer_new ();
}

void
ghid_cancel_lead_user (void)
{
  render_priv *priv = gport->render_priv;

  if (priv->lead_user_timeout)
    g_source_remove (priv->lead_user_timeout);

  if (priv->lead_user_timer)
    g_timer_destroy (priv->lead_user_timer);

  if (priv->lead_user)
    ghid_invalidate_all ();

  priv->lead_user_timeout = 0;
  priv->lead_user_timer = NULL;
  priv->lead_user = false;
}
