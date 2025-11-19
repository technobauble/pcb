/*!
 * \file tests/cpp/actions/message_action_test.cpp
 *
 * \brief Standalone tests for MessageAction
 *
 * These tests run in complete isolation - no dependency on action.c
 * or the full PCB library. Message() is mocked to capture output.
 *
 * This demonstrates the Layer 2 testing strategy from the migration plan.
 */

#include <gtest/gtest.h>
#include <sstream>
#include <cstdarg>

// Include the C++ action infrastructure
#include "actions/Action.h"

// Mock implementation of Message() to capture output
static std::stringstream g_message_output;
static int g_message_call_count = 0;

extern "C" {
    // Mock global.h types (minimal definitions needed)
    typedef int Coord;

    // Mock Message() function - captures output instead of displaying
    void Message(const char* format, ...) {
        g_message_call_count++;

        char buffer[4096];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        g_message_output << buffer;
    }
}

namespace pcb {
namespace actions {
namespace test {

/*!
 * \brief Test fixture for MessageAction
 */
class MessageActionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear mock state before each test
        g_message_output.str("");
        g_message_output.clear();
        g_message_call_count = 0;

        // Look up the registered action
        action_ = ActionRegistry::instance().lookup("Message");

        // Verify action was registered
        ASSERT_NE(nullptr, action_) << "MessageAction not registered";
        ASSERT_STREQ("Message", action_->name());
    }

    void TearDown() override {
        // Clear mock state after test
        g_message_output.str("");
        g_message_output.clear();
        g_message_call_count = 0;
    }

    // Helper to get captured output
    std::string getCapturedOutput() const {
        return g_message_output.str();
    }

    // The action under test
    Action* action_;
};

// Test: Single argument
TEST_F(MessageActionTest, SingleArgument) {
    char* argv[] = {const_cast<char*>("Hello World")};

    int result = action_->execute(1, argv, 0, 0);

    EXPECT_EQ(0, result) << "Action should succeed";

    std::string output = getCapturedOutput();
    EXPECT_NE(std::string::npos, output.find("Hello World"))
        << "Output should contain the message";
    EXPECT_NE(std::string::npos, output.find("\n"))
        << "Output should contain newline";

    // Should call Message() twice: once for text, once for newline
    EXPECT_EQ(2, g_message_call_count);
}

// Test: Multiple arguments
TEST_F(MessageActionTest, MultipleArguments) {
    char* argv[] = {
        const_cast<char*>("Line 1"),
        const_cast<char*>("Line 2"),
        const_cast<char*>("Line 3")
    };

    int result = action_->execute(3, argv, 0, 0);

    EXPECT_EQ(0, result) << "Action should succeed";

    std::string output = getCapturedOutput();
    EXPECT_NE(std::string::npos, output.find("Line 1"));
    EXPECT_NE(std::string::npos, output.find("Line 2"));
    EXPECT_NE(std::string::npos, output.find("Line 3"));

    // Should call Message() 6 times: 3 messages + 3 newlines
    EXPECT_EQ(6, g_message_call_count);
}

// Test: No arguments (error case)
TEST_F(MessageActionTest, NoArguments) {
    int result = action_->execute(0, nullptr, 0, 0);

    EXPECT_EQ(1, result) << "Action should fail with no arguments";

    std::string output = getCapturedOutput();
    EXPECT_NE(std::string::npos, output.find("Syntax error"))
        << "Should display syntax error";
    EXPECT_NE(std::string::npos, output.find("Usage"))
        << "Should display usage message";
    EXPECT_NE(std::string::npos, output.find("Message("))
        << "Should display action syntax";
}

// Test: Empty string argument
TEST_F(MessageActionTest, EmptyStringArgument) {
    char* argv[] = {const_cast<char*>("")};

    int result = action_->execute(1, argv, 0, 0);

    EXPECT_EQ(0, result) << "Action should succeed with empty string";

    std::string output = getCapturedOutput();
    // Should just have a newline
    EXPECT_EQ("\n", output);
}

// Test: NULL argument pointer (defensive)
TEST_F(MessageActionTest, NullArgumentPointer) {
    char* argv[] = {nullptr};

    int result = action_->execute(1, argv, 0, 0);

    EXPECT_EQ(0, result) << "Action should handle NULL gracefully";

    // Should still print newline but skip the NULL message
    std::string output = getCapturedOutput();
    EXPECT_EQ("\n", output);
}

// Test: Coordinates are ignored (not used by Message action)
TEST_F(MessageActionTest, CoordinatesIgnored) {
    char* argv[] = {const_cast<char*>("Test")};

    // Try with different coordinates
    int result1 = action_->execute(1, argv, 0, 0);
    g_message_output.str("");
    int result2 = action_->execute(1, argv, 12345, 67890);

    EXPECT_EQ(result1, result2) << "Coordinates should not affect result";
}

// Test: Action metadata
TEST_F(MessageActionTest, ActionMetadata) {
    EXPECT_STREQ("Message", action_->name());
    EXPECT_NE(nullptr, action_->help());
    EXPECT_NE(nullptr, action_->syntax());

    // Verify help text is reasonable
    std::string help_text = action_->help();
    EXPECT_FALSE(help_text.empty());
    EXPECT_NE(std::string::npos, help_text.find("log window"));

    // Verify syntax is reasonable
    std::string syntax_text = action_->syntax();
    EXPECT_FALSE(syntax_text.empty());
    EXPECT_NE(std::string::npos, syntax_text.find("Message("));
}

// Test: Special characters in message
TEST_F(MessageActionTest, SpecialCharacters) {
    char* argv[] = {
        const_cast<char*>("Tab:\ttest"),
        const_cast<char*>("Quote:\"test\""),
        const_cast<char*>("Backslash:\\test")
    };

    int result = action_->execute(3, argv, 0, 0);

    EXPECT_EQ(0, result);

    std::string output = getCapturedOutput();
    EXPECT_NE(std::string::npos, output.find("Tab:\ttest"));
    EXPECT_NE(std::string::npos, output.find("Quote:\"test\""));
    EXPECT_NE(std::string::npos, output.find("Backslash:\\test"));
}

} // namespace test
} // namespace actions
} // namespace pcb
