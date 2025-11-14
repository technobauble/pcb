/*!
 * \file src/coord_types.h
 *
 * \brief Minimal coordinate type definitions for PCB.
 *
 * This header provides only the coordinate types without pulling in
 * all of global.h. Use this when you only need Coord/Angle types,
 * such as in the action system base classes.
 *
 * For full PCB data structures and functions, include global.h instead.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2025 PCB Contributors
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

#ifndef PCB_COORD_TYPES_H
#define PCB_COORD_TYPES_H

/*
 * Include config.h to get COORD_TYPE definition from configure.
 * COORD_TYPE is set based on --enable-coord32 or --enable-coord64
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Ensure we have stdint.h types if available */
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

/*
 * Coordinate type - configured by autotools based on:
 *   --enable-coord32  → int32_t (or int)
 *   --enable-coord64  → int64_t (or long long)
 *
 * Default fallback is int32_t/int for maximum compatibility.
 */
#ifndef COORD_TYPE
  #ifdef HAVE_STDINT_H
    #define COORD_TYPE int32_t
  #else
    #define COORD_TYPE int
  #endif
#endif

/*!
 * \brief PCB base coordinate unit.
 *
 * All coordinates in PCB are measured in this type. The actual
 * size (32-bit vs 64-bit) is determined at configure time.
 *
 * Internal units are typically 0.01 mil (1/100000 inch).
 */
typedef COORD_TYPE Coord;

/*!
 * \brief Angle type for rotations and arcs.
 *
 * Angles are typically measured in degrees (0-360).
 */
typedef double Angle;

#endif /* PCB_COORD_TYPES_H */
