/*!
 * \file tests/cpp/pcb_stubs.cpp
 *
 * \brief Stub implementations of PCB functions for C++ unit tests.
 *
 * These are minimal stub implementations that allow the action system
 * to compile and link without requiring the entire PCB codebase.
 * Tests that actually need to execute actions should use integration
 * tests instead.
 */

#include <cstring>

extern "C" {

// Include action.h to get FunctionID enum and GetFunctionID declaration
#include "action.h"

// Macros needed by action implementations
#ifndef UNKNOWN
#define UNKNOWN(a) ((a) && *(a) ? (a) : "(unknown)")
#endif

// GetFunctionID stub - minimal implementation for tests
// This is a simplified version that handles only the function IDs needed by tests
int GetFunctionID(const char* str) {
    if (!str) return -1;
    if (strcmp(str, "Save") == 0) return F_Save;
    if (strcmp(str, "Restore") == 0) return F_Restore;
    if (strcmp(str, "Close") == 0) return F_Close;
    if (strcmp(str, "Block") == 0) return F_Block;
    if (strcmp(str, "Center") == 0) return F_Center;
    if (strcmp(str, "Object") == 0) return F_Object;
    if (strcmp(str, "Selected") == 0) return F_Selected;
    if (strcmp(str, "SelectedElements") == 0) return F_SelectedElements;
    if (strcmp(str, "All") == 0) return F_All;
    if (strcmp(str, "Found") == 0) return F_Found;
    if (strcmp(str, "Connection") == 0) return F_Connection;
    if (strcmp(str, "BuriedVias") == 0) return F_BuriedVias;
    return -1;
}

// Stub implementations of PCB functions needed by SelectionActions

// From select.c
int SelectObject(void) { return 0; }
int UnselectObject(void) { return 0; }
int SelectBlock(void* box, int flag) { (void)box; (void)flag; return 0; }
int SelectByFlag(int flag, int flag2) { (void)flag; (void)flag2; return 0; }
void SelectObjectByName(int type, const char* pattern, int flag) {
    (void)type; (void)pattern; (void)flag;
}
void UnselectObjectByName(int type, const char* pattern, int flag) {
    (void)type; (void)pattern; (void)flag;
}
int SelectBuriedVias(int flag) { (void)flag; return 0; }

// From remove.c
int RemoveSelected(void) { return 0; }

// From set.c
void SetChangedFlag(int flag) { (void)flag; }

// From crosshair.c
void notify_crosshair_change(int flag) { (void)flag; }
void notify_mark_change(int flag) { (void)flag; }

// From search.c
void LookupConnection(int x, int y, int flag1, int flag2, int flag3, int flag4) {
    (void)x; (void)y; (void)flag1; (void)flag2; (void)flag3; (void)flag4;
}

// From find.c
int LookupPVConnectionsToPVList(int flag) { (void)flag; return 0; }
int LookupLOConnectionsToPVList(int flag) { (void)flag; return 0; }
int LookupLOConnectionsToLOList(int flag) { (void)flag; return 0; }

// From misc.c
int ResetFoundLinesAndPolygons(int flag) { (void)flag; return 0; }
int ResetFoundPinsViasAndPads(int flag) { (void)flag; return 0; }

// From undo.c
void IncrementUndoSerialNumber(void) {}
void SaveUndoSerialNumber(void) {}
void RestoreUndoSerialNumber(void) {}
int Bumped = 0;

// From error.c
void Message(const char* fmt, ...) { (void)fmt; }

// From draw.c
void Draw(void) {}

// From misc.c (system actions)
void QuitApplication(void) {}

// Global state structures needed by action system
struct CrosshairType {
    int X, Y;
    struct {
        int State;
        struct { int X, Y; } Point1;
        struct { int X, Y; } Point2;
    } AttachedBox;
} Crosshair = {0, 0, {0, {0, 0}, {0, 0}}};

struct MarkType {
    bool status;
    int X, Y;
} Marked = {false, 0, 0};

struct PCBType {
    int Data;
} *PCB = nullptr;

// HID structure (minimal stub)
#define HID_CLOSE_CONFIRM_OK 0

struct hid_st {
    int (*close_confirm_dialog)(void);
};

static int stub_close_confirm_dialog(void) {
    return HID_CLOSE_CONFIRM_OK;
}

static struct hid_st stub_gui = {
    stub_close_confirm_dialog
};

typedef struct hid_st HID;
HID* gui = &stub_gui;

// Library structure for DumpLibrary action
// The types are already declared in global.h (included via action.h)
LibraryType Library = {0, 0, nullptr};

} // extern "C"
