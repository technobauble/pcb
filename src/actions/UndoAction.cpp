#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "actions/ActionContext.h"
#include "crosshair.h"
#include "draw.h"
#include "misc.h"
#include "move.h"
#include "search.h"
#include "set.h"
#include "undo.h"
}

#include <cstring>

namespace pcb {
namespace actions {

class UndoAction : public Action {
public:
    UndoAction() : Action("Undo",
        "Undo recent changes",
        "Undo()\n"
        "Undo(ClearList)") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        char* function = argc > 0 ? argv[0] : nullptr;

        if (!function || !*function) {
            // Don't allow undo in the middle of an operation
            if (Settings.Mode != POLYGONHOLE_MODE &&
                Crosshair.AttachedObject.State != STATE_FIRST) {
                return 1;
            }
            if (Crosshair.AttachedBox.State != STATE_FIRST &&
                Settings.Mode != ARC_MODE) {
                return 1;
            }

            // Undo the last operation
            notify_crosshair_change(false);

            if ((Settings.Mode == POLYGON_MODE ||
                 Settings.Mode == POLYGONHOLE_MODE) &&
                Crosshair.AttachedPolygon.PointN) {
                GoToPreviousPoint();
                notify_crosshair_change(true);
                return 0;
            }

            // Move anchor point if undoing during line creation
            if (Settings.Mode == LINE_MODE) {
                if (Crosshair.AttachedLine.State == STATE_SECOND) {
                    if (TEST_FLAG(AUTODRCFLAG, PCB)) {
                        Undo(true);  // undo the connection find
                    }
                    Crosshair.AttachedLine.State = STATE_FIRST;
                    SetLocalRef(0, 0, false);
                    notify_crosshair_change(true);
                    return 0;
                }

                if (Crosshair.AttachedLine.State == STATE_THIRD) {
                    int type;
                    void *ptr1, *ptr3, *ptrtmp;
                    LineType *ptr2;

                    // This search is guaranteed to succeed
                    SearchObjectByLocation(LINE_TYPE | RATLINE_TYPE, &ptr1,
                                         &ptrtmp, &ptr3,
                                         Crosshair.AttachedLine.Point1.X,
                                         Crosshair.AttachedLine.Point1.Y, 0);
                    ptr2 = (LineType *) ptrtmp;

                    // Save both ends of line
                    Crosshair.AttachedLine.Point2.X = ptr2->Point1.X;
                    Crosshair.AttachedLine.Point2.Y = ptr2->Point1.Y;

                    if ((type = Undo(true))) {
                        SetChangedFlag(true);
                    }

                    // Check that the undo was of the right type
                    if ((type & UNDO_CREATE) == 0) {
                        // Wrong undo type, restore anchor points
                        Crosshair.AttachedLine.Point2.X = Crosshair.AttachedLine.Point1.X;
                        Crosshair.AttachedLine.Point2.Y = Crosshair.AttachedLine.Point1.Y;
                        notify_crosshair_change(true);
                        return 0;
                    }

                    // Move to new anchor
                    Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X;
                    Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y;

                    // Check if an intermediate point was removed
                    if (type & UNDO_REMOVE) {
                        // This search should find the restored line
                        SearchObjectByLocation(LINE_TYPE | RATLINE_TYPE, &ptr1,
                                             &ptrtmp, &ptr3,
                                             Crosshair.AttachedLine.Point2.X,
                                             Crosshair.AttachedLine.Point2.Y, 0);
                        ptr2 = (LineType *) ptrtmp;

                        if (TEST_FLAG(AUTODRCFLAG, PCB)) {
                            // Undo loses CONNECTEDFLAG and FOUNDFLAG
                            SET_FLAG(CONNECTEDFLAG, ptr2);
                            SET_FLAG(FOUNDFLAG, ptr2);
                            DrawLine(CURRENT, ptr2);
                        }

                        Crosshair.AttachedLine.Point1.X =
                            Crosshair.AttachedLine.Point2.X = ptr2->Point2.X;
                        Crosshair.AttachedLine.Point1.Y =
                            Crosshair.AttachedLine.Point2.Y = ptr2->Point2.Y;
                    }

                    FitCrosshairIntoGrid(Crosshair.X, Crosshair.Y);
                    AdjustAttachedObjects();

                    // Use ActionContext for addedLines and lastLayer
                    if (--pcb_action_context->addedLines == 0) {
                        Crosshair.AttachedLine.State = STATE_SECOND;
                        pcb_action_context->lastLayer = CURRENT;
                    } else {
                        // This search is guaranteed to succeed too
                        SearchObjectByLocation(LINE_TYPE | RATLINE_TYPE, &ptr1,
                                             &ptrtmp, &ptr3,
                                             Crosshair.AttachedLine.Point1.X,
                                             Crosshair.AttachedLine.Point1.Y, 0);
                        ptr2 = (LineType *) ptrtmp;
                        pcb_action_context->lastLayer = (LayerType *) ptr1;
                    }

                    notify_crosshair_change(true);
                    return 0;
                }
            }

            if (Settings.Mode == ARC_MODE) {
                if (Crosshair.AttachedBox.State == STATE_SECOND) {
                    Crosshair.AttachedBox.State = STATE_FIRST;
                    notify_crosshair_change(true);
                    return 0;
                }

                if (Crosshair.AttachedBox.State == STATE_THIRD) {
                    void *ptr1, *ptr2, *ptr3;
                    BoxType *bx;

                    // Guaranteed to succeed
                    SearchObjectByLocation(ARC_TYPE, &ptr1, &ptr2, &ptr3,
                                         Crosshair.AttachedBox.Point1.X,
                                         Crosshair.AttachedBox.Point1.Y, 0);
                    bx = GetArcEnds((ArcType *) ptr2);

                    Crosshair.AttachedBox.Point1.X =
                        Crosshair.AttachedBox.Point2.X = bx->X1;
                    Crosshair.AttachedBox.Point1.Y =
                        Crosshair.AttachedBox.Point2.Y = bx->Y1;

                    AdjustAttachedObjects();

                    // Use ActionContext for addedLines
                    if (--pcb_action_context->addedLines == 0) {
                        Crosshair.AttachedBox.State = STATE_SECOND;
                    }
                }
            }

            // Undo the last destructive operation
            if (Undo(true)) {
                SetChangedFlag(true);
            }
        } else if (function) {
            switch (GetFunctionID(function)) {
                // Clear 'undo objects' list
                case F_ClearList:
                    ClearUndoList(false);
                    break;
            }
        }

        notify_crosshair_change(true);
        return 0;
    }
};

REGISTER_ACTION(UndoAction);

}} // namespace pcb::actions
