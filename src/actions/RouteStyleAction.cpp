/*!
 * \file src/actions/RouteStyleAction.cpp
 *
 * \brief RouteStyle action - selects a routing style.
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
#include "error.h"   // For Message()
#include "set.h"     // For SetLineSize(), etc.
#include "hid.h"     // For hid_action()
}

#include <cstdlib>   // For atoi

namespace pcb {
namespace actions {

/*!
 * \brief RouteStyleAction - Select a routing style
 *
 * Copies the indicated routing style (1-4) into the current sizes.
 * This sets the line thickness, via diameter, via drill hole,
 * keepaway width, and via mask aperture.
 *
 * Syntax: RouteStyle(1|2|3|4)
 */
class RouteStyleAction : public Action {
public:
    RouteStyleAction()
        : Action("RouteStyle",
                 "Copies the indicated routing style into the current sizes.",
                 "RouteStyle(1|2|3|4)")
    {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        if (argc < 1) {
            Message("Syntax error. Usage:\n%s\n", syntax());
            return 1;
        }

        const char* str = argv[0];
        int number = atoi(str);

        if (number > 0 && number <= NUM_STYLES) {
            RouteStyleType* rts = &PCB->RouteStyle[number - 1];
            SetLineSize(rts->Thick);
            SetViaSize(rts->Diameter, true);
            SetViaDrillingHole(rts->Hole, true);
            SetKeepawayWidth(rts->Keepaway);
            SetViaMaskAperture(rts->ViaMask);
            hid_action("RouteStylesChanged");
        }
        else {
            Message("RouteStyle: Invalid style number %d. Use 1-%d.\n", number, NUM_STYLES);
            return 1;
        }

        return 0;
    }
};

// Auto-register this action
REGISTER_ACTION(RouteStyleAction);

}} // namespace pcb::actions
