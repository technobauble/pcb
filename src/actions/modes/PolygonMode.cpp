/*!
 * \file src/actions/modes/PolygonMode.cpp
 *
 * \brief Polygon drawing mode implementation
 *
 * Allows the user to draw polygons by clicking multiple points.
 * Clicking on the first point closes the polygon.
 *
 * Uses AttachedPolygon to accumulate points and AttachedLine to track
 * the current segment (reusing LINE_MODE mechanism).
 *
 * State flow:
 * - Click adds points to AttachedPolygon
 * - Clicking on first point (when n >= 3) closes polygon
 * - NotifyLine() handles AttachedLine state
 *
 * Extracted from action.c POLYGON_MODE case in NotifyMode().
 */

#include "EditorMode.h"
#include "ModesCommon.h"

extern "C" {
#include "create.h"
#include "crosshair.h"
#include "polygon.h"
#include "draw.h"
}

// Forward declaration - NotifyLine is in action.c
extern "C" void NotifyLine_Internal(void);

namespace pcb {
namespace modes {

/*!
 * \brief Polygon drawing mode
 *
 * Creates polygon fills by clicking vertices.
 *
 * Features:
 * - Click to add polygon vertices
 * - Rubber-band preview of current edge
 * - Click on first vertex to close polygon (requires 3+ points)
 * - Creates filled polygon on current layer
 */
class PolygonMode : public EditorMode {
public:
    explicit PolygonMode(ActionContext* context)
        : EditorMode(context)
    {}

    void onNotify(Coord x, Coord y) override {
        PointType* points = Crosshair.AttachedPolygon.Points;
        Cardinal n = Crosshair.AttachedPolygon.PointN;

        // Use LINE_MODE mechanism for state handling
        // This updates AttachedLine.Point2 based on crosshair position
        notifyLineInternal();

        // Check if clicking on first point to close polygon
        // Need at least 3 points to form a valid polygon
        if (n >= 3 &&
            points->X == Crosshair.AttachedLine.Point2.X &&
            points->Y == Crosshair.AttachedLine.Point2.Y) {
            // Close the polygon
            CopyAttachedPolygonToLayer();
            Draw();
            return;
        }

        // Add new point if:
        // - First point (!n), or
        // - Different from last point
        if (!n ||
            points[n - 1].X != Crosshair.AttachedLine.Point2.X ||
            points[n - 1].Y != Crosshair.AttachedLine.Point2.Y) {

            // Add point to polygon
            CreateNewPointInPolygon(
                &Crosshair.AttachedPolygon,
                Crosshair.AttachedLine.Point2.X,
                Crosshair.AttachedLine.Point2.Y);

            // Update AttachedLine.Point1 for next segment
            Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X;
            Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y;
        }
    }

    void onRelease() override {
        // Polygon mode doesn't use release
    }

    void onExit() override {
        // Reset polygon state when exiting mode
        Crosshair.AttachedPolygon.PointN = 0;
        Crosshair.AttachedLine.State = STATE_FIRST;
    }

    const char* getName() const override { return "Polygon"; }
    int getModeId() const override { return POLYGON_MODE; }

private:
    /*!
     * \brief Internal NotifyLine handling
     *
     * Replicates the NotifyLine() logic for polygon mode.
     * Updates AttachedLine state machine.
     */
    void notifyLineInternal() {
        // Set local reference if needed
        if (!Marked.status || TEST_FLAG(LOCALREFFLAG, PCB)) {
            SetLocalRef(Crosshair.X, Crosshair.Y, true);
        }

        switch (Crosshair.AttachedLine.State) {
        case STATE_FIRST:
            // First point - set both Point1 and Point2
            Crosshair.AttachedLine.Point1.X =
                Crosshair.AttachedLine.Point2.X = Crosshair.X;
            Crosshair.AttachedLine.Point1.Y =
                Crosshair.AttachedLine.Point2.Y = Crosshair.Y;
            Crosshair.AttachedLine.State = STATE_SECOND;
            break;

        case STATE_SECOND:
            // Continue drawing - state stays SECOND for polygon
            // Point2 is updated by crosshair tracking
            context_->lastLayer = CURRENT;
            // For polygon, we stay in STATE_SECOND to keep adding points
            // (unlike line which goes to STATE_THIRD)
            break;

        default:
            break;
        }
    }
};

// Factory function for ModeManager registration
std::unique_ptr<EditorMode> createPolygonMode(ActionContext* context) {
    return std::unique_ptr<EditorMode>(new PolygonMode(context));
}

} // namespace modes
} // namespace pcb
