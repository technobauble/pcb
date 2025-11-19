#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "error.h"
#include "hid.h"
#include "undo.h"
}

#include <cstdio>
#include <cstring>

namespace pcb {
namespace actions {

// File-scope static variables for update deferral
static int defer_updates = 0;
static int defer_needs_update = 0;

// Syntax string for AFAIL macro
static const char* executefile_syntax =
    "ExecuteFile(filename)";

class ExecuteFileAction : public Action {
public:
    ExecuteFileAction() : Action("ExecuteFile",
        "Execute actions from a file",
        "ExecuteFile(filename)") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        if (argc != 1) {
            Message(_("ExecuteFile requires a filename argument\n"));
            return 1;
        }

        const char* fname = argv[0];
        FILE* fp = fopen(fname, "r");

        if (fp == nullptr) {
            fprintf(stderr, _("Could not open actions file \"%s\".\n"), fname);
            return 1;
        }

        defer_updates = 1;
        defer_needs_update = 0;

        char line[256];
        int n = 0;

        while (fgets(line, sizeof(line), fp) != nullptr) {
            n++;
            char* sp = line;

            // Eat the trailing newline
            while (*sp && *sp != '\r' && *sp != '\n')
                sp++;
            *sp = '\0';

            // Eat leading spaces and tabs
            sp = line;
            while (*sp && (*sp == ' ' || *sp == '\t'))
                sp++;

            // If we have anything left and it's not a comment line, execute it
            if (*sp && *sp != '#') {
                hid_parse_actions(sp);
            }
        }

        defer_updates = 0;
        if (defer_needs_update) {
            IncrementUndoSerialNumber();
            gui->invalidate_all();
        }

        fclose(fp);
        return 0;
    }
};

REGISTER_ACTION(ExecuteFileAction);

}} // namespace pcb::actions
