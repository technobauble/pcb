# Action Migration: Dependency Management Strategy

**Date:** November 14, 2025
**Purpose:** Prevent the mess that happened last time
**Goal:** Migrate actions incrementally without breaking existing code

---

## The Problem from Last Time

Based on your experience, the issues were likely:
- **Circular dependencies** - C++ calling C, C calling C++
- **Linking failures** - Symbol visibility problems across C/C++ boundary
- **Constant patching** - Tests breaking, requiring continuous fixes
- **Unmanageable complexity** - Too many moving parts at once

## Core Principle: Isolation & Validation

**Never modify action.c until the C++ version is proven to work**

---

## Strategy: 3-Layer Approach

```
Layer 1: Isolated C++ Implementation
   ↓
Layer 2: Standalone Testing (no action.c dependency)
   ↓
Layer 3: Integration with Fallback (action.c still works)
```

---

## Layer 1: Isolated C++ Implementation

### Rule: Zero Dependencies on action.c

Each C++ action should:
- Include only stable C headers (error.h, global.h, etc.)
- Never include action.h or action.c
- Call only well-defined C functions
- Be compilable as a standalone .cpp file

### Example: MessageAction

```cpp
// src/actions/MessageAction.cpp
#include "Action.h"

extern "C" {
#include "global.h"
#include "error.h"  // For Message()
}

namespace pcb {
namespace actions {

class MessageAction : public Action {
public:
    MessageAction()
        : Action("Message",
                 "Display a message",
                 "Message(text1, text2, ...)") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        if (argc < 1) {
            Message("Message() requires at least one argument\n");
            return -1;
        }

        for (int i = 0; i < argc; i++) {
            if (argv[i]) {
                Message(argv[i]);
                Message("\n");
            }
        }

        return 0;
    }
};

REGISTER_ACTION(MessageAction);

}} // namespace pcb::actions
```

**Key Points:**
- ✅ Only depends on error.h (stable interface)
- ✅ No action.c dependency
- ✅ Can compile independently
- ✅ Registers itself automatically

---

## Layer 2: Standalone Testing

### Rule: Test Without Integration

Create tests that don't require the full PCB application:

```cpp
// tests/cpp/actions/message_action_test.cpp
#include <gtest/gtest.h>
#include "actions/Action.h"
#include <sstream>

// Mock Message() to capture output instead of displaying
static std::stringstream g_message_output;

extern "C" void Message(const char* format, ...) {
    // Simple mock - just capture the string
    g_message_output << format;
}

namespace pcb {
namespace actions {

class MessageActionTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_message_output.str("");  // Clear buffer
        g_message_output.clear();

        action_ = ActionRegistry::instance().lookup("Message");
        ASSERT_NE(nullptr, action_) << "Message action not registered";
    }

    Action* action_;
};

TEST_F(MessageActionTest, SingleArgument) {
    char* argv[] = {const_cast<char*>("Hello World")};

    int result = action_->execute(1, argv, 0, 0);

    EXPECT_EQ(0, result);
    std::string output = g_message_output.str();
    EXPECT_NE(std::string::npos, output.find("Hello World"));
}

TEST_F(MessageActionTest, MultipleArguments) {
    char* argv[] = {
        const_cast<char*>("Line 1"),
        const_cast<char*>("Line 2"),
        const_cast<char*>("Line 3")
    };

    int result = action_->execute(3, argv, 0, 0);

    EXPECT_EQ(0, result);
    std::string output = g_message_output.str();
    EXPECT_NE(std::string::npos, output.find("Line 1"));
    EXPECT_NE(std::string::npos, output.find("Line 2"));
    EXPECT_NE(std::string::npos, output.find("Line 3"));
}

TEST_F(MessageActionTest, NoArguments) {
    int result = action_->execute(0, nullptr, 0, 0);

    EXPECT_EQ(-1, result);
    std::string output = g_message_output.str();
    EXPECT_NE(std::string::npos, output.find("requires at least one argument"));
}

}} // namespace
```

**Key Points:**
- ✅ Mock Message() to avoid GUI dependency
- ✅ Test in complete isolation
- ✅ Verify behavior matches ActionMessage in action.c
- ✅ No linking against action.c needed

### Build Configuration

```makefile
# tests/cpp/Makefile.am (separate test binary)
check_PROGRAMS = action_tests

action_tests_SOURCES = \
    message_action_test.cpp \
    ../../../src/actions/MessageAction.cpp \
    ../../../src/actions/Action.cpp \
    ../../../src/actions/action_bridge.cpp \
    mock_pcb_functions.cpp  # Mocks for C functions

action_tests_CXXFLAGS = \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/src/actions \
    $(GTEST_CFLAGS) \
    -std=c++11

action_tests_LDADD = $(GTEST_LIBS) -lgtest_main
```

**No dependency on libpcb.a or action.c!**

---

## Layer 3: Integration with Fallback

### Rule: Dual Operation Period

Only after Layer 2 tests pass 100%, integrate with a fallback mechanism:

```c
// In action.c - TEMPORARY during migration
static int ActionMessage(int argc, char **argv, Coord x, Coord y) {
    // Try C++ implementation first
    int cpp_result = pcb_action_execute("Message", argc, argv, x, y);

    if (cpp_result != -1) {
        // C++ action handled it
        #ifdef DEBUG_MIGRATION
        Message("[DEBUG] MessageAction handled by C++ implementation\n");
        #endif
        return cpp_result;
    }

    // C++ action not available - use C fallback
    #ifdef DEBUG_MIGRATION
    Message("[DEBUG] MessageAction handled by C fallback\n");
    #endif

    // Original C implementation
    int i;
    if (argc < 1)
        AFAIL (message);

    for (i = 0; i < argc; i++) {
        Message (argv[i]);
        Message ("\n");
    }

    return 0;
}
```

**Key Points:**
- ✅ action.c still works if C++ fails
- ✅ Can enable/disable C++ with runtime flag
- ✅ Easy to compare C vs C++ behavior
- ✅ No risk of breaking existing functionality

---

## Dependency Resolution Framework

### Allowed Dependencies (Safe)

These C interfaces are stable and won't cause problems:

```c
// Tier 1: Always safe to call from C++
error.h         - Message(), MyFatal()
global.h        - Type definitions (PCBType, Coord, etc.)
data.h          - Data structure access (read-only)
const.h         - Constants and macros

// Tier 2: Safe if you don't modify state
misc.h          - Utility functions (GetValue, etc.)
pcb-printf.h    - Printf functions

// Tier 3: Use with caution
hid.h           - GUI callbacks (may have state)
undo.h          - Undo system (complex state)
```

### Forbidden Dependencies (Dangerous)

```c
action.h        - NEVER include (circular dependency)
action.c        - NEVER include (that's what we're replacing)
```

### Dependency Analysis Checklist

Before implementing an action, analyze its dependencies:

```bash
# Example: What does ActionChangeSize depend on?
grep -A 50 "^ActionChangeSize" src/action.c | \
    grep -E "^[[:space:]]+(Message|GetValue|SearchScreen|Change|Set|Increment)" | \
    sort -u

# For each function found:
# 1. Is it in a Tier 1/2/3 header? → OK
# 2. Is it static in action.c? → Need to extract or reimplement
# 3. Is it in another .c file? → Check if header exists
```

---

## Proof of Concept: MessageAction Migration

Let's validate the strategy with the simplest action:

### Phase 0: Analysis (✅ Done)

```
Action: ActionMessage
Location: action.c:1993
Dependencies:
  - Message() from error.h ✅ Tier 1, safe
  - AFAIL macro - need to handle
Complexity: TRIVIAL
Risk: MINIMAL
```

### Phase 1: Implement C++ Version

**File:** `src/actions/MessageAction.cpp`
**Status:** Ready to create
**Dependencies:** Only error.h
**Estimate:** 30 minutes

### Phase 2: Create Standalone Test

**File:** `tests/cpp/message_action_test.cpp`
**Mock:** Message() function
**Status:** Ready to create
**Estimate:** 1 hour

### Phase 3: Validate Behavior Match

Run both versions and compare:

```bash
# Test C version
pcb --action 'Message(Hello, World)'

# Test C++ version (once integrated)
PCB_USE_CPP_ACTIONS=1 pcb --action 'Message(Hello, World)'

# Compare outputs - should be identical
```

### Phase 4: Integration (Only if Phase 2 passes)

Add fallback to action.c
Run full test suite
Monitor for 1 week

### Phase 5: Cleanup (Only if Phase 4 stable)

Remove C fallback
Update documentation

---

## Validation Gate: Go/No-Go Criteria

Before moving to the next action, MessageAction must meet ALL criteria:

**Layer 2 (Standalone Testing):**
- [ ] Compiles without warnings
- [ ] All unit tests pass (100%)
- [ ] Valgrind shows no leaks
- [ ] Behavior matches C version exactly
- [ ] No dependency on action.c or action.h

**Layer 3 (Integration):**
- [ ] C fallback still works
- [ ] C++ version works in real PCB
- [ ] No performance regression (< 5% overhead)
- [ ] No crash reports for 1 week
- [ ] All existing integration tests pass

**Only proceed to next action if ALL boxes checked**

---

## Risk Mitigation: Escape Hatches

### Escape Hatch 1: Feature Flag

```c
// In main.c or config
bool use_cpp_actions = getenv("PCB_USE_CPP_ACTIONS") != NULL;

// In action.c fallback
if (use_cpp_actions) {
    result = pcb_action_execute(...);
    if (result != -1) return result;
}
// Fall through to C version
```

**Usage:**
```bash
# Force C version (safe mode)
PCB_USE_CPP_ACTIONS=0 pcb

# Use C++ version (testing)
PCB_USE_CPP_ACTIONS=1 pcb
```

### Escape Hatch 2: Per-Action Disable

```c
// actions_config.txt
Message=cpp        # Use C++ version
ChangeSize=c       # Use C version (broken in C++)
Select=disabled    # Disable entirely
```

### Escape Hatch 3: Git Rollback

Because we keep action.c untouched (except for fallback), rolling back is easy:

```bash
# Rollback just the fallback mechanism
git revert <commit-with-fallback>

# Action.c still works as before
```

---

## Dependency Extraction Strategy

Some actions will need helper functions from action.c. Here's how to handle them:

### Option 1: Extract to Shared Library

```c
// src/action_helpers.h (NEW FILE)
#ifdef __cplusplus
extern "C" {
#endif

FunctionID GetFunctionID(const char *str);
Coord GetValueEx(const char *val, const char *units, bool *absolute);

#ifdef __cplusplus
}
#endif
```

```c
// src/action_helpers.c (extracted from action.c)
#include "action_helpers.h"

FunctionID GetFunctionID(const char *str) {
    // Copy implementation from action.c
}
```

**Benefit:** Both C and C++ can use it

### Option 2: Reimplement in C++

```cpp
// src/actions/ActionHelpers.h
namespace pcb {
namespace actions {

class ActionHelpers {
public:
    static Coord parseSize(const char* value, const char* units, bool& absolute);
    // ...
};

}} // namespace
```

**Benefit:** Type-safe C++ version

### Option 3: Inline Simple Cases

For trivial helpers (< 5 lines), just inline them in the C++ action.

---

## Timeline for Proof of Concept

**Week 1: MessageAction (This is the critical test)**

- Day 1: Implement MessageAction.cpp
- Day 2: Create standalone test with mocks
- Day 3: Validate behavior match
- Day 4: Add fallback to action.c
- Day 5: Integration testing
- Days 6-7: Monitor, fix any issues

**Gate: If MessageAction works smoothly, continue**
**If problems arise, STOP and reassess strategy**

**Week 2: Second Action (TBD based on learnings)**

Only proceed if Week 1 was smooth.

---

## Success Metrics

### Process Metrics (Did we avoid the mess?)

- [ ] **Zero commits to fix broken tests** (proof we're isolated)
- [ ] **No linking errors** (proof dependencies are clean)
- [ ] **No circular dependency issues** (proof architecture works)
- [ ] **No "just one more patch" commits** (proof we're not in mess mode)

### Quality Metrics

- [ ] All tests pass first try
- [ ] No performance regression
- [ ] Code review approved with zero major issues
- [ ] Documentation complete

---

## Red Flags: Stop Immediately If...

🚩 You find yourself adding `extern` declarations to get things to compile
🚩 You're modifying action.c to expose internal functions
🚩 Tests require #ifdef hacks to compile
🚩 You're debugging linker errors for more than 30 minutes
🚩 The phrase "let me just patch this one thing" appears

**If you see any red flag → STOP → Reassess → Adjust strategy**

---

## Questions Before Starting

1. **Linking:** Will C++ actions be in a separate .so or linked into main binary?
2. **Mocking:** Do we have a mocking framework or need to write manual mocks?
3. **CI:** Can we run C++ tests in CI without the full PCB build?
4. **Rollback:** What's the rollback plan if this doesn't work?

---

## Conclusion

**The key to avoiding the previous mess:**

1. ✅ **Isolation** - C++ actions don't depend on action.c
2. ✅ **Validation** - Test standalone before integration
3. ✅ **Fallback** - C version always works
4. ✅ **Escape** - Multiple ways to disable/rollback
5. ✅ **Stop conditions** - Clear red flags to abort

**Start small, validate thoroughly, only proceed when confident.**

If MessageAction goes smoothly, we have a proven pattern.
If it doesn't, we learn and adjust before wasting more time.

---

**Next Step:** Review this strategy, then implement MessageAction as proof of concept.
