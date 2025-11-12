# PCB - Printed Circuit Board Design Tool

[![Build Status](https://github.com/technobauble/pcb/workflows/Build%20and%20Test/badge.svg)](https://github.com/technobauble/pcb/actions)
[![License: GPL v2+](https://img.shields.io/badge/License-GPL%20v2+-blue.svg)](COPYING)

PCB is a powerful, feature-rich CAD (Computer Aided Design) program for the physical design of printed circuit boards. It has been in active development since 1994 and is part of the [gEDA project](http://www.geda-project.org/) ecosystem.

## Features

- **Multi-layer PCB design** with unlimited layers
- **Autorouting** with multiple routing algorithms including topological router
- **Design Rule Check (DRC)** engine for validation
- **Multiple export formats**: Gerber RS-274X, G-code, PNG, PostScript, BOM, and more
- **Component libraries** with thousands of footprints
- **Cross-platform** support (Linux, macOS, Windows)
- **Multiple GUI options**: GTK+, Lesstif/Motif, or headless batch mode
- **Python/scripting support** for automation
- **OpenGL accelerated rendering** (optional)

## Quick Start

### Using Docker (Recommended for Development)

The fastest way to get started:

```bash
# Clone the repository
git clone https://github.com/technobauble/pcb.git
cd pcb

# Start development environment
docker-compose up -d
docker-compose exec pcb-dev bash

# Inside container: build and test
./autogen.sh
./configure --disable-doc
make -j$(nproc)
make check
```

### Building from Source

#### Prerequisites

**Ubuntu/Debian:**
```bash
sudo apt-get install build-essential autoconf automake autopoint \
  gettext intltool flex bison m4 gawk \
  libglib2.0-dev libgtk2.0-dev libgtkglext1-dev libgd-dev \
  gerbv imagemagick wish
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc autoconf automake gettext-devel intltool \
  flex bison m4 gawk \
  glib2-devel gtk2-devel gtkglext-devel gd-devel \
  gerbv ImageMagick tk
```

**macOS:**
```bash
brew install autoconf automake gettext intltool \
  glib gtk+ gtkglext gd gerbv imagemagick tcl-tk
```

#### Build Steps

```bash
# Generate configure script
./autogen.sh

# Configure (see below for options)
./configure --disable-doc

# Build
make -j$(nproc)

# Run tests
make check

# Install (optional)
sudo make install
```

#### Build Options

- `--with-gui=gtk` - GTK+ GUI (default, recommended)
- `--with-gui=lesstif` - Lesstif/Motif GUI
- `--with-gui=batch` - Headless batch mode only
- `--enable-gl` - Enable OpenGL acceleration
- `--disable-doc` - Skip documentation (faster builds)
- `--prefix=/usr/local` - Installation directory

## Installation

### From Package Managers

**Debian/Ubuntu:**
```bash
sudo apt-get install pcb
```

**Fedora:**
```bash
sudo dnf install pcb
```

**macOS:**
```bash
brew install pcb
```

> **Note:** Package manager versions may be older than the latest development version. Building from source gives you the latest features.

## Documentation

- **User Manual**: See `doc/pcb.pdf` or run `man pcb`
- **Getting Started Guide**: See `doc/gs/`
- **Tutorial**: See `tutorial/` directory
- **Contributing**: See [CONTRIBUTING.md](CONTRIBUTING.md)
- **Online Documentation**: [PCB Documentation](http://pcb.geda-project.org)

## Usage Examples

### Basic Usage

```bash
# Start PCB with GTK GUI
pcb

# Open existing design
pcb myboard.pcb

# Batch mode (export Gerber files)
pcb -x gerber myboard.pcb

# Export PNG
pcb -x png --dpi 600 --outfile board.png myboard.pcb
```

### Scripting

PCB can be automated for batch processing:

```bash
# Export all layers to individual Gerber files
pcb -x gerber --all-layers design.pcb

# Generate Bill of Materials
pcb -x bom --bomfile output.csv design.pcb

# Generate G-code for milling
pcb -x gcode --dpi 1200 design.pcb
```

## Project Status

**Latest Release:** v4.2.0 (February 2019)

**Current Development:** The project is undergoing modernization efforts including:
- Migration to GitHub Actions CI/CD
- Docker-based development environment
- Code quality improvements
- Removal of legacy artifacts
- Future: GTK3 migration, improved build system

See [RENEWAL_PROPOSAL.md](RENEWAL_PROPOSAL.md) for the complete modernization roadmap.

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for:
- Development environment setup
- Code style guidelines
- Git workflow
- Submitting pull requests
- Reporting bugs

### Quick Contribution Guide

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/my-feature`)
3. Make your changes and add tests
4. Ensure tests pass (`make check`)
5. Commit with clear messages
6. Push and open a Pull Request

## Support

- **Bug Reports**: [GitHub Issues](https://github.com/technobauble/pcb/issues)
- **Discussions**: [GitHub Discussions](https://github.com/technobauble/pcb/discussions)
- **Website**: http://pcb.geda-project.org

## File Format

PCB uses a custom ASCII file format that is human-readable and version-control friendly. It also supports:
- **EDIF** netlist import
- **gschem** netlist integration (gEDA project)
- Various export formats (Gerber, G-code, PNG, PS, BOM, etc.)

## Architecture

PCB uses a modular **HID (Hardware Interface Driver)** architecture:

- **GUI HIDs**: GTK+, Lesstif/Motif, Batch (headless)
- **Export HIDs**: Gerber, PNG, PostScript, G-code, BOM, etc.
- **Core Engine**: Platform-independent PCB design logic

This design makes it easy to add new GUIs or export formats without modifying the core.

## Testing

PCB has a comprehensive test suite with 379+ test cases:

```bash
# Run all tests
make check

# Run specific test
cd tests
./run_tests.sh test_name

# View test results
cat tests/test-suite.log
```

## License

PCB is licensed under the **GNU General Public License v2.0 or later**. See [COPYING](COPYING) for details.

Individual source files contain specific copyright notices and licensing information.

## Credits

PCB has been developed by many contributors over 25+ years:

- **Original Author**: Thomas Nau (1994-1996)
- **Major Development**: Harry Eaton (1997-2001)
- **GTK Port**: Bill Wilson
- **Current Maintainers**: See AUTHORS file

## Historical Context

PCB has a rich 25-year history:
- **1994**: Initial development by Thomas Nau
- **1997-2001**: Major development by Harry Eaton
- **2000s**: Integration with gEDA project
- **2010s**: GTK2 GUI, topological autorouter, extensive testing
- **2019**: Last major release (v4.2.0)
- **2025**: Modernization and renewal efforts

## Alternatives

Other PCB design tools you might consider:
- **KiCad** - Modern, actively developed, large community
- **gEDA/gaf** - Other tools in the gEDA ecosystem
- **Fritzing** - Beginner-friendly, breadboard view
- **Eagle** - Commercial (now owned by Autodesk)

PCB remains valuable for:
- Users who prefer lightweight tools
- Integration with gEDA workflow
- Scriptable batch processing
- Open-source licensing

## Development Roadmap

See [RENEWAL_PROPOSAL.md](RENEWAL_PROPOSAL.md) for detailed modernization plans:

**Phase 1 (Current)**: Infrastructure modernization
- ✅ GitHub Actions CI/CD
- ✅ Docker development environment
- ✅ CVS artifacts cleanup
- ✅ Modern documentation

**Phase 2 (Planned)**: Dependency updates
- GTK3 migration
- Meson build system
- Updated dependencies

**Phase 3 (Planned)**: Code quality improvements
- Remove obsolete patterns
- Expand test coverage
- Static analysis integration

## Contact

For questions, suggestions, or contributions:

- **GitHub**: https://github.com/technobauble/pcb
- **Website**: http://pcb.geda-project.org
- **Original Author Email**: haceaton@aplcomm.jhuapl.edu (Harry Eaton, 1997-2001)

---

**Note**: For installation instructions, see [INSTALL](INSTALL). For detailed changelog, see [ChangeLog](ChangeLog) or git commit history.
