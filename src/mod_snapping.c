//
//  mod_snapping.c
//  
//  A plugin to change the snapping behavior when a modifier key is pressed.
//
//  Created by Parker, Charles W. on 7/11/18.
//
//  Build from pcb/src with:
//  Linux:
//  gcc -fPIC -shared -o myplugin.so \
//       myplugin.c -I.. -I/opt/local/include/glib-2.0 \
//       -I/opt/local/lib/glib-2.0/include -DHAVE_CONFIG_H
//  MacOS:
//  gcc -fPIC -dynamiclib -Wl,-undefined,dynamic_lookup -o myplugin.so \
//       myplugin.c -I.. -I/opt/local/include/glib-2.0 \
//       -I/opt/local/lib/glib-2.0/include -DHAVE_CONFIG_H
//
//  Make sure that the resulting shared object file (dynamic library) produces
//  the following output from the "file" function:
//  $ file myplugin.so
//  Linux:
//  myplugin.so: ELF 64-bit LSB shared object, x86-64, version 1 (SYSV), dynamically linked, with debug_info, not stripped
//  MacOS:
//  myplugin.so: Mach-O 64-bit dynamically linked shared library x86_64
//
//

#include <stdio.h>

#include "globalconst.h"
#include "global.h" // types

#include "crosshair.h"
#include "snap.h"
#include "error.h" // Message

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "hid/gtk/gui.h" // gport

SnapListType * mod_list;
SnapListType * norm_list;

/*
 * Event handler to switch the lists 
 */
static gboolean
mod_snap_key_event(G_GNUC_UNUSED GtkWidget *widget,
          GdkEventKey *event)
{
	if (event->keyval == GDK_KEY_Alt_L || event->keyval == GDK_KEY_Alt_R){
	  //fprintf(stderr, "mod event... ");
      if (ghid_mod1_is_pressed()){
        Crosshair.snaps = mod_list;
        //fprintf(stderr, "mod pressed\n");
      } else {
        Crosshair.snaps = norm_list;
        //fprintf(stderr, "mod released\n");
      }
	}

    return FALSE;
}

/* There's a version of this defined in action.c. It should really be
 * defined more globally so that other functions can use it, but right now
 * it's not. */
#ifndef ARG
  #define ARG(n) (argc > (n) ? argv[n] : NULL)
#endif

static const char mod_snap_set_property_help[] = "Change the property of a given snap in the modifier set.";
static const char mod_snap_set_property_syntax[] = "ModSnapSetProperty(<name>, <property>, <new value>)";

/*! \brief Set the specified property of the specified snap. Copied from
 * snap.c and modified to affect the mod_list instead of the crosshair list. */
static int
ActionModSnapSetProperty(int argc, char **argv, Coord x, Coord y)
{
  char *name = ARG(0);
  char *prop = ARG(1);
  char *val = ARG(2);
  bool absolute;
  Coord value;
  SnapSpecType * spec;

  /* Sanity check the inputs */
  if (argc != 3 || !name || !prop || !val) {
	Message("ModSnapSetProperty: exactly 3 arguments required.\n");
	return -1;
  }

  /* Find the snap we need to change*/
  spec = snap_list_find_snap_by_name(mod_list, name);
  if (!spec) {
	Message("ModSnapSetProperty: snap %s not found.\n");
	return -1;
  }

  // figure out what property is to be changed, and change it.
  if (strcasecmp (prop, "Enabled") == 0) {
	  // we should have a general function for converting strings to
	  // booleans, or ints, or whatever.
    if (   strcasecmp (val, "true") == 0
	    || strcasecmp (val, "on") == 0
	    || strcasecmp (val, "1") == 0) {
	  spec->enabled = true;
    } else if (
		   strcasecmp (val, "false") == 0
	    || strcasecmp (val, "off") == 0
	    || strcasecmp (val, "0") == 0) {
	  spec->enabled = false;
	} else {
	  Message("ModSnapSetProperty: Invalid value for property \"enabled\": %s.\n", val);
	  return -1;
	}

  } else if (strcasecmp (prop, "Radius") == 0) {
    value = GetValue(val, NULL, &absolute);
	if (absolute) spec->radius = value;
	else spec->radius += value;

  } else if (strcasecmp (prop, "Priority") == 0) {
	SnapSpecType * snap_copy = snap_spec_copy(spec);
	value = GetUnitlessValue(val, &absolute);
    snap_list_remove_snap_by_name(mod_list, spec->name);
    if (absolute) snap_copy->priority = value;
	else snap_copy->priority += value;
    snap_list_add_snap(mod_list, snap_copy);

  } else {
    Message("ModSnapSetProperty: Invalid proprty: %s.\n", prop);
	return -1;
  }
  return 0;
}

static int mod_snapping_initialized = 0;

static gulong mod_snap_key_press_handler;
static gulong mod_snap_key_release_handler;

static const char mod_snap_enable_help[] = "Enable or disable the mod_snap plugin";
static const char mod_snap_enable_syntax[] = "ModSnapEnable(true|false)";

/*! \brief Set the specified property of the specified snap. Copied from
 * snap.c and modified to affect the mod_list instead of the crosshair list. */
static int
ActionModSnapEnable(int argc, char **argv, Coord x, Coord y)
{
  char *val = ARG(0);
  if (!mod_snapping_initialized) mod_snapping_plugin_init();
  
  /* Sanity check the inputs */
  if (argc != 1 || !val) {
	Message("ModSnapEnable: exactly 1 arguments required.\n");
	return -1;
  }

  // we should have a general function for converting strings to
  // booleans, or ints, or whatever.
  if (   strcasecmp (val, "true") == 0
      || strcasecmp (val, "on") == 0
      || strcasecmp (val, "1") == 0) {
    // save the default list as the "mod" list
    mod_list = Crosshair.snaps;
    Crosshair.snaps = norm_list;
    mod_snap_key_press_handler = 
      g_signal_connect(G_OBJECT(gport->drawing_area),
                       "key-press-event",
                       G_CALLBACK(mod_snap_key_event),
                       NULL);
    mod_snap_key_release_handler =
      g_signal_connect(G_OBJECT(gport->drawing_area),
                       "key-release-event",
                       G_CALLBACK(mod_snap_key_event),
                       NULL);
  } else if (
		   strcasecmp (val, "false") == 0
	    || strcasecmp (val, "off") == 0
	    || strcasecmp (val, "0") == 0) {
    Crosshair.snaps = mod_list;
	g_signal_handler_disconnect(gport->drawing_area, mod_snap_key_press_handler);
	g_signal_handler_disconnect(gport->drawing_area, mod_snap_key_release_handler);
	mod_snap_key_press_handler = 0;
    mod_snap_key_release_handler = 0;
  } 
  return 0;
}

HID_Action mod_snap_action_list[] = {
  {"ModSnapEnable", 0, ActionModSnapEnable, mod_snap_enable_help, mod_snap_enable_syntax},
  {"ModSnapSetProperty", 0, ActionModSnapSetProperty, mod_snap_set_property_help, mod_snap_set_property_syntax}
};

REGISTER_ACTIONS(mod_snap_action_list)

extern SnapSpecType grid_snap, pin_pad_snap, element_snap, via_snap, line_snap, arc_snap, polygon_snap;

void
mod_snapping_plugin_init()
{
  SnapSpecType * snap;
  printf("Loading plugin: mod_snapping\n");

  // create the new list, to be used as the default list
  norm_list = snap_list_new();

  snap = snap_list_add_snap(norm_list, &grid_snap);
  // everything else we turn off
  snap = snap_list_add_snap(norm_list, &pin_pad_snap);
  snap->enabled = false;
  snap = snap_list_add_snap(norm_list, &element_snap);
  snap->enabled = false;
  snap = snap_list_add_snap(norm_list, &via_snap);
  snap->enabled = false;
  snap = snap_list_add_snap(norm_list, &line_snap);
  snap->enabled = false;
  snap = snap_list_add_snap(norm_list, &arc_snap);
  snap->enabled = false;
  snap = snap_list_add_snap(norm_list, &polygon_snap);
  snap->enabled = false;

  mod_snapping_initialized = 1;
  //register_mod_snap_action_list();
}
