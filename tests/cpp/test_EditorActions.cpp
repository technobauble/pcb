/*!
 * \file tests/cpp/test_EditorActions.cpp
 *
 * \brief Google Test unit tests for EditorActions.
 *
 * Tests for the Atomic and MarkCrosshair actions to verify they are
 * properly registered and have correct metadata.
 */

#include <gtest/gtest.h>
#include <cstring>

// Include action system headers
#include "actions/EditorActions.h"
#include "actions/action_bridge.h"
#include "actions/Action.h"

namespace pcb {
namespace test {

//=============================================================================
// Test Fixture for EditorActions
//=============================================================================

class EditorActionsTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Initialize action system before each test
    pcb_action_init();
  }

  void TearDown() override {
    // Cleanup after each test if needed
  }
};

//=============================================================================
// Registration Tests
//=============================================================================

TEST_F(EditorActionsTest, ActionsAreRegistered) {
  // Verify both actions are registered
  EXPECT_EQ(1, pcb_action_exists("Atomic")) << "Atomic action not registered";
  EXPECT_EQ(1, pcb_action_exists("MarkCrosshair")) << "MarkCrosshair action not registered";
}

TEST_F(EditorActionsTest, ActionsHaveCorrectNames) {
  pcb::actions::ActionRegistry& registry = pcb::actions::ActionRegistry::instance();

  pcb::actions::Action* atomic = registry.lookup("Atomic");
  ASSERT_NE(nullptr, atomic) << "Atomic action not found in registry";
  EXPECT_STREQ("Atomic", atomic->name());

  pcb::actions::Action* markcrosshair = registry.lookup("MarkCrosshair");
  ASSERT_NE(nullptr, markcrosshair) << "MarkCrosshair action not found in registry";
  EXPECT_STREQ("MarkCrosshair", markcrosshair->name());
}

//=============================================================================
// Metadata Tests
//=============================================================================

TEST_F(EditorActionsTest, ActionsHaveHelpText) {
  pcb::actions::ActionRegistry& registry = pcb::actions::ActionRegistry::instance();

  pcb::actions::Action* atomic = registry.lookup("Atomic");
  ASSERT_NE(nullptr, atomic);
  EXPECT_NE(nullptr, atomic->help());
  EXPECT_GT(strlen(atomic->help()), 0) << "Atomic action has empty help text";
  EXPECT_NE(nullptr, strstr(atomic->help(), "undo")) << "Atomic help should mention undo";

  pcb::actions::Action* markcrosshair = registry.lookup("MarkCrosshair");
  ASSERT_NE(nullptr, markcrosshair);
  EXPECT_NE(nullptr, markcrosshair->help());
  EXPECT_GT(strlen(markcrosshair->help()), 0) << "MarkCrosshair action has empty help text";
  EXPECT_NE(nullptr, strstr(markcrosshair->help(), "Crosshair") ||
                     strstr(markcrosshair->help(), "mark"))
    << "MarkCrosshair help should mention crosshair or mark";
}

TEST_F(EditorActionsTest, ActionsHaveSyntax) {
  pcb::actions::ActionRegistry& registry = pcb::actions::ActionRegistry::instance();

  pcb::actions::Action* atomic = registry.lookup("Atomic");
  ASSERT_NE(nullptr, atomic);
  EXPECT_NE(nullptr, atomic->syntax());
  EXPECT_NE(nullptr, strstr(atomic->syntax(), "Atomic")) << "Atomic syntax should contain 'Atomic'";
  EXPECT_NE(nullptr, strstr(atomic->syntax(), "Save")) << "Atomic syntax should mention Save";

  pcb::actions::Action* markcrosshair = registry.lookup("MarkCrosshair");
  ASSERT_NE(nullptr, markcrosshair);
  EXPECT_NE(nullptr, markcrosshair->syntax());
  EXPECT_NE(nullptr, strstr(markcrosshair->syntax(), "MarkCrosshair"))
    << "MarkCrosshair syntax should contain 'MarkCrosshair'";
}

//=============================================================================
// C Bridge Tests
//=============================================================================

TEST_F(EditorActionsTest, CanLookupViaCBridge) {
  // Verify actions can be looked up via C bridge
  EXPECT_EQ(1, pcb_action_exists("Atomic"));
  EXPECT_EQ(1, pcb_action_exists("MarkCrosshair"));
}

TEST_F(EditorActionsTest, UnknownActionReturnsNotFound) {
  int result = pcb_action_execute("ThisActionDoesNotExist", 0, nullptr, 0, 0);
  EXPECT_EQ(-1, result) << "Unknown action should return -1";
}

//=============================================================================
// Individual Action Tests
//=============================================================================

TEST_F(EditorActionsTest, AtomicActionExists) {
  pcb::actions::ActionRegistry& registry = pcb::actions::ActionRegistry::instance();
  pcb::actions::Action* action = registry.lookup("Atomic");

  ASSERT_NE(nullptr, action);
  EXPECT_STREQ("Atomic", action->name());
  EXPECT_NE(nullptr, action->help());
  EXPECT_NE(nullptr, action->syntax());
}

TEST_F(EditorActionsTest, MarkCrosshairActionExists) {
  pcb::actions::ActionRegistry& registry = pcb::actions::ActionRegistry::instance();
  pcb::actions::Action* action = registry.lookup("MarkCrosshair");

  ASSERT_NE(nullptr, action);
  EXPECT_STREQ("MarkCrosshair", action->name());
  EXPECT_NE(nullptr, action->help());
  EXPECT_NE(nullptr, action->syntax());
}

} // namespace test
} // namespace pcb
