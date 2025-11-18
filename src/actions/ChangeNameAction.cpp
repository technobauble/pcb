#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "change.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "hid.h"
#include "move.h"
#include "rats.h"
#include "search.h"
#include "set.h"
#include "undo.h"
}

namespace pcb {
namespace actions {

class ChangeNameAction : public Action {
public:
    ChangeNameAction() : Action("ChangeName",
        "ChangeName(Object)\n"
        "ChangeName(Layout|Layer)",
        "Sets the name of objects.") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        char* function = ARG(0);
        char* name;

        if (function) {
            switch (GetFunctionID(function)) {
                /* change the name of an object */
                case F_Object: {
                    int type;
                    void *ptr1, *ptr2, *ptr3;

                    gui->get_coords(_("Select an Object"), &x, &y);
                    if ((type = SearchScreen(x, y, CHANGENAME_TYPES,
                                            &ptr1, &ptr2, &ptr3)) != NO_TYPE) {
                        SaveUndoSerialNumber();
                        if (QueryInputAndChangeObjectName(type, ptr1, ptr2, ptr3)) {
                            SetChangedFlag(true);
                            if (type == ELEMENT_TYPE) {
                                RubberbandType *ptr;
                                int i;

                                RestoreUndoSerialNumber();
                                Crosshair.AttachedObject.RubberbandN = 0;
                                LookupRatLines(type, ptr1, ptr2, ptr3);
                                ptr = Crosshair.AttachedObject.Rubberband;
                                for (i = 0; i < Crosshair.AttachedObject.RubberbandN;
                                     i++, ptr++) {
                                    if (PCB->RatOn)
                                        EraseRat(static_cast<RatType*>(ptr->Line));
                                    MoveObjectToRemoveUndoList(RATLINE_TYPE,
                                                              ptr->Line, ptr->Line,
                                                              ptr->Line);
                                }
                                IncrementUndoSerialNumber();
                                Draw();
                            }
                        }
                    }
                    break;
                }

                /* change the layout's name */
                case F_Layout:
                    name = gui->prompt_for(_("Enter the layout name:"), EMPTY(PCB->Name));
                    /* NB: ChangeLayoutName takes ownership of the passed memory */
                    if (name && ChangeLayoutName(name))
                        SetChangedFlag(true);
                    break;

                /* change the name of the active layer */
                case F_Layer:
                    name = gui->prompt_for(_("Enter the layer name:"),
                                          EMPTY(CURRENT->Name));
                    /* NB: ChangeLayerName takes ownership of the passed memory */
                    if (name && ChangeLayerName(CURRENT, name))
                        SetChangedFlag(true);
                    break;
            }
        }
        return 0;
    }
};

REGISTER_ACTION(ChangeNameAction);

}} // namespace pcb::actions
