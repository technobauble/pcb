#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "change.h"
#include "crosshair.h"
#include "data.h"
#include "error.h"
#include "search.h"
#include "set.h"
}

#include <algorithm>
#include <cstring>

namespace pcb {
namespace actions {

// Helper function to identify layer by name or index
static bool identify_layer(char* layer_name, Cardinal* layer_no) {
    if (strcmp(layer_name, "-") == 0) {
        *layer_no = static_cast<Cardinal>(-1);
        return true;
    }

    if (strcmp(layer_name, "c") == 0) {
        if (static_cast<unsigned int>(INDEXOFCURRENT) < max_copper_layer) {
            *layer_no = INDEXOFCURRENT;
            return true;
        }
    }

    int layer = SearchLayerByName(PCB->Data, layer_name);
    if (layer == -1) {
        if (sscanf(layer_name, "%d", &layer) != 1) {
            layer = -1;
        }
    }

    if (layer != -1) {
        *layer_no = layer;
    }

    return (layer != -1);
}

// Syntax string for AFAIL macro
static const char* setvialayers_syntax =
    "SetViaLayers(Object|Selected|SelectedVias, from_layer, to_layer)";

class SetViaLayersAction : public Action {
public:
    SetViaLayersAction() : Action("SetViaLayers",
        "Set the layer range for vias",
        "SetViaLayers(Object|Selected|SelectedVias, from_layer, to_layer)\n"
        "SetViaLayers(ThroughHole)") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        char* function = argc > 0 ? argv[0] : nullptr;
        char* layername_from = argc > 1 ? argv[1] : nullptr;
        char* layername_to = argc > 2 ? argv[2] : nullptr;
        Cardinal layer_from;
        Cardinal layer_to = static_cast<Cardinal>(-1);

        if (!function) {
            Message(_("SetViaLayers requires at least one argument\n"));
            return 1;
        }

        if (argc < 2) {
            Message(_("This GUI doesn't support Via Layers editing\n"));
            return 1;
        }

        if (GetFunctionID(layername_from) == F_ThroughHole) {
            layer_from = 0;
            layer_to = 0;
        } else {
            if (!identify_layer(layername_from, &layer_from) ||
                !identify_layer(layername_to, &layer_to)) {
                Message(_("Sorry, wrong layers specified.\n"));
                return 1;
            }
        }

        // Ensure that layer_from < layer_to
        if (layer_from != static_cast<Cardinal>(-1) &&
            layer_to != static_cast<Cardinal>(-1) &&
            layer_from > layer_to) {
            std::swap(layer_from, layer_to);
        }

        if (layer_to != static_cast<Cardinal>(-1)) {
            layer_to = std::min(layer_to, static_cast<Cardinal>(max_copper_layer - 1));
        }

        switch (GetFunctionID(function)) {
            case F_Object: {
                int type;
                void* ptr1;
                void* ptr2;
                void* ptr3;

                type = SearchScreen(Crosshair.X, Crosshair.Y, VIA_TYPE,
                                  &ptr1, &ptr2, &ptr3);
                if (type != NO_TYPE) {
                    if (TEST_FLAG(LOCKFLAG, static_cast<PinType*>(ptr1))) {
                        Message(_("Sorry, the object is locked\n"));
                    } else {
                        if (ChangeObjectViaLayers(ptr1, ptr2, ptr3, layer_from, layer_to)) {
                            SetChangedFlag(true);
                        }
                    }
                }
                break;
            }

            case F_SelectedVias:
            case F_Selected:
                if (ChangeSelectedViaLayers(layer_from, layer_to)) {
                    SetChangedFlag(true);
                }
                break;
        }

        return 0;
    }
};

REGISTER_ACTION(SetViaLayersAction);

}} // namespace pcb::actions
