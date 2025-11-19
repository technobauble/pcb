#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "hid.h"
#include "misc.h"
#include "search.h"
#include "set.h"
}

namespace pcb {
namespace actions {

class SetSameAction : public Action {
public:
    SetSameAction() : Action("SetSame",
        "SetSame()",
        "Sets current layer and sizes to match indicated item.") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        void *ptr1, *ptr2, *ptr3;
        int type;
        LayerType *layer = CURRENT;

        type = SearchScreen(x, y, CLONE_TYPES, &ptr1, &ptr2, &ptr3);

        // Set layer current and size from line or arc
        switch (type) {
            case LINE_TYPE:
                notify_crosshair_change(false);
                Settings.LineThickness = static_cast<LineType*>(ptr2)->Thickness;
                Settings.Keepaway = static_cast<LineType*>(ptr2)->Clearance / 2;
                layer = static_cast<LayerType*>(ptr1);
                if (Settings.Mode != LINE_MODE)
                    SetMode(LINE_MODE);
                notify_crosshair_change(true);
                hid_action("RouteStylesChanged");
                break;

            case ARC_TYPE:
                notify_crosshair_change(false);
                Settings.LineThickness = static_cast<ArcType*>(ptr2)->Thickness;
                Settings.Keepaway = static_cast<ArcType*>(ptr2)->Clearance / 2;
                layer = static_cast<LayerType*>(ptr1);
                if (Settings.Mode != ARC_MODE)
                    SetMode(ARC_MODE);
                notify_crosshair_change(true);
                hid_action("RouteStylesChanged");
                break;

            case POLYGON_TYPE:
                layer = static_cast<LayerType*>(ptr1);
                break;

            case VIA_TYPE:
                notify_crosshair_change(false);
                Settings.ViaThickness = static_cast<PinType*>(ptr2)->Thickness;
                Settings.ViaDrillingHole = static_cast<PinType*>(ptr2)->DrillingHole;
                Settings.Keepaway = static_cast<PinType*>(ptr2)->Clearance / 2;
                if (Settings.Mode != VIA_MODE)
                    SetMode(VIA_MODE);
                notify_crosshair_change(true);
                hid_action("RouteStylesChanged");
                break;

            default:
                return 1;
        }

        if (layer != CURRENT) {
            ChangeGroupVisibility(GetLayerNumber(PCB->Data, layer), true, true);
            Redraw();
        }

        return 0;
    }
};

REGISTER_ACTION(SetSameAction);

}} // namespace pcb::actions
