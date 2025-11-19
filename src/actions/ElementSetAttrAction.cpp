#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "create.h"
#include "data.h"
#include "error.h"
}

#include <cstring>

namespace pcb {
namespace actions {

// Helper functions for attribute manipulation
static AttributeType* lookup_attr(AttributeListType *list, const char *name) {
    int i;
    for (i = 0; i < list->Number; i++) {
        if (strcmp(list->List[i].name, name) == 0)
            return &list->List[i];
    }
    return NULL;
}

static void delete_attr(AttributeListType *list, AttributeType *attr) {
    int idx = attr - list->List;
    if (idx < 0 || idx >= list->Number)
        return;
    if (list->Number - idx > 1)
        memmove(attr, attr + 1, (list->Number - idx - 1) * sizeof(AttributeType));
    list->Number--;
}

class ElementSetAttrAction : public Action {
public:
    ElementSetAttrAction() : Action("ElementSetAttr",
        "ElementSetAttr(refdes,name[,value])",
        "Sets or clears an element-specific attribute.") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        ElementType *e = NULL;
        char *refdes, *name, *value;
        AttributeType *attr;

        if (argc < 2) {
            Message(_("Syntax error.  Usage:\n%s\n"), _("ElementSetAttr(refdes,name[,value])"));
            return 1;
        }

        refdes = argv[0];
        name = argv[1];
        value = ARG(2);

        ELEMENT_LOOP(PCB->Data);
        {
            if (NSTRCMP(refdes, NAMEONPCB_NAME(element)) == 0) {
                e = element;
                break;
            }
        }
        END_LOOP;

        if (!e) {
            Message(_("Cannot change attribute of %s - element not found\n"), refdes);
            return 1;
        }

        attr = lookup_attr(&e->Attributes, name);

        if (attr && value) {
            free(attr->value);
            attr->value = strdup(value);
        }
        if (attr && !value) {
            delete_attr(&e->Attributes, attr);
        }
        if (!attr && value) {
            CreateNewAttribute(&e->Attributes, name, value);
        }

        return 0;
    }
};

REGISTER_ACTION(ElementSetAttrAction);

}} // namespace pcb::actions
