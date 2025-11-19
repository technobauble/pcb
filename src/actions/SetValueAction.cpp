#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "data.h"
#include "error.h"
#include "hid.h"
#include "misc.h"
#include "pcb-printf.h"
#include "set.h"
}

namespace pcb {
namespace actions {

class SetValueAction : public Action {
public:
    SetValueAction() : Action("SetValue",
        "SetValue(Grid|Line|LineSize|Text|TextScale|ViaDrillingHole|Via|ViaSize, delta)",
        "Change various board-wide values and sizes.") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        char* function = ARG(0);
        char* val = ARG(1);
        char* units = ARG(2);
        bool absolute;
        double value;
        int text_scale;
        int err = 0;

        if (function && val) {
            value = GetValue(val, units, &absolute);
            switch (GetFunctionID(const_cast<char*>(function))) {
                case F_ViaDrillingHole:
                    SetViaDrillingHole(absolute ? value :
                                      value + Settings.ViaDrillingHole,
                                      false);
                    hid_action("RouteStylesChanged");
                    break;

                case F_Grid:
                    if (absolute) {
                        SetGrid(value, false);
                    } else {
                        if (value == 0) {
                            value = val[0] == '-' ? -Settings.increments->grid
                                                  :  Settings.increments->grid;
                        }
                        // On the way down, short against the minimum PCB drawing unit
                        if ((value + PCB->Grid) < 1)
                            SetGrid(1, false);
                        else if (PCB->Grid == 1)
                            SetGrid(value, false);
                        else
                            SetGrid(value + PCB->Grid, false);
                    }
                    break;

                case F_LineSize:
                case F_Line:
                    if (!absolute && value == 0) {
                        value = val[0] == '-' ? -Settings.increments->line
                                              :  Settings.increments->line;
                    }
                    SetLineSize(absolute ? value : value + Settings.LineThickness);
                    hid_action("RouteStylesChanged");
                    break;

                case F_Via:
                case F_ViaSize:
                    SetViaSize(absolute ? value : value + Settings.ViaThickness, false);
                    hid_action("RouteStylesChanged");
                    break;

                case F_Text:
                case F_TextScale:
                    text_scale = value / (double)FONT_CAPHEIGHT * 100.;
                    if (!absolute)
                        text_scale += Settings.TextScale;
                    SetTextScale(text_scale);
                    break;

                default:
                    err = 1;
                    break;
            }
            if (!err)
                return 0;
        }

        Message(_("Syntax error.  Usage:\n%s\n"),
                _("SetValue(Grid|Line|LineSize|Text|TextScale|ViaDrillingHole|Via|ViaSize, delta)"));
        return 1;
    }
};

REGISTER_ACTION(SetValueAction);

}} // namespace pcb::actions
