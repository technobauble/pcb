# C++ Tests for PCB

This directory contains C++ tests using Google Test framework.

## Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install libgtest-dev cmake
cd /usr/src/gtest
sudo cmake CMakeLists.txt
sudo make
sudo cp lib/*.a /usr/lib

# Fedora
sudo dnf install gtest-devel

# macOS
brew install googletest
```

## Building C++ Tests

```bash
./configure --enable-cpp-tests --disable-doc
make
```

## Running C++ Tests

```bash
# Run all C++ tests
cd tests/cpp
./unittest_cpp

# Run specific test suite
./unittest_cpp --gtest_filter=GeometryTest.*

# Run with verbose output
./unittest_cpp --gtest_filter=* --gtest_print_time=1

# List all tests
./unittest_cpp --gtest_list_tests
```

## Test Structure

- `main_test.cpp` - Test entry point (uses gtest_main)
- `*_test.cpp` - Test files for specific modules
- `*_wrapper.cpp` - RAII wrappers for C code

## Writing Tests

See [CPP_MIGRATION.md](../../doc/CPP_MIGRATION.md) for detailed guidance.

### Quick Example

```cpp
#include <gtest/gtest.h>

TEST(ModuleTest, FunctionName) {
  EXPECT_EQ(5, add(2, 3));
}
```

## Integration with C Code

C++ tests can call C code directly:

```cpp
extern "C" {
#include "pcb.h"
}

TEST(PCBTest, CreatePCB) {
  PCBType* pcb = create_pcb();
  EXPECT_NE(nullptr, pcb);
  free_pcb(pcb);
}
```

Or use RAII wrappers for automatic cleanup:

```cpp
#include "pcb_wrapper.hpp"

TEST(PCBTest, CreatePCB) {
  PCBWrapper pcb;
  EXPECT_NE(nullptr, pcb.get());
  // Automatic cleanup
}
```

## Current Test Coverage

| Module | C Tests (GLib) | C++ Tests (GTest) | Status |
|--------|----------------|-------------------|--------|
| pcb-printf | 2 | 0 | C only |
| object_list | 1 | 0 | C only |
| geometry | 0 | TBD | Migration target |

## Status

**Phase:** Proof of Concept
**Test Count:** 0 (infrastructure setup)
**Next Steps:**
1. Add Google Test to build system
2. Create first wrapper tests
3. Migrate geometry module

---

**Last Updated:** November 13, 2025
