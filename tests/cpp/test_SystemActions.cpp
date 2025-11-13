/*!
 * \file tests/cpp/test_SystemActions.cpp
 *
 * \brief Google Test unit tests for SystemActions.
 *
 * Tests for the Quit, Message, and DumpLibrary actions to verify
 * they are properly registered and have correct metadata.
 */

#include <gtest/gtest.h>
#include <cstring>

// Include action system headers
#include "actions/SystemActions.h"
#include "actions/action_bridge.h"
#include "actions/Action.h"

namespace pcb {
namespace test {

//=============================================================================
// Test Fixture for SystemActions
//=============================================================================

class SystemActionsTest : public ::testing::Test {
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

TEST_F(SystemActionsTest, ActionsAreRegistered) {
  // Verify all three actions are registered
  EXPECT_EQ(1, pcb_action_exists("Quit")) << "Quit action not registered";
  EXPECT_EQ(1, pcb_action_exists("Message")) << "Message action not registered";
  EXPECT_EQ(1, pcb_action_exists("DumpLibrary")) << "DumpLibrary action not registered";
}

TEST_F(SystemActionsTest, ActionsHaveCorrectNames) {
  pcb::actions::ActionRegistry& registry = pcb::actions::ActionRegistry::instance();

  pcb::actions::Action* quit = registry.lookup("Quit");
  ASSERT_NE(nullptr, quit) << "Quit action not found in registry";
  EXPECT_STREQ("Quit", quit->name());

  pcb::actions::Action* message = registry.lookup("Message");
  ASSERT_NE(nullptr, message) << "Message action not found in registry";
  EXPECT_STREQ("Message", message->name());

  pcb::actions::Action* dumplibrary = registry.lookup("DumpLibrary");
  ASSERT_NE(nullptr, dumplibrary) << "DumpLibrary action not found in registry";
  EXPECT_STREQ("DumpLibrary", dumplibrary->name());
}

//=============================================================================
// Metadata Tests
//=============================================================================

TEST_F(SystemActionsTest, ActionsHaveHelpText) {
  pcb::actions::ActionRegistry& registry = pcb::actions::ActionRegistry::instance();

  pcb::actions::Action* quit = registry.lookup("Quit");
  ASSERT_NE(nullptr, quit);
  EXPECT_NE(nullptr, quit->help());
  EXPECT_GT(strlen(quit->help()), 0) << "Quit action has empty help text";
  EXPECT_NE(nullptr, strstr(quit->help(), "Quit")) << "Quit help should mention quitting";

  pcb::actions::Action* message = registry.lookup("Message");
  ASSERT_NE(nullptr, message);
  EXPECT_NE(nullptr, message->help());
  EXPECT_GT(strlen(message->help()), 0) << "Message action has empty help text";
  EXPECT_NE(nullptr, strstr(message->help(), "message")) << "Message help should mention messages";

  pcb::actions::Action* dumplibrary = registry.lookup("DumpLibrary");
  ASSERT_NE(nullptr, dumplibrary);
  EXPECT_NE(nullptr, dumplibrary->help());
  EXPECT_GT(strlen(dumplibrary->help()), 0) << "DumpLibrary action has empty help text";
  EXPECT_NE(nullptr, strstr(dumplibrary->help(), "librar")) << "DumpLibrary help should mention library";
}

TEST_F(SystemActionsTest, ActionsHaveSyntax) {
  pcb::actions::ActionRegistry& registry = pcb::actions::ActionRegistry::instance();

  pcb::actions::Action* quit = registry.lookup("Quit");
  ASSERT_NE(nullptr, quit);
  EXPECT_NE(nullptr, quit->syntax());
  EXPECT_STREQ("Quit()", quit->syntax());

  pcb::actions::Action* message = registry.lookup("Message");
  ASSERT_NE(nullptr, message);
  EXPECT_NE(nullptr, message->syntax());
  EXPECT_NE(nullptr, strstr(message->syntax(), "Message")) << "Message syntax should contain 'Message'";

  pcb::actions::Action* dumplibrary = registry.lookup("DumpLibrary");
  ASSERT_NE(nullptr, dumplibrary);
  EXPECT_NE(nullptr, dumplibrary->syntax());
  EXPECT_STREQ("DumpLibrary()", dumplibrary->syntax());
}

//=============================================================================
// C Bridge Tests
//=============================================================================

TEST_F(SystemActionsTest, CanLookupViaC Bridge) {
  // Verify actions can be looked up via C bridge
  EXPECT_EQ(1, pcb_action_exists("Quit"));
  EXPECT_EQ(1, pcb_action_exists("Message"));
  EXPECT_EQ(1, pcb_action_exists("DumpLibrary"));
}

TEST_F(SystemActionsTest, UnknownActionReturnsNotFound) {
  int result = pcb_action_execute("ThisActionDoesNotExist", 0, nullptr, 0, 0);
  EXPECT_EQ(-1, result) << "Unknown action should return -1";
}

//=============================================================================
// Individual Action Tests
//=============================================================================

TEST_F(SystemActionsTest, QuitActionExists) {
  pcb::actions::ActionRegistry& registry = pcb::actions::ActionRegistry::instance();
  pcb::actions::Action* action = registry.lookup("Quit");

  ASSERT_NE(nullptr, action);
  EXPECT_STREQ("Quit", action->name());
  EXPECT_NE(nullptr, action->help());
  EXPECT_NE(nullptr, action->syntax());
}

TEST_F(SystemActionsTest, MessageActionExists) {
  pcb::actions::ActionRegistry& registry = pcb::actions::ActionRegistry::instance();
  pcb::actions::Action* action = registry.lookup("Message");

  ASSERT_NE(nullptr, action);
  EXPECT_STREQ("Message", action->name());
  EXPECT_NE(nullptr, action->help());
  EXPECT_NE(nullptr, action->syntax());
}

TEST_F(SystemActionsTest, DumpLibraryActionExists) {
  pcb::actions::ActionRegistry& registry = pcb::actions::ActionRegistry::instance();
  pcb::actions::Action* action = registry.lookup("DumpLibrary");

  ASSERT_NE(nullptr, action);
  EXPECT_STREQ("DumpLibrary", action->name());
  EXPECT_NE(nullptr, action->help());
  EXPECT_NE(nullptr, action->syntax());
}

} // namespace test
} // namespace pcb
