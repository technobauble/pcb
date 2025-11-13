/*!
 * \file tests/cpp/test_SelectionActions.cpp
 *
 * \brief Google Test unit tests for SelectionActions.
 *
 * Tests for the Select, Unselect, and RemoveSelected actions to verify
 * they are properly registered and can be executed via the C++ action
 * system and C bridge.
 */

#include <gtest/gtest.h>
#include <cstring>

// Include action system headers
// Note: These are C++ headers, no extern "C" needed
#include "actions/SelectionActions.h"
#include "actions/action_bridge.h"
#include "actions/Action.h"

namespace pcb {
namespace test {

//=============================================================================
// Test Fixture for SelectionActions
//=============================================================================

class SelectionActionsTest : public ::testing::Test {
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

TEST_F(SelectionActionsTest, ActionsAreRegistered) {
  // Verify all three actions are registered
  int count = pcb_action_count();
  EXPECT_GE(count, 3) << "Expected at least 3 actions to be registered";

  EXPECT_EQ(1, pcb_action_exists("Select")) << "Select action not registered";
  EXPECT_EQ(1, pcb_action_exists("Unselect")) << "Unselect action not registered";
  EXPECT_EQ(1, pcb_action_exists("RemoveSelected")) << "RemoveSelected action not registered";
}

TEST_F(SelectionActionsTest, ActionsHaveCorrectNames) {
  pcb::actions::ActionRegistry& registry = pcb::actions::ActionRegistry::instance();

  pcb::actions::Action* select = registry.lookup("Select");
  ASSERT_NE(nullptr, select) << "Select action not found in registry";
  EXPECT_STREQ("Select", select->name());

  pcb::actions::Action* unselect = registry.lookup("Unselect");
  ASSERT_NE(nullptr, unselect) << "Unselect action not found in registry";
  EXPECT_STREQ("Unselect", unselect->name());

  pcb::actions::Action* remove = registry.lookup("RemoveSelected");
  ASSERT_NE(nullptr, remove) << "RemoveSelected action not found in registry";
  EXPECT_STREQ("RemoveSelected", remove->name());
}

//=============================================================================
// Metadata Tests
//=============================================================================

TEST_F(SelectionActionsTest, ActionsHaveHelpText) {
  pcb::actions::ActionRegistry& registry = pcb::actions::ActionRegistry::instance();

  pcb::actions::Action* select = registry.lookup("Select");
  ASSERT_NE(nullptr, select);
  EXPECT_NE(nullptr, select->help());
  EXPECT_GT(strlen(select->help()), 0) << "Select action has empty help text";

  pcb::actions::Action* unselect = registry.lookup("Unselect");
  ASSERT_NE(nullptr, unselect);
  EXPECT_NE(nullptr, unselect->help());
  EXPECT_GT(strlen(unselect->help()), 0) << "Unselect action has empty help text";

  pcb::actions::Action* remove = registry.lookup("RemoveSelected");
  ASSERT_NE(nullptr, remove);
  EXPECT_NE(nullptr, remove->help());
  EXPECT_GT(strlen(remove->help()), 0) << "RemoveSelected action has empty help text";
}

TEST_F(SelectionActionsTest, ActionsHaveSyntax) {
  pcb::actions::ActionRegistry& registry = pcb::actions::ActionRegistry::instance();

  pcb::actions::Action* select = registry.lookup("Select");
  ASSERT_NE(nullptr, select);
  EXPECT_NE(nullptr, select->syntax());
  EXPECT_GT(strlen(select->syntax()), 0) << "Select action has empty syntax";

  pcb::actions::Action* unselect = registry.lookup("Unselect");
  ASSERT_NE(nullptr, unselect);
  EXPECT_NE(nullptr, unselect->syntax());
  EXPECT_GT(strlen(unselect->syntax()), 0) << "Unselect action has empty syntax";

  pcb::actions::Action* remove = registry.lookup("RemoveSelected");
  ASSERT_NE(nullptr, remove);
  EXPECT_NE(nullptr, remove->syntax());
  EXPECT_GT(strlen(remove->syntax()), 0) << "RemoveSelected action has empty syntax";
}

//=============================================================================
// C Bridge Tests
//=============================================================================

TEST_F(SelectionActionsTest, CanExecuteViaBridge) {
  // Note: These will fail without a full PCB environment, but we can verify
  // that the bridge routing works and doesn't crash

  // Execute RemoveSelected with no arguments (should handle gracefully)
  // Result might be 0 (success) or error code, but should not be -1 (not found)
  int result = pcb_action_execute("RemoveSelected", 0, nullptr, 0, 0);
  EXPECT_NE(-1, result) << "RemoveSelected not found via C bridge";
}

TEST_F(SelectionActionsTest, UnknownActionReturnsNotFound) {
  int result = pcb_action_execute("ThisActionDoesNotExist", 0, nullptr, 0, 0);
  EXPECT_EQ(-1, result) << "Unknown action should return -1";
}

//=============================================================================
// Individual Action Tests
//=============================================================================

TEST_F(SelectionActionsTest, SelectActionExists) {
  pcb::actions::ActionRegistry& registry = pcb::actions::ActionRegistry::instance();
  pcb::actions::Action* action = registry.lookup("Select");

  ASSERT_NE(nullptr, action);
  EXPECT_STREQ("Select", action->name());
  EXPECT_NE(nullptr, action->help());
  EXPECT_NE(nullptr, action->syntax());
}

TEST_F(SelectionActionsTest, UnselectActionExists) {
  pcb::actions::ActionRegistry& registry = pcb::actions::ActionRegistry::instance();
  pcb::actions::Action* action = registry.lookup("Unselect");

  ASSERT_NE(nullptr, action);
  EXPECT_STREQ("Unselect", action->name());
  EXPECT_NE(nullptr, action->help());
  EXPECT_NE(nullptr, action->syntax());
}

TEST_F(SelectionActionsTest, RemoveSelectedActionExists) {
  pcb::actions::ActionRegistry& registry = pcb::actions::ActionRegistry::instance();
  pcb::actions::Action* action = registry.lookup("RemoveSelected");

  ASSERT_NE(nullptr, action);
  EXPECT_STREQ("RemoveSelected", action->name());
  EXPECT_NE(nullptr, action->help());
  EXPECT_NE(nullptr, action->syntax());
}

} // namespace test
} // namespace pcb
