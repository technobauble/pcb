# How to Write and Register Unit Tests in PCB

This guide provides step-by-step instructions for adding new unit tests to the PCB codebase.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Understanding the Test Framework](#understanding-the-test-framework)
3. [Step-by-Step: Adding Your First Test](#step-by-step-adding-your-first-test)
4. [Test Structure and Best Practices](#test-structure-and-best-practices)
5. [Common Patterns and Examples](#common-patterns-and-examples)
6. [Running and Debugging Tests](#running-and-debugging-tests)
7. [Troubleshooting](#troubleshooting)

---

## Quick Start

**5-minute test addition checklist:**

1. ✅ Add test code to your module (inside `#ifdef PCB_UNIT_TEST`)
2. ✅ Create a `module_register_tests()` function
3. ✅ Declare it in `src/main-test.c`
4. ✅ Call it from `main()` in `src/main-test.c`
5. ✅ Rebuild and run: `make && ./unittest`

---

## Understanding the Test Framework

PCB uses **GLib's `g_test` framework** for unit testing.

### Key Concepts

- **Test Functions:** Functions that verify specific behaviors
- **Assertions:** Macros like `g_assert_cmpint()` that check conditions
- **Test Paths:** Hierarchical names like `/module/function/case`
- **Registration:** Tests must be registered before they run

### File Organization

```
src/
├── mymodule.c              # Your module with tests inside #ifdef
├── main-test.c             # Test registry and main()
└── Makefile.am             # Build configuration (usually no changes needed)
```

---

## Global State and Testing Isolation

### Understanding `initialize_units()`

In `src/main-test.c`, you'll notice that `main()` calls `initialize_units()` before registering tests:

```c
int
main(int argc, char *argv[])
{
  initialize_units();  // <-- Why is this needed?

  pcb_printf_register_tests();
  object_list_register_tests();

  g_test_init(&argc, &argv, NULL);
  return g_test_run();
}
```

**Why does this exist?**

PCB has global state that must be initialized before many functions work correctly:
- **Unit conversion tables** (mil/mm/inch conversions)
- **Coordinate system settings**
- **Default settings and configurations**

This is **technical debt** from PCB's legacy architecture. In an ideal world, unit tests should not require global initialization.

### The Problem with Global State

Global state violates unit testing principles:

1. **Tests aren't truly isolated** - They share global state
2. **Hidden dependencies** - Tests may pass/fail based on initialization order
3. **Harder to test** - Can't easily test functions in different states
4. **Slower tests** - Can't run tests in parallel safely

### Better Approach: GTest Fixtures

GTest provides **fixtures** for per-test setup and teardown, which is a more modern approach:

```c
/* Define a fixture structure */
typedef struct {
  UnitSystem *units;
  Settings *settings;
  /* Other test-specific state */
} MyFixture;

/* Setup function - called before each test */
static void
my_fixture_setup(MyFixture *fixture, gconstpointer user_data)
{
  /* Initialize only what this test needs */
  fixture->units = create_unit_system();
  fixture->settings = create_default_settings();
}

/* Teardown function - called after each test */
static void
my_fixture_teardown(MyFixture *fixture, gconstpointer user_data)
{
  /* Clean up */
  free_unit_system(fixture->units);
  free_settings(fixture->settings);
}

/* Test function using fixture */
static void
test_convert_units(MyFixture *fixture, gconstpointer user_data)
{
  /* Test has access to fixture->units and fixture->settings */
  Coord result = convert_to_mil(100, fixture->units);
  g_assert_cmpint(result, ==, 100);
}

/* Register test with fixture */
void
mymodule_register_tests(void)
{
  g_test_add("/mymodule/convert-units",
             MyFixture,
             NULL,  /* user_data */
             my_fixture_setup,
             test_convert_units,
             my_fixture_teardown);
}
```

### Benefits of Fixtures

- ✅ **True isolation** - Each test gets fresh state
- ✅ **Explicit dependencies** - Clear what each test needs
- ✅ **Easier to maintain** - Setup/teardown in one place
- ✅ **Better testing** - Can easily test different configurations
- ✅ **Parallel execution** - Tests can run concurrently

### Current State vs. Ideal State

**Current State (PCB):**
```c
// main-test.c
int main() {
  initialize_units();  // Global initialization

  // All tests share this global state
  g_test_run();
}
```

**Ideal State (with fixtures):**
```c
// mymodule.c
static void
test_with_fixture(MyFixture *fixture, gconstpointer user_data)
{
  // Each test gets its own isolated state
  // No global dependencies
}
```

### Practical Guidance for PCB

**For now:**
- Continue using `initialize_units()` in `main-test.c`
- It's necessary for many PCB functions to work
- Document which global state your tests depend on

**For new code:**
- Try to write functions that don't require global state
- Pass dependencies as parameters instead of using globals
- Use fixtures when possible for new test modules

**Long-term goal:**
- Refactor PCB to reduce global state
- Migrate tests to use fixtures
- Enable parallel test execution

### Example: Testing with and without Global State

**Function depending on global state:**
```c
/* Bad - depends on global Settings */
Coord
scale_dimension(Coord value)
{
  return value * Settings.grid_scale;  // Uses global
}

/* Test must use global state */
static void
test_scale_dimension(void)
{
  Settings.grid_scale = 2.0;  // Modify global
  Coord result = scale_dimension(100);
  g_assert_cmpint(result, ==, 200);
  Settings.grid_scale = 1.0;  // Reset global
}
```

**Better design - explicit dependencies:**
```c
/* Good - takes settings as parameter */
Coord
scale_dimension(Coord value, double grid_scale)
{
  return value * grid_scale;
}

/* Test is isolated */
static void
test_scale_dimension(void)
{
  Coord result = scale_dimension(100, 2.0);
  g_assert_cmpint(result, ==, 200);
  /* No global state to clean up */
}
```

### When to Use Fixtures

Use fixtures when:
- Your tests need setup/teardown
- You're testing a new module without global dependencies
- You need different configurations for different tests
- You want true test isolation

Continue using global initialization when:
- Testing legacy code that requires it
- Refactoring to remove globals would be too invasive
- You're working with coordinate systems or unit conversions

---

## Step-by-Step: Adding Your First Test

Let's add a test for a hypothetical function `calculate_distance()` in `src/geometry.c`.

### Step 1: Add Test Code to Your Module

Open the file you want to test (e.g., `src/geometry.c`) and add test code at the bottom:

```c
// src/geometry.c

/* Regular production code here */

int
calculate_distance(Point p1, Point p2)
{
  int dx = p2.x - p1.x;
  int dy = p2.y - p1.y;
  return sqrt(dx * dx + dy * dy);
}

/* More production code... */

/*
 * ============================================================================
 *                              UNIT TESTS
 * ============================================================================
 */
#ifdef PCB_UNIT_TEST
#include <glib.h>

/* Test: Calculate distance between two points on same horizontal line */
static void
test_calculate_distance_horizontal(void)
{
  Point p1 = {.x = 0, .y = 0};
  Point p2 = {.x = 10, .y = 0};

  int distance = calculate_distance(p1, p2);

  g_assert_cmpint(distance, ==, 10);
}

/* Test: Calculate distance between two points on same vertical line */
static void
test_calculate_distance_vertical(void)
{
  Point p1 = {.x = 0, .y = 0};
  Point p2 = {.x = 0, .y = 10};

  int distance = calculate_distance(p1, p2);

  g_assert_cmpint(distance, ==, 10);
}

/* Test: Calculate distance for diagonal (3-4-5 triangle) */
static void
test_calculate_distance_diagonal(void)
{
  Point p1 = {.x = 0, .y = 0};
  Point p2 = {.x = 3, .y = 4};

  int distance = calculate_distance(p1, p2);

  g_assert_cmpint(distance, ==, 5);
}

/* Test: Distance to same point should be zero */
static void
test_calculate_distance_same_point(void)
{
  Point p1 = {.x = 5, .y = 7};
  Point p2 = {.x = 5, .y = 7};

  int distance = calculate_distance(p1, p2);

  g_assert_cmpint(distance, ==, 0);
}

/* Registration function - called from main-test.c */
void
geometry_register_tests(void)
{
  g_test_add_func("/geometry/calculate-distance/horizontal",
                  test_calculate_distance_horizontal);
  g_test_add_func("/geometry/calculate-distance/vertical",
                  test_calculate_distance_vertical);
  g_test_add_func("/geometry/calculate-distance/diagonal",
                  test_calculate_distance_diagonal);
  g_test_add_func("/geometry/calculate-distance/same-point",
                  test_calculate_distance_same_point);
}

#endif /* PCB_UNIT_TEST */
```

### Step 2: Declare Registration Function in main-test.c

Open `src/main-test.c` and add your declaration near the top with the others:

```c
// src/main-test.c

/* ... existing includes ... */

#ifdef PCB_UNIT_TEST

/* Declare all test registration functions */
void pcb_printf_register_tests(void);
void object_list_register_tests(void);
void geometry_register_tests(void);        // <-- ADD THIS LINE

int
main (int argc, char *argv[])
{
  /* ... rest of main ... */
}
```

### Step 3: Call Registration Function from main()

Still in `src/main-test.c`, add the call to your registration function:

```c
// src/main-test.c (continued)

int
main (int argc, char *argv[])
{
  initialize_units ();

  /* Register all test suites */
  pcb_printf_register_tests ();
  object_list_register_tests ();
  geometry_register_tests ();              // <-- ADD THIS LINE

  g_test_init (&argc, &argv, NULL);
  return g_test_run ();
}

#endif /* PCB_UNIT_TEST */
```

### Step 4: Build and Run

```bash
# Rebuild
cd /path/to/pcb
make

# Run all tests
cd src
./unittest

# Run only your new tests
./unittest -p /geometry/
```

**Expected Output:**
```
/geometry/calculate-distance/horizontal: OK
/geometry/calculate-distance/vertical: OK
/geometry/calculate-distance/diagonal: OK
/geometry/calculate-distance/same-point: OK
```

---

## Test Structure and Best Practices

### Anatomy of a Good Test

```c
static void
test_function_name_scenario(void)
{
  /* 1. ARRANGE - Set up test data */
  Point p1 = {.x = 0, .y = 0};
  Point p2 = {.x = 10, .y = 0};

  /* 2. ACT - Call the function being tested */
  int result = calculate_distance(p1, p2);

  /* 3. ASSERT - Verify the result */
  g_assert_cmpint(result, ==, 10);
}
```

### Test Naming Conventions

**Format:** `test_<function_name>_<scenario>`

**Examples:**
- `test_parse_pcb_valid_file()`
- `test_parse_pcb_empty_file()`
- `test_parse_pcb_corrupt_data()`
- `test_line_intersection_parallel()`
- `test_line_intersection_perpendicular()`

### Test Path Conventions

**Format:** `/module-name/function-or-feature/test-case`

**Examples:**
```c
g_test_add_func("/geometry/calculate-distance/horizontal", ...);
g_test_add_func("/geometry/calculate-distance/vertical", ...);
g_test_add_func("/parser/parse-pcb/valid-file", ...);
g_test_add_func("/parser/parse-pcb/invalid-syntax", ...);
g_test_add_func("/drc/clearance-check/pass", ...);
g_test_add_func("/drc/clearance-check/fail", ...);
```

### Best Practices

#### ✅ DO

- **Test one thing per test** - Each test should verify one specific behavior
- **Use descriptive names** - Name should explain what's being tested
- **Keep tests fast** - Unit tests should run in milliseconds
- **Test edge cases** - Zero, negative, NULL, empty, maximum values
- **Clean up resources** - Free allocated memory, close files
- **Make tests independent** - Each test should work in isolation
- **Use specific assertions** - Prefer `g_assert_cmpint()` over `g_assert()`

#### ❌ DON'T

- **Don't test implementation details** - Test behavior, not internals
- **Don't depend on other tests** - Tests should not run in specific order
- **Don't use production resources** - Tests should use test data
- **Don't ignore failures** - All assertions should be meaningful
- **Don't make tests complex** - Simple tests are better tests

---

## Common Patterns and Examples

### Pattern 1: Testing Integer Functions

```c
static void
test_add_positive_numbers(void)
{
  int result = add(5, 3);
  g_assert_cmpint(result, ==, 8);
}

static void
test_add_negative_numbers(void)
{
  int result = add(-5, -3);
  g_assert_cmpint(result, ==, -8);
}

static void
test_add_mixed_signs(void)
{
  int result = add(5, -3);
  g_assert_cmpint(result, ==, 2);
}

static void
test_add_with_zero(void)
{
  int result = add(5, 0);
  g_assert_cmpint(result, ==, 5);
}
```

### Pattern 2: Testing String Functions

```c
static void
test_parse_name_valid(void)
{
  const char *input = "Component_R1";
  char *result = parse_component_name(input);

  g_assert_nonnull(result);
  g_assert_cmpstr(result, ==, "R1");

  g_free(result);
}

static void
test_parse_name_empty(void)
{
  const char *input = "";
  char *result = parse_component_name(input);

  g_assert_null(result);
}
```

### Pattern 3: Testing Boolean Functions

```c
static void
test_is_valid_coordinate_inside_board(void)
{
  Point p = {.x = 100, .y = 100};
  Board board = {.width = 200, .height = 200};

  bool result = is_valid_coordinate(&p, &board);

  g_assert_true(result);
}

static void
test_is_valid_coordinate_outside_board(void)
{
  Point p = {.x = 300, .y = 100};
  Board board = {.width = 200, .height = 200};

  bool result = is_valid_coordinate(&p, &board);

  g_assert_false(result);
}
```

### Pattern 4: Testing with Structures

```c
static void
test_create_line_valid_points(void)
{
  Point p1 = {.x = 0, .y = 0};
  Point p2 = {.x = 100, .y = 100};

  LineType *line = create_line(&p1, &p2, 10);

  g_assert_nonnull(line);
  g_assert_cmpint(line->Point1.X, ==, 0);
  g_assert_cmpint(line->Point1.Y, ==, 0);
  g_assert_cmpint(line->Point2.X, ==, 100);
  g_assert_cmpint(line->Point2.Y, ==, 100);
  g_assert_cmpint(line->Thickness, ==, 10);

  free_line(line);
}
```

### Pattern 5: Testing Error Handling

```c
static void
test_open_file_not_found(void)
{
  FILE *fp = open_pcb_file("/nonexistent/file.pcb");

  g_assert_null(fp);
  // Verify error was set appropriately
}

static void
test_parse_invalid_syntax(void)
{
  const char *invalid = "INVALID [[[[ SYNTAX";
  PCBType *pcb = parse_pcb_string(invalid);

  g_assert_null(pcb);
}
```

### Pattern 6: Using Helper Functions

```c
#ifdef PCB_UNIT_TEST

/* Helper function to create test points */
static Point
make_point(int x, int y)
{
  Point p = {.x = x, .y = y};
  return p;
}

/* Helper function to create test lines */
static LineType*
make_test_line(int x1, int y1, int x2, int y2, int thickness)
{
  LineType *line = g_new0(LineType, 1);
  line->Point1.X = x1;
  line->Point1.Y = y1;
  line->Point2.X = x2;
  line->Point2.Y = y2;
  line->Thickness = thickness;
  return line;
}

static void
test_line_length(void)
{
  LineType *line = make_test_line(0, 0, 3, 4, 10);

  int length = calculate_line_length(line);

  g_assert_cmpint(length, ==, 5);

  g_free(line);
}

#endif
```

---

## GLib Test Assertions Reference

### Integer Comparisons

```c
g_assert_cmpint(n1, ==, n2);    // n1 equals n2
g_assert_cmpint(n1, !=, n2);    // n1 not equals n2
g_assert_cmpint(n1, <, n2);     // n1 less than n2
g_assert_cmpint(n1, >, n2);     // n1 greater than n2
g_assert_cmpint(n1, <=, n2);    // n1 less than or equal
g_assert_cmpint(n1, >=, n2);    // n1 greater than or equal

// Example:
g_assert_cmpint(result, ==, 42);
```

### Unsigned Integer Comparisons

```c
g_assert_cmpuint(n1, ==, n2);
g_assert_cmpuint(count, >, 0);
```

### Floating Point Comparisons

```c
g_assert_cmpfloat(f1, ==, f2);
g_assert_cmpfloat_with_epsilon(f1, f2, epsilon);

// Example:
g_assert_cmpfloat_with_epsilon(result, 3.14159, 0.0001);
```

### String Comparisons

```c
g_assert_cmpstr(s1, ==, s2);    // Strings equal
g_assert_cmpstr(s1, !=, s2);    // Strings not equal

// Example:
g_assert_cmpstr(name, ==, "Component_R1");
```

### Pointer Checks

```c
g_assert_null(ptr);             // Pointer is NULL
g_assert_nonnull(ptr);          // Pointer is not NULL

// Example:
PCBType *pcb = load_pcb(file);
g_assert_nonnull(pcb);
```

### Boolean Checks

```c
g_assert_true(condition);       // Condition is true
g_assert_false(condition);      // Condition is false

// Example:
g_assert_true(is_valid_coordinate(&p));
```

### Generic Assertion

```c
g_assert(condition);            // Use sparingly, prefer specific assertions

// Example (but prefer specific assertions):
g_assert(result > 0 && result < 100);
```

### Memory Checks

```c
// For checking memory was allocated
LineType *line = create_line();
g_assert_nonnull(line);

// Don't forget to free in your test!
free_line(line);
```

---

## Running and Debugging Tests

### Running All Tests

```bash
cd src
./unittest
```

### Running Specific Tests

```bash
# Run all tests in a module
./unittest -p /geometry/

# Run all tests for a specific function
./unittest -p /geometry/calculate-distance/

# Run a single test
./unittest -p /geometry/calculate-distance/horizontal
```

### Verbose Output

```bash
./unittest --verbose
```

### Running Under Debugger

```bash
gdb ./unittest
(gdb) run -p /geometry/calculate-distance/horizontal
(gdb) break test_calculate_distance_horizontal
(gdb) run
```

### Running with Valgrind

```bash
valgrind --leak-check=full ./unittest
```

### Running with AddressSanitizer

```bash
# Rebuild with ASan
cd /path/to/pcb
./configure CFLAGS="-fsanitize=address -g" LDFLAGS="-fsanitize=address"
make

# Run tests
cd src
./unittest
```

---

## Troubleshooting

### Problem: Test Not Running

**Symptom:** Your test doesn't appear in the output.

**Solution:** Check that you:
1. Declared the registration function in `main-test.c`
2. Called the registration function from `main()`
3. Rebuilt: `make clean && make`

### Problem: Undefined Reference Error

**Symptom:**
```
undefined reference to `geometry_register_tests'
```

**Solution:** Make sure the function is inside `#ifdef PCB_UNIT_TEST` in your source file.

### Problem: Test Fails with Segfault

**Symptom:** Test crashes with segmentation fault.

**Solution:**
1. Check for NULL pointer dereferences
2. Ensure all pointers are initialized
3. Run under Valgrind to find the issue:
   ```bash
   valgrind ./unittest -p /your/failing/test
   ```

### Problem: Test Passes Locally But Fails in CI

**Symptom:** Test works on your machine but fails in GitHub Actions.

**Solution:**
1. Check for platform-specific assumptions (paths, line endings)
2. Ensure test doesn't depend on specific hardware/display
3. Check for timing-dependent behavior
4. Review CI logs for specific error messages

### Problem: Memory Leak Reported

**Symptom:** Valgrind or ASan reports memory leak in your test.

**Solution:** Make sure you free all allocated memory:
```c
static void
test_create_something(void)
{
  Thing *thing = create_thing();

  /* ... test assertions ... */

  free_thing(thing);  // Don't forget this!
}
```

---

## Complete Example: Adding Tests for a New Module

Let's add comprehensive tests for a new module `src/coordinate.c`:

### coordinate.c

```c
// src/coordinate.c

#include "coordinate.h"
#include <math.h>

/* Transform a coordinate by scaling */
Coord
scale_coordinate(Coord c, double scale)
{
  return (Coord)(c * scale);
}

/* Check if coordinate is within bounds */
bool
coordinate_in_bounds(Coord c, Coord min, Coord max)
{
  return c >= min && c <= max;
}

/* Calculate Manhattan distance between two points */
Coord
manhattan_distance(Coord x1, Coord y1, Coord x2, Coord y2)
{
  return abs(x2 - x1) + abs(y2 - y1);
}

/*
 * ============================================================================
 *                              UNIT TESTS
 * ============================================================================
 */
#ifdef PCB_UNIT_TEST
#include <glib.h>

static void
test_scale_coordinate_positive(void)
{
  Coord result = scale_coordinate(100, 2.0);
  g_assert_cmpint(result, ==, 200);
}

static void
test_scale_coordinate_negative(void)
{
  Coord result = scale_coordinate(100, -1.0);
  g_assert_cmpint(result, ==, -100);
}

static void
test_scale_coordinate_fractional(void)
{
  Coord result = scale_coordinate(100, 0.5);
  g_assert_cmpint(result, ==, 50);
}

static void
test_scale_coordinate_zero(void)
{
  Coord result = scale_coordinate(100, 0.0);
  g_assert_cmpint(result, ==, 0);
}

static void
test_coordinate_in_bounds_inside(void)
{
  bool result = coordinate_in_bounds(50, 0, 100);
  g_assert_true(result);
}

static void
test_coordinate_in_bounds_at_min(void)
{
  bool result = coordinate_in_bounds(0, 0, 100);
  g_assert_true(result);
}

static void
test_coordinate_in_bounds_at_max(void)
{
  bool result = coordinate_in_bounds(100, 0, 100);
  g_assert_true(result);
}

static void
test_coordinate_in_bounds_below_min(void)
{
  bool result = coordinate_in_bounds(-1, 0, 100);
  g_assert_false(result);
}

static void
test_coordinate_in_bounds_above_max(void)
{
  bool result = coordinate_in_bounds(101, 0, 100);
  g_assert_false(result);
}

static void
test_manhattan_distance_horizontal(void)
{
  Coord dist = manhattan_distance(0, 0, 10, 0);
  g_assert_cmpint(dist, ==, 10);
}

static void
test_manhattan_distance_vertical(void)
{
  Coord dist = manhattan_distance(0, 0, 0, 10);
  g_assert_cmpint(dist, ==, 10);
}

static void
test_manhattan_distance_diagonal(void)
{
  Coord dist = manhattan_distance(0, 0, 3, 4);
  g_assert_cmpint(dist, ==, 7);  // 3 + 4
}

static void
test_manhattan_distance_same_point(void)
{
  Coord dist = manhattan_distance(5, 5, 5, 5);
  g_assert_cmpint(dist, ==, 0);
}

static void
test_manhattan_distance_negative_coords(void)
{
  Coord dist = manhattan_distance(-5, -5, 5, 5);
  g_assert_cmpint(dist, ==, 20);  // 10 + 10
}

void
coordinate_register_tests(void)
{
  g_test_add_func("/coordinate/scale/positive",
                  test_scale_coordinate_positive);
  g_test_add_func("/coordinate/scale/negative",
                  test_scale_coordinate_negative);
  g_test_add_func("/coordinate/scale/fractional",
                  test_scale_coordinate_fractional);
  g_test_add_func("/coordinate/scale/zero",
                  test_scale_coordinate_zero);

  g_test_add_func("/coordinate/in-bounds/inside",
                  test_coordinate_in_bounds_inside);
  g_test_add_func("/coordinate/in-bounds/at-min",
                  test_coordinate_in_bounds_at_min);
  g_test_add_func("/coordinate/in-bounds/at-max",
                  test_coordinate_in_bounds_at_max);
  g_test_add_func("/coordinate/in-bounds/below-min",
                  test_coordinate_in_bounds_below_min);
  g_test_add_func("/coordinate/in-bounds/above-max",
                  test_coordinate_in_bounds_above_max);

  g_test_add_func("/coordinate/manhattan-distance/horizontal",
                  test_manhattan_distance_horizontal);
  g_test_add_func("/coordinate/manhattan-distance/vertical",
                  test_manhattan_distance_vertical);
  g_test_add_func("/coordinate/manhattan-distance/diagonal",
                  test_manhattan_distance_diagonal);
  g_test_add_func("/coordinate/manhattan-distance/same-point",
                  test_manhattan_distance_same_point);
  g_test_add_func("/coordinate/manhattan-distance/negative-coords",
                  test_manhattan_distance_negative_coords);
}

#endif /* PCB_UNIT_TEST */
```

### Update main-test.c

```c
// src/main-test.c

/* Add to declarations */
void coordinate_register_tests(void);

/* Add to main() */
int
main(int argc, char *argv[])
{
  initialize_units();

  pcb_printf_register_tests();
  object_list_register_tests();
  coordinate_register_tests();     // <-- Add this

  g_test_init(&argc, &argv, NULL);
  return g_test_run();
}
```

### Run the Tests

```bash
make
cd src
./unittest -p /coordinate/

# Output:
# /coordinate/scale/positive: OK
# /coordinate/scale/negative: OK
# /coordinate/scale/fractional: OK
# /coordinate/scale/zero: OK
# /coordinate/in-bounds/inside: OK
# /coordinate/in-bounds/at-min: OK
# /coordinate/in-bounds/at-max: OK
# /coordinate/in-bounds/below-min: OK
# /coordinate/in-bounds/above-max: OK
# /coordinate/manhattan-distance/horizontal: OK
# /coordinate/manhattan-distance/vertical: OK
# /coordinate/manhattan-distance/diagonal: OK
# /coordinate/manhattan-distance/same-point: OK
# /coordinate/manhattan-distance/negative-coords: OK
```

---

## Summary Checklist

When adding unit tests, follow this checklist:

- [ ] Write test functions using naming convention `test_<function>_<scenario>`
- [ ] Use appropriate GLib assertions (`g_assert_cmpint`, `g_assert_cmpstr`, etc.)
- [ ] Wrap tests in `#ifdef PCB_UNIT_TEST` ... `#endif`
- [ ] Create `module_register_tests()` function
- [ ] Register each test with `g_test_add_func(path, function)`
- [ ] Use meaningful test paths: `/module/function/case`
- [ ] Declare registration function in `src/main-test.c`
- [ ] Call registration function from `main()` in `src/main-test.c`
- [ ] Rebuild: `make`
- [ ] Run tests: `cd src && ./unittest`
- [ ] Verify all tests pass
- [ ] Clean up memory in tests (no leaks)
- [ ] Test edge cases (zero, negative, NULL, empty, max values)

---

## Additional Resources

- **GLib Testing:** https://docs.gtk.org/glib/testing.html
- **Main Testing Guide:** [doc/TESTING.md](TESTING.md)
- **Testing Infrastructure:** [doc/TESTING_INFRASTRUCTURE.md](TESTING_INFRASTRUCTURE.md)
- **Existing Tests:** Look at `src/pcb-printf.c` and `src/object_list.c` for examples

---

**Last Updated:** November 13, 2025
