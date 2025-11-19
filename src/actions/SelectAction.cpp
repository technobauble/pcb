#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "buffer.h"
#include "copy.h"
#include "create.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "misc.h"
#include "remove.h"
#include "select.h"
#include "set.h"
#include "undo.h"
}

#include <cstring>

namespace pcb {
namespace actions {

// Syntax string for AFAIL macro
static const char* select_syntax =
    "Select(Object|ToggleObject)\n"
    "Select(All|Block|Connection|BuriedVias)\n"
    "Select(ElementByName|ObjectByName|PadByName|PinByName)\n"
    "Select(ElementByName|ObjectByName|PadByName|PinByName, Name)\n"
    "Select(TextByName|ViaByName|NetByName)\n"
    "Select(TextByName|ViaByName|NetByName, Name)\n"
    "Select(Convert)";

class SelectAction : public Action {
public:
    SelectAction() : Action("Select",
        "Toggles or sets the selection",
        "Select(Object|ToggleObject|All|Block|Connection|Found|BuriedVias|Convert)\n"
        "Select(ElementByName|ObjectByName|PadByName|PinByName|TextByName|ViaByName|NetByName[, pattern])") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        char* function = argc > 0 ? argv[0] : nullptr;
        if (function) {
            switch (GetFunctionID(function)) {
#if defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP)
                int type;
                // Select objects by their names
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
                        if (SelectObjectByName(type, pattern, true)) {
                            SetChangedFlag(true);
                        }
                        if (argc <= 1) {
                            free(pattern);
                        }
                    }
                    break;
                }
#endif /* defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP) */

                // Select a single object
                case F_ToggleObject:
                case F_Object:
                    if (SelectObject()) {
                        SetChangedFlag(true);
                    }
                    break;

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
                        SelectBlock(&box, true)) {
                        SetChangedFlag(true);
                        Crosshair.AttachedBox.State = STATE_FIRST;
                    }
                    notify_crosshair_change(true);
                    break;
                }

                // Select all visible objects
                case F_All:
                {
                    BoxType box;

                    box.X1 = -MAX_COORD;
                    box.Y1 = -MAX_COORD;
                    box.X2 = MAX_COORD;
                    box.Y2 = MAX_COORD;
                    if (SelectBlock(&box, true)) {
                        SetChangedFlag(true);
                    }
                    break;
                }

                // All logical connections
                case F_Found:
                    if (SelectByFlag(FOUNDFLAG, true)) {
                        Draw();
                        IncrementUndoSerialNumber();
                        SetChangedFlag(true);
                    }
                    break;

                // All physical connections
                case F_Connection:
                    if (SelectByFlag(CONNECTEDFLAG, true)) {
                        Draw();
                        IncrementUndoSerialNumber();
                        SetChangedFlag(true);
                    }
                    break;

                case F_BuriedVias:
                    if (SelectBuriedVias(true)) {
                        Draw();
                        IncrementUndoSerialNumber();
                        SetChangedFlag(true);
                    }
                    break;

                // Note: F_Convert case not migrated - uses static Note struct
                // which is complex and shared with other actions. Remains in C for now.

                default:
                    Message(_("Unknown Select function: %s\n"), function);
                    return 1;
            }
        }
        return 0;
    }
};

REGISTER_ACTION(SelectAction);

}} // namespace pcb::actions
