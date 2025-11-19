#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "buffer.h"
#include "change.h"
#include "copy.h"
#include "create.h"
#include "data.h"
#include "error.h"
#include "hid.h"
#include "misc.h"
#include "mirror.h"
#include "remove.h"
#include "rotate.h"
#include "set.h"
}

#include <cstdlib>
#include <cstring>

namespace pcb {
namespace actions {

// File-scope static variables
static ElementType* element_cache = nullptr;
static int number_of_footprints_not_found = 0;

// Helper function: find element by reference designator
static ElementType* find_element_by_refdes(char* refdes) {
    if (element_cache &&
        NAMEONPCB_NAME(element_cache) &&
        strcmp(NAMEONPCB_NAME(element_cache), refdes) == 0) {
        return element_cache;
    }

    ELEMENT_LOOP(PCB->Data);
    {
        if (NAMEONPCB_NAME(element) &&
            strcmp(NAMEONPCB_NAME(element), refdes) == 0) {
            element_cache = element;
            return element_cache;
        }
    }
    END_LOOP;
    return nullptr;
}

// Helper function: parse layout attribute units
static int parse_layout_attribute_units(const char* name, int def) {
    char* as = AttributeGet(PCB, const_cast<char*>(name));
    if (!as) {
        return def;
    }
    return GetValue(as, nullptr, nullptr);
}

// Syntax string for AFAIL macro
static const char* elementlist_syntax =
    "ElementList(Start|Done|Need,<refdes>,<footprint>,<value>)";

class ElementListAction : public Action {
public:
    ElementListAction() : Action("ElementList",
        "Adds the given element if it doesn't already exist",
        "ElementList(Start|Done|Need,<refdes>,<footprint>,<value>)") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        ElementType* e = nullptr;
        char* refdes;
        char* value;
        char* footprint;
        char* old;
        char* args[3];
        char* function;

        if (argc < 1) {
            Message(_("ElementList requires a function argument\n"));
            return 1;
        }

        function = argv[0];

#ifdef DEBUG
        printf("Entered ActionElementList, executing function %s\n", function);
#endif

        if (strcasecmp(function, "start") == 0) {
            ELEMENT_LOOP(PCB->Data);
            {
                CLEAR_FLAG(FOUNDFLAG, element);
            }
            END_LOOP;
            element_cache = nullptr;
            number_of_footprints_not_found = 0;
            return 0;
        }

        if (strcasecmp(function, "done") == 0) {
            ELEMENT_LOOP(PCB->Data);
            {
                if (TEST_FLAG(FOUNDFLAG, element)) {
                    CLEAR_FLAG(FOUNDFLAG, element);
                } else if (!EMPTY_STRING_P(NAMEONPCB_NAME(element))) {
                    // Unnamed elements should remain untouched
                    SET_FLAG(SELECTEDFLAG, element);
                }
            }
            END_LOOP;
            if (number_of_footprints_not_found > 0) {
                const char* msg = _("Not all requested footprints were found.\n"
                                    "See the message log for details");
                gui->confirm_dialog(const_cast<char*>(msg), const_cast<char*>("Ok"), nullptr);
            }
            return 0;
        }

        if (strcasecmp(function, "need") != 0) {
            Message(_("ElementList: unknown function %s\n"), function);
            return 1;
        }

        if (argc != 4) {
            Message(_("ElementList(Need) requires refdes, footprint, and value\n"));
            return 1;
        }

        argc--;
        argv++;

        refdes = argv[0];
        footprint = argv[1];
        value = argv[2];

        args[0] = footprint;
        args[1] = refdes;
        args[2] = value;

#ifdef DEBUG
        printf("  ... footprint = %s\n", footprint);
        printf("  ... refdes = %s\n", refdes);
        printf("  ... value = %s\n", value);
#endif

        e = find_element_by_refdes(refdes);

        if (!e) {
            Coord nx, ny, d;

#ifdef DEBUG
            printf("  ... Footprint not on board, need to add it.\n");
#endif
            // Not on board, need to add it
            if (LoadFootprint(argc, args, x, y)) {
                number_of_footprints_not_found++;
                return 1;
            }

            nx = PCB->MaxWidth / 2;
            ny = PCB->MaxHeight / 2;
            d = MIN(PCB->MaxWidth, PCB->MaxHeight) / 10;

            nx = parse_layout_attribute_units("import::newX", nx);
            ny = parse_layout_attribute_units("import::newY", ny);
            d = parse_layout_attribute_units("import::disperse", d);

            if (d > 0) {
                nx += rand() % (d * 2) - d;
                ny += rand() % (d * 2) - d;
            }

            if (nx < 0)
                nx = 0;
            if (nx >= PCB->MaxWidth)
                nx = PCB->MaxWidth - 1;
            if (ny < 0)
                ny = 0;
            if (ny >= PCB->MaxHeight)
                ny = PCB->MaxHeight - 1;

            // Place components onto center of board
            if (CopyPastebufferToLayout(nx, ny)) {
                SetChangedFlag(true);
            }
        } else if (e && DESCRIPTION_NAME(e) &&
                   strcmp(DESCRIPTION_NAME(e), footprint) != 0) {
            int er, pr, i;
            Coord mx, my;
            ElementType* pe;

#ifdef DEBUG
            printf("  ... Footprint on board, but different from footprint loaded.\n");
#endif
            // Different footprint, we need to swap them out
            if (LoadFootprint(argc, args, x, y)) {
                number_of_footprints_not_found++;
                return 1;
            }

            er = ElementOrientation(e);
            pe = static_cast<ElementType*>(PASTEBUFFER->Data->Element->data);
            if (!FRONT(e)) {
                MirrorElementCoordinates(PASTEBUFFER->Data, pe, pe->MarkY * 2 - PCB->MaxHeight);
            }
            pr = ElementOrientation(pe);

            mx = e->MarkX;
            my = e->MarkY;

            if (er != pr) {
                RotateElementLowLevel(PASTEBUFFER->Data, pe, pe->MarkX, pe->MarkY, (er - pr + 4) % 4);
            }

            for (i = 0; i < MAX_ELEMENTNAMES; i++) {
                pe->Name[i].X = e->Name[i].X - mx + pe->MarkX;
                pe->Name[i].Y = e->Name[i].Y - my + pe->MarkY;
                pe->Name[i].Direction = e->Name[i].Direction;
                pe->Name[i].Scale = e->Name[i].Scale;
            }

            RemoveElement(e);

            if (CopyPastebufferToLayout(mx, my)) {
                SetChangedFlag(true);
            }
        }

        // Now reload footprint
        element_cache = nullptr;
        e = find_element_by_refdes(refdes);

        old = ChangeElementText(PCB, PCB->Data, e, NAMEONPCB_INDEX, strdup(refdes));
        if (old) {
            free(old);
        }
        old = ChangeElementText(PCB, PCB->Data, e, VALUE_INDEX, strdup(value));
        if (old) {
            free(old);
        }

        SET_FLAG(FOUNDFLAG, e);

#ifdef DEBUG
        printf(" ... Leaving ActionElementList.\n");
#endif

        return 0;
    }
};

REGISTER_ACTION(ElementListAction);

}} // namespace pcb::actions
