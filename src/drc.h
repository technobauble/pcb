/*!
 * \file src/drc.h
 *
 * \brief DRC related structures and functions
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2018 Charles Parker <parker.charles@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef PCB_DRC_H
#define PCB_DRC_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "global.h"

typedef struct drc_object_id_list
{
  int count; /* number of objects actually in list */
  int size; /* allocated size of list */
  long int *id_list;
  int *type_list;
} drc_object_id_list;

extern drc_object_id_list * drc_current_violation_list;

drc_object_id_list * drc_object_id_list_new(int n);
void drc_object_id_list_delete(drc_object_id_list * list);
void drc_object_id_list_clear(drc_object_id_list * list);
drc_object_id_list * drc_object_id_list_expand(drc_object_id_list * list,
                                               int n);
drc_object_id_list * drc_object_id_list_append(drc_object_id_list * list,
                                               int type, long id);
void drc_object_id_list_reset_with(drc_object_id_list * list,
                                   int type, long id);

typedef struct drc_violation_st
{
  char *title;
  char *explanation;
  Coord x, y;
  Angle angle;
  int have_measured;
  Coord measured_value;
  Coord required_value;
  int object_count;
  long int *object_id_list;
  int *object_type_list;
} DrcViolationType; 

DrcViolationType *pcb_drc_violation_new (const char *title,
                        const char *explanation,
                        Coord x, Coord y,
                        Angle angle,
                        bool have_measured,
                        Coord measured_value,
                        Coord required_value,
                        int object_count,
                        long int *object_id_list,
                        int *object_type_list);

void pcb_drc_violation_free (DrcViolationType *violation);

void reset_drc_dialog_message(void);
void append_drc_dialog_message(const char *fmt, ...);

void append_drc_violation (DrcViolationType *violation);

int throw_drc_dialog(void);

void LocateError(Coord *, Coord *);
void GotoError(void);

bool SetThing(int, void *, void *, void *);
int GetThing(void **, void **, void **);

#endif /* PCB_DRC_H */

