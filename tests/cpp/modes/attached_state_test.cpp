/*!
 * \file tests/cpp/modes/attached_state_test.cpp
 *
 * \brief Unit tests for attached state interfaces and adapters
 *
 * Tests the IAttachedState interface hierarchy and adapter classes:
 * - AttachedLineAdapter
 * - AttachedBoxAdapter
 * - AttachedObjectAdapter
 */

#include <gtest/gtest.h>

// Include the headers under test
extern "C" {
#include "global.h"
#include "crosshair.h"
#include "const.h"
}

#include "attached_state.h"
#include "attached_state_adapters.h"

namespace pcb {
namespace test {

// ============================================================================
// AttachedLineAdapter Tests
// ============================================================================

class AttachedLineAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize a fresh AttachedLineType for each test
        memset(&line_, 0, sizeof(line_));
        line_.State = STATE_FIRST;
    }

    AttachedLineType line_;
};

TEST_F(AttachedLineAdapterTest, InitialState) {
    AttachedLineAdapter adapter(line_);

    EXPECT_EQ(STATE_FIRST, adapter.getState());
    EXPECT_TRUE(adapter.isFirst());
    EXPECT_FALSE(adapter.isSecond());
    EXPECT_FALSE(adapter.isThird());
}

TEST_F(AttachedLineAdapterTest, StateTransitions) {
    AttachedLineAdapter adapter(line_);

    adapter.setState(STATE_SECOND);
    EXPECT_EQ(STATE_SECOND, adapter.getState());
    EXPECT_FALSE(adapter.isFirst());
    EXPECT_TRUE(adapter.isSecond());

    adapter.setState(STATE_THIRD);
    EXPECT_EQ(STATE_THIRD, adapter.getState());
    EXPECT_TRUE(adapter.isThird());
}

TEST_F(AttachedLineAdapterTest, ModifiesUnderlyingStruct) {
    AttachedLineAdapter adapter(line_);

    adapter.setState(STATE_SECOND);
    EXPECT_EQ(STATE_SECOND, line_.State);  // Direct struct access
}

TEST_F(AttachedLineAdapterTest, Reset) {
    AttachedLineAdapter adapter(line_);

    // Set some values
    adapter.setState(STATE_SECOND);
    adapter.setPoint1({100, 200, 0, 0, 0});
    adapter.setPoint2({300, 400, 0, 0, 0});
    adapter.setDraw(true);

    // Reset should clear everything
    adapter.reset();

    EXPECT_EQ(STATE_FIRST, adapter.getState());
    EXPECT_EQ(0, adapter.getPoint1().X);
    EXPECT_EQ(0, adapter.getPoint1().Y);
    EXPECT_EQ(0, adapter.getPoint2().X);
    EXPECT_EQ(0, adapter.getPoint2().Y);
    EXPECT_FALSE(adapter.getDraw());
}

TEST_F(AttachedLineAdapterTest, PointOperations) {
    AttachedLineAdapter adapter(line_);

    PointType p1 = {1000, 2000, 0, 0, 42};
    PointType p2 = {3000, 4000, 0, 0, 43};

    adapter.setPoint1(p1);
    adapter.setPoint2(p2);

    EXPECT_EQ(1000, adapter.getPoint1().X);
    EXPECT_EQ(2000, adapter.getPoint1().Y);
    EXPECT_EQ(42, adapter.getPoint1().ID);

    EXPECT_EQ(3000, adapter.getPoint2().X);
    EXPECT_EQ(4000, adapter.getPoint2().Y);
    EXPECT_EQ(43, adapter.getPoint2().ID);
}

TEST_F(AttachedLineAdapterTest, SetPointFromCoords) {
    AttachedLineAdapter adapter(line_);

    // Use the interface's convenience overload via pointer
    ITwoPointAttachment* iface = &adapter;
    iface->setPoint1(500, 600);
    iface->setPoint2(700, 800);

    EXPECT_EQ(500, adapter.getPoint1().X);
    EXPECT_EQ(600, adapter.getPoint1().Y);
    EXPECT_EQ(700, adapter.getPoint2().X);
    EXPECT_EQ(800, adapter.getPoint2().Y);
}

TEST_F(AttachedLineAdapterTest, SetPoints) {
    AttachedLineAdapter adapter(line_);

    PointType p1 = {100, 200, 0, 0, 0};
    PointType p2 = {300, 400, 0, 0, 0};

    adapter.setPoints(p1, p2);

    EXPECT_EQ(100, adapter.getPoint1().X);
    EXPECT_EQ(300, adapter.getPoint2().X);
}

TEST_F(AttachedLineAdapterTest, DrawFlag) {
    AttachedLineAdapter adapter(line_);

    EXPECT_FALSE(adapter.getDraw());

    adapter.setDraw(true);
    EXPECT_TRUE(adapter.getDraw());
    EXPECT_TRUE(line_.draw);  // Verify underlying struct

    adapter.setDraw(false);
    EXPECT_FALSE(adapter.getDraw());
}

TEST_F(AttachedLineAdapterTest, RawAccess) {
    AttachedLineAdapter adapter(line_);

    adapter.raw().State = STATE_THIRD;
    EXPECT_EQ(STATE_THIRD, adapter.getState());

    const AttachedLineAdapter constAdapter(line_);
    EXPECT_EQ(STATE_THIRD, constAdapter.raw().State);
}

// ============================================================================
// AttachedBoxAdapter Tests
// ============================================================================

class AttachedBoxAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        memset(&box_, 0, sizeof(box_));
        box_.State = STATE_FIRST;
    }

    AttachedBoxType box_;
};

TEST_F(AttachedBoxAdapterTest, InitialState) {
    AttachedBoxAdapter adapter(box_);

    EXPECT_EQ(STATE_FIRST, adapter.getState());
    EXPECT_TRUE(adapter.isFirst());
}

TEST_F(AttachedBoxAdapterTest, StateTransitions) {
    AttachedBoxAdapter adapter(box_);

    adapter.setState(STATE_SECOND);
    EXPECT_TRUE(adapter.isSecond());

    adapter.setState(STATE_THIRD);
    EXPECT_TRUE(adapter.isThird());
}

TEST_F(AttachedBoxAdapterTest, Reset) {
    AttachedBoxAdapter adapter(box_);

    adapter.setState(STATE_SECOND);
    adapter.setPoint1({100, 200, 0, 0, 0});
    adapter.setPoint2({300, 400, 0, 0, 0});
    adapter.setOtherway(true);

    adapter.reset();

    EXPECT_EQ(STATE_FIRST, adapter.getState());
    EXPECT_EQ(0, adapter.getPoint1().X);
    EXPECT_EQ(0, adapter.getPoint2().X);
    EXPECT_FALSE(adapter.getOtherway());
}

TEST_F(AttachedBoxAdapterTest, PointOperations) {
    AttachedBoxAdapter adapter(box_);

    adapter.setPoint1({1000, 2000, 0, 0, 0});
    adapter.setPoint2({3000, 4000, 0, 0, 0});

    EXPECT_EQ(1000, adapter.getPoint1().X);
    EXPECT_EQ(2000, adapter.getPoint1().Y);
    EXPECT_EQ(3000, adapter.getPoint2().X);
    EXPECT_EQ(4000, adapter.getPoint2().Y);
}

TEST_F(AttachedBoxAdapterTest, OtherwayFlag) {
    AttachedBoxAdapter adapter(box_);

    EXPECT_FALSE(adapter.getOtherway());

    adapter.setOtherway(true);
    EXPECT_TRUE(adapter.getOtherway());
    EXPECT_TRUE(box_.otherway);  // Verify underlying struct
}

TEST_F(AttachedBoxAdapterTest, ToggleOtherway) {
    AttachedBoxAdapter adapter(box_);

    EXPECT_FALSE(adapter.getOtherway());

    adapter.toggleOtherway();
    EXPECT_TRUE(adapter.getOtherway());

    adapter.toggleOtherway();
    EXPECT_FALSE(adapter.getOtherway());
}

// ============================================================================
// AttachedObjectAdapter Tests
// ============================================================================

class AttachedObjectAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        memset(&obj_, 0, sizeof(obj_));
        obj_.State = STATE_FIRST;
        obj_.Type = NO_TYPE;
    }

    AttachedObjectType obj_;
};

TEST_F(AttachedObjectAdapterTest, InitialState) {
    AttachedObjectAdapter adapter(obj_);

    EXPECT_EQ(STATE_FIRST, adapter.getState());
    EXPECT_TRUE(adapter.isFirst());
    EXPECT_EQ(NO_TYPE, adapter.getObjectType());
    EXPECT_FALSE(adapter.hasObject());
}

TEST_F(AttachedObjectAdapterTest, StateTransitions) {
    AttachedObjectAdapter adapter(obj_);

    adapter.setState(STATE_SECOND);
    EXPECT_TRUE(adapter.isSecond());
}

TEST_F(AttachedObjectAdapterTest, Reset) {
    AttachedObjectAdapter adapter(obj_);

    int dummy1, dummy2, dummy3;
    adapter.setState(STATE_SECOND);
    adapter.setObjectType(LINE_TYPE);
    adapter.setPointers(&dummy1, &dummy2, &dummy3);
    adapter.setPosition(1000, 2000);

    adapter.reset();

    EXPECT_EQ(STATE_FIRST, adapter.getState());
    EXPECT_EQ(NO_TYPE, adapter.getObjectType());
    EXPECT_EQ(nullptr, adapter.getPtr1());
    EXPECT_EQ(nullptr, adapter.getPtr2());
    EXPECT_EQ(nullptr, adapter.getPtr3());
    EXPECT_EQ(0, adapter.getX());
    EXPECT_EQ(0, adapter.getY());
}

TEST_F(AttachedObjectAdapterTest, ObjectType) {
    AttachedObjectAdapter adapter(obj_);

    EXPECT_FALSE(adapter.hasObject());

    adapter.setObjectType(VIA_TYPE);
    EXPECT_EQ(VIA_TYPE, adapter.getObjectType());
    EXPECT_TRUE(adapter.hasObject());
    EXPECT_EQ(VIA_TYPE, obj_.Type);  // Verify underlying struct
}

TEST_F(AttachedObjectAdapterTest, Pointers) {
    AttachedObjectAdapter adapter(obj_);

    int data1 = 1, data2 = 2, data3 = 3;

    adapter.setPointers(&data1, &data2, &data3);

    EXPECT_EQ(&data1, adapter.getPtr1());
    EXPECT_EQ(&data2, adapter.getPtr2());
    EXPECT_EQ(&data3, adapter.getPtr3());
}

TEST_F(AttachedObjectAdapterTest, Position) {
    AttachedObjectAdapter adapter(obj_);

    adapter.setPosition(5000, 6000);

    EXPECT_EQ(5000, adapter.getX());
    EXPECT_EQ(6000, adapter.getY());
    EXPECT_EQ(5000, obj_.X);  // Verify underlying struct
    EXPECT_EQ(6000, obj_.Y);
}

TEST_F(AttachedObjectAdapterTest, BoundingBox) {
    AttachedObjectAdapter adapter(obj_);

    BoxType box = {100, 200, 300, 400};
    adapter.setBoundingBox(box);

    const BoxType& retrieved = adapter.getBoundingBox();
    EXPECT_EQ(100, retrieved.X1);
    EXPECT_EQ(200, retrieved.Y1);
    EXPECT_EQ(300, retrieved.X2);
    EXPECT_EQ(400, retrieved.Y2);
}

TEST_F(AttachedObjectAdapterTest, RubberbandOperations) {
    AttachedObjectAdapter adapter(obj_);

    // Initial state
    EXPECT_EQ(0u, adapter.getRubberbandCount());
    EXPECT_EQ(nullptr, adapter.getRubberband());

    // Set up rubberband (simulating allocation)
    RubberbandType rb[10];
    adapter.setRubberband(rb, 10);

    EXPECT_EQ(10u, adapter.getRubberbandMax());
    EXPECT_EQ(rb, adapter.getRubberband());

    // Set count
    adapter.setRubberbandCount(5);
    EXPECT_EQ(5u, adapter.getRubberbandCount());

    // Clear count
    adapter.clearRubberband();
    EXPECT_EQ(0u, adapter.getRubberbandCount());
}

// ============================================================================
// Interface Polymorphism Tests
// ============================================================================

TEST(InterfaceTest, ITwoPointAttachmentPolymorphism) {
    AttachedLineType line = {};
    AttachedBoxType box = {};

    AttachedLineAdapter lineAdapter(line);
    AttachedBoxAdapter boxAdapter(box);

    // Both can be used through base interface
    ITwoPointAttachment* attachments[] = {&lineAdapter, &boxAdapter};

    for (ITwoPointAttachment* att : attachments) {
        att->setState(STATE_SECOND);
        att->setPoint1({100, 200, 0, 0, 0});
        att->setPoint2({300, 400, 0, 0, 0});

        EXPECT_EQ(STATE_SECOND, att->getState());
        EXPECT_TRUE(att->isSecond());
        EXPECT_EQ(100, att->getPoint1().X);
        EXPECT_EQ(300, att->getPoint2().X);
    }
}

TEST(InterfaceTest, IAttachedStatePolymorphism) {
    AttachedLineType line = {};
    AttachedBoxType box = {};
    AttachedObjectType obj = {};

    AttachedLineAdapter lineAdapter(line);
    AttachedBoxAdapter boxAdapter(box);
    AttachedObjectAdapter objAdapter(obj);

    // All can be used through base interface
    IAttachedState* states[] = {&lineAdapter, &boxAdapter, &objAdapter};

    for (IAttachedState* state : states) {
        state->setState(STATE_SECOND);
        EXPECT_EQ(STATE_SECOND, state->getState());
        EXPECT_TRUE(state->isSecond());

        state->reset();
        EXPECT_EQ(STATE_FIRST, state->getState());
        EXPECT_TRUE(state->isFirst());
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(EdgeCaseTest, MultipleAdaptersForSameStruct) {
    AttachedLineType line = {};

    AttachedLineAdapter adapter1(line);
    AttachedLineAdapter adapter2(line);

    // Both adapters wrap the same struct
    adapter1.setState(STATE_SECOND);
    EXPECT_EQ(STATE_SECOND, adapter2.getState());

    adapter2.setPoint1({999, 888, 0, 0, 0});
    EXPECT_EQ(999, adapter1.getPoint1().X);
}

} // namespace test
} // namespace pcb
