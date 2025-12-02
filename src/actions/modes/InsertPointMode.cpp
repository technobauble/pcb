/*!
 * \file src/actions/modes/InsertPointMode.cpp
 *
 * \brief Insert point mode implementation
 *
 * Allows the user to insert new points into lines, arcs, or polygons.
 * Click to select an edge, then click to place the new point.
 *
 * State machine:
 * - STATE_FIRST: Click to select object edge
 * - STATE_SECOND: Click to insert point on the edge
 *
 * Uses AttachedObject to track the selected object.
 * Special handling for polygons to find the nearest segment.
 *
 * Extracted from action.c INSERTPOINT_MODE case in NotifyMode().
 */

#include "StatefulMode.h"
#include "ModesCommon.h"

extern "C" {
#include "insert.h"
#include "crosshair.h"
#include "polygon.h"
#include "error.h"
}

namespace pcb {
namespace modes {

/*!
 * \brief Insert point mode
 *
 * Inserts new vertices into lines, arcs, or polygons.
 *
 * Features:
 * - Click near an edge to select it
 * - For polygons, finds nearest segment
 * - Preview shows insertion point
 * - Click to insert the point
 * - Locked objects cannot be modified
 */
class InsertPointMode : public TwoClickMode {
public:
    explicit InsertPointMode(ActionContext* context)
        : TwoClickMode(context)
        , attachment_(Crosshair.AttachedObject)
    {}

    void onNotify(Coord x, Coord y) override {
        switch (getState()) {
        case STATE_FIRST:
            selectEdge();
            break;

        case STATE_SECOND:
            insertPoint();
            break;
        }
    }

    void onRelease() override {
        // Insert point mode doesn't use release
    }

    const char* getName() const override { return "InsertPoint"; }
    int getModeId() const override { return INSERTPOINT_MODE; }

protected:
    IAttachedState& getAttachment() override {
        return attachment_;
    }

private:
    AttachedObjectAdapter attachment_;

    /*!
     * \brief Select an edge to insert point into
     *
     * Searches for an insertable object (line, arc, polygon).
     * For polygons, finds the nearest segment and stores info in context.
     * Respects lock flags.
     */
    void selectEdge() {
        // Search for insertable object
        Crosshair.AttachedObject.Type = SearchScreen(
            context_->Note.X, context_->Note.Y,
            INSERT_TYPES,
            &Crosshair.AttachedObject.Ptr1,
            &Crosshair.AttachedObject.Ptr2,
            &Crosshair.AttachedObject.Ptr3);

        if (Crosshair.AttachedObject.Type == NO_TYPE) {
            return;
        }

        // Check if object is locked
        if (TEST_FLAG(LOCKFLAG, (PolygonType*)Crosshair.AttachedObject.Ptr2)) {
            Message(_("Sorry, the object is locked\n"));
            Crosshair.AttachedObject.Type = NO_TYPE;
            return;
        }

        // Special handling for polygons - find nearest segment
        if (Crosshair.AttachedObject.Type == POLYGON_TYPE) {
            context_->fake.poly = (PolygonType*)Crosshair.AttachedObject.Ptr2;

            // Find the polygon point closest to click location
            context_->polyIndex = GetLowestDistancePolygonPoint(
                context_->fake.poly,
                context_->Note.X, context_->Note.Y);

            // Set up a fake line representing the segment
            context_->fake.line.Point1 =
                context_->fake.poly->Points[context_->polyIndex];
            context_->fake.line.Point2 =
                context_->fake.poly->Points[
                    prev_contour_point(context_->fake.poly, context_->polyIndex)];

            // Point Ptr2 to the fake line for display
            Crosshair.AttachedObject.Ptr2 = &context_->fake.line;
        }

        // Advance state and calculate insertion point
        advanceState();
        context_->InsertedPoint = *AdjustInsertPoint();
    }

    /*!
     * \brief Insert the point into the object
     *
     * Inserts a new vertex at the calculated insertion point.
     * Handles both polygon and line/arc insertion.
     */
    void insertPoint() {
        if (Crosshair.AttachedObject.Type == POLYGON_TYPE) {
            // Insert into polygon - use stored polygon pointer
            InsertPointIntoObject(
                POLYGON_TYPE,
                Crosshair.AttachedObject.Ptr1,
                context_->fake.poly,
                &context_->polyIndex,
                context_->InsertedPoint.X,
                context_->InsertedPoint.Y,
                false, false);
        } else {
            // Insert into line or arc
            InsertPointIntoObject(
                Crosshair.AttachedObject.Type,
                Crosshair.AttachedObject.Ptr1,
                Crosshair.AttachedObject.Ptr2,
                &context_->polyIndex,
                context_->InsertedPoint.X,
                context_->InsertedPoint.Y,
                false, false);
        }

        SetChangedFlag(true);

        // Reset for next insertion
        Crosshair.AttachedObject.Type = NO_TYPE;
        resetToFirst();
    }
};

// Factory function for ModeManager registration
std::unique_ptr<EditorMode> createInsertPointMode(ActionContext* context) {
    return std::unique_ptr<EditorMode>(new InsertPointMode(context));
}

} // namespace modes
} // namespace pcb
