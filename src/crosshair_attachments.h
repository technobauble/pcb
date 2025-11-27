/*!
 * \file src/crosshair_attachments.h
 *
 * \brief Convenience accessors for global Crosshair attachments
 *
 * Provides a clean C++ interface to access the attached state structures
 * within the global Crosshair variable. This centralizes access and makes
 * it easy to switch from raw struct access to interface-based access.
 *
 * Usage in modes:
 *   auto& line = CrosshairAttachments::line();
 *   if (line.isFirst()) {
 *       line.setPoint1(x, y);
 *       line.setState(STATE_SECOND);
 *   }
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef PCB_CROSSHAIR_ATTACHMENTS_H
#define PCB_CROSSHAIR_ATTACHMENTS_H

#include "attached_state_adapters.h"

#ifdef __cplusplus

extern "C" {
#include "data.h"  /* Crosshair global variable */
}

/*!
 * \brief Provides typed access to Crosshair attachment structures
 *
 * Static accessor class that returns adapters for the global Crosshair's
 * attachment structures. Since there's only one Crosshair, all methods
 * are static.
 *
 * The returned adapters wrap references to the actual Crosshair members,
 * so modifications through the adapter affect the global state.
 */
class CrosshairAttachments {
public:
    /* No instances - all static */
    CrosshairAttachments() = delete;

    /*!
     * \brief Get adapter for line attachment
     * \return AttachedLineAdapter wrapping Crosshair.AttachedLine
     *
     * Used by line drawing modes (LINE_MODE, POLYLINE_MODE, etc.)
     */
    static AttachedLineAdapter line() {
        return AttachedLineAdapter(Crosshair.AttachedLine);
    }

    /*!
     * \brief Get adapter for box attachment
     * \return AttachedBoxAdapter wrapping Crosshair.AttachedBox
     *
     * Used by rectangle and arc drawing modes (RECTANGLE_MODE, ARC_MODE)
     */
    static AttachedBoxAdapter box() {
        return AttachedBoxAdapter(Crosshair.AttachedBox);
    }

    /*!
     * \brief Get adapter for object attachment
     * \return AttachedObjectAdapter wrapping Crosshair.AttachedObject
     *
     * Used by object manipulation modes (COPY_MODE, MOVE_MODE, etc.)
     */
    static AttachedObjectAdapter object() {
        return AttachedObjectAdapter(Crosshair.AttachedObject);
    }

    /*!
     * \brief Get direct access to polygon attachment
     * \return Reference to Crosshair.AttachedPolygon
     *
     * AttachedPolygon is a full PolygonType, not a simple state holder.
     * It will be handled as part of PCB object unification, not wrapped
     * in an adapter here.
     */
    static PolygonType& polygon() {
        return Crosshair.AttachedPolygon;
    }

    /*!
     * \brief Reset all attachments to initial state
     *
     * Convenience method to reset all attachment states at once.
     * Does not reset AttachedPolygon (that requires special handling).
     */
    static void resetAll() {
        line().reset();
        box().reset();
        object().reset();
    }

    /*!
     * \brief Get the current crosshair position
     * \param x Output X coordinate
     * \param y Output Y coordinate
     */
    static void getPosition(Coord& x, Coord& y) {
        x = Crosshair.X;
        y = Crosshair.Y;
    }

    /*!
     * \brief Get crosshair X position
     * \return X coordinate in PCB units
     */
    static Coord getX() { return Crosshair.X; }

    /*!
     * \brief Get crosshair Y position
     * \return Y coordinate in PCB units
     */
    static Coord getY() { return Crosshair.Y; }
};

#endif /* __cplusplus */

#endif /* PCB_CROSSHAIR_ATTACHMENTS_H */
