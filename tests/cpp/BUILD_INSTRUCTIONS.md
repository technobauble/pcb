# Building C++ Tests

## Prerequisites

The C++ test infrastructure requires:
- C++17 compatible compiler (g++ 7+ or clang++ 5+)
- Google Test 1.10.0 or later
- pkg-config

### Installing Google Test

**Ubuntu/Debian:**
```bash
sudo apt-get install -y libgtest-dev cmake

# Google Test source needs to be built
cd /usr/src/gtest
sudo cmake CMakeLists.txt
sudo make
sudo cp lib/*.a /usr/lib
# Or on newer systems:
sudo cp googlemock/gtest/*.a /usr/lib
```

**Fedora/RHEL:**
```bash
sudo dnf install gtest-devel
```

**macOS (Homebrew):**
```bash
brew install googletest
```

**Arch Linux:**
```bash
sudo pacman -S gtest
```

## Building with C++ Tests

```bash
# Generate build files
./autogen.sh

# Configure with C++ tests enabled (automatic if Google Test found)
./configure --disable-doc

# Build
make -j$(nproc)

# Run all tests (C + C++)
make check

# Run only C++ tests
cd tests/cpp
./unittest_cpp

# Run with verbose output
./unittest_cpp --gtest_print_time=1 --gtest_color=yes

# Run specific tests
./unittest_cpp --gtest_filter=ExampleTest.*

# List all tests
./unittest_cpp --gtest_list_tests
```

## Configuration Output

When Google Test is **found:**
```
checking for C++17 support... yes
checking for Google Test... yes (C++ tests will be built)
...
C++ tests (Google Test):  yes
```

When Google Test is **not found:**
```
checking for C++17 support... yes
checking for Google Test... no (Google Test not found)
configure: C++ tests disabled: Google Test not found
configure: Install with: sudo apt-get install libgtest-dev
...
C++ tests (Google Test):  no
```

Build continues successfully either way - C++ tests are optional!

## Integration with Coverage

C++ tests contribute to code coverage when built with `--enable-coverage`:

```bash
./configure --enable-coverage --disable-doc
make check

# Generate coverage report (includes both C and C++ tests)
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/tests/*' --output-file coverage_filtered.info
genhtml coverage_filtered.info --output-directory coverage_html
```

## Troubleshooting

### "Google Test not found" but it's installed

Check pkg-config can find it:
```bash
pkg-config --modversion gtest
```

If not found, you may need to install the pkg-config files:
```bash
# Create /usr/lib/pkgconfig/gtest.pc manually or
# install Google Test via package manager instead of building from source
```

### C++17 not supported

Upgrade your compiler:
```bash
# Ubuntu
sudo apt-get install g++-9

# Set as default
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 90
```

### Tests compile but fail to link

Ensure pthread is available:
```bash
gcc -pthread -lgtest -lgtest_main tests/cpp/example_test.cpp
```

If that fails, Google Test may not be properly installed.

## Manual Build (without autotools)

For development/testing without running full configure:

```bash
cd tests/cpp

# Build manually
g++ -std=c++17 -I../../src \
    $(pkg-config --cflags gtest) \
    example_test.cpp \
    $(pkg-config --libs gtest) -lgtest_main -lpthread \
    -o unittest_cpp

# Run
./unittest_cpp
```

Or use the provided stub:
```bash
cd tests/cpp
make -f Makefile.stub
make -f Makefile.stub check
```

---

**See also:**
- [CPP_MIGRATION.md](../../doc/CPP_MIGRATION.md) - C++ migration strategy
- [README.md](README.md) - C++ test overview
- [TESTING.md](../../doc/TESTING.md) - Main testing guide
