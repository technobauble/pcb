#include "Action.h"
#include "action_bridge.h"

extern "C" {
#include "global.h"
#include "change.h"
#include "data.h"
#include "misc.h"
#include "search.h"
#include "set.h"
}

namespace {

class ChangeClearSizeAction : public Action {
public:
    ChangeClearSizeAction() : Action("ChangeClearSize",
        "ChangeClearSize(Object, delta)\n"
        "ChangeClearSize(SelectedPins|SelectedPads|SelectedVias, delta)\n"
        "ChangeClearSize(SelectedLines|SelectedArcs, delta\n"
        "ChangeClearSize(Selected|SelectedObjects, delta)",
        "Changes the clearance size of objects.") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        char* function = ARG(0);
        char* delta = ARG(1);
        char* units = ARG(2);
        bool absolute;
        Coord value;

        if (function && delta) {
            value = 2 * GetValue(delta, units, &absolute);
            if ((value == 0) && !absolute) {
                value = delta[0] == '-' ? -Settings.increments->clear
                                        : Settings.increments->clear;
            }

            switch (GetFunctionID(const_cast<char*>(function))) {
                case F_Object: {
                    int type;
                    void *ptr1, *ptr2, *ptr3;

                    gui->get_coords(const_cast<char*>(_("Select an Object")), &x, &y);
                    if ((type = SearchScreen(x, y, CHANGECLEARSIZE_TYPES,
                                            &ptr1, &ptr2, &ptr3)) != NO_TYPE) {
                        if (ChangeObjectClearSize(type, ptr1, ptr2, ptr3, value, absolute)) {
                            SetChangedFlag(true);
                        }
                    }
                    break;
                }

                case F_SelectedVias:
                    if (ChangeSelectedClearSize(VIA_TYPE, value, absolute)) {
                        SetChangedFlag(true);
                    }
                    break;

                case F_SelectedPads:
                    if (ChangeSelectedClearSize(PAD_TYPE, value, absolute)) {
                        SetChangedFlag(true);
                    }
                    break;

                case F_SelectedPins:
                    if (ChangeSelectedClearSize(PIN_TYPE, value, absolute)) {
                        SetChangedFlag(true);
                    }
                    break;

                case F_SelectedLines:
                    if (ChangeSelectedClearSize(LINE_TYPE, value, absolute)) {
                        SetChangedFlag(true);
                    }
                    break;

                case F_SelectedArcs:
                    if (ChangeSelectedClearSize(ARC_TYPE, value, absolute)) {
                        SetChangedFlag(true);
                    }
                    break;

                case F_Selected:
                case F_SelectedObjects:
                    if (ChangeSelectedClearSize(CHANGECLEARSIZE_TYPES, value, absolute)) {
                        SetChangedFlag(true);
                    }
                    break;
            }
        }
        return 0;
    }
};

REGISTER_ACTION(ChangeClearSizeAction)

} // namespace
