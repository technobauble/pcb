# PCB Testing Guide

This guide explains how to run and write tests for the PCB project.

## Table of Contents

1. [Overview](#overview)
2. [Running Tests](#running-tests)
3. [Code Coverage](#code-coverage)
4. [Memory Safety Testing](#memory-safety-testing)
5. [Writing Unit Tests](#writing-unit-tests)
6. [Writing Integration Tests](#writing-integration-tests)
7. [Continuous Integration](#continuous-integration)
8. [Troubleshooting](#troubleshooting)

---

## Overview

PCB uses two types of tests:

- **Unit Tests:** Test individual functions/modules in isolation (GLib `g_test` framework)
- **Integration Tests:** Test full workflows and export formats (shell scripts with golden files)

Current test statistics:
- 379 integration tests covering all export HIDs and DRC functionality
- 3 unit tests (expanding - see [TESTING_INFRASTRUCTURE.md](TESTING_INFRASTRUCTURE.md))

---

## Running Tests

### All Tests

```bash
make check
```

This runs both unit tests and integration tests.

### Unit Tests Only

```bash
cd src
./unittest
```

To run specific unit test suites:

```bash
./unittest -p /pcb-printf/  # Only pcb-printf tests
./unittest -p /object-list/ # Only object-list tests
```

### Integration Tests

```bash
cd tests
./run_tests.sh
```

To run specific integration tests:

```bash
./run_tests.sh hid_gerber1           # Run single test
./run_tests.sh hid_gerber1 hid_bom1  # Run multiple tests
```

### Quick Test Subset

For rapid iteration during development:

```bash
# Unit tests (fast)
cd src && ./unittest

# Fast integration tests
cd tests && ./run_tests.sh hid_bom1 hid_png1
```

---

## Code Coverage

Code coverage shows which parts of the codebase are exercised by tests.

### Enabling Coverage

```bash
# Configure with coverage instrumentation
./configure --enable-coverage --disable-doc

# Build
make -j$(nproc)

# Run tests
make check
```

### Generating Coverage Reports

#### HTML Report (Local Viewing)

```bash
# Capture coverage data
lcov --capture --directory . --output-file coverage.info

# Remove external/generated code
lcov --remove coverage.info \
  '/usr/*' \
  '*/gts/*' \
  '*/parse_y.c' \
  '*/parse_l.c' \
  '*/edif.c' \
  '*/tests/*' \
  --output-file coverage_filtered.info

# Generate HTML report
genhtml coverage_filtered.info --output-directory coverage_html

# View in browser
firefox coverage_html/index.html
# or
open coverage_html/index.html  # macOS
```

#### Summary Report (Terminal)

```bash
lcov --list coverage_filtered.info
```

### Interpreting Coverage

**Coverage Metrics:**
- **Line Coverage:** Percentage of code lines executed
- **Function Coverage:** Percentage of functions called
- **Branch Coverage:** Percentage of decision branches taken

**What Coverage Means:**
- `> 80%`: Excellent coverage
- `50-80%`: Good coverage
- `30-50%`: Moderate coverage (current target)
- `< 30%`: Poor coverage (needs improvement)

**Coverage by Module (Goals):**
- Critical modules (DRC, geometry): Target 60%+
- Core functionality: Target 40%+
- UI code: Target 20%+
- Overall project: Target 30%+

### CI Coverage Reports

Coverage is automatically tracked in CI:
- View reports at: https://codecov.io/gh/technobauble/pcb
- Coverage badge in README shows current percentage
- PR comments show coverage impact of changes

---

## Memory Safety Testing

Memory safety testing catches bugs like:
- Memory leaks
- Buffer overflows
- Use-after-free
- Uninitialized memory reads

### AddressSanitizer (ASan)

ASan is the fastest memory error detector.

```bash
# Configure with ASan
./configure \
  CFLAGS="-fsanitize=address -g -O1 -fno-omit-frame-pointer" \
  LDFLAGS="-fsanitize=address" \
  --disable-doc

# Build
make -j$(nproc)

# Run tests
make check
```

**Note:** ASan will abort on first error detected. Check output for details.

### Valgrind

Valgrind is slower but finds more leak types.

```bash
# Configure normally
./configure --disable-doc
make -j$(nproc)

# Run unit tests under Valgrind
cd src
valgrind --leak-check=full --show-leak-kinds=all ./unittest

# Run specific integration test
cd tests
valgrind --leak-check=full ../src/pcb -x gerber inputs/gerber_oneline.pcb
```

**Suppressing False Positives:**

Create `tests/valgrind.supp`:

```
{
   gtk_init_leak
   Memcheck:Leak
   ...
   fun:gtk_init
}
```

Use with:

```bash
valgrind --suppressions=tests/valgrind.supp --leak-check=full ./unittest
```

### CI Memory Safety

Memory safety tests run automatically in CI:
- AddressSanitizer build and test
- Logs uploaded as artifacts if errors detected
- Builds continue even if warnings found (informational)

---

## Writing Unit Tests

Unit tests use GLib's `g_test` framework.

### Test File Structure

```c
// In src/mymodule.c

/* Regular code here */

#ifdef PCB_UNIT_TEST
#include <glib.h>

/* Helper functions for tests */
static TestObject *
create_test_object(int value)
{
  TestObject *obj = g_new0(TestObject, 1);
  obj->value = value;
  return obj;
}

/* Test functions */
static void
test_myfunction_normal_case(void)
{
  int result = my_function(5);
  g_assert_cmpint(result, ==, 10);
}

static void
test_myfunction_edge_case(void)
{
  int result = my_function(0);
  g_assert_cmpint(result, ==, 0);
}

static void
test_myfunction_error_case(void)
{
  /* Test that function handles errors correctly */
  int result = my_function(-1);
  g_assert_cmpint(result, ==, -1);
}

/* Registration function */
void
mymodule_register_tests(void)
{
  g_test_add_func("/mymodule/myfunction/normal", test_myfunction_normal_case);
  g_test_add_func("/mymodule/myfunction/edge", test_myfunction_edge_case);
  g_test_add_func("/mymodule/myfunction/error", test_myfunction_error_case);
}
#endif
```

### Register Tests in main-test.c

```c
// In src/main-test.c

#ifdef PCB_UNIT_TEST
/* Declare registration functions */
void pcb_printf_register_tests(void);
void object_list_register_tests(void);
void mymodule_register_tests(void);  // Add this

int
main(int argc, char *argv[])
{
  initialize_units();

  /* Register all test modules */
  pcb_printf_register_tests();
  object_list_register_tests();
  mymodule_register_tests();  // Add this

  g_test_init(&argc, &argv, NULL);
  return g_test_run();
}
#endif
```

### GLib Test Assertions

```c
/* Integer comparisons */
g_assert_cmpint(n1, ==, n2);
g_assert_cmpint(n1, !=, n2);
g_assert_cmpint(n1, <, n2);
g_assert_cmpint(n1, >, n2);

/* String comparisons */
g_assert_cmpstr(s1, ==, s2);
g_assert_cmpstr(s1, !=, s2);

/* Floating point comparisons */
g_assert_cmpfloat(f1, ==, f2);
g_assert_cmpfloat_with_epsilon(f1, f2, epsilon);

/* Pointer checks */
g_assert_null(ptr);
g_assert_nonnull(ptr);

/* Boolean */
g_assert_true(condition);
g_assert_false(condition);

/* Generic */
g_assert(condition);  // Use sparingly, prefer specific assertions
```

### Test Naming Convention

Test paths should follow the pattern:

```
/module-name/function-name/test-case
```

Examples:

```
/pcb-printf/test-unit
/pcb-printf/test-printf
/object-list/test
/intersect/line-line/parallel
/intersect/line-line/perpendicular
/drc/clearance/pass
/drc/clearance/fail
```

### Test Best Practices

1. **One concept per test:** Each test should verify one specific behavior
2. **Descriptive names:** Test name should describe what is being tested
3. **Arrange-Act-Assert:** Structure tests in three phases
   - Arrange: Set up test data
   - Act: Call the function being tested
   - Assert: Verify the results
4. **Clean up:** Free allocated memory, close files
5. **Independent tests:** Tests should not depend on each other
6. **Fast tests:** Unit tests should run in milliseconds

**Example:**

```c
static void
test_line_intersection_perpendicular(void)
{
  /* Arrange */
  LineType line1 = {.Point1.X = 50, .Point1.Y = 0,
                    .Point2.X = 50, .Point2.Y = 100};
  LineType line2 = {.Point1.X = 0, .Point1.Y = 50,
                    .Point2.X = 100, .Point2.Y = 50};
  PointType intersection;

  /* Act */
  bool result = line_intersects_line_at(&line1, &line2, &intersection);

  /* Assert */
  g_assert_true(result);
  g_assert_cmpint(intersection.X, ==, 50);
  g_assert_cmpint(intersection.Y, ==, 50);
}
```

---

## Writing Integration Tests

Integration tests are defined in `tests/tests.list`. For detailed instructions, see `tests/README.txt`.

### Quick Reference

**Test Format:**

```
test_name | layout_files | HID | options | mismatch | output_files
```

**Example:**

```
hid_gerber1 | gerber_oneline.pcb | gerber | | | \
  gbx:gerber_oneline.bottom.gbr \
  gbx:gerber_oneline.top.gbr \
  cnc:gerber_oneline.plated-drill.cnc
```

### Adding a New Integration Test

1. **Create input PCB file** in `tests/inputs/`
2. **Add test entry** to `tests/tests.list`
3. **Generate golden files:**
   ```bash
   cd tests
   ./run_tests.sh --regen my_new_test
   ```
4. **Verify golden files** are correct
5. **Update Makefile.am** to include new files in `EXTRA_DIST`
6. **Test:**
   ```bash
   ./run_tests.sh my_new_test
   ```

### Output File Types

- `bom` - Bill of materials
- `xy` - Centroid file
- `gbx` - Gerber (RS-274X)
- `cnc` - Excellon drill file
- `gcode` - G-code file
- `png` - PNG image
- `ps` - PostScript
- `pcb` - PCB layout file (for action tests)

---

## Continuous Integration

### GitHub Actions Workflows

PCB uses GitHub Actions for CI with multiple jobs:

**Build Jobs:**
- GTK GUI build
- GTK + OpenGL build
- Batch (headless) build

**Quality Jobs:**
- Code coverage (lcov + Codecov)
- Memory safety (AddressSanitizer)
- Static analysis (cppcheck + clang-tidy)

**All jobs run on:**
- Every push to `master`
- Every push to `claude/**` branches
- Every pull request

### Viewing CI Results

1. Go to: https://github.com/technobauble/pcb/actions
2. Click on a workflow run
3. View job results and logs
4. Download artifacts (coverage reports, logs)

### CI Artifacts

Available for 7-30 days after run:

- **Build logs:** configure.log, build.log, test.log
- **Test logs:** unittest.log, test-suite.log
- **Coverage:** coverage_filtered.info, coverage_summary.txt
- **ASan logs:** asan-unittest.log, asan-tests.log
- **Static analysis:** cppcheck-report.txt, clang-tidy-report.txt
- **Binaries:** pcb-linux-gtk (GTK build)

### Coverage in Pull Requests

Codecov automatically comments on PRs with:
- Coverage change (increase/decrease)
- Coverage of new code
- Files with coverage changes

---

## Troubleshooting

### Unit Tests Fail to Build

```
undefined reference to `mymodule_register_tests'
```

**Solution:** Add declaration and call in `src/main-test.c`

### Coverage Data Not Generated

```
genhtml: ERROR: no valid records found in tracefile
```

**Solution:** Ensure you configured with `--enable-coverage` and ran tests

### ASan Reports False Positives

**Solution:** Add suppressions or use `ASAN_OPTIONS`:

```bash
export ASAN_OPTIONS=detect_leaks=0
./unittest
```

### Integration Tests Fail on Different Machines

**Common causes:**
- ImageMagick version differences (pixel-perfect comparison)
- Missing dependencies (gerbv, imagemagick)
- X11 not available (action tests)

**Solutions:**
- Use `PCB_MAGIC_TEST_SKIP=yes make check` to skip comparison
- Install all dependencies
- Use `xvfb-run` for headless X11

### Valgrind Shows GTK Leaks

**Solution:** GTK/GLib have known "leaks" that are not real leaks. Add suppressions:

```bash
valgrind --suppressions=/usr/share/glib-2.0/valgrind/glib.supp \
         --suppressions=tests/valgrind.supp \
         --leak-check=full \
         ./unittest
```

---

## Additional Resources

- **Testing Infrastructure Proposal:** [doc/TESTING_INFRASTRUCTURE.md](TESTING_INFRASTRUCTURE.md)
- **Integration Test Details:** [tests/README.txt](../tests/README.txt)
- **GLib Testing:** https://docs.gtk.org/glib/testing.html
- **lcov Documentation:** http://ltp.sourceforge.net/coverage/lcov.php
- **AddressSanitizer:** https://clang.llvm.org/docs/AddressSanitizer.html
- **Codecov:** https://codecov.io/gh/technobauble/pcb

---

## Quick Command Reference

```bash
# Build and test
./autogen.sh
./configure --disable-doc
make -j$(nproc)
make check

# Coverage
./configure --enable-coverage --disable-doc
make check
lcov --capture --directory . -o coverage.info
genhtml coverage.info -o coverage_html

# Memory safety
./configure CFLAGS="-fsanitize=address -g" LDFLAGS="-fsanitize=address" --disable-doc
make check

# Unit tests
cd src && ./unittest
cd src && ./unittest -p /pcb-printf/

# Integration tests
cd tests && ./run_tests.sh
cd tests && ./run_tests.sh hid_gerber1

# Regenerate golden files
cd tests && ./run_tests.sh --regen hid_gerber1
```

---

**Last Updated:** November 13, 2025
**See Also:** [TESTING_INFRASTRUCTURE.md](TESTING_INFRASTRUCTURE.md) for modernization roadmap
