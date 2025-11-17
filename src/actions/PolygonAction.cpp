/*!
 * \file src/actions/PolygonAction.cpp
 *
 * \brief Polygon action - polygon drawing operations.
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
#include "error.h"       // For Message()
#include "polygon.h"     // For ClosePolygon(), GoToPreviousPoint()
#include "crosshair.h"   // For notify_crosshair_change()
}

#include <cstring>       // For strcmp

namespace pcb {
namespace actions {

/*!
 * \brief PolygonAction - Polygon drawing operations
 *
 * Polygons need a special action routine to make life easier during
 * polygon drawing mode.
 *
 * - Close: Creates the final segment of the polygon
 * - PreviousPoint: Go back to the previous point
 *
 * This action only works when in POLYGON_MODE.
 *
 * Syntax: Polygon(Close|PreviousPoint)
 */
class PolygonAction : public Action {
public:
    PolygonAction()
        : Action("Polygon",
                 "Some polygon related stuff.",
                 "Polygon(Close|PreviousPoint)")
    {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        if (argc < 1) {
            Message("Syntax error. Usage:\n%s\n", syntax());
            return 1;
        }

        const char* function = argv[0];

        // This action only works in POLYGON_MODE
        if (Settings.Mode != POLYGON_MODE) {
            return 0;
        }

        notify_crosshair_change(false);

        if (strcmp(function, "Close") == 0) {
            ClosePolygon();
        }
        else if (strcmp(function, "PreviousPoint") == 0) {
            GoToPreviousPoint();
        }
        else {
            notify_crosshair_change(true);
            Message("Polygon: Unknown argument '%s'. Use Close or PreviousPoint.\n", function);
            return 1;
        }

        notify_crosshair_change(true);
        return 0;
    }
};

// Auto-register this action
REGISTER_ACTION(PolygonAction);

}} // namespace pcb::actions
