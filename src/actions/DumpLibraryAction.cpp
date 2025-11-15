/*!
 * \file src/actions/DumpLibraryAction.cpp
 *
 * \brief Implementation of DumpLibrary action.
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
 */

#include "Action.h"

extern "C" {
#include "coord_types.h"
#include "error.h"       // For Message()
#include "data.h"        // For Library extern
#include "macro.h"       // For UNKNOWN macro
#include <stdio.h>       // For printf()
}

namespace pcb {
namespace actions {

class DumpLibraryAction : public Action {
public:
    DumpLibraryAction()
        : Action("DumpLibrary",
                 "Display the entire contents of the libraries.",
                 "DumpLibrary()")
    {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        // Suppress unused parameter warnings
        (void)argc;
        (void)argv;
        (void)x;
        (void)y;

        int i, j;

        printf ("**** Do not count on this format.  It will change ****\n\n");
        printf ("MenuN   = %d\n", (int) Library.MenuN);
        printf ("MenuMax = %d\n", (int) Library.MenuMax);
        for (i = 0; i < Library.MenuN; i++)
        {
            printf ("Library #%d:\n", i);
            printf ("    EntryN    = %d\n", (int) Library.Menu[i].EntryN);
            printf ("    EntryMax  = %d\n", (int) Library.Menu[i].EntryMax);
            printf ("    Name      = \"%s\"\n", UNKNOWN (Library.Menu[i].Name));
            printf ("    directory = \"%s\"\n",
                    UNKNOWN (Library.Menu[i].directory));
            printf ("    Style     = \"%s\"\n", UNKNOWN (Library.Menu[i].Style));
            printf ("    flag      = %d\n", Library.Menu[i].flag);

            for (j = 0; j < Library.Menu[i].EntryN; j++)
            {
                printf ("    #%4d: ", j);
                if (Library.Menu[i].Entry[j].Template == (char *) -1)
                {
                    printf ("newlib: \"%s\"\n",
                            UNKNOWN (Library.Menu[i].Entry[j].ListEntry));
                }
                else
                {
                    printf ("\"%s\", \"%s\", \"%s\", \"%s\", \"%s\"\n",
                            UNKNOWN (Library.Menu[i].Entry[j].ListEntry),
                            UNKNOWN (Library.Menu[i].Entry[j].Template),
                            UNKNOWN (Library.Menu[i].Entry[j].Package),
                            UNKNOWN (Library.Menu[i].Entry[j].Value),
                            UNKNOWN (Library.Menu[i].Entry[j].Description));
                }
            }
        }

        return 0;
    }
};

REGISTER_ACTION(DumpLibraryAction);

}} // namespace pcb::actions
