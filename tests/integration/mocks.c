/*!
 * \file tests/integration/mocks.c
 *
 * \brief Mock implementations of PCB functions for integration testing
 *
 * Provides minimal implementations of the functions that actions call,
 * so we can test the action system without the full PCB application.
 */

#include <stdio.h>
#include <stdarg.h>

/*!
 * \brief Mock Message function
 *
 * In real PCB, this displays messages to the GUI log window.
 * Here, we just print to stdout for testing.
 */
void Message(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("[Message] ");
    vprintf(format, args);
    va_end(args);
}

/*!
 * \brief Mock hid_save_settings function
 *
 * In real PCB, this saves settings to a file.
 * Here, we just print what would be saved.
 */
void hid_save_settings(int locally) {
    if (locally) {
        printf("[hid_save_settings] Saving settings locally (project-specific)\n");
    } else {
        printf("[hid_save_settings] Saving settings globally (user preferences)\n");
    }
}
