/*!
 * \file tests/cpp/pcb_stubs.cpp
 *
 * \brief Stub implementations for C++ unit tests
 *
 * This file provides minimal stub implementations of PCB globals
 * and functions needed by the mode integration tests.
 */

extern "C" {
#include "global.h"
#include "crosshair.h"
#include "const.h"
#include "data.h"
#include "actions/ActionContext.h"
}

// Global stubs - these are the actual PCB globals that would normally
// be defined in crosshair.c, data.c, etc.
CrosshairType Crosshair;
PCBType *PCB = nullptr;

// Mode manager global - defined here for tests
namespace pcb {
namespace modes {
class ModeManager;
}
}
pcb::modes::ModeManager* pcb_mode_manager = nullptr;

// Action context global
ActionContext *pcb_action_context = nullptr;

// Stub functions that modes may call - these do nothing in tests
extern "C" {

void SetMode(int mode) {
    // Stub - do nothing (actual implementation in set.c)
    (void)mode;
}

void SetStatusLine(void) {
    // Stub - do nothing
}

void EventMoveCrosshair(Coord X, Coord Y) {
    (void)X;
    (void)Y;
}

void RestoreCrosshair(void) {
    // Stub
}

void HideCrosshair(void) {
    // Stub
}

void FitCrosshairIntoGrid(Coord X, Coord Y) {
    (void)X;
    (void)Y;
}

bool MoveCrosshairAbsolute(Coord X, Coord Y) {
    Crosshair.X = X;
    Crosshair.Y = Y;
    return true;
}

void Draw(void) {
    // Stub
}

void ClearAndRedrawOutput(void) {
    // Stub
}

void IncrementUndoSerialNumber(void) {
    // Stub
}

void RestoreUndoSerialNumber(void) {
    // Stub
}

int SaveUndoSerialNumber(void) {
    return 0;
}

void Message(const char *fmt, ...) {
    (void)fmt;
    // Stub - ignore messages in tests
}

void gui_log(const char *fmt, ...) {
    (void)fmt;
}

} // extern "C"
