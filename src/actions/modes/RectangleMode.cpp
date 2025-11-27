/*!
 * \file src/actions/modes/RectangleMode.cpp
 *
 * \brief Rectangle drawing mode implementation
 *
 * Allows the user to draw rectangular polygons by clicking two corners.
 * This is the first StatefulMode implementation using the attached state
 * interfaces.
 *
 * State machine:
 * - STATE_FIRST: Waiting for first corner click
 * - STATE_SECOND: First corner set, waiting for second corner
 * - STATE_THIRD: Both corners set, create rectangle
 *
 * Extracted from action.c NotifyBlock() and RECTANGLE_MODE case.
 */

#include "StatefulMode.h"
#include "ModesCommon.h"

extern "C" {
#include "create.h"
#include "draw.h"
#include "crosshair.h"
#include "polygon.h"
}

namespace pcb {
namespace modes {

/*!
 * \brief Rectangle drawing mode
 *
 * Creates rectangular polygons by clicking two diagonal corners.
 * Uses AttachedBox to track the two corner points.
 *
 * Features:
 * - First click sets one corner
 * - Preview rectangle follows cursor
 * - Second click creates the polygon
 * - Automatically resets for next rectangle
 */
class RectangleMode : public StatefulMode {
public:
    explicit RectangleMode(ActionContext* context)
        : StatefulMode(context)
        , attachment_(Crosshair.AttachedBox)
    {}

    void onNotify(Coord x, Coord y) override {
        // Update state machine based on current state
        notifyBlock();

        // Check if we should create the rectangle
        if (getState() == STATE_THIRD) {
            createRectangle();
        }
    }

    void onRelease() override {
        // Rectangle mode doesn't use release
    }

    const char* getName() const override { return "Rectangle"; }
    int getModeId() const override { return RECTANGLE_MODE; }

protected:
    IAttachedState& getAttachment() override {
        return attachment_;
    }

private:
    AttachedBoxAdapter attachment_;

    /*!
     * \brief Handle state transitions (extracted from NotifyBlock)
     *
     * STATE_FIRST → STATE_SECOND: Set first corner
     * STATE_SECOND → STATE_THIRD: Set second corner
     */
    void notifyBlock() {
        notify_crosshair_change(false);

        switch (getState()) {
        case STATE_FIRST:
            // Setup first corner - both points start at same location
            attachment_.setPoint1({Crosshair.X, Crosshair.Y, 0, 0, 0});
            attachment_.setPoint2({Crosshair.X, Crosshair.Y, 0, 0, 0});
            setState(STATE_SECOND);
            break;

        case STATE_SECOND:
            // Second corner is already tracked by AdjustAttachedBox()
            // Just advance state to trigger creation
            setState(STATE_THIRD);
            break;

        default:
            break;
        }

        notify_crosshair_change(true);
    }

    /*!
     * \brief Create the rectangle polygon
     *
     * Called when STATE_THIRD is reached. Creates a polygon from
     * the two corner points if they define a valid rectangle.
     */
    void createRectangle() {
        PointType p1 = attachment_.getPoint1();
        PointType p2 = attachment_.getPoint2();

        // Only create if width and height are non-zero
        if (p1.X != p2.X && p1.Y != p2.Y) {
            int flags = CLEARPOLYFLAG;
            if (TEST_FLAG(NEWFULLPOLYFLAG, PCB)) {
                flags |= FULLPOLYFLAG;
            }

            PolygonType* polygon = CreateNewPolygonFromRectangle(
                CURRENT,
                p1.X, p1.Y,
                p2.X, p2.Y,
                MakeFlags(flags)
            );

            if (polygon != NULL) {
                AddObjectToCreateUndoList(POLYGON_TYPE, CURRENT, polygon, polygon);
                IncrementUndoSerialNumber();
                DrawPolygon(CURRENT, polygon);
                Draw();
            }
        }

        // Reset state to first corner for next rectangle
        setState(STATE_FIRST);
    }
};

// Factory function for ModeManager registration
std::unique_ptr<EditorMode> createRectangleMode(ActionContext* context) {
    return std::unique_ptr<EditorMode>(new RectangleMode(context));
}

} // namespace modes
} // namespace pcb
