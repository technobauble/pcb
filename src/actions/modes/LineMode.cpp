/*!
 * \file src/actions/modes/LineMode.cpp
 *
 * \brief Line/trace drawing mode implementation
 *
 * Allows the user to draw copper traces (lines) and rat lines by clicking
 * points. This is a complex StatefulMode with support for:
 * - Regular line drawing
 * - Rat line drawing (RatDraw mode)
 * - AutoDRC connection lookups
 * - Automatic via creation when changing layers
 * - Clipping modes (45°/90° angle constraints)
 *
 * State machine:
 * - STATE_FIRST: Waiting for start point click
 * - STATE_SECOND: Start point set, tracking endpoint
 * - STATE_THIRD: Ready to create line segment
 *
 * Extracted from action.c NotifyLine() and LINE_MODE case in NotifyMode().
 */

#include "StatefulMode.h"
#include "ModesCommon.h"

extern "C" {
#include "create.h"
#include "draw.h"
#include "crosshair.h"
#include "rats.h"
#include "find.h"
#include "autoroute.h"
}

namespace pcb {
namespace modes {

/*!
 * \brief Line/trace drawing mode
 *
 * Creates copper traces by clicking start and end points.
 * Supports continuous drawing where each segment end becomes
 * the next segment start.
 *
 * Features:
 * - Click to set line start point
 * - Rubber-band preview follows cursor
 * - Click again to create segment and start next
 * - Supports RatDraw mode for rat lines
 * - AutoDRC mode highlights connections
 * - Auto-creates vias when layer changes mid-trace
 */
class LineMode : public StatefulMode {
public:
    explicit LineMode(ActionContext* context)
        : StatefulMode(context)
        , attachment_(Crosshair.AttachedLine)
    {}

    void onNotify(Coord x, Coord y) override {
        // First update state machine (handles STATE_FIRST -> STATE_SECOND)
        notifyLine();

        // If we reached STATE_THIRD, create the line
        if (getState() == STATE_THIRD) {
            createLine();
        }
    }

    void onRelease() override {
        // Line mode doesn't use release
    }

    const char* getName() const override { return "Line"; }
    int getModeId() const override { return LINE_MODE; }

protected:
    IAttachedState& getAttachment() override {
        return attachment_;
    }

private:
    AttachedLineAdapter attachment_;

    /*!
     * \brief Handle state transitions (extracted from NotifyLine)
     *
     * STATE_FIRST → STATE_SECOND:
     * - In RatDraw mode, must click on pin/pad/via
     * - In AutoDRC mode, lookup connections for highlighting
     * - Snap to pin/pad/via center if clicked near one
     * - Otherwise use crosshair position
     *
     * STATE_SECOND → STATE_THIRD:
     * - Save current layer for auto-via detection
     * - Advance to STATE_THIRD to trigger line creation
     */
    void notifyLine() {
        int type = NO_TYPE;
        void *ptr1, *ptr2, *ptr3;

        // Set local reference if needed
        if (!Marked.status || TEST_FLAG(LOCALREFFLAG, PCB)) {
            SetLocalRef(Crosshair.X, Crosshair.Y, true);
        }

        switch (getState()) {
        case STATE_FIRST:
            // RatDraw mode requires starting on a pin/pad/via
            if (PCB->RatDraw && SearchScreen(Crosshair.X, Crosshair.Y,
                                             PAD_TYPE | PIN_TYPE,
                                             &ptr1, &ptr1, &ptr1) == NO_TYPE) {
                gui->beep();
                break;
            }

            // AutoDRC mode: lookup connections for highlighting
            if (TEST_FLAG(AUTODRCFLAG, PCB) && Settings.Mode == LINE_MODE) {
                type = SearchScreen(Crosshair.X, Crosshair.Y,
                                   PIN_TYPE | PAD_TYPE | VIA_TYPE,
                                   &ptr1, &ptr2, &ptr3);
                LookupConnection(Crosshair.X, Crosshair.Y, true, 1, CONNECTEDFLAG, false);
                LookupConnection(Crosshair.X, Crosshair.Y, true, 1, FOUNDFLAG, true);
            }

            // Snap to pin/via center if found
            if (type == PIN_TYPE || type == VIA_TYPE) {
                PinType* pin = (PinType*)ptr2;
                attachment_.setPoint1({pin->X, pin->Y, 0, 0, 0});
                attachment_.setPoint2({pin->X, pin->Y, 0, 0, 0});
            }
            // Snap to nearest pad endpoint
            else if (type == PAD_TYPE) {
                PadType* pad = (PadType*)ptr2;
                double d1 = Distance(Crosshair.X, Crosshair.Y,
                                    pad->Point1.X, pad->Point1.Y);
                double d2 = Distance(Crosshair.X, Crosshair.Y,
                                    pad->Point2.X, pad->Point2.Y);
                if (d2 < d1) {
                    attachment_.setPoint1(pad->Point2);
                    attachment_.setPoint2(pad->Point2);
                } else {
                    attachment_.setPoint1(pad->Point1);
                    attachment_.setPoint2(pad->Point1);
                }
            }
            // Otherwise use crosshair position
            else {
                PointType pt = {Crosshair.X, Crosshair.Y, 0, 0, 0};
                attachment_.setPoint1(pt);
                attachment_.setPoint2(pt);
            }
            setState(STATE_SECOND);
            break;

        case STATE_SECOND:
            // Fall through to advance state
            context_->lastLayer = CURRENT;
            // Fall through intentional

        default:
            // Advance to STATE_THIRD to trigger line creation
            setState(STATE_THIRD);
            break;
        }
    }

    /*!
     * \brief Create line segment(s)
     *
     * Called when STATE_THIRD is reached. Handles:
     * - Clicking on start point = cancel (reset mode)
     * - RatDraw mode = create rat line
     * - Normal mode = create copper trace
     * - Clipping mode = create one or two 45°/90° segments
     * - Auto-create via if layer changed
     * - Reset Point1 to Point2 for continuous drawing
     */
    void createLine() {
        void *ptr1, *ptr2, *ptr3;
        PointType p1 = attachment_.getPoint1();
        PointType p2 = attachment_.getPoint2();

        // Clicking on start point cancels the line
        if (Crosshair.X == p1.X && Crosshair.Y == p1.Y) {
            SetMode(LINE_MODE);  // This resets the mode
            return;
        }

        // RatDraw mode: create rat line
        if (PCB->RatDraw) {
            createRatLine();
            return;
        }

        // Normal line creation
        int line_flags = 0;

        if (TEST_FLAG(AUTODRCFLAG, PCB) && !TEST_SILK_LAYER(CURRENT)) {
            line_flags |= CONNECTEDFLAG | FOUNDFLAG;
        }

        if (TEST_FLAG(CLEARNEWFLAG, PCB)) {
            line_flags |= CLEARLINEFLAG;
        }

        // Handle clipping mode segment swap
        // If Point1 == Point2 but Note.X/Y differs, we need to create
        // the second segment only - so swap the points
        if (PCB->Clipping &&
            p1.X == p2.X && p1.Y == p2.Y &&
            (p2.X != context_->Note.X || p2.Y != context_->Note.Y)) {
            p2.X = context_->Note.X;
            p2.Y = context_->Note.Y;
            attachment_.setPoint2(p2);
        }

        // Create first line segment if not zero length
        if (p1.X != p2.X || p1.Y != p2.Y) {
            LineType* line = CreateDrawnLineOnLayer(
                CURRENT, p1.X, p1.Y, p2.X, p2.Y,
                Settings.LineThickness, 2 * Settings.Keepaway,
                MakeFlags(line_flags));

            if (line != NULL) {
                context_->addedLines++;
                AddObjectToCreateUndoList(LINE_TYPE, CURRENT, line, line);
                DrawLine(CURRENT, line);
            }

            // Auto-create via if layer changed
            createAutoVia(p1.X, p1.Y);

            // Update Point1 to Point2 for continuous drawing
            attachment_.setPoint1(p2);
            IncrementUndoSerialNumber();
            context_->lastLayer = CURRENT;
        }

        // Create second segment if clipping mode and Note differs from Point2
        if (PCB->Clipping &&
            (context_->Note.X != p2.X || context_->Note.Y != p2.Y)) {

            LineType* line = CreateDrawnLineOnLayer(
                CURRENT, p2.X, p2.Y,
                context_->Note.X, context_->Note.Y,
                Settings.LineThickness, 2 * Settings.Keepaway,
                MakeFlags(line_flags));

            if (line != NULL) {
                context_->addedLines++;
                AddObjectToCreateUndoList(LINE_TYPE, CURRENT, line, line);
                IncrementUndoSerialNumber();
                DrawLine(CURRENT, line);
            }

            // Move to new start point
            PointType newPt = {context_->Note.X, context_->Note.Y, 0, 0, 0};
            attachment_.setPoint1(newPt);
            attachment_.setPoint2(newPt);

            // Optionally swap clipping direction
            if (TEST_FLAG(SWAPSTARTDIRFLAG, PCB)) {
                PCB->Clipping ^= 3;
            }
        }

        // AutoDRC: lookup connections at new position
        if (TEST_FLAG(AUTODRCFLAG, PCB) && !TEST_SILK_LAYER(CURRENT)) {
            LookupConnection(context_->Note.X, context_->Note.Y,
                            true, 1, CONNECTEDFLAG, false);
        }

        Draw();
    }

    /*!
     * \brief Create rat line (in RatDraw mode)
     */
    void createRatLine() {
        RatType* line = AddNet();
        if (line != NULL) {
            context_->addedLines++;
            AddObjectToCreateUndoList(RATLINE_TYPE, line, line, line);
            IncrementUndoSerialNumber();
            DrawRat(line);

            // Update Point1 for continuous rat drawing
            PointType p2 = attachment_.getPoint2();
            attachment_.setPoint1(p2);

            Draw();
        }
    }

    /*!
     * \brief Create via if layer group changed since last segment
     * \param x X coordinate for via placement
     * \param y Y coordinate for via placement
     */
    void createAutoVia(Coord x, Coord y) {
        if (!PCB->ViaOn) return;

        // Check if layer group changed
        if (GetLayerGroupNumberByPointer(CURRENT) ==
            GetLayerGroupNumberByPointer(context_->lastLayer)) {
            return;
        }

        // Don't create via if there's already a pin/via here
        void *ptr1, *ptr2, *ptr3;
        if (SearchObjectByLocation(PIN_TYPES, &ptr1, &ptr2, &ptr3,
                                  x, y, Settings.ViaThickness / 2) != NO_TYPE) {
            return;
        }

        // Determine layer range for buried via
        Cardinal layer_from, layer_to;
        if (TEST_FLAG(AUTOBURIEDVIASFLAG, PCB)) {
            layer_from = GetLayerNumber(PCB->Data, context_->lastLayer);
            layer_to = GetLayerNumber(PCB->Data, CURRENT);
        } else {
            layer_from = 0;
            layer_to = 0;
        }

        // Create the via
        PinType* via = CreateNewViaEx(
            PCB->Data, x, y,
            Settings.ViaThickness, 2 * Settings.Keepaway,
            Settings.ViaMaskAperture, Settings.ViaDrillingHole,
            NULL, NoFlags(), layer_from, layer_to);

        if (via != NULL) {
            AddObjectToCreateUndoList(VIA_TYPE, via, via, via);
            DrawVia(via);
        }
    }
};

// Factory function for ModeManager registration
std::unique_ptr<EditorMode> createLineMode(ActionContext* context) {
    return std::unique_ptr<EditorMode>(new LineMode(context));
}

} // namespace modes
} // namespace pcb
