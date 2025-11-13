/*!
 * \file src/actions/test_SelectionActions.cpp
 *
 * \brief Unit tests for SelectionActions.
 *
 * This file contains unit tests for the Select, Unselect, and RemoveSelected
 * actions to verify they are properly registered and can be executed.
 */

#include "SelectionActions.h"
#include "action_bridge.h"
#include "Action.h"

#include <iostream>
#include <cstring>
#include <cassert>

// Simple test framework
static int g_tests_run = 0;
static int g_tests_passed = 0;

#define TEST(name) \
    static void test_##name(); \
    static void run_test_##name() { \
        std::cout << "Running test: " #name << "..." << std::flush; \
        g_tests_run++; \
        test_##name(); \
        g_tests_passed++; \
        std::cout << " PASSED" << std::endl; \
    } \
    static void test_##name()

#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "FAILED: " << #condition << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            exit(1); \
        } \
    } while(0)

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            std::cerr << "FAILED: " << #a << " (" << (a) << ") != " << #b << " (" << (b) << ")" << std::endl; \
            exit(1); \
        } \
    } while(0)

//=============================================================================
// Tests
//=============================================================================

TEST(actions_are_registered) {
    // Verify all three actions are registered
    int count = pcb_action_count();
    ASSERT(count >= 3); // At least our 3 actions should be registered

    ASSERT(pcb_action_exists("Select") == 1);
    ASSERT(pcb_action_exists("Unselect") == 1);
    ASSERT(pcb_action_exists("RemoveSelected") == 1);
}

TEST(actions_have_correct_names) {
    pcb::actions::ActionRegistry& registry = pcb::actions::ActionRegistry::instance();

    pcb::actions::Action* select = registry.lookup("Select");
    ASSERT(select != nullptr);
    ASSERT(strcmp(select->name(), "Select") == 0);

    pcb::actions::Action* unselect = registry.lookup("Unselect");
    ASSERT(unselect != nullptr);
    ASSERT(strcmp(unselect->name(), "Unselect") == 0);

    pcb::actions::Action* remove = registry.lookup("RemoveSelected");
    ASSERT(remove != nullptr);
    ASSERT(strcmp(remove->name(), "RemoveSelected") == 0);
}

TEST(actions_have_help_text) {
    pcb::actions::ActionRegistry& registry = pcb::actions::ActionRegistry::instance();

    pcb::actions::Action* select = registry.lookup("Select");
    ASSERT(select->help() != nullptr);
    ASSERT(strlen(select->help()) > 0);

    pcb::actions::Action* unselect = registry.lookup("Unselect");
    ASSERT(unselect->help() != nullptr);
    ASSERT(strlen(unselect->help()) > 0);

    pcb::actions::Action* remove = registry.lookup("RemoveSelected");
    ASSERT(remove->help() != nullptr);
    ASSERT(strlen(remove->help()) > 0);
}

TEST(actions_have_syntax) {
    pcb::actions::ActionRegistry& registry = pcb::actions::ActionRegistry::instance();

    pcb::actions::Action* select = registry.lookup("Select");
    ASSERT(select->syntax() != nullptr);
    ASSERT(strlen(select->syntax()) > 0);

    pcb::actions::Action* unselect = registry.lookup("Unselect");
    ASSERT(unselect->syntax() != nullptr);
    ASSERT(strlen(unselect->syntax()) > 0);

    pcb::actions::Action* remove = registry.lookup("RemoveSelected");
    ASSERT(remove->syntax() != nullptr);
    ASSERT(strlen(remove->syntax()) > 0);
}

TEST(can_execute_via_bridge) {
    // Note: These will fail without a full PCB environment, but we can verify
    // that the bridge routing works and returns a valid result code

    // Execute with no arguments (should handle gracefully)
    int result = pcb_action_execute("RemoveSelected", 0, nullptr, 0, 0);
    // Result might be 0 or error, but should not be -1 (not found)
    ASSERT(result != -1);
}

TEST(unknown_action_returns_not_found) {
    int result = pcb_action_execute("ThisActionDoesNotExist", 0, nullptr, 0, 0);
    ASSERT_EQ(result, -1); // -1 means action not found
}

//=============================================================================
// Main test runner
//=============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "SelectionActions Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Initialize the action system
    pcb_action_init();

    // Run all tests
    run_test_actions_are_registered();
    run_test_actions_have_correct_names();
    run_test_actions_have_help_text();
    run_test_actions_have_syntax();
    run_test_can_execute_via_bridge();
    run_test_unknown_action_returns_not_found();

    // Print summary
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Tests: " << g_tests_passed << "/" << g_tests_run << " passed" << std::endl;

    if (g_tests_passed == g_tests_run) {
        std::cout << "ALL TESTS PASSED ✓" << std::endl;
        std::cout << "========================================" << std::endl;
        return 0;
    } else {
        std::cout << "SOME TESTS FAILED ✗" << std::endl;
        std::cout << "========================================" << std::endl;
        return 1;
    }
}
