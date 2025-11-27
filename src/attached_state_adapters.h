/*!
 * \file src/attached_state_adapters.h
 *
 * \brief Adapter classes wrapping existing attached state C structs
 *
 * These adapters implement the IAttachedState interface hierarchy,
 * wrapping the existing C structures without modifying them. This
 * allows incremental migration: new code uses the interfaces while
 * existing code continues to work with raw structs.
 *
 * Adapters provided:
 *   - AttachedLineAdapter  - wraps AttachedLineType
 *   - AttachedBoxAdapter   - wraps AttachedBoxType
 *   - AttachedObjectAdapter - wraps AttachedObjectType
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

#ifndef PCB_ATTACHED_STATE_ADAPTERS_H
#define PCB_ATTACHED_STATE_ADAPTERS_H

#include "attached_state.h"

#ifdef __cplusplus

/*!
 * \brief Adapter for AttachedLineType
 *
 * Wraps the AttachedLineType struct used during line drawing operations.
 * Provides ITwoPointAttachment interface plus line-specific 'draw' flag.
 */
class AttachedLineAdapter : public ITwoPointAttachment {
    AttachedLineType& line_;

public:
    /*!
     * \brief Construct adapter wrapping a line attachment
     * \param line Reference to AttachedLineType struct
     */
    explicit AttachedLineAdapter(AttachedLineType& line) : line_(line) {}

    /* IAttachedState interface */
    int getState() const override { return static_cast<int>(line_.State); }
    void setState(int s) override { line_.State = s; }

    void reset() override {
        line_.State = STATE_FIRST;
        line_.Point1.X = line_.Point1.Y = 0;
        line_.Point1.X2 = line_.Point1.Y2 = 0;
        line_.Point1.ID = 0;
        line_.Point2.X = line_.Point2.Y = 0;
        line_.Point2.X2 = line_.Point2.Y2 = 0;
        line_.Point2.ID = 0;
        line_.draw = false;
    }

    /* ITwoPointAttachment interface */
    PointType getPoint1() const override { return line_.Point1; }
    void setPoint1(PointType p) override { line_.Point1 = p; }
    PointType getPoint2() const override { return line_.Point2; }
    void setPoint2(PointType p) override { line_.Point2 = p; }

    /* Line-specific accessors */

    /*!
     * \brief Get whether line preview should be drawn
     * \return true if preview should be drawn
     */
    bool getDraw() const { return line_.draw; }

    /*!
     * \brief Set whether line preview should be drawn
     * \param d true to draw preview
     */
    void setDraw(bool d) { line_.draw = d; }

    /*!
     * \brief Get direct access to underlying struct
     * \return Reference to wrapped AttachedLineType
     *
     * Use sparingly - prefer interface methods. Provided for
     * compatibility with legacy code during migration.
     */
    AttachedLineType& raw() { return line_; }
    const AttachedLineType& raw() const { return line_; }
};

/*!
 * \brief Adapter for AttachedBoxType
 *
 * Wraps the AttachedBoxType struct used during rectangle and arc
 * drawing operations. Provides ITwoPointAttachment interface plus
 * box-specific 'otherway' flag for arc direction.
 */
class AttachedBoxAdapter : public ITwoPointAttachment {
    AttachedBoxType& box_;

public:
    /*!
     * \brief Construct adapter wrapping a box attachment
     * \param box Reference to AttachedBoxType struct
     */
    explicit AttachedBoxAdapter(AttachedBoxType& box) : box_(box) {}

    /* IAttachedState interface */
    int getState() const override { return static_cast<int>(box_.State); }
    void setState(int s) override { box_.State = s; }

    void reset() override {
        box_.State = STATE_FIRST;
        box_.Point1.X = box_.Point1.Y = 0;
        box_.Point1.X2 = box_.Point1.Y2 = 0;
        box_.Point1.ID = 0;
        box_.Point2.X = box_.Point2.Y = 0;
        box_.Point2.X2 = box_.Point2.Y2 = 0;
        box_.Point2.ID = 0;
        box_.otherway = false;
    }

    /* ITwoPointAttachment interface */
    PointType getPoint1() const override { return box_.Point1; }
    void setPoint1(PointType p) override { box_.Point1 = p; }
    PointType getPoint2() const override { return box_.Point2; }
    void setPoint2(PointType p) override { box_.Point2 = p; }

    /* Box-specific accessors */

    /*!
     * \brief Get arc direction flag
     * \return true if arc should go the "other way"
     */
    bool getOtherway() const { return box_.otherway; }

    /*!
     * \brief Set arc direction flag
     * \param o true for alternate arc direction
     */
    void setOtherway(bool o) { box_.otherway = o; }

    /*!
     * \brief Toggle arc direction
     */
    void toggleOtherway() { box_.otherway = !box_.otherway; }

    /*!
     * \brief Get direct access to underlying struct
     * \return Reference to wrapped AttachedBoxType
     */
    AttachedBoxType& raw() { return box_; }
    const AttachedBoxType& raw() const { return box_; }
};

/*!
 * \brief Adapter for AttachedObjectType
 *
 * Wraps the AttachedObjectType struct used during object manipulation
 * operations (copy, move, etc.). Provides IObjectAttachment interface
 * for accessing object references, position, and rubberband data.
 */
class AttachedObjectAdapter : public IObjectAttachment {
    AttachedObjectType& obj_;

public:
    /*!
     * \brief Construct adapter wrapping an object attachment
     * \param obj Reference to AttachedObjectType struct
     */
    explicit AttachedObjectAdapter(AttachedObjectType& obj) : obj_(obj) {}

    /* IAttachedState interface */
    int getState() const override { return static_cast<int>(obj_.State); }
    void setState(int s) override { obj_.State = s; }

    void reset() override {
        obj_.State = STATE_FIRST;
        obj_.Type = NO_TYPE;
        obj_.Ptr1 = obj_.Ptr2 = obj_.Ptr3 = nullptr;
        obj_.X = obj_.Y = 0;
        /* Note: Rubberband array is not freed here - that's handled
         * by the mode or calling code that allocated it */
    }

    /* IObjectAttachment interface */
    int getObjectType() const override { return static_cast<int>(obj_.Type); }
    void setObjectType(int t) override { obj_.Type = t; }

    void* getPtr1() const override { return obj_.Ptr1; }
    void* getPtr2() const override { return obj_.Ptr2; }
    void* getPtr3() const override { return obj_.Ptr3; }

    void setPointers(void* p1, void* p2, void* p3) override {
        obj_.Ptr1 = p1;
        obj_.Ptr2 = p2;
        obj_.Ptr3 = p3;
    }

    Coord getX() const override { return obj_.X; }
    Coord getY() const override { return obj_.Y; }
    void setPosition(Coord x, Coord y) override {
        obj_.X = x;
        obj_.Y = y;
    }

    const BoxType& getBoundingBox() const override { return obj_.BoundingBox; }
    void setBoundingBox(const BoxType& box) override { obj_.BoundingBox = box; }

    Cardinal getRubberbandCount() const override { return obj_.RubberbandN; }
    RubberbandType* getRubberband() override { return obj_.Rubberband; }

    /* Object-specific accessors */

    /*!
     * \brief Get rubberband array capacity
     * \return Maximum number of rubberband entries allocated
     */
    Cardinal getRubberbandMax() const { return obj_.RubberbandMax; }

    /*!
     * \brief Set rubberband count
     * \param n New count
     */
    void setRubberbandCount(Cardinal n) { obj_.RubberbandN = n; }

    /*!
     * \brief Set rubberband array and capacity
     * \param rb Pointer to rubberband array
     * \param max Maximum entries in array
     */
    void setRubberband(RubberbandType* rb, Cardinal max) {
        obj_.Rubberband = rb;
        obj_.RubberbandMax = max;
    }

    /*!
     * \brief Clear rubberband tracking (count only, not memory)
     */
    void clearRubberband() { obj_.RubberbandN = 0; }

    /*!
     * \brief Get direct access to underlying struct
     * \return Reference to wrapped AttachedObjectType
     */
    AttachedObjectType& raw() { return obj_; }
    const AttachedObjectType& raw() const { return obj_; }
};

#endif /* __cplusplus */

#endif /* PCB_ATTACHED_STATE_ADAPTERS_H */
