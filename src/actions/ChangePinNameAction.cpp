#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "data.h"
#include "error.h"
#include "set.h"
#include "undo.h"
}

#include <cstring>

namespace pcb {
namespace actions {

class ChangePinNameAction : public Action {
public:
    ChangePinNameAction() : Action("ChangePinName",
        "ChangePinName(ElementName,PinNumber,PinName)",
        "Sets the name of a specific pin on a specific element.") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        int changed = 0;
        char *refdes, *pinnum, *pinname;

        if (argc != 3) {
            Message(_("Syntax error.  Usage:\n%s\n"), _("ChangePinName(ElementName,PinNumber,PinName)"));
            return 1;
        }

        refdes = argv[0];
        pinnum = argv[1];
        pinname = argv[2];

        ELEMENT_LOOP(PCB->Data);
        {
            if (NSTRCMP(refdes, NAMEONPCB_NAME(element)) == 0) {
                PIN_LOOP(element);
                {
                    if (NSTRCMP(pinnum, pin->Number) == 0) {
                        AddObjectToChangeNameUndoList(PIN_TYPE, NULL, NULL,
                                                     pin, pin->Name);
                        // Note: we can't free() pin->Name first because
                        // it is used in the undo list
                        pin->Name = strdup(pinname);
                        SetChangedFlag(true);
                        changed = 1;
                    }
                }
                END_LOOP;

                PAD_LOOP(element);
                {
                    if (NSTRCMP(pinnum, pad->Number) == 0) {
                        AddObjectToChangeNameUndoList(PAD_TYPE, NULL, NULL,
                                                     pad, pad->Name);
                        // Note: we can't free() pad->Name first because
                        // it is used in the undo list
                        pad->Name = strdup(pinname);
                        SetChangedFlag(true);
                        changed = 1;
                    }
                }
                END_LOOP;
            }
        }
        END_LOOP;

        // Done with our action so increment the undo # if we actually changed anything
        if (changed) {
            IncrementUndoSerialNumber();
            gui->invalidate_all();
        }

        return 0;
    }
};

REGISTER_ACTION(ChangePinNameAction);

}} // namespace pcb::actions
