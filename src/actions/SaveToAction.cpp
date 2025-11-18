#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "buffer.h"
#include "data.h"
#include "error.h"
#include "file.h"
#include "hid.h"
#include "find.h"
#include "search.h"
}

#include <cstdio>
#include <cstring>

namespace pcb {
namespace actions {

// Syntax string for AFAIL macro
static const char* saveto_syntax =
    "SaveTo(Layout|LayoutAs,filename)\n"
    "SaveTo(AllConnections|AllUnusedPins|ElementConnections,filename)\n"
    "SaveTo(PasteBuffer,filename)";

class SaveToAction : public Action {
public:
    SaveToAction() : Action("SaveTo",
        "SaveTo(Layout|LayoutAs,filename)\n"
        "SaveTo(AllConnections|AllUnusedPins|ElementConnections,filename)\n"
        "SaveTo(PasteBuffer,filename)",
        "Saves data to a file.") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        char* function;
        char* name;

        function = ARG(0);

        if (!function || strcasecmp(function, "Layout") == 0) {
            if (SavePCB(PCB->Filename) == 0)
                SetChangedFlag(false);
            return 0;
        }

        if (argc != 2) {
            AFAIL(saveto);
        }

        name = argv[1];

        if (strcasecmp(function, "LayoutAs") == 0) {
            if (SavePCB(name) == 0) {
                SetChangedFlag(false);
                free(PCB->Filename);
                PCB->Filename = strdup(name);
                if (gui->notify_filename_changed != NULL)
                    gui->notify_filename_changed();
            }
            return 0;
        }

        if (strcasecmp(function, "AllConnections") == 0) {
            FILE *fp;
            bool result;
            if ((fp = CheckAndOpenFile(name, true, false, &result, NULL)) != NULL) {
                LookupConnectionsToAllElements(fp);
                fclose(fp);
                SetChangedFlag(true);
            }
            return 0;
        }

        if (strcasecmp(function, "AllUnusedPins") == 0) {
            FILE *fp;
            bool result;
            if ((fp = CheckAndOpenFile(name, true, false, &result, NULL)) != NULL) {
                LookupUnusedPins(fp);
                fclose(fp);
                SetChangedFlag(true);
            }
            return 0;
        }

        if (strcasecmp(function, "ElementConnections") == 0) {
            ElementType *element;
            void *ptrtmp;
            FILE *fp;
            bool result;

            if ((SearchScreen(Crosshair.X, Crosshair.Y, ELEMENT_TYPE,
                             &ptrtmp, &ptrtmp, &ptrtmp)) != NO_TYPE) {
                element = static_cast<ElementType*>(ptrtmp);
                if ((fp = CheckAndOpenFile(name, true, false, &result, NULL)) != NULL) {
                    LookupElementConnections(element, fp);
                    fclose(fp);
                    SetChangedFlag(true);
                }
            }
            return 0;
        }

        if (strcasecmp(function, "PasteBuffer") == 0) {
            return SaveBufferElements(name);
        }

        AFAIL(saveto);
    }
};

REGISTER_ACTION(SaveToAction);

}} // namespace pcb::actions
