#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "buffer.h"
#include "crosshair.h"
#include "data.h"
#include "error.h"
#include "file.h"
#include "hid.h"
#include "misc.h"
#include "mymem.h"
#include "set.h"
}

#include <cstring>

namespace pcb {
namespace actions {

// Syntax string for AFAIL macro
static const char* loadfrom_syntax =
    "LoadFrom(Layout|LayoutToBuffer|ElementToBuffer|Netlist|Revert,filename)";

class LoadFromAction : public Action {
public:
    LoadFromAction() : Action("LoadFrom",
        "LoadFrom(Layout|LayoutToBuffer|ElementToBuffer|Netlist|Revert,filename)",
        "Load layout data from a file.") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        char* function;
        char* name;

        if (argc < 2) {
            AFAIL(loadfrom);
        }

        function = argv[0];
        name = argv[1];

        if (strcasecmp(function, "ElementToBuffer") == 0) {
            notify_crosshair_change(false);
            if (LoadElementToBuffer(PASTEBUFFER, name, true))
                SetMode(PASTEBUFFER_MODE);
            notify_crosshair_change(true);
        }

        else if (strcasecmp(function, "LayoutToBuffer") == 0) {
            notify_crosshair_change(false);
            if (LoadLayoutToBuffer(PASTEBUFFER, name))
                SetMode(PASTEBUFFER_MODE);
            notify_crosshair_change(true);
        }

        else if (strcasecmp(function, "Layout") == 0) {
            if (!PCB->Changed ||
                gui->confirm_dialog(_("OK to override layout data?"), 0))
                LoadPCB(name);
        }

        else if (strcasecmp(function, "Netlist") == 0) {
            if (PCB->Netlistname)
                free(PCB->Netlistname);
            PCB->Netlistname = StripWhiteSpaceAndDup(name);
            FreeLibraryMemory(&PCB->NetlistLib);
            ImportNetlist(PCB->Netlistname);
            NetlistChanged(1);
        }
        else if (strcasecmp(function, "Revert") == 0 && PCB->Filename
                 && (!PCB->Changed
                     || gui->confirm_dialog(_("OK to override changes?"), 0))) {
            RevertPCB();
        }

        return 0;
    }
};

REGISTER_ACTION(LoadFromAction);

}} // namespace pcb::actions
