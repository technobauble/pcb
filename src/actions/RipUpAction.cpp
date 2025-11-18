#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "buffer.h"
#include "copy.h"
#include "data.h"
#include "draw.h"
#include "remove.h"
#include "search.h"
#include "set.h"
#include "undo.h"
}

namespace pcb {
namespace actions {

class RipUpAction : public Action {
public:
    RipUpAction() : Action("RipUp",
        "RipUp(All|Selected|Element)",
        "Ripup auto-routed tracks, or convert an element to parts.") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        char* function = ARG(0);
        bool changed = false;

        if (function) {
            switch (GetFunctionID(const_cast<char*>(function))) {
                case F_All:
                    ALLLINE_LOOP(PCB->Data);
                    {
                        if (TEST_FLAG(AUTOFLAG, line) && !TEST_FLAG(LOCKFLAG, line)) {
                            RemoveObject(LINE_TYPE, layer, line, line);
                            changed = true;
                        }
                    }
                    ENDALL_LOOP;
                    ALLARC_LOOP(PCB->Data);
                    {
                        if (TEST_FLAG(AUTOFLAG, arc) && !TEST_FLAG(LOCKFLAG, arc)) {
                            RemoveObject(ARC_TYPE, layer, arc, arc);
                            changed = true;
                        }
                    }
                    ENDALL_LOOP;
                    VIA_LOOP(PCB->Data);
                    {
                        if (TEST_FLAG(AUTOFLAG, via) && !TEST_FLAG(LOCKFLAG, via)) {
                            RemoveObject(VIA_TYPE, via, via, via);
                            changed = true;
                        }
                    }
                    END_LOOP;

                    if (changed) {
                        IncrementUndoSerialNumber();
                        SetChangedFlag(true);
                    }
                    break;

                case F_Selected:
                    VISIBLELINE_LOOP(PCB->Data);
                    {
                        if (TEST_FLAGS(AUTOFLAG | SELECTEDFLAG, line)
                            && !TEST_FLAG(LOCKFLAG, line)) {
                            RemoveObject(LINE_TYPE, layer, line, line);
                            changed = true;
                        }
                    }
                    ENDALL_LOOP;
                    if (PCB->ViaOn)
                        VIA_LOOP(PCB->Data);
                    {
                        if (TEST_FLAGS(AUTOFLAG | SELECTEDFLAG, via)
                            && !TEST_FLAG(LOCKFLAG, via)) {
                            RemoveObject(VIA_TYPE, via, via, via);
                            changed = true;
                        }
                    }
                    END_LOOP;
                    if (changed) {
                        IncrementUndoSerialNumber();
                        SetChangedFlag(true);
                    }
                    break;

                case F_Element: {
                    void *ptr1, *ptr2, *ptr3;

                    if (SearchScreen(Crosshair.X, Crosshair.Y, ELEMENT_TYPE,
                                    &ptr1, &ptr2, &ptr3) != NO_TYPE) {
                        Note.Buffer = Settings.BufferNumber;
                        SetBufferNumber(MAX_BUFFER - 1);
                        ClearBuffer(PASTEBUFFER);
                        CopyObjectToBuffer(PASTEBUFFER->Data, PCB->Data,
                                         ELEMENT_TYPE, ptr1, ptr2, ptr3);
                        SmashBufferElement(PASTEBUFFER);
                        PASTEBUFFER->X = 0;
                        PASTEBUFFER->Y = 0;
                        SaveUndoSerialNumber();
                        EraseObject(ELEMENT_TYPE, ptr1, ptr1);
                        MoveObjectToRemoveUndoList(ELEMENT_TYPE, ptr1, ptr2, ptr3);
                        RestoreUndoSerialNumber();
                        CopyPastebufferToLayout(0, 0);
                        SetBufferNumber(Note.Buffer);
                        SetChangedFlag(true);
                    }
                }
                break;
            }
        }
        return 0;
    }
};

REGISTER_ACTION(RipUpAction);

}} // namespace pcb::actions
