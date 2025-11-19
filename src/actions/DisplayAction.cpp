#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "find.h"
#include "hid.h"
#include "misc.h"
#include "move.h"
#include "search.h"
#include "set.h"
#include "undo.h"
}

#include <cstring>

namespace pcb {
namespace actions {

class DisplayAction : public Action {
public:
    DisplayAction() : Action("Display",
        "Several display-related actions",
        "Display(NameOnPCB|Description|Value)\n"
        "Display(Grid|Redraw)\n"
        "Display(CycleClip|CycleCrosshair|Toggle45Degree|ToggleStartDirection)\n"
        "Display(ToggleGrid|ToggleRubberBandMode|ToggleUniqueNames)\n"
        "Display(ToggleMask|ToggleName|ToggleClearLine|ToggleFullPoly|ToggleSnapPin)\n"
        "Display(ToggleThindraw|ToggleThindrawPoly|ToggleOrthoMove|ToggleLocalRef)\n"
        "Display(ToggleCheckPlanes|ToggleShowDRC|ToggleAutoDRC)\n"
        "Display(ToggleLiveRoute|LockNames|OnlyNames)\n"
        "Display(Pinout|PinOrPadName)") {}

    int execute(int argc, char** argv, Coord childX, Coord childY) override {
        char *function, *str_dir;
        int id;
        int err = 0;

        function = argc > 0 ? argv[0] : nullptr;
        str_dir = argc > 1 ? argv[1] : nullptr;

        if (function && (!str_dir || !*str_dir)) {
            switch (id = GetFunctionID(function)) {

                // Redraw layout
                case F_ClearAndRedraw:
                case F_Redraw:
                    Redraw();
                    break;

                // Change the displayed name of elements
                case F_Value:
                case F_NameOnPCB:
                case F_Description:
                    ELEMENT_LOOP(PCB->Data);
                    {
                        EraseElementName(element);
                    }
                    END_LOOP;
                    CLEAR_FLAG(DESCRIPTIONFLAG | NAMEONPCBFLAG, PCB);
                    switch (id) {
                        case F_Value:
                            break;
                        case F_NameOnPCB:
                            SET_FLAG(NAMEONPCBFLAG, PCB);
                            break;
                        case F_Description:
                            SET_FLAG(DESCRIPTIONFLAG, PCB);
                            break;
                    }
                    ELEMENT_LOOP(PCB->Data);
                    {
                        DrawElementName(element);
                    }
                    END_LOOP;
                    Draw();
                    break;

                // Toggle line-adjust flag
                case F_ToggleAllDirections:
                    TOGGLE_FLAG(ALLDIRECTIONFLAG, PCB);
                    AdjustAttachedObjects();
                    break;

                case F_CycleClip:
                    notify_crosshair_change(false);
                    if (TEST_FLAG(ALLDIRECTIONFLAG, PCB)) {
                        TOGGLE_FLAG(ALLDIRECTIONFLAG, PCB);
                        PCB->Clipping = 0;
                    } else {
                        PCB->Clipping = (PCB->Clipping + 1) % 3;
                    }
                    AdjustAttachedObjects();
                    notify_crosshair_change(true);
                    break;

                case F_CycleCrosshair:
                    notify_crosshair_change(false);
                    Crosshair.shape = CrosshairShapeIncrement(Crosshair.shape);
                    if (Crosshair_Shapes_Number == Crosshair.shape) {
                        Crosshair.shape = Basic_Crosshair_Shape;
                    }
                    notify_crosshair_change(true);
                    break;

                case F_ToggleRubberBandMode:
                    notify_crosshair_change(false);
                    TOGGLE_FLAG(RUBBERBANDFLAG, PCB);
                    notify_crosshair_change(true);
                    break;

                case F_ToggleAutoBuriedVias:
                    notify_crosshair_change(false);
                    TOGGLE_FLAG(AUTOBURIEDVIASFLAG, PCB);
                    notify_crosshair_change(true);
                    break;

                case F_ToggleStartDirection:
                    notify_crosshair_change(false);
                    TOGGLE_FLAG(SWAPSTARTDIRFLAG, PCB);
                    notify_crosshair_change(true);
                    break;

                case F_ToggleUniqueNames:
                    TOGGLE_FLAG(UNIQUENAMEFLAG, PCB);
                    break;

                case F_ToggleSnapPin:
                    notify_crosshair_change(false);
                    TOGGLE_FLAG(SNAPPINFLAG, PCB);
                    notify_crosshair_change(true);
                    break;

                case F_ToggleLocalRef:
                    TOGGLE_FLAG(LOCALREFFLAG, PCB);
                    break;

                case F_ToggleThindraw:
                    TOGGLE_FLAG(THINDRAWFLAG, PCB);
                    Redraw();
                    break;

                case F_ToggleThindrawPoly:
                    TOGGLE_FLAG(THINDRAWPOLYFLAG, PCB);
                    Redraw();
                    break;

                case F_ToggleLockNames:
                    TOGGLE_FLAG(LOCKNAMESFLAG, PCB);
                    CLEAR_FLAG(ONLYNAMESFLAG, PCB);
                    break;

                case F_ToggleOnlyNames:
                    TOGGLE_FLAG(ONLYNAMESFLAG, PCB);
                    CLEAR_FLAG(LOCKNAMESFLAG, PCB);
                    break;

                case F_ToggleHideNames:
                    TOGGLE_FLAG(HIDENAMESFLAG, PCB);
                    Redraw();
                    break;

                case F_ToggleShowDRC:
                    TOGGLE_FLAG(SHOWDRCFLAG, PCB);
                    break;

                case F_ToggleLiveRoute:
                    TOGGLE_FLAG(LIVEROUTEFLAG, PCB);
                    break;

                case F_ToggleAutoDRC:
                    notify_crosshair_change(false);
                    TOGGLE_FLAG(AUTODRCFLAG, PCB);
                    if (TEST_FLAG(AUTODRCFLAG, PCB) && Settings.Mode == LINE_MODE) {
                        if (ClearFlagOnAllObjects(CONNECTEDFLAG | FOUNDFLAG, true)) {
                            IncrementUndoSerialNumber();
                            Draw();
                        }
                        if (Crosshair.AttachedLine.State != STATE_FIRST) {
                            LookupConnection(Crosshair.AttachedLine.Point1.X,
                                           Crosshair.AttachedLine.Point1.Y,
                                           true, 1, CONNECTEDFLAG, false);
                            LookupConnection(Crosshair.AttachedLine.Point1.X,
                                           Crosshair.AttachedLine.Point1.Y,
                                           true, 1, FOUNDFLAG, true);
                        }
                    }
                    notify_crosshair_change(true);
                    break;

                case F_ToggleCheckPlanes:
                    TOGGLE_FLAG(CHECKPLANESFLAG, PCB);
                    Redraw();
                    break;

                case F_ToggleOrthoMove:
                    TOGGLE_FLAG(ORTHOMOVEFLAG, PCB);
                    break;

                case F_ToggleName:
                    TOGGLE_FLAG(SHOWNUMBERFLAG, PCB);
                    Redraw();
                    break;

                case F_ToggleMask:
                    TOGGLE_FLAG(SHOWMASKFLAG, PCB);
                    Redraw();
                    break;

                case F_ToggleClearLine:
                    TOGGLE_FLAG(CLEARNEWFLAG, PCB);
                    break;

                case F_ToggleFullPoly:
                    TOGGLE_FLAG(NEWFULLPOLYFLAG, PCB);
                    break;

                // Shift grid alignment
                case F_ToggleGrid:
                {
                    Coord oldGrid = PCB->Grid;
                    PCB->Grid = 1;
                    if (MoveCrosshairAbsolute(Crosshair.X, Crosshair.Y)) {
                        notify_crosshair_change(true);  // First notify was in MoveCrosshairAbs
                    }
                    SetGrid(oldGrid, true);
                }
                break;

                // Toggle displaying of the grid
                case F_Grid:
                    Settings.DrawGrid = !Settings.DrawGrid;
                    Redraw();
                    break;

                // Display the pinout of an element
                case F_Pinout:
                {
                    ElementType *element;
                    void *ptrtmp;
                    Coord x, y;

                    gui->get_coords(_("Click on an element"), &x, &y);
                    if ((SearchScreen(x, y, ELEMENT_TYPE, &ptrtmp,
                                     &ptrtmp, &ptrtmp)) != NO_TYPE) {
                        element = (ElementType *) ptrtmp;
                        gui->show_item(element);
                    }
                    break;
                }

                // Toggle displaying of pin/pad/via names
                case F_PinOrPadName:
                {
                    void *ptr1, *ptr2, *ptr3;
                    Coord x, y;

                    gui->get_coords(_("Click on an element"), &x, &y);

                    switch (SearchScreen(x, y,
                                        ELEMENT_TYPE | PIN_TYPE | PAD_TYPE | VIA_TYPE,
                                        (void **) &ptr1, (void **) &ptr2,
                                        (void **) &ptr3)) {
                        case ELEMENT_TYPE:
                            PIN_LOOP((ElementType *) ptr1);
                            {
                                if (TEST_FLAG(DISPLAYNAMEFLAG, pin)) {
                                    ErasePinName(pin);
                                } else {
                                    DrawPinName(pin);
                                }
                                AddObjectToFlagUndoList(PIN_TYPE, ptr1, pin, pin);
                                TOGGLE_FLAG(DISPLAYNAMEFLAG, pin);
                            }
                            END_LOOP;
                            PAD_LOOP((ElementType *) ptr1);
                            {
                                if (TEST_FLAG(DISPLAYNAMEFLAG, pad)) {
                                    ErasePadName(pad);
                                } else {
                                    DrawPadName(pad);
                                }
                                AddObjectToFlagUndoList(PAD_TYPE, ptr1, pad, pad);
                                TOGGLE_FLAG(DISPLAYNAMEFLAG, pad);
                            }
                            END_LOOP;
                            SetChangedFlag(true);
                            IncrementUndoSerialNumber();
                            Draw();
                            break;

                        case PIN_TYPE:
                            if (TEST_FLAG(DISPLAYNAMEFLAG, (PinType *) ptr2)) {
                                ErasePinName((PinType *) ptr2);
                            } else {
                                DrawPinName((PinType *) ptr2);
                            }
                            AddObjectToFlagUndoList(PIN_TYPE, ptr1, ptr2, ptr3);
                            TOGGLE_FLAG(DISPLAYNAMEFLAG, (PinType *) ptr2);
                            SetChangedFlag(true);
                            IncrementUndoSerialNumber();
                            Draw();
                            break;

                        case PAD_TYPE:
                            if (TEST_FLAG(DISPLAYNAMEFLAG, (PadType *) ptr2)) {
                                ErasePadName((PadType *) ptr2);
                            } else {
                                DrawPadName((PadType *) ptr2);
                            }
                            AddObjectToFlagUndoList(PAD_TYPE, ptr1, ptr2, ptr3);
                            TOGGLE_FLAG(DISPLAYNAMEFLAG, (PadType *) ptr2);
                            SetChangedFlag(true);
                            IncrementUndoSerialNumber();
                            Draw();
                            break;

                        case VIA_TYPE:
                            if (TEST_FLAG(DISPLAYNAMEFLAG, (PinType *) ptr2)) {
                                EraseViaName((PinType *) ptr2);
                            } else {
                                DrawViaName((PinType *) ptr2);
                            }
                            AddObjectToFlagUndoList(VIA_TYPE, ptr1, ptr2, ptr3);
                            TOGGLE_FLAG(DISPLAYNAMEFLAG, (PinType *) ptr2);
                            SetChangedFlag(true);
                            IncrementUndoSerialNumber();
                            Draw();
                            break;
                    }
                    break;
                }

                default:
                    err = 1;
            }
        } else if (function && str_dir) {
            switch (GetFunctionID(function)) {
                case F_ToggleGrid:
                    if (argc > 2) {
                        PCB->GridOffsetX = GetValue(argv[1], nullptr, nullptr);
                        PCB->GridOffsetY = GetValue(argv[2], nullptr, nullptr);
                        if (Settings.DrawGrid) {
                            Redraw();
                        }
                    }
                    break;

                default:
                    err = 1;
                    break;
            }
        } else {
            err = 1;
        }

        if (err) {
            Message(_("Syntax error. Usage:\n%s\n"),
                    _("Display(NameOnPCB|Description|Value)\n"
                      "Display(Grid|Redraw)\n"
                      "Display(CycleClip|CycleCrosshair|Toggle45Degree|ToggleStartDirection)\n"
                      "Display(ToggleGrid|ToggleRubberBandMode|ToggleUniqueNames)\n"
                      "Display(ToggleMask|ToggleName|ToggleClearLine|ToggleFullPoly|ToggleSnapPin)\n"
                      "Display(ToggleThindraw|ToggleThindrawPoly|ToggleOrthoMove|ToggleLocalRef)\n"
                      "Display(ToggleCheckPlanes|ToggleShowDRC|ToggleAutoDRC)\n"
                      "Display(ToggleLiveRoute|LockNames|OnlyNames)\n"
                      "Display(Pinout|PinOrPadName)"));
            return 1;
        }

        return 0;
    }
};

REGISTER_ACTION(DisplayAction);

}} // namespace pcb::actions
