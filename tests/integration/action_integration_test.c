/*!
 * \file tests/integration/action_integration_test.c
 *
 * \brief Integration test for C++ action system
 *
 * This tests that C++ actions can be called from C code and work
 * correctly with the real PCB infrastructure (not mocked).
 *
 * This is a minimal integration test - doesn't modify action.c,
 * just validates the action_bridge works.
 */

#include <stdio.h>
#include <string.h>

// Include the C++ action bridge
#include "actions/action_bridge.h"

int main() {
    printf("==========================================\n");
    printf("C++ Action Integration Test\n");
    printf("==========================================\n\n");

    // Initialize C++ action system
    printf("Initializing C++ action system...\n");
    pcb_action_init();
    printf("✓ Initialized\n\n");

    // Check how many actions are registered
    int count = pcb_action_count();
    printf("Registered C++ actions: %d\n", count);

    if (count < 2) {
        fprintf(stderr, "ERROR: Expected at least 2 actions, got %d\n", count);
        fprintf(stderr, "Did MessageAction and SaveSettingsAction register?\n");
        return 1;
    }
    printf("✓ Expected actions registered\n\n");

    // List all registered actions
    printf("Listing all registered C++ actions:\n");
    pcb_action_list_all();
    printf("\n");

    // Test 1: Message action exists
    printf("Test 1: Check Message action exists...\n");
    if (!pcb_action_exists("Message")) {
        fprintf(stderr, "ERROR: Message action not registered\n");
        return 1;
    }
    printf("✓ Message action found\n\n");

    // Test 2: SaveSettings action exists
    printf("Test 2: Check SaveSettings action exists...\n");
    if (!pcb_action_exists("SaveSettings")) {
        fprintf(stderr, "ERROR: SaveSettings action not registered\n");
        return 1;
    }
    printf("✓ SaveSettings action found\n\n");

    // Test 3: Execute Message action
    printf("Test 3: Execute Message action...\n");
    char* msg_argv[] = {
        "Integration test message 1",
        "Integration test message 2"
    };
    int result1 = pcb_action_execute("Message", 2, msg_argv, 0, 0);
    if (result1 != 0) {
        fprintf(stderr, "ERROR: Message action returned %d (expected 0)\n", result1);
        return 1;
    }
    printf("✓ Message action executed successfully\n\n");

    // Test 4: Execute SaveSettings action with "local"
    printf("Test 4: Execute SaveSettings action (local)...\n");
    char* save_argv1[] = {"local"};
    int result2 = pcb_action_execute("SaveSettings", 1, save_argv1, 0, 0);
    if (result2 != 0) {
        fprintf(stderr, "ERROR: SaveSettings action returned %d (expected 0)\n", result2);
        return 1;
    }
    printf("✓ SaveSettings(local) executed successfully\n\n");

    // Test 5: Execute SaveSettings action without args
    printf("Test 5: Execute SaveSettings action (no args)...\n");
    int result3 = pcb_action_execute("SaveSettings", 0, NULL, 0, 0);
    if (result3 != 0) {
        fprintf(stderr, "ERROR: SaveSettings action returned %d (expected 0)\n", result3);
        return 1;
    }
    printf("✓ SaveSettings() executed successfully\n\n");

    // Test 6: Non-existent action returns -1
    printf("Test 6: Non-existent action returns -1...\n");
    int result4 = pcb_action_execute("NonExistentAction", 0, NULL, 0, 0);
    if (result4 != -1) {
        fprintf(stderr, "ERROR: Non-existent action returned %d (expected -1)\n", result4);
        return 1;
    }
    printf("✓ Non-existent action handled correctly\n\n");

    // Test 7: Message action with no args returns error
    printf("Test 7: Message action with no args returns error...\n");
    int result5 = pcb_action_execute("Message", 0, NULL, 0, 0);
    if (result5 != 1) {
        fprintf(stderr, "ERROR: Message() with no args returned %d (expected 1)\n", result5);
        return 1;
    }
    printf("✓ Message() validation works\n\n");

    // All tests passed
    printf("==========================================\n");
    printf("✓ ALL 7 INTEGRATION TESTS PASSED!\n");
    printf("==========================================\n\n");

    printf("Integration validated:\n");
    printf("  ✓ C++ action system initializes\n");
    printf("  ✓ Actions register correctly\n");
    printf("  ✓ C code can call C++ actions\n");
    printf("  ✓ Arguments pass correctly\n");
    printf("  ✓ Return values work\n");
    printf("  ✓ Error handling works\n");
    printf("  ✓ Non-existent actions handled\n\n");

    printf("Ready for Layer 3 integration with action.c!\n");

    return 0;
}
