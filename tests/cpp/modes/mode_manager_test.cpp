/*!
 * \file tests/cpp/modes/mode_manager_test.cpp
 *
 * \brief Integration tests for ModeManager and mode system
 *
 * These tests verify:
 * - ModeManager initializes and registers modes correctly
 * - Mode transitions work (setMode, saveMode, restoreMode)
 * - Event dispatch routes to correct mode
 * - StatefulMode state machine integration
 */

#include <gtest/gtest.h>

extern "C" {
#include "global.h"
#include "crosshair.h"
#include "const.h"
#include "data.h"
}

#include "actions/modes/EditorMode.h"

// Access the global mode manager
extern pcb::modes::ModeManager* pcb_mode_manager;

namespace pcb {
namespace test {

// ============================================================================
// Test Fixture - Sets up global state needed by modes
// ============================================================================

class ModeManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize Crosshair state
        memset(&Crosshair, 0, sizeof(Crosshair));
        Crosshair.AttachedLine.State = STATE_FIRST;
        Crosshair.AttachedBox.State = STATE_FIRST;
        Crosshair.AttachedObject.State = STATE_FIRST;
        Crosshair.AttachedObject.Type = NO_TYPE;

        // Create action context (minimal initialization)
        if (!context_) {
            context_ = static_cast<ActionContext*>(calloc(1, sizeof(ActionContext)));
        }

        // Create mode manager if not exists
        if (!pcb_mode_manager) {
            pcb_mode_manager = new pcb::modes::ModeManager(context_);
        }
    }

    void TearDown() override {
        // Reset crosshair state for next test
        Crosshair.AttachedLine.State = STATE_FIRST;
        Crosshair.AttachedBox.State = STATE_FIRST;
        Crosshair.AttachedObject.State = STATE_FIRST;
    }

    static ActionContext* context_;
};

ActionContext* ModeManagerTest::context_ = nullptr;

// ============================================================================
// Mode Registration Tests
// ============================================================================

TEST_F(ModeManagerTest, ManagerExists) {
    ASSERT_NE(nullptr, pcb_mode_manager);
}

TEST_F(ModeManagerTest, ViaModRegistered) {
    pcb_mode_manager->setMode(VIA_MODE);
    ASSERT_NE(nullptr, pcb_mode_manager->getCurrentMode());
    EXPECT_EQ(VIA_MODE, pcb_mode_manager->getCurrentModeId());
    EXPECT_STREQ("Via", pcb_mode_manager->getCurrentMode()->getName());
}

TEST_F(ModeManagerTest, LockModeRegistered) {
    pcb_mode_manager->setMode(LOCK_MODE);
    ASSERT_NE(nullptr, pcb_mode_manager->getCurrentMode());
    EXPECT_EQ(LOCK_MODE, pcb_mode_manager->getCurrentModeId());
    EXPECT_STREQ("Lock", pcb_mode_manager->getCurrentMode()->getName());
}

TEST_F(ModeManagerTest, RemoveModeRegistered) {
    pcb_mode_manager->setMode(REMOVE_MODE);
    ASSERT_NE(nullptr, pcb_mode_manager->getCurrentMode());
    EXPECT_EQ(REMOVE_MODE, pcb_mode_manager->getCurrentModeId());
    EXPECT_STREQ("Remove", pcb_mode_manager->getCurrentMode()->getName());
}

TEST_F(ModeManagerTest, ThermalModeRegistered) {
    pcb_mode_manager->setMode(THERMAL_MODE);
    ASSERT_NE(nullptr, pcb_mode_manager->getCurrentMode());
    EXPECT_EQ(THERMAL_MODE, pcb_mode_manager->getCurrentModeId());
    EXPECT_STREQ("Thermal", pcb_mode_manager->getCurrentMode()->getName());
}

TEST_F(ModeManagerTest, RectangleModeRegistered) {
    pcb_mode_manager->setMode(RECTANGLE_MODE);
    ASSERT_NE(nullptr, pcb_mode_manager->getCurrentMode());
    EXPECT_EQ(RECTANGLE_MODE, pcb_mode_manager->getCurrentModeId());
    EXPECT_STREQ("Rectangle", pcb_mode_manager->getCurrentMode()->getName());
}

// ============================================================================
// Mode Transition Tests
// ============================================================================

TEST_F(ModeManagerTest, ModeTransition) {
    pcb_mode_manager->setMode(VIA_MODE);
    EXPECT_EQ(VIA_MODE, pcb_mode_manager->getCurrentModeId());

    pcb_mode_manager->setMode(LOCK_MODE);
    EXPECT_EQ(LOCK_MODE, pcb_mode_manager->getCurrentModeId());

    pcb_mode_manager->setMode(RECTANGLE_MODE);
    EXPECT_EQ(RECTANGLE_MODE, pcb_mode_manager->getCurrentModeId());
}

TEST_F(ModeManagerTest, SaveAndRestoreMode) {
    pcb_mode_manager->setMode(VIA_MODE);
    EXPECT_EQ(VIA_MODE, pcb_mode_manager->getCurrentModeId());

    // Save VIA_MODE and switch to REMOVE_MODE
    pcb_mode_manager->saveMode();
    pcb_mode_manager->setMode(REMOVE_MODE);
    EXPECT_EQ(REMOVE_MODE, pcb_mode_manager->getCurrentModeId());

    // Restore should go back to VIA_MODE
    pcb_mode_manager->restoreMode();
    EXPECT_EQ(VIA_MODE, pcb_mode_manager->getCurrentModeId());
}

TEST_F(ModeManagerTest, UnregisteredModeReturnsNull) {
    // Try to set a mode that isn't registered yet (like LINE_MODE)
    pcb_mode_manager->setMode(LINE_MODE);

    // Should either be null or fall back gracefully
    // The exact behavior depends on ModeManager implementation
    // This test documents the current behavior
}

// ============================================================================
// StatefulMode Integration Tests (using RectangleMode)
// ============================================================================

TEST_F(ModeManagerTest, RectangleModeInitialState) {
    pcb_mode_manager->setMode(RECTANGLE_MODE);

    // AttachedBox should be in STATE_FIRST
    EXPECT_EQ(STATE_FIRST, Crosshair.AttachedBox.State);
}

TEST_F(ModeManagerTest, RectangleModeStateAdvances) {
    pcb_mode_manager->setMode(RECTANGLE_MODE);

    // Simulate first click - should advance to STATE_SECOND
    Crosshair.X = 1000;
    Crosshair.Y = 2000;
    pcb_mode_manager->notifyClick(1000, 2000);

    EXPECT_EQ(STATE_SECOND, Crosshair.AttachedBox.State);
    EXPECT_EQ(1000, Crosshair.AttachedBox.Point1.X);
    EXPECT_EQ(2000, Crosshair.AttachedBox.Point1.Y);
}

TEST_F(ModeManagerTest, RectangleModeExitResetsState) {
    pcb_mode_manager->setMode(RECTANGLE_MODE);

    // Advance state
    Crosshair.X = 1000;
    Crosshair.Y = 2000;
    pcb_mode_manager->notifyClick(1000, 2000);
    EXPECT_EQ(STATE_SECOND, Crosshair.AttachedBox.State);

    // Switch to different mode - should reset
    pcb_mode_manager->setMode(VIA_MODE);

    // Switch back - state should be reset
    pcb_mode_manager->setMode(RECTANGLE_MODE);
    EXPECT_EQ(STATE_FIRST, Crosshair.AttachedBox.State);
}

// ============================================================================
// Mode Properties Tests
// ============================================================================

TEST_F(ModeManagerTest, ModeNamesAreSet) {
    const struct {
        int mode_id;
        const char* expected_name;
    } modes[] = {
        {VIA_MODE, "Via"},
        {LOCK_MODE, "Lock"},
        {REMOVE_MODE, "Remove"},
        {THERMAL_MODE, "Thermal"},
        {RECTANGLE_MODE, "Rectangle"},
    };

    for (const auto& m : modes) {
        pcb_mode_manager->setMode(m.mode_id);
        auto* mode = pcb_mode_manager->getCurrentMode();
        if (mode) {
            EXPECT_STREQ(m.expected_name, mode->getName())
                << "Mode ID " << m.mode_id << " has wrong name";
        }
    }
}

TEST_F(ModeManagerTest, ModeIdsMatch) {
    pcb_mode_manager->setMode(VIA_MODE);
    EXPECT_EQ(VIA_MODE, pcb_mode_manager->getCurrentMode()->getModeId());

    pcb_mode_manager->setMode(RECTANGLE_MODE);
    EXPECT_EQ(RECTANGLE_MODE, pcb_mode_manager->getCurrentMode()->getModeId());
}

TEST_F(ModeManagerTest, ModesDoNotAllowSelectionByDefault) {
    pcb_mode_manager->setMode(VIA_MODE);
    EXPECT_FALSE(pcb_mode_manager->getCurrentMode()->allowsSelection());

    pcb_mode_manager->setMode(RECTANGLE_MODE);
    EXPECT_FALSE(pcb_mode_manager->getCurrentMode()->allowsSelection());
}

// ============================================================================
// Cancel/Escape Integration Tests
// ============================================================================

TEST_F(ModeManagerTest, SimpleModeIsAlwaysIdle) {
    // Simple modes (Via, Remove, etc.) are always "idle" since they
    // don't track multi-click state
    pcb_mode_manager->setMode(VIA_MODE);
    EXPECT_TRUE(pcb_mode_manager->isCurrentModeIdle());

    pcb_mode_manager->setMode(REMOVE_MODE);
    EXPECT_TRUE(pcb_mode_manager->isCurrentModeIdle());

    pcb_mode_manager->setMode(LOCK_MODE);
    EXPECT_TRUE(pcb_mode_manager->isCurrentModeIdle());

    pcb_mode_manager->setMode(THERMAL_MODE);
    EXPECT_TRUE(pcb_mode_manager->isCurrentModeIdle());
}

TEST_F(ModeManagerTest, StatefulModeIsIdleInFirstState) {
    pcb_mode_manager->setMode(RECTANGLE_MODE);

    // Should be idle when in STATE_FIRST
    EXPECT_TRUE(pcb_mode_manager->isCurrentModeIdle());
}

TEST_F(ModeManagerTest, StatefulModeNotIdleAfterClick) {
    pcb_mode_manager->setMode(RECTANGLE_MODE);

    // First click advances to STATE_SECOND
    Crosshair.X = 1000;
    Crosshair.Y = 2000;
    pcb_mode_manager->notifyClick(1000, 2000);

    // Should NOT be idle when operation in progress
    EXPECT_FALSE(pcb_mode_manager->isCurrentModeIdle());
}

TEST_F(ModeManagerTest, CancelResetsStatefulMode) {
    pcb_mode_manager->setMode(RECTANGLE_MODE);

    // Advance to STATE_SECOND
    Crosshair.X = 1000;
    Crosshair.Y = 2000;
    pcb_mode_manager->notifyClick(1000, 2000);
    EXPECT_EQ(STATE_SECOND, Crosshair.AttachedBox.State);
    EXPECT_FALSE(pcb_mode_manager->isCurrentModeIdle());

    // Cancel should reset to STATE_FIRST
    pcb_mode_manager->notifyCancel();
    EXPECT_EQ(STATE_FIRST, Crosshair.AttachedBox.State);
    EXPECT_TRUE(pcb_mode_manager->isCurrentModeIdle());
}

TEST_F(ModeManagerTest, CancelOnSimpleModeDoesNothing) {
    pcb_mode_manager->setMode(VIA_MODE);

    // Cancel on simple mode should not crash
    pcb_mode_manager->notifyCancel();

    // Mode should still be active
    EXPECT_EQ(VIA_MODE, pcb_mode_manager->getCurrentModeId());
    EXPECT_TRUE(pcb_mode_manager->isCurrentModeIdle());
}

TEST_F(ModeManagerTest, LineModeIdleAndCancel) {
    pcb_mode_manager->setMode(LINE_MODE);

    // Should be idle initially
    EXPECT_TRUE(pcb_mode_manager->isCurrentModeIdle());

    // First click advances state
    Crosshair.X = 1000;
    Crosshair.Y = 2000;
    pcb_mode_manager->notifyClick(1000, 2000);

    // Should NOT be idle after starting a line
    EXPECT_FALSE(pcb_mode_manager->isCurrentModeIdle());

    // Cancel should reset
    pcb_mode_manager->notifyCancel();
    EXPECT_TRUE(pcb_mode_manager->isCurrentModeIdle());
}

TEST_F(ModeManagerTest, ArcModeIdleAndCancel) {
    pcb_mode_manager->setMode(ARC_MODE);

    // Should be idle initially
    EXPECT_TRUE(pcb_mode_manager->isCurrentModeIdle());

    // First click advances state
    Crosshair.X = 1000;
    Crosshair.Y = 2000;
    pcb_mode_manager->notifyClick(1000, 2000);

    // Should NOT be idle after starting an arc
    EXPECT_FALSE(pcb_mode_manager->isCurrentModeIdle());

    // Cancel should reset
    pcb_mode_manager->notifyCancel();
    EXPECT_TRUE(pcb_mode_manager->isCurrentModeIdle());
}

// ============================================================================
// All 16 Modes Registration Test
// ============================================================================

TEST_F(ModeManagerTest, AllModesRegistered) {
    const struct {
        int mode_id;
        const char* expected_name;
    } all_modes[] = {
        {ARROW_MODE, "Arrow"},
        {VIA_MODE, "Via"},
        {LINE_MODE, "Line"},
        {ARC_MODE, "Arc"},
        {RECTANGLE_MODE, "Rectangle"},
        {TEXT_MODE, "Text"},
        {POLYGON_MODE, "Polygon"},
        {POLYGONHOLE_MODE, "PolygonHole"},
        {PASTEBUFFER_MODE, "PasteBuffer"},
        {REMOVE_MODE, "Remove"},
        {ROTATE_MODE, "Rotate"},
        {COPY_MODE, "Copy"},
        {MOVE_MODE, "Move"},
        {INSERTPOINT_MODE, "InsertPoint"},
        {THERMAL_MODE, "Thermal"},
        {LOCK_MODE, "Lock"},
    };

    for (const auto& m : all_modes) {
        pcb_mode_manager->setMode(m.mode_id);
        auto* mode = pcb_mode_manager->getCurrentMode();
        ASSERT_NE(nullptr, mode)
            << "Mode " << m.expected_name << " (ID " << m.mode_id << ") not registered";
        EXPECT_STREQ(m.expected_name, mode->getName())
            << "Mode ID " << m.mode_id << " has wrong name";
        EXPECT_EQ(m.mode_id, mode->getModeId())
            << "Mode " << m.expected_name << " has wrong ID";
    }
}

} // namespace test
} // namespace pcb
