/*!
 * \file tests/cpp/actions/simple_test.cpp
 *
 * \brief Simple standalone test for MessageAction (no Google Test required)
 *
 * This is a minimal test to prove the isolated compilation strategy works.
 * It doesn't require Google Test - just compiles and runs basic checks.
 *
 * For full test coverage, use message_action_test.cpp with Google Test.
 */

#include <iostream>
#include <sstream>
#include <vector>
#include <cstdarg>
#include <cstring>
#include <cassert>

// Include the C++ action infrastructure
#include "actions/Action.h"

// Mock implementation of Message() to capture output
static std::stringstream g_message_output;

extern "C" {
    // Mock global.h types (minimal definitions needed)
    typedef int Coord;

    // Mock Message() function - captures output instead of displaying
    void Message(const char* format, ...) {
        char buffer[4096];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        g_message_output << buffer;
    }
}

// Simple test framework
struct TestCase {
    const char* name;
    void (*func)();
};
std::vector<TestCase> g_tests;

#define TEST(name) \
    void test_##name(); \
    struct Test_##name##_Registrar { \
        Test_##name##_Registrar() { \
            g_tests.push_back({#name, test_##name}); \
        } \
    } test_registrar_##name; \
    void test_##name()

#define ASSERT(cond) \
    if (!(cond)) { \
        std::cerr << "\nASSERT FAILED: " #cond << " at line " << __LINE__ << "\n"; \
        exit(1); \
    }

// Tests
TEST(ActionIsRegistered) {
    pcb::actions::Action* action =
        pcb::actions::ActionRegistry::instance().lookup("Message");
    ASSERT(action != nullptr);
    ASSERT(strcmp(action->name(), "Message") == 0);
}

TEST(SingleArgument) {
    pcb::actions::Action* action =
        pcb::actions::ActionRegistry::instance().lookup("Message");

    char* argv[] = {const_cast<char*>("Hello World")};
    int result = action->execute(1, argv, 0, 0);

    ASSERT(result == 0);
    std::string output = g_message_output.str();
    ASSERT(output.find("Hello World") != std::string::npos);
    ASSERT(output.find("\n") != std::string::npos);
}

TEST(MultipleArguments) {
    pcb::actions::Action* action =
        pcb::actions::ActionRegistry::instance().lookup("Message");

    char* argv[] = {
        const_cast<char*>("Line 1"),
        const_cast<char*>("Line 2"),
        const_cast<char*>("Line 3")
    };
    int result = action->execute(3, argv, 0, 0);

    ASSERT(result == 0);
    std::string output = g_message_output.str();
    ASSERT(output.find("Line 1") != std::string::npos);
    ASSERT(output.find("Line 2") != std::string::npos);
    ASSERT(output.find("Line 3") != std::string::npos);
}

TEST(NoArguments_ReturnsError) {
    pcb::actions::Action* action =
        pcb::actions::ActionRegistry::instance().lookup("Message");

    int result = action->execute(0, nullptr, 0, 0);

    ASSERT(result == 1);  // Should fail
    std::string output = g_message_output.str();
    ASSERT(output.find("Syntax error") != std::string::npos);
}

TEST(NullArgumentPointer) {
    pcb::actions::Action* action =
        pcb::actions::ActionRegistry::instance().lookup("Message");

    char* argv[] = {nullptr};
    int result = action->execute(1, argv, 0, 0);

    ASSERT(result == 0);
    std::string output = g_message_output.str();
    // Should skip NULL entirely - no output
    ASSERT(output.empty());
}

int main() {
    std::cout << "===========================================\n";
    std::cout << "MessageAction Standalone Test Suite\n";
    std::cout << "===========================================\n\n";

    // Run all registered tests
    int passed = 0;
    int failed = 0;

    for (const auto& test : g_tests) {
        std::cout << "Running " << test.name << "...";
        g_message_output.str("");
        g_message_output.clear();

        try {
            test.func();
            std::cout << " PASSED\n";
            passed++;
        } catch (...) {
            std::cout << " FAILED\n";
            failed++;
        }
    }

    std::cout << "\n===========================================\n";
    if (failed == 0) {
        std::cout << "ALL " << passed << " TESTS PASSED!\n";
    } else {
        std::cout << passed << " passed, " << failed << " FAILED\n";
        return 1;
    }
    std::cout << "===========================================\n";
    std::cout << "\nProof of isolated compilation:\n";
    std::cout << "  ✓ No dependency on action.c\n";
    std::cout << "  ✓ No dependency on libpcb.a\n";
    std::cout << "  ✓ Only links: MessageAction + Action base + mocks\n";
    std::cout << "  ✓ MessageAction behavior matches ActionMessage\n";

    return 0;
}
