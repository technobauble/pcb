#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "select.h"
#include "set.h"
#include "undo.h"
}

#include <cstring>

namespace pcb {
namespace actions {

// Syntax string for AFAIL macro
static const char* unselect_syntax =
    "Unselect(All|Block|Connection)\n"
    "Unselect(ElementByName|ObjectByName|PadByName|PinByName)\n"
    "Unselect(ElementByName|ObjectByName|PadByName|PinByName, Name)\n"
    "Unselect(TextByName|ViaByName)\n"
    "Unselect(TextByName|ViaByName, Name)\n";

class UnselectAction : public Action {
public:
    UnselectAction() : Action("Unselect",
        "Unselects the object at the pointer location or the specified objects",
        "Unselect(All|Block|Connection|Found)\n"
        "Unselect(ElementByName|ObjectByName|PadByName|PinByName|TextByName|ViaByName|NetByName[, pattern])") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        char* function = argc > 0 ? argv[0] : nullptr;
        if (function) {
            switch (GetFunctionID(function)) {
#if defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP)
                int type;
                // Unselect objects by their names
                case F_ElementByName:
                    type = ELEMENT_TYPE;
                    goto commonByName;
                case F_ObjectByName:
                    type = ALL_TYPES;
                    goto commonByName;
                case F_PadByName:
                    type = PAD_TYPE;
                    goto commonByName;
                case F_PinByName:
                    type = PIN_TYPE;
                    goto commonByName;
                case F_TextByName:
                    type = TEXT_TYPE;
                    goto commonByName;
                case F_ViaByName:
                    type = VIA_TYPE;
                    goto commonByName;
                case F_NetByName:
                    type = NET_TYPE;
                    goto commonByName;

                commonByName:
                {
                    char* pattern = argc > 1 ? argv[1] : nullptr;

                    if (pattern || (pattern = gui->prompt_for(_("Enter pattern:"), "")) != nullptr) {
                        if (SelectObjectByName(type, pattern, false)) {
                            SetChangedFlag(true);
                        }
                        if (argc <= 1) {
                            free(pattern);
                        }
                    }
                    break;
                }
#endif /* defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP) */

                // All objects in block
                case F_Block:
                {
                    BoxType box;

                    box.X1 = MIN(Crosshair.AttachedBox.Point1.X, Crosshair.AttachedBox.Point2.X);
                    box.Y1 = MIN(Crosshair.AttachedBox.Point1.Y, Crosshair.AttachedBox.Point2.Y);
                    box.X2 = MAX(Crosshair.AttachedBox.Point1.X, Crosshair.AttachedBox.Point2.X);
                    box.Y2 = MAX(Crosshair.AttachedBox.Point1.Y, Crosshair.AttachedBox.Point2.Y);
                    notify_crosshair_change(false);
                    NotifyBlock();
                    if (Crosshair.AttachedBox.State == STATE_THIRD &&
                        SelectBlock(&box, false)) {
                        SetChangedFlag(true);
                        Crosshair.AttachedBox.State = STATE_FIRST;
                    }
                    notify_crosshair_change(true);
                    break;
                }

                // Unselect all visible objects
                case F_All:
                {
                    BoxType box;

                    box.X1 = -MAX_COORD;
                    box.Y1 = -MAX_COORD;
                    box.X2 = MAX_COORD;
                    box.Y2 = MAX_COORD;
                    if (SelectBlock(&box, false)) {
                        SetChangedFlag(true);
                    }
                    break;
                }

                // All logical connections
                case F_Found:
                    if (SelectByFlag(FOUNDFLAG, false)) {
                        Draw();
                        IncrementUndoSerialNumber();
                        SetChangedFlag(true);
                    }
                    break;

                // All physical connections
                case F_Connection:
                    if (SelectByFlag(CONNECTEDFLAG, false)) {
                        Draw();
                        IncrementUndoSerialNumber();
                        SetChangedFlag(true);
                    }
                    break;

                case F_BuriedVias:
                    if (SelectBuriedVias(false)) {
                        Draw();
                        IncrementUndoSerialNumber();
                        SetChangedFlag(true);
                    }
                    break;

                default:
                    Message(_("Unknown Unselect function: %s\n"), function);
                    return 1;
            }
        }
        return 0;
    }
};

REGISTER_ACTION(UnselectAction);

}} // namespace pcb::actions
