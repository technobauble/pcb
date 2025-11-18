#include "Action.h"

extern "C" {
#include "global.h"
#include "hid.h"
}

namespace pcb {
namespace actions {

class PSCalibAction : public Action {
public:
    PSCalibAction() : Action("pscalib",
        "pscalib()",
        "Calibrates the PostScript output.") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        HID *ps = hid_find_exporter("ps");
        if (ps) {
            ps->calibrate(0.0, 0.0);
        }
        return 0;
    }
};

REGISTER_ACTION(PSCalibAction);

}} // namespace pcb::actions
