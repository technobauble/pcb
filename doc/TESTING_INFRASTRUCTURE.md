# PCB Testing Infrastructure - Review & Modernization Proposal

**Document Version:** 1.0
**Date:** November 13, 2025
**Status:** Proposal - Awaiting Implementation

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Current State Analysis](#current-state-analysis)
3. [Identified Gaps & Weaknesses](#identified-gaps--weaknesses)
4. [Proposed Improvements](#proposed-improvements)
5. [Implementation Roadmap](#implementation-roadmap)
6. [Success Metrics](#success-metrics)
7. [Recommendations](#recommendations)
8. [Implementation Todo List](#implementation-todo-list)
9. [References](#references)

---

## Executive Summary

PCB has a solid foundation of **379 integration tests** covering export formats and DRC functionality, but has significant gaps in **unit testing** (only 3 tests), **code coverage tracking**, and **modern testing practices**. This proposal outlines a phased approach to modernize the testing infrastructure while maintaining backward compatibility.

### Key Findings

- ✅ **Strong:** 379 comprehensive integration tests with golden file methodology
- ✅ **Strong:** Active CI/CD pipeline with GitHub Actions
- ❌ **Weak:** Only 3 unit tests for ~77,000 lines of C code (<0.01% coverage)
- ❌ **Weak:** No code coverage tracking or reporting
- ❌ **Weak:** No memory safety testing (Valgrind, ASan)
- ❌ **Weak:** No fuzzing for parser/file format robustness

### Recommendation

Pursue a **phased modernization approach** that:
1. Adds visibility (coverage tracking, metrics)
2. Expands unit testing systematically
3. Integrates modern testing tools (fuzzing, ASan, benchmarks)
4. Maintains the strong integration test foundation

**Estimated Timeline:** 3-6 months for core improvements
**Expected Impact:** 50%+ code coverage, 200+ unit tests, significantly improved code quality

---

## Current State Analysis

### Test Infrastructure Overview

```
pcb/
├── tests/                          # Integration tests (379 tests)
│   ├── run_tests.sh               # Shell-based test runner
│   ├── tests.list                 # Test definitions
│   ├── inputs/                    # Test input files (.pcb, .script)
│   └── golden/                    # Reference output files
├── src/
│   ├── main-test.c                # Unit test entry point
│   ├── object_list.c              # 1 unit test
│   └── pcb-printf.c               # 2 unit tests
└── .github/workflows/build.yml    # CI/CD pipeline
```

### Strengths

#### 1. Comprehensive Integration Tests (379 tests)

**Location:** `tests/run_tests.sh`, `tests/tests.list`

**Coverage Areas:**
- **Export HIDs:** Gerber (RS-274X), BOM, Centroid (XY), PNG, PostScript, G-Code, Nelma, GSVIT, IPC-D-356
- **DRC Checks:** Minimum size, clearance, polygon clearance for all object types
- **Actions:** MinMaskGap, ChangeClearSize, FileVersions, RouteStyles
- **Multiple test scenarios:** 10+ tests per export format with different options

**Test Methodology:**
```bash
# Example test flow
1. PCB exports test layout to format (e.g., Gerber)
2. Output compared against golden reference file
3. Comparison method varies by format:
   - ASCII: diff with normalization (dates, authors)
   - Images: ImageMagick perceptual comparison
   - Gerber: Convert to PNG via gerbv, then compare
   - PostScript: Validate syntax, count pages
```

**Documentation:** Well-documented in `tests/README.txt`

**Example Test Entry:**
```
# tests/tests.list format:
# test_name | layout files | HID | options | mismatch | output files
hid_gerber1 | gerber_oneline.pcb | gerber | | | \
  gbx:gerber_oneline.bottom.gbr \
  gbx:gerber_oneline.top.gbr \
  cnc:gerber_oneline.plated-drill.cnc
```

#### 2. CI/CD Infrastructure

**File:** `.github/workflows/build.yml`

**Test Configurations:**
- GTK GUI build
- GTK + OpenGL build
- Batch (headless) build

**Quality Checks:**
- cppcheck static analysis
- Legacy pattern detection (obsolete `register` keyword)
- CVS artifact detection

**Artifact Collection:**
- Build logs
- Test logs (`test-suite.log`, `unittest.log`)
- Test error output (`run_tests.err.log`)

**Good Error Reporting:**
```yaml
# Extracts failed test details
if grep -q "FAILED TESTS:" test.log; then
  echo "### Failed Tests:" >> $GITHUB_STEP_SUMMARY
  sed -n '/FAILED TESTS:/,/^------/p' test.log >> $GITHUB_STEP_SUMMARY
fi
```

#### 3. Basic Unit Testing Framework

**Framework:** GLib's `g_test` (standard for GNOME projects)

**Current Tests:**
```c
// src/main-test.c
int main(int argc, char *argv[]) {
  initialize_units();
  pcb_printf_register_tests();      // 2 tests
  object_list_register_tests();     // 1 test

  g_test_init(&argc, &argv, NULL);
  return g_test_run();
}
```

**Test Example:**
```c
// src/object_list.c
#ifdef PCB_UNIT_TEST
static void object_list_test(void) {
  /* Test list operations */
  g_assert_cmpint(result, ==, expected);
}

void object_list_register_tests(void) {
  g_test_add_func("/object-list/test", object_list_test);
}
#endif
```

**Build Integration:**
```makefile
# src/Makefile.am
unittest_CPPFLAGS = -I$(top_srcdir) -DPCB_UNIT_TEST
unittest_SOURCES = ${TEST_SRCS}
check_PROGRAMS = unittest
TESTS = unittest
```

---

## Identified Gaps & Weaknesses

### 1. Minimal Unit Test Coverage

**Problem:** Only **3 unit tests** for ~77,000 lines of C code

**Impact:**
- Core algorithms untested (geometry, DRC, routing)
- Refactoring is risky (no safety net)
- Bugs found late (integration tests only)
- Difficult to test edge cases

**Affected Modules** (untested):
- `src/intersect.c` - Geometry intersection algorithms
- `src/find.c` - DRC core logic
- `src/autoroute.c` - Routing algorithms
- `src/buffer.c` - Buffer operations
- `src/file.c` - File I/O and parsing
- 100+ other source files

### 2. No Code Coverage Tracking

**Problem:** No visibility into what code is tested

**Missing Infrastructure:**
- No `--enable-coverage` configure flag
- No gcov/lcov integration
- No coverage reports in CI
- No coverage badges or dashboards

**Impact:**
- Can't identify untested code paths
- Can't track improvement over time
- Can't set coverage requirements for PRs

**Example of What's Missing:**
```bash
# Should be able to do:
./configure --enable-coverage
make check
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
# View coverage_html/index.html

# Currently: Not supported
```

### 3. Limited Static Analysis

**Current:** Only basic cppcheck configured

**Missing Tools:**
- **clang-tidy:** Modern C/C++ linter with hundreds of checks
- **scan-build:** Clang static analyzer (finds bugs, memory leaks)
- **CodeQL:** GitHub's security vulnerability scanner
- **clang-format:** Consistent code formatting
- **include-what-you-use:** Header dependency management

**Impact:**
- Miss opportunities to find bugs early
- No security-focused scanning
- Code style inconsistencies
- Modern C++ issues not caught

### 4. No Memory Safety Testing

**Problem:** No automated memory leak/corruption detection

**Missing Tools:**
- **Valgrind:** Memory leak detection, invalid access
- **AddressSanitizer (ASan):** Buffer overflows, use-after-free
- **MemorySanitizer (MSan):** Uninitialized memory reads
- **UndefinedBehaviorSanitizer (UBSan):** Undefined behavior detection

**Impact:**
- Memory leaks can accumulate
- Buffer overflows undetected
- Undefined behavior may cause crashes on different platforms

**Example Issues That Would Be Caught:**
```c
// Memory leak
char *buf = malloc(100);
if (error) return;  // Leaked!
free(buf);

// Buffer overflow
char buf[10];
strcpy(buf, long_string);  // Overflow!

// Use after free
free(ptr);
*ptr = 5;  // Crash!
```

### 5. Platform Coverage Limitations

**Current:** Only Linux CI testing

**Missing:**
- macOS builds/tests
- Windows builds/tests
- Multiple Linux distributions
- Different library versions

**Impact:**
- Platform-specific bugs not caught until users report
- Cross-platform regressions possible
- Build system portability issues

### 6. Test Fragility Issues

**Problem 1: Pixel-Perfect Image Comparison**
```bash
# Current approach - brittle
compare image1.png image2.png null:
# Fails if antialiasing differs by 1 pixel!
```

**Problem 2: X11 Dependency**
```bash
# Action tests require X display
${XHOST} >/dev/null 2>&1
if [ $? -ne 0 ]; then
  # Test skipped in headless environments
fi
```

**Impact:**
- Tests fail due to ImageMagick version differences
- Can't run full test suite in Docker/CI without X11
- False positives waste developer time

### 7. Missing Modern Practices

**No Fuzzing:**
- Parser robustness unknown (PCB file format, Gerber, action scripts)
- Potential for crashes on malformed input

**No Benchmarking:**
- Performance regressions not tracked
- No data on optimization effectiveness

**No C++ Testing:**
- Recent C++ infrastructure not tested
- C/C++ interop issues possible

**No Concurrent Testing:**
- No tests for thread safety
- Multi-threaded code paths not verified

---

## Proposed Improvements

### Phase 1: Foundation (Immediate - 2-4 weeks)

**Goal:** Add visibility and essential safety nets with minimal effort

#### 1.1 Add Code Coverage Tracking

**Implementation Steps:**

1. **Add configure flag:**
```bash
# configure.ac
AC_ARG_ENABLE([coverage],
  [AS_HELP_STRING([--enable-coverage], [Enable code coverage tracking])],
  [enable_coverage=$enableval], [enable_coverage=no])

if test "x$enable_coverage" = "xyes"; then
  COVERAGE_CFLAGS="--coverage -fprofile-arcs -ftest-coverage"
  COVERAGE_LDFLAGS="--coverage"
  AC_SUBST(COVERAGE_CFLAGS)
  AC_SUBST(COVERAGE_LDFLAGS)
fi
```

2. **Update Makefile.am:**
```makefile
# src/Makefile.am
if ENABLE_COVERAGE
  AM_CFLAGS += $(COVERAGE_CFLAGS)
  AM_LDFLAGS += $(COVERAGE_LDFLAGS)
endif
```

3. **Add CI job:**
```yaml
# .github/workflows/build.yml
- name: Generate coverage report
  run: |
    ./configure --enable-coverage --disable-doc
    make -j$(nproc)
    make check
    lcov --capture --directory . --output-file coverage.info
    lcov --remove coverage.info '/usr/*' --output-file coverage.info
    lcov --list coverage.info

- name: Upload to Codecov
  uses: codecov/codecov-action@v3
  with:
    files: ./coverage.info
    fail_ci_if_error: false
```

4. **Add badge to README:**
```markdown
[![codecov](https://codecov.io/gh/technobauble/pcb/branch/master/graph/badge.svg)](https://codecov.io/gh/technobauble/pcb)
```

**Expected Impact:**
- Visibility into test coverage (likely <5% initially)
- Identify completely untested modules
- Track coverage improvement over time
- Foundation for coverage-based development

**Effort:** 1 week
**Priority:** High

---

#### 1.2 Enhance Static Analysis

**Add New Tools:**

**1. clang-tidy (Modern C/C++ Linter):**
```yaml
# .github/workflows/build.yml
- name: Run clang-tidy
  run: |
    clang-tidy src/*.c \
      -checks='*,-llvm-*,-fuchsia-*,-google-*,-android-*' \
      -warnings-as-errors='' \
      -- -I. -Isrc $(pkg-config --cflags glib-2.0 gtk+-2.0)
```

**2. scan-build (Clang Static Analyzer):**
```yaml
- name: Run scan-build
  run: |
    scan-build --status-bugs \
      --exclude gts/ \
      ./configure --disable-doc
    scan-build --status-bugs -o scan-results make -j$(nproc)
```

**3. CodeQL (Security Scanning):**
```yaml
# .github/workflows/codeql.yml
name: "CodeQL"
on:
  push:
    branches: [ master ]
  pull_request:
    branches: [ master ]

jobs:
  analyze:
    runs-on: ubuntu-latest
    permissions:
      security-events: write
    steps:
      - uses: actions/checkout@v4
      - uses: github/codeql-action/init@v2
        with:
          languages: cpp
      - run: |
          ./autogen.sh
          ./configure --disable-doc
          make -j$(nproc)
      - uses: github/codeql-action/analyze@v2
```

**Expected Impact:**
- Find bugs, security issues, code smells
- Enforce modern C/C++ best practices
- Catch potential vulnerabilities early

**Effort:** 1 week
**Priority:** High

---

#### 1.3 Add Memory Safety Testing

**1. Valgrind Integration:**

```bash
# Add valgrind test target
# tests/Makefile.am
check-valgrind:
	$(MAKE) check TESTS_ENVIRONMENT="valgrind \
		--leak-check=full \
		--show-leak-kinds=all \
		--error-exitcode=1 \
		--suppressions=$(srcdir)/valgrind.supp"
```

```yaml
# .github/workflows/build.yml
- name: Run tests with Valgrind
  run: |
    sudo apt-get install -y valgrind
    cd tests
    make check-valgrind
```

**2. AddressSanitizer (ASan):**

```yaml
- name: Build with AddressSanitizer
  run: |
    ./configure CFLAGS="-fsanitize=address -g -O1" \
                LDFLAGS="-fsanitize=address" \
                --disable-doc
    make -j$(nproc)
    make check
```

**3. Create Suppressions File:**
```
# tests/valgrind.supp
# Suppress known issues in external libraries
{
   glib_type_system_init
   Memcheck:Leak
   ...
   fun:g_type_init_with_debug_flags
}
```

**Expected Impact:**
- Catch memory leaks immediately
- Find buffer overflows, use-after-free bugs
- Prevent security vulnerabilities
- Improve stability

**Effort:** 1 week
**Priority:** Medium

---

### Phase 2: Expand Test Coverage (2-3 months)

**Goal:** Systematically add unit tests for critical code paths

#### 2.1 Systematic Unit Testing

**Target Modules** (prioritized by criticality):

**Priority 1: Core Geometry (Critical - 15-20 tests)**
- `src/intersect.c` - Line/arc intersection algorithms
- `src/line.c` - Line manipulation
- `src/polygon.c` - Polygon operations

**Example Tests:**
```c
// src/intersect.c
#ifdef PCB_UNIT_TEST

static void test_line_line_intersection_parallel(void) {
  // Parallel lines should not intersect
  LineType line1 = {0, 0, 100, 0, ...};
  LineType line2 = {0, 10, 100, 10, ...};

  g_assert_false(line_intersects_line(&line1, &line2));
}

static void test_line_line_intersection_perpendicular(void) {
  // Perpendicular lines should intersect
  LineType line1 = {50, 0, 50, 100, ...};
  LineType line2 = {0, 50, 100, 50, ...};
  PointType intersection;

  g_assert_true(line_intersects_line_at(&line1, &line2, &intersection));
  g_assert_cmpint(intersection.X, ==, 50);
  g_assert_cmpint(intersection.Y, ==, 50);
}

static void test_line_arc_intersection(void) {
  // Line through arc center
  LineType line = {...};
  ArcType arc = {...};

  int count = line_arc_intersections(&line, &arc, points);
  g_assert_cmpint(count, ==, 2);  // Should intersect at 2 points
}

void intersect_register_tests(void) {
  g_test_add_func("/intersect/line-line/parallel",
                  test_line_line_intersection_parallel);
  g_test_add_func("/intersect/line-line/perpendicular",
                  test_line_line_intersection_perpendicular);
  g_test_add_func("/intersect/line-arc",
                  test_line_arc_intersection);
}
#endif
```

**Priority 2: DRC Engine (Critical - 20-30 tests)**
- `src/find.c` - DRC core logic
- `libdrc/` - DRC rules

**Example Tests:**
```c
// Test DRC clearance checking
static void test_drc_line_clearance_pass(void) {
  // Two lines with sufficient clearance
  LineType line1 = create_line(0, 0, 100, 0, 10);
  LineType line2 = create_line(0, 30, 100, 30, 10);

  DRCViolation *violations = check_line_clearance(&line1, &line2, 5);
  g_assert_null(violations);  // No violations expected
}

static void test_drc_line_clearance_fail(void) {
  // Two lines too close
  LineType line1 = create_line(0, 0, 100, 0, 10);
  LineType line2 = create_line(0, 12, 100, 12, 10);

  DRCViolation *violations = check_line_clearance(&line1, &line2, 5);
  g_assert_nonnull(violations);
  g_assert_cmpstr(violations->type, ==, "clearance");
}
```

**Priority 3: File I/O (High - 10-15 tests)**
- `src/file.c` - PCB file loading/saving
- `src/parse_l.l`, `src/parse_y.y` - Parser

**Example Tests:**
```c
static void test_parse_simple_pcb(void) {
  const char *pcb_content =
    "PCB[\"Test Board\" 600000 500000]\n"
    "Layer(1 \"top\")\n"
    "(\n"
    "  Line[10000 10000 20000 20000 1000 2000 \"clearline\"]\n"
    ")\n";

  FILE *f = tmpfile();
  fputs(pcb_content, f);
  rewind(f);

  PCBType *pcb = parse_pcb_from_file(f);
  g_assert_nonnull(pcb);
  g_assert_cmpstr(pcb->Name, ==, "Test Board");
  g_assert_cmpint(pcb->MaxWidth, ==, 600000);

  fclose(f);
  free_pcb(pcb);
}

static void test_parse_invalid_pcb_syntax(void) {
  const char *invalid = "INVALID SYNTAX[[[";

  FILE *f = tmpfile();
  fputs(invalid, f);
  rewind(f);

  PCBType *pcb = parse_pcb_from_file(f);
  g_assert_null(pcb);  // Should fail gracefully

  fclose(f);
}
```

**Priority 4: Buffer Operations (Medium - 10 tests)**
- `src/buffer.c` - Copy/paste operations

**Priority 5: Routing Algorithms (Medium - 15-20 tests)**
- `src/autoroute.c` - Autorouting
- `src/djopt.c` - Djikstra optimization

**Test Template Structure:**
```c
// Standard test file structure
#ifdef PCB_UNIT_TEST
#include <glib.h>

/* Helper functions */
static TestObject create_test_object(...) {
  // Factory for test objects
}

/* Test cases */
static void test_feature_normal_case(void) { }
static void test_feature_edge_case(void) { }
static void test_feature_error_handling(void) { }

/* Registration */
void module_register_tests(void) {
  g_test_add_func("/module/feature/normal", test_feature_normal_case);
  g_test_add_func("/module/feature/edge", test_feature_edge_case);
  g_test_add_func("/module/feature/error", test_feature_error_handling);
}
#endif
```

**Target:** 200+ unit tests, 30% code coverage

**Effort:** 6-8 weeks
**Priority:** High

---

#### 2.2 C++ Compatibility Testing

**Context:** Recent PR added C++ action system infrastructure

**Add Google Test Framework:**

```bash
# Download and build gtest
git submodule add https://github.com/google/googletest.git tests/googletest

# configure.ac
AC_ARG_ENABLE([gtest],
  [AS_HELP_STRING([--enable-gtest], [Enable Google Test framework])],
  [enable_gtest=$enableval], [enable_gtest=no])

AM_CONDITIONAL([ENABLE_GTEST], [test "x$enable_gtest" = "xyes"])
```

**Create C++ Test Suite:**
```cpp
// tests/cpp/test_action_system.cpp
#include <gtest/gtest.h>
#include "action.h"

TEST(ActionSystem, RegisterAction) {
  // Test C++ action registration
  bool result = HID_Action::Register("TestAction", test_callback);
  EXPECT_TRUE(result);
}

TEST(ActionSystem, InvokeAction) {
  // Test action invocation from C++
  int result = HID_Action::Invoke("TestAction", 0, nullptr, nullptr);
  EXPECT_EQ(result, 0);
}

TEST(ActionSystem, CInterop) {
  // Test calling C actions from C++
  // and vice versa
}
```

**Build Integration:**
```makefile
# tests/Makefile.am
if ENABLE_GTEST
  check_PROGRAMS += cpp_unittest
  cpp_unittest_SOURCES = cpp/test_action_system.cpp
  cpp_unittest_LDADD = googletest/build/lib/libgtest.a
  TESTS += cpp_unittest
endif
```

**Expected Impact:**
- Ensure C/C++ interop works correctly
- Test new C++ infrastructure
- Enable future C++ development

**Effort:** 2 weeks
**Priority:** High

---

#### 2.3 Property-Based Testing (Optional)

**Concept:** Generate random inputs, check properties hold

**Use Hypothesis for C or DeepState:**

```c
// Example: Test DRC commutativity
TEST_PROPERTY(drc_order_independence) {
  // Generate random layout
  PCBType *pcb = GENERATE_RANDOM_PCB();

  // Run DRC in two different orders
  DRCViolationList *v1 = run_drc_forward(pcb);
  DRCViolationList *v2 = run_drc_backward(pcb);

  // Results should be the same
  ASSERT_EQUAL_VIOLATIONS(v1, v2);
}
```

**Properties to Test:**
- Commutativity (order independence)
- Idempotence (f(f(x)) == f(x))
- Inverse operations (save/load roundtrip)
- Invariants (e.g., clearances always positive)

**Expected Impact:**
- Find edge cases that manual tests miss
- Increase confidence in core algorithms

**Effort:** 2 weeks
**Priority:** Low

---

### Phase 3: Advanced Testing (3-6 months)

**Goal:** Add sophisticated testing for robustness and performance

#### 3.1 Fuzzing Infrastructure

**LibFuzzer Integration:**

```c
// tests/fuzz/fuzz_pcb_parser.c
#include <stdint.h>
#include <stddef.h>
#include "parse.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  // Write data to temp file
  FILE *f = tmpfile();
  fwrite(data, 1, size, f);
  rewind(f);

  // Try to parse (should not crash)
  PCBType *pcb = parse_pcb_from_file(f);
  if (pcb) {
    free_pcb(pcb);
  }

  fclose(f);
  return 0;
}
```

**Build Fuzzer:**
```bash
# Build with libFuzzer
clang -fsanitize=fuzzer,address -g -O1 \
  fuzz_pcb_parser.c src/*.c -o fuzz_pcb_parser

# Run fuzzer
./fuzz_pcb_parser -max_total_time=3600 corpus/
```

**Fuzz Targets:**
1. PCB file parser (`src/parse_l.l`, `src/parse_y.y`)
2. Gerber export (`src/hid/gerber/`)
3. Action script parsing
4. Font file parsing (`src/font.c`)
5. Netlist import

**OSS-Fuzz Integration:**
```yaml
# .github/workflows/ossfuzz.yml
# Continuous fuzzing via Google's infrastructure
- name: Build for OSS-Fuzz
  run: |
    export CC=clang
    export CXX=clang++
    export CFLAGS="-fsanitize=fuzzer-no-link"
    ./compile_fuzzers.sh
```

**Expected Impact:**
- Find parser crashes and hangs
- Improve robustness against malformed files
- Security hardening

**Effort:** 3-4 weeks
**Priority:** Medium

---

#### 3.2 Performance Benchmarking

**Google Benchmark Integration:**

```cpp
// tests/bench/bench_autoroute.cpp
#include <benchmark/benchmark.h>
#include "autoroute.h"

static void BM_AutorouteSimpleBoard(benchmark::State& state) {
  PCBType *pcb = load_test_layout("simple_2layer.pcb");

  for (auto _ : state) {
    NetListType *netlist = get_netlist(pcb);
    autoroute(pcb, netlist);
  }

  state.SetItemsProcessed(state.iterations());
  free_pcb(pcb);
}
BENCHMARK(BM_AutorouteSimpleBoard);

static void BM_DRCCheck(benchmark::State& state) {
  PCBType *pcb = load_test_layout("complex.pcb");

  for (auto _ : state) {
    DRCViolationList *violations = run_drc(pcb);
    free_violations(violations);
  }

  free_pcb(pcb);
}
BENCHMARK(BM_DRCCheck);

BENCHMARK_MAIN();
```

**CI Integration:**
```yaml
- name: Run benchmarks
  run: |
    make benchmarks
    ./bench_autoroute --benchmark_format=json > bench_results.json

- name: Compare performance
  run: |
    # Compare against baseline
    python3 tools/compare_benchmarks.py \
      bench_results.json baseline.json
    # Fail if regression > 10%
```

**Track Metrics:**
- Autorouting time
- DRC check duration
- File load/save time
- Export generation time

**Expected Impact:**
- Prevent performance regressions
- Identify optimization opportunities
- Track improvement over time

**Effort:** 2-3 weeks
**Priority:** Low

---

#### 3.3 Cross-Platform Testing

**Add macOS and Windows CI:**

```yaml
# .github/workflows/build.yml
strategy:
  fail-fast: false
  matrix:
    os: [ubuntu-latest, macos-latest, windows-latest]
    config:
      - name: "GTK GUI"
        configure_flags: "--with-gui=gtk --disable-doc"
      - name: "Batch"
        configure_flags: "--with-gui=batch --disable-doc"
    exclude:
      # OpenGL not working on Windows CI yet
      - os: windows-latest
        config: {name: "GTK + OpenGL"}

runs-on: ${{ matrix.os }}

steps:
  - name: Install dependencies (macOS)
    if: runner.os == 'macOS'
    run: |
      brew install autoconf automake gtk+ gtkglext gd gerbv

  - name: Install dependencies (Windows)
    if: runner.os == 'Windows'
    run: |
      choco install msys2
      # Install dependencies via MSYS2
```

**Docker Multi-Distro Testing:**
```yaml
# Test on multiple Linux distributions
strategy:
  matrix:
    distro: [ubuntu:20.04, ubuntu:22.04, debian:11, fedora:38]

- name: Test on ${{ matrix.distro }}
  run: |
    docker run --rm -v $PWD:/pcb ${{ matrix.distro }} \
      /bin/sh -c "cd /pcb && ./run_tests_in_container.sh"
```

**Expected Impact:**
- Catch platform-specific bugs early
- Ensure portability
- Build system robustness

**Effort:** 2 weeks
**Priority:** Medium

---

#### 3.4 Integration Test Improvements

**Problem 1: Brittle Image Comparison**

**Solution: Perceptual Diff**
```bash
# Use PSNR (Peak Signal-to-Noise Ratio) instead of exact match
compare -metric PSNR ref.png out.png null: 2>&1 | \
  awk '{if($1 > 30){exit 0}else{exit 1}}'
# PSNR > 30dB is visually identical
```

**Solution: Structural Similarity**
```bash
# Use SSIM (Structural Similarity Index)
compare -metric SSIM ref.png out.png null: 2>&1 | \
  awk '{if($1 > 0.95){exit 0}else{exit 1}}'
# SSIM > 0.95 is very similar
```

**Problem 2: X11 Dependency**

**Solution: Virtual Framebuffer**
```bash
# Install Xvfb (X Virtual Framebuffer)
sudo apt-get install xvfb

# Wrap tests
xvfb-run -a ./run_tests.sh
```

**Update CI:**
```yaml
- name: Run tests with virtual display
  run: |
    sudo apt-get install -y xvfb
    xvfb-run -a make check
```

**Problem 3: Golden File Maintenance**

**Solution: Snapshot Testing**
```bash
# Instead of comparing pixels, compare structure
gerbv --export=svg input.gbr -o output.svg
xmllint --xpath '//svg/g' output.svg > structure.txt
diff structure.txt golden/structure.txt
```

**Expected Impact:**
- More reliable tests across environments
- Fewer false positives
- Tests run in Docker/CI without X11

**Effort:** 3-4 weeks
**Priority:** Medium

---

### Phase 4: Quality Infrastructure (Ongoing)

**Goal:** Maintain high quality standards continuously

#### 4.1 Test Quality Metrics

**Track Metrics:**
```yaml
# .github/workflows/metrics.yml
- name: Collect test metrics
  run: |
    echo "## Test Metrics" >> $GITHUB_STEP_SUMMARY
    echo "" >> $GITHUB_STEP_SUMMARY

    # Code coverage
    COVERAGE=$(lcov --summary coverage.info | grep lines | awk '{print $2}')
    echo "- Code Coverage: $COVERAGE" >> $GITHUB_STEP_SUMMARY

    # Test count
    TEST_COUNT=$(./unittest --list | wc -l)
    echo "- Unit Tests: $TEST_COUNT" >> $GITHUB_STEP_SUMMARY

    # Integration tests
    INT_TESTS=$(grep -c "^[^#]" tests/tests.list)
    echo "- Integration Tests: $INT_TESTS" >> $GITHUB_STEP_SUMMARY

    # Test execution time
    echo "- Test Duration: ${TEST_TIME}s" >> $GITHUB_STEP_SUMMARY
```

**Dashboards:**
- Codecov for coverage trends
- GitHub Actions for test results
- Custom dashboard for benchmarks

**Quality Gates:**
```yaml
- name: Check quality gates
  run: |
    # Fail if coverage drops
    if [ $NEW_COVERAGE -lt $OLD_COVERAGE ]; then
      echo "::error::Coverage decreased from $OLD_COVERAGE to $NEW_COVERAGE"
      exit 1
    fi

    # Fail if tests too slow (> 10 min)
    if [ $TEST_TIME -gt 600 ]; then
      echo "::warning::Tests took ${TEST_TIME}s (> 10min)"
    fi
```

**Effort:** 1 week
**Priority:** Low

---

#### 4.2 Testing Documentation

**Create Testing Guide:**

File: `doc/TESTING.md`

```markdown
# PCB Testing Guide

## Overview

PCB uses two types of tests:
- **Unit Tests:** Test individual functions/modules (GLib g_test)
- **Integration Tests:** Test full workflows (shell scripts)

## Running Tests

### All Tests
```bash
make check
```

### Unit Tests Only
```bash
cd src && ./unittest
```

### Specific Integration Test
```bash
cd tests && ./run_tests.sh hid_gerber1
```

## Writing Unit Tests

### Example

```c
// In src/mymodule.c
#ifdef PCB_UNIT_TEST
#include <glib.h>

static void test_my_function(void) {
  int result = my_function(5);
  g_assert_cmpint(result, ==, 10);
}

void mymodule_register_tests(void) {
  g_test_add_func("/mymodule/my-function", test_my_function);
}
#endif
```

### Register in main-test.c

```c
// In src/main-test.c
mymodule_register_tests();  // Add this line
```

## Writing Integration Tests

See `tests/README.txt` for detailed instructions.

## Code Coverage

### Generate Coverage Report
```bash
./configure --enable-coverage
make check
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
firefox coverage_html/index.html
```

## Memory Testing

### Valgrind
```bash
cd tests
make check-valgrind
```

### AddressSanitizer
```bash
./configure CFLAGS="-fsanitize=address -g" LDFLAGS="-fsanitize=address"
make check
```

## Continuous Integration

See `.github/workflows/build.yml` for CI configuration.
```

**Effort:** 2 weeks
**Priority:** Medium

---

#### 4.3 Pre-commit Hooks

**Create `.pre-commit-config.yaml`:**

```yaml
repos:
  - repo: local
    hooks:
      - id: unit-tests
        name: Run unit tests
        entry: make -C src unittest && src/unittest
        language: system
        pass_filenames: false
        stages: [commit]

      - id: code-format
        name: Check code formatting
        entry: clang-format --dry-run --Werror
        language: system
        files: \.(c|h|cpp|hpp)$

      - id: trailing-whitespace
        name: Remove trailing whitespace
        entry: sed -i 's/[[:space:]]*$//'
        language: system
        files: \.(c|h|cpp|hpp|md)$
```

**Installation:**
```bash
pip install pre-commit
pre-commit install
```

**Expected Impact:**
- Catch issues before commit
- Enforce code quality standards
- Reduce CI failures

**Effort:** 1 week
**Priority:** Low

---

## Implementation Roadmap

### Summary Table

| Phase | Timeline | Effort | Priority | Tasks |
|-------|----------|--------|----------|-------|
| **Phase 1: Foundation** | 2-4 weeks | Low | **High** | 3 tasks |
| Coverage tracking | 1 week | Low | High | 1 task |
| Enhanced static analysis | 1 week | Low | High | 1 task |
| Memory safety (Valgrind/ASan) | 1 week | Low | Medium | 1 task |
| **Phase 2: Expand Coverage** | 2-3 months | Medium | **High** | 3 tasks |
| 200+ unit tests | 6-8 weeks | High | High | 5 modules |
| C++ compatibility tests | 2 weeks | Medium | High | 1 task |
| Property-based testing | 2 weeks | Medium | Low | 1 task |
| **Phase 3: Advanced** | 3-6 months | High | Medium | 4 tasks |
| Fuzzing infrastructure | 3-4 weeks | Medium | Medium | 5 fuzz targets |
| Performance benchmarks | 2-3 weeks | Medium | Low | 1 task |
| Cross-platform CI | 2 weeks | Low | Medium | 1 task |
| Integration test improvements | 3-4 weeks | Medium | Medium | 1 task |
| **Phase 4: Quality** | Ongoing | Low | Medium | 3 tasks |
| Metrics dashboard | 1 week | Low | Low | 1 task |
| Documentation | 2 weeks | Low | Medium | 1 task |
| Pre-commit hooks | 1 week | Low | Low | 1 task |

---

### Detailed Timeline

```
Month 1-2: Phase 1 Foundation
├── Week 1: Coverage tracking
│   ├── Add configure flag
│   ├── Update Makefiles
│   ├── Add CI job
│   └── Set up Codecov
├── Week 2: Static analysis
│   ├── Add clang-tidy
│   ├── Add scan-build
│   ├── Add CodeQL
│   └── Create suppressions
├── Week 3: Memory safety
│   ├── Valgrind integration
│   ├── ASan CI job
│   └── Create suppressions
└── Week 4: Documentation & refinement

Month 3-5: Phase 2 Expand Coverage
├── Month 3: Core geometry tests (20 tests)
│   ├── intersect.c
│   ├── line.c
│   └── polygon.c
├── Month 4: DRC and file I/O (30 tests)
│   ├── find.c (DRC)
│   ├── file.c
│   └── parse_l.l/parse_y.y
└── Month 5: Buffer, routing, C++ (50 tests)
    ├── buffer.c
    ├── autoroute.c
    └── C++ action tests

Month 6-8: Phase 3 Advanced (Optional)
├── Month 6: Fuzzing
│   ├── PCB parser fuzzer
│   ├── Gerber fuzzer
│   ├── Action script fuzzer
│   └── OSS-Fuzz setup
├── Month 7: Performance & cross-platform
│   ├── Benchmark suite
│   ├── macOS CI
│   └── Windows CI
└── Month 8: Test improvements
    ├── Perceptual image diff
    ├── Xvfb integration
    └── Snapshot testing

Ongoing: Phase 4 Quality
└── Continuous maintenance
    ├── Update metrics dashboard
    ├── Expand documentation
    └── Refine pre-commit hooks
```

---

## Success Metrics

### Short-term (3 months)

**Coverage:**
- ✅ Code coverage tracking enabled
- ✅ Coverage reports in CI
- ✅ Coverage badge on README
- ✅ Coverage > 20%

**Tests:**
- ✅ 100+ unit tests added
- ✅ Core geometry tested
- ✅ DRC basics tested

**Quality:**
- ✅ ASan/Valgrind integrated
- ✅ No memory leaks in test suite
- ✅ clang-tidy running in CI

### Medium-term (6 months)

**Coverage:**
- ✅ Coverage > 30%
- ✅ All critical paths covered

**Tests:**
- ✅ 200+ unit tests
- ✅ C++ compatibility tested
- ✅ Property-based tests for key algorithms

**Quality:**
- ✅ Fuzzing finds 0 new crashes
- ✅ Performance benchmarks tracking
- ✅ Cross-platform CI (Linux/macOS)

### Long-term (12 months)

**Coverage:**
- ✅ Coverage > 50% on core modules
- ✅ Coverage > 30% overall

**Tests:**
- ✅ 300+ unit tests
- ✅ All modules have at least basic tests
- ✅ Comprehensive integration tests

**Quality:**
- ✅ Zero memory leaks
- ✅ OSS-Fuzz integrated
- ✅ Windows CI working
- ✅ Performance regression detection

**Process:**
- ✅ Testing guide completed
- ✅ Pre-commit hooks in use
- ✅ Quality gates enforced

---

## Recommendations

### Immediate Actions (Next 2 Weeks)

**Priority 1: Enable Code Coverage Tracking**
- Add `--enable-coverage` configure flag
- Integrate with Codecov
- Add coverage badge to README
- **Impact:** Visibility into testing gaps
- **Effort:** 1 week

**Priority 2: Add AddressSanitizer CI Job**
- Catches memory bugs immediately
- Low effort, high value
- **Impact:** Find memory issues early
- **Effort:** 1 day

**Priority 3: Start Unit Testing Critical Modules**
- Begin with `src/intersect.c` (geometry is testable)
- Aim for 10-15 tests as proof of concept
- **Impact:** Safety net for refactoring
- **Effort:** 1 week

### Quick Wins

- ✅ **Valgrind in CI** - 1 day, finds memory leaks
- ✅ **clang-tidy** - 2 days, modern linting
- ✅ **Coverage reporting** - 3 days, visibility into gaps
- ✅ **xvfb-run for X11 tests** - 1 day, fixes headless CI issues

### Best Practices

**DO:**
- ✅ Start with high-value, low-effort items (Phase 1)
- ✅ Test critical paths first (geometry, DRC)
- ✅ Make coverage visible but not blocking initially
- ✅ Document testing process
- ✅ Celebrate wins (show coverage improvements)

**DON'T:**
- ❌ Rewrite existing integration tests (they work well)
- ❌ Require 80%+ coverage immediately (unrealistic)
- ❌ Abandon golden file approach (appropriate for this domain)
- ❌ Block PRs on coverage before reaching 20% baseline
- ❌ Skip memory safety testing (critical for C code)

### Risk Mitigation

**Risk: Coverage reveals large gaps**
- Mitigation: Set realistic incremental goals (5% per month)

**Risk: Unit tests find bugs in production code**
- Mitigation: This is good! Fix bugs as found, don't block on 100% pass rate initially

**Risk: CI becomes too slow**
- Mitigation: Parallelize tests, use test sharding, cache dependencies

**Risk: False positives from static analysis**
- Mitigation: Create suppressions file, tune checks gradually

---

## Implementation Todo List

### Phase 1: Foundation (Weeks 1-4)

#### Week 1: Coverage Tracking
- [ ] Add `--enable-coverage` to configure.ac
- [ ] Update src/Makefile.am with coverage flags
- [ ] Create .codecov.yml configuration
- [ ] Add coverage job to .github/workflows/build.yml
- [ ] Install lcov in CI
- [ ] Generate coverage report in CI
- [ ] Upload to Codecov
- [ ] Add coverage badge to README.md
- [ ] Document coverage usage in doc/TESTING.md
- [ ] Run initial coverage baseline (expect <5%)

#### Week 2: Enhanced Static Analysis
- [ ] Add clang-tidy to .github/workflows/build.yml
- [ ] Create .clang-tidy configuration file
- [ ] Add scan-build job to CI
- [ ] Create .github/workflows/codeql.yml
- [ ] Configure CodeQL for C/C++
- [ ] Create suppressions for false positives
- [ ] Document static analysis in doc/TESTING.md
- [ ] Review and triage initial findings
- [ ] Set up automated issue creation for critical findings

#### Week 3: Memory Safety Testing
- [ ] Add Valgrind to test dependencies
- [ ] Create tests/valgrind.supp suppressions file
- [ ] Add check-valgrind target to tests/Makefile.am
- [ ] Add Valgrind job to CI
- [ ] Add ASan build configuration
- [ ] Add ASan job to CI
- [ ] Test with ASan on unit tests
- [ ] Test with ASan on integration tests
- [ ] Document memory testing in doc/TESTING.md
- [ ] Fix any memory leaks found

#### Week 4: Documentation and Refinement
- [ ] Create doc/TESTING.md
- [ ] Document test writing process
- [ ] Document coverage usage
- [ ] Document memory testing
- [ ] Add examples for unit tests
- [ ] Add examples for integration tests
- [ ] Update CONTRIBUTING.md with testing requirements
- [ ] Create test writing checklist
- [ ] Review and refine CI configuration
- [ ] Announce new testing infrastructure to team

### Phase 2: Expand Coverage (Weeks 5-20)

#### Weeks 5-8: Core Geometry Tests (Target: 20 tests)
- [ ] Add test infrastructure to src/intersect.c
- [ ] Test line-line intersection (5 tests)
  - [ ] Parallel lines (no intersection)
  - [ ] Perpendicular lines
  - [ ] Overlapping lines
  - [ ] Touching endpoints
  - [ ] Skew lines
- [ ] Test line-arc intersection (5 tests)
  - [ ] Line through center
  - [ ] Line tangent to arc
  - [ ] Line missing arc
  - [ ] Line through endpoints
  - [ ] Line parallel to arc chord
- [ ] Test arc-arc intersection (5 tests)
- [ ] Test polygon operations (5 tests)
- [ ] Register all tests in main-test.c
- [ ] Run coverage report (target: 10%)
- [ ] Document geometry test patterns

#### Weeks 9-12: DRC Engine Tests (Target: 30 tests)
- [ ] Add test infrastructure to src/find.c
- [ ] Test line clearance checking (8 tests)
  - [ ] Sufficient clearance (pass)
  - [ ] Insufficient clearance (fail)
  - [ ] Touching (fail)
  - [ ] Different layers (pass)
  - [ ] Parallel lines
  - [ ] Perpendicular lines
  - [ ] Angled lines
  - [ ] Zero-length lines (edge case)
- [ ] Test pad clearance checking (8 tests)
- [ ] Test via clearance checking (6 tests)
- [ ] Test polygon clearance (8 tests)
- [ ] Test minimum size checks (5 tests)
- [ ] Register all tests in main-test.c
- [ ] Run coverage report (target: 15%)

#### Weeks 13-16: File I/O Tests (Target: 15 tests)
- [ ] Add test infrastructure to src/file.c
- [ ] Test simple PCB parsing (3 tests)
- [ ] Test complex PCB parsing (3 tests)
- [ ] Test invalid syntax handling (3 tests)
- [ ] Test save/load roundtrip (3 tests)
- [ ] Test backward compatibility (3 tests)
- [ ] Create test fixture PCB files
- [ ] Register all tests in main-test.c
- [ ] Run coverage report (target: 20%)

#### Weeks 17-18: Buffer Operations (Target: 10 tests)
- [ ] Add test infrastructure to src/buffer.c
- [ ] Test buffer add operations (3 tests)
- [ ] Test buffer remove operations (3 tests)
- [ ] Test buffer clear (2 tests)
- [ ] Test buffer copy (2 tests)
- [ ] Register all tests in main-test.c

#### Weeks 19-20: C++ Compatibility Tests (Target: 10 tests)
- [ ] Add Google Test as submodule
- [ ] Configure Google Test build
- [ ] Create tests/cpp/ directory
- [ ] Create test_action_system.cpp
- [ ] Test C++ action registration (3 tests)
- [ ] Test C/C++ interop (4 tests)
- [ ] Test HID_Action class (3 tests)
- [ ] Add cpp_unittest to Makefile
- [ ] Run coverage report (target: 25%)

#### Week 20: Phase 2 Review
- [ ] Run full test suite
- [ ] Review coverage report (target: 30%)
- [ ] Document new tests
- [ ] Update TESTING.md
- [ ] Create Phase 3 detailed plan

### Phase 3: Advanced Testing (Weeks 21-40) - Optional

#### Weeks 21-24: Fuzzing Infrastructure
- [ ] Install libFuzzer/AFL
- [ ] Create tests/fuzz/ directory
- [ ] Create fuzz_pcb_parser.c
- [ ] Create fuzz_gerber_export.c
- [ ] Create fuzz_action_script.c
- [ ] Create fuzz_font_parser.c
- [ ] Create fuzz_netlist_import.c
- [ ] Set up corpus directories
- [ ] Add fuzzing build target
- [ ] Run initial fuzzing (24 hours)
- [ ] Triage and fix crashes
- [ ] Apply for OSS-Fuzz
- [ ] Configure continuous fuzzing
- [ ] Document fuzzing in TESTING.md

#### Weeks 25-27: Performance Benchmarking
- [ ] Add Google Benchmark as submodule
- [ ] Create tests/bench/ directory
- [ ] Create bench_autoroute.cpp (3 benchmarks)
- [ ] Create bench_drc.cpp (3 benchmarks)
- [ ] Create bench_export.cpp (3 benchmarks)
- [ ] Create bench_file_io.cpp (3 benchmarks)
- [ ] Add benchmark build target
- [ ] Run baseline benchmarks
- [ ] Add benchmark CI job
- [ ] Create benchmark comparison script
- [ ] Set performance regression threshold (10%)
- [ ] Document benchmarking in TESTING.md

#### Weeks 28-29: Cross-Platform CI
- [ ] Add macOS to CI matrix
- [ ] Test macOS build
- [ ] Fix macOS-specific issues
- [ ] Add Windows to CI matrix
- [ ] Test Windows build (MSYS2)
- [ ] Fix Windows-specific issues
- [ ] Add multi-distro Docker testing
- [ ] Test on Ubuntu 20.04, 22.04
- [ ] Test on Debian 11
- [ ] Test on Fedora 38
- [ ] Document platform-specific issues

#### Weeks 30-33: Integration Test Improvements
- [ ] Research perceptual diff tools
- [ ] Implement PSNR-based comparison
- [ ] Implement SSIM-based comparison
- [ ] Add thresholds to run_tests.sh
- [ ] Install Xvfb in CI
- [ ] Wrap action tests with xvfb-run
- [ ] Test headless execution
- [ ] Research snapshot testing
- [ ] Implement structural comparison for Gerber
- [ ] Implement structural comparison for SVG
- [ ] Update golden file regeneration process
- [ ] Document test improvements in README.txt

### Phase 4: Quality Infrastructure (Ongoing)

#### Metrics Dashboard
- [ ] Set up Codecov dashboard
- [ ] Create custom metrics page
- [ ] Track test count over time
- [ ] Track coverage over time
- [ ] Track test execution time
- [ ] Add performance graphs
- [ ] Set up alerts for regressions
- [ ] Document metrics in TESTING.md

#### Documentation
- [ ] Complete doc/TESTING.md
- [ ] Add unit test examples
- [ ] Add integration test examples
- [ ] Add coverage examples
- [ ] Add memory testing examples
- [ ] Add fuzzing guide
- [ ] Add benchmarking guide
- [ ] Update CONTRIBUTING.md
- [ ] Create testing FAQ
- [ ] Add troubleshooting section

#### Pre-commit Hooks
- [ ] Create .pre-commit-config.yaml
- [ ] Add unit test hook
- [ ] Add code format hook
- [ ] Add trailing whitespace hook
- [ ] Test pre-commit hooks
- [ ] Document hook installation
- [ ] Add to development setup guide
- [ ] Create bypass instructions for emergencies

---

## Progress Tracking

### Completion Status

**Phase 1: Foundation**
- [ ] Coverage Tracking (0/10 tasks)
- [ ] Enhanced Static Analysis (0/9 tasks)
- [ ] Memory Safety Testing (0/10 tasks)
- [ ] Documentation (0/10 tasks)

**Phase 2: Expand Coverage**
- [ ] Core Geometry Tests (0/20 tests)
- [ ] DRC Engine Tests (0/30 tests)
- [ ] File I/O Tests (0/15 tests)
- [ ] Buffer Operations (0/10 tests)
- [ ] C++ Compatibility (0/10 tests)

**Phase 3: Advanced Testing**
- [ ] Fuzzing Infrastructure (0/14 tasks)
- [ ] Performance Benchmarking (0/12 tasks)
- [ ] Cross-Platform CI (0/11 tasks)
- [ ] Integration Test Improvements (0/12 tasks)

**Phase 4: Quality Infrastructure**
- [ ] Metrics Dashboard (0/8 tasks)
- [ ] Documentation (0/10 tasks)
- [ ] Pre-commit Hooks (0/8 tasks)

### Metrics to Track

| Metric | Baseline | Target (3mo) | Target (6mo) | Target (12mo) | Current |
|--------|----------|--------------|--------------|---------------|---------|
| Unit Tests | 3 | 100 | 200 | 300 | 3 |
| Integration Tests | 379 | 379 | 390 | 400 | 379 |
| Code Coverage | <1% | 20% | 30% | 50% | <1% |
| Memory Leaks | Unknown | 0 | 0 | 0 | Unknown |
| Build Time | ~5 min | <7 min | <8 min | <10 min | ~5 min |
| Test Time | ~2 min | <5 min | <8 min | <10 min | ~2 min |

---

## References

### Tools Documentation

- **GLib Testing:** https://docs.gtk.org/glib/testing.html
- **Google Test:** https://google.github.io/googletest/
- **lcov:** http://ltp.sourceforge.net/coverage/lcov.php
- **Valgrind:** https://valgrind.org/docs/manual/quick-start.html
- **AddressSanitizer:** https://clang.llvm.org/docs/AddressSanitizer.html
- **libFuzzer:** https://llvm.org/docs/LibFuzzer.html
- **OSS-Fuzz:** https://google.github.io/oss-fuzz/
- **Codecov:** https://docs.codecov.com/docs
- **clang-tidy:** https://clang.llvm.org/extra/clang-tidy/
- **CodeQL:** https://codeql.github.com/docs/

### Related Documentation

- `tests/README.txt` - Integration test guide
- `CONTRIBUTING.md` - Contribution guidelines
- `RENEWAL_PROPOSAL.md` - Project modernization plan
- `.github/workflows/build.yml` - CI configuration

### Contact

For questions about this testing infrastructure proposal:
- Open an issue: https://github.com/technobauble/pcb/issues
- Discussions: https://github.com/technobauble/pcb/discussions

---

**Document Status:** Draft proposal for review
**Last Updated:** November 13, 2025
**Next Review:** After Phase 1 completion
