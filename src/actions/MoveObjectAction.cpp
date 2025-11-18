#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "data.h"
#include "error.h"
#include "move.h"
#include "rats.h"
#include "rubberband.h"
#include "search.h"
}

namespace pcb {
namespace actions {

class MoveObjectAction : public Action {
public:
    MoveObjectAction() : Action("MoveObject",
        "MoveObject(X,Y,dim)",
        "Moves the object under the crosshair.") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        char* x_str = ARG(0);
        char* y_str = ARG(1);
        char* units = ARG(2);
        Coord nx, ny;
        bool absolute1, absolute2;
        void *ptr1, *ptr2, *ptr3;
        int type;

        ny = GetValue(y_str, units, &absolute1);
        nx = GetValue(x_str, units, &absolute2);

        type = SearchScreen(x, y, MOVE_TYPES, &ptr1, &ptr2, &ptr3);
        if (type == NO_TYPE) {
            Message(_("Nothing found under crosshair\n"));
            return 1;
        }
        if (absolute1)
            nx -= x;
        if (absolute2)
            ny -= y;
        Crosshair.AttachedObject.RubberbandN = 0;
        if (TEST_FLAG(RUBBERBANDFLAG, PCB))
            LookupRubberbandLines(type, ptr1, ptr2, ptr3);
        if (type == ELEMENT_TYPE)
            LookupRatLines(type, ptr1, ptr2, ptr3);
        MoveObjectAndRubberband(type, ptr1, ptr2, ptr3, nx, ny);
        SetChangedFlag(true);
        return 0;
    }
};

REGISTER_ACTION(MoveObjectAction);

}} // namespace pcb::actions
