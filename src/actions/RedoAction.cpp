#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "crosshair.h"
#include "data.h"
#include "set.h"
#include "undo.h"
}

namespace pcb {
namespace actions {

class RedoAction : public Action {
public:
    RedoAction() : Action("Redo",
        "Redo()",
        "Redo recent \"undo\" operations.") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        if (((Settings.Mode == POLYGON_MODE ||
              Settings.Mode == POLYGONHOLE_MODE) &&
             Crosshair.AttachedPolygon.PointN) ||
            Crosshair.AttachedLine.State == STATE_SECOND)
            return 1;
        notify_crosshair_change(false);
        if (Redo(true)) {
            SetChangedFlag(true);
            if (Settings.Mode == LINE_MODE &&
                Crosshair.AttachedLine.State != STATE_FIRST) {
                LineType *line = static_cast<LineType*>(g_list_last(CURRENT->Line)->data);
                Crosshair.AttachedLine.Point1.X =
                    Crosshair.AttachedLine.Point2.X = line->Point2.X;
                Crosshair.AttachedLine.Point1.Y =
                    Crosshair.AttachedLine.Point2.Y = line->Point2.Y;
                addedLines++;
            }
        }
        notify_crosshair_change(true);
        return 0;
    }
};

REGISTER_ACTION(RedoAction);

}} // namespace pcb::actions
