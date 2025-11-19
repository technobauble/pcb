/*!
 * \file src/actions/ToggleHideNameAction.cpp
 *
 * \brief ToggleHideName action - toggles element name visibility.
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
#include "draw.h"        // For EraseElementName(), DrawElementName(), Draw()
#include "undo.h"        // For AddObjectToFlagUndoList(), IncrementUndoSerialNumber()
#include "hid.h"         // For gui
#include "macro.h"       // For ELEMENT_LOOP
}

namespace pcb {
namespace actions {

/*!
 * \brief ToggleHideNameAction - Toggle element name visibility
 *
 * Toggles the visibility of element names. Hidden names won't appear
 * on screen or on the silk layer when printed.
 *
 * Syntax: ToggleHideName(Object|SelectedElements|Selected)
 */
class ToggleHideNameAction : public Action {
public:
    ToggleHideNameAction()
        : Action("ToggleHideName",
                 "Toggles the visibility of element names.",
                 "ToggleHideName(Object|SelectedElements)")
    {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        if (argc < 1 || !PCB->ElementOn) {
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
                type = SearchScreen(x, y, ELEMENT_TYPE, &ptr1, &ptr2, &ptr3);
                if (type != NO_TYPE) {
                    ElementType* element = static_cast<ElementType*>(ptr2);
                    AddObjectToFlagUndoList(type, ptr1, ptr2, ptr3);
                    EraseElementName(element);
                    TOGGLE_FLAG(HIDENAMEFLAG, element);
                    DrawElementName(element);
                    Draw();
                    IncrementUndoSerialNumber();
                }
                break;
            }

            case F_SelectedElements:
            case F_Selected: {
                bool changed = false;
                ELEMENT_LOOP(PCB->Data);
                {
                    if ((TEST_FLAG(SELECTEDFLAG, element) ||
                         TEST_FLAG(SELECTEDFLAG, &NAMEONPCB_TEXT(element))) &&
                        (FRONT(element) || PCB->InvisibleObjectsOn)) {
                        AddObjectToFlagUndoList(ELEMENT_TYPE, element, element, element);
                        EraseElementName(element);
                        TOGGLE_FLAG(HIDENAMEFLAG, element);
                        DrawElementName(element);
                        changed = true;
                    }
                }
                END_LOOP;
                if (changed) {
                    Draw();
                    IncrementUndoSerialNumber();
                }
                break;
            }

            default:
                break;
        }

        return 0;
    }
};

// Auto-register this action
REGISTER_ACTION(ToggleHideNameAction);

}} // namespace pcb::actions
