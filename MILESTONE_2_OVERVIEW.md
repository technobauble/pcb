# Milestone 2: Drawing & Rendering - Overview

**GTK3 Migration - Week 2**
**Duration:** 5 days (40 hours)
**Goal:** Implement Cairo-based 2D rendering for GTK3 HID

---

## Breaking Down Milestone 2

Milestone 2 (Drawing & Rendering) is complex and involves migrating ~3,500 lines of GDK drawing code to Cairo. To make this more manageable, it's broken into two sub-milestones:

### Milestone 2A: Cairo Foundation & Basic Drawing
**Duration:** 3 days (24 hours)
**Focus:** Set up parallel GTK3 HID, create Cairo infrastructure, basic primitives

**Deliverables:**
- Parallel GTK3 HID structure (gtk3/ directory)
- Cairo drawing infrastructure
- Basic drawing primitives working (lines, rectangles, arcs)
- Simple test shapes rendering

**Success Criteria:**
- Can launch PCB with `--hid gtk3`
- Can draw basic shapes with Cairo
- Window displays simple geometric test patterns
- No crashes in drawing code

**Files Created/Modified:**
- `src/hid/gtk3/` (new directory)
- Copy and adapt files from gtk2
- Basic Cairo drawing functions
- ~1,500 lines of code

---

### Milestone 2B: Complete PCB Rendering
**Duration:** 2 days (16 hours)
**Focus:** Complex shapes, PCB-specific drawing, full rendering

**Deliverables:**
- Complex shape rendering (polygons, filled shapes)
- PCB-specific drawing (traces, pads, vias, arcs)
- Event handling integrated
- Full PCB board rendering

**Success Criteria:**
- Can load and display real PCB files
- All PCB elements render correctly
- Visual parity with GTK2 HID
- Mouse/keyboard events work
- Performance acceptable

**Files Completed:**
- gtkhid-gdk.c (~2,000 lines migrated)
- gui-output-events.c (~1,500 lines migrated)
- All drawing functions working

---

## Why Split Milestone 2?

### Complexity Management
- **Large Code Volume:** ~3,500 lines to migrate
- **High Risk:** Drawing is core functionality
- **Testing Burden:** Need incremental verification

### Parallel Development Benefits
- **Safe Testing:** GTK2 HID remains functional
- **Easy Comparison:** Can test GTK3 vs GTK2 side-by-side
- **Rollback Option:** Can revert without breaking main HID
- **Gradual Migration:** Can merge when stable

### Clear Progress Markers
- **2A Complete:** Basic drawing works, infrastructure ready
- **2B Complete:** Full PCB rendering functional
- **Better Planning:** Smaller tasks, clearer estimates

---

## Parallel HID Development Approach

### Directory Structure

```
src/hid/
├── gtk/              # Existing GTK2 HID (untouched)
│   ├── gtkhid-main.c
│   ├── gtkhid-gdk.c
│   ├── gui-top-window.c
│   └── ... (24 files)
│
└── gtk3/             # New GTK3 HID (parallel development)
    ├── gtkhid-main.c      # Copied and adapted
    ├── gtkhid-gdk.c       # Cairo-based drawing
    ├── gui-top-window.c   # GTK3 layouts
    └── ... (24 files adapted)
```

### HID Registration

Both HIDs will be available:

```bash
# List available HIDs
./pcb --hid-list
# Output:
#   gtk     - GTK2-based GUI (existing, stable)
#   gtk3    - GTK3-based GUI (new, experimental)
#   ps      - PostScript export
#   gerber  - Gerber export
#   ...

# Use GTK2 (default)
./pcb myboard.pcb

# Use GTK3 (new)
./pcb --hid gtk3 myboard.pcb
```

### Benefits of Parallel Approach

1. **Safety:** GTK2 HID always works as fallback
2. **Testing:** Easy A/B comparison
3. **Gradual Rollout:** Users can opt-in to GTK3
4. **Risk Mitigation:** Can abandon GTK3 if problems
5. **Development Speed:** No pressure to maintain GTK2 during GTK3 work

### Migration Timeline

```
Week 1 (M1):  Build system ready for GTK3
Week 2 (M2A): Create gtk3/ HID, basic drawing works
Week 2 (M2B): Complete PCB rendering
Week 3 (M3):  OpenGL support in gtk3/ HID
Week 4 (M4):  All dialogs/widgets in gtk3/ HID
Week 5+:      Testing, refinement
Final:        Deprecate GTK2 HID, make GTK3 default
```

---

## Implementation Strategy

### Phase 1: Setup Parallel Structure (Milestone 2A, Day 1)

**Tasks:**
- Create `src/hid/gtk3/` directory
- Copy GTK2 HID files to gtk3/
- Register GTK3 HID in build system
- Update HID struct for gtk3
- Verify basic compilation

**Deliverable:** Can compile both HIDs, GTK3 HID appears in `--hid-list`

---

### Phase 2: Basic Cairo Drawing (Milestone 2A, Days 2-3)

**Tasks:**
- Create Cairo drawing infrastructure
- Implement basic primitives (lines, rectangles, arcs)
- Create test rendering function
- Display simple shapes

**Deliverable:** GTK3 HID can draw basic geometric shapes

---

### Phase 3: Complete Drawing (Milestone 2B, Days 4-5)

**Tasks:**
- Implement complex shapes (polygons, filled paths)
- Implement PCB-specific drawing (traces, pads)
- Integrate event handling
- Test with real PCB files

**Deliverable:** GTK3 HID renders complete PCB boards

---

## Testing Strategy

### Incremental Testing

**After Milestone 2A:**
```bash
# Test basic drawing
./pcb --hid gtk3 tests/empty.pcb
# Should show window with test pattern

# Compare with GTK2
./pcb --hid gtk tests/empty.pcb
# Should look similar (different rendering)
```

**After Milestone 2B:**
```bash
# Test real PCB rendering
./pcb --hid gtk3 tests/inputs/complex_board.pcb

# Visual comparison
./pcb --hid gtk tests/inputs/complex_board.pcb

# Export test (verify rendering works)
./pcb --hid gtk3 --export png tests/inputs/complex_board.pcb
```

### Side-by-Side Testing

Create comparison script:
```bash
#!/bin/bash
# Compare GTK2 vs GTK3 rendering

BOARD=$1

# Launch both side-by-side
./pcb --hid gtk "$BOARD" &
sleep 1
./pcb --hid gtk3 "$BOARD" &

echo "Compare the two windows visually"
echo "GTK2 (left) vs GTK3 (right)"
```

---

## Success Metrics

### Milestone 2A Success
- ✅ GTK3 HID compiles and registers
- ✅ Can launch with `--hid gtk3`
- ✅ Basic shapes draw correctly
- ✅ No crashes in drawing code
- ✅ Window responds to events

### Milestone 2B Success
- ✅ Real PCB files render completely
- ✅ All PCB elements visible (traces, pads, vias, text)
- ✅ Visual quality matches GTK2
- ✅ Performance is acceptable (< 2x slower than GTK2)
- ✅ Mouse/keyboard events work
- ✅ No memory leaks in drawing code

---

## Risk Assessment

### Lower Risk with Parallel Approach

| Risk | GTK2 Replacement | Parallel Development |
|------|------------------|----------------------|
| Break existing users | HIGH | LOW (GTK2 still works) |
| Testing difficulty | HIGH | LOW (easy comparison) |
| Rollback complexity | HIGH | LOW (just don't use gtk3) |
| Development pressure | HIGH | LOW (no GTK2 maintenance) |
| User adoption resistance | HIGH | LOW (opt-in) |

### Risk Mitigation

1. **Compilation Errors:** Isolate to gtk3/ directory
2. **Runtime Crashes:** GTK2 HID unaffected
3. **Visual Regressions:** Easy to compare side-by-side
4. **Performance Issues:** Can optimize without pressure
5. **User Complaints:** GTK2 remains default

---

## Detailed Plans

### Milestone 2A: Cairo Foundation & Basic Drawing
**Document:** `MILESTONE_2A_CAIRO_FOUNDATION.md`
**Duration:** 3 days (24 hours)
**Detail Level:** Day-by-day task list

### Milestone 2B: Complete PCB Rendering
**Document:** `MILESTONE_2B_COMPLETE_RENDERING.md`
**Duration:** 2 days (16 hours)
**Detail Level:** Day-by-day task list

---

## Next Steps

1. **Review this overview** - Ensure approach makes sense
2. **Read Milestone 2A plan** - Detailed 3-day guide
3. **Read Milestone 2B plan** - Detailed 2-day guide
4. **Start implementation** - Begin with Milestone 2A Day 1

---

**Document Version:** 1.0
**Last Updated:** November 17, 2025
**Status:** Ready for Implementation
