#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "change.h"
#include "data.h"
#include "draw.h"
#include "misc.h"
#include "search.h"
#include "undo.h"
}

namespace pcb {
namespace actions {

class SetThermalAction : public Action {
public:
    SetThermalAction() : Action("SetThermal",
        "SetThermal(Object|Selected|SelectedElements|SelectedPins|SelectedVias, Style)\n"
        "Style = 0 - no thermal.\n"
        "Style = 1 has diagonal fingers with sharp edges.\n"
        "Style = 2 has horizontal and vertical fingers with sharp edges.\n"
        "Style = 3 is a solid connection to the plane.\n"
        "Style = 4 has diagonal fingers with rounded edges.\n"
        "Style = 5 has horizontal and vertical fingers with rounded edges.",
        "Set the thermal (on the current layer) of pins or vias.") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        char* function = ARG(0);
        char* style = ARG(1);
        void *ptr1, *ptr2, *ptr3;
        int type, kind;
        int err = 0;

        if (function && *function) {
            bool absolute;

            if (!style || !*style) {
                kind = PCB->ThermStyle;
                absolute = true;
            } else {
                kind = GetUnitlessValue(style, &absolute);
            }

            // To allow relative values we could search for the first selected
            // item and make 'kind' relative to that, but that's not too useful
            // and requires quite some code. For example there's no
            // GetFirstSelectedPin() function available. Let's postpone this
            // functionality, there are more urgent things to do.

            if (absolute) {
                switch (GetFunctionID(const_cast<char*>(function))) {
                    case F_Object:
                        if ((type = SearchScreen(Crosshair.X, Crosshair.Y, CHANGETHERMAL_TYPES,
                                                &ptr1, &ptr2, &ptr3)) != NO_TYPE) {
                            ChangeObjectThermal(type, ptr1, ptr2, ptr3, kind);
                            IncrementUndoSerialNumber();
                            Draw();
                        }
                        break;

                    case F_SelectedPins:
                        ChangeSelectedThermals(PIN_TYPE, kind);
                        break;

                    case F_SelectedVias:
                        ChangeSelectedThermals(VIA_TYPE, kind);
                        break;

                    case F_Selected:
                    case F_SelectedElements:
                        ChangeSelectedThermals(CHANGETHERMAL_TYPES, kind);
                        break;

                    default:
                        err = 1;
                        break;
                }
            } else {
                err = 1;
            }
        } else {
            err = 1;
        }

        if (err) {
            Message(_("Syntax error.  Usage:\n%s\n"), _("SetThermal(Object|Selected|SelectedElements|SelectedPins|SelectedVias, Style)"));
            return 1;
        }

        return 0;
    }
};

REGISTER_ACTION(SetThermalAction);

}} // namespace pcb::actions
