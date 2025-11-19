#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "move.h"
#include "set.h"
#include "undo.h"
}

namespace pcb {
namespace actions {

#define GAP MIL_TO_COORD(100)

// Syntax string for AFAIL macro
static const char* disperseelements_syntax =
    "DisperseElements(All|Selected)";

class DisperseElementsAction : public Action {
public:
    DisperseElementsAction() : Action("DisperseElements",
        "Disperses elements",
        "DisperseElements(All|Selected)") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        char* function = argc > 0 ? argv[0] : nullptr;
        Coord minx = GAP;
        Coord miny = GAP;
        Coord maxy = GAP;
        Coord dx, dy;
        int all = 0;
        int bad = 0;

        if (!function || !*function) {
            bad = 1;
        } else {
            switch (GetFunctionID(function)) {
                case F_All:
                    all = 1;
                    break;

                case F_Selected:
                    all = 0;
                    break;

                default:
                    bad = 1;
            }
        }

        if (bad) {
            Message(_("DisperseElements requires All or Selected argument\n"));
            return 1;
        }

        ELEMENT_LOOP(PCB->Data);
        {
            /*
             * If we want to disperse selected elements, maybe we need smarter
             * code here to avoid putting components on top of others which
             * are not selected. For now, I'm assuming that this is typically
             * going to be used either with a brand new design or a scratch
             * design holding some new components
             */
            if (!TEST_FLAG(LOCKFLAG, element) &&
                (all || TEST_FLAG(SELECTEDFLAG, element))) {

                // Figure out how much to move the element
                dx = minx - element->BoundingBox.X1;

                // Snap to the grid
                dx -= (element->MarkX + dx) % PCB->Grid;

                // And add one grid size so we make sure we always space by GAP or more
                dx += PCB->Grid;

                // Figure out if this row has room. If not, start a new row
                if (GAP + element->BoundingBox.X2 + dx > PCB->MaxWidth) {
                    miny = maxy + GAP;
                    minx = GAP;
                }

                // Figure out how much to move the element
                dx = minx - element->BoundingBox.X1;
                dy = miny - element->BoundingBox.Y1;

                // Snap to the grid
                dx -= (element->MarkX + dx) % PCB->Grid;
                dx += PCB->Grid;
                dy -= (element->MarkY + dy) % PCB->Grid;
                dy += PCB->Grid;

                // Move the element
                MoveElementLowLevel(PCB->Data, element, dx, dy);

                // And add to the undo list so we can undo this operation
                AddObjectToMoveUndoList(ELEMENT_TYPE, nullptr, nullptr, element, dx, dy);

                // Keep track of how tall this row is
                minx += element->BoundingBox.X2 - element->BoundingBox.X1 + GAP;
                if (maxy < element->BoundingBox.Y2) {
                    maxy = element->BoundingBox.Y2;
                }
            }
        }
        END_LOOP;

        // Done with our action so increment the undo #
        IncrementUndoSerialNumber();

        Redraw();
        SetChangedFlag(true);

        return 0;
    }
};

REGISTER_ACTION(DisperseElementsAction);

#undef GAP

}} // namespace pcb::actions
