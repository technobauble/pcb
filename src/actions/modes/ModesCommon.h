/*!
 * \file src/actions/modes/ModesCommon.h
 *
 * \brief Common includes for all editor mode implementations
 *
 * This header provides a "starter pack" of commonly-needed C headers for
 * mode implementations. It follows the existing PCB architecture where
 * global.h provides minimal core types/macros, and individual files
 * include operational headers as needed.
 *
 * By centralizing common mode dependencies here, we:
 * - Reduce boilerplate across 16 mode files
 * - Ensure consistent includes (fewer missing header errors)
 * - Keep the include scope limited to modes (not project-wide)
 * - Maintain global.h's minimal design
 *
 * Usage:
 *   #include "EditorMode.h"
 *   #include "ModesCommon.h"
 *   // Then add mode-specific headers as needed
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef PCB_MODES_COMMON_H
#define PCB_MODES_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Core operational headers needed by most/all modes
 */

#include "global.h"      // Core types, macros, flags (already in EditorMode.h, but listed for clarity)
#include "data.h"        // PCB, Settings, layer variables (bottom_silk_layer, top_silk_layer, LayerStack)
#include "search.h"      // SearchScreen - nearly all modes search for objects
#include "change.h"      // Change* functions - common operations (ChangeObjectThermal, etc.)
#include "undo.h"        // IncrementUndoSerialNumber, undo operations
#include "hid.h"         // GUI interface, shift key detection, hid_actionl
#include "misc.h"        // SetChangedFlag and other utility functions
#include "set.h"         // ClearWarnings and setting operations

#ifdef __cplusplus
}
#endif

#endif /* PCB_MODES_COMMON_H */
