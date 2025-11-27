/*!
 * \file src/attached_state.h
 *
 * \brief Unified interfaces for attached state structures
 *
 * This header defines the interface hierarchy for PCB's attached state
 * structures used during interactive editing. These interfaces provide
 * a clean abstraction over the legacy C structures (AttachedLineType,
 * AttachedBoxType, AttachedObjectType) enabling:
 *
 * - Clean mode implementations that work with interfaces, not raw structs
 * - Future-proof design for data structure modernization
 * - Testability through mockable interfaces
 * - Type-safe access preventing mixing up different attachment types
 *
 * Interface Hierarchy:
 *   IAttachedState (base)
 *   ├── ITwoPointAttachment - for line/box drawing with Point1/Point2
 *   └── IObjectAttachment   - for object manipulation (copy/move/etc.)
 *
 * Note: AttachedPolygon uses PolygonType (a full PCB object) and is
 * handled separately as part of PCB object unification.
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

#ifndef PCB_ATTACHED_STATE_H
#define PCB_ATTACHED_STATE_H

#ifdef __cplusplus

extern "C" {
#include "global.h"
#include "crosshair.h"  /* STATE_FIRST, STATE_SECOND, STATE_THIRD */
#include "const.h"      /* NO_TYPE */
}

/*!
 * \brief Base interface for all attached state types
 *
 * Provides common state machine functionality shared by all attachment
 * types. The state values (STATE_FIRST, STATE_SECOND, STATE_THIRD)
 * track the current phase of a multi-click editing operation.
 */
class IAttachedState {
public:
    virtual ~IAttachedState() = default;

    /*!
     * \brief Get the current state of the attachment
     * \return Current state (STATE_FIRST, STATE_SECOND, or STATE_THIRD)
     */
    virtual int getState() const = 0;

    /*!
     * \brief Set the current state of the attachment
     * \param state New state value
     */
    virtual void setState(int state) = 0;

    /*!
     * \brief Reset the attachment to initial state
     *
     * Resets state to STATE_FIRST and clears all associated data.
     * Implementation varies by attachment type.
     */
    virtual void reset() = 0;

    /*!
     * \brief Check if attachment is in initial state
     * \return true if state is STATE_FIRST
     */
    bool isFirst() const { return getState() == STATE_FIRST; }

    /*!
     * \brief Check if attachment is in second state
     * \return true if state is STATE_SECOND
     */
    bool isSecond() const { return getState() == STATE_SECOND; }

    /*!
     * \brief Check if attachment is in third state
     * \return true if state is STATE_THIRD
     */
    bool isThird() const { return getState() == STATE_THIRD; }
};

/*!
 * \brief Interface for two-point attachment operations
 *
 * Used by line and box drawing modes that track two points during
 * creation. Both AttachedLineType and AttachedBoxType have Point1/Point2
 * fields that this interface abstracts.
 */
class ITwoPointAttachment : public IAttachedState {
public:
    /*!
     * \brief Get the first point
     * \return PointType containing first point coordinates
     */
    virtual PointType getPoint1() const = 0;

    /*!
     * \brief Set the first point
     * \param p New point coordinates
     */
    virtual void setPoint1(PointType p) = 0;

    /*!
     * \brief Get the second point
     * \return PointType containing second point coordinates
     */
    virtual PointType getPoint2() const = 0;

    /*!
     * \brief Set the second point
     * \param p New point coordinates
     */
    virtual void setPoint2(PointType p) = 0;

    /*!
     * \brief Set both points at once
     * \param p1 First point coordinates
     * \param p2 Second point coordinates
     */
    void setPoints(PointType p1, PointType p2) {
        setPoint1(p1);
        setPoint2(p2);
    }

    /*!
     * \brief Set first point from X,Y coordinates
     * \param x X coordinate
     * \param y Y coordinate
     */
    void setPoint1(Coord x, Coord y) {
        PointType p = {x, y, 0, 0, 0};
        setPoint1(p);
    }

    /*!
     * \brief Set second point from X,Y coordinates
     * \param x X coordinate
     * \param y Y coordinate
     */
    void setPoint2(Coord x, Coord y) {
        PointType p = {x, y, 0, 0, 0};
        setPoint2(p);
    }
};

/*!
 * \brief Interface for object attachment operations
 *
 * Used by modes that manipulate existing PCB objects (copy, move, etc.).
 * Wraps AttachedObjectType which holds references to the selected object
 * and its rubberband connections.
 */
class IObjectAttachment : public IAttachedState {
public:
    /*!
     * \brief Get the type of attached object
     * \return Object type flags (LINE_TYPE, VIA_TYPE, etc.)
     */
    virtual int getObjectType() const = 0;

    /*!
     * \brief Set the type of attached object
     * \param type Object type flags
     */
    virtual void setObjectType(int type) = 0;

    /*!
     * \brief Get first object pointer (layer/element container)
     * \return Pointer to containing structure
     */
    virtual void* getPtr1() const = 0;

    /*!
     * \brief Get second object pointer (object itself)
     * \return Pointer to the object
     */
    virtual void* getPtr2() const = 0;

    /*!
     * \brief Get third object pointer (specific point/name)
     * \return Pointer to specific sub-element
     */
    virtual void* getPtr3() const = 0;

    /*!
     * \brief Set all three object pointers
     * \param p1 First pointer (container)
     * \param p2 Second pointer (object)
     * \param p3 Third pointer (sub-element)
     */
    virtual void setPointers(void* p1, void* p2, void* p3) = 0;

    /*!
     * \brief Get saved X position (move origin)
     * \return X coordinate
     */
    virtual Coord getX() const = 0;

    /*!
     * \brief Get saved Y position (move origin)
     * \return Y coordinate
     */
    virtual Coord getY() const = 0;

    /*!
     * \brief Set saved position (move origin)
     * \param x X coordinate
     * \param y Y coordinate
     */
    virtual void setPosition(Coord x, Coord y) = 0;

    /*!
     * \brief Get the bounding box of attached object
     * \return Reference to bounding box
     */
    virtual const BoxType& getBoundingBox() const = 0;

    /*!
     * \brief Set the bounding box of attached object
     * \param box New bounding box
     */
    virtual void setBoundingBox(const BoxType& box) = 0;

    /*!
     * \brief Get number of rubberband connections
     * \return Count of rubberband lines
     */
    virtual Cardinal getRubberbandCount() const = 0;

    /*!
     * \brief Get rubberband array
     * \return Pointer to rubberband array
     */
    virtual RubberbandType* getRubberband() = 0;

    /*!
     * \brief Check if an object is attached
     * \return true if object type is not NO_TYPE
     */
    bool hasObject() const { return getObjectType() != NO_TYPE; }
};

#endif /* __cplusplus */

#endif /* PCB_ATTACHED_STATE_H */
