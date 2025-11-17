/*!
 * \file src/actions/MorphPolygonAction.cpp
 *
 * \brief MorphPolygon action - morphs polygons to remove unnecessary points.
 *
 * <hr>
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2025 PCB Contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "Action.h"

// Only include stable C interfaces
extern "C" {
#include "global.h"
#include "data.h"        // For PCB
#include "action.h"      // For GetFunctionID()
#include "search.h"      // For SearchScreen()
#include "polygon.h"     // For MorphPolygon()
#include "draw.h"        // For Draw()
#include "undo.h"        // For IncrementUndoSerialNumber()
#include "hid.h"         // For gui
#include "macro.h"       // For ALLPOLYGON_LOOP
}

namespace pcb {
namespace actions {

/*!
 * \brief MorphPolygonAction - Morph polygons to simplify shape
 *
 * Morphs a polygon by removing unnecessary points while maintaining
 * the same shape. This simplifies complex polygons.
 *
 * Syntax: MorphPolygon(Object|Selected|SelectedObjects)
 */
class MorphPolygonAction : public Action {
public:
    MorphPolygonAction()
        : Action("MorphPolygon",
                 "Morphs a polygon.",
                 "MorphPolygon(Object|Selected|SelectedObjects)")
    {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        if (argc < 1) {
            return 0;
        }

        const char* function = argv[0];

        switch (GetFunctionID(const_cast<char*>(function))) {
            case F_Object: {
                int type;
                void* ptr1;
                void* ptr2;
                void* ptr3;

                gui->get_coords(const_cast<char*>(_("Select an Object")), &x, &y);
                type = SearchScreen(x, y, POLYGON_TYPE, &ptr1, &ptr2, &ptr3);
                if (type != NO_TYPE) {
                    MorphPolygon(static_cast<LayerType*>(ptr1),
                               static_cast<PolygonType*>(ptr3));
                    Draw();
                    IncrementUndoSerialNumber();
                }
                break;
            }

            case F_Selected:
            case F_SelectedObjects:
                ALLPOLYGON_LOOP(PCB->Data);
                {
                    if (TEST_FLAG(SELECTEDFLAG, polygon)) {
                        MorphPolygon(layer, polygon);
                    }
                }
                ENDALL_LOOP;
                Draw();
                IncrementUndoSerialNumber();
                break;

            default:
                // Unknown function - just ignore
                break;
        }

        return 0;
    }
};

// Auto-register this action
REGISTER_ACTION(MorphPolygonAction);

}} // namespace pcb::actions
