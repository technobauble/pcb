#include "Action.h"
#include "action_bridge.h"

extern "C" {
#include "global.h"
#include "change.h"
#include "data.h"
#include "misc.h"
#include "undo.h"
}

#include <cstring>

namespace {

class MinMaskGapAction : public Action {
public:
    MinMaskGapAction() : Action("MinMaskGap",
        "MinMaskGap(delta)\n"
        "MinMaskGap(Selected, delta)",
        "Ensures the mask is a minimum distance from pins and pads.") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        char* function = ARG(0);
        char* delta = ARG(1);
        char* units = ARG(2);
        bool absolute;
        Coord value;
        Coord thickness;
        int flags;

        if (!function)
            return 1;

        if (strcasecmp(function, "Selected") == 0) {
            flags = SELECTEDFLAG;
        } else {
            units = delta;
            delta = function;
            flags = 0;
        }
        value = 2 * GetValue(delta, units, &absolute);

        SaveUndoSerialNumber();
        ELEMENT_LOOP(PCB->Data);
        {
            PIN_LOOP(element);
            {
                if (!TEST_FLAGS(flags, pin) || !pin->Mask)
                    continue;

                thickness = pin->DrillingHole;

                if (pin->Thickness > thickness)
                    thickness = pin->Thickness;

                thickness += value;

                if (pin->Mask < thickness) {
                    ChangeObjectMaskSize(PIN_TYPE, element, pin, 0, thickness, 1);
                    RestoreUndoSerialNumber();
                }
            }
            END_LOOP;
            PAD_LOOP(element);
            {
                if (!TEST_FLAGS(flags, pad) || !pad->Mask)
                    continue;
                if (pad->Mask < pad->Thickness + value) {
                    ChangeObjectMaskSize(PAD_TYPE, element, pad, 0,
                                       pad->Thickness + value, 1);
                    RestoreUndoSerialNumber();
                }
            }
            END_LOOP;
        }
        END_LOOP;
        VIA_LOOP(PCB->Data);
        {
            if (!TEST_FLAGS(flags, via) || !via->Mask)
                continue;

            thickness = via->DrillingHole;
            if (via->Thickness > thickness)
                thickness = via->Thickness;
            thickness += value;

            if (via->Mask < thickness) {
                ChangeObjectMaskSize(VIA_TYPE, via, 0, 0, thickness, 1);
                RestoreUndoSerialNumber();
            }
        }
        END_LOOP;
        RestoreUndoSerialNumber();
        IncrementUndoSerialNumber();
        return 0;
    }
};

REGISTER_ACTION(MinMaskGapAction)

} // namespace
