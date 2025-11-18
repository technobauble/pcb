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

namespace {

class ChangeSizeAction : public Action {
public:
    ChangeSizeAction() : Action("ChangeSize",
        "ChangeSize(Object, delta)\n"
        "ChangeSize(SelectedObjects|Selected, delta)\n"
        "ChangeSize(SelectedLines|SelectedPins|SelectedVias, delta)\n"
        "ChangeSize(SelectedPads|SelectedTexts|SelectedNames, delta)\n"
        "ChangeSize(SelectedElements, delta)",
        "Changes the size of objects.") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        char* function = ARG(0);
        char* delta = ARG(1);
        char* units = ARG(2);
        bool absolute;
        Coord value;

        if (function && delta) {
            value = GetValue(delta, units, &absolute);
            if (value == 0) {
                value = delta[0] == '-' ? -Settings.increments->size
                                        : Settings.increments->size;
            }

            switch (GetFunctionID(const_cast<char*>(function))) {
                case F_Object: {
                    int type;
                    void *ptr1, *ptr2, *ptr3;

                    if ((type = SearchScreen(Crosshair.X, Crosshair.Y, CHANGESIZE_TYPES,
                                            &ptr1, &ptr2, &ptr3)) != NO_TYPE) {
                        if (TEST_FLAG(LOCKFLAG, static_cast<PinType*>(ptr2))) {
                            Message(_("Sorry, the object is locked\n"));
                        }
                        if (ChangeObjectSize(type, ptr1, ptr2, ptr3, value, absolute)) {
                            SetChangedFlag(true);
                        }
                    }
                    break;
                }

                case F_SelectedVias:
                    if (ChangeSelectedSize(VIA_TYPE, value, absolute)) {
                        SetChangedFlag(true);
                    }
                    break;

                case F_SelectedPins:
                    if (ChangeSelectedSize(PIN_TYPE, value, absolute)) {
                        SetChangedFlag(true);
                    }
                    break;

                case F_SelectedPads:
                    if (ChangeSelectedSize(PAD_TYPE, value, absolute)) {
                        SetChangedFlag(true);
                    }
                    break;

                case F_SelectedArcs:
                    if (ChangeSelectedSize(ARC_TYPE, value, absolute)) {
                        SetChangedFlag(true);
                    }
                    break;

                case F_SelectedLines:
                    if (ChangeSelectedSize(LINE_TYPE, value, absolute)) {
                        SetChangedFlag(true);
                    }
                    break;

                case F_SelectedTexts:
                    if (ChangeSelectedSize(TEXT_TYPE, value, absolute)) {
                        SetChangedFlag(true);
                    }
                    break;

                case F_SelectedNames:
                    if (ChangeSelectedSize(ELEMENTNAME_TYPE, value, absolute)) {
                        SetChangedFlag(true);
                    }
                    break;

                case F_SelectedElements:
                    if (ChangeSelectedSize(ELEMENT_TYPE, value, absolute)) {
                        SetChangedFlag(true);
                    }
                    break;

                case F_Selected:
                case F_SelectedObjects:
                    if (ChangeSelectedSize(CHANGESIZE_TYPES, value, absolute)) {
                        SetChangedFlag(true);
                    }
                    break;
            }
        }
        return 0;
    }
};

REGISTER_ACTION(ChangeSizeAction)

} // namespace
