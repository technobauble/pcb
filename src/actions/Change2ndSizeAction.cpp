#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "change.h"
#include "data.h"
#include "misc.h"
#include "search.h"
#include "set.h"
}

namespace pcb {
namespace actions {

class Change2ndSizeAction : public Action {
public:
    Change2ndSizeAction() : Action("ChangeDrillSize",
        "ChangeDrillSize(Object, delta)\n"
        "ChangeDrillSize(SelectedPins|SelectedVias|Selected|SelectedObjects, delta)",
        "Changes the drilling hole size of objects.") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        char* function = ARG(0);
        char* delta = ARG(1);
        char* units = ARG(2);
        bool absolute;
        Coord value;

        if (function && delta) {
            value = GetValue(delta, units, &absolute);

            switch (GetFunctionID(const_cast<char*>(function))) {
                case F_Object: {
                    int type;
                    void *ptr1, *ptr2, *ptr3;

                    gui->get_coords(const_cast<char*>(_("Select an Object")), &x, &y);
                    if ((type = SearchScreen(x, y, CHANGE2NDSIZE_TYPES,
                                            &ptr1, &ptr2, &ptr3)) != NO_TYPE) {
                        if (ChangeObject2ndSize(type, ptr1, ptr2, ptr3, value, absolute, true)) {
                            SetChangedFlag(true);
                        }
                    }
                    break;
                }

                case F_SelectedVias:
                    if (ChangeSelected2ndSize(VIA_TYPE, value, absolute)) {
                        SetChangedFlag(true);
                    }
                    break;

                case F_SelectedPins:
                    if (ChangeSelected2ndSize(PIN_TYPE, value, absolute)) {
                        SetChangedFlag(true);
                    }
                    break;

                case F_Selected:
                case F_SelectedObjects:
                    if (ChangeSelected2ndSize(PIN_TYPES, value, absolute)) {
                        SetChangedFlag(true);
                    }
                    break;
            }
        }
        return 0;
    }
};

REGISTER_ACTION(Change2ndSizeAction);

}} // namespace pcb::actions
