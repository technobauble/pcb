# Contributing to PCB

Thank you for your interest in contributing to PCB! This document provides guidelines and instructions for contributing to the project.

## Table of Contents

- [Getting Started](#getting-started)
- [Development Environment](#development-environment)
- [Building from Source](#building-from-source)
- [Running Tests](#running-tests)
- [Code Style Guidelines](#code-style-guidelines)
- [Git Workflow](#git-workflow)
- [Submitting Changes](#submitting-changes)
- [Reporting Bugs](#reporting-bugs)
- [Feature Requests](#feature-requests)
- [Code Review Process](#code-review-process)

## Getting Started

1. **Fork the repository** on GitHub
2. **Clone your fork** locally:
   ```bash
   git clone https://github.com/YOUR-USERNAME/pcb.git
   cd pcb
   ```
3. **Add upstream remote**:
   ```bash
   git remote add upstream https://github.com/technobauble/pcb.git
   ```

## Development Environment

### Option 1: Docker (Recommended)

The easiest way to get started is using our Docker development environment:

```bash
# Build and start the development container
docker-compose up -d

# Enter the container
docker-compose exec pcb-dev bash

# Inside the container, build the project
./autogen.sh
./configure --disable-doc
make -j$(nproc)
make check
```

### Option 2: Local Installation

Install the required dependencies on your system:

**Ubuntu/Debian:**
```bash
sudo apt-get install \
  build-essential autoconf automake autopoint gettext intltool \
  flex bison m4 gawk \
  libglib2.0-dev libgtk2.0-dev libgtkglext1-dev libgd-dev \
  gerbv imagemagick wish
```

**Fedora/RHEL:**
```bash
sudo dnf install \
  gcc autoconf automake gettext-devel intltool \
  flex bison m4 gawk \
  glib2-devel gtk2-devel gtkglext-devel gd-devel \
  gerbv ImageMagick tk
```

**macOS:**
```bash
brew install \
  autoconf automake gettext intltool \
  glib gtk+ gtkglext gd \
  gerbv imagemagick tcl-tk
```

## Building from Source

```bash
# Generate the configure script
./autogen.sh

# Configure the build
./configure --disable-doc --disable-dbus --disable-update-desktop-database

# Build (use all available CPU cores)
make -j$(nproc)

# Run tests
make check

# Optional: Install locally
sudo make install
```

### Build Options

Common configure options:

- `--with-gui=gtk` - Build GTK GUI (default)
- `--with-gui=lesstif` - Build Lesstif GUI
- `--with-gui=batch` - Build headless batch mode only
- `--enable-gl` - Enable OpenGL support (requires GtkGLExt)
- `--disable-doc` - Skip documentation building (faster builds)
- `--disable-dbus` - Disable D-Bus support
- `--prefix=/usr/local` - Installation prefix

## Running Tests

```bash
# Run all tests
make check

# Run specific test
cd tests
./run_tests.sh test_name

# Regenerate golden files (if you intentionally changed output)
./run_tests.sh --regen test_name
```

## Code Style Guidelines

### C Code Style

- **Standard**: C99 compliant code
- **Indentation**: Tabs (width 8) for indentation, spaces for alignment
- **Braces**: K&R style (opening brace on same line, except for functions)
- **Line length**: Try to keep lines under 100 characters
- **Comments**: Use `/* */` for multi-line, `//` for single-line is acceptable

**Example:**
```c
/* Function documentation comment */
int
my_function(int param1, const char *param2)
{
	if (condition) {
		do_something();
	} else {
		do_something_else();
	}

	return result;
}
```

### Naming Conventions

- **Functions**: `snake_case` or `PascalCase` (follow existing patterns in the file)
- **Variables**: `snake_case`
- **Constants/Macros**: `UPPER_CASE`
- **Struct types**: `PascalCase` or `snake_case_t`

### Modern C Practices

**DO:**
- Use `bool` from `<stdbool.h>`
- Use fixed-width types from `<stdint.h>` when size matters
- Use `const` for read-only parameters
- Check return values of functions that can fail
- Free allocated memory properly
- Initialize variables when declared

**DON'T:**
- Use obsolete `register` keyword (being removed from codebase)
- Use deprecated functions (strcat, strcpy - use safer alternatives)
- Ignore compiler warnings
- Use magic numbers (define named constants instead)

### Documentation

- Add Doxygen-style comments for public functions:
  ```c
  /*!
   * \brief Brief description
   * \param param1 Description of param1
   * \param param2 Description of param2
   * \return Description of return value
   */
  ```

## Git Workflow

### Branch Naming

- Feature branches: `feature/short-description`
- Bug fixes: `bugfix/issue-number-short-description`
- Refactoring: `refactor/short-description`

### Commit Messages

Follow the conventional commit format:

```
<type>: <short summary> (50 chars or less)

<detailed description if needed>
<wrapped at 72 characters>

Fixes: #123
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, no logic change)
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `chore`: Maintenance tasks, dependency updates
- `perf`: Performance improvements

**Examples:**
```
feat: Add dark theme support to GTK GUI

Implements user-requested dark theme by detecting system theme
preference and applying appropriate CSS classes.

Fixes: #456
```

```
fix: Correct DRC clearance calculation for curved traces

The DRC engine was incorrectly calculating clearances for traces
with tight curves, causing false positives.

Fixes: #789
```

### Keeping Your Fork Updated

```bash
# Fetch upstream changes
git fetch upstream

# Update your main branch
git checkout master
git merge upstream/master

# Rebase your feature branch (if needed)
git checkout your-feature-branch
git rebase master
```

## Submitting Changes

1. **Create a feature branch** from `master`:
   ```bash
   git checkout -b feature/my-new-feature master
   ```

2. **Make your changes** with clear, logical commits

3. **Test your changes**:
   ```bash
   make check
   ```

4. **Update documentation** if needed

5. **Push to your fork**:
   ```bash
   git push origin feature/my-new-feature
   ```

6. **Open a Pull Request** on GitHub:
   - Provide clear title and description
   - Reference any related issues
   - Describe what you changed and why
   - Include screenshots for UI changes

### Pull Request Checklist

Before submitting a PR, ensure:

- [ ] Code compiles without warnings
- [ ] All tests pass (`make check`)
- [ ] New tests added for new features
- [ ] Documentation updated if needed
- [ ] Code follows project style guidelines
- [ ] Commit messages are clear and descriptive
- [ ] No unnecessary whitespace changes
- [ ] Changes are focused (one feature/fix per PR)

## Reporting Bugs

When reporting bugs, please include:

1. **PCB version** (from `pcb --version`)
2. **Operating system** and version
3. **Steps to reproduce** the issue
4. **Expected behavior**
5. **Actual behavior**
6. **Relevant files** (PCB design files if applicable)
7. **Screenshots** or error messages

Use the GitHub issue tracker and apply appropriate labels.

## Feature Requests

Feature requests are welcome! Please:

1. Check if the feature already exists or is requested
2. Clearly describe the feature and use case
3. Explain why it would be valuable
4. Consider whether you can implement it yourself

## Code Review Process

### For Contributors

- Be responsive to review comments
- Address all feedback or explain why you disagree
- Keep discussions professional and constructive
- Be patient - reviewers are volunteers

### For Reviewers

- Be respectful and constructive
- Focus on the code, not the person
- Explain why changes are needed
- Recognize good work
- Approve when ready, suggest improvements for follow-up

### Review Focus Areas

Reviewers will check:

- **Correctness**: Does it work as intended?
- **Testing**: Are there adequate tests?
- **Style**: Does it follow coding guidelines?
- **Performance**: Any performance concerns?
- **Security**: Any security implications?
- **Documentation**: Is it documented?
- **Backward compatibility**: Does it break existing functionality?

## Development Tips

### Debugging

```bash
# Build with debug symbols
./configure --enable-debug
make clean && make

# Run with GDB
gdb --args ./src/pcb [arguments]

# Check for memory leaks
valgrind --leak-check=full ./src/pcb [arguments]
```

### Code Analysis

```bash
# Static analysis with cppcheck
cppcheck --enable=all src/

# Check with clang-tidy
clang-tidy src/*.c -- $(pkg-config --cflags glib-2.0 gtk+-2.0)
```

### Understanding the Code

Key directories:

- `src/` - Core application logic
- `src/hid/` - Hardware Interface Drivers (UI and export formats)
- `src/hid/gtk/` - GTK GUI implementation
- `src/hid/gerber/` - Gerber export
- `gts/` - Embedded geometry library
- `lib/` - Legacy component library (M4-based)
- `newlib/` - Modern component library
- `tests/` - Test suite

## Getting Help

- **GitHub Discussions**: For questions and general discussion
- **Issues**: For bugs and feature requests
- **Documentation**: See `doc/pcb.texi` for user manual

## License

By contributing, you agree that your contributions will be licensed under the same license as the project (GPL v2 or later). See the `COPYING` file for details.

---

Thank you for contributing to PCB! Your efforts help make PCB better for everyone.
