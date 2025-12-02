/*!
 * \file src/actions/modes/PolygonHoleMode.cpp
 *
 * \brief Polygon hole drawing mode implementation
 *
 * Allows the user to cut holes in existing polygons.
 * First click selects a polygon, then subsequent clicks define the hole.
 *
 * State machine:
 * - STATE_FIRST: Click on polygon to select it
 * - STATE_SECOND: Draw hole vertices, close to subtract
 *
 * Uses AttachedObject to track selected polygon and AttachedPolygon
 * to accumulate hole vertices.
 *
 * Extracted from action.c POLYGONHOLE_MODE case in NotifyMode().
 */

#include "StatefulMode.h"
#include "ModesCommon.h"

extern "C" {
#include "create.h"
#include "crosshair.h"
#include "polygon.h"
#include "draw.h"
#include "remove.h"
#include "error.h"
#include "polyarea.h"
}

namespace pcb {
namespace modes {

/*!
 * \brief Polygon hole mode
 *
 * Creates holes in existing polygons using Boolean subtraction.
 *
 * Features:
 * - Click on polygon to select it for modification
 * - Click to add hole vertices
 * - Click on first vertex to close hole (requires 3+ points)
 * - Uses polygon Boolean operations to subtract hole
 */
class PolygonHoleMode : public TwoClickMode {
public:
    explicit PolygonHoleMode(ActionContext* context)
        : TwoClickMode(context)
        , attachment_(Crosshair.AttachedObject)
    {}

    void onNotify(Coord x, Coord y) override {
        switch (getState()) {
        case STATE_FIRST:
            selectPolygon();
            if (getState() == STATE_SECOND) {
                // Fall through to add first point
                addHolePoint();
            }
            break;

        case STATE_SECOND:
            addHolePoint();
            break;
        }
    }

    void onRelease() override {
        // Polygon hole mode doesn't use release
    }

    void onExit() override {
        // Reset state when exiting mode
        memset(&Crosshair.AttachedPolygon, 0, sizeof(PolygonType));
        Crosshair.AttachedObject.Type = NO_TYPE;
        Crosshair.AttachedObject.State = STATE_FIRST;
        context_->addedLines = 0;
    }

    const char* getName() const override { return "PolygonHole"; }
    int getModeId() const override { return POLYGONHOLE_MODE; }

protected:
    IAttachedState& getAttachment() override {
        return attachment_;
    }

private:
    AttachedObjectAdapter attachment_;

    /*!
     * \brief Select polygon to add hole to
     *
     * Searches for a polygon at the click location.
     * First click must be on a polygon.
     */
    void selectPolygon() {
        Crosshair.AttachedObject.Type = SearchScreen(
            context_->Note.X, context_->Note.Y,
            POLYGON_TYPE,
            &Crosshair.AttachedObject.Ptr1,
            &Crosshair.AttachedObject.Ptr2,
            &Crosshair.AttachedObject.Ptr3);

        if (Crosshair.AttachedObject.Type == NO_TYPE) {
            Message(_("The first point of a polygon hole must be on a polygon.\n"));
            return;
        }

        if (TEST_FLAG(LOCKFLAG, (PolygonType*)Crosshair.AttachedObject.Ptr2)) {
            Message(_("Sorry, the object is locked\n"));
            Crosshair.AttachedObject.Type = NO_TYPE;
            return;
        }

        // Advance to drawing state
        advanceState();
    }

    /*!
     * \brief Add a point to the hole or close it
     *
     * Uses NotifyLine mechanism for state tracking.
     * If clicking on first point with 3+ points, closes hole
     * and performs Boolean subtraction.
     */
    void addHolePoint() {
        PointType* points = Crosshair.AttachedPolygon.Points;
        Cardinal n = Crosshair.AttachedPolygon.PointN;

        // Use LINE_MODE mechanism for state handling
        notifyLineInternal();

        // Check if closing the hole
        if (n >= 3 &&
            points->X == Crosshair.AttachedLine.Point2.X &&
            points->Y == Crosshair.AttachedLine.Point2.Y) {
            subtractHoleFromPolygon();
            return;
        }

        // Add new point if first or different from last
        if (!n ||
            points[n - 1].X != Crosshair.AttachedLine.Point2.X ||
            points[n - 1].Y != Crosshair.AttachedLine.Point2.Y) {

            CreateNewPointInPolygon(
                &Crosshair.AttachedPolygon,
                Crosshair.AttachedLine.Point2.X,
                Crosshair.AttachedLine.Point2.Y);

            // Update AttachedLine for next segment
            Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X;
            Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y;
        }
    }

    /*!
     * \brief Subtract hole from selected polygon
     *
     * Uses polygon Boolean operations to cut the hole.
     * Creates new polygon(s) and removes original.
     */
    void subtractHoleFromPolygon() {
        POLYAREA *original, *new_hole, *result;

        // Create POLYAREAs from original polygon and hole
        original = PolygonToPoly((PolygonType*)Crosshair.AttachedObject.Ptr2);
        new_hole = PolygonToPoly(&Crosshair.AttachedPolygon);

        // Subtract hole from original
        poly_Boolean_free(original, new_hole, &result, PBO_SUB);

        // Convert result to polygon(s) on layer
        SaveUndoSerialNumber();
        FlagType flags = ((PolygonType*)Crosshair.AttachedObject.Ptr2)->Flags;
        PolyToPolygonsOnLayer(
            PCB->Data,
            (LayerType*)Crosshair.AttachedObject.Ptr1,
            result, flags);

        // Remove original polygon
        RemoveObject(
            POLYGON_TYPE,
            Crosshair.AttachedObject.Ptr1,
            Crosshair.AttachedObject.Ptr2,
            Crosshair.AttachedObject.Ptr3);

        RestoreUndoSerialNumber();
        IncrementUndoSerialNumber();
        Draw();

        // Reset state for next hole
        memset(&Crosshair.AttachedPolygon, 0, sizeof(PolygonType));
        context_->addedLines = 0;
        resetToFirst();
    }

    /*!
     * \brief Internal NotifyLine handling for hole drawing
     */
    void notifyLineInternal() {
        if (!Marked.status || TEST_FLAG(LOCALREFFLAG, PCB)) {
            SetLocalRef(Crosshair.X, Crosshair.Y, true);
        }

        switch (Crosshair.AttachedLine.State) {
        case STATE_FIRST:
            Crosshair.AttachedLine.Point1.X =
                Crosshair.AttachedLine.Point2.X = Crosshair.X;
            Crosshair.AttachedLine.Point1.Y =
                Crosshair.AttachedLine.Point2.Y = Crosshair.Y;
            Crosshair.AttachedLine.State = STATE_SECOND;
            break;

        case STATE_SECOND:
            context_->lastLayer = CURRENT;
            break;

        default:
            break;
        }
    }
};

// Factory function for ModeManager registration
std::unique_ptr<EditorMode> createPolygonHoleMode(ActionContext* context) {
    return std::unique_ptr<EditorMode>(new PolygonHoleMode(context));
}

} // namespace modes
} // namespace pcb
