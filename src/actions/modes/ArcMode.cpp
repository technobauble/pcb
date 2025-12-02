/*!
 * \file src/actions/modes/ArcMode.cpp
 *
 * \brief Arc drawing mode implementation
 *
 * Allows the user to draw arcs by clicking a center point and endpoint.
 * Arcs are constrained to 45° or 90° angles depending on compile options.
 *
 * State machine:
 * - STATE_FIRST: Waiting for center point click
 * - STATE_SECOND: Center set, waiting for endpoint
 * - STATE_THIRD: Continuous arc drawing (endpoint becomes next center)
 *
 * Uses AttachedBox for tracking the center (Point1) and endpoint (Point2).
 * The "otherway" flag toggles the arc direction.
 *
 * Extracted from action.c ARC_MODE case in NotifyMode().
 */

#include "StatefulMode.h"
#include "ModesCommon.h"

extern "C" {
#include "create.h"
#include "draw.h"
#include "crosshair.h"
#include "macro.h"  // SGNZ, XOR
}

namespace pcb {
namespace modes {

/*!
 * \brief Arc drawing mode
 *
 * Creates arcs by clicking center and endpoint.
 * Supports continuous drawing where each arc's end becomes
 * the center of the next arc.
 *
 * Features:
 * - Click to set arc center
 * - Rubber-band preview follows cursor
 * - Click to create arc and set up next
 * - Arc direction based on cursor position relative to center
 * - "otherway" flag to flip arc direction
 */
class ArcMode : public StatefulMode {
public:
    explicit ArcMode(ActionContext* context)
        : StatefulMode(context)
        , attachment_(Crosshair.AttachedBox)
    {}

    void onNotify(Coord x, Coord y) override {
        switch (getState()) {
        case STATE_FIRST:
            // Set center point
            setupCenter();
            setState(STATE_SECOND);
            break;

        case STATE_SECOND:
        case STATE_THIRD:
            // Create arc from center to endpoint
            createArc();
            break;
        }
    }

    void onRelease() override {
        // Arc mode doesn't use release
    }

    const char* getName() const override { return "Arc"; }
    int getModeId() const override { return ARC_MODE; }

protected:
    IAttachedState& getAttachment() override {
        return attachment_;
    }

private:
    AttachedBoxAdapter attachment_;

    /*!
     * \brief Set up arc center point
     *
     * Sets both Point1 (center) and Point2 (endpoint) to the
     * click location initially.
     */
    void setupCenter() {
        PointType pt = {context_->Note.X, context_->Note.Y, 0, 0, 0};
        attachment_.setPoint1(pt);
        attachment_.setPoint2(pt);
    }

    /*!
     * \brief Create arc and update for next arc
     *
     * Calculates arc geometry based on:
     * - Center point (Point1)
     * - Click position (Note.X, Note.Y)
     * - otherway flag for direction toggle
     *
     * Arc parameters:
     * - Radius is determined by distance from center to click
     * - Start angle and direction depend on quadrant
     * - Arc can be 45° or 90° depending on compile options
     */
    void createArc() {
        PointType p1 = attachment_.getPoint1();

        // Calculate offset from center
        Coord wx = context_->Note.X - p1.X;
        Coord wy = context_->Note.Y - p1.Y;

        Coord arcCenterX, arcCenterY;
        Angle startAngle, direction;
        Coord radius;

        // Determine arc geometry based on direction of offset
        // The otherway flag flips the comparison
        if (XOR(Crosshair.AttachedBox.otherway, abs(wy) > abs(wx))) {
            // Vertical dominant - horizontal arc
            arcCenterX = p1.X + abs(wy) * SGNZ(wx);
            arcCenterY = p1.Y;
            radius = abs(wy);

            startAngle = (wx >= 0) ? 0 : 180;

#ifdef ARC45
            if (abs(wy) / 2 >= abs(wx)) {
                direction = (SGNZ(wx) == SGNZ(wy)) ? 45 : -45;
            } else
#endif
            {
                direction = (SGNZ(wx) == SGNZ(wy)) ? 90 : -90;
            }
        } else {
            // Horizontal dominant - vertical arc
            arcCenterX = p1.X;
            arcCenterY = p1.Y + abs(wx) * SGNZ(wy);
            radius = abs(wx);

            startAngle = (wy >= 0) ? -90 : 90;

#ifdef ARC45
            if (abs(wx) / 2 >= abs(wy)) {
                direction = (SGNZ(wx) == SGNZ(wy)) ? -45 : 45;
            } else
#endif
            {
                direction = (SGNZ(wx) == SGNZ(wy)) ? -90 : 90;
            }
        }

        // Only create arc if radius is non-zero
        if (radius == 0) {
            return;
        }

        // Determine arc flags
        int flags = 0;
        if (TEST_FLAG(CLEARNEWFLAG, PCB)) {
            flags = CLEARLINEFLAG;
        }

        // Create the arc
        ArcType* arc = CreateNewArcOnLayer(
            CURRENT,
            arcCenterX, arcCenterY,
            radius, radius,  // Width and height (same for circular arc)
            startAngle, direction,
            Settings.LineThickness,
            2 * Settings.Keepaway,
            MakeFlags(flags));

        if (arc != NULL) {
            // Get arc endpoint for next arc's center
            BoxType* box = GetArcEnds(arc);

            // Set Point1 and Point2 to arc endpoint for continuous drawing
            PointType endPt = {box->X2, box->Y2, 0, 0, 0};
            attachment_.setPoint1(endPt);
            attachment_.setPoint2(endPt);

            // Add to undo and draw
            AddObjectToCreateUndoList(ARC_TYPE, CURRENT, arc, arc);
            IncrementUndoSerialNumber();
            context_->addedLines++;
            DrawArc(CURRENT, arc);
            Draw();

            // Stay in STATE_THIRD for continuous arc drawing
            setState(STATE_THIRD);
        }
    }
};

// Factory function for ModeManager registration
std::unique_ptr<EditorMode> createArcMode(ActionContext* context) {
    return std::unique_ptr<EditorMode>(new ArcMode(context));
}

} // namespace modes
} // namespace pcb
