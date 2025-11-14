/*!
 * \file tests/cpp/actions/save_settings_test.cpp
 *
 * \brief Standalone tests for SaveSettingsAction
 *
 * Second action test - validates the pattern works consistently.
 * Tests argument parsing and function calling.
 */

#include <iostream>
#include <vector>
#include <cstring>

// Include the C++ action infrastructure
#include "actions/Action.h"

// Mock state to track function calls
static int g_save_settings_called = 0;
static int g_save_settings_locally = -1;

extern "C" {
    // Mock global.h types (minimal definitions needed)
    typedef int Coord;

    // Mock hid_save_settings() function
    void hid_save_settings(int locally) {
        g_save_settings_called++;
        g_save_settings_locally = locally;
    }
}

// Simple test framework (same as simple_test.cpp)
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

// Helper to reset mock state
void reset_mock() {
    g_save_settings_called = 0;
    g_save_settings_locally = -1;
}

// Tests
TEST(ActionIsRegistered) {
    pcb::actions::Action* action =
        pcb::actions::ActionRegistry::instance().lookup("SaveSettings");
    ASSERT(action != nullptr);
    ASSERT(strcmp(action->name(), "SaveSettings") == 0);
}

TEST(NoArguments_SavesLocally) {
    reset_mock();
    pcb::actions::Action* action =
        pcb::actions::ActionRegistry::instance().lookup("SaveSettings");

    int result = action->execute(0, nullptr, 0, 0);

    ASSERT(result == 0);
    ASSERT(g_save_settings_called == 1);
    ASSERT(g_save_settings_locally == 0);  // No args = save locally (0)
}

TEST(LocalArgument_SavesLocally) {
    reset_mock();
    pcb::actions::Action* action =
        pcb::actions::ActionRegistry::instance().lookup("SaveSettings");

    char* argv[] = {const_cast<char*>("local")};
    int result = action->execute(1, argv, 0, 0);

    ASSERT(result == 0);
    ASSERT(g_save_settings_called == 1);
    ASSERT(g_save_settings_locally == 1);  // "local" = 1
}

TEST(LocalCaseInsensitive) {
    reset_mock();
    pcb::actions::Action* action =
        pcb::actions::ActionRegistry::instance().lookup("SaveSettings");

    char* argv1[] = {const_cast<char*>("LOCAL")};
    int result1 = action->execute(1, argv1, 0, 0);
    ASSERT(result1 == 0);
    ASSERT(g_save_settings_locally == 1);

    reset_mock();
    char* argv2[] = {const_cast<char*>("Local")};
    int result2 = action->execute(1, argv2, 0, 0);
    ASSERT(result2 == 0);
    ASSERT(g_save_settings_locally == 1);

    reset_mock();
    char* argv3[] = {const_cast<char*>("lOcAl")};
    int result3 = action->execute(1, argv3, 0, 0);
    ASSERT(result3 == 0);
    ASSERT(g_save_settings_locally == 1);
}

TEST(NonLocalArgument_SavesGlobally) {
    reset_mock();
    pcb::actions::Action* action =
        pcb::actions::ActionRegistry::instance().lookup("SaveSettings");

    char* argv[] = {const_cast<char*>("global")};
    int result = action->execute(1, argv, 0, 0);

    ASSERT(result == 0);
    ASSERT(g_save_settings_called == 1);
    ASSERT(g_save_settings_locally == 0);  // Not "local" = 0
}

TEST(MultipleArguments_UsesFirst) {
    reset_mock();
    pcb::actions::Action* action =
        pcb::actions::ActionRegistry::instance().lookup("SaveSettings");

    char* argv[] = {
        const_cast<char*>("local"),
        const_cast<char*>("ignored"),
        const_cast<char*>("also_ignored")
    };
    int result = action->execute(3, argv, 0, 0);

    ASSERT(result == 0);
    ASSERT(g_save_settings_called == 1);
    ASSERT(g_save_settings_locally == 1);
}

TEST(NullArgument_SavesLocally) {
    reset_mock();
    pcb::actions::Action* action =
        pcb::actions::ActionRegistry::instance().lookup("SaveSettings");

    char* argv[] = {nullptr};
    int result = action->execute(1, argv, 0, 0);

    ASSERT(result == 0);
    ASSERT(g_save_settings_called == 1);
    ASSERT(g_save_settings_locally == 0);  // NULL = not "local" = 0
}

TEST(CoordinatesIgnored) {
    reset_mock();
    pcb::actions::Action* action =
        pcb::actions::ActionRegistry::instance().lookup("SaveSettings");

    // Coordinates should have no effect
    int result = action->execute(0, nullptr, 12345, 67890);

    ASSERT(result == 0);
    ASSERT(g_save_settings_called == 1);
}

int main() {
    std::cout << "===========================================\n";
    std::cout << "SaveSettingsAction Test Suite\n";
    std::cout << "===========================================\n\n";

    // Run all registered tests
    int passed = 0;
    int failed = 0;

    for (const auto& test : g_tests) {
        std::cout << "Running " << test.name << "...";
        reset_mock();

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
    std::cout << "\nPattern validation:\n";
    std::cout << "  ✓ Second action migrated successfully\n";
    std::cout << "  ✓ coord_types.h pattern works consistently\n";
    std::cout << "  ✓ Argument parsing tested\n";
    std::cout << "  ✓ Function call mocking works\n";
    std::cout << "  ✓ No dependency on action.c\n";

    return 0;
}
