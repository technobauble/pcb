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

class MinClearGapAction : public Action {
public:
    MinClearGapAction() : Action("MinClearGap",
        "MinClearGap(delta)\n"
        "MinClearGap(Selected, delta)",
        "Ensures that polygons are a minimum distance from objects.") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        char* function = ARG(0);
        char* delta = ARG(1);
        char* units = ARG(2);
        bool absolute;
        Coord value;
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
                if (!TEST_FLAGS(flags, pin))
                    continue;
                if (pin->Clearance < value) {
                    ChangeObjectClearSize(PIN_TYPE, element, pin, 0, value, 1);
                    RestoreUndoSerialNumber();
                }
            }
            END_LOOP;
            PAD_LOOP(element);
            {
                if (!TEST_FLAGS(flags, pad))
                    continue;
                if (pad->Clearance < value) {
                    ChangeObjectClearSize(PAD_TYPE, element, pad, 0, value, 1);
                    RestoreUndoSerialNumber();
                }
            }
            END_LOOP;
        }
        END_LOOP;
        VIA_LOOP(PCB->Data);
        {
            if (!TEST_FLAGS(flags, via))
                continue;
            if (via->Clearance < value) {
                ChangeObjectClearSize(VIA_TYPE, via, 0, 0, value, 1);
                RestoreUndoSerialNumber();
            }
        }
        END_LOOP;
        ALLLINE_LOOP(PCB->Data);
        {
            if (!TEST_FLAGS(flags, line))
                continue;
            if (line->Clearance < value) {
                ChangeObjectClearSize(LINE_TYPE, layer, line, 0, value, 1);
                RestoreUndoSerialNumber();
            }
        }
        ENDALL_LOOP;
        ALLARC_LOOP(PCB->Data);
        {
            if (!TEST_FLAGS(flags, arc))
                continue;
            if (arc->Clearance < value) {
                ChangeObjectClearSize(ARC_TYPE, layer, arc, 0, value, 1);
                RestoreUndoSerialNumber();
            }
        }
        ENDALL_LOOP;
        RestoreUndoSerialNumber();
        IncrementUndoSerialNumber();
        return 0;
    }
};

REGISTER_ACTION(MinClearGapAction)

} // namespace
