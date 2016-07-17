/*!
 * \file src/hid/gcode/decompose.h
 *
 * \brief Header file for auxiliary bitmap manipulations.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2001-2007 Peter Selinger.
 *
 * This file is part of Potrace. It is free software and it is covered
 * by the GNU General Public License. See the file COPYING for details.
 */

/* decompose.h 147 2007-04-09 00:44:09Z selinger */

#ifndef DECOMPOSE_H
#define DECOMPOSE_H

#include "potracelib.h"
//#include "progress.h"

int bm_to_pathlist (const potrace_bitmap_t * bm, path_t ** plistp,
		    const potrace_param_t * param);

#endif /* DECOMPOSE_H */
