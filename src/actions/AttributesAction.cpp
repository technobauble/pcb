#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "data.h"
#include "error.h"
#include "hid.h"
#include "search.h"
}

#include <cstdlib>
#include <cstring>

namespace pcb {
namespace actions {

class AttributesAction : public Action {
public:
    AttributesAction() : Action("Attributes",
        "Attributes(Layout|Layer|Element)\n"
        "Attributes(Layer,layername)",
        "Let the user edit the attributes of the layout, current or given layer, or selected element.") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        char* function = ARG(0);
        char* layername = ARG(1);
        char* buf;

        if (!function) {
            Message(_("Syntax error.  Usage:\n%s\n"),
                    _("Attributes(Layout|Layer|Element)\nAttributes(Layer,layername)"));
            return 1;
        }

        if (!gui->edit_attributes) {
            Message(_("This GUI doesn't support Attribute Editing\n"));
            return 1;
        }

        switch (GetFunctionID(const_cast<char*>(function))) {
            case F_Layout: {
                gui->edit_attributes(const_cast<char*>(_("Layout Attributes")), &(PCB->Attributes));
                return 0;
            }

            case F_Layer: {
                LayerType *layer = CURRENT;
                if (layername) {
                    int i;
                    layer = NULL;
                    for (i = 0; i < max_copper_layer; i++) {
                        if (strcmp(PCB->Data->Layer[i].Name, layername) == 0) {
                            layer = &(PCB->Data->Layer[i]);
                            break;
                        }
                    }
                    if (layer == NULL) {
                        Message(_("No layer named %s\n"), layername);
                        return 1;
                    }
                }
                buf = static_cast<char*>(malloc(strlen(layer->Name) +
                    strlen(_("Layer %s Attributes"))));
                sprintf(buf, _("Layer %s Attributes"), layer->Name);
                gui->edit_attributes(buf, &(layer->Attributes));
                free(buf);
                return 0;
            }

            case F_Element: {
                int n_found = 0;
                ElementType *e = NULL;
                ELEMENT_LOOP(PCB->Data);
                {
                    if (TEST_FLAG(SELECTEDFLAG, element)) {
                        e = element;
                        n_found++;
                    }
                }
                END_LOOP;
                if (n_found > 1) {
                    Message(_("Too many elements selected\n"));
                    return 1;
                }
                if (n_found == 0) {
                    void *ptrtmp;
                    gui->get_coords(const_cast<char*>(_("Click on an element")), &x, &y);
                    if ((SearchScreen(x, y, ELEMENT_TYPE, &ptrtmp,
                                     &ptrtmp, &ptrtmp)) != NO_TYPE)
                        e = static_cast<ElementType*>(ptrtmp);
                    else {
                        Message(_("No element found there\n"));
                        return 1;
                    }
                }

                if (NAMEONPCB_NAME(e)) {
                    buf = static_cast<char*>(malloc(strlen(NAMEONPCB_NAME(e)) +
                        strlen(_("Element %s Attributes"))));
                    sprintf(buf, _("Element %s Attributes"), NAMEONPCB_NAME(e));
                } else {
                    buf = strdup(_("Unnamed Element Attributes"));
                }
                gui->edit_attributes(buf, &(e->Attributes));
                free(buf);
                break;
            }

            default:
                Message(_("Syntax error.  Usage:\n%s\n"),
                        _("Attributes(Layout|Layer|Element)\nAttributes(Layer,layername)"));
                return 1;
        }

        return 0;
    }
};

REGISTER_ACTION(AttributesAction);

}} // namespace pcb::actions
