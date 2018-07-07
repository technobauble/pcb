/*!
 * \file src/snap.c
 *
 * \brief Snapping functions.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2018 Charles Parker
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 *
 * <hr>
 *
 * This file implements the infrastructure for searching an X, Y location for 
 * something that the crosshair can snap to.
 *
 * TODO:
 *   * Big major todo list item is to implement an action interface for the snap
 *     list so that the user can configure the snaps from the command line. This
 *     is almost essential unles I want to implement something in the lesstif
 *     hid as well as the gtk hid and all future graphical hids.
 *
 *   * snapping should really have a radius in pixels on the screen, not
 *     the linear distance in mm or mils. In the later case, the snapping radius
 *     doesn't change with zoom level, which is, I think a desireable feature.
 *     Maybe I can implement this as an option.
 *
 *   * This file contains the implementations of three different data
 *     structures, and the actions used to configure the snapping behavior.
 *     This should be broken up into several files and put in their own
 *     directory.
 *
 */

#include "snap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "global.h"

#include "data.h"   // Crosshair structure
#include "error.h"  // Message
#include "search.h" // Searching functions

/*
 * Snap Type functions
 */

/*!
 * \brief SnapSpec constructor
 *
 * Allocate memory for and initialize a new SnapSpecType object.
 *
 * \param name pointer to the name of new snap spec
 * \param priority the priority of the new snap spec, indicating where it should
 * go in the list.
 * \return a pointer to the new snap spec.
 */
SnapSpecType *
snap_spec_new(char * name, int priority)
{
  SnapSpecType * snap = malloc(sizeof(SnapSpecType));
  snap->name = strdup(name);
  snap->priority = priority;
  snap->enabled = false;
  snap->search = NULL;
  snap->radius = 0;
  return snap;
}

/*!
 * \brief SnapSpec destructor
 *
 * \param snap a pointer to the snap spec to be destroyed.
 */
void
snap_spec_delete(SnapSpecType * snap)
{
  free(snap->name);
  free(snap);
}

/*!
 * \brief SnapSpec copy constructor
 *
 * \param snap a pointer to the snap spec that should be copied.
 * \return a pointer to the new snap spec.
 */
SnapSpecType *
snap_spec_copy(SnapSpecType * snap)
{
  SnapSpecType * new_snap = malloc (sizeof(SnapSpecType));
  memcpy(new_snap, snap, sizeof(SnapSpecType));
  return new_snap;
}


/*
 * Snap List Type functions
 */

/*!
 * \brief SnapList constructor
 *
 * Create a new snap list.
 * Allocate and initialize memory for a snap list structure.
 *
 * \return A pointer to the new snap list.
 *
 */
SnapListType *
snap_list_new(void)
{
  SnapListType * list = malloc(sizeof(SnapListType));
  list->n = 0;
  list->max = 0;
  list->snaps = NULL;
  return list;
}

/*!
 * \brief SnapList destructor 
 *
 * Delete a snap list.
 * 
 * Free the memory allocated to the snap list. This frees all of the snaps in 
 * the list, since we make copies of them.
 * 
 * TODO:
 * Are other objects allowed to keep pointers to snaps in the list? If so, then
 * when we free them here, the objects that have those pointers might try to 
 * access them after we've freed them. So, either we don't let things keep 
 * pointers, or we have to let the other objects own the snaps.
 *
 * \param list a pointer to the list to be deleted.
 */
void
snap_list_delete(SnapListType * list)
{
  free(list->snaps);
  free(list);
}

/*!
 * \brief Add a snap to a snap list
 * 
 * The snap is inserted into the list according to its priority number. The
 * list is maintained sorted by priority with the highest priority items
 * being located at lower addresses. 
 *
 * When a snap is added to the list, a copy is made. The idea is to keep
 * them all together in memory since we're likely going to be iterating over 
 * the list. If the list isn't large enough to accommodate the new snap,
 * then it is enlarged.
 *
 * TODO: Should this function actually return a pointer to the new snap, or
 * something else? Since we can create, relocate, or destroy our local copies 
 * of snaps at any time, external functions can't count on the snap being at
 * the returned location in the future. It might be better to return an
 * integer to indicate were in the list the snap was placed, or failure to
 * do so, if, for example, memory could not be allocated.
 *
 * \param list a pointer to the list to add the snap to
 * \param snap a pointer to the snap to add to the list
 * \return a pointer to the new snap in the snap list
 */
SnapSpecType *
snap_list_add_snap(SnapListType * list, SnapSpecType * snap)
{
  int i, in = list->n;
  
  if ((list == NULL) || (snap == NULL)) return NULL;
  
  // figure out where in the list to put the new snap
  for(i = 0; i < list->n; i++)
  {
    /* use the name to detect and reject duplicates */
    if (strcmp (list->snaps[i].name, snap->name)==0) return NULL;
    /* make a note of where to insert the entry */
    if (snap->priority > list->snaps[i].priority) {
      in = i;
      break;
    }
  }
  
  /* reallocate the memory to the new size */
  if (list->n + 1 > list->max)
  {
    list->snaps = (SnapSpecType *) realloc(list->snaps,
                                       (list->n + 1) * sizeof(SnapSpecType));
    list->max += 1;
  }
  /* shift everything after the new entry down */
  memmove(list->snaps + in + 1, list->snaps + in,
          (list->n - in)*sizeof(SnapSpecType));
  /* copy in the new entry */
  memcpy(list->snaps+in, snap, sizeof(SnapSpecType));
  /* increment the list counter */
  list->n += 1;
  return list->snaps+in;
}

/*!
 * \brief Remove a snap from the list, identified by its name.
 * 
 * The snap list is searched for a snap with the given name, and the first
 * snap with that name is removed from the list, although, names should be
 * unique. The list is consolidated, but the memory allocation is kept in case 
 * we need to add more snaps later.
 *
 * This behavior could cause pcb to consume lots of memory if large
 * numbers of snaps are allocated, but the structures are fairly small, so,
 * it would take a LOT of snaps to be noticeable. This is not expected to
 * happen, but, it might be good form to place limits on the maximum size of
 * the list.
 *
 * \param list the list from which a snap of the given name should be
 * removed.
 * \param name a pointer to the name that should be removed from the list.
 * \return the location in the list from which the element was removed. I'm
 * not sure how useful that actually is...
 */
int
snap_list_remove_snap_by_name(SnapListType * list, char * name)
{
  int i;
  
  if ((list == NULL) || (name == NULL)) return -1;
  
  for(i = 0; i < list->n; i++)
    if (strcmp (list->snaps[i].name, name)==0)
    {
      if (i+1 != list->n)
      /* item was not the last element in the list */
        memmove(list->snaps + i, list->snaps + i + 1,
                (list->n - i - 1)*sizeof(SnapSpecType));
      list->n -= 1;
    }
  return i;
}

/*!
 * \brief Find a snap in the list with a given name.
 *
 * Search through the snap list and return a pointer to the snap with the
 * given name, or return NULL if no matching name is found.
 *
 * \param list a pointer to the list that should be searched.
 * \param name a pointer to the name that should be searched for.
 * \return a pointer to a snap in the snap list with the given name, or null
 * if no snap with a matching name is in the list.
 */
SnapSpecType *
snap_list_find_snap_by_name(SnapListType * list, char * name)
{
  int i;
  
  if ((list == NULL) || (name == NULL)) return NULL;
  
  for(i = 0; i < list->n; i++)
    if (strcmp (list->snaps[i].name, name)==0) return list->snaps + i;
  return NULL;
}

/*!
 * \brief Search for something to snap to at the given coordinates.
 *
 * Iterate through the list of snaps until one of the search functions
 * returns a valid snap. This implements a policy of snapping to the highest
 * priority item that is within the snapping radius of the specified
 * coordinates. 
 *
 * Note that this is only one possible snapping policy. This function ought
 * to be renamed to clarify what snapping policy it implements.
 *
 * \param list the snap list to be searched.
 * \param x the x coordinate to search at
 * \param y the y coordinate to search at
 * \return A pointer to the SnapType that contains information about the
 * result of the snap search, or NULL if nothing was found to snap to.
 */
SnapType *
snap_list_search_snaps(SnapListType * list, Coord x, Coord y)
{
  int i;
  static SnapType snap;
  
  for (i=0; i < list->n; i++){
    if (!list->snaps[i].enabled) continue;
    snap = list->snaps[i].search(x, y, list->snaps[i].radius);
    snap.spec = list->snaps + i;
    if (snap.valid) return &snap;
  }
  return NULL;
}

static const char listsnaps_help[] = "Print the list of snaps to the log window";
static const char listsnaps_syntax[] = "ListSnaps()";

/* %start-doc actions ListSnaps

Prints a list of snaps to the message log.

%end-doc */

/*! \brief Print a list of snaps to the message log. */
static int
ActionListSnaps(int argc, char **argv, Coord x, Coord y)
{
  int i;
  SnapListType * snaps = Crosshair.snaps;
  Message("List has %d snaps out of a maximum %d\n", snaps->n, snaps->max);
  for(i=0; i< snaps->n; i++)
    Message("\t%s (%s, r = %d, p = %d)\n",
            snaps->snaps[i].name,
            snaps->snaps[i].enabled ? "enabled":"disabled",
            snaps->snaps[i].radius,
            snaps->snaps[i].priority);
  return 0;
}


HID_Action snap_action_list[] = {
  {"ListSnaps", 0, ActionListSnaps, listsnaps_help, listsnaps_syntax}
};

REGISTER_ACTIONS(snap_action_list)
