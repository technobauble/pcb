/*!
 * \file src/actions/AutoRouteAction.cpp
 *
 * \brief AutoRoute action - auto-route rat lines.
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
#include "autoroute.h"   // For AutoRoute()
#include "set.h"         // For SetChangedFlag()
#include "hid.h"         // For hid_action()
}

namespace pcb {
namespace actions {

/*!
 * \brief AutoRouteAction - Auto-route rat lines
 *
 * Attempts to automatically route either all rats or just selected rats.
 * Uses GetFunctionID for dispatch.
 *
 * Syntax: AutoRoute(AllRats|SelectedRats|Selected)
 */
class AutoRouteAction : public Action {
public:
    AutoRouteAction()
        : Action("AutoRoute",
                 "Auto-route some or all rat lines.",
                 "AutoRoute(AllRats|SelectedRats)")
    {}

    int execute(int argc, char** argv, Coord /*x*/, Coord /*y*/) override {
        hid_action(const_cast<char*>("Busy"));

        if (argc < 1) {
            return 0;
        }

        const char* function = argv[0];

        switch (GetFunctionID(const_cast<char*>(function))) {
            case F_AllRats:
                if (AutoRoute(false)) {
                    SetChangedFlag(true);
                }
                break;

            case F_SelectedRats:
            case F_Selected:
                if (AutoRoute(true)) {
                    SetChangedFlag(true);
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
REGISTER_ACTION(AutoRouteAction);

}} // namespace pcb::actions
