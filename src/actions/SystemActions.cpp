/*!
 * \file src/actions/SystemActions.cpp
 *
 * \brief System-level actions (Quit, Message, DumpLibrary).
 *
 * This module implements simple system-level actions:
 * - Quit: Exit the application (with optional force flag)
 * - Message: Write messages to the log window
 * - DumpLibrary: Display library contents for debugging
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
 */

#include "SystemActions.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>

extern "C" {
#include "global.h"
#include "error.h"
#include "misc.h"
#include "data.h"
}

namespace pcb {
namespace actions {

//=============================================================================
// QuitAction
//=============================================================================

static const char quit_syntax[] = "Quit()";

static const char quit_help[] = "Quits the application after confirming.";

QuitAction::QuitAction()
    : Action("Quit", quit_help, quit_syntax)
{}

int QuitAction::execute(int /*argc*/, char** argv, Coord /*x*/, Coord /*y*/)
{
    const char* force = arg(0, argv);

    if (force && strcasecmp(force, "force") == 0) {
        PCB->Changed = 0;
        exit(0);
    }

    if (!PCB->Changed || gui->close_confirm_dialog() == HID_CLOSE_CONFIRM_OK) {
        QuitApplication();
    }

    return 1;
}

//=============================================================================
// MessageAction
//=============================================================================

static const char message_syntax[] = "Message(message)";

static const char message_help[] = "Writes a message to the log window.";

MessageAction::MessageAction()
    : Action("Message", message_help, message_syntax)
{}

int MessageAction::execute(int argc, char** argv, Coord /*x*/, Coord /*y*/)
{
    if (argc < 1) {
        Message("ERROR: Message() requires at least one argument\n");
        return 1;
    }

    for (int i = 0; i < argc; i++) {
        Message(argv[i]);
        Message("\n");
    }

    return 0;
}

//=============================================================================
// DumpLibraryAction
//=============================================================================

static const char dumplibrary_syntax[] = "DumpLibrary()";

static const char dumplibrary_help[] = "Display the entire contents of the libraries.";

DumpLibraryAction::DumpLibraryAction()
    : Action("DumpLibrary", dumplibrary_help, dumplibrary_syntax)
{}

int DumpLibraryAction::execute(int /*argc*/, char** /*argv*/, Coord /*x*/, Coord /*y*/)
{
    printf("**** Do not count on this format.  It will change ****\n\n");
    printf("MenuN   = %d\n", (int) Library.MenuN);
    printf("MenuMax = %d\n", (int) Library.MenuMax);

    for (int i = 0; i < Library.MenuN; i++) {
        printf("Library #%d:\n", i);
        printf("    EntryN    = %d\n", (int) Library.Menu[i].EntryN);
        printf("    EntryMax  = %d\n", (int) Library.Menu[i].EntryMax);
        printf("    Name      = \"%s\"\n", UNKNOWN(Library.Menu[i].Name));
        printf("    directory = \"%s\"\n", UNKNOWN(Library.Menu[i].directory));
        printf("    Style     = \"%s\"\n", UNKNOWN(Library.Menu[i].Style));
        printf("    flag      = %d\n", Library.Menu[i].flag);

        for (int j = 0; j < Library.Menu[i].EntryN; j++) {
            printf("    #%4d: ", j);
            if (Library.Menu[i].Entry[j].Template == (char*) -1) {
                printf("newlib: \"%s\"\n",
                       UNKNOWN(Library.Menu[i].Entry[j].ListEntry));
            } else {
                printf("\"%s\", \"%s\", \"%s\", \"%s\", \"%s\"\n",
                       UNKNOWN(Library.Menu[i].Entry[j].ListEntry),
                       UNKNOWN(Library.Menu[i].Entry[j].Template),
                       UNKNOWN(Library.Menu[i].Entry[j].Package),
                       UNKNOWN(Library.Menu[i].Entry[j].Value),
                       UNKNOWN(Library.Menu[i].Entry[j].Description));
            }
        }
    }

    return 0;
}

//=============================================================================
// Registration
//=============================================================================

REGISTER_ACTION(QuitAction);
REGISTER_ACTION(MessageAction);
REGISTER_ACTION(DumpLibraryAction);

} // namespace actions
} // namespace pcb
