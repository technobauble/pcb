/*!
 * \file src/actions/ConnectionAction.cpp
 *
 * \brief Connection action - finds and highlights connections.
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
#include "action.h"      // For GetFunctionID()
#include "find.h"        // For LookupConnection()
#include "flags.h"       // For ClearFlagOn*()
#include "draw.h"        // For Draw()
#include "undo.h"        // For IncrementUndoSerialNumber()
#include "hid.h"         // For gui
}

namespace pcb {
namespace actions {

/*!
 * \brief ConnectionAction - Find and highlight connections
 *
 * Finds electrical connections from a clicked point and highlights them.
 * Can also reset (clear) the connection highlighting.
 *
 * Syntax: Connection(Find|Reset|ResetLinesAndPolygons|ResetPinsViasAndPads)
 */
class ConnectionAction : public Action {
public:
    ConnectionAction()
        : Action("Connection",
                 "Searches connections of the object at the cursor position.",
                 "Connection(Find|Reset|ResetLinesAndPolygons|ResetPinsViasAndPads)")
    {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        if (argc < 1) {
            return 0;
        }

        const char* function = argv[0];

        switch (GetFunctionID(const_cast<char*>(function))) {
            case F_Find:
                gui->get_coords(const_cast<char*>(_("Click on a connection")), &x, &y);
                LookupConnection(x, y, true, 1, CONNECTEDFLAG, false);
                LookupConnection(x, y, true, 1, FOUNDFLAG, true);
                break;

            case F_ResetLinesAndPolygons:
                if (ClearFlagOnLinesAndPolygons(CONNECTEDFLAG | FOUNDFLAG, true)) {
                    IncrementUndoSerialNumber();
                    Draw();
                }
                break;

            case F_ResetPinsViasAndPads:
                if (ClearFlagOnPinsViasAndPads(CONNECTEDFLAG | FOUNDFLAG, true)) {
                    IncrementUndoSerialNumber();
                    Draw();
                }
                break;

            case F_Reset:
                if (ClearFlagOnAllObjects(CONNECTEDFLAG | FOUNDFLAG, true)) {
                    IncrementUndoSerialNumber();
                    Draw();
                }
                break;

            default:
                // Unknown function - just ignore
                break;
        }

        return 0;
    }
};

// Auto-register this action
REGISTER_ACTION(ConnectionAction);

}} // namespace pcb::actions
