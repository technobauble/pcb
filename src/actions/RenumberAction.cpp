#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "change.h"
#include "data.h"
#include "error.h"
#include "hid.h"
#include "misc.h"
#include "pcb-printf.h"
#include "set.h"
#include "undo.h"
}

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace pcb {
namespace actions {

class RenumberAction : public Action {
private:
    static char* default_file;

    struct _cnt_list {
        char *name;
        unsigned int cnt;
    };

public:
    RenumberAction() : Action("Renumber",
        "Renumber all elements. The changes will be recorded to filename for use in backannotating these changes to the schematic.",
        "Renumber()\n"
        "Renumber(filename)") {}

    ~RenumberAction() {
        // Cleanup static default_file if needed
    }

    int execute(int argc, char** argv, Coord x, Coord y) override {
        bool changed = false;
        ElementType **element_list;
        ElementType **locked_element_list;
        unsigned int i, j, k, cnt, lock_cnt;
        unsigned int tmpi;
        size_t sz;
        char *tmps;
        char *name;
        FILE *out;
        size_t cnt_list_sz = 100;
        struct _cnt_list *cnt_list;
        char **was, **is, *pin;
        unsigned int c_cnt = 0;
        int unique, ok;
        int free_name = 0;

        if (argc < 1) {
            // We deal with the case where name already exists in this
            // function so the GUI doesn't need to deal with it
            name = gui->fileselect(_("Save Renumber Annotation File As ..."),
                                   _("Choose a file to record the renumbering to.\n"
                                     "This file may be used to back annotate the\n"
                                     "change to the schematics.\n"),
                                   default_file, ".eco", "eco", 0);
            free_name = 1;
        } else {
            name = argv[0];
        }

        if (default_file) {
            free(default_file);
            default_file = nullptr;
        }

        if (name && *name) {
            default_file = strdup(name);
        }

        if ((out = fopen(name, "r"))) {
            fclose(out);
            if (!gui->confirm_dialog(_("File exists!  Ok to overwrite?"), 0)) {
                if (free_name && name) {
                    free(name);
                }
                return 0;
            }
        }

        if ((out = fopen(name, "w")) == nullptr) {
            Message(_("Could not open %s\n"), name);
            if (free_name && name) {
                free(name);
            }
            return 1;
        }

        if (free_name && name) {
            free(name);
        }

        fprintf(out, "*COMMENT* PCB Annotation File\n");
        fprintf(out, "*FILEVERSION* 20061031\n");

        // Make a first pass through all of the elements and sort them out
        // by location on the board. While here we also collect a list of
        // locked elements.
        //
        // We'll actually renumber things in the 2nd pass.
        element_list = (ElementType **)calloc(PCB->Data->ElementN, sizeof(ElementType *));
        locked_element_list = (ElementType **)calloc(PCB->Data->ElementN, sizeof(ElementType *));
        was = (char **)calloc(PCB->Data->ElementN, sizeof(char *));
        is = (char **)calloc(PCB->Data->ElementN, sizeof(char *));

        if (element_list == nullptr || locked_element_list == nullptr ||
            was == nullptr || is == nullptr) {
            fprintf(stderr, "calloc() failed in %s\n", __FUNCTION__);
            exit(1);
        }

        cnt = 0;
        lock_cnt = 0;
        ELEMENT_LOOP(PCB->Data);
        {
            if (TEST_FLAG(LOCKFLAG, element->Name) || TEST_FLAG(LOCKFLAG, element)) {
                // Add to the list of locked elements which we won't try to
                // renumber and whose reference designators are now reserved.
                pcb_fprintf(out,
                           "*WARN* Element \"%s\" at %$md is locked and will not be renumbered.\n",
                           UNKNOWN(NAMEONPCB_NAME(element)), element->MarkX, element->MarkY);
                locked_element_list[lock_cnt] = element;
                lock_cnt++;
            } else {
                // Count of devices which will be renumbered
                cnt++;

                // Search for correct position in the list
                i = 0;
                while (element_list[i] && element->MarkY > element_list[i]->MarkY) {
                    i++;
                }

                // We have found the position where we have the first element that
                // has the same Y value or a lower Y value. Now move forward if
                // needed through the X values
                while (element_list[i] &&
                       element->MarkY == element_list[i]->MarkY &&
                       element->MarkX > element_list[i]->MarkX) {
                    i++;
                }

                for (j = cnt - 1; j > i; j--) {
                    element_list[j] = element_list[j - 1];
                }
                element_list[i] = element;
            }
        }
        END_LOOP;

        // Now that the elements are sorted by board position, we go through
        // and renumber them.

        // Turn off the flag which requires unique names so it doesn't get
        // in our way. When we're done with the renumber we will have unique
        // names.
        unique = TEST_FLAG(UNIQUENAMEFLAG, PCB);
        CLEAR_FLAG(UNIQUENAMEFLAG, PCB);

        cnt_list = (struct _cnt_list *)calloc(cnt_list_sz, sizeof(struct _cnt_list));

        for (i = 0; i < cnt; i++) {
            // If there is no refdes, maybe just spit out a warning
            if (NAMEONPCB_NAME(element_list[i])) {
                // Figure out the prefix
                tmps = strdup(NAMEONPCB_NAME(element_list[i]));
                j = 0;
                while (tmps[j] && (tmps[j] < '0' || tmps[j] > '9') && tmps[j] != '?') {
                    j++;
                }
                tmps[j] = '\0';

                // Check the counter for this prefix
                for (j = 0;
                     cnt_list[j].name && (strcmp(cnt_list[j].name, tmps) != 0) &&
                     j < cnt_list_sz; j++);

                // Grow the list if needed
                if (j == cnt_list_sz) {
                    cnt_list_sz += 100;
                    cnt_list = (struct _cnt_list *)realloc(cnt_list, cnt_list_sz * sizeof(struct _cnt_list));
                    if (cnt_list == nullptr) {
                        fprintf(stderr, _("realloc() failed in %s()\n"), __FUNCTION__);
                        exit(1);
                    }
                    // Zero out the memory that we added
                    for (tmpi = j; tmpi < cnt_list_sz; tmpi++) {
                        cnt_list[tmpi].name = nullptr;
                        cnt_list[tmpi].cnt = 0;
                    }
                }

                // Start a new counter if we don't have a counter for this prefix
                if (!cnt_list[j].name) {
                    cnt_list[j].name = strdup(tmps);
                    cnt_list[j].cnt = 0;
                }

                // Check to see if the new refdes is already used by a locked element
                do {
                    ok = 1;
                    cnt_list[j].cnt++;
                    free(tmps);

                    // Space for the prefix plus 1 digit plus the '\0'
                    sz = strlen(cnt_list[j].name) + 2;

                    // And 1 more per extra digit needed to hold the number
                    tmpi = cnt_list[j].cnt;
                    while (tmpi > 10) {
                        sz++;
                        tmpi = tmpi / 10;
                    }
                    tmps = (char *)malloc(sz * sizeof(char));
                    sprintf(tmps, "%s%d", cnt_list[j].name, (int) cnt_list[j].cnt);

                    // Now compare to the list of reserved (by locked elements) names
                    for (k = 0; k < lock_cnt; k++) {
                        if (strcmp(UNKNOWN(NAMEONPCB_NAME(locked_element_list[k])), tmps) == 0) {
                            ok = 0;
                            break;
                        }
                    }
                } while (!ok);

                if (strcmp(tmps, NAMEONPCB_NAME(element_list[i])) != 0) {
                    fprintf(out, "*RENAME* \"%s\" \"%s\"\n",
                           NAMEONPCB_NAME(element_list[i]), tmps);

                    // Add this rename to our table of renames so we can update the netlist
                    was[c_cnt] = strdup(NAMEONPCB_NAME(element_list[i]));
                    is[c_cnt] = strdup(tmps);
                    c_cnt++;

                    AddObjectToChangeNameUndoList(ELEMENT_TYPE, nullptr, nullptr,
                                                 element_list[i],
                                                 NAMEONPCB_NAME(element_list[i]));

                    ChangeObjectName(ELEMENT_TYPE, element_list[i], nullptr, nullptr, tmps);
                    changed = true;

                    // We don't free tmps in this case because it is used
                } else {
                    free(tmps);
                }
            } else {
                pcb_fprintf(out, "*WARN* Element at %$md has no name.\n",
                           element_list[i]->MarkX, element_list[i]->MarkY);
            }
        }

        fclose(out);

        // Restore the unique flag setting
        if (unique) {
            SET_FLAG(UNIQUENAMEFLAG, PCB);
        }

        if (changed) {
            // Update the netlist
            AddNetlistLibToUndoList(&(PCB->NetlistLib));

            // Iterate over each net
            for (i = 0; i < PCB->NetlistLib.MenuN; i++) {
                // Iterate over each pin on the net
                for (j = 0; j < PCB->NetlistLib.Menu[i].EntryN; j++) {
                    // Figure out the pin number part from strings like U3-21
                    tmps = strdup(PCB->NetlistLib.Menu[i].Entry[j].ListEntry);
                    for (k = 0; tmps[k] && tmps[k] != '-'; k++);
                    tmps[k] = '\0';
                    pin = tmps + k + 1;

                    // Iterate over the list of changed reference designators
                    for (k = 0; k < c_cnt; k++) {
                        // If the pin needs to change, change it and quit searching in the list.
                        if (strcmp(tmps, was[k]) == 0) {
                            free(PCB->NetlistLib.Menu[i].Entry[j].ListEntry);
                            PCB->NetlistLib.Menu[i].Entry[j].ListEntry =
                                (char *)malloc((strlen(is[k]) + strlen(pin) + 2) * sizeof(char));
                            sprintf(PCB->NetlistLib.Menu[i].Entry[j].ListEntry,
                                   "%s-%s", is[k], pin);
                            k = c_cnt;
                        }
                    }
                    free(tmps);
                }
            }

            for (k = 0; k < c_cnt; k++) {
                free(was[k]);
                free(is[k]);
            }

            NetlistChanged(0);
            IncrementUndoSerialNumber();
            SetChangedFlag(true);
        }

        free(locked_element_list);
        free(element_list);
        free(cnt_list);
        free(is);
        free(was);
        return 0;
    }
};

// Initialize static member
char* RenumberAction::default_file = nullptr;

REGISTER_ACTION(RenumberAction);

}} // namespace pcb::actions
