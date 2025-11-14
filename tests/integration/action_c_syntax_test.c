/*
 * Minimal test to verify action.c modifications compile correctly
 * Tests that the action_bridge.h include and pcb_action_execute calls work
 */

#include <stdint.h>
#define COORD_TYPE int32_t
typedef COORD_TYPE Coord;

/* Include the action bridge */
#include "actions/action_bridge.h"

/* Mock a minimal Message function */
void Message(const char* fmt, ...) {
    (void)fmt;
}

/* Mock minimal AFAIL macro */
#define AFAIL(x) return 1

/* Mock hid_save_settings */
void hid_save_settings(int locally) {
    (void)locally;
}

/* Test ActionMessage with fallback pattern */
static int ActionMessage_test(int argc, char **argv, Coord x, Coord y) {
    int i;

    /* Try C++ version first */
    int cpp_result = pcb_action_execute("Message", argc, argv, x, y);
    if (cpp_result != -1)
        return cpp_result;  /* C++ handled it */

    /* Fall back to C version */
    if (argc < 1)
        AFAIL(message);

    for (i = 0; i < argc; i++) {
        Message(argv[i]);
        Message("\n");
    }

    return 0;
}

/* Test ActionSaveSettings with fallback pattern */
static int ActionSaveSettings_test(int argc, char **argv, Coord x, Coord y) {
    /* Try C++ version first */
    int cpp_result = pcb_action_execute("SaveSettings", argc, argv, x, y);
    if (cpp_result != -1)
        return cpp_result;  /* C++ handled it */

    /* Fall back to C version */
    int locally = argc > 0 ? (strncasecmp(argv[0], "local", 5) == 0) : 0;
    hid_save_settings(locally);
    return 0;
}

int main() {
    char* test_argv[] = {"test"};

    /* Test that the functions compile and link */
    ActionMessage_test(1, test_argv, 0, 0);
    ActionSaveSettings_test(1, test_argv, 0, 0);

    return 0;
}
