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

extern "C" {

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

// From error.c
void Message(const char* fmt, ...) { (void)fmt; }

// From draw.c
void Draw(void) {}

// Global state structures needed by action system
struct CrosshairType {
    int X, Y;
    struct {
        int State;
        struct { int X, Y; } Point1;
        struct { int X, Y; } Point2;
    } AttachedBox;
} Crosshair = {0, 0, {0, {0, 0}, {0, 0}}};

struct PCBType {
    int Data;
} *PCB = nullptr;

// HID structure (minimal stub)
struct hid_st;
typedef struct hid_st HID;

HID* gui = nullptr;

} // extern "C"
