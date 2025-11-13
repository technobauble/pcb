# PCB C++ Action System

## Overview

This directory contains the C++ infrastructure for the new PCB action system. This is part of a refactoring effort to break up the monolithic 8,427-line `action.c` file into modular, maintainable components.

## Status

**Phase 1: COMPLETE** ✅

The C++ build infrastructure is now in place and tested.

## Directory Structure

```
src/actions/
├── README.md              # This file
├── Action.h               # Base Action class and ActionRegistry
├── Action.cpp             # Implementation of Action and ActionRegistry
├── action_bridge.h        # C/C++ bridge interface
└── action_bridge.cpp      # C/C++ bridge implementation
```

## Architecture

### Core Components

#### 1. **Action Base Class** (`Action.h`, `Action.cpp`)

All actions inherit from the `Action` base class:

```cpp
class Action {
public:
    Action(const char* name, const char* help, const char* syntax);
    virtual int execute(int argc, char** argv, Coord x, Coord y) = 0;
    // ...
};
```

**Key features:**
- Pure virtual `execute()` method that subclasses must implement
- Auto-registration with ActionRegistry upon construction
- Helper methods: `arg()`, `hasArg()`
- Metadata: name, help text, syntax string

#### 2. **ActionRegistry** (`Action.h`, `Action.cpp`)

Singleton registry that maintains all registered actions:

```cpp
class ActionRegistry {
public:
    static ActionRegistry& instance();
    void registerAction(Action* action);
    Action* lookup(const std::string& name);
    std::vector<Action*> allActions() const;
    // ...
};
```

**Key features:**
- Singleton pattern - single global registry
- String-based lookup (no enums!)
- Actions register themselves via constructor
- Thread-safe (C++ static initialization)

#### 3. **C/C++ Bridge** (`action_bridge.h`, `action_bridge.cpp`)

C-compatible interface for calling C++ actions from existing C code:

```c
int pcb_action_execute(const char* name, int argc, char** argv, Coord x, Coord y);
void pcb_action_init(void);
int pcb_action_count(void);
int pcb_action_exists(const char* name);
void pcb_action_list_all(void);
```

**Key features:**
- `extern "C"` linkage for C compatibility
- Exception handling (catches C++ exceptions, returns error codes)
- Backwards compatibility with HID interface

## How to Create a New Action

### Example: Simple Action

```cpp
// src/actions/MyActions.cpp
#include "Action.h"

namespace pcb {
namespace actions {

class HelloAction : public Action {
public:
    HelloAction()
        : Action("Hello",
                 "Prints a greeting message",
                 "Hello(name)")
    {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        const char* name = arg(0, argv);
        if (!name || !name[0]) {
            name = "World";
        }

        printf("Hello, %s! (x=%d, y=%d)\n", name, x, y);
        return 0;
    }
};

// Static instance - auto-registers on startup
REGISTER_ACTION(HelloAction);

}} // namespace pcb::actions
```

### Steps:

1. **Create a new `.cpp` file** in `src/actions/`
2. **Include `Action.h`** and any PCB headers you need
3. **Define your action class** inheriting from `pcb::actions::Action`
4. **Implement `execute()`** with your action logic
5. **Use `REGISTER_ACTION(YourClass)`** to auto-register
6. **Add to `Makefile.am`** in the `PCB_CXX_SRCS` variable
7. **Rebuild** and your action is available!

## Calling Actions

### From C Code

```c
#include "actions/action_bridge.h"

// Execute an action
char* argv[] = {"arg1", "arg2"};
int result = pcb_action_execute("Hello", 2, argv, 100, 200);

if (result == -1) {
    // Action not found
}
```

### From C++ Code

```cpp
#include "actions/Action.h"

// Direct lookup and execute
Action* action = ActionRegistry::instance().lookup("Hello");
if (action) {
    char* argv[] = {"arg1", "arg2"};
    int result = action->execute(2, argv, 100, 200);
}
```

## Build System Integration

### Files Modified

- **`src/Makefile.am`**: Added `PCB_CXX_SRCS` variable and `AM_CXXFLAGS`
- **`configure.ac`**: Already had `AC_PROG_CXX` (C++ support was pre-configured!)

### Compiler Flags

```makefile
AM_CXXFLAGS = -std=c++11 -Wall -Wextra
```

**C++11 features used:**
- `auto` keyword
- `nullptr`
- `override` keyword
- `default`/`delete` constructors
- Range-based for loops
- Lambda functions (for auto-registration)
- `std::map`, `std::vector`, `std::string`

### Mixed C/C++ Compilation

The build system automatically:
- Compiles `.c` files with `gcc`
- Compiles `.cpp` files with `g++`
- Links everything together with the C++ linker

## Testing

A standalone test has verified:

✅ Action class compiles correctly
✅ ActionRegistry singleton works
✅ Auto-registration works
✅ C++ virtual functions work
✅ C/C++ bridge (extern "C") works
✅ Action lookup and execution works
✅ Error handling works

## Migration Plan

### Phase 1: Infrastructure Setup ✅ COMPLETE

- [x] Create `src/actions/` directory
- [x] Implement `Action.h` and `Action.cpp`
- [x] Implement `action_bridge.h` and `action_bridge.cpp`
- [x] Update `src/Makefile.am`
- [x] Test compilation

### Phase 2: Extract First Module (NEXT)

Target: **SelectionActions** (smallest, simplest module)

Actions to extract:
- `ActionSelect` → `SelectAction`
- `ActionUnselect` → `UnselectAction`
- `ActionRemoveSelected` → `RemoveSelectedAction`

**Steps:**
1. Create `src/actions/SelectionActions.cpp`
2. Implement the 3 action classes
3. Test selection functionality
4. Update `action.c` to call C++ actions first, fall back to C
5. Gradually remove C implementations

### Phase 3: Extract Remaining Modules

Order (increasing complexity):
1. SelectionActions ✅ (Phase 2)
2. BufferActions
3. FlagActions
4. ThermalActions
5. TransformActions
6. SystemActions
7. FileActions
8. LayoutActions
9. RoutingActions
10. PropertyActions (largest)
11. DisplayActions (most complex)

### Phase 4: Delete action.c

Once all actions are migrated:
- Remove `src/action.c`
- Remove FunctionID enum
- Update documentation

## Design Decisions

### Why C++ Over C?

**Advantages:**
1. **Classes & inheritance** - Natural fit for action polymorphism
2. **Virtual functions** - Replace enum-based dispatch
3. **Namespaces** - Avoid global namespace pollution
4. **std::map** - Efficient action lookup without hand-rolled hash tables
5. **Better tools** - IDEs, debuggers, static analyzers
6. **Type safety** - Compile-time error checking

### Why Not Full C++ Rewrite?

**Conservative approach:**
- Only new action system uses C++
- Existing C code unchanged
- Easy to rollback if needed
- Gradual adoption as team learns C++

### Why Auto-Registration?

**Benefits:**
- No central registration file to maintain
- Add new action = just add new .cpp file
- Can't forget to register (happens automatically)
- Follows RAII principle

**Pattern:**
```cpp
static MyAction g_my_action_instance;  // Registers on startup
```

## Performance

C++ features we use have **negligible overhead**:

| Feature | Overhead | Notes |
|---------|----------|-------|
| Virtual functions | ~1 CPU cycle | Same as C function pointer |
| Classes | 0 | Just data + functions |
| std::map | O(log n) | Better than linear search |
| Namespaces | 0 | Compile-time only |

Action execution speed is identical to current C code.

## Dependencies

### Required:
- **g++ 4.8+** (C++11 support)
- **autotools** (already required)
- **Standard C++ library** (libstdc++)

### Optional:
- None (all standard libraries)

## Compatibility

- ✅ **Linux**: Tested with g++ 13.3.0
- ✅ **C/C++ interop**: Tested and working
- ✅ **Existing HIDs**: C bridge provides compatibility
- ⚠️ **Windows**: Should work, but not yet tested
- ⚠️ **macOS**: Should work, but not yet tested

## Troubleshooting

### "undefined reference to vtable"

**Cause:** Pure virtual function not implemented

**Solution:** Ensure all virtual functions are implemented in subclass

### "multiple definition of g_MyAction_instance"

**Cause:** REGISTER_ACTION used in header file

**Solution:** Only use REGISTER_ACTION in .cpp files

### "action not found" at runtime

**Cause:** Action not registered (static initialization issue)

**Solution:** Ensure action .cpp file is linked into binary

## Future Enhancements

**Possible improvements:**
1. **TargetSelector** framework (eliminate FunctionID enum completely)
2. **ActionContext** object (instead of argc/argv/x/y)
3. **Async actions** (for long-running operations)
4. **Action decorators** (logging, undo, timing, etc.)
5. **Plugin system** (load actions from shared libraries)
6. **Unit testing** (Google Test framework)

## Resources

- **Original discussion**: See refactoring plan document
- **C++ style guide**: Follow existing PCB coding conventions
- **Action.c original**: `src/action.c` (8,427 lines - being replaced)

## Questions?

If you have questions about this infrastructure, refer to:
1. This README
2. Comments in `Action.h` and `action_bridge.h`
3. The standalone test (demonstrates usage patterns)

---

**Last Updated:** 2025-11-13
**Status:** Phase 1 Complete, Ready for Phase 2
**Next Action:** Extract SelectionActions module
