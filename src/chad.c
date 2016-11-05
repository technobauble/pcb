/*!
 * =====================================================================================
 *
 *                                      COPYRIGHT
 *  Example action implementation for PCB.
 *  Copyright (C) 2015 Robert Zeegers
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *  \File:   example.c
 *  \Brief:  Example on how to implement a action in the PCB program.
 *  \par Description
 *   Example action implementation for PCB, interactive printed circuit board design
 *  \copyright (C) 2015 Robert Zeegers
 *
 *  \Version:  1.0
 *  Created:  24/07/15
 *
 *  \Todo:     nothing
 *  \Bug:      not that I know of
 *
 * =====================================================================================
 */

/* First thing todo is include global.h  */
/* This will automatically also include const.h and macro.h */
#include "global.h"
#include "create.h"
#include "data.h"
#include "search.h"
#include "const.h"

/* Second we include hid.h because we want to register our action to the HID */
#include "hid.h"

/* We are going to add an action and therefore we want it to show-up in the PCB manual, */
/* so we add some documentation comments */
/* For the documentation style see the "extract-docs" perl script in the doc directory */

/* %start-doc actions DoSilly
 This function doesn't do anything useful.
 
 @example
 DoSilly()
 @end example
 
 %end-doc */

/* All action entry functions must have the same syntax as defined in */
/* typedef struct HID_Action (hid.h)  */
static int
ActionListObjects (int argc, char **argv, int x, int y)
{
    int type;
    void *ptr1, *ptr2, *ptr3;
    for (int i=1; i < GetNextObjectID(); i++)
    {
        type = SearchObjectByID (PCB->Data, &ptr1, &ptr2, &ptr3, i, ALL_TYPES);
        Message ("Object ID: %d, type %08x\n", i, type);
    }
}

/* Now we have to make an action list. */
/* Here we make the connection between our command "DoSilly()" and */
/* the actual function which should be executed ExampleDo().*/
static HID_Action exampledo_action_list[] = {
    {"ListObjects", "Lists objects", ActionListObjects, "Lists the objects", "ListObjects()"}
};

/* Next a macro to register the action in the HID */
/* Note the missing ; at the end, that's correct ;-) */
REGISTER_ACTIONS (exampledo_action_list)//

