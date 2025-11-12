# PCB CAD Application Renewal Proposal

**Date:** November 2025
**Project:** PCB - Printed Circuit Board CAD Software
**Last Release:** v4.2.0 (February 2019)
**Project Age:** 25+ years (1994-2019)
**Status:** Dormant since 2019 (0 commits since 2020)

---

## Executive Summary

PCB is a mature, feature-rich CAD application for printed circuit board design with a 25-year history and a strong foundation. However, the project has been dormant for approximately 6 years, and several aspects of its technology stack have become outdated. This proposal outlines a comprehensive renewal strategy to modernize the codebase, update dependencies, improve developer experience, and position the project for sustainable long-term development.

**Key Recommendation:** Pursue a phased modernization approach that balances risk with benefit, prioritizing infrastructure updates, code quality improvements, and community re-engagement.

---

## Current State Assessment

### Strengths

1. **Solid Architecture**
   - Clean HID (Hardware Interface Driver) plugin architecture
   - Well-separated concerns between core logic and UI/export layers
   - 14 different HID implementations (2 GUIs, 8 exporters, etc.)

2. **Comprehensive Feature Set**
   - Full-featured PCB design capabilities
   - Multiple export formats (Gerber, G-code, PNG, PS, BOM, etc.)
   - Advanced features: autorouter, toporouter, DRC engine
   - Embedded GTS library for computational geometry

3. **Quality Engineering Practices**
   - 379 automated regression tests with golden file methodology
   - Extensive documentation (Texinfo manual, man pages, Doxygen)
   - C99 standard compliance
   - ~128,000 lines of well-structured C code

4. **Production-Ready**
   - Stable file format (custom ASCII + EDIF support)
   - Active community usage (part of gEDA ecosystem)
   - Cross-platform support (Linux, Windows, macOS)

### Critical Issues

1. **Dormant Development**
   - **0 commits since January 2020** (6 years of inactivity)
   - Last release: v4.2.0 (February 2019)
   - Risk of dependency incompatibilities with modern systems

2. **Outdated Technology Stack**
   - **GTK+ 2.0** (EOL: GTK 2.24 released in 2011, GTK 3 released 2011, GTK 4 released 2020)
   - **Travis CI** (free tier discontinued in 2020, requires migration)
   - **Autotools** (functional but verbose, modern alternatives exist)

3. **Legacy Code Artifacts**
   - 15 `.cvsignore` files (CVS → Git migration incomplete)
   - 15 files using obsolete `register` keyword
   - 177 TODO/FIXME comments across 45 files
   - Code patterns and comments from 1990s-2000s era

4. **Build System Complexity**
   - M4 macro-based component library system (unconventional)
   - Heavy documentation build requirements (LaTeX, Texinfo)
   - Platform-specific builds (win32, w32 directories)

5. **Modern Development Gaps**
   - No GitHub Actions / modern CI
   - No container-based development environment
   - Limited contribution guidelines
   - Potential security vulnerabilities in aged dependencies

---

## Renewal Goals

### Primary Objectives

1. **Restore Active Development**
   - Re-establish development momentum
   - Create welcoming environment for new contributors
   - Modernize development workflows

2. **Ensure Long-Term Viability**
   - Update to actively maintained dependencies
   - Improve build system maintainability
   - Establish sustainable CI/CD pipeline

3. **Enhance Code Quality**
   - Remove legacy artifacts
   - Address technical debt
   - Improve code documentation and testing

4. **Maintain Backward Compatibility**
   - Preserve file format compatibility
   - Maintain existing functionality
   - Ensure smooth upgrade path for users

5. **Expand Platform Support**
   - Verify Windows/macOS builds still work
   - Add container-based deployment
   - Improve cross-platform build experience

---

## Proposed Modernization Roadmap

### Phase 1: Foundation & Infrastructure (2-3 months)

**Priority: CRITICAL - Enable modern development**

#### 1.1 Version Control Cleanup
- [ ] Remove all 15 `.cvsignore` files
- [ ] Delete obsolete CVS documentation (`README.cvs_branches`)
- [ ] Remove `utils/cvs2cl.pl` script
- [ ] Clean up CVS references in `configure.ac`
- [ ] Audit and update `.gitignore` for completeness

#### 1.2 CI/CD Modernization
- [ ] **Migrate from Travis CI to GitHub Actions**
  - Create `.github/workflows/` directory
  - Implement multi-platform build matrix (Ubuntu, macOS, Windows)
  - Add separate workflows for:
    - Build verification
    - Test suite execution
    - Documentation generation
    - Release packaging
- [ ] **Add modern build configurations**
  - Docker/Podman containerized builds
  - Reproducible build environments
  - Caching strategies for dependencies
- [ ] **Implement continuous testing**
  - Run test suite on every PR
  - Generate test coverage reports
  - Visual diff testing for export formats

#### 1.3 Development Environment
- [ ] Create `Dockerfile` and `docker-compose.yml`
- [ ] Add VS Code devcontainer configuration
- [ ] Document local development setup
- [ ] Create quick-start guide for contributors

#### 1.4 Documentation Updates
- [ ] Create `CONTRIBUTING.md` with:
  - Code style guidelines
  - Git workflow (feature branches, PRs)
  - Testing requirements
  - Commit message conventions
- [ ] Update `README.md` with:
  - Current project status
  - Quick start instructions
  - Build instructions for modern systems
  - Screenshot/demo
- [ ] Add `SECURITY.md` for vulnerability reporting
- [ ] Create `CODE_OF_CONDUCT.md`

**Deliverables:**
- Green CI builds on GitHub Actions
- Containerized development environment
- Updated contribution guidelines

---

### Phase 2: Dependency Modernization (3-4 months)

**Priority: HIGH - Update aging dependencies**

#### 2.1 GTK Migration Strategy

**Option A: GTK3 Migration (Recommended)**
- GTK3 is mature (2011-2021), well-documented, widely supported
- Moderate porting effort (~2-3 months)
- Good balance of modernity and stability
- Strong backward compatibility guarantees

**Option B: GTK4 Migration (Future-proof)**
- GTK4 is current (2020+), modern API design
- Larger porting effort (~4-6 months)
- Maximum longevity
- May require more extensive refactoring

**Recommended Approach:**
1. Start with GTK3 migration (incremental updates)
2. Keep Lesstif HID temporarily as fallback
3. Plan GTK4 migration as separate future phase

**Migration Tasks:**
- [ ] Audit GTK2 API usage across 24 GTK HID files
- [ ] Create GTK3/GTK4 compatibility layer
- [ ] Update deprecated widget usage:
  - GtkTable → GtkGrid
  - GtkVBox/GtkHBox → GtkBox
  - Stock items → named icons
  - GtkUIManager → GMenuModel/GtkBuilder
- [ ] Update GtkGLExt to modern OpenGL context creation
- [ ] Test all GUI features in new GTK version
- [ ] Update screenshot/documentation

#### 2.2 Build System Evaluation

**Option A: Keep Autotools (Conservative)**
- Minimal disruption
- Well-understood by existing users
- Continue maintenance

**Option B: Migrate to Meson (Modern)**
- Faster builds (parallel by default)
- Better cross-platform support
- Cleaner, more maintainable syntax
- Recommended by GTK project
- Growing ecosystem adoption

**Recommended Approach:**
1. Start with Meson migration (3-4 weeks effort)
2. Keep Autotools alongside temporarily
3. Phase out Autotools after community validation

**Migration Tasks:**
- [ ] Create `meson.build` files throughout tree
- [ ] Migrate M4 library generation to Python scripts
- [ ] Update documentation build integration
- [ ] Verify all configure options preserved
- [ ] Test cross-compilation scenarios

#### 2.3 Dependency Updates
- [ ] Update minimum versions:
  - GLib 2.56+ (2018)
  - Cairo 1.16+ (2018)
  - GTK 3.24+ (2019)
- [ ] Audit embedded libraries:
  - GTS library (v0.7.6 from 2006) - check for updates
  - Consider using system libraries where possible
- [ ] Review and update optional dependencies:
  - D-Bus (current)
  - ImageMagick (for tests)
  - Gerbv (for validation)

**Deliverables:**
- Working GTK3-based build
- Modern build system (Meson preferred)
- Updated dependency documentation

---

### Phase 3: Code Quality & Technical Debt (2-3 months)

**Priority: MEDIUM - Clean up legacy code**

#### 3.1 Code Modernization
- [ ] **Remove obsolete C keywords**
  - Remove `register` keyword from 15 files
  - Audit for other deprecated patterns
- [ ] **Address compiler warnings**
  - Enable stricter warning flags (-Wextra, -Wpedantic)
  - Fix all warnings in modern compilers (GCC 11+, Clang 15+)
  - Add `-Werror` to CI builds
- [ ] **TODO/FIXME Triage**
  - Review 177 TODO/FIXME comments
  - Convert valid issues to GitHub Issues
  - Remove obsolete comments
  - Fix quick wins

#### 3.2 Code Quality Tools
- [ ] **Add static analysis**
  - Integrate `clang-tidy` checks
  - Add `cppcheck` scanning
  - Scan with `scan-build` analyzer
- [ ] **Add dynamic analysis**
  - Valgrind memory leak detection
  - AddressSanitizer (ASAN) builds
  - UndefinedBehaviorSanitizer (UBSAN)
- [ ] **Code formatting**
  - Add `.clang-format` configuration
  - Format all source files consistently
  - Add format checking to CI
- [ ] **Security scanning**
  - Integrate CodeQL or similar
  - Dependency vulnerability scanning
  - Regular security audits

#### 3.3 Testing Improvements
- [ ] **Expand test coverage**
  - Add unit tests for core modules
  - Test coverage measurement (gcov/lcov)
  - Set coverage targets (aim for 70%+)
- [ ] **Improve test infrastructure**
  - Parallelize test execution
  - Add test result visualization
  - Reduce test execution time
- [ ] **Add integration tests**
  - End-to-end workflow tests
  - File format round-trip tests
  - Export format validation

#### 3.4 Documentation Enhancement
- [ ] **API Documentation**
  - Complete Doxygen coverage
  - Generate and publish API docs
  - Add architecture diagrams
- [ ] **User Documentation**
  - Convert Texinfo to Markdown/Sphinx
  - Add modern screenshots
  - Create video tutorials
- [ ] **Developer Documentation**
  - Document HID architecture
  - Explain plugin system
  - Add code examples

**Deliverables:**
- Clean, modernized C codebase
- Automated quality checks in CI
- Improved test coverage (70%+ target)

---

### Phase 4: Feature & UX Improvements (3-6 months)

**Priority: LOW-MEDIUM - Enhance user experience**

#### 4.1 Modern UX Enhancements
- [ ] **UI improvements**
  - Dark theme support
  - High-DPI display support
  - Improved icon set
  - Keyboard shortcut customization
- [ ] **Workflow improvements**
  - Undo/redo enhancements
  - Real-time DRC checking
  - Improved component library browser
  - Better netlist integration
- [ ] **Export improvements**
  - PDF export support
  - SVG export improvements
  - 3D export (STEP/STL)

#### 4.2 Component Library Modernization
- [ ] **Replace M4 macro system**
  - Evaluate modern footprint formats (KiCad, gEDA PCB)
  - Consider JSON-based library format
  - Build library conversion tools
- [ ] **Online library integration**
  - Component search integration
  - Popular footprint library imports
  - Library management GUI

#### 4.3 Interoperability
- [ ] **Import/Export enhancements**
  - KiCad file format support (import/export)
  - Eagle file format support
  - Improved EDIF handling
- [ ] **Integration capabilities**
  - REST API for automation
  - Python scripting support
  - CLI improvements for headless operation

#### 4.4 Modern Features
- [ ] **Advanced routing**
  - Push-and-shove routing
  - Differential pair routing
  - Length matching
- [ ] **Design rule improvements**
  - Impedance control
  - Signal integrity checks
  - Manufacturing design rules

**Deliverables:**
- Enhanced user interface
- Modern component library system
- Improved interoperability

---

### Phase 5: Community & Sustainability (Ongoing)

**Priority: HIGH - Ensure long-term success**

#### 5.1 Community Building
- [ ] **Communication channels**
  - Set up GitHub Discussions
  - Create/revive mailing list
  - Consider Discord/Matrix chat
  - Regular development updates
- [ ] **Contributor onboarding**
  - Good first issue tags
  - Mentorship program
  - Contributor recognition
- [ ] **Project governance**
  - Define maintainer roles
  - Contribution review process
  - Release management procedures

#### 5.2 Release Management
- [ ] **Establish release cadence**
  - Regular releases (quarterly or semi-annual)
  - Clear versioning scheme (SemVer)
  - Release notes automation
- [ ] **Distribution**
  - Update package repositories (Debian, Fedora, etc.)
  - Flatpak/Snap packages
  - Windows installer
  - macOS DMG/Homebrew

#### 5.3 Marketing & Outreach
- [ ] **Project website**
  - Modern project website
  - Feature showcase
  - Tutorial content
  - News/blog
- [ ] **Social media**
  - Regular project updates
  - Tutorial videos
  - User showcase
- [ ] **Documentation**
  - Getting started guide
  - Video tutorials
  - Example projects

**Deliverables:**
- Active community engagement
- Regular release schedule
- Growing contributor base

---

## Priority Matrix

| Phase | Priority | Effort | Impact | Risk | Timeline |
|-------|----------|--------|--------|------|----------|
| Phase 1: Infrastructure | CRITICAL | Medium | High | Low | Months 1-3 |
| Phase 2: Dependencies | HIGH | High | High | Medium | Months 4-7 |
| Phase 3: Code Quality | MEDIUM | Medium | Medium | Low | Months 8-10 |
| Phase 4: Features | LOW-MEDIUM | High | Medium | Low | Months 11-16 |
| Phase 5: Community | HIGH | Medium | High | Low | Ongoing |

### Recommended Fast-Track Items (First 90 Days)

1. **CI/CD Migration** (Week 1-2)
   - GitHub Actions setup
   - Basic build verification

2. **CVS Cleanup** (Week 2)
   - Remove all CVS artifacts
   - One-time cleanup task

3. **Container Environment** (Week 3-4)
   - Dockerfile for development
   - Simplifies onboarding

4. **Documentation** (Week 4-6)
   - README update
   - CONTRIBUTING.md
   - Modern screenshots

5. **Security Scan** (Week 6-8)
   - Vulnerability assessment
   - Critical fixes

6. **Build System Evaluation** (Week 8-12)
   - Meson proof-of-concept
   - Community feedback

---

## Risk Assessment

### Technical Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| GTK3/4 migration breaks features | Medium | High | Comprehensive testing, keep GTK2 branch temporarily |
| Build system migration issues | Medium | Medium | Parallel maintenance of both systems initially |
| Dependency conflicts on old systems | Low | Medium | Clear minimum system requirements, containers |
| Test suite failures | Medium | Low | Fix tests incrementally, maintain golden files |
| Community resistance to changes | Low | Medium | Clear communication, gradual rollout |

### Project Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Lack of maintainer availability | Medium | High | Build contributor base early, distributed ownership |
| User migration to alternatives | Medium | High | Prioritize stability, clear migration path |
| Scope creep | Medium | Medium | Phased approach, clear goals per phase |
| Funding constraints | High | Medium | Focus on volunteer effort, seek sponsorship |
| Breaking backward compatibility | Low | High | Extensive testing, clear version communication |

---

## Success Metrics

### Phase 1-2 (Months 1-7)
- [ ] Green CI builds on 3+ platforms
- [ ] 5+ external contributions accepted
- [ ] All CVS artifacts removed
- [ ] GTK3 migration 100% complete
- [ ] Test suite passes at 95%+

### Phase 3-4 (Months 8-16)
- [ ] Code coverage >70%
- [ ] Zero critical compiler warnings
- [ ] 50+ issues triaged/closed
- [ ] 10+ new features implemented
- [ ] User documentation updated

### Phase 5 (Ongoing)
- [ ] Regular releases (2-3 per year)
- [ ] Active community (20+ contributors)
- [ ] 100+ stars on GitHub
- [ ] Growing user base (measurable downloads)
- [ ] Positive community feedback

---

## Resource Requirements

### Team Composition (Ideal)

- **1 Lead Maintainer** (20 hrs/week)
  - Overall project direction
  - Code review and architecture decisions
  - Community management

- **2-3 Core Contributors** (10 hrs/week each)
  - Feature implementation
  - Bug fixes
  - Code reviews

- **5-10 Community Contributors** (variable)
  - Bug reports
  - Small features
  - Documentation
  - Testing

### Infrastructure

- **GitHub Organization** (free tier)
  - Code hosting
  - Issue tracking
  - CI/CD (Actions)
  - Discussions

- **Optional:**
  - Domain for project website ($10-20/year)
  - Hosting for documentation (GitHub Pages free)
  - Communication platform (Discord/Matrix free)

---

## Alternative Approaches

### Option 1: Minimal Maintenance
**Focus:** Keep project working on modern systems with minimal changes

**Pros:**
- Low effort
- Low risk
- Preserves stability

**Cons:**
- Continued technical debt accumulation
- Limited new contributor attraction
- Eventual incompatibility with modern systems

**Verdict:** Not recommended - delays inevitable modernization

---

### Option 2: Fork & Rewrite
**Focus:** Start fresh with modern technology (e.g., Rust, Qt, web-based)

**Pros:**
- Clean slate
- Modern architecture
- No legacy constraints

**Cons:**
- Enormous effort (years)
- Loss of existing features
- Community fragmentation
- File format compatibility challenges

**Verdict:** Too risky - existing codebase is solid

---

### Option 3: Phased Modernization (RECOMMENDED)
**Focus:** Incremental updates while maintaining stability

**Pros:**
- Balanced risk/reward
- Maintains compatibility
- Attracts new contributors
- Preserves existing work

**Cons:**
- Moderate ongoing effort
- Some difficult migrations (GTK)

**Verdict:** Best approach - achievable and sustainable

---

## Conclusion

The PCB project has a solid foundation built over 25 years of development. While it has been dormant for 6 years, the codebase remains well-structured and feature-rich. The primary challenges are outdated dependencies and aging development practices, both of which are addressable through systematic modernization.

### Key Recommendations

1. **Adopt phased modernization approach** - Incremental improvements reduce risk
2. **Prioritize infrastructure first** - CI/CD and development environment enable everything else
3. **Migrate to GTK3** - Balance of modernity and effort
4. **Consider Meson build system** - Significant maintainability improvement
5. **Build community early** - Essential for long-term sustainability

### Next Steps

1. **Review and approve proposal** - Gather stakeholder feedback
2. **Identify initial maintainers** - Who will drive Phase 1?
3. **Create Phase 1 GitHub project board** - Track infrastructure work
4. **Announce renewal effort** - Communicate to community
5. **Start with quick wins** - CI setup, CVS cleanup (Weeks 1-2)

With focused effort over 12-18 months, this project can be successfully modernized while maintaining its strengths and serving its user community for another decade.

---

**Prepared by:** Claude (AI Assistant)
**Date:** November 12, 2025
**Version:** 1.0
**Status:** Draft for Review
