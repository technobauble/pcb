# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

PCB is a CAD program for designing printed circuit boards. It's a mature C/C++ codebase (since 1994) that uses autotools for building. The project is part of the gEDA ecosystem.

## Build Commands

```bash
# First-time setup (generate configure script)
./autogen.sh

# Configure (disable docs for faster builds)
./configure --disable-doc

# Build
make -j$(nproc)

# Run tests
make check

# Run a specific test
cd tests && ./run_tests.sh <test_name>

# Run C++ tests (if Google Test installed)
cd tests/cpp && ./unittest_cpp

# Clean build
make clean
```

### Configure Options

- `--with-gui=gtk|lesstif|batch` - GUI backend (default: gtk)
- `--enable-gl` - Enable OpenGL acceleration (GTK only)
- `--disable-doc` - Skip documentation build (faster)
- `--enable-debug` - Enable debug code
- `--enable-coverage` - Enable code coverage instrumentation

## Architecture

### HID (Hardware Interface Driver) System

PCB uses a modular HID architecture that separates:
- **GUI HIDs**: GTK+ (`src/hid/gtk/`), Lesstif (`src/hid/lesstif/`), Batch mode (`src/hid/batch/`)
- **Export HIDs**: Gerber, PNG, PostScript, G-code, BOM, etc. (`src/hid/*/`)
- **Core Engine**: Platform-independent PCB design logic (`src/`)

### C++ Action System (New)

The codebase is being refactored to migrate from a monolithic `action.c` to modular C++ actions:

- `src/actions/Action.h` - Base Action class and ActionRegistry
- `src/actions/action_bridge.h` - C/C++ bridge for calling C++ actions from C code
- `src/actions/modes/` - Mode implementations (StatefulMode pattern)

To add a new action:
1. Create a class inheriting from `pcb::actions::Action` in `src/actions/`
2. Implement the `execute()` method
3. Use `REGISTER_ACTION(YourClass)` macro for auto-registration
4. Add to `PCB_CXX_SRCS` in `src/Makefile.am`

### Key Source Files

- `src/global.h` - Global type definitions and data structures
- `src/data.h` - PCB data structures (PCBType, LayerType, etc.)
- `src/action.c` / `src/action.h` - Action system (being refactored)
- `src/set.c` / `src/set.h` - Mode setting functions
- `src/crosshair.c` - Crosshair and attached object handling

### DRC (Design Rule Check)

DRC implementation is in `src/drc/`:
- `drc.c` / `drc.h` - Main DRC engine
- `drc_violation.c` / `drc_violation.h` - Violation reporting
- `drc_object.h` - Object definitions for DRC

## Testing

### Regression Tests

Tests are defined in `tests/tests.list` and use golden file comparison:

```bash
# Run all tests
cd tests && ./run_tests.sh

# Run specific test
./run_tests.sh hid_gerber1 hid_png1

# Regenerate golden files (verify manually!)
./run_tests.sh --regen <test_name>
```

### C++ Unit Tests

Requires Google Test. Build with:
```bash
./configure --disable-doc  # GTest auto-detected
make
cd tests/cpp && ./unittest_cpp
```

## Code Style

- C code: C99 standard
- C++ code: C++11 minimum (C++17 for tests with GTest)
- Compiler flags: `-Wall -Wextra` for C++, `-Wall -Wdeclaration-after-statement` for C

## Mixed C/C++ Compilation

The build system handles mixed compilation:
- `.c` files compiled with `gcc`
- `.cpp` files compiled with `g++`
- Linking done with C++ linker
- Use `extern "C"` in bridge headers for C compatibility

## Key Data Types

- `Coord` - Coordinate type (configurable 32/64-bit via `--enable-coord32/64`)
- `PCBType` - Main PCB data structure
- `LayerType`, `LineType`, `ArcType`, `PolygonType` - Layer elements
- `ElementType` - Component footprint
- `PinType`, `PadType`, `ViaType` - Connection points
