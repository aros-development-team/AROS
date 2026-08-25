/*
 * GUI
 */

#include <aros/isoascii.h>
#include "debug.h"
#include "numtostr.h"

#include "bthid.h"

#ifdef USE_NLIST

#undef ListObject
#define ListObject MUIOBJMACRO_START("NList.mcc")

#undef ListviewObject
#define ListviewObject MUIOBJMACRO_START("NListview.mcc")
#endif

extern const STRPTR GM_UNIQUENAME(libname);

/* /// "Strings" */
static char *MainGUIPages[] = { "General", "Keyboard", "Action", NULL };
static char *MainGUIPagesDefault[] = { "General", "Keyboard", NULL };

static char *LLPortStrings[] = { "Don't touch", "Overwrite with USB", "Merge with USB", "Disable", "Analogue Hack", NULL };
static char *LLRumbleStrings[] = { "Off", "Port 0", "Port 1", "Port 2", "Port 3", NULL };

static char *TurboMouseStrings[] = { "Off", "1000 Hz", "500 Hz", "250 Hz", "125 Hz", NULL };

static char *ActionTypeStrings[] = { "No action", "Qualifiers", "Keymapping", "Raw key",
                                     "Vanilla key", "Keystring", "Mouse position", "Mouse buttons",
                                     "Tablet data", "Digital joystick", "Analogue joystick",
                                     "Scrollwheel", "Sound", "Shell", "Arexx", "HID output",
                                     "HID feature", "Miscellaneous", "Variables", "Ext. Raw key", NULL };

static char *ActionTriggerStrings[] = { "Down", "Up", "Any", "Always", "NaN", NULL };
static UWORD ActionTriggerVals[] = { HUA_DOWNEVENT, HUA_UPEVENT, HUA_ANY, HUA_ALWAYS, HUA_NAN, 0 };

static char *A_CCVariableStrings[] = { "Eval. item val", "Orig. item value", "Constant", "Click count", "Click time",
                                       "USB qualifiers", "All qualifiers", "Random bit", "Random value", "Timer",
                                       "Local var 1", "Local var 2", "Local var 3", "Local var 4",
                                       "Local var 5", "Local var 6", "Local var 7", "Local var 8",
                                       "Global var A", "Global var B", "Global var C", "Global var D",
                                       "Global var E", "Global var F", "Global var G", "Global var H",
                                       NULL };
static UWORD A_CCVariableVals[] = { HUAT_EITEMVALUE, HUAT_OITEMVALUE, HUAT_CONST, HUAT_CLICKCOUNT, HUAT_CLICKTIME,
                                    HUAT_QUALIFIERS, HUAT_ALLQUAL, HUAT_RANDOMBIT, HUAT_RANDOMVAL, HUAT_TIMER,
                                    HUAT_LOCALVAR1, HUAT_LOCALVAR2, HUAT_LOCALVAR3, HUAT_LOCALVAR4,
                                    HUAT_LOCALVAR5, HUAT_LOCALVAR6, HUAT_LOCALVAR7, HUAT_LOCALVAR8,
                                    HUAT_GLOBVARA, HUAT_GLOBVARB, HUAT_GLOBVARC, HUAT_GLOBVARD,
                                    HUAT_GLOBVARE, HUAT_GLOBVARF, HUAT_GLOBVARG, HUAT_GLOBVARH,
                                    0 };

static char *A_CCCondStrings[] = { "==", "!=", "<", "<=", ">", ">=", "and", "nand", "or", "xor", "and not",
                                   "bw and", "bw nand", "bw or", "bw xor", "bw and not", NULL };
static UWORD A_CCCondVals[] = { HUAT_EQ, HUAT_NE, HUAT_LT, HUAT_LE, HUAT_GT, HUAT_GE, HUAT_AND, HUAT_NAND, HUAT_OR, HUAT_XOR, HUAT_ANDNOT,
                                HUAT_BWAND, HUAT_BWNAND, HUAT_BWOR, HUAT_BWXOR, HUAT_BWANDNOT, 0 };

static char *A_QualOpStrings[] = { "Set", "Clear", "Toggle", "Assign", NULL };
static UWORD A_QualOpVals[] = { HUAT_SET, HUAT_CLEAR, HUAT_TOGGLE, HUAT_ASSIGN, 0 };
static char *A_QualifierStrings[] = { "Left shift", "Right shift", "Caps lock", "Control",
                                      "Left alt", "Right alt", "Left amiga", "Right amiga",
                                      "Numeric pad", NULL };

static char *A_MousePosOpStrings[] = { "relative horizontal", "relative vertical", "to absolute X", "to absolute Y", NULL };
static UWORD A_MousePosOpVals[] = { HUAT_DELTAX, HUAT_DELTAY, HUAT_ABSX, HUAT_ABSY, 0 };

static char *A_MouseButOpStrings[] = { "Press", "Release", "Flip", "Assign", NULL };
static UWORD A_MouseButOpVals[] = { HUAT_SET, HUAT_CLEAR, HUAT_TOGGLE, HUAT_ASSIGN, 0 };
static char *A_MouseButStrings[] = { "left", "right", "middle", "fourth", "fifth", NULL };

static char *A_TabletAxisStrings[] = { "pressure", "X rotation", "Y rotation", "Z rotation", "proximity", "Z position", NULL };
static UWORD A_TabletAxisVals[] = { HUAT_PRESSURE, HUAT_XROT, HUAT_YROT, HUAT_ZROT, HUAT_PROX, HUAT_ABSZ, 0 };

static char *A_WheelOpStrings[] = { "horizontal movement", "vertical movement", "left (by distance)", "right (by distance)",
                                    "up (by distance)", "down (by distance)", NULL };
static UWORD A_WheelOpVals[] = { HUAT_DELTAX, HUAT_DELTAY, HUAT_LEFT, HUAT_RIGHT, HUAT_UP, HUAT_DOWN, 0 };

static char *A_JoypadOpStrings[] = { "Push", "Release", "Toggle", "Assign", NULL };
static UWORD A_JoypadOpVals[] = { HUAT_SET, HUAT_CLEAR, HUAT_TOGGLE, HUAT_ASSIGN, 0 };

static char *A_JoypadFeatStrings[] = { "left", "right", "up", "down", "hatswitch",
                                       "red (fire)", "blue (2nd)", "green (shuffle)", "yellow (repeat)",
                                       "forward", "reverse", "play/pause", NULL };
static UWORD A_JoypadFeatVals[] = { HUAT_LEFT, HUAT_RIGHT, HUAT_UP, HUAT_DOWN, HUAT_HATSWITCH,
                                    HUAT_RED, HUAT_BLUE, HUAT_GREEN, HUAT_YELLOW,
                                    HUAT_FORWARD, HUAT_REVERSE, HUAT_PLAY, 0 };

static char *A_APadFeatStrings[] = { "X axis", "Y axis", NULL };
static UWORD A_APadFeatVals[] = { HUAT_ABSX, HUAT_ABSY, 0 };

static char *A_TarVariableStrings[] = { "Local var 1", "Local var 2", "Local var 3", "Local var 4",
                                        "Local var 5", "Local var 6", "Local var 7", "Local var 8",
                                        "Global var A", "Global var B", "Global var C", "Global var D",
                                        "Global var E", "Global var F", "Global var G", "Global var H",
                                        NULL };
static UWORD A_TarVariableVals[] = { HUAT_LOCALVAR1, HUAT_LOCALVAR2, HUAT_LOCALVAR3, HUAT_LOCALVAR4,
                                     HUAT_LOCALVAR5, HUAT_LOCALVAR6, HUAT_LOCALVAR7, HUAT_LOCALVAR8,
                                     HUAT_GLOBVARA, HUAT_GLOBVARB, HUAT_GLOBVARC, HUAT_GLOBVARD,
                                     HUAT_GLOBVARE, HUAT_GLOBVARF, HUAT_GLOBVARG, HUAT_GLOBVARH,
                                     0 };

static char *A_TarVarOpStrings[] = { "assign :=", "not := !", "add +=", "sub -=", "mult *=", "div /=", "mod %=",
                                     "and (x && y)", "nand !(x && y)", "or (x || y)", "xor (x ^^ y)", "and not (x && !y)",
                                     "bw and (x & y)", "bw nand ~(x & y)", "bw or (x | y)", "bw xor (x ^ y)", "bw and not (x & ~y)",
                                     "shift <- (x << y)", "shift -> (x >> y)",
                                     NULL };
static UWORD A_TarVarOpVals[] = { HUAT_ASSIGN, HUAT_ASSNOT, HUAT_ADD, HUAT_SUB, HUAT_MULTIPLY, HUAT_DIVIDE, HUAT_MODULO,
                                  HUAT_AND, HUAT_NAND, HUAT_OR, HUAT_XOR, HUAT_ANDNOT,
                                  HUAT_BWAND, HUAT_BWNAND, HUAT_BWOR, HUAT_BWXOR, HUAT_BWANDNOT,
                                  HUAT_ASL, HUAT_ASR, 0 };

static char *A_OutOpStrings[] = { "Set", "Clear", "Toggle", "Assign", NULL };
static UWORD A_OutOpVals[] = { HUAT_SET, HUAT_CLEAR, HUAT_TOGGLE, HUAT_ASSIGN, 0 };

/* JPB_JOY_LEFT, JPB_JOY_RIGHT, JPB_JOY_UP, JPB_JOY_DOWN,
                                    JPB_BUTTON_RED, JPB_BUTTON_BLUE, JPB_BUTTON_GREEN, JPB_BUTTON_YELLOW,
                                    JPB_BUTTON_FORWARD, JPB_BUTTON_REVERSE, JPB_BUTTON_PLAY,
                                    0xffff, 0 }; */

static char *A_JoypadPortStrings[] = { "port 0", "port 1", "port 2", "port 3", NULL };

static char *A_MiscOpStrings[] = { "Activate window", "Window to front", "Window to back", "Close window",
                                   "Zip window", "Screen cycle", "WB to front", "Display beep",
                                   "Reboot machine", "Flush events", NULL };
static UWORD A_MiscOpVals[] = { HUAT_ACTWINDOW, HUAT_WIN2FRONT, HUAT_WIN2BACK, HUAT_CLOSEWINDOW,
                                HUAT_ZIPWINDOW, HUAT_SCREENCYCLE, HUAT_WB2FRONT, HUAT_DISPLAYBEEP,
                                HUAT_REBOOT, HUAT_FLUSHEVENTS, 0 };
/* \\\ */

/* /// "bRevLookup()" */
ULONG bRevLookup(UWORD id, UWORD def, UWORD *field)
{
    ULONG res = 0;
    while(*field)
    {
        if(*field++ == id)
        {
            return(res);
        }
        res++;
    }
    return(def);
}
/* \\\ */

/* /// "bGUITask()" */
AROS_UFH0(void, GM_UNIQUENAME(bGUITask))
{
    AROS_USERFUNC_INIT

    struct Task *thistask;
    struct BTHidBase *nh;
    struct BTHidBinding *nhb;
    UWORD count;
    struct HidUsageIDMap *hum;

    char barbar[] = "BAR,BAR,";
    //char barbarbar[] = "BAR,BAR,BAR,";
    char barbarbarbar[] = "BAR,BAR,BAR,BAR,";

    thistask = FindTask(NULL);
#undef BluetoothBase
#define BluetoothBase nhb->nhb_BtBase
#undef IntuitionBase
#define IntuitionBase nhb->nhb_IntBase
#undef KeymapBase
#define KeymapBase nhb->nhb_KeyBase
#undef MUIMasterBase
#define MUIMasterBase nhb->nhb_MUIBase

    nhb = thistask->tc_UserData;
    nh = nhb->nhb_ClsBase;

    ++nh->nh_Library.lib_OpenCnt;
    NewList(&nhb->nhb_GUIItems);
    NewList(&nhb->nhb_GUIOutItems);
    if(!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)))
    {
        KPRINTF(10, ("Couldn't open muimaster.library.\n"));
        GM_UNIQUENAME(bGUITaskCleanup)(nhb);
        return;
    }

    if(!(IntuitionBase = OpenLibrary("intuition.library", 39)))
    {
        KPRINTF(10, ("Couldn't open intuition.library.\n"));
        GM_UNIQUENAME(bGUITaskCleanup)(nhb);
        return;
    }
    if(!(KeymapBase = OpenLibrary("keymap.library", 39)))
    {
        KPRINTF(10, ("Couldn't open keymap.library.\n"));
        GM_UNIQUENAME(bGUITaskCleanup)(nhb);
        return;
    }
    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        KPRINTF(10, ("Couldn't open bluetooth.library.\n"));
        GM_UNIQUENAME(bGUITaskCleanup)(nhb);
        return;
    }

    if(nhb->nhb_Service)
    {
        STRPTR devname = NULL;
        btGetAttrs(BGA_DEVICE, nhb->nhb_Device, BDA_Name, &devname, TAG_END);
        btSafeRawDoFmt(nhb->nhb_WinTitle, sizeof(nhb->nhb_WinTitle), "%s: %s", GM_UNIQUENAME(libname), devname ? devname : (STRPTR) "device");
    } else {
        btSafeRawDoFmt(nhb->nhb_WinTitle, sizeof(nhb->nhb_WinTitle), "%s defaults", GM_UNIQUENAME(libname));
    }

    nhb->nhb_ActionClass = MUI_CreateCustomClass(NULL, MUIC_Area  , NULL, sizeof(struct ActionData), GM_UNIQUENAME(ActionDispatcher));
    if(!nhb->nhb_ActionClass)
    {
        KPRINTF(10, ("Couldn't create ActionClass.\n"));
        GM_UNIQUENAME(bGUITaskCleanup)(nhb);
        return;
    }
    for(count = 0; count < 128; count++)
    {
        struct InputEvent ie;
        UBYTE buf[80];
        UBYTE buf2[80];
        LONG actual;
        BOOL printable;
        WORD charpos;
        LONG targetpos;
        STRPTR betterstr;

        ie.ie_Class = IECLASS_RAWKEY;
        ie.ie_SubClass = 0;
        ie.ie_Qualifier = 0;
        ie.ie_Code = count;
        ie.ie_EventAddress = NULL;

        if((betterstr = bNumToStr(nhb, NTS_RAWKEY, (ULONG) count, NULL)))
        {
            strcpy(buf, betterstr);
            actual = strlen(buf);
        } else {
            actual = MapRawKey(&ie, buf, 15, NULL);
        }
        if(actual > 0)
        {
            printable = TRUE;
            charpos = 0;
            do
            {
                if((buf[charpos] < 0x20) || ((buf[charpos] >= 0x80) && (buf[charpos] < 0xa0)))
                {
                    printable = FALSE;
                    break;
                }
            } while(++charpos < actual);
            if(printable)
            {
                buf[actual] = 0;
                nhb->nhb_RawKeyArray[count] = btCopyStrFmt("0x%02lx (%s)", count, buf);
            } else {
                strcpy(buf2, "unprintable seq.");
                charpos = 0;
                do
                {
                    targetpos = strlen(buf2);
                    buf2[targetpos++] = ' ';
                    btSafeRawDoFmt(&buf2[targetpos], 79-targetpos, "$%02lx", buf[charpos]);
                } while((++charpos < actual) && targetpos < 75);
                nhb->nhb_RawKeyArray[count] = btCopyStrFmt("0x%02lx (%s)", count, buf2);
            }
        } else {
            nhb->nhb_RawKeyArray[count] = btCopyStrFmt("0x%02lx", count);
        }
    }

    for(count = 0; count < 128; count++)
    {
        nhb->nhb_ExtRawKeyArray[count] = btCopyStrFmt("0x%02lx %s", count, bNumToStr(nhb, NTS_EXTRAWKEY, (ULONG) count, "<undef.>"));
    }

    hum = (struct HidUsageIDMap *) hidusage07;
    count = 0;
    while(hum->hum_String)
    {
        nhb->nhb_USBKeyArray[count] = hum++;
        count++;
    }

    nhb->nhb_USBKeyListDisplayHook.h_Data = nhb;
    nhb->nhb_ReportListDisplayHook.h_Data = nhb;
    nhb->nhb_ItemListDisplayHook.h_Data = nhb;
    nhb->nhb_ActionListDisplayHook.h_Data = nhb;

    nhb->nhb_USBKeyListDisplayHook.h_Entry = (APTR) GM_UNIQUENAME(USBKeyListDisplayHook);
    nhb->nhb_ReportListDisplayHook.h_Entry = (APTR) GM_UNIQUENAME(ReportListDisplayHook);
    nhb->nhb_ItemListDisplayHook.h_Entry = (APTR) GM_UNIQUENAME(ItemListDisplayHook);
    nhb->nhb_ActionListDisplayHook.h_Entry = (APTR) GM_UNIQUENAME(ActionListDisplayHook);

    nhb->nhb_App = ApplicationObject,
        MUIA_Application_Title      , (IPTR)GM_UNIQUENAME(libname),
        MUIA_Application_Version    , (IPTR)VERSION_STRING,
        MUIA_Application_Copyright  , (IPTR)ISOASCII_COPYRIGHT "2002-2009 Chris Hodges, 2026 The AROS Development Team",
        MUIA_Application_Author     , (IPTR)"Chris Hodges <chrisly@platon42.de>",
        MUIA_Application_Description, (IPTR)"Settings for the Bluetooth HID class",
        MUIA_Application_Base       , (IPTR)"BTHID",
        MUIA_Application_Menustrip  , (IPTR)MenustripObject,
            Child, (IPTR)MenuObjectT((IPTR)"Project"),
                Child, (IPTR)(nhb->nhb_AboutMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"About...",
                    MUIA_Menuitem_Shortcut, (IPTR)"?",
                    End),
                End,
            Child, (IPTR)MenuObjectT((IPTR)"Quick Setup"),
                //Child, (IPTR)MUIA_Menu_Enabled, nhb->nhb_Service ? TRUE : FALSE,
                Child, (IPTR)MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"Mouse",
                    Child, (IPTR)(nhb->nhb_SwapLMBRMBMI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"Swap LMB<->RMB",
                        End),
                    Child, (IPTR)MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)NM_BARLABEL,
                        End,
                    Child, (IPTR)(nhb->nhb_MouseAccel100MI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"Acceleration off",
                        MUIA_Menuitem_Enabled, FALSE,
                        End),
                    Child, (IPTR)(nhb->nhb_MouseAccel150MI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"Acceleration 150%",
                        MUIA_Menuitem_Enabled, FALSE,
                        End),
                    Child, (IPTR)(nhb->nhb_MouseAccel200MI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"Acceleration 200%",
                        MUIA_Menuitem_Enabled, FALSE,
                        End),
                    End,
                Child, (IPTR)MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"Joystick",
                    Child, (IPTR)(nhb->nhb_JoyPort0MI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"Select Port 0",
                        End),
                    Child, (IPTR)(nhb->nhb_JoyPort1MI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"Select Port 1",
                        End),
                    Child, (IPTR)(nhb->nhb_JoyPort2MI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"Select Port 2",
                        End),
                    Child, (IPTR)(nhb->nhb_JoyPort3MI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"Select Port 3",
                        End),
                    Child, (IPTR)MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)NM_BARLABEL,
                        End,
                    Child, (IPTR)(nhb->nhb_JoyAutofireMI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"Add Autofire Actions",
                        MUIA_Menuitem_Enabled, FALSE,
                        End),
                    End,
                End,
            Child, (IPTR)MenuObjectT((IPTR)"Settings"),
                Child, (IPTR)(nhb->nhb_UseMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"Save",
                    MUIA_Menuitem_Shortcut, (IPTR)"S",
                    End),
                Child, (IPTR)(nhb->nhb_SetDefaultMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"Save as Default",
                    MUIA_Menuitem_Shortcut, (IPTR)"D",
                    End),
                Child, (IPTR)MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)NM_BARLABEL,
                    End,
                Child, (IPTR)(nhb->nhb_MUIPrefsMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"MUI Settings",
                    MUIA_Menuitem_Shortcut, (IPTR)"M",
                    End),
                End,
            Child, (IPTR)MenuObjectT((IPTR)"Debug"),
                Child, (IPTR)(nhb->nhb_DebugReportMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"Report Descriptor",
                    End),
                End,
            End,

        SubWindow, (IPTR)(nhb->nhb_MainWindow = WindowObject,
            MUIA_Window_ID   , MAKE_ID('M','A','I','N'),
            MUIA_Window_Title, (IPTR)nhb->nhb_WinTitle,
            MUIA_HelpNode, (IPTR)GM_UNIQUENAME(libname),

            WindowContents, (IPTR)VGroup,
                Child, (IPTR)(nhb->nhb_ActionObj = NewObject(nhb->nhb_ActionClass->mcc_Class, 0, MUIA_ShowMe, FALSE, TAG_END)),
                Child, (IPTR)RegisterGroup(nhb->nhb_Service ? MainGUIPages : MainGUIPagesDefault),
                    MUIA_CycleChain, 1,
                    MUIA_Register_Frame, TRUE,
                    Child, (IPTR)VGroup,
                        Child, (IPTR)VSpace(0),
                        Child, (IPTR)ColGroup(2), GroupFrameT(nhb->nhb_Service ? "Device Settings" : "Default Device Settings"),
                            //Child, (IPTR)HSpace(0),
                            Child, (IPTR)Label((IPTR) "Shell console window:"),
                            Child, (IPTR)(nhb->nhb_ConWindowObj = StringObject,
                                StringFrame,
                                MUIA_CycleChain, 1,
                                MUIA_String_AdvanceOnCR, TRUE,
                                MUIA_String_Contents, (IPTR)nhb->nhb_CDC->cdc_ShellCon,
                                MUIA_String_MaxLen, 127,
                                End),
                            Child, (IPTR)Label((IPTR) "Shell default stack:"),
                            Child, (IPTR)(nhb->nhb_ShellStackObj = StringObject,
                                StringFrame,
                                MUIA_CycleChain, 1,
                                MUIA_String_AdvanceOnCR, TRUE,
                                MUIA_String_Integer, nhb->nhb_CDC->cdc_ShellStack,
                                MUIA_String_Accept, (IPTR)"0123456789",
                                End),
                            Child, (IPTR)Label((IPTR) "Enable keyboard reset:"),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)(nhb->nhb_EnableKBResetObj = ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, nhb->nhb_CDC->cdc_EnableKBReset,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                Child, (IPTR)HSpace(0),
                                Child, (IPTR)Label((IPTR) "Hijack ResetHandlers:"),
                                Child, (IPTR)(nhb->nhb_EnableRHObj = ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, nhb->nhb_CDC->cdc_EnableRH,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                End,
                            Child, (IPTR)Label((IPTR) "Reset delay:"),
                            Child, (IPTR)(nhb->nhb_ResetDelayObj = SliderObject, SliderFrame,
                                MUIA_CycleChain, 1,
                                MUIA_Numeric_Min, 0,
                                MUIA_Numeric_Max, 60,
                                MUIA_Numeric_Value, nhb->nhb_CDC->cdc_ResetDelay,
                                MUIA_Numeric_Format, (IPTR) "%ldsec",
                                End),
                            Child, (IPTR)Label((IPTR) "Turbo mouse:"),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)(nhb->nhb_TurboMouseObj = CycleObject,
                                    MUIA_CycleChain, 1,
                                    MUIA_Cycle_Entries, (IPTR)TurboMouseStrings,
                                    MUIA_Cycle_Active, nhb->nhb_CDC->cdc_TurboMouse,
                                    End),
                                Child, (IPTR)HSpace(0),
                                End,
                            End,
                        Child, (IPTR)VSpace(0),
                        Child, (IPTR)ColGroup(2), GroupFrameT("HID Output Control Window"),
                            Child, (IPTR)Label((IPTR) "Open on startup:"),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)(nhb->nhb_HIDCtrlAutoObj = ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, nhb->nhb_CDC->cdc_HIDCtrlOpen,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                Child, (IPTR)HSpace(0),
                                Child, (IPTR)(nhb->nhb_HIDCtrlOpenObj = TextObject, ButtonFrame,
                                    MUIA_ShowMe, (IPTR)nhb->nhb_Service,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_RelVerify,
                                    MUIA_Text_Contents, (IPTR)"\33c Open now ",
                                    End),
                                End,
                            Child, (IPTR)Label((IPTR) "Window Title:"),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)(nhb->nhb_HIDCtrlTitleObj = StringObject,
                                    StringFrame,
                                    MUIA_CycleChain, 1,
                                    MUIA_String_AdvanceOnCR, TRUE,
                                    MUIA_String_Contents, (IPTR)nhb->nhb_CDC->cdc_HIDCtrlTitle,
                                    MUIA_String_MaxLen, 31,
                                    End),
                                Child, (IPTR)Label((IPTR) "Rexx Port:"),
                                Child, (IPTR)(nhb->nhb_HIDCtrlRexxObj = StringObject,
                                    StringFrame,
                                    MUIA_CycleChain, 1,
                                    MUIA_String_AdvanceOnCR, TRUE,
                                    MUIA_String_Contents, (IPTR)nhb->nhb_CDC->cdc_HIDCtrlRexx,
                                    MUIA_String_MaxLen, 31,
                                    End),
                                End,
                            End,
                        Child, (IPTR)VSpace(0),
                        Child, (IPTR)ColGroup(4), GroupFrameT("LowLevel Library Joypad emulation"),
                            Child, (IPTR)Label((IPTR) "Port 0:"),
                            Child, (IPTR)(nhb->nhb_LLPortModeObj[0] = CycleObject,
                                MUIA_CycleChain, 1,
                                MUIA_Cycle_Entries, (IPTR)LLPortStrings,
                                MUIA_Cycle_Active, nhb->nhb_CDC->cdc_LLPortMode[0],
                                End),
                            Child, (IPTR)Label((IPTR) "Port 2:"),
                            Child, (IPTR)(nhb->nhb_LLPortModeObj[2] = CycleObject,
                                MUIA_CycleChain, 1,
                                MUIA_Cycle_Entries, (IPTR)LLPortStrings,
                                MUIA_Cycle_Active, nhb->nhb_CDC->cdc_LLPortMode[2],
                                End),
                            Child, (IPTR)Label((IPTR) "Port 1:"),
                            Child, (IPTR)(nhb->nhb_LLPortModeObj[1] = CycleObject,
                                MUIA_CycleChain, 1,
                                MUIA_Cycle_Entries, (IPTR)LLPortStrings,
                                MUIA_Cycle_Active, nhb->nhb_CDC->cdc_LLPortMode[1],
                                End),
                            Child, (IPTR)Label((IPTR) "Port 3:"),
                            Child, (IPTR)(nhb->nhb_LLPortModeObj[3] = CycleObject,
                                MUIA_CycleChain, 1,
                                MUIA_Cycle_Entries, (IPTR)LLPortStrings,
                                MUIA_Cycle_Active, nhb->nhb_CDC->cdc_LLPortMode[3],
                                End),
                            Child, (IPTR)Label((IPTR) "Rumble Port:"),
                            Child, (IPTR)(nhb->nhb_LLRumblePortObj = CycleObject,
                                MUIA_CycleChain, 1,
                                MUIA_Cycle_Entries, (IPTR)LLRumbleStrings,
                                MUIA_Cycle_Active, nhb->nhb_CDC->cdc_LLRumblePort,
                                End),
                            Child, (IPTR)HSpace(0),
                            Child, (IPTR)HSpace(0),
                            End,
                        Child, (IPTR)VSpace(0),
                        End,
                    Child, (IPTR)VGroup,
                        //Child, (IPTR)VSpace(0),
                        Child, (IPTR)HGroup, GroupFrameT((IPTR)(nhb->nhb_Service ? "Keyboard mapping" : "Default Keyboard mapping")),
                            Child, (IPTR)VGroup,
                                Child, (IPTR)HGroup,
                                    Child, (IPTR)(nhb->nhb_USBKeymapLVObj = ListviewObject,
                                        MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
                                        MUIA_Listview_List, (IPTR)ListObject,
                                            MUIA_CycleChain, 1,
                                            InputListFrame,
                                            MUIA_List_SourceArray, (IPTR)nhb->nhb_USBKeyArray,
                                            MUIA_List_DisplayHook, (IPTR)&nhb->nhb_USBKeyListDisplayHook,
                                            MUIA_List_AutoVisible, TRUE,
                                            End,
                                        End),
                                    Child, (IPTR)VGroup,
                                        Child, (IPTR)VSpace(0),
                                        Child, (IPTR)Label((IPTR) "->"),
                                        Child, (IPTR)VSpace(0),
                                        End,
                                    Child, (IPTR)(nhb->nhb_RawKeymapLVObj = ListviewObject,
                                        MUIA_Listview_Input, TRUE,
                                        MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
                                        MUIA_Listview_List, (IPTR)ListObject,
                                            InputListFrame,
                                            MUIA_CycleChain, 1,
                                            MUIA_List_SourceArray, (IPTR)nhb->nhb_RawKeyArray,
                                            MUIA_List_AutoVisible, TRUE,
                                            End,
                                        End),
                                    End,
                                Child, (IPTR)HGroup,
                                    Child, (IPTR)(nhb->nhb_RestoreDefKeymapObj = TextObject, ButtonFrame,
                                        MUIA_Background, MUII_ButtonBack,
                                        MUIA_CycleChain, 1,
                                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                                        MUIA_Text_Contents, (IPTR)"\33c Restore default keymap ",
                                        End),
                                    Child, (IPTR)HSpace(0),
                                    Child, (IPTR)(nhb->nhb_TrackKeyEventsObj = ImageObject, ImageButtonFrame,
                                        MUIA_Background, MUII_ButtonBack,
                                        MUIA_CycleChain, 1,
                                        MUIA_InputMode, MUIV_InputMode_Toggle,
                                        MUIA_Image_Spec, MUII_CheckMark,
                                        MUIA_Image_FreeVert, TRUE,
                                        MUIA_Disabled, !nhb->nhb_Service,
                                        MUIA_ShowSelState, FALSE,
                                        End),
                                    Child, (IPTR)Label((IPTR) "Track incoming key events"),
                                    End,
                                End,
                            End,
                        //Child, (IPTR)VSpace(0),
                        End,
                    Child, (IPTR)VGroup,
                        //Child, (IPTR)VSpace(0),
                        Child, (IPTR)VGroup, GroupFrameT((IPTR)"Action handling"),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)VGroup, GroupFrameT((IPTR)"Reports and collections"),
                                    MUIA_HorizWeight, 10,
                                    Child, (IPTR)(nhb->nhb_ReportLVObj = ListviewObject,
                                        MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
                                        MUIA_Listview_List, (IPTR)ListObject,
                                            MUIA_CycleChain, 1,
                                            InputListFrame,
                                            MUIA_List_DisplayHook, (IPTR)&nhb->nhb_ReportListDisplayHook,
                                            MUIA_List_AutoVisible, TRUE,
                                            End,
                                        End),
                                    Child, (IPTR)HGroup,
                                        Child, (IPTR)(nhb->nhb_FillDefObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                                            MUIA_Text_Contents, (IPTR)"\33c Fill defaults ",
                                            End),
                                        Child, (IPTR)(nhb->nhb_ClearActionsObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                                            MUIA_Text_Contents, (IPTR)"\33c Clear actions ",
                                            End),
                                        End,
                                    End,
                                Child, (IPTR)VGroup, GroupFrameT((IPTR)"Usage items"),
                                    Child, (IPTR)(nhb->nhb_ItemLVObj = ListviewObject,
                                        MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
                                        MUIA_Listview_List, (IPTR)ListObject,
                                            InputListFrame,
                                            MUIA_CycleChain, 1,
                                            MUIA_List_DisplayHook, (IPTR)&nhb->nhb_ItemListDisplayHook,
                                            MUIA_List_Format, (IPTR)barbarbarbar,
                                            MUIA_List_Title, TRUE,
                                            MUIA_List_AutoVisible, TRUE,
                                            End,
                                        End),
                                    End,
                                End,
                            Child, (IPTR)HGroup,
                                Child, (IPTR)(nhb->nhb_TrackEventsObj = ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    //MUIA_Selected,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                Child, (IPTR)Label((IPTR) "Track incoming events"),
                                Child, (IPTR)HSpace(0),
                                Child, (IPTR)(nhb->nhb_ReportValuesObj = ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    //MUIA_Selected,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                Child, (IPTR)Label((IPTR) "Report current values"),
                                Child, (IPTR)HSpace(0),
                                Child, (IPTR)(nhb->nhb_DisableActionsObj = ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    //MUIA_Selected,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                Child, (IPTR)Label((IPTR) "Disable all actions"),
                                End,
                            Child, (IPTR)HGroup, GroupFrameT("Performed actions"),
                                Child, (IPTR)VGroup,
                                    Child, (IPTR)(nhb->nhb_ActionLVObj = ListviewObject,
                                        MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
                                        MUIA_Listview_List, (IPTR)ListObject,
                                            InputListFrame,
                                            MUIA_CycleChain, 1,
                                            MUIA_List_DisplayHook, (IPTR)&nhb->nhb_ActionListDisplayHook,
                                            MUIA_List_Format, (IPTR)barbar,
                                            MUIA_List_Title, TRUE,
                                            MUIA_List_AutoVisible, TRUE,
                                            End,
                                        End),
                                    Child, (IPTR)HGroup,
                                        Child, (IPTR)(nhb->nhb_ActionNewObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                                            MUIA_Text_Contents, (IPTR)"\33c New ",
                                            End),
                                        Child, (IPTR)(nhb->nhb_ActionCopyObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                                            MUIA_Text_Contents, (IPTR)"\33c Copy ",
                                            End),
                                        Child, (IPTR)(nhb->nhb_ActionDelObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                                            MUIA_Text_Contents, (IPTR)"\33c Del ",
                                            End),
                                        Child, (IPTR)(nhb->nhb_ActionUpObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                                            MUIA_Text_Contents, (IPTR)"\33c Up ",
                                            End),
                                        Child, (IPTR)(nhb->nhb_ActionDownObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                                            MUIA_Text_Contents, (IPTR)"\33c Down ",
                                            End),
                                        End,
                                    End,
                                Child, (IPTR)(nhb->nhb_ActionAreaObj = VGroup,
                                    MUIA_Disabled, TRUE,
                                    Child, (IPTR)HGroup,
                                        Child, (IPTR)(nhb->nhb_ActionSelectorObj = CycleObject,
                                            MUIA_CycleChain, 1,
                                            MUIA_Cycle_Entries, (IPTR)ActionTypeStrings,
                                            MUIA_Cycle_Active, 0,
                                            End),
                                        Child, (IPTR)Label((IPTR) "Trigger:"),
                                        Child, (IPTR)(nhb->nhb_ActionTriggerObj = CycleObject,
                                            MUIA_CycleChain, 1,
                                            MUIA_Cycle_Entries, (IPTR)ActionTriggerStrings,
                                            MUIA_Cycle_Active, 0,
                                            End),
                                        End,
                                    Child, (IPTR)HGroup,
                                        Child, (IPTR)Label((IPTR) "Opts:"),
                                        Child, (IPTR)(nhb->nhb_ActionAbsToRelObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_Toggle,
                                            MUIA_Text_Contents, (IPTR)"\33c Abs->Rel ",
                                            End),
                                        Child, (IPTR)(nhb->nhb_ActionClipEnableObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_Toggle,
                                            MUIA_Text_Contents, (IPTR)"\33c Clip ",
                                            End),
                                        Child, (IPTR)(nhb->nhb_ActionScaleEnableObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_Toggle,
                                            MUIA_Text_Contents, (IPTR)"\33c Scale ",
                                            End),
                                        Child, (IPTR)(nhb->nhb_ActionCCEnableObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_Toggle,
                                            MUIA_Text_Contents, (IPTR)"\33c CC ",
                                            End),
                                        Child, (IPTR)(nhb->nhb_ActionValEnableObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_Toggle,
                                            MUIA_Text_Contents, (IPTR)"\33c Val ",
                                            End),
                                        End,
                                    Child, (IPTR)(nhb->nhb_A_ClipGroupObj = HGroup, GroupFrameT("Clipping"),
                                        MUIA_ShowMe, FALSE,
                                        Child, (IPTR)Label((IPTR) "Min:"),
                                        Child, (IPTR)(nhb->nhb_A_ClipMinObj = SliderObject, SliderFrame,
                                            MUIA_CycleChain, 1,
                                            MUIA_Numeric_Min, 0,
                                            MUIA_Numeric_Max, 100,
                                            MUIA_Numeric_Format, (IPTR)"%ld%%",
                                            End),
                                        Child, (IPTR)Label((IPTR) "Max:"),
                                        Child, (IPTR)(nhb->nhb_A_ClipMaxObj = SliderObject, SliderFrame,
                                            MUIA_CycleChain, 1,
                                            MUIA_Numeric_Min, 0,
                                            MUIA_Numeric_Max, 100,
                                            MUIA_Numeric_Format, (IPTR)"%ld%%",
                                            End),
                                        Child, (IPTR)Label((IPTR) "Stretch:"),
                                        Child, (IPTR)(nhb->nhb_A_ClipStretchObj = ImageObject, ImageButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_Toggle,
                                            MUIA_Image_Spec, MUII_CheckMark,
                                            MUIA_Image_FreeVert, TRUE,
                                            //MUIA_Selected,
                                            MUIA_ShowSelState, FALSE,
                                            End),
                                        End),
                                    Child, (IPTR)(nhb->nhb_A_ScaleGroupObj = HGroup, GroupFrameT("Scaling"),
                                        MUIA_ShowMe, FALSE,
                                        Child, (IPTR)Label((IPTR) "Min:"),
                                        Child, (IPTR)(nhb->nhb_A_ScaleMinObj = StringObject,
                                            StringFrame,
                                            MUIA_String_Accept, (IPTR)"0123456789-",
                                            MUIA_CycleChain, 1,
                                            MUIA_String_AdvanceOnCR, TRUE,
                                            MUIA_String_MaxLen, 10,
                                            End),
                                        Child, (IPTR)Label((IPTR) "Max:"),
                                        Child, (IPTR)(nhb->nhb_A_ScaleMaxObj = StringObject,
                                            StringFrame,
                                            MUIA_String_Accept, (IPTR)"0123456789-",
                                            MUIA_CycleChain, 1,
                                            MUIA_String_AdvanceOnCR, TRUE,
                                            MUIA_String_MaxLen, 10,
                                            End),
                                        End),
                                    Child, (IPTR)(nhb->nhb_A_CCGroupObj = VGroup, GroupFrameT("Pre-condition code"),
                                        MUIA_ShowMe, FALSE,
                                        Child, (IPTR)HGroup,
                                            Child, (IPTR)Label((IPTR) "If"),
                                            Child, (IPTR)(nhb->nhb_A_CCVar1Obj = CycleObject,
                                                MUIA_CycleChain, 1,
                                                MUIA_Cycle_Entries, (IPTR)A_CCVariableStrings,
                                                End),
                                            Child, (IPTR)(nhb->nhb_A_CCCondObj = CycleObject,
                                                MUIA_CycleChain, 1,
                                                MUIA_Cycle_Entries, (IPTR)A_CCCondStrings,
                                                End),
                                            Child, (IPTR)(nhb->nhb_A_CCVar2Obj = CycleObject,
                                                MUIA_CycleChain, 1,
                                                MUIA_Cycle_Entries, (IPTR)A_CCVariableStrings,
                                                End),
                                            End,
                                        Child, (IPTR)HGroup,
                                            Child, (IPTR)Label((IPTR) "Left constant:"),
                                            Child, (IPTR)(nhb->nhb_A_CCConst1Obj = StringObject,
                                                StringFrame,
                                                MUIA_String_Accept, (IPTR)"0123456789-",
                                                MUIA_CycleChain, 1,
                                                MUIA_String_AdvanceOnCR, TRUE,
                                                MUIA_String_MaxLen, 10,
                                                End),
                                            Child, (IPTR)Label((IPTR) "Right constant:"),
                                            Child, (IPTR)(nhb->nhb_A_CCConst2Obj = StringObject,
                                                StringFrame,
                                                MUIA_String_Accept, (IPTR)"0123456789-",
                                                MUIA_CycleChain, 1,
                                                MUIA_String_AdvanceOnCR, TRUE,
                                                MUIA_String_MaxLen, 10,
                                                End),
                                            End,
                                        End),
                                    Child, (IPTR)(nhb->nhb_A_ValGroupObj = VGroup, GroupFrameT("Input value redirection"),
                                        MUIA_ShowMe, FALSE,
                                        Child, (IPTR)HGroup,
                                            Child, (IPTR)Label((IPTR) "Take value for action from"),
                                            Child, (IPTR)(nhb->nhb_A_ValVarObj = CycleObject,
                                                MUIA_CycleChain, 1,
                                                MUIA_Cycle_Entries, (IPTR)A_CCVariableStrings,
                                                End),
                                            End,
                                        Child, (IPTR)HGroup,
                                            Child, (IPTR)Label((IPTR) "Constant:"),
                                            Child, (IPTR)(nhb->nhb_A_ValConstObj = StringObject,
                                                StringFrame,
                                                MUIA_String_Accept, (IPTR)"0123456789-",
                                                MUIA_CycleChain, 1,
                                                MUIA_String_AdvanceOnCR, TRUE,
                                                MUIA_String_MaxLen, 10,
                                                End),
                                            End,
                                        End),
                                    Child, (IPTR)(nhb->nhb_ActionPageObj = VGroup,
                                        MUIA_Group_PageMode, TRUE,
                                        //MUIA_Group_ActivePage, MUIV_Group_ActivePage_First,
                                        Child, (IPTR)VGroup, GroupFrameT("No action"), /* HUA_NOP */
                                            Child, (IPTR)VSpace(0),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)HSpace(0),
                                                Child, (IPTR)Label((IPTR) "Does absolutely nothing."),
                                                Child, (IPTR)HSpace(0),
                                                End,
                                            Child, (IPTR)VSpace(0),
                                            End,
                                        Child, (IPTR)VGroup, GroupFrameT("Change key qualifiers"), /* HUA_QUALIFIER */
                                            Child, (IPTR)VSpace(0),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)HSpace(0),
                                                Child, (IPTR)(nhb->nhb_A_KeyQualOpObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_QualOpStrings,
                                                    End),
                                                Child, (IPTR)Label((IPTR) "qualifier"),
                                                Child, (IPTR)(nhb->nhb_A_KeyQualObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_QualifierStrings,
                                                    End),
                                                Child, (IPTR)HSpace(0),
                                                End,
                                            Child, (IPTR)VSpace(0),
                                            End,
                                        Child, (IPTR)VGroup, GroupFrameT("USB Keyboard mapping"), /* HUA_KEYMAP */
                                            Child, (IPTR)VSpace(0),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)HSpace(0),
                                                Child, (IPTR)Label((IPTR) "\33cMapping of USB keycodes\nto Amiga rawcodes"),
                                                Child, (IPTR)HSpace(0),
                                                End,
                                            Child, (IPTR)VSpace(0),
                                            End,
                                        Child, (IPTR)VGroup, GroupFrameT("Raw key"), /* HUA_RAWKEY */
                                            //Child, (IPTR)VSpace(0),
                                            Child, (IPTR)Label((IPTR) "\33cSelect key to send"),
                                            Child, (IPTR)(nhb->nhb_A_RawKeyObj = ListviewObject,
                                                MUIA_Listview_Input, TRUE,
                                                MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
                                                MUIA_Listview_List, (IPTR)ListObject,
                                                    InputListFrame,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_List_SourceArray, (IPTR)nhb->nhb_RawKeyArray,
                                                    MUIA_List_AutoVisible, TRUE,
                                                    End,
                                                End),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)(nhb->nhb_A_RawKeyUpObj = ImageObject, ImageButtonFrame,
                                                    MUIA_Background, MUII_ButtonBack,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                                    MUIA_Image_Spec, MUII_CheckMark,
                                                    MUIA_Image_FreeVert, TRUE,
                                                    //MUIA_Selected,
                                                    MUIA_ShowSelState, FALSE,
                                                    End),
                                                Child, (IPTR)Label((IPTR) "Send keyup event instead of keydown"),
                                                Child, (IPTR)HSpace(0),
                                                End,
                                            //Child, (IPTR)VSpace(0),
                                            End,
                                        Child, (IPTR)VGroup, GroupFrameT("Vanilla key"), /* HUA_VANILLA */
                                            Child, (IPTR)VSpace(0),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)Label((IPTR) "Key to send:"),
                                                Child, (IPTR)(nhb->nhb_A_VanillaStrObj = StringObject,
                                                    StringFrame,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_String_AdvanceOnCR, TRUE,
                                                    MUIA_String_MaxLen, 80,
                                                    End),
                                                End,
                                            Child, (IPTR)VSpace(0),
                                            End,
                                        Child, (IPTR)VGroup, GroupFrameT("Keystring"), /* HUA_KEYSTRING */
                                            Child, (IPTR)VSpace(0),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)Label((IPTR) "String to send:"),
                                                Child, (IPTR)(nhb->nhb_A_KeyStringObj = StringObject,
                                                    StringFrame,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_String_AdvanceOnCR, TRUE,
                                                    MUIA_String_MaxLen, 80,
                                                    End),
                                                End,
                                            Child, (IPTR)VSpace(0),
                                            End,
                                        Child, (IPTR)VGroup, GroupFrameT("Mouse pointer position"), /* HUA_MOUSEPOS */
                                            Child, (IPTR)VSpace(0),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)HSpace(0),
                                                Child, (IPTR)Label((IPTR) "Move mouse"),
                                                Child, (IPTR)(nhb->nhb_A_MousePosOpObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_MousePosOpStrings,
                                                    End),
                                                Child, (IPTR)HSpace(0),
                                                End,
                                            Child, (IPTR)VSpace(0),
                                            End,
                                        Child, (IPTR)VGroup, GroupFrameT("Mouse buttons"), /* HUA_BUTTONS */
                                            Child, (IPTR)VSpace(0),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)HSpace(0),
                                                Child, (IPTR)(nhb->nhb_A_MouseButOpObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_MouseButOpStrings,
                                                    End),
                                                Child, (IPTR)(nhb->nhb_A_MouseButObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_MouseButStrings,
                                                    End),
                                                Child, (IPTR)Label((IPTR) "mouse button"),
                                                Child, (IPTR)HSpace(0),
                                                End,
                                            Child, (IPTR)VSpace(0),
                                            End,
                                        Child, (IPTR)VGroup, GroupFrameT("Tablet information"), /* HUA_TABLET */
                                            Child, (IPTR)VSpace(0),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)HSpace(0),
                                                Child, (IPTR)Label((IPTR) "Send"),
                                                Child, (IPTR)(nhb->nhb_A_TabletAxisObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_TabletAxisStrings,
                                                    End),
                                                Child, (IPTR)Label((IPTR) "data"),
                                                Child, (IPTR)HSpace(0),
                                                End,
                                            Child, (IPTR)VSpace(0),
                                            End,
                                        Child, (IPTR)VGroup, GroupFrameT("Digital joypad (lowlevel.library)"), /* HUA_DIGJOY */
                                            Child, (IPTR)VSpace(0),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)HSpace(0),
                                                Child, (IPTR)(nhb->nhb_A_JoypadOpObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_JoypadOpStrings,
                                                    End),
                                                Child, (IPTR)(nhb->nhb_A_JoypadFeatObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_JoypadFeatStrings,
                                                    End),
                                                Child, (IPTR)Label((IPTR) "on"),
                                                Child, (IPTR)(nhb->nhb_A_JoypadPortObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_JoypadPortStrings,
                                                    End),
                                                Child, (IPTR)HSpace(0),
                                                End,
                                            Child, (IPTR)VSpace(0),
                                            End,
                                        Child, (IPTR)VGroup, GroupFrameT("Analogue joysticks (lowlevel hack)"), /* HUA_ANALOGJOY */
                                            Child, (IPTR)VSpace(0),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)HSpace(0),
                                                Child, (IPTR)Label((IPTR) "Set"),
                                                Child, (IPTR)(nhb->nhb_A_APadFeatObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_APadFeatStrings,
                                                    End),
                                                Child, (IPTR)Label((IPTR) "of"),
                                                Child, (IPTR)(nhb->nhb_A_APadPortObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_JoypadPortStrings,
                                                    End),
                                                Child, (IPTR)HSpace(0),
                                                End,
                                            Child, (IPTR)VSpace(0),
                                            End,
                                        Child, (IPTR)VGroup, GroupFrameT("Scrollwheel information"), /* HUA_WHEEL */
                                            Child, (IPTR)VSpace(0),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)HSpace(0),
                                                Child, (IPTR)Label((IPTR) "Send"),
                                                Child, (IPTR)(nhb->nhb_A_WheelOpObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_WheelOpStrings,
                                                    End),
                                                Child, (IPTR)Label((IPTR) "event"),
                                                Child, (IPTR)HSpace(0),
                                                End,
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)Label((IPTR) "Distance:"),
                                                Child, (IPTR)(nhb->nhb_A_WheelDistObj = SliderObject, SliderFrame,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Numeric_Min, 1,
                                                    MUIA_Numeric_Max, 32,
                                                    End),
                                                End,
                                            Child, (IPTR)VSpace(0),
                                            End,
                                        Child, (IPTR)VGroup, GroupFrameT("Sound playback"), /* HUA_SOUND */
                                            Child, (IPTR)VSpace(0),
                                            Child, (IPTR)ColGroup(2),
                                                Child, (IPTR)Label((IPTR) "Soundfile:"),
                                                Child, (IPTR)PopaslObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Popstring_String, (IPTR)(nhb->nhb_A_SoundFileObj = StringObject,
                                                        StringFrame,
                                                        MUIA_CycleChain, 1,
                                                        MUIA_String_AdvanceOnCR, TRUE,
                                                        End),
                                                    MUIA_Popstring_Button, (IPTR)PopButton(MUII_PopFile),
                                                    ASLFR_TitleText, (IPTR)"Select sound file...",
                                                    End,
                                                Child, (IPTR)Label((IPTR) "Volume:"),
                                                Child, (IPTR)(nhb->nhb_A_SoundVolObj = SliderObject, SliderFrame,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Numeric_Min, 0,
                                                    MUIA_Numeric_Max, 64,
                                                    End),
                                                End,
                                            Child, (IPTR)VSpace(0),
                                            End,
                                        Child, (IPTR)VGroup, GroupFrameT("Shell command execution"), /* HUA_SHELL */
                                            Child, (IPTR)VSpace(0),
                                            Child, (IPTR)ColGroup(2),
                                                Child, (IPTR)Label((IPTR) "Command:"),
                                                Child, (IPTR)PopaslObject,
                                                    MUIA_Popstring_String, (IPTR)(nhb->nhb_A_ShellComObj = StringObject,
                                                        StringFrame,
                                                        MUIA_CycleChain, 1,
                                                        MUIA_String_AdvanceOnCR, TRUE,
                                                        End),
                                                    MUIA_Popstring_Button, (IPTR)PopButton(MUII_PopFile),
                                                    ASLFR_TitleText, (IPTR) "Select an executable...",
                                                    End,
                                                Child, (IPTR)Label((IPTR) "ASync:"),
                                                Child, (IPTR)HGroup,
                                                    Child, (IPTR)(nhb->nhb_A_ShellAsyncObj = ImageObject, ImageButtonFrame,
                                                        MUIA_Background, MUII_ButtonBack,
                                                        MUIA_CycleChain, 1,
                                                        MUIA_InputMode, MUIV_InputMode_Toggle,
                                                        MUIA_Image_Spec, MUII_CheckMark,
                                                        MUIA_Image_FreeVert, TRUE,
                                                        MUIA_Selected, nhb->nhb_CDC->cdc_EnableRH,
                                                        MUIA_ShowSelState, FALSE,
                                                        End),
                                                    Child, (IPTR)HSpace(0),
                                                    End,
                                                End,
                                            Child, (IPTR)VSpace(0),
                                            End,
                                        Child, (IPTR)VGroup, GroupFrameT("Arexx command execution"), /* HUA_AREXX */
                                            Child, (IPTR)VSpace(0),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)HSpace(0),
                                                Child, (IPTR)Label((IPTR) "Sorry, not yet implemented."),
                                                Child, (IPTR)HSpace(0),
                                                End,
                                            Child, (IPTR)VSpace(0),
                                            End,
                                        Child, (IPTR)VGroup, GroupFrameT("HID output variables"), /* HUA_OUTPUT */
                                            Child, (IPTR)(nhb->nhb_A_OutItemLVObj = ListviewObject,
                                                MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
                                                MUIA_Listview_List, (IPTR)ListObject,
                                                    InputListFrame,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_List_DisplayHook, (IPTR)&nhb->nhb_ItemListDisplayHook,
                                                    MUIA_List_Format, (IPTR)barbarbarbar,
                                                    MUIA_List_Title, TRUE,
                                                    MUIA_List_AutoVisible, TRUE,
                                                    End,
                                                End),
                                            //Child, (IPTR)VSpace(0),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)Label((IPTR) "Array values:"),
                                                Child, (IPTR)(nhb->nhb_A_OutArrayObj = StringObject,
                                                    StringFrame,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_String_AdvanceOnCR, TRUE,
                                                    MUIA_String_MaxLen, 256,
                                                    End),
                                                End,
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)Label((IPTR) "Operation:"),
                                                Child, (IPTR)(nhb->nhb_A_OutOpObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_OutOpStrings,
                                                    End),
                                                End,
                                            End,
                                        Child, (IPTR)VGroup, GroupFrameT("HID feature variables"), /* HUA_FEATURE */
                                            Child, (IPTR)(nhb->nhb_A_FeatItemLVObj = ListviewObject,
                                                MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
                                                MUIA_Listview_List, (IPTR)ListObject,
                                                    InputListFrame,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_List_DisplayHook, (IPTR)&nhb->nhb_ItemListDisplayHook,
                                                    MUIA_List_Format, (IPTR)barbarbarbar,
                                                    MUIA_List_Title, TRUE,
                                                    MUIA_List_AutoVisible, TRUE,
                                                    End,
                                                End),
                                            //Child, (IPTR)VSpace(0),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)Label((IPTR) "Array values:"),
                                                Child, (IPTR)(nhb->nhb_A_FeatArrayObj = StringObject,
                                                    StringFrame,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_String_AdvanceOnCR, TRUE,
                                                    MUIA_String_MaxLen, 256,
                                                    End),
                                                End,
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)Label((IPTR) "Operation:"),
                                                Child, (IPTR)(nhb->nhb_A_FeatOpObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_OutOpStrings,
                                                    End),
                                                End,
                                            End,
                                        Child, (IPTR)VGroup, GroupFrameT("Miscellaneous stuff"), /* HUA_MISC */
                                            Child, (IPTR)VSpace(0),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)HSpace(0),
                                                Child, (IPTR)Label((IPTR) "Event:"),
                                                Child, (IPTR)(nhb->nhb_A_MiscOpObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_MiscOpStrings,
                                                    End),
                                                Child, (IPTR)HSpace(0),
                                                End,
                                            Child, (IPTR)VSpace(0),
                                            End,
                                        Child, (IPTR)VGroup, GroupFrameT("Changing variables"), /* HUA_VARIABLES */
                                            Child, (IPTR)VSpace(0),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)(nhb->nhb_A_TarVarObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_TarVariableStrings,
                                                    End),
                                                Child, (IPTR)(nhb->nhb_A_TarVarOpObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_TarVarOpStrings,
                                                    End),
                                                End,
                                            Child, (IPTR)VSpace(0),
                                            End,
                                        Child, (IPTR)VGroup, GroupFrameT("Extended Raw key"), /* HUA_EXTRAWKEY */
                                            //Child, (IPTR)VSpace(0),
                                            Child, (IPTR)Label((IPTR) "\33cSelect key to send"),
                                            Child, (IPTR)(nhb->nhb_A_ExtRawKeyObj = ListviewObject,
                                                MUIA_Listview_Input, TRUE,
                                                MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
                                                MUIA_Listview_List, (IPTR)ListObject,
                                                    InputListFrame,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_List_SourceArray, (IPTR)nhb->nhb_ExtRawKeyArray,
                                                    MUIA_List_AutoVisible, TRUE,
                                                    End,
                                                End),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)(nhb->nhb_A_ExtRawKeyUpObj = ImageObject, ImageButtonFrame,
                                                    MUIA_Background, MUII_ButtonBack,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                                    MUIA_Image_Spec, MUII_CheckMark,
                                                    MUIA_Image_FreeVert, TRUE,
                                                    MUIA_ShowSelState, FALSE,
                                                    End),
                                                Child, (IPTR)Label((IPTR) "Send keyup event instead of keydown"),
                                                Child, (IPTR)HSpace(0),
                                                End,
                                            //Child, (IPTR)VSpace(0),
                                            End,
                                        End),
                                    End),
                                End,
                            End,
                        //Child, (IPTR)VSpace(0),
                        End,
                    End,
                //Child, (IPTR)VSpace(0),
                Child, (IPTR)HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    Child, (IPTR)(nhb->nhb_UseObj = TextObject, ButtonFrame,
                        MUIA_ShowMe, (IPTR)nhb->nhb_Service,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, (IPTR)"\33c Save ",
                        End),
                    Child, (IPTR)(nhb->nhb_SetDefaultObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, (IPTR)(nhb->nhb_Service ? "\33c Save as Default " : "\33c Save Defaults "),
                        End),
                    Child, (IPTR)(nhb->nhb_CloseObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, (IPTR)"\33c Use ",
                        End),
                    End,
                End,
            End),
        End;

    if(!nhb->nhb_App)
    {
        KPRINTF(10, ("Couldn't create application\n"));
        GM_UNIQUENAME(bGUITaskCleanup)(nhb);
        return;
    }

    {
        struct ActionData *ad = INST_DATA(nhb->nhb_ActionClass->mcc_Class, nhb->nhb_ActionObj);
        ad->ad_NCH = nhb;
    }

    nhb->nhb_GUICurrentColl = NULL;
    nhb->nhb_GUICurrentItem = NULL;
    nhb->nhb_QuitGUI = FALSE;

    DoMethod(nhb->nhb_MainWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             nhb->nhb_ActionObj, 1, MUIM_Action_UseConfig);
    DoMethod(nhb->nhb_UseObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nhb->nhb_ActionObj, 1, MUIM_Action_StoreConfig);
    DoMethod(nhb->nhb_SetDefaultObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nhb->nhb_ActionObj, 1, MUIM_Action_DefaultConfig);
    DoMethod(nhb->nhb_CloseObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nhb->nhb_ActionObj, 1, MUIM_Action_UseConfig);

    DoMethod(nhb->nhb_AboutMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_About);
    DoMethod(nhb->nhb_UseMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_StoreConfig);
    DoMethod(nhb->nhb_SetDefaultMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_DefaultConfig);
    DoMethod(nhb->nhb_MUIPrefsMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nhb->nhb_App, 2, MUIM_Application_OpenConfigWindow, 0);
    DoMethod(nhb->nhb_SwapLMBRMBMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_SwapLMBRMB);
    DoMethod(nhb->nhb_MouseAccel100MI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nhb->nhb_ActionObj, 2, MUIM_Action_SetMouseAccel, 100);
    DoMethod(nhb->nhb_MouseAccel150MI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nhb->nhb_ActionObj, 2, MUIM_Action_SetMouseAccel, 150);
    DoMethod(nhb->nhb_MouseAccel200MI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nhb->nhb_ActionObj, 2, MUIM_Action_SetMouseAccel, 200);
    DoMethod(nhb->nhb_JoyPort0MI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nhb->nhb_ActionObj, 2, MUIM_Action_SetJoyPort, 0);
    DoMethod(nhb->nhb_JoyPort1MI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nhb->nhb_ActionObj, 2, MUIM_Action_SetJoyPort, 1);
    DoMethod(nhb->nhb_JoyPort2MI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nhb->nhb_ActionObj, 2, MUIM_Action_SetJoyPort, 2);
    DoMethod(nhb->nhb_JoyPort3MI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nhb->nhb_ActionObj, 2, MUIM_Action_SetJoyPort, 3);
    DoMethod(nhb->nhb_JoyAutofireMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_AddAutofire);

    DoMethod(nhb->nhb_DebugReportMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_DebugReport);

    DoMethod(nhb->nhb_EnableKBResetObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateDevPrefs);
    DoMethod(nhb->nhb_EnableRHObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateDevPrefs);
    DoMethod(nhb->nhb_ResetDelayObj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateDevPrefs);
    DoMethod(nhb->nhb_ShellStackObj, MUIM_Notify, MUIA_String_Integer, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateDevPrefs);
    DoMethod(nhb->nhb_TurboMouseObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateDevPrefs);

    DoMethod(nhb->nhb_HIDCtrlAutoObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateDevPrefs);
    DoMethod(nhb->nhb_HIDCtrlOpenObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nhb->nhb_ActionObj, 1, MUIM_Action_ShowHIDControl);

    for(count = 0; count < 4; count++)
    {
        DoMethod(nhb->nhb_LLPortModeObj[count], MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
                 nhb->nhb_ActionObj, 1, MUIM_Action_UpdateDevPrefs);
    }
    DoMethod(nhb->nhb_LLRumblePortObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateDevPrefs);

    DoMethod(nhb->nhb_USBKeymapLVObj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_KeymapSelectUSB);
    DoMethod(nhb->nhb_RawKeymapLVObj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_KeymapSelectRaw);
    DoMethod(nhb->nhb_RestoreDefKeymapObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nhb->nhb_ActionObj, 1, MUIM_Action_RestDefKeymap);
    DoMethod(nhb->nhb_TrackKeyEventsObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_SetTracking);

    DoMethod(nhb->nhb_ActionSelectorObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionPageObj, 3, MUIM_Set, MUIA_Group_ActivePage, MUIV_TriggerValue);

    DoMethod(nhb->nhb_ReportLVObj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_SelectReport);

    DoMethod(nhb->nhb_FillDefObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nhb->nhb_ActionObj, 1, MUIM_Action_FillDefReport);
    DoMethod(nhb->nhb_ClearActionsObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nhb->nhb_ActionObj, 1, MUIM_Action_ClearReport);

    DoMethod(nhb->nhb_TrackEventsObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_SetTracking);
    DoMethod(nhb->nhb_ReportValuesObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_SetTracking);
    DoMethod(nhb->nhb_DisableActionsObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_SetTracking);

    DoMethod(nhb->nhb_ItemLVObj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_SelectItem);

    DoMethod(nhb->nhb_ActionLVObj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_SelectAction);

    DoMethod(nhb->nhb_ActionNewObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nhb->nhb_ActionObj, 1, MUIM_Action_NewAction);
    DoMethod(nhb->nhb_ActionCopyObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nhb->nhb_ActionObj, 1, MUIM_Action_CopyAction);
    DoMethod(nhb->nhb_ActionDelObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nhb->nhb_ActionObj, 1, MUIM_Action_DelAction);
    DoMethod(nhb->nhb_ActionUpObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nhb->nhb_ActionObj, 1, MUIM_Action_MoveActionUp);
    DoMethod(nhb->nhb_ActionDownObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nhb->nhb_ActionObj, 1, MUIM_Action_MoveActionDown);

    DoMethod(nhb->nhb_ActionClipEnableObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nhb->nhb_A_ClipGroupObj, 3, MUIM_Set, MUIA_ShowMe, MUIV_TriggerValue);
    DoMethod(nhb->nhb_ActionScaleEnableObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nhb->nhb_A_ScaleGroupObj, 3, MUIM_Set, MUIA_ShowMe, MUIV_TriggerValue);
    DoMethod(nhb->nhb_ActionCCEnableObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nhb->nhb_A_CCGroupObj, 3, MUIM_Set, MUIA_ShowMe, MUIV_TriggerValue);
    DoMethod(nhb->nhb_ActionValEnableObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nhb->nhb_A_ValGroupObj, 3, MUIM_Set, MUIA_ShowMe, MUIV_TriggerValue);

    DoMethod(nhb->nhb_ActionSelectorObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_SetActionType);
    DoMethod(nhb->nhb_ActionTriggerObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);

    DoMethod(nhb->nhb_ActionAbsToRelObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nhb->nhb_ActionClipEnableObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nhb->nhb_ActionScaleEnableObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nhb->nhb_ActionCCEnableObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nhb->nhb_ActionValEnableObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAOptions);

    DoMethod(nhb->nhb_A_ClipMinObj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nhb->nhb_A_ClipMaxObj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nhb->nhb_A_ClipStretchObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAOptions);

    DoMethod(nhb->nhb_A_ScaleMinObj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nhb->nhb_A_ScaleMaxObj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAOptions);

    DoMethod(nhb->nhb_A_CCVar1Obj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nhb->nhb_A_CCCondObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nhb->nhb_A_CCVar2Obj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nhb->nhb_A_CCConst1Obj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nhb->nhb_A_CCConst2Obj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAOptions);

    DoMethod(nhb->nhb_A_ValVarObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nhb->nhb_A_ValConstObj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAOptions);

    DoMethod(nhb->nhb_A_KeyQualObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_KeyQualOpObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_RawKeyObj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_RawKeyUpObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_VanillaStrObj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_KeyStringObj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_MousePosOpObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_MouseButOpObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_MouseButObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_TabletAxisObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_WheelOpObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_WheelDistObj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_JoypadOpObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_JoypadFeatObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_JoypadPortObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_APadFeatObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_APadPortObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_SoundFileObj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_SoundVolObj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_ShellComObj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_ShellAsyncObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_MiscOpObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_TarVarObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_TarVarOpObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_OutOpObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_OutArrayObj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_OutItemLVObj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_FeatOpObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_FeatArrayObj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_FeatItemLVObj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_ExtRawKeyObj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nhb->nhb_A_ExtRawKeyUpObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nhb->nhb_ActionObj, 1, MUIM_Action_UpdateAction);

    if(nhb->nhb_Service)
    {
        struct BtHidCollection *nhc;
        struct BtHidReport *nhr = (struct BtHidReport *) nhb->nhb_HidReports.lh_Head;
        struct BtHidItem *nhi;
        struct BtHidGItem *nhgi;
        Object *outobj;

        nhb->nhb_GUICurrentColl = NULL;
        while(nhr->nhr_Node.ln_Succ)
        {
            nhc = (struct BtHidCollection *) nhr->nhr_Collections.lh_Head;
            while(nhc->nhc_Node.ln_Succ)
            {
                if(nhc->nhc_Items.lh_Head->ln_Succ)
                {
                    DoMethod(nhb->nhb_ReportLVObj, MUIM_List_InsertSingle, nhc, MUIV_List_Insert_Bottom);
                    if(!nhb->nhb_GUICurrentColl)
                    {
                        nhb->nhb_GUICurrentColl = nhc;
                        set(nhb->nhb_ReportLVObj, MUIA_List_Active, MUIV_List_Active_Top);
                    }
                }
                nhi = (struct BtHidItem *) nhc->nhc_Items.lh_Head;
                while(nhi->nhi_Node.ln_Succ)
                {
                    if(nhi->nhi_Type == REPORT_MAIN_OUTPUT)
                    {
                        outobj = nhb->nhb_A_OutItemLVObj;
                    }
                    else if(nhi->nhi_Type == REPORT_MAIN_FEATURE)
                    {
                        outobj = nhb->nhb_A_FeatItemLVObj;
                    } else {
                        outobj = NULL;
                    }
                    if(outobj)
                    {
                        if(nhi->nhi_Flags & RPF_MAIN_VARIABLE)
                        {
                            if((nhgi = bAllocGOutItem(nhb, nhi, &nhi->nhi_ActionList, nhi->nhi_Usage)))
                            {
                                DoMethod(outobj, MUIM_List_InsertSingle, nhgi, MUIV_List_Insert_Bottom);
                            }
                        } else {
                            STRPTR newstr;
                            //ULONG acount;
                            //if(nhi->nhi_SameUsages)
                            {
                                if((nhgi = bAllocGOutItem(nhb, nhi, &nhi->nhi_ActionList, nhi->nhi_Usage)))
                                {
                                    if((newstr = btCopyStrFmt("%s Array [%ld]", nhgi->nhgi_Name, nhi->nhi_Count)))
                                    {
                                        btFreeVec(nhgi->nhgi_Name);
                                        nhgi->nhgi_Name = newstr;
                                    }
                                    DoMethod(outobj, MUIM_List_InsertSingle, nhgi, MUIV_List_Insert_Bottom);
                                }
                            }
#if 0
                            else {
                                acount = 0;
                                do
                                {
                                    if((nhgi = bAllocGOutItem(nhb, nhi, &nhi->nhi_ActionMap[acount], nhi->nhi_UsageMap[acount])))
                                    {
                                        if((newstr = btCopyStrFmt(" +%s", nhgi->nhgi_Name)))
                                        {
                                            btFreeVec(nhgi->nhgi_Name);
                                            nhgi->nhgi_Name = newstr;
                                        }

                                        DoMethod(outobj, MUIM_List_InsertSingle, nhgi, MUIV_List_Insert_Bottom);
                                    }
                                } while(++acount < (nhi->nhi_LogicalMax-nhi->nhi_LogicalMin+1));
                            }
#endif
                        }
                    }
                    nhi = (struct BtHidItem *) nhi->nhi_Node.ln_Succ;
                }
                nhc = (struct BtHidCollection *) nhc->nhc_Node.ln_Succ;
            }
            nhr = (struct BtHidReport *) nhr->nhr_Node.ln_Succ;
        }
    }

    {
        IPTR  isopen = 0;
        IPTR  iconify = 0;
        ULONG sigs;
        ULONG sigmask;
        LONG retid;

        get(nhb->nhb_App, MUIA_Application_Iconified, &iconify);
        set(nhb->nhb_MainWindow, MUIA_Window_Open, TRUE);
        get(nhb->nhb_MainWindow, MUIA_Window_Open, &isopen);
        if(!(isopen || iconify))
        {
            GM_UNIQUENAME(bGUITaskCleanup)(nhb);
            return;
        }
        nhb->nhb_TrackingSignal = AllocSignal(-1);
        sigmask = (1<<nhb->nhb_TrackingSignal);
        do
        {
            retid = DoMethod(nhb->nhb_App, MUIM_Application_NewInput, &sigs);
            if(sigs)
            {
                sigs = Wait(sigs | sigmask | SIGBREAKF_CTRL_C);
                if(nhb->nhb_TrackEvents && (sigs & sigmask))
                {
                    BOOL track;
                    ULONG count;
                    struct BtHidItem *titem;
                    struct List *talist;
                    struct BtHidCollection *nhc;
                    struct BtHidGItem *nhgi;

                    talist = nhb->nhb_LastItemAList;
                    if((titem = nhb->nhb_LastItem))
                    {
                        track = TRUE;
                        if(nhb->nhb_GUICurrentItem)
                        {
                            if(nhb->nhb_GUICurrentItem->nhgi_ActionList == talist)
                            {
                                track = FALSE;
                            }
                        }
                        if(track)
                        {
                            if(nhb->nhb_GUICurrentColl == titem->nhi_Collection)
                            {
                                /* Already the right collection, find item */
                                count = 0;
                                nhgi = NULL;
                                do
                                {
                                    DoMethod(nhb->nhb_ItemLVObj, MUIM_List_GetEntry, count, &nhgi);
                                    if(!nhgi)
                                    {
                                        break;
                                    }
                                    if(nhgi->nhgi_ActionList == talist)
                                    {
                                        /* Heureka! */
                                        set(nhb->nhb_ItemLVObj, MUIA_List_Active, count);
                                        DoMethod(nhb->nhb_ItemLVObj, MUIM_List_Jump, count);
                                        nhb->nhb_LastItemAList = NULL;
                                        nhb->nhb_LastItem = NULL;
                                        break;
                                    }
                                    count++;
                                } while(TRUE);
                            } else {
                                /* Find collection */
                                count = 0;
                                nhc = NULL;
                                do
                                {
                                    DoMethod(nhb->nhb_ReportLVObj, MUIM_List_GetEntry, count, &nhc);
                                    if(!nhc)
                                    {
                                        break;
                                    }
                                    if(nhc == titem->nhi_Collection)
                                    {
                                        /* Heureka! */
                                        set(nhb->nhb_ReportLVObj, MUIA_List_Active, count);
                                        DoMethod(nhb->nhb_ReportLVObj, MUIM_List_Jump, count);
                                        break;
                                    }
                                    count++;
                                } while(TRUE);
                            }
                        }
                    }
                }
                if(nhb->nhb_TrackKeyEvents && (sigs & sigmask))
                {
                    if((nhb->nhb_CurrUSBKey != nhb->nhb_LastUSBKey) && nhb->nhb_LastUSBKey)
                    {
                        hum = (struct HidUsageIDMap *) hidusage07;
                        count = 0;
                        while(hum->hum_String)
                        {
                            if(hum->hum_ID == (nhb->nhb_LastUSBKey & 0xffff))
                            {
                                set(nhb->nhb_USBKeymapLVObj, MUIA_List_Active, count);
                                break;
                            }
                            hum++;
                            count++;
                        }
                        nhb->nhb_CurrUSBKey = nhb->nhb_LastUSBKey;
                    }
                }

                if(nhb->nhb_ReportValues && (sigs & sigmask) && nhb->nhb_ItemChanged)
                {
                    DoMethod(nhb->nhb_ItemLVObj, MUIM_List_Redraw, MUIV_List_Redraw_Active);
                    nhb->nhb_ItemChanged = FALSE;
                }

                if(sigs & SIGBREAKF_CTRL_C)
                {
                    break;
                }
            }
        } while((!nhb->nhb_QuitGUI) && (retid != MUIV_Application_ReturnID_Quit));
        FreeSignal(nhb->nhb_TrackingSignal);
        nhb->nhb_TrackingSignal = -1;
        set(nhb->nhb_MainWindow, MUIA_Window_Open, FALSE);
    }
    GM_UNIQUENAME(bGUITaskCleanup)(nhb);

    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "bGUITaskCleanup()" */
void GM_UNIQUENAME(bGUITaskCleanup)(struct BTHidBinding *nhb)
{
    UWORD count;
    struct BtHidGItem *nhgi;

    if(nhb->nhb_App)
    {
        MUI_DisposeObject(nhb->nhb_App);
        nhb->nhb_App = NULL;
    }

    for(count = 0; count < 128; count++)
    {
        btFreeVec(nhb->nhb_RawKeyArray[count]);
        nhb->nhb_RawKeyArray[count] = NULL;
        btFreeVec(nhb->nhb_ExtRawKeyArray[count]);
        nhb->nhb_ExtRawKeyArray[count] = NULL;
    }
    nhgi = (struct BtHidGItem *) nhb->nhb_GUIItems.lh_Head;
    while(nhgi->nhgi_Node.ln_Succ)
    {
        bFreeGItem(nhb, nhgi);
        nhgi = (struct BtHidGItem *) nhb->nhb_GUIItems.lh_Head;
    }
    nhgi = (struct BtHidGItem *) nhb->nhb_GUIOutItems.lh_Head;
    while(nhgi->nhgi_Node.ln_Succ)
    {
        bFreeGItem(nhb, nhgi);
        nhgi = (struct BtHidGItem *) nhb->nhb_GUIOutItems.lh_Head;
    }
    if(nhb->nhb_ActionClass)
    {
        MUI_DeleteCustomClass(nhb->nhb_ActionClass);
        nhb->nhb_ActionClass = NULL;
    }

    if(MUIMasterBase)
    {
        CloseLibrary(MUIMasterBase);
        MUIMasterBase = NULL;
    }
    if(IntuitionBase)
    {
        CloseLibrary(IntuitionBase);
        IntuitionBase = NULL;
    }
    if(KeymapBase)
    {
        CloseLibrary(KeymapBase);
        KeymapBase = NULL;
    }
    if(BluetoothBase)
    {
        CloseLibrary(BluetoothBase);
        BluetoothBase = NULL;
    }
    Forbid();
    nhb->nhb_GUITask = NULL;
    if(nhb->nhb_ReadySigTask)
    {
        Signal(nhb->nhb_ReadySigTask, 1L<<nhb->nhb_ReadySignal);
    }
    --nhb->nhb_ClsBase->nh_Library.lib_OpenCnt;
}
/* \\\ */

/* /// "bAllocGItem()" */
struct BtHidGItem * bAllocGItem(struct BTHidBinding *nhb, struct BtHidItem *nhi, struct List *actionlist, ULONG usageid)
{
    struct BtHidGItem *nhgi;

    if(!(nhgi = btAllocVec(sizeof(struct BtHidGItem))))
    {
        return(NULL);
    }
    nhgi->nhgi_Item = nhi;
    nhgi->nhgi_ActionList = actionlist;
    if(usageid)
    {
        nhgi->nhgi_Name = bGetUsageName(nhb, usageid);
    }
    AddTail(&nhb->nhb_GUIItems, &nhgi->nhgi_Node);
    return(nhgi);
}
/* \\\ */

/* /// "bAllocGOutItem()" */
struct BtHidGItem * bAllocGOutItem(struct BTHidBinding *nhb, struct BtHidItem *nhi, struct List *actionlist, ULONG usageid)
{
    struct BtHidGItem *nhgi;

    if(!(nhgi = btAllocVec(sizeof(struct BtHidGItem))))
    {
        return(NULL);
    }
    nhgi->nhgi_Item = nhi;
    nhgi->nhgi_ActionList = actionlist;
    if(usageid)
    {
        nhgi->nhgi_Name = bGetUsageName(nhb, usageid);
    }
    AddTail(&nhb->nhb_GUIOutItems, &nhgi->nhgi_Node);
    return(nhgi);
}
/* \\\ */

/* /// "bFreeGItem()" */
void bFreeGItem(struct BTHidBinding *nhb, struct BtHidGItem *nhgi)
{
    if(nhgi)
    {
        Remove(&nhgi->nhgi_Node);
        btFreeVec(nhgi->nhgi_Name);
        btFreeVec(nhgi);
    }
}
/* \\\ */

/* /// "USBKeyListDisplayHook()" */
AROS_UFH3(LONG, GM_UNIQUENAME(USBKeyListDisplayHook),
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(char **, strarr, A2),
          AROS_UFHA(struct HidUsageIDMap *, hum, A1))
{
    AROS_USERFUNC_INIT

    if(hum)
    {
        *strarr = hum->hum_String;
    } else {
        *strarr = "\33l\33uHID Key name";
    }
    return(0);

    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "ReportListDisplayHook()" */
AROS_UFH3(LONG, GM_UNIQUENAME(ReportListDisplayHook),
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(char **, strarr, A2),
          AROS_UFHA(struct BtHidCollection *, nhc, A1))
{
    AROS_USERFUNC_INIT

    struct BTHidBinding *nhb = (struct BTHidBinding *) hook->h_Data;

    if(nhc)
    {
        if(!nhc->nhc_Parent)
        {
            *strarr = nhc->nhc_Name;
        } else {
            STRPTR srcptr;
            STRPTR tarptr;
            *strarr = tarptr = nhb->nhb_TmpStrBufReport;
            srcptr = nhc->nhc_Name;
            while((*tarptr++ = *srcptr++));
            while((nhc = nhc->nhc_Parent))
            {
                tarptr[-1] = ' ';
                *tarptr++ = '(';
                srcptr = nhc->nhc_Name;
                while((*tarptr++ = *srcptr++));
                tarptr[-1] = ')';
                *tarptr++ = 0;
            }
        }
    } else {
        *strarr = "\33l\33uName";
    }
    return(0);

    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "ItemListDisplayHook()" */
AROS_UFH3(LONG, GM_UNIQUENAME(ItemListDisplayHook),
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(char **, strarr, A2),
          AROS_UFHA(struct BtHidGItem *, nhgi, A1))
{
    AROS_USERFUNC_INIT

    struct BTHidBinding *nhb = (struct BTHidBinding *) hook->h_Data;
    struct BtHidItem *nhi;
    STRPTR buf;
    ULONG flags;
    if(nhgi)
    {
        nhi = nhgi->nhgi_Item;
        btSafeRawDoFmt(nhb->nhb_TmpStrBufItem, 10, "%ld", nhi->nhi_LogicalMin);
        btSafeRawDoFmt(&nhb->nhb_TmpStrBufItem[10], 10, "%ld", nhi->nhi_LogicalMax);
        buf = &nhb->nhb_TmpStrBufItem[30];
        flags = nhi->nhi_Flags;
        if(flags & RPF_MAIN_CONST)
        {
            strcpy(buf, "const ");
            buf += 6;
        }
        if(!(flags & RPF_MAIN_NONLINEAR))
        {
            strcpy(buf, "lin. ");
            buf += 5;
        }
        if(flags & RPF_MAIN_RELATIVE)
        {
            strcpy(buf, "rel. ");
            buf += 5;
        } else {
            strcpy(buf, "abs. ");
            buf += 5;
        }
        *strarr++ = nhgi->nhgi_Name;
        if(flags & RPF_MAIN_VARIABLE)
        {
            strcpy(buf, "var. ");
            buf += 5;
            btSafeRawDoFmt(&nhb->nhb_TmpStrBufItem[20], 10, "%ld", nhi->nhi_OldValue);
            *strarr++ = &nhb->nhb_TmpStrBufItem[20];
        } else {
            strcpy(buf, "array ");
            buf += 6;
            *strarr++ = "n/a";
        }
        if(flags & RPF_MAIN_CONST)
        {
            strcpy(buf, "const ");
            buf += 6;
        }
        if(flags & RPF_MAIN_WRAP)
        {
            strcpy(buf, "wrap ");
            buf += 5;
        }
        if(flags & RPF_MAIN_NOPREF)
        {
            strcpy(buf, "nopref. ");
            buf += 8;
        }
        if(flags & RPF_MAIN_NULLSTATE)
        {
            strcpy(buf, "null ");
            buf += 5;
        }
        if(flags & RPF_MAIN_VOLATILE)
        {
            strcpy(buf, "vola. ");
            buf += 6;
        }
        if(flags & RPF_MAIN_BUFBYTES)
        {
            strcpy(buf, "bufbytes");
        } else {
            strcpy(buf, "bitfield");
        }
        *strarr++ = nhb->nhb_TmpStrBufItem;
        *strarr++ = &nhb->nhb_TmpStrBufItem[10];
        *strarr = &nhb->nhb_TmpStrBufItem[30];
    } else {
        *strarr++ = "\33l\33uName";
        *strarr++ = "\33l\33uVal";
        *strarr++ = "\33l\33uMin";
        *strarr++ = "\33l\33uMax";
        *strarr = "\33l\33uType";
    }
    
    return(0);

    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "ActionListDisplayHook()" */
AROS_UFH3(LONG, GM_UNIQUENAME(ActionListDisplayHook),
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(char **, strarr, A2),
          AROS_UFHA(struct BtHidAction *, nha, A1))
{
    AROS_USERFUNC_INIT

    struct BTHidBinding *nhb = (struct BTHidBinding *) hook->h_Data;
    STRPTR p1str, p2str;

    if(nha)
    {
        *strarr++ = ActionTypeStrings[nha->nha_Type & HUA_ATYPEMASK];
        switch(nha->nha_Type & HUA_TRIGMASK)
        {
            case HUA_DOWNEVENT:
                *strarr++ = "Down";
                break;
            case HUA_UPEVENT:
                *strarr++ = "Up";
                break;
            case HUA_ANY:
                *strarr++ = "Any";
                break;
            case HUA_ALWAYS:
                *strarr++ = "Always";
                break;
            case HUA_NAN:
                *strarr++ = "NaN";
                break;
            default:
                *strarr++ = "???";
        }

        *strarr = "???";

        switch(nha->nha_Type & HUA_ATYPEMASK)
        {
            case HUA_NOP:
            case HUA_KEYMAP:
                *strarr = "None";
                break;

            case HUA_QUALIFIER:
                p1str = "???";
                switch(nha->nha_QualMode)
                {
                    case HUAT_SET:
                        p1str = "Set";
                        break;
                    case HUAT_CLEAR:
                        p1str = "Clear";
                        break;
                    case HUAT_TOGGLE:
                        p1str = "Toggle";
                        break;
                    case HUAT_ASSIGN:
                        p1str = "Assign";
                        break;
                }

                btSafeRawDoFmt(nhb->nhb_TmpStrBufAction, 80, "%s %s", p1str, A_QualifierStrings[nha->nha_Qualifier]);
                *strarr = nhb->nhb_TmpStrBufAction;
                break;

            case HUA_MOUSEPOS:
                switch(nha->nha_MouseAxis)
                {
                    case HUAT_DELTAX:
                        *strarr = "Delta X";
                        break;

                    case HUAT_DELTAY:
                        *strarr = "Delta Y";
                        break;

                    case HUAT_ABSX:
                        *strarr = "Absolute X";
                        break;

                    case HUAT_ABSY:
                        *strarr = "Absolute Y";
                        break;
                }
                break;

            case HUA_BUTTONS:
                p1str = "???";
                switch(nha->nha_ButtonMode)
                {
                    case HUAT_SET:
                        p1str = "Press";
                        break;
                    case HUAT_CLEAR:
                        p1str = "Release";
                        break;
                    case HUAT_TOGGLE:
                        p1str = "Flip";
                        break;
                    case HUAT_ASSIGN:
                        p1str = "Assign";
                        break;
                }
                p2str = "???";
                switch(nha->nha_ButtonNo)
                {
                    case 1:
                        p2str = "left mouse button";
                        break;

                    case 2:
                        p2str = "right mouse button";
                        break;

                    case 3:
                        p2str = "middle mouse button";
                        break;

                    case 4:
                        p2str = "fourth mouse button";
                        break;

                    case 5:
                        p2str = "fifth mouse button";
                        break;
                }
                btSafeRawDoFmt(*strarr = nhb->nhb_TmpStrBufAction, 80, "%s %s", p1str, p2str);
                break;

            case HUA_WHEEL:
                switch(nha->nha_WheelMode)
                {
                    case HUAT_DELTAX:
                        *strarr = "Wheel horizontal";
                        break;

                    case HUAT_DELTAY:
                        *strarr = "Wheel vertical";
                        break;

                    case HUAT_LEFT:
                        *strarr = "Wheel left";
                        break;

                    case HUAT_RIGHT:
                        *strarr = "Wheel right";
                        break;

                    case HUAT_UP:
                        *strarr = "Wheel up";
                        break;

                    case HUAT_DOWN:
                        *strarr = "Wheel down";
                        break;
                }
                break;

            case HUA_DIGJOY:
                p1str = A_JoypadOpStrings[bRevLookup(nha->nha_JoypadOp, 0, A_JoypadOpVals)];
                p2str = A_JoypadFeatStrings[bRevLookup(nha->nha_JoypadFeat, 0, A_JoypadFeatVals)];
                btSafeRawDoFmt(*strarr = nhb->nhb_TmpStrBufAction, 80, "%s %s (port %ld)", p1str, p2str, (ULONG) nha->nha_JoypadPort);
                break;

            case HUA_ANALOGJOY:
                p1str = A_APadFeatStrings[bRevLookup(nha->nha_APadFeat, 0, A_APadFeatVals)];
                btSafeRawDoFmt(*strarr = nhb->nhb_TmpStrBufAction, 80, "Set %s (port %ld)", p1str, (ULONG) nha->nha_JoypadPort);
                break;

            case HUA_TABLET:
                switch(nha->nha_TabletAxis)
                {
                    case HUAT_PRESSURE:
                        *strarr = "Pressure";
                        break;

                    case HUAT_XROT:
                        *strarr = "X rotation";
                        break;

                    case HUAT_YROT:
                        *strarr = "Y rotation";
                        break;

                    case HUAT_ZROT:
                        *strarr = "Z rotation";
                        break;

                    case HUAT_PROX:
                        *strarr = "In proximity";
                        break;

                    case HUAT_ABSZ:
                        *strarr = "Z position";
                }
                break;

            case HUA_RAWKEY:
                btSafeRawDoFmt(*strarr = nhb->nhb_TmpStrBufAction, 80, "%s %s",
                                (nha->nha_RawKey & IECODE_UP_PREFIX) ? "keyup" : "keydown",
                                nhb->nhb_RawKeyArray[nha->nha_RawKey & (~IECODE_UP_PREFIX)]);
                break;

            case HUA_SOUND:
                *strarr = p1str = nha->nha_SoundFile;
                while(*p1str)
                {
                    if(*p1str++ == '/')
                    {
                        *strarr = p1str;
                    }
                }
                break;

            case HUA_VANILLA:
                *strarr = nha->nha_VanillaString;
                break;

            case HUA_KEYSTRING:
                *strarr = nha->nha_KeyString;
                break;

            case HUA_SHELL:
                *strarr = nha->nha_ExeString;
                break;

            case HUA_OUTPUT:
            {
                struct BtHidItem *nhi;
                ULONG dummy;
                BOOL freeit = FALSE;

                p1str = A_OutOpStrings[bRevLookup(nha->nha_OutOp, 0, A_OutOpVals)];
                p2str = "???";
                if((nhi = bFindItemID(nhb, nha->nha_OutItem, REPORT_MAIN_OUTPUT, &dummy)))
                {
                    p2str = bGetUsageName(nhb, nhi->nhi_Usage);
                    freeit =  TRUE;
                    if(!(nhi->nhi_Flags & RPF_MAIN_VARIABLE))
                    {
                        p1str = nha->nha_OutArray;
                    }
                }
                btSafeRawDoFmt(*strarr = nhb->nhb_TmpStrBufAction, 80, "%s %s", p1str, p2str);
                if(freeit)
                {
                    btFreeVec(p2str);
                }
                break;
            }

            case HUA_FEATURE:
            {
                struct BtHidItem *nhi;
                ULONG dummy;
                BOOL freeit = FALSE;

                p1str = A_OutOpStrings[bRevLookup(nha->nha_FeatOp, 0, A_OutOpVals)];
                p2str = "???";
                if((nhi = bFindItemID(nhb, nha->nha_FeatItem, REPORT_MAIN_FEATURE, &dummy)))
                {
                    p2str = bGetUsageName(nhb, nhi->nhi_Usage);
                    freeit =  TRUE;
                    if(!(nhi->nhi_Flags & RPF_MAIN_VARIABLE))
                    {
                        p1str = nha->nha_OutArray;
                    }
                }
                btSafeRawDoFmt(*strarr = nhb->nhb_TmpStrBufAction, 80, "%s %s", p1str, p2str);
                if(freeit)
                {
                    btFreeVec(p2str);
                }
                break;
            }

            case HUA_MISC:
                *strarr = A_MiscOpStrings[bRevLookup(nha->nha_MiscMode, 0, A_MiscOpVals)];
                break;

            case HUA_VARIABLES:
                p1str = A_TarVariableStrings[bRevLookup(nha->nha_TarVar, 0, A_TarVariableVals)];
                p2str = A_TarVarOpStrings[bRevLookup(nha->nha_TarVarOp, 0, A_TarVarOpVals)];
                btSafeRawDoFmt(*strarr = nhb->nhb_TmpStrBufAction, 80, "%s %s", p1str, p2str);
                break;

            case HUA_EXTRAWKEY:
                btSafeRawDoFmt(*strarr = nhb->nhb_TmpStrBufAction, 80, "%s %s",
                                (nha->nha_RawKey & IECODE_UP_PREFIX) ? "keyup" : "keydown",
                                nhb->nhb_ExtRawKeyArray[nha->nha_RawKey & (~IECODE_UP_PREFIX)]);
                break;

        }
    } else {
        *strarr++ = "\33l\33uType";
        *strarr++ = "\33l\33uTrigger";
        *strarr = "\33l\33uParams";
    }
    return(0);

    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "ActionDispatcher()" */
AROS_UFH3(IPTR, GM_UNIQUENAME(ActionDispatcher),
          AROS_UFHA(struct IClass *, cl, A0),
          AROS_UFHA(Object *, obj, A2),
          AROS_UFHA(Msg, msg, A1))
{
    AROS_USERFUNC_INIT

    struct ActionData *ad = (struct ActionData *) 0xABADCAFE;
    struct BTHidBinding *nhb = NULL;
    struct HidUsageIDMap *hum;
    ULONG tmpval;
    STRPTR tmpstr;
    ULONG count;

    if(msg->MethodID != OM_NEW)
    {
        ad = INST_DATA(cl, obj);
        nhb = ad->ad_NCH;
    }
    switch(msg->MethodID)
    {
        case OM_NEW:
            if(!(obj = (Object *) DoSuperMethodA(cl,obj,msg)))
            {
                return(0);
            }
            return (IPTR)obj;

        case MUIM_Action_UseConfig:
        case MUIM_Action_DefaultConfig:
        case MUIM_Action_StoreConfig:
        {
            struct BtIFFContext *pic;
            struct BtIFFContext *rppic;
            struct BtHidReport *nhr;
            struct BtHidCollection *nhc;
            struct BtHidItem *nhi;
            struct List *alistptr;
            ULONG *usageptr;
            ULONG count;
            ULONG newform[3];

            DoMethod(nhb->nhb_ActionObj, MUIM_Action_UpdateDevPrefs);
            DoMethod(nhb->nhb_ActionObj, MUIM_Action_UpdateAction);
            DoMethod(nhb->nhb_ActionObj, MUIM_Action_UpdateAOptions);

            if(msg->MethodID == MUIM_Action_DefaultConfig)
            {
                pic = btGetClsCfg(GM_UNIQUENAME(libname));
                if(!pic)
                {
                    btSetClsCfg(GM_UNIQUENAME(libname), NULL);
                    pic = btGetClsCfg(GM_UNIQUENAME(libname));
                }
                if(pic)
                {
                    btAddCfgEntry(pic, nhb->nhb_CDC);
                    btAddCfgEntry(pic, &nhb->nhb_KeymapCfg);
                    btSaveCfgToDisk(NULL, FALSE);
                }
            }
            if(nhb->nhb_Service)
            {
                pic = btGetDevCfg(GM_UNIQUENAME(libname), nhb->nhb_DevIDString, nhb->nhb_SvcIDString);
                if(!pic)
                {
                    btSetDevCfg(GM_UNIQUENAME(libname), nhb->nhb_DevIDString, nhb->nhb_SvcIDString, NULL);
                    pic = btGetDevCfg(GM_UNIQUENAME(libname), nhb->nhb_DevIDString, nhb->nhb_SvcIDString);
                }
                if(pic)
                {
                    btAddCfgEntry(pic, nhb->nhb_CDC);
                    if(memcmp(&nhb->nhb_KeymapCfg, &nhb->nhb_ClsBase->nh_DefaultBinding.nhb_KeymapCfg, sizeof(struct KeymapCfg)))
                    {
                        btAddCfgEntry(pic, &nhb->nhb_KeymapCfg);
                    } else {
                        btRemCfgChunk(pic, AROS_LONG2BE(nhb->nhb_KeymapCfg.kmc_ChunkID));
                    }

                    /* Create config file */
                    newform[0] = AROS_LONG2BE(ID_FORM);
                    newform[1] = AROS_LONG2BE(4);
                    nhr = (struct BtHidReport *) nhb->nhb_HidReports.lh_Head;
                    while(nhr->nhr_Node.ln_Succ)
                    {
                        if(nhr->nhr_ReportID == 0xffff)
                        {
                            newform[2] = AROS_LONG2BE(MAKE_ID('X','R','P','T'));
                        }
                        else if(nhr->nhr_ReportID == 0xfffe)
                        {
                            newform[2] = AROS_LONG2BE(MAKE_ID('W','C','O','M'));
                        } else {
                            newform[2] = AROS_LONG2BE(MAKE_ID('R','P','T','0')+nhr->nhr_ReportID);
                        }

                        rppic = btFindCfgForm(pic, AROS_LONG2BE(newform[2]));
                        if(!rppic)
                        {
                            rppic = btAddCfgEntry(pic, newform);
                            if(!rppic)
                            {
                                break;
                            }
                        } else {
                            btRemCfgChunk(rppic, 0);
                        }

                        /* find out, if we can get rid of defaults before saving */
                        nhc = (struct BtHidCollection *) nhr->nhr_Collections.lh_Head;
                        while(nhc->nhc_Node.ln_Succ)
                        {
                            nhi = (struct BtHidItem *) nhc->nhc_Items.lh_Head;
                            while(nhi->nhi_Node.ln_Succ)
                            {
                                if(nhi->nhi_Flags & RPF_MAIN_VARIABLE)
                                {
                                    usageptr = &nhi->nhi_Usage;
                                    alistptr = &nhi->nhi_ActionList;
                                    count = 1;
                                } else {
                                    usageptr = nhi->nhi_UsageMap;
                                    alistptr = nhi->nhi_ActionMap;
                                    count = nhi->nhi_MapSize;
                                }
                                do
                                {
                                    bCheckForDefaultAction(nhb, nhi, alistptr++, nhc, *usageptr++);
                                } while(--count);
                                nhi = (struct BtHidItem *) nhi->nhi_Node.ln_Succ;
                            }
                            nhc = (struct BtHidCollection *) nhc->nhc_Node.ln_Succ;
                        }

                        nhc = (struct BtHidCollection *) nhr->nhr_Collections.lh_Head;
                        while(nhc->nhc_Node.ln_Succ)
                        {
                            nhi = (struct BtHidItem *) nhc->nhc_Items.lh_Head;
                            while(nhi->nhi_Node.ln_Succ)
                            {
                                bSaveItem(nhb, rppic, &nhi->nhi_ActionList, nhr->nhr_ItemIDBase);
                                if((alistptr = nhi->nhi_ActionMap))
                                {
                                    count = nhi->nhi_MapSize;
                                    do
                                    {
                                        bSaveItem(nhb, rppic, alistptr++, nhr->nhr_ItemIDBase);
                                    } while(--count);
                                }
                                nhi = (struct BtHidItem *) nhi->nhi_Node.ln_Succ;
                            }
                            nhc = (struct BtHidCollection *) nhc->nhc_Node.ln_Succ;
                        }
                        nhr = (struct BtHidReport *) nhr->nhr_Node.ln_Succ;
                    }
                    if(msg->MethodID != MUIM_Action_UseConfig)
                    {
                        btSaveCfgToDisk(NULL, FALSE);
                    }
                    nhb->nhb_QuitGUI = TRUE;
                }
            } else {
                nhb->nhb_QuitGUI = TRUE;
            }
            return(TRUE);
        }

        case MUIM_Action_About:
            MUI_RequestA(nhb->nhb_App, nhb->nhb_MainWindow, 0, NULL, "Blimey!", VERSION_STRING, NULL);
            return(TRUE);

        case MUIM_Action_UpdateDevPrefs:
        {
            CONST_STRPTR tmpstr;
            tmpstr = "";
            get(nhb->nhb_ConWindowObj, MUIA_String_Contents, &tmpstr);
            strncpy(nhb->nhb_CDC->cdc_ShellCon, tmpstr, 127);

            get(nhb->nhb_EnableKBResetObj, MUIA_Selected, &nhb->nhb_CDC->cdc_EnableKBReset);
            get(nhb->nhb_EnableRHObj, MUIA_Selected, &nhb->nhb_CDC->cdc_EnableRH);
            get(nhb->nhb_ResetDelayObj, MUIA_Numeric_Value, &nhb->nhb_CDC->cdc_ResetDelay);
            get(nhb->nhb_ShellStackObj, MUIA_String_Integer, &nhb->nhb_CDC->cdc_ShellStack);
            get(nhb->nhb_TurboMouseObj, MUIA_Cycle_Active, &nhb->nhb_CDC->cdc_TurboMouse);

            get(nhb->nhb_HIDCtrlAutoObj, MUIA_Selected, &nhb->nhb_CDC->cdc_HIDCtrlOpen);
            tmpstr = "";
            get(nhb->nhb_HIDCtrlRexxObj, MUIA_String_Contents, &tmpstr);
            strncpy(nhb->nhb_CDC->cdc_HIDCtrlRexx, tmpstr, 31);
            tmpstr = "";
            get(nhb->nhb_HIDCtrlTitleObj, MUIA_String_Contents, &tmpstr);
            strncpy(nhb->nhb_CDC->cdc_HIDCtrlTitle, tmpstr, 31);

            for(count = 0; count < 4; count++)
            {
                get(nhb->nhb_LLPortModeObj[count], MUIA_Cycle_Active, &nhb->nhb_CDC->cdc_LLPortMode[count]);
            }
            get(nhb->nhb_LLRumblePortObj, MUIA_Cycle_Active, &nhb->nhb_CDC->cdc_LLRumblePort);

            return(TRUE);
        }

        case MUIM_Action_SelectReport:
        {
            struct BtHidGItem *nhgi = NULL;
            struct BtHidItem *nhi = NULL;
            ULONG acount;
            STRPTR newstr;
            STRPTR idstr1, idstr2;
            ULONG pos;
            ULONG jumppos = 0;

            nhb->nhb_SilentActionUpdate = TRUE;
            DoMethod(nhb->nhb_ReportLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nhb->nhb_GUICurrentColl);
            set(nhb->nhb_ItemLVObj, MUIA_List_Quiet, TRUE);
            DoMethod(nhb->nhb_ItemLVObj, MUIM_List_Clear);
            nhgi = (struct BtHidGItem *) nhb->nhb_GUIItems.lh_Head;
            while(nhgi->nhgi_Node.ln_Succ)
            {
                bFreeGItem(nhb, nhgi);
                nhgi = (struct BtHidGItem *) nhb->nhb_GUIItems.lh_Head;
            }
            nhb->nhb_GUICurrentItem = NULL;
            if(nhb->nhb_GUICurrentColl)
            {
                pos = 0;
                nhi = (struct BtHidItem *) nhb->nhb_GUICurrentColl->nhc_Items.lh_Head;
                while(nhi->nhi_Node.ln_Succ)
                {
                    if(nhi->nhi_Type == REPORT_MAIN_INPUT)
                    {
                        if(nhi->nhi_Flags & RPF_MAIN_VARIABLE)
                        {
                            if((nhgi = bAllocGItem(nhb, nhi, &nhi->nhi_ActionList, nhi->nhi_Usage)))
                            {
                                DoMethod(nhb->nhb_ItemLVObj, MUIM_List_InsertSingle, nhgi, MUIV_List_Insert_Bottom);
                                if((!nhb->nhb_GUICurrentItem) ||
                                   (nhb->nhb_TrackEvents && (nhgi->nhgi_ActionList == nhb->nhb_LastItemAList)))
                                {
                                    nhb->nhb_GUICurrentItem = nhgi;
                                    jumppos = pos;
                                }
                                pos++;
                            }
                        } else {
                            if((nhgi = bAllocGItem(nhb, nhi, &nhi->nhi_ActionList, 0)))
                            {
                                idstr1 = bGetUsageName(nhb, nhi->nhi_UsageMap[0]);
                                if(nhi->nhi_SameUsages)
                                {
                                    nhgi->nhgi_Name = btCopyStrFmt("%s Array", idstr1);
                                } else {
                                    idstr2 = bGetUsageName(nhb, nhi->nhi_UsageMap[nhi->nhi_LogicalMax-nhi->nhi_LogicalMin]);
                                    nhgi->nhgi_Name = btCopyStrFmt("Default for %s->%s", idstr1, idstr2);
                                    btFreeVec(idstr2);
                                }
                                btFreeVec(idstr1);
                                DoMethod(nhb->nhb_ItemLVObj, MUIM_List_InsertSingle, nhgi, MUIV_List_Insert_Bottom);
                                if((!nhb->nhb_GUICurrentItem) ||
                                   (nhb->nhb_TrackEvents && (nhgi->nhgi_ActionList == nhb->nhb_LastItemAList)))
                                {
                                    nhb->nhb_GUICurrentItem = nhgi;
                                    jumppos = pos;
                                }
                                pos++;
                            }
                            if(!nhi->nhi_SameUsages)
                            {
                                acount = 0;
                                do
                                {
                                    if((nhgi = bAllocGItem(nhb, nhi, &nhi->nhi_ActionMap[acount], nhi->nhi_UsageMap[acount])))
                                    {
                                        if((newstr = btCopyStrFmt(" +%s", nhgi->nhgi_Name)))
                                        {
                                            btFreeVec(nhgi->nhgi_Name);
                                            nhgi->nhgi_Name = newstr;
                                        }

                                        DoMethod(nhb->nhb_ItemLVObj, MUIM_List_InsertSingle, nhgi, MUIV_List_Insert_Bottom);
                                        if((!nhb->nhb_GUICurrentItem) ||
                                           (nhb->nhb_TrackEvents && (nhgi->nhgi_ActionList == nhb->nhb_LastItemAList)))
                                        {
                                            nhb->nhb_GUICurrentItem = nhgi;
                                            jumppos = pos;
                                        }
                                        pos++;
                                    }
                                } while(++acount < (nhi->nhi_LogicalMax-nhi->nhi_LogicalMin+1));
                            }
                        }
                    }
                    nhi = (struct BtHidItem *) nhi->nhi_Node.ln_Succ;
                }
                set(nhb->nhb_FillDefObj, MUIA_Disabled, FALSE);
                set(nhb->nhb_ClearActionsObj, MUIA_Disabled, FALSE);
                set(nhb->nhb_ItemLVObj, MUIA_Disabled, FALSE);
                set(nhb->nhb_ItemLVObj, MUIA_List_Quiet, FALSE);
                nhb->nhb_SilentActionUpdate = FALSE;
                set(nhb->nhb_ItemLVObj, MUIA_List_Active, jumppos);
                DoMethod(nhb->nhb_ItemLVObj, MUIM_List_Jump, jumppos);
            } else {
                set(nhb->nhb_FillDefObj, MUIA_Disabled, TRUE);
                set(nhb->nhb_ClearActionsObj, MUIA_Disabled, TRUE);
                nhb->nhb_SilentActionUpdate = FALSE;
                set(nhb->nhb_ItemLVObj, MUIA_Disabled, TRUE);
                set(nhb->nhb_ItemLVObj, MUIA_List_Quiet, FALSE);
            }
            if(!nhb->nhb_GUICurrentAction)
            {
                DoMethod(nhb->nhb_ActionObj, MUIM_Action_SelectAction);
            }
            return(TRUE);
        }

        case MUIM_Action_DebugReport:
            if(nhb->nhb_GUICurrentColl)
            {
                bDebugReport(nhb, nhb->nhb_GUICurrentColl->nhc_Report);
            }
            return(TRUE);

        case MUIM_Action_SelectItem:
        {
            struct BtHidGItem *nhgi;
            struct BtHidAction *nha;

            nhb->nhb_SilentActionUpdate = TRUE;
            DoMethod(nhb->nhb_ItemLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nhb->nhb_GUICurrentItem);
            set(nhb->nhb_ActionLVObj, MUIA_List_Quiet, TRUE);
            DoMethod(nhb->nhb_ActionLVObj, MUIM_List_Clear);
            nhb->nhb_GUICurrentAction = NULL;
            nhb->nhb_SilentActionUpdate = FALSE;
            if((nhgi = nhb->nhb_GUICurrentItem))
            {
                nha = (struct BtHidAction *) nhgi->nhgi_ActionList->lh_Head;
                while(nha->nha_Node.ln_Succ)
                {
                    DoMethod(nhb->nhb_ActionLVObj, MUIM_List_InsertSingle, nha, MUIV_List_Insert_Bottom);
                    if(!nhb->nhb_GUICurrentAction)
                    {
                        nhb->nhb_GUICurrentAction = nha;
                        set(nhb->nhb_ActionLVObj, MUIA_List_Active, MUIV_List_Active_Top);
                    }
                    nha = (struct BtHidAction *) nha->nha_Node.ln_Succ;
                }
                set(nhb->nhb_ActionLVObj, MUIA_Disabled, FALSE);
                set(nhb->nhb_ActionNewObj, MUIA_Disabled, FALSE);
                //set(nhb->nhb_ActionLVObj, MUIA_List_Active, MUIV_List_Active_Off);
            } else {
                set(nhb->nhb_ActionLVObj, MUIA_Disabled, TRUE);
                set(nhb->nhb_ActionNewObj, MUIA_Disabled, TRUE);
                set(nhb->nhb_ActionCopyObj, MUIA_Disabled, TRUE);
                set(nhb->nhb_ActionDelObj, MUIA_Disabled, TRUE);
                set(nhb->nhb_ActionUpObj, MUIA_Disabled, TRUE);
                set(nhb->nhb_ActionDownObj, MUIA_Disabled, TRUE);
            }
            if(!nhb->nhb_GUICurrentAction)
            {
                DoMethod(nhb->nhb_ActionObj, MUIM_Action_SelectAction);
            }
            set(nhb->nhb_ActionLVObj, MUIA_List_Quiet, FALSE);

            return(TRUE);
        }

        case MUIM_Action_SelectAction:
        {
            struct BtHidAction *nha;
            if(!nhb->nhb_SilentActionUpdate)
            {
                DoMethod(nhb->nhb_ActionLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nhb->nhb_GUICurrentAction);
                if((nha = nhb->nhb_GUICurrentAction))
                {
                    set(nhb->nhb_ActionSelectorObj, MUIA_Cycle_Active, nha->nha_Type & HUA_ATYPEMASK);
                    set(nhb->nhb_ActionAreaObj, MUIA_Disabled, FALSE);
                    set(nhb->nhb_ActionCopyObj, MUIA_Disabled, FALSE);
                    set(nhb->nhb_ActionDelObj, MUIA_Disabled, FALSE);
                    set(nhb->nhb_ActionUpObj, MUIA_Disabled, FALSE);
                    set(nhb->nhb_ActionDownObj, MUIA_Disabled, FALSE);
                    DoMethod(nhb->nhb_ActionObj, MUIM_Action_SetActionType);
                } else {
                    set(nhb->nhb_ActionAreaObj, MUIA_Disabled, TRUE);
                    set(nhb->nhb_ActionCopyObj, MUIA_Disabled, TRUE);
                    set(nhb->nhb_ActionDelObj, MUIA_Disabled, TRUE);
                    set(nhb->nhb_ActionUpObj, MUIA_Disabled, TRUE);
                    set(nhb->nhb_ActionDownObj, MUIA_Disabled, TRUE);
                }
            }
            return(TRUE);
        }

        case MUIM_Action_FillDefReport:
        {
            struct BtHidItem *nhi;
            struct BtHidCollection *nhc;
            ULONG count;
            struct List *alistptr;
            ULONG *usageptr;

            DoMethod(nhb->nhb_ReportLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nhc);
            if(nhc)
            {
                if(MUI_Request(nhb->nhb_App, nhb->nhb_MainWindow, 0, NULL, "Fill with default|Cancel",
                               "Warning! This operation will erase\n"
                               "all of the actions defined for\n"
                               "the selected collection '%s'\n"
                               "replace it with default values.", nhc->nhc_Name))
                {
                    DoMethod(nhb->nhb_ActionLVObj, MUIM_List_Clear);
                    Forbid();
                    bCleanCollection(nhb, nhc);
                    nhi = (struct BtHidItem *) nhc->nhc_Items.lh_Head;
                    while(nhi->nhi_Node.ln_Succ)
                    {
                        if(nhi->nhi_Flags & RPF_MAIN_VARIABLE)
                        {
                            usageptr = &nhi->nhi_Usage;
                            alistptr = &nhi->nhi_ActionList;
                            count = 1;
                        } else {
                            usageptr = nhi->nhi_UsageMap;
                            alistptr = nhi->nhi_ActionMap;
                            count = nhi->nhi_MapSize;
                        }
                        do
                        {
                            bDetectDefaultAction(nhb, nhi, alistptr++, nhc, *usageptr++);
                        } while(--count);
                        nhi = (struct BtHidItem *) nhi->nhi_Node.ln_Succ;
                    }
                    Permit();
                    nhb->nhb_GUICurrentAction = NULL;
                    set(nhb->nhb_ItemLVObj, MUIA_List_Active, MUIV_List_Active_Off);
                    set(nhb->nhb_ItemLVObj, MUIA_List_Active, MUIV_List_Active_Top);
                }
            }
            return(TRUE);
        }

        case MUIM_Action_SwapLMBRMB:
        case MUIM_Action_SetJoyPort:
        {
            struct BtHidItem *nhi;
            struct BtHidCollection *nhc;
            struct BtHidAction *nha;
            ULONG count;
            struct List *alistptr;

            DoMethod(nhb->nhb_ReportLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nhc);
            if(nhc)
            {
                nhi = (struct BtHidItem *) nhc->nhc_Items.lh_Head;
                while(nhi->nhi_Node.ln_Succ)
                {
                    if(nhi->nhi_Flags & RPF_MAIN_VARIABLE)
                    {
                        alistptr = &nhi->nhi_ActionList;
                        count = 1;
                    } else {
                        alistptr = nhi->nhi_ActionMap;
                        count = nhi->nhi_MapSize;
                    }
                    do
                    {
                        nha = (struct BtHidAction *) alistptr->lh_Head;
                        while(nha->nha_Node.ln_Succ)
                        {
                            if(msg->MethodID == MUIM_Action_SwapLMBRMB)
                            {
                                if(nha->nha_ButtonNo == 1)
                                {
                                    nha->nha_ButtonNo = 2;
                                }
                                else if(nha->nha_ButtonNo == 2)
                                {
                                    nha->nha_ButtonNo = 1;
                                }
                            } else {
                                if(((nha->nha_Type & HUA_ATYPEMASK) == HUA_DIGJOY) ||
                                   ((nha->nha_Type & HUA_ATYPEMASK) == HUA_ANALOGJOY))
                                {
                                    nha->nha_JoypadPort = (IPTR)((struct opSet *) msg)->ops_AttrList;
                                }
                            }
                            nha = (struct BtHidAction *) nha->nha_Node.ln_Succ;
                        }
                        alistptr++;
                    } while(--count);
                    nhi = (struct BtHidItem *) nhi->nhi_Node.ln_Succ;
                }
                DoMethod(nhb->nhb_ActionObj, MUIM_Action_SelectItem);
            }

            return(TRUE);
        }

        case MUIM_Action_ClearReport:
        {
            struct BtHidCollection *nhc;

            DoMethod(nhb->nhb_ReportLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nhc);
            if(nhc)
            {
                if(MUI_Request(nhb->nhb_App, nhb->nhb_MainWindow, 0, NULL, "Clear|Cancel",
                               "Warning! This operation will erase\n"
                               "all of the actions defined for\n"
                               "the selected collection '%s'\n"
                               "replace them with default values.", nhc->nhc_Name))
                {
                    DoMethod(nhb->nhb_ActionLVObj, MUIM_List_Clear);
                    Forbid();
                    bCleanCollection(nhb, nhc);
                    Permit();
                    nhb->nhb_GUICurrentAction = NULL;
                    set(nhb->nhb_ItemLVObj, MUIA_List_Active, MUIV_List_Active_Off);
                    set(nhb->nhb_ItemLVObj, MUIA_List_Active, MUIV_List_Active_Top);
                }
            }
            return(TRUE);
        }

        case MUIM_Action_SetTracking:
        {
            ULONG state = 0;
            get(nhb->nhb_TrackKeyEventsObj, MUIA_Selected, &state);
            nhb->nhb_TrackKeyEvents = state;
            get(nhb->nhb_TrackEventsObj, MUIA_Selected, &state);
            nhb->nhb_TrackEvents = state;
            get(nhb->nhb_ReportValuesObj, MUIA_Selected, &state);
            nhb->nhb_ReportValues = state;
            get(nhb->nhb_DisableActionsObj, MUIA_Selected, &state);
            nhb->nhb_DisableActions = state;
            return(TRUE);
        }

        case MUIM_Action_NewAction:
        {
            struct BtHidAction *nha;
            if(nhb->nhb_GUICurrentItem)
            {
                Forbid();
                nha = bAllocAction(nhb, nhb->nhb_GUICurrentItem->nhgi_ActionList, HUA_NOP|HUA_DOWNEVENT);
                Permit();
                if(nha)
                {
                    DoMethod(nhb->nhb_ActionLVObj, MUIM_List_InsertSingle, nha, MUIV_List_Insert_Bottom);
                    set(nhb->nhb_ActionLVObj, MUIA_List_Active, MUIV_List_Active_Bottom);
                }
            }
            return(TRUE);
        }

        case MUIM_Action_CopyAction:
        {
            struct BtHidAction *nha;
            struct BtHidAction *newnha;
            DoMethod(nhb->nhb_ActionLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nhb->nhb_GUICurrentAction);
            if((nha = nhb->nhb_GUICurrentAction))
            {
                newnha = btAllocVec(sizeof(struct BtHidAction));
                if(newnha)
                {
                    *newnha = *nha;
                    Forbid();
                    AddTail(nhb->nhb_GUICurrentItem->nhgi_ActionList, &newnha->nha_Node);
                    Permit();
                    DoMethod(nhb->nhb_ActionLVObj, MUIM_List_InsertSingle, newnha, MUIV_List_Insert_Bottom);
                    set(nhb->nhb_ActionLVObj, MUIA_List_Active, MUIV_List_Active_Bottom);
                }
            }
            return(TRUE);
        }

        case MUIM_Action_DelAction:
        {
            struct BtHidAction *nha;
            DoMethod(nhb->nhb_ActionLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nhb->nhb_GUICurrentAction);
            if((nha = nhb->nhb_GUICurrentAction))
            {
                nhb->nhb_GUICurrentAction = NULL;
                DoMethod(nhb->nhb_ActionLVObj, MUIM_List_Remove, MUIV_List_Remove_Active);
                Forbid();
                Remove(&nha->nha_Node);
                btFreeVec(nha);
                Permit();
            }
            return(TRUE);
        }

        case MUIM_Action_MoveActionUp:
        {
            struct Node *nha;
            struct Node *nhapred;
            struct Node *nhapredpred;
            struct Node *nhasucc;
            DoMethod(nhb->nhb_ActionLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nha);
            if(nha)
            {
                nhasucc = nha->ln_Succ;
                nhapred = nha->ln_Pred;
                if((nhapredpred = nhapred->ln_Pred))
                {
                    Forbid();
                    nhapredpred->ln_Succ = nha;
                    nha->ln_Pred = nhapredpred;
                    nha->ln_Succ = nhapred;
                    nhapred->ln_Pred = nha;
                    nhapred->ln_Succ = nhasucc;
                    nhasucc->ln_Pred = nhapred;
                    Permit();
                    DoMethod(nhb->nhb_ActionLVObj, MUIM_List_Move, MUIV_List_Move_Active, MUIV_List_Move_Previous);
                    set(nhb->nhb_ActionLVObj, MUIA_List_Active, MUIV_List_Active_Up);
                }
            }
            return(TRUE);
        }

        case MUIM_Action_MoveActionDown:
        {
            struct Node *nha;
            struct Node *nhapred;
            struct Node *nhasucc;
            struct Node *nhasuccsucc;
            DoMethod(nhb->nhb_ActionLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nha);
            if(nha)
            {
                nhasucc = nha->ln_Succ;
                nhapred = nha->ln_Pred;
                if((nhasuccsucc = nhasucc->ln_Succ))
                {
                    Forbid();
                    nhapred->ln_Succ = nhasucc;
                    nhasucc->ln_Pred = nhapred;
                    nhasucc->ln_Succ = nha;
                    nha->ln_Pred = nhasucc;
                    nha->ln_Succ = nhasuccsucc;
                    nhasuccsucc->ln_Pred = nha;
                    Permit();
                    DoMethod(nhb->nhb_ActionLVObj, MUIM_List_Move, MUIV_List_Move_Active, MUIV_List_Move_Next);
                    set(nhb->nhb_ActionLVObj, MUIA_List_Active, MUIV_List_Active_Down);
                }
            }
            return(TRUE);
        }

        case MUIM_Action_SetActionType:
        {
            struct BtHidAction *nha;
            if((nha = nhb->nhb_GUICurrentAction))
            {
                tmpval = 0;
                get(nhb->nhb_ActionSelectorObj, MUIA_Cycle_Active, &tmpval);
                if(tmpval != (nha->nha_Type & HUA_ATYPEMASK))
                {
                    nha->nha_IsDefault = FALSE;
                }
                nha->nha_Type = (nha->nha_Type & HUA_TRIGMASK) | tmpval;
                nnset(nhb->nhb_ActionTriggerObj, MUIA_Cycle_Active, bRevLookup(nha->nha_Type & HUA_TRIGMASK, 0, ActionTriggerVals));

                nnset(nhb->nhb_ActionAbsToRelObj, MUIA_Selected, nha->nha_AbsToRel);
                nnset(nhb->nhb_ActionClipEnableObj, MUIA_Selected, nha->nha_ClipEnable);
                nnset(nhb->nhb_A_ClipMinObj, MUIA_Numeric_Value, nha->nha_ClipMin);
                nnset(nhb->nhb_A_ClipMaxObj, MUIA_Numeric_Value, nha->nha_ClipMax);
                nnset(nhb->nhb_A_ClipStretchObj, MUIA_Selected, nha->nha_ClipStretch);

                nnset(nhb->nhb_ActionScaleEnableObj, MUIA_Selected, nha->nha_ScaleEnable);
                btSafeRawDoFmt(nhb->nhb_TmpStrBuf0, 80, "%ld", nha->nha_ScaleMin);
                nnset(nhb->nhb_A_ScaleMinObj, MUIA_String_Contents, nhb->nhb_TmpStrBuf0);
                btSafeRawDoFmt(nhb->nhb_TmpStrBuf0, 80, "%ld", nha->nha_ScaleMax);
                nnset(nhb->nhb_A_ScaleMaxObj, MUIA_String_Contents, nhb->nhb_TmpStrBuf0);

                nnset(nhb->nhb_ActionCCEnableObj, MUIA_Selected, nha->nha_CCEnable);
                nnset(nhb->nhb_A_CCVar1Obj, MUIA_Cycle_Active, bRevLookup(nha->nha_CCVar1, 0, A_CCVariableVals));
                nnset(nhb->nhb_A_CCCondObj, MUIA_Cycle_Active, bRevLookup(nha->nha_CCCond, 0, A_CCCondVals));
                nnset(nhb->nhb_A_CCVar2Obj, MUIA_Cycle_Active, bRevLookup(nha->nha_CCVar2, 0, A_CCVariableVals));
                btSafeRawDoFmt(nhb->nhb_TmpStrBuf0, 80, "%ld", nha->nha_CCConst1);
                nnset(nhb->nhb_A_CCConst1Obj, MUIA_String_Contents, nhb->nhb_TmpStrBuf0);
                btSafeRawDoFmt(nhb->nhb_TmpStrBuf0, 80, "%ld", nha->nha_CCConst2);
                nnset(nhb->nhb_A_CCConst2Obj, MUIA_String_Contents, nhb->nhb_TmpStrBuf0);

                nnset(nhb->nhb_ActionValEnableObj, MUIA_Selected, nha->nha_ValEnable);
                nnset(nhb->nhb_A_ValVarObj, MUIA_Cycle_Active, bRevLookup(nha->nha_ValVar, 0, A_CCVariableVals));
                btSafeRawDoFmt(nhb->nhb_TmpStrBuf0, 80, "%ld", nha->nha_ValConst);
                nnset(nhb->nhb_A_ValConstObj, MUIA_String_Contents, nhb->nhb_TmpStrBuf0);

                nnset(nhb->nhb_A_ClipGroupObj, MUIA_ShowMe, nha->nha_ClipEnable);
                nnset(nhb->nhb_A_ScaleGroupObj, MUIA_ShowMe, nha->nha_ScaleEnable);
                nnset(nhb->nhb_A_CCGroupObj, MUIA_ShowMe, nha->nha_CCEnable);
                nnset(nhb->nhb_A_ValGroupObj, MUIA_ShowMe, nha->nha_ValEnable);

                nnset(nhb->nhb_A_KeyQualOpObj, MUIA_Cycle_Active, bRevLookup(nha->nha_QualMode, 0, A_QualOpVals));
                nnset(nhb->nhb_A_KeyQualObj, MUIA_Cycle_Active, nha->nha_Qualifier);
                nnset(nhb->nhb_A_RawKeyUpObj, MUIA_Selected, nha->nha_RawKey & IECODE_UP_PREFIX);
                nnset(nhb->nhb_A_RawKeyObj, MUIA_List_Active, nha->nha_RawKey & (~IECODE_UP_PREFIX));
                nnset(nhb->nhb_A_VanillaStrObj, MUIA_String_Contents, nha->nha_VanillaString);
                nnset(nhb->nhb_A_KeyStringObj, MUIA_String_Contents, nha->nha_KeyString);
                nnset(nhb->nhb_A_MousePosOpObj, MUIA_Cycle_Active, bRevLookup(nha->nha_MouseAxis, 0, A_MousePosOpVals));
                nnset(nhb->nhb_A_MouseButOpObj, MUIA_Cycle_Active, bRevLookup(nha->nha_ButtonMode, 0, A_MouseButOpVals));
                nnset(nhb->nhb_A_MouseButObj, MUIA_Cycle_Active, nha->nha_ButtonNo ? nha->nha_ButtonNo-1 : 0);
                nnset(nhb->nhb_A_TabletAxisObj, MUIA_Cycle_Active, bRevLookup(nha->nha_TabletAxis, 0, A_TabletAxisVals));
                nnset(nhb->nhb_A_WheelOpObj, MUIA_Cycle_Active, bRevLookup(nha->nha_WheelMode, 0, A_WheelOpVals));
                nnset(nhb->nhb_A_WheelDistObj, MUIA_Numeric_Value, nha->nha_WheelDist);
                nnset(nhb->nhb_A_JoypadOpObj, MUIA_Cycle_Active, bRevLookup(nha->nha_JoypadOp, 0, A_JoypadOpVals));
                nnset(nhb->nhb_A_JoypadFeatObj, MUIA_Cycle_Active, bRevLookup(nha->nha_JoypadFeat, 0, A_JoypadFeatVals));
                nnset(nhb->nhb_A_JoypadPortObj, MUIA_Cycle_Active, nha->nha_JoypadPort);
                nnset(nhb->nhb_A_APadFeatObj, MUIA_Cycle_Active, bRevLookup(nha->nha_APadFeat, 0, A_APadFeatVals));
                nnset(nhb->nhb_A_APadPortObj, MUIA_Cycle_Active, nha->nha_JoypadPort);
                nnset(nhb->nhb_A_SoundFileObj, MUIA_String_Contents, nha->nha_SoundFile);
                nnset(nhb->nhb_A_SoundVolObj, MUIA_Numeric_Value, nha->nha_SoundVolume);
                nnset(nhb->nhb_A_ShellComObj, MUIA_String_Contents, nha->nha_ExeString);
                nnset(nhb->nhb_A_ShellAsyncObj, MUIA_Selected, nha->nha_ShellAsync);
                nnset(nhb->nhb_A_OutOpObj, MUIA_Cycle_Active, bRevLookup(nha->nha_OutOp, 0, A_OutOpVals));
                nnset(nhb->nhb_A_OutArrayObj, MUIA_String_Contents, nha->nha_OutArray);
                {
                    struct BtHidItem *nhi;
                    struct BtHidGItem *nhgi;
                    ULONG pos;
                    if((nhi = bFindItemID(nhb, nha->nha_OutItem, REPORT_MAIN_OUTPUT, &pos)))
                    {
                        pos = 0;
                        do
                        {
                            nhgi = NULL;
                            DoMethod(nhb->nhb_A_OutItemLVObj, MUIM_List_GetEntry, pos, &nhgi);
                            if(!nhgi)
                            {
                                break;
                            }
                            if(nhgi->nhgi_Item == nhi)
                            {
                                nnset(nhb->nhb_A_OutItemLVObj, MUIA_List_Active, pos);
                                break;
                            }
                            pos++;
                        }
                        while(TRUE);
                    }
                }
                nnset(nhb->nhb_A_FeatOpObj, MUIA_Cycle_Active, bRevLookup(nha->nha_FeatOp, 0, A_OutOpVals));
                nnset(nhb->nhb_A_FeatArrayObj, MUIA_String_Contents, nha->nha_OutArray);
                {
                    struct BtHidItem *nhi;
                    struct BtHidGItem *nhgi;
                    ULONG pos;
                    if((nhi = bFindItemID(nhb, nha->nha_FeatItem, REPORT_MAIN_FEATURE, &pos)))
                    {
                        pos = 0;
                        do
                        {
                            nhgi = NULL;
                            DoMethod(nhb->nhb_A_FeatItemLVObj, MUIM_List_GetEntry, pos, &nhgi);
                            if(!nhgi)
                            {
                                break;
                            }
                            if(nhgi->nhgi_Item == nhi)
                            {
                                nnset(nhb->nhb_A_FeatItemLVObj, MUIA_List_Active, pos);
                                break;
                            }
                            pos++;
                        }
                        while(TRUE);
                    }
                }
                nnset(nhb->nhb_A_MiscOpObj, MUIA_Cycle_Active, bRevLookup(nha->nha_MiscMode, 0, A_MiscOpVals));

                nnset(nhb->nhb_A_TarVarObj, MUIA_Cycle_Active, bRevLookup(nha->nha_TarVar, 0, A_TarVariableVals));
                nnset(nhb->nhb_A_TarVarOpObj, MUIA_Cycle_Active, bRevLookup(nha->nha_TarVarOp, 0, A_TarVarOpVals));

                nnset(nhb->nhb_A_ExtRawKeyUpObj, MUIA_Selected, nha->nha_RawKey & IECODE_UP_PREFIX);
                nnset(nhb->nhb_A_ExtRawKeyObj, MUIA_List_Active, nha->nha_RawKey & (~IECODE_UP_PREFIX));

                DoMethod(nhb->nhb_ActionObj, MUIM_Action_UpdateAction);
                DoMethod(nhb->nhb_ActionObj, MUIM_Action_UpdateAOptions);
                DoMethod(nhb->nhb_ActionLVObj, MUIM_List_Redraw, MUIV_List_Redraw_Active);
            }
            return(TRUE);
        }

        case MUIM_Action_UpdateAOptions:
        {
            struct BtHidAction *nha;

            if((nha = nhb->nhb_GUICurrentAction))
            {
                nha->nha_IsDefault = FALSE;
                tmpval = 0;
                get(nhb->nhb_ActionAbsToRelObj, MUIA_Selected, &tmpval);
                nha->nha_AbsToRel = tmpval;

                tmpval = 0;
                get(nhb->nhb_ActionClipEnableObj, MUIA_Selected, &tmpval);
                nha->nha_ClipEnable = tmpval;
                if(tmpval)
                {
                    tmpval = 0;
                    get(nhb->nhb_A_ClipMinObj, MUIA_Numeric_Value, &tmpval);
                    nha->nha_ClipMin = tmpval;
                    tmpval = 0;
                    get(nhb->nhb_A_ClipMaxObj, MUIA_Numeric_Value, &tmpval);
                    nha->nha_ClipMax = tmpval;
                    tmpval = 0;
                    get(nhb->nhb_A_ClipStretchObj, MUIA_Selected, &tmpval);
                    nha->nha_ClipStretch = tmpval;
                }

                tmpval = 0;
                get(nhb->nhb_ActionScaleEnableObj, MUIA_Selected, &tmpval);
                nha->nha_ScaleEnable = tmpval;
                if(tmpval)
                {
                    tmpval = 0;
                    get(nhb->nhb_A_ScaleMinObj, MUIA_String_Integer, &tmpval);
                    nha->nha_ScaleMin = tmpval;
                    tmpval = 0;
                    get(nhb->nhb_A_ScaleMaxObj, MUIA_String_Integer, &tmpval);
                    nha->nha_ScaleMax = tmpval;
                }

                tmpval = 0;
                get(nhb->nhb_ActionCCEnableObj, MUIA_Selected, &tmpval);
                nha->nha_CCEnable = tmpval;
                if(tmpval)
                {
                    tmpval = 0;
                    get(nhb->nhb_A_CCVar1Obj, MUIA_Cycle_Active, &tmpval);
                    nha->nha_CCVar1 = A_CCVariableVals[tmpval];
                    tmpval = 0;
                    get(nhb->nhb_A_CCCondObj, MUIA_Cycle_Active, &tmpval);
                    nha->nha_CCCond = A_CCCondVals[tmpval];
                    tmpval = 0;
                    get(nhb->nhb_A_CCVar2Obj, MUIA_Cycle_Active, &tmpval);
                    nha->nha_CCVar2 = A_CCVariableVals[tmpval];
                    tmpval = 0;
                    get(nhb->nhb_A_CCConst1Obj, MUIA_String_Integer, &tmpval);
                    nha->nha_CCConst1 = tmpval;
                    tmpval = 0;
                    get(nhb->nhb_A_CCConst2Obj, MUIA_String_Integer, &tmpval);
                    nha->nha_CCConst2 = tmpval;
                }

                tmpval = 0;
                get(nhb->nhb_ActionValEnableObj, MUIA_Selected, &tmpval);
                nha->nha_ValEnable = tmpval;
                if(tmpval)
                {
                    tmpval = 0;
                    get(nhb->nhb_A_ValVarObj, MUIA_Cycle_Active, &tmpval);
                    nha->nha_ValVar = A_CCVariableVals[tmpval];
                    tmpval = 0;
                    get(nhb->nhb_A_ValConstObj, MUIA_String_Integer, &tmpval);
                    nha->nha_ValConst = tmpval;
                }
            }
            return(TRUE);
        }

        case MUIM_Action_UpdateAction:
        {
            struct BtHidAction *nha;
            struct BtHidGItem *nhgi;

            if((nha = nhb->nhb_GUICurrentAction))
            {
                nha->nha_IsDefault = FALSE;
                tmpval = 0;
                get(nhb->nhb_ActionTriggerObj, MUIA_Cycle_Active, &tmpval);
                nha->nha_Type = (nha->nha_Type & HUA_ATYPEMASK) | ActionTriggerVals[tmpval];
                switch(nha->nha_Type & HUA_ATYPEMASK)
                {
                    case HUA_QUALIFIER:
                        tmpval = 0;
                        get(nhb->nhb_A_KeyQualOpObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_QualMode = A_QualOpVals[tmpval];
                        tmpval = 0;
                        get(nhb->nhb_A_KeyQualObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_Qualifier = tmpval;
                        break;

                    case HUA_RAWKEY:
                        tmpval = 0;
                        get(nhb->nhb_A_RawKeyUpObj, MUIA_Selected, &tmpval);
                        if(tmpval)
                        {
                            tmpval = 0;
                            get(nhb->nhb_A_RawKeyObj, MUIA_List_Active, &tmpval);
                            nha->nha_RawKey = tmpval|IECODE_UP_PREFIX;
                        } else {
                            tmpval = 0;
                            get(nhb->nhb_A_RawKeyObj, MUIA_List_Active, &tmpval);
                            nha->nha_RawKey = tmpval;
                        }
                        break;

                    case HUA_VANILLA:
                        tmpstr = "";
                        get(nhb->nhb_A_VanillaStrObj, MUIA_String_Contents, &tmpstr);
                        strncpy(nha->nha_VanillaString, tmpstr, 79);
                        break;

                    case HUA_KEYSTRING:
                        tmpstr = "";
                        get(nhb->nhb_A_KeyStringObj, MUIA_String_Contents, &tmpstr);
                        strncpy(nha->nha_KeyString, tmpstr, 79);
                        break;

                    case HUA_MOUSEPOS:
                        tmpval = 0;
                        get(nhb->nhb_A_MousePosOpObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_MouseAxis = A_MousePosOpVals[tmpval];
                        break;

                    case HUA_BUTTONS:
                        tmpval = 0;
                        get(nhb->nhb_A_MouseButOpObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_ButtonMode = A_MouseButOpVals[tmpval];
                        tmpval = 0;
                        get(nhb->nhb_A_MouseButObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_ButtonNo = tmpval+1;
                        break;

                    case HUA_TABLET:
                        tmpval = 0;
                        get(nhb->nhb_A_TabletAxisObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_TabletAxis = A_TabletAxisVals[tmpval];
                        break;

                    case HUA_WHEEL:
                        tmpval = 0;
                        get(nhb->nhb_A_WheelOpObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_WheelMode = A_WheelOpVals[tmpval];
                        tmpval = 0;
                        get(nhb->nhb_A_WheelDistObj, MUIA_Numeric_Value, &tmpval);
                        nha->nha_WheelDist = tmpval;
                        break;

                    case HUA_DIGJOY:
                        tmpval = 0;
                        get(nhb->nhb_A_JoypadOpObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_JoypadOp = A_JoypadOpVals[tmpval];
                        tmpval = 0;
                        get(nhb->nhb_A_JoypadFeatObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_JoypadFeat = A_JoypadFeatVals[tmpval];
                        tmpval = 0;
                        get(nhb->nhb_A_JoypadPortObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_JoypadPort = tmpval;
                        break;

                    case HUA_ANALOGJOY:
                        tmpval = 0;
                        get(nhb->nhb_A_APadFeatObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_APadFeat = A_APadFeatVals[tmpval];
                        tmpval = 0;
                        get(nhb->nhb_A_APadPortObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_JoypadPort = tmpval;
                        break;

                    case HUA_SOUND:
                        tmpstr = "";
                        get(nhb->nhb_A_SoundFileObj, MUIA_String_Contents, &tmpstr);
                        strncpy(nha->nha_SoundFile, tmpstr, 255);
                        tmpval = 0;
                        get(nhb->nhb_A_SoundVolObj, MUIA_Numeric_Value, &tmpval);
                        nha->nha_SoundVolume = tmpval;
                        break;

                    case HUA_SHELL:
                        tmpstr = "";
                        get(nhb->nhb_A_ShellComObj, MUIA_String_Contents, &tmpstr);
                        strncpy(nha->nha_ExeString, tmpstr, 79);
                        tmpval = 0;
                        get(nhb->nhb_A_ShellAsyncObj, MUIA_Selected, &tmpval);
                        nha->nha_ShellAsync = tmpval;
                        break;

                    case HUA_OUTPUT:
                        DoMethod(nhb->nhb_A_OutItemLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nhgi);
                        tmpval = 0;
                        get(nhb->nhb_A_OutOpObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_OutOp = A_OutOpVals[tmpval];
                        tmpstr = "";
                        get(nhb->nhb_A_OutArrayObj, MUIA_String_Contents, &tmpstr);
                        strncpy(nha->nha_OutArray, tmpstr, 255);
                        if(nhgi)
                        {
                            nha->nha_OutItem = GET_WTYPE(nhgi->nhgi_ActionList);
                            if(nhgi->nhgi_Item->nhi_Flags & RPF_MAIN_VARIABLE)
                            {
                                set(nhb->nhb_A_OutArrayObj, MUIA_Disabled, TRUE);
                                set(nhb->nhb_A_OutOpObj, MUIA_Disabled, FALSE);
                            } else {
                                set(nhb->nhb_A_OutArrayObj, MUIA_Disabled, FALSE);
                                set(nhb->nhb_A_OutOpObj, MUIA_Disabled, TRUE);
                            }
                        } else {
                            nha->nha_OutItem = 0;
                        }
                        break;

                    case HUA_FEATURE:
                        DoMethod(nhb->nhb_A_FeatItemLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nhgi);
                        tmpval = 0;
                        get(nhb->nhb_A_FeatOpObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_FeatOp = A_OutOpVals[tmpval];
                        tmpstr = "";
                        get(nhb->nhb_A_FeatArrayObj, MUIA_String_Contents, &tmpstr);
                        strncpy(nha->nha_OutArray, tmpstr, 255);
                        if(nhgi)
                        {
                            nha->nha_FeatItem = GET_WTYPE(nhgi->nhgi_ActionList);
                        } else {
                            nha->nha_FeatItem = 0;
                        }
                        break;

                    case HUA_MISC:
                        tmpval = 0;
                        get(nhb->nhb_A_MiscOpObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_MiscMode = A_MiscOpVals[tmpval];
                        break;

                    case HUA_VARIABLES:
                        tmpval = 0;
                        get(nhb->nhb_A_TarVarObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_TarVar = A_TarVariableVals[tmpval];
                        tmpval = 0;
                        get(nhb->nhb_A_TarVarOpObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_TarVarOp = A_TarVarOpVals[tmpval];
                        break;

                   case HUA_EXTRAWKEY:
                        tmpval = 0;
                        get(nhb->nhb_A_ExtRawKeyUpObj, MUIA_Selected, &tmpval);
                        if(tmpval)
                        {
                            tmpval = 0;
                            get(nhb->nhb_A_ExtRawKeyObj, MUIA_List_Active, &tmpval);
                            nha->nha_RawKey = tmpval|IECODE_UP_PREFIX;
                        } else {
                            tmpval = 0;
                            get(nhb->nhb_A_ExtRawKeyObj, MUIA_List_Active, &tmpval);
                            nha->nha_RawKey = tmpval;
                        }
                        break;

                }
                DoMethod(nhb->nhb_ActionLVObj, MUIM_List_Redraw, MUIV_List_Redraw_Active);
            }
            return(TRUE);
        }

        case MUIM_Action_KeymapSelectUSB:
            DoMethod(nhb->nhb_USBKeymapLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &hum);
            if(hum)
            {
                nhb->nhb_CurrUSBKey = hum->hum_ID | 0x070000;
                set(nhb->nhb_RawKeymapLVObj, MUIA_List_Active, nhb->nhb_KeymapCfg.kmc_Keymap[hum->hum_ID]);
            }
            return(TRUE);

        case MUIM_Action_KeymapSelectRaw:
            DoMethod(nhb->nhb_USBKeymapLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &hum);
            if(hum)
            {
                tmpval = 0;
                get(nhb->nhb_RawKeymapLVObj, MUIA_List_Active, &tmpval);
                nhb->nhb_KeymapCfg.kmc_Keymap[hum->hum_ID] = tmpval;
            }
            return(TRUE);

        case MUIM_Action_RestDefKeymap:
            CopyMemQuick(usbkeymap, nhb->nhb_KeymapCfg.kmc_Keymap, 256);
            DoMethod(nhb->nhb_USBKeymapLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &hum);
            if(hum)
            {
                nhb->nhb_CurrUSBKey = hum->hum_ID | 0x070000;
                set(nhb->nhb_RawKeymapLVObj, MUIA_List_Active, nhb->nhb_KeymapCfg.kmc_Keymap[hum->hum_ID]);
            }
            return(TRUE);

        case MUIM_Action_ShowHIDControl:
            Forbid();
            if(nhb->nhb_HCApp)
            {
                DoMethod(nhb->nhb_HCApp, MUIM_Application_PushMethod,
                         nhb->nhb_HCActionObj, 1, MUIM_Action_ShowHIDControl);
            }
            Permit();
            return(TRUE);
    }
    return(DoSuperMethodA(cl,obj,msg));

    AROS_USERFUNC_EXIT
}
/* \\\ */

