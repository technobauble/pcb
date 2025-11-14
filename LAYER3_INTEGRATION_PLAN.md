# Layer 3 Integration Test Plan

**Date:** November 14, 2025
**Goal:** Validate C++ actions work in real PCB application
**Actions to integrate:** MessageAction, SaveSettingsAction

---

## Integration Strategy

### Phase 1: Add to Build System

**Modify src/Makefile.am:**
```makefile
# Add C++ action sources
PCB_CXX_SRCS = \
	actions/Action.cpp \
	actions/action_bridge.cpp \
	actions/MessageAction.cpp \
	actions/SaveSettingsAction.cpp

# Include in main build
pcb_SOURCES = $(PCB_SRCS) $(PCB_CXX_SRCS)
```

### Phase 2: Add Fallback to action.c

**Minimal changes to action.c:**

```c
// At top of file, after includes:
#include "actions/action_bridge.h"

// Modify ActionMessage:
static int ActionMessage(int argc, char **argv, Coord x, Coord y) {
    // Try C++ version first
    int cpp_result = pcb_action_execute("Message", argc, argv, x, y);
    if (cpp_result != -1) {
        return cpp_result;  // C++ handled it
    }

    // Fall back to C version
    int i;
    if (argc < 1)
        AFAIL (message);
    for (i = 0; i < argc; i++) {
        Message (argv[i]);
        Message ("\n");
    }
    return 0;
}

// Modify ActionSaveSettings:
static int ActionSaveSettings(int argc, char **argv, Coord x, Coord y) {
    // Try C++ version first
    int cpp_result = pcb_action_execute("SaveSettings", argc, argv, x, y);
    if (cpp_result != -1) {
        return cpp_result;  // C++ handled it
    }

    // Fall back to C version
    int locally = argc > 0 ? (strncasecmp (argv[0], "local", 5) == 0) : 0;
    hid_save_settings (locally);
    return 0;
}
```

### Phase 3: Build and Test

**Build commands:**
```bash
cd /home/user/pcb
./autogen.sh
./configure --disable-doc
make clean
make
```

**Test plan:**
1. **Verify compilation** - Both C and C++ code compile together
2. **Verify linking** - Action registry links correctly
3. **Test Message action** - Run PCB, execute Message action
4. **Test SaveSettings action** - Run PCB, execute SaveSettings
5. **Verify fallback** - Disable C++ actions, verify C version still works

---

## Expected Outcomes

### Success Criteria

✅ **Compilation:** No errors, both .c and .cpp files compile
✅ **Linking:** Binary links successfully
✅ **Registration:** pcb_action_count() returns 2
✅ **Execution:** Message() action works in PCB
✅ **Execution:** SaveSettings() action works in PCB
✅ **Fallback:** If C++ disabled, C version still works

### Risk Mitigation

**If something breaks:**
1. Check action_bridge.cpp exports (extern "C")
2. Verify static initialization order
3. Test with simple main() that calls pcb_action_init()
4. Enable DEBUG_ACTIONS in action_bridge.cpp

**Rollback plan:**
```bash
git revert HEAD  # Remove fallback changes
make clean && make  # Back to pure C
```

---

## Alternative: Minimal Integration Test

Instead of modifying action.c, create a standalone integration test:

**File: tests/integration/action_integration_test.c**

```c
#include <stdio.h>
#include "actions/action_bridge.h"

int main() {
    printf("Testing C++ action integration...\n\n");

    // Initialize C++ action system
    pcb_action_init();

    // Check registration
    int count = pcb_action_count();
    printf("Registered C++ actions: %d\n", count);

    if (count != 2) {
        fprintf(stderr, "ERROR: Expected 2 actions, got %d\n", count);
        return 1;
    }

    // Test Message action
    printf("\nTesting Message action:\n");
    char* msg_argv[] = {"Test message from integration"};
    int result1 = pcb_action_execute("Message", 1, msg_argv, 0, 0);
    printf("Result: %d (0 = success)\n", result1);

    // Test SaveSettings action
    printf("\nTesting SaveSettings action:\n");
    char* save_argv[] = {"local"};
    int result2 = pcb_action_execute("SaveSettings", 1, save_argv, 0, 0);
    printf("Result: %d (0 = success)\n", result2);

    // Test non-existent action
    printf("\nTesting non-existent action:\n");
    int result3 = pcb_action_execute("NonExistent", 0, NULL, 0, 0);
    printf("Result: %d (-1 = not found)\n", result3);

    if (result1 == 0 && result2 == 0 && result3 == -1) {
        printf("\n✓ ALL INTEGRATION TESTS PASSED!\n");
        return 0;
    } else {
        printf("\n✗ INTEGRATION TESTS FAILED\n");
        return 1;
    }
}
```

**Benefits of minimal approach:**
- No changes to action.c yet
- Tests integration without risk
- Validates action_bridge works
- Can test before committing to fallback

---

## Recommended Approach

**Start with minimal integration test:**

1. Create integration test (above)
2. Add to build system
3. Run test
4. **Only if test passes**, add fallback to action.c

This validates the bridge works before touching action.c.

---

## Next Steps

**Option A: Full Integration (higher risk)**
1. Modify src/Makefile.am
2. Modify action.c with fallback
3. Build and test
4. Commit if successful

**Option B: Minimal Integration (lower risk)** ✅ RECOMMENDED
1. Create integration test
2. Add to build system
3. Test C++ actions work
4. **Then** add fallback to action.c

Choose Option B to validate incrementally.

