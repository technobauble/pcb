#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "buffer.h"
#include "copy.h"
#include "crosshair.h"
#include "data.h"
#include "error.h"
#include "hid.h"
#include "misc.h"
#include "rotate.h"
#include "set.h"
}

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace pcb {
namespace actions {

// File-scope static variables for persistent state
static char* default_file = nullptr;
static Coord oldx = 0, oldy = 0;

// Syntax string for AFAIL macro
static const char* pastebuffer_syntax =
    "PasteBuffer(AddSelected|Clear|1..MAX_BUFFER)\n"
    "PasteBuffer(Rotate, 1..3)\n"
    "PasteBuffer(Convert|Save|Restore|Mirror)\n"
    "PasteBuffer(ToLayout, X, Y, units)";

class PasteBufferAction : public Action {
public:
    PasteBufferAction() : Action("PasteBuffer",
        "PasteBuffer(AddSelected|Clear|1..MAX_BUFFER)\n"
        "PasteBuffer(Rotate, 1..3)\n"
        "PasteBuffer(Convert|Save|Restore|Mirror)\n"
        "PasteBuffer(ToLayout, X, Y, units)",
        "Various operations on the paste buffer.") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        char* function = argc ? argv[0] : const_cast<char*>("");
        char* sbufnum = argc > 1 ? argv[1] : const_cast<char*>("");
        char* name;
        int free_name = 0;

        notify_crosshair_change(false);
        if (function) {
            switch (GetFunctionID(function)) {
                /* clear contents of paste buffer */
                case F_Clear:
                    ClearBuffer(PASTEBUFFER);
                    break;

                /* copies objects to paste buffer */
                case F_AddSelected:
                    AddSelectedToBuffer(PASTEBUFFER, 0, 0, false);
                    break;

                /* converts buffer contents into an element */
                case F_Convert:
                    ConvertBufferToElement(PASTEBUFFER);
                    break;

                /* break up element for editing */
                case F_Restore:
                    SmashBufferElement(PASTEBUFFER);
                    break;

                /* Mirror buffer */
                case F_Mirror:
                    MirrorBuffer(PASTEBUFFER);
                    break;

                case F_Rotate:
                    if (sbufnum) {
                        RotateBuffer(PASTEBUFFER, static_cast<BYTE>(atoi(sbufnum)));
                        crosshair_update_range();
                    }
                    break;

                case F_Save:
                    if (PASTEBUFFER->Data->ElementN == 0) {
                        Message(_("Buffer has no elements!\n"));
                        break;
                    }
                    free_name = 0;
                    if (argc <= 1) {
                        name = gui->fileselect(_("Save Paste Buffer As ..."),
                                              _("Choose a file to save the contents of the\n"
                                                "paste buffer to.\n"),
                                              default_file, ".fp", "footprint",
                                              0);

                        if (default_file) {
                            free(default_file);
                            default_file = nullptr;
                        }
                        if (name && *name) {
                            default_file = strdup(name);
                        }
                        free_name = 1;
                    } else {
                        name = argv[1];
                    }

                    {
                        FILE *exist;

                        if ((exist = fopen(name, "r"))) {
                            fclose(exist);
                            if (gui->confirm_dialog(_("File exists!  Ok to overwrite?"), 0))
                                SaveBufferElements(name);
                        } else {
                            SaveBufferElements(name);
                        }

                        if (free_name && name)
                            free(name);
                    }
                    break;

                case F_ToLayout: {
                    Coord lx, ly;
                    bool absolute;

                    if (argc == 1) {
                        lx = ly = 0;
                    } else if (argc == 3 || argc == 4) {
                        lx = GetValue(ARG(1), ARG(3), &absolute);
                        if (!absolute)
                            lx += oldx;
                        ly = GetValue(ARG(2), ARG(3), &absolute);
                        if (!absolute)
                            ly += oldy;
                    } else {
                        notify_crosshair_change(true);
                        AFAIL(pastebuffer);
                    }

                    oldx = lx;
                    oldy = ly;
                    if (CopyPastebufferToLayout(lx, ly))
                        SetChangedFlag(true);
                }
                break;

                /* set number */
                default: {
                    int number = atoi(function);

                    /* correct number */
                    if (number)
                        SetBufferNumber(number - 1);
                }
            }
        }

        notify_crosshair_change(true);
        return 0;
    }
};

REGISTER_ACTION(PasteBufferAction);

}} // namespace pcb::actions
