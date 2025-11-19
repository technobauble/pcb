# Global.h Analysis - What Do Actions Actually Need?

**Date:** November 14, 2025
**Purpose:** Determine minimal refactoring needed for action isolation

---

## The Discovery

**Action.h only uses ONE thing from global.h:**
```cpp
virtual int execute(int argc, char** argv, Coord x, Coord y) = 0;
                                          ^^^^      ^^^^
```

That's it! Just `Coord`.

---

## Current Dependency Chain

```
Action.h
  → #include "global.h"           (30KB, 1000+ lines)
    → #include "const.h"
    → #include "globalconst.h"     (auto-generated)
    → #include <glib.h>
    → #include "hid.h"             (500+ lines)
    → #include "polyarea.h"
    → #include "drc/drc_violation.h"
    → ... (20+ more headers)
```

**All for one typedef!**

---

## What Coord Actually Is

From `configure.ac`:
```bash
case "$enable_coord32:$enable_coord64:$ac_cv_header_stdint_h" in
  yes:no:yes ) COORD_TYPE="int32_t" ;;
  no:yes:yes ) COORD_TYPE="int64_t" ;;
  yes:no:no  ) COORD_TYPE="int" ;;
  no:yes:no  ) COORD_TYPE="long long" ;;
esac
```

From `global.h:79`:
```c
typedef COORD_TYPE Coord;  /*!< pcb base unit. */
```

So `Coord` is basically `int32_t` (or `int64_t` for large boards).

---

## Proposed Solution: Extract Coord

### Option 4: Create coord_types.h (RECOMMENDED)

**New file:** `src/coord_types.h`

```c
/*!
 * \file src/coord_types.h
 * \brief Minimal coordinate type definitions for PCB.
 *
 * This header provides only the coordinate types without pulling in
 * all of global.h. Use this when you only need Coord/Angle types.
 */

#ifndef PCB_COORD_TYPES_H
#define PCB_COORD_TYPES_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Ensure we have stdint.h types if available */
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

/* Coordinate type - configured by autotools */
#ifndef COORD_TYPE
  #ifdef HAVE_STDINT_H
    #define COORD_TYPE int32_t
  #else
    #define COORD_TYPE int
  #endif
#endif

typedef COORD_TYPE Coord;

/* Angle type (used by some actions) */
typedef double Angle;

#endif /* PCB_COORD_TYPES_H */
```

**Change Action.h:**
```cpp
// OLD:
extern "C" {
#include "global.h"
}

// NEW:
extern "C" {
#include "coord_types.h"
}
```

**Result:**
- Action.h compiles without global.h ✅
- No dependency cascade ✅
- Still gets Coord type ✅
- Only ~50 lines instead of 1000+ ✅

---

## Impact Analysis

### Files That Need Changes

**1. Create new file:**
- `src/coord_types.h` (new, ~50 lines)

**2. Modify:**
- `src/actions/Action.h` (change 1 line: include coord_types.h instead of global.h)

**3. Update build system:**
- `src/Makefile.am` (add coord_types.h to noinst_HEADERS)

**Total changes: 1 new file, 2 modified files**

### Risks

**Low Risk:**
- New file, doesn't affect existing code
- Action.h only has 2 classes currently (Action and ActionRegistry)
- If it breaks, easy to revert (1 line change)

**Testing:**
```bash
# Before change: Action.h includes global.h
g++ -c Action.cpp  # Compiles, pulls in 20+ headers

# After change: Action.h includes coord_types.h
g++ -c Action.cpp  # Compiles, pulls in only config.h + stdint.h

# Verify no regression
make check
```

---

## Comparison: Full Refactor vs Minimal Extraction

### Full Global.h Refactor (Option 3)

**Scope:**
- Split global.h into 10+ headers
- Update 200+ files that include global.h
- Handle circular dependencies
- Test entire codebase

**Estimate:** 4-6 weeks, high risk

### Minimal Coord Extraction (Option 4)

**Scope:**
- Create coord_types.h (1 file)
- Update Action.h (1 line)
- Update Makefile.am (1 line)
- Test action system

**Estimate:** 2-4 hours, low risk

---

## Recommendation

**OPTION 4: Extract Coord to coord_types.h**

### Why This Is The Right Approach

1. **Solves our immediate problem**
   - Actions compile without global.h dependency cascade
   - MessageAction.cpp can be tested standalone

2. **Low risk, high reward**
   - Minimal changes (3 files)
   - Easy to test and verify
   - Easy to revert if needed

3. **Incremental improvement**
   - Makes the codebase slightly better
   - Doesn't require massive refactoring
   - Follows "start small" principle

4. **Precedent for future**
   - If this works, we can extract other minimal headers as needed
   - Path to gradually improving global.h without big-bang rewrite

5. **Unblocks action migration**
   - Can proceed with MessageAction immediately after
   - Proven approach for future actions

---

## Implementation Plan

### Phase 0: Create coord_types.h (30 minutes)

1. Create src/coord_types.h with minimal content
2. Add to src/Makefile.am
3. Test compilation

### Phase 1: Update Action.h (15 minutes)

1. Change include from global.h to coord_types.h
2. Compile Action.cpp
3. Fix any issues

### Phase 2: Verify (30 minutes)

1. Build action infrastructure
2. Run existing tests
3. Verify no regression

### Phase 3: Resume MessageAction (back on track!)

1. MessageAction.cpp now compiles cleanly
2. Tests can be written without dependency cascade
3. Proceed with Layer 2 testing

**Total time investment: ~2 hours**
**Payoff: Unblocks all future action migrations**

---

## Alternative: What Individual Actions Need

If we do Option 1 (use build system) instead, here's what each action needs:

| Action | Needs from global.h | Can use coord_types.h? |
|--------|-------------------|----------------------|
| Message | none (only error.h) | ✅ Yes |
| Select | PCBType, SearchScreen() | ❌ No, needs full global.h |
| ChangeSize | PCBType, ChangeXYZ() | ❌ No, needs full global.h |
| SaveTo | PCBType, file functions | ❌ No, needs full global.h |

**Insight:** Most actions will need more than just Coord, BUT:
- The Action base class only needs Coord
- Individual actions can include global.h if they need it
- We still win by reducing Action.h dependencies

---

## Decision Matrix

|  | Option 1: Use Build System | Option 4: Extract Coord |
|--|---------------------------|------------------------|
| **Time to implement** | 0 (already done) | 2 hours |
| **Solves immediate problem** | ✅ Yes | ✅ Yes |
| **Solves future problems** | ⚠️ Partial | ✅ Yes |
| **Risk** | Low | Very low |
| **Code quality improvement** | None | Significant |
| **Technical debt** | Same | Reduced |

**Recommendation: Do Option 4 FIRST, then continue with build system integration**

This gives us:
1. Better architecture (extracted coord_types.h)
2. Easier testing going forward
3. Less dependency coupling
4. Only 2 hours of work

Then MessageAction proceeds smoothly with cleaner dependencies.

---

## Conclusion

**Yes, it makes sense to tackle a MINIMAL global.h extraction first.**

Not a full refactor - just extract Coord into coord_types.h. This:
- Solves the dependency cascade problem
- Takes ~2 hours instead of weeks
- Low risk, high reward
- Improves the codebase
- Unblocks action migration

**Recommended next steps:**
1. Create coord_types.h (this commit)
2. Update Action.h to use it (next commit)
3. Verify builds (testing)
4. Resume MessageAction testing (back on track)

