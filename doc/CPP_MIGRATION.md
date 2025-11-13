# C++ Migration Guide for PCB

**Document Version:** 1.0
**Date:** November 13, 2025
**Status:** Active Development

---

## Table of Contents

1. [Overview](#overview)
2. [Dual Test Infrastructure Setup](#dual-test-infrastructure-setup)
3. [Migration Strategy](#migration-strategy)
4. [C/C++ Interop Patterns](#cc-interop-patterns)
5. [Build System Configuration](#build-system-configuration)
6. [Testing Approaches](#testing-approaches)
7. [Best Practices](#best-practices)
8. [Migration Roadmap](#migration-roadmap)

---

## Overview

PCB is gradually migrating from C to C++ to improve:
- **Testability** - Easier fixtures, RAII, dependency injection
- **Type safety** - Strong typing, templates, compile-time checks
- **Memory safety** - Smart pointers, RAII automatic cleanup
- **Maintainability** - Classes, encapsulation, better abstractions

**Migration Philosophy:**
- ✅ Incremental - Module by module, not all at once
- ✅ Coexistence - C and C++ code work together
- ✅ Backward compatible - Existing functionality always works
- ✅ Test-driven - Add tests before and after migration
- ✅ Low-risk - Can pause or rollback at any point

---

## Dual Test Infrastructure Setup

We maintain **two parallel test frameworks** during migration:

```
tests/
├── c/
│   ├── main-test.c          # GLib g_test (existing)
│   └── unittest             # C test binary
├── cpp/
│   ├── main_test.cpp        # Google Test (new)
│   └── unittest_cpp         # C++ test binary
└── integration/
    └── run_tests.sh         # Shared integration tests (language-agnostic)
```

### Framework Comparison

| Aspect | C (GLib g_test) | C++ (Google Test) |
|--------|----------------|-------------------|
| **Registration** | Manual via `g_test_add_func()` | Automatic via `TEST()` macro |
| **Fixtures** | Manual setup/teardown functions | Class-based with RAII |
| **Assertions** | `g_assert_cmpint()` | `EXPECT_EQ()`, `ASSERT_EQ()` |
| **Mocking** | Not available | Google Mock built-in |
| **Memory cleanup** | Manual | Automatic (RAII) |
| **Maturity** | Established (379 tests) | Growing (new modules) |

### When to Use Which Framework

**Use C tests (GLib g_test) for:**
- Existing C code that isn't being migrated yet
- Testing legacy modules
- Code with heavy global state dependencies
- Quick bug fixes to existing code

**Use C++ tests (Google Test) for:**
- New modules written in C++
- Refactored modules with reduced global state
- Code requiring mocking/dependency injection
- Performance-critical code needing benchmarks

---

## Migration Strategy

### Phase-Based Approach

#### Phase 1: Dual Infrastructure Setup (Week 1-2)

**Goal:** Get both test frameworks working side-by-side

**Tasks:**
- [ ] Add Google Test dependency to build system
- [ ] Create `tests/cpp/` directory structure
- [ ] Configure `make check` to run both C and C++ tests
- [ ] Add CI jobs for C++ tests
- [ ] Document C++ testing guidelines

**Deliverables:**
- Both test suites run in CI
- Developers can choose which framework to use
- No disruption to existing tests

#### Phase 2: Wrapper Pattern (Week 3-6)

**Goal:** Make existing C code testable from C++

**Approach:**
```cpp
// Wrap C structs in C++ classes for RAII
class Line {
public:
  Line(Coord x1, Coord y1, Coord x2, Coord y2) {
    line_ = create_line_c(x1, y1, x2, y2);  // C function
  }

  ~Line() {
    free_line_c(line_);  // Automatic cleanup
  }

  LineType* get() { return line_; }

private:
  LineType* line_;
};

// Test using C++ wrapper
TEST(LineTest, Create) {
  Line line(0, 0, 100, 100);
  EXPECT_NE(nullptr, line.get());
}
```

**Benefits:**
- Test C code with C++ tools
- No changes to C code required
- Gradual migration path

#### Phase 3: Incremental Refactoring (Month 2-6)

**Goal:** Convert modules from C to C++ one at a time

**Priority Order:**
1. **Geometry/Math modules** - Fewest dependencies, pure functions
2. **Data structures** - Lists, trees, hash tables
3. **String utilities** - Parsing, formatting
4. **DRC algorithms** - Complex logic, needs good tests
5. **GUI code** - Last, most complex dependencies

**Per-Module Checklist:**
- [ ] Write comprehensive C tests for current behavior
- [ ] Create C++ wrapper classes
- [ ] Write C++ tests alongside C tests
- [ ] Refactor C code to C++ incrementally
- [ ] Verify both test suites pass
- [ ] Remove C tests once migration complete
- [ ] Update documentation

#### Phase 4: Global State Elimination (Month 6-12)

**Goal:** Replace global state with dependency injection

**Approach:**
```cpp
// Before (C global state)
extern Settings Settings;

void scale_dimension(Coord value) {
  return value * Settings.grid_scale;
}

// After (C++ dependency injection)
class GridService {
public:
  explicit GridService(const GridSettings& settings)
    : settings_(settings) {}

  Coord scale_dimension(Coord value) const {
    return value * settings_.grid_scale;
  }

private:
  const GridSettings& settings_;
};
```

---

## C/C++ Interop Patterns

### Pattern 1: RAII Wrappers for C Resources

**Problem:** C code allocates resources that must be manually freed

**Solution:** C++ RAII wrapper

```cpp
// pcb_wrapper.h
class PCBWrapper {
public:
  PCBWrapper() : pcb_(create_pcb()) {
    if (!pcb_) throw std::runtime_error("Failed to create PCB");
  }

  ~PCBWrapper() {
    if (pcb_) free_pcb(pcb_);
  }

  // Disable copying to prevent double-free
  PCBWrapper(const PCBWrapper&) = delete;
  PCBWrapper& operator=(const PCBWrapper&) = delete;

  // Enable moving
  PCBWrapper(PCBWrapper&& other) noexcept
    : pcb_(other.pcb_) {
    other.pcb_ = nullptr;
  }

  PCBWrapper& operator=(PCBWrapper&& other) noexcept {
    if (this != &other) {
      if (pcb_) free_pcb(pcb_);
      pcb_ = other.pcb_;
      other.pcb_ = nullptr;
    }
    return *this;
  }

  PCBType* get() { return pcb_; }
  const PCBType* get() const { return pcb_; }

private:
  PCBType* pcb_;
};

// Usage in test
TEST(PCBTest, Create) {
  PCBWrapper pcb;
  EXPECT_NE(nullptr, pcb.get());
  // Automatic cleanup - no manual free needed
}
```

### Pattern 2: Extern "C" for C++ Code Called from C

**Problem:** Need to call C++ code from C

**Solution:** Extern "C" interface

```cpp
// geometry.h
#ifdef __cplusplus
extern "C" {
#endif

Coord calculate_distance(Coord x1, Coord y1, Coord x2, Coord y2);

#ifdef __cplusplus
}
#endif

// geometry.cpp
#include "geometry.h"
#include <cmath>

extern "C" Coord calculate_distance(Coord x1, Coord y1, Coord x2, Coord y2) {
  // C++ implementation can use modern features
  Coord dx = x2 - x1;
  Coord dy = y2 - y1;
  return std::sqrt(dx * dx + dy * dy);
}
```

### Pattern 3: Opaque Pointer Pattern

**Problem:** Hide C++ implementation from C code

**Solution:** Opaque pointer (pimpl idiom)

```cpp
// geometry_service.h (C-compatible)
#ifdef __cplusplus
extern "C" {
#endif

typedef struct GeometryService GeometryService;

GeometryService* geometry_service_create(void);
void geometry_service_destroy(GeometryService* service);
Coord geometry_service_distance(GeometryService* service,
                                 Coord x1, Coord y1, Coord x2, Coord y2);

#ifdef __cplusplus
}
#endif

// geometry_service.cpp
class GeometryService {
public:
  Coord distance(Coord x1, Coord y1, Coord x2, Coord y2) {
    // C++ implementation with modern features
    return std::hypot(x2 - x1, y2 - y1);
  }
};

extern "C" {
  GeometryService* geometry_service_create() {
    return new GeometryService();
  }

  void geometry_service_destroy(GeometryService* service) {
    delete service;
  }

  Coord geometry_service_distance(GeometryService* service,
                                   Coord x1, Coord y1, Coord x2, Coord y2) {
    return service->distance(x1, y1, x2, y2);
  }
}
```

### Pattern 4: Smart Pointer Helpers

**Problem:** C code returns raw pointers that need automatic cleanup

**Solution:** Custom deleters with smart pointers

```cpp
// smart_pcb.h
#include <memory>

// Custom deleter for C types
struct PCBDeleter {
  void operator()(PCBType* pcb) const {
    if (pcb) free_pcb(pcb);
  }
};

struct LineDeleter {
  void operator()(LineType* line) const {
    if (line) free_line(line);
  }
};

// Type aliases for convenience
using UniquePCB = std::unique_ptr<PCBType, PCBDeleter>;
using UniqueLine = std::unique_ptr<LineType, LineDeleter>;

// Helper factory functions
inline UniquePCB make_pcb() {
  return UniquePCB(create_pcb());
}

inline UniqueLine make_line(Coord x1, Coord y1, Coord x2, Coord y2) {
  return UniqueLine(create_line(x1, y1, x2, y2));
}

// Usage in tests
TEST(SmartPointerTest, AutomaticCleanup) {
  auto pcb = make_pcb();
  auto line = make_line(0, 0, 100, 100);

  EXPECT_NE(nullptr, pcb.get());
  EXPECT_NE(nullptr, line.get());

  // Automatic cleanup when test ends
}
```

---

## Build System Configuration

### Configure.ac Changes

```bash
# Check for C++ compiler
AC_PROG_CXX
AC_LANG_PUSH([C++])
AX_CXX_COMPILE_STDCXX([17], [noext], [optional])
AC_LANG_POP([C++])

# Check for Google Test (optional - don't fail if missing)
PKG_CHECK_MODULES([GTEST], [gtest >= 1.10.0], [have_gtest=yes], [have_gtest=no])
AM_CONDITIONAL([HAVE_GTEST], [test "x$have_gtest" = "xyes"])

if test "x$have_gtest" = "xyes"; then
  AC_MSG_NOTICE([Google Test found - C++ tests will be built])
else
  AC_MSG_NOTICE([Google Test not found - C++ tests will be skipped])
  AC_MSG_NOTICE([Install with: sudo apt-get install libgtest-dev])
fi
```

### Src/Makefile.am Changes

```makefile
# C tests (existing)
if BUILD_WITH_GLIB
check_PROGRAMS = unittest

unittest_SOURCES = \
  main-test.c \
  object_list.c \
  pcb-printf.c

unittest_CFLAGS = -DPCB_UNIT_TEST $(GLIB_CFLAGS)
unittest_LDADD = $(GLIB_LIBS)

TESTS = unittest
endif

# C++ tests (new)
if HAVE_GTEST
check_PROGRAMS += unittest_cpp

unittest_cpp_SOURCES = \
  tests/cpp/main_test.cpp \
  tests/cpp/geometry_test.cpp \
  tests/cpp/pcb_wrapper.cpp

unittest_cpp_CXXFLAGS = \
  -DPCB_UNIT_TEST \
  $(GTEST_CFLAGS) \
  -std=c++17

unittest_cpp_LDADD = \
  $(GTEST_LIBS) \
  -lgtest_main \
  -lpthread

TESTS += unittest_cpp
endif

# Both test types contribute to coverage
if ENABLE_COVERAGE
unittest_CFLAGS += $(COVERAGE_CFLAGS)
unittest_LDFLAGS = $(COVERAGE_LDFLAGS)

if HAVE_GTEST
unittest_cpp_CXXFLAGS += $(COVERAGE_CFLAGS)
unittest_cpp_LDFLAGS = $(COVERAGE_LDFLAGS)
endif
endif
```

### Make Check Target

```makefile
# Run all tests
check-local:
	@echo "=== Running C tests (GLib) ==="
	@if test -f ./unittest; then \
		./unittest; \
	else \
		echo "C tests not built"; \
	fi
	@echo ""
	@echo "=== Running C++ tests (Google Test) ==="
	@if test -f ./unittest_cpp; then \
		./unittest_cpp; \
	else \
		echo "C++ tests not built (Google Test not available)"; \
	fi
```

---

## Testing Approaches

### Approach 1: Pure C++ Tests for New Code

```cpp
// tests/cpp/geometry_test.cpp
#include <gtest/gtest.h>
#include "geometry.hpp"

namespace pcb {
namespace test {

TEST(GeometryTest, CalculateDistance) {
  Point p1{0, 0};
  Point p2{3, 4};

  EXPECT_EQ(5, calculate_distance(p1, p2));
}

class GeometryFixture : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup before each test
  }

  void TearDown() override {
    // Cleanup after each test
  }
};

TEST_F(GeometryFixture, LineIntersection) {
  Line line1{{0, 0}, {10, 10}};
  Line line2{{0, 10}, {10, 0}};

  auto intersection = find_intersection(line1, line2);

  ASSERT_TRUE(intersection.has_value());
  EXPECT_EQ(5, intersection->x);
  EXPECT_EQ(5, intersection->y);
}

} // namespace test
} // namespace pcb
```

### Approach 2: C++ Wrappers Testing C Code

```cpp
// tests/cpp/legacy_pcb_test.cpp
#include <gtest/gtest.h>
#include "pcb_wrapper.hpp"

extern "C" {
#include "pcb.h"
#include "create.h"
}

TEST(LegacyPCBTest, CreateAndDestroy) {
  PCBWrapper pcb;

  EXPECT_NE(nullptr, pcb.get());
  EXPECT_GT(pcb.get()->MaxWidth, 0);
  EXPECT_GT(pcb.get()->MaxHeight, 0);

  // Automatic cleanup via RAII
}

TEST(LegacyPCBTest, AddLine) {
  PCBWrapper pcb;

  // Use C functions wrapped in RAII
  LineWrapper line = pcb.create_line(0, 0, 100, 100, 10);

  EXPECT_EQ(0, line.get()->Point1.X);
  EXPECT_EQ(100, line.get()->Point2.X);
}
```

### Approach 3: Parameterized Tests

```cpp
// tests/cpp/coordinate_test.cpp
#include <gtest/gtest.h>

struct CoordTestCase {
  Coord input;
  Coord scale;
  Coord expected;
};

class CoordinateScaleTest : public ::testing::TestWithParam<CoordTestCase> {};

TEST_P(CoordinateScaleTest, Scale) {
  const auto& test_case = GetParam();

  Coord result = scale_coordinate(test_case.input, test_case.scale);

  EXPECT_EQ(test_case.expected, result);
}

INSTANTIATE_TEST_SUITE_P(
  ScaleTests,
  CoordinateScaleTest,
  ::testing::Values(
    CoordTestCase{100, 2.0, 200},
    CoordTestCase{100, 0.5, 50},
    CoordTestCase{100, 1.0, 100},
    CoordTestCase{100, 0.0, 0},
    CoordTestCase{0, 2.0, 0}
  )
);
```

### Approach 4: Death Tests

```cpp
// tests/cpp/safety_test.cpp
#include <gtest/gtest.h>

TEST(SafetyTest, NullPointerCheck) {
  // Test that function properly handles NULL
  EXPECT_DEATH({
    process_pcb(nullptr);
  }, "Assertion.*failed");
}

TEST(SafetyTest, InvalidInput) {
  EXPECT_THROW({
    parse_coordinate("invalid");
  }, std::invalid_argument);
}
```

---

## Best Practices

### Naming Conventions

**C++ files:**
- Headers: `.hpp` (C++ only) or `.h` (C-compatible)
- Source: `.cpp`
- Tests: `_test.cpp` suffix

**Namespaces:**
```cpp
namespace pcb {
  // All PCB code in pcb namespace

  namespace geometry {
    // Geometric functions
  }

  namespace test {
    // Test helpers
  }
}
```

### Header Guards

```cpp
// Use #pragma once for C++ headers
#pragma once

#include <memory>
#include <string>

namespace pcb {
// ...
}
```

### C/C++ Compatible Headers

```cpp
// For headers that must work in both C and C++
#ifndef PCB_GEOMETRY_H
#define PCB_GEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

// C-compatible declarations
typedef struct Point {
  Coord x;
  Coord y;
} Point;

Coord calculate_distance(Point p1, Point p2);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
// C++-only additions
namespace pcb {
  inline Coord distance(const Point& p1, const Point& p2) {
    return calculate_distance(p1, p2);
  }
}
#endif

#endif // PCB_GEOMETRY_H
```

### Type Safety

```cpp
// Use enum class instead of #define
enum class Layer : uint8_t {
  Top,
  Bottom,
  Inner1,
  Inner2
};

// Use strongly-typed coordinates
struct Coord {
  int32_t value;

  explicit Coord(int32_t v) : value(v) {}

  // Prevent implicit conversions
  explicit operator int32_t() const { return value; }
};
```

### Error Handling

```cpp
// C code: Returns NULL or -1 on error
PCBType* create_pcb() {
  // ...
  if (error) return NULL;
}

// C++ code: Uses exceptions or std::optional
std::unique_ptr<PCB> create_pcb() {
  // ...
  if (error) throw std::runtime_error("Failed to create PCB");
}

std::optional<PCB> try_create_pcb() {
  // ...
  if (error) return std::nullopt;
  return pcb;
}
```

---

## Migration Roadmap

### Module-by-Module Priority

#### High Priority (Month 1-3)

**1. Geometry/Math Utilities**
- Pure functions, minimal dependencies
- High test value, complex logic
- Current: `rotate.c`, `mirror.c`, `polygon1.c`

**2. Data Structures**
- Lists, trees, hash tables
- Easy to wrap in C++ containers
- Current: `object_list.c`, `heap.c`

**3. String Utilities**
- Parsing, formatting, conversion
- Perfect for C++ std::string
- Current: `pcb-printf.c`, `parse_l.l`

#### Medium Priority (Month 4-6)

**4. Coordinate System**
- Reduce global state
- Dependency injection opportunity
- Current: Unit conversion tables

**5. DRC Algorithms**
- Complex logic needs good tests
- Benefits from classes/encapsulation
- Current: `find.c`, `thermal.c`

#### Low Priority (Month 7-12)

**6. GUI Code**
- Most complex dependencies
- Can stay in C longer
- Current: `hid/gtk/*`

**7. File I/O**
- Works fine as-is
- Lower test value
- Current: `file.c`, `parse_y.y`

### Success Metrics

Track migration progress:

```bash
# Count C++ vs C test functions
cpp_tests=$(grep -r "^TEST" tests/cpp/ | wc -l)
c_tests=$(grep -r "g_test_add_func" src/*.c | wc -l)

# Track lines of C++ vs C code
cpp_lines=$(find src -name "*.cpp" -o -name "*.hpp" | xargs wc -l | tail -1)
c_lines=$(find src -name "*.c" -o -name "*.h" | xargs wc -l | tail -1)

# Target: 50% C++ within 6 months
```

---

## Continuous Integration

### GitHub Actions Updates

```yaml
# .github/workflows/build.yml

jobs:
  build-cpp:
    name: Build and Test C++
    runs-on: ubuntu-latest

    steps:
    - uses: actions/checkout@v4

    - name: Install C++ dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y \
          g++ \
          libgtest-dev \
          cmake

        # Build and install Google Test if needed
        cd /usr/src/gtest
        sudo cmake CMakeLists.txt
        sudo make
        sudo cp lib/*.a /usr/lib

    - name: Configure with C++ tests
      run: |
        ./autogen.sh
        ./configure --enable-cpp-tests --disable-doc

    - name: Build C++ tests
      run: make -j$(nproc)

    - name: Run C++ tests
      run: |
        cd src
        ./unittest_cpp --gtest_output=xml:cpp_test_results.xml

    - name: Upload C++ test results
      uses: actions/upload-artifact@v4
      with:
        name: cpp-test-results
        path: src/cpp_test_results.xml
```

---

## Summary

This dual infrastructure approach allows PCB to:

✅ **Maintain stability** - Existing C code and tests unchanged
✅ **Gradual migration** - One module at a time, low risk
✅ **Modern testing** - C++ tests for new/refactored code
✅ **Team flexibility** - Developers choose C or C++ per task
✅ **Continuous delivery** - Both frameworks work in parallel

**Next Steps:**
1. Review and approve this migration strategy
2. Set up Google Test in build system
3. Create first C++ wrapper tests for existing code
4. Migrate geometry module as proof-of-concept
5. Refine approach based on learnings

---

**Last Updated:** November 13, 2025
**See Also:**
- [TESTING.md](TESTING.md) - Current C testing guide
- [WRITING_UNIT_TESTS.md](WRITING_UNIT_TESTS.md) - C unit test guide
- [TESTING_INFRASTRUCTURE.md](TESTING_INFRASTRUCTURE.md) - Overall testing strategy
