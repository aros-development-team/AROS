/*
 * GUI
 */

#include "debug.h"
#include "numtostr.h"

#include "hid.class.h"

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

/* /// "nRevLookup()" */
ULONG nRevLookup(UWORD id, UWORD def, UWORD *field)
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

/* /// "nGUITask()" */
AROS_UFH0(void, GM_UNIQUENAME(nGUITask))
{
    AROS_USERFUNC_INIT

    struct Task *thistask;
    struct NepHidBase *nh;
    struct NepClassHid *nch;
    UWORD count;
    struct HidUsageIDMap *hum;

    char barbar[] = "BAR,BAR,";
    //char barbarbar[] = "BAR,BAR,BAR,";
    char barbarbarbar[] = "BAR,BAR,BAR,BAR,";

    thistask = FindTask(NULL);
#undef ps
#define ps nch->nch_PsdBase
#undef IntuitionBase
#define IntuitionBase nch->nch_IntBase
#undef KeymapBase
#define KeymapBase nch->nch_KeyBase
#undef MUIMasterBase
#define MUIMasterBase nch->nch_MUIBase

    nch = thistask->tc_UserData;
    nh = nch->nch_ClsBase;

    ++nh->nh_Library.lib_OpenCnt;
    NewList(&nch->nch_GUIItems);
    NewList(&nch->nch_GUIOutItems);
    if(!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)))
    {
        KPRINTF(10, ("Couldn't open muimaster.library.\n"));
        GM_UNIQUENAME(nGUITaskCleanup)(nch);
        return;
    }

    if(!(IntuitionBase = OpenLibrary("intuition.library", 39)))
    {
        KPRINTF(10, ("Couldn't open intuition.library.\n"));
        GM_UNIQUENAME(nGUITaskCleanup)(nch);
        return;
    }
    if(!(KeymapBase = OpenLibrary("keymap.library", 39)))
    {
        KPRINTF(10, ("Couldn't open keymap.library.\n"));
        GM_UNIQUENAME(nGUITaskCleanup)(nch);
        return;
    }
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        KPRINTF(10, ("Couldn't open poseidon.library.\n"));
        GM_UNIQUENAME(nGUITaskCleanup)(nch);
        return;
    }

    nch->nch_ActionClass = MUI_CreateCustomClass(NULL, MUIC_Area  , NULL, sizeof(struct ActionData), GM_UNIQUENAME(ActionDispatcher));
    if(!nch->nch_ActionClass)
    {
        KPRINTF(10, ("Couldn't create ActionClass.\n"));
        GM_UNIQUENAME(nGUITaskCleanup)(nch);
        return;
    }
    for(count = 0; count < 128; count++)
    {
        struct InputEvent ie;
        UBYTE buf[16];
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

        if((betterstr = nNumToStr(nch, NTS_RAWKEY, (ULONG) count, NULL)))
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
                nch->nch_RawKeyArray[count] = psdCopyStrFmt("0x%02lx (%s)", count, buf);
            } else {
                strcpy(buf2, "unprintable seq.");
                charpos = 0;
                do
                {
                    targetpos = strlen(buf2);
                    buf2[targetpos++] = ' ';
                    psdSafeRawDoFmt(&buf2[targetpos], 79-targetpos, "$%02lx", buf[charpos]);
                } while((++charpos < actual) && targetpos < 75);
                nch->nch_RawKeyArray[count] = psdCopyStrFmt("0x%02lx (%s)", count, buf2);
            }
        } else {
            nch->nch_RawKeyArray[count] = psdCopyStrFmt("0x%02lx", count);
        }
    }

    for(count = 0; count < 128; count++)
    {
        nch->nch_ExtRawKeyArray[count] = psdCopyStrFmt("0x%02lx %s", count, nNumToStr(nch, NTS_EXTRAWKEY, (ULONG) count, "<undef.>"));
    }

    hum = (struct HidUsageIDMap *) hidusage07;
    count = 0;
    while(hum->hum_String)
    {
        nch->nch_USBKeyArray[count] = hum++;
        count++;
    }

    nch->nch_USBKeyListDisplayHook.h_Data = nch;
    nch->nch_ReportListDisplayHook.h_Data = nch;
    nch->nch_ItemListDisplayHook.h_Data = nch;
    nch->nch_ActionListDisplayHook.h_Data = nch;

    nch->nch_USBKeyListDisplayHook.h_Entry = (APTR) GM_UNIQUENAME(USBKeyListDisplayHook);
    nch->nch_ReportListDisplayHook.h_Entry = (APTR) GM_UNIQUENAME(ReportListDisplayHook);
    nch->nch_ItemListDisplayHook.h_Entry = (APTR) GM_UNIQUENAME(ItemListDisplayHook);
    nch->nch_ActionListDisplayHook.h_Entry = (APTR) GM_UNIQUENAME(ActionListDisplayHook);

    nch->nch_App = ApplicationObject,
        MUIA_Application_Title      , (IPTR)GM_UNIQUENAME(libname),
        MUIA_Application_Version    , (IPTR)VERSION_STRING,
        MUIA_Application_Copyright  , (IPTR)"©2002-2009 Chris Hodges",
        MUIA_Application_Author     , (IPTR)"Chris Hodges <chrisly@platon42.de>",
        MUIA_Application_Description, (IPTR)"Settings for the hid.class",
        MUIA_Application_Base       , (IPTR)"HID",
        MUIA_Application_HelpFile   , (IPTR)"HELP:Poseidon.guide",
        MUIA_Application_Menustrip  , (IPTR)MenustripObject,
            Child, (IPTR)MenuObjectT((IPTR)"Project"),
                Child, (IPTR)(nch->nch_AboutMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"About...",
                    MUIA_Menuitem_Shortcut, (IPTR)"?",
                    End),
                End,
            Child, (IPTR)MenuObjectT((IPTR)"Quick Setup"),
                //Child, (IPTR)MUIA_Menu_Enabled, nch->nch_Interface ? TRUE : FALSE,
                Child, (IPTR)MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"Mouse",
                    Child, (IPTR)(nch->nch_SwapLMBRMBMI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"Swap LMB<->RMB",
                        End),
                    Child, (IPTR)MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)NM_BARLABEL,
                        End,
                    Child, (IPTR)(nch->nch_MouseAccel100MI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"Acceleration off",
                        MUIA_Menuitem_Enabled, FALSE,
                        End),
                    Child, (IPTR)(nch->nch_MouseAccel150MI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"Acceleration 150%",
                        MUIA_Menuitem_Enabled, FALSE,
                        End),
                    Child, (IPTR)(nch->nch_MouseAccel200MI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"Acceleration 200%",
                        MUIA_Menuitem_Enabled, FALSE,
                        End),
                    End,
                Child, (IPTR)MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"Joystick",
                    Child, (IPTR)(nch->nch_JoyPort0MI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"Select Port 0",
                        End),
                    Child, (IPTR)(nch->nch_JoyPort1MI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"Select Port 1",
                        End),
                    Child, (IPTR)(nch->nch_JoyPort2MI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"Select Port 2",
                        End),
                    Child, (IPTR)(nch->nch_JoyPort3MI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"Select Port 3",
                        End),
                    Child, (IPTR)MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)NM_BARLABEL,
                        End,
                    Child, (IPTR)(nch->nch_JoyAutofireMI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"Add Autofire Actions",
                        MUIA_Menuitem_Enabled, FALSE,
                        End),
                    End,
                End,
            Child, (IPTR)MenuObjectT((IPTR)"Settings"),
                Child, (IPTR)(nch->nch_UseMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"Save",
                    MUIA_Menuitem_Shortcut, (IPTR)"S",
                    End),
                Child, (IPTR)(nch->nch_SetDefaultMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"Save as Default",
                    MUIA_Menuitem_Shortcut, (IPTR)"D",
                    End),
                Child, (IPTR)MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)NM_BARLABEL,
                    End,
                Child, (IPTR)(nch->nch_MUIPrefsMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"MUI Settings",
                    MUIA_Menuitem_Shortcut, (IPTR)"M",
                    End),
                End,
            Child, (IPTR)MenuObjectT((IPTR)"Debug"),
                Child, (IPTR)(nch->nch_DebugReportMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"Report Descriptor",
                    End),
                End,
            End,

        SubWindow, (IPTR)(nch->nch_MainWindow = WindowObject,
            MUIA_Window_ID   , MAKE_ID('M','A','I','N'),
            MUIA_Window_Title, (IPTR)GM_UNIQUENAME(libname),
            MUIA_HelpNode, (IPTR)GM_UNIQUENAME(libname),

            WindowContents, (IPTR)VGroup,
                Child, (IPTR)(nch->nch_ActionObj = NewObject(nch->nch_ActionClass->mcc_Class, 0, MUIA_ShowMe, FALSE, TAG_END)),
                Child, (IPTR)RegisterGroup(nch->nch_Interface ? MainGUIPages : MainGUIPagesDefault),
                    MUIA_CycleChain, 1,
                    MUIA_Register_Frame, TRUE,
                    Child, (IPTR)VGroup,
                        Child, (IPTR)VSpace(0),
                        Child, (IPTR)ColGroup(2), GroupFrameT(nch->nch_Interface ? "Device Settings" : "Default Device Settings"),
                            //Child, (IPTR)HSpace(0),
                            Child, (IPTR)Label((IPTR) "Shell console window:"),
                            Child, (IPTR)(nch->nch_ConWindowObj = StringObject,
                                StringFrame,
                                MUIA_CycleChain, 1,
                                MUIA_String_AdvanceOnCR, TRUE,
                                MUIA_String_Contents, (IPTR)nch->nch_CDC->cdc_ShellCon,
                                MUIA_String_MaxLen, 127,
                                End),
                            Child, (IPTR)Label((IPTR) "Shell default stack:"),
                            Child, (IPTR)(nch->nch_ShellStackObj = StringObject,
                                StringFrame,
                                MUIA_CycleChain, 1,
                                MUIA_String_AdvanceOnCR, TRUE,
                                MUIA_String_Integer, nch->nch_CDC->cdc_ShellStack,
                                MUIA_String_Accept, (IPTR)"0123456789",
                                End),
                            Child, (IPTR)Label((IPTR) "Enable keyboard reset:"),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)(nch->nch_EnableKBResetObj = ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, nch->nch_CDC->cdc_EnableKBReset,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                Child, (IPTR)HSpace(0),
                                Child, (IPTR)Label((IPTR) "Hijack ResetHandlers:"),
                                Child, (IPTR)(nch->nch_EnableRHObj = ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, nch->nch_CDC->cdc_EnableRH,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                End,
                            Child, (IPTR)Label((IPTR) "Reset delay:"),
                            Child, (IPTR)(nch->nch_ResetDelayObj = SliderObject, SliderFrame,
                                MUIA_CycleChain, 1,
                                MUIA_Numeric_Min, 0,
                                MUIA_Numeric_Max, 60,
                                MUIA_Numeric_Value, nch->nch_CDC->cdc_ResetDelay,
                                MUIA_Numeric_Format, (IPTR) "%ldsec",
                                End),
                            Child, (IPTR)Label((IPTR) "Turbo mouse:"),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)(nch->nch_TurboMouseObj = CycleObject,
                                    MUIA_CycleChain, 1,
                                    MUIA_Cycle_Entries, (IPTR)TurboMouseStrings,
                                    MUIA_Cycle_Active, nch->nch_CDC->cdc_TurboMouse,
                                    End),
                                Child, (IPTR)HSpace(0),
                                End,
                            End,
                        Child, (IPTR)VSpace(0),
                        Child, (IPTR)ColGroup(2), GroupFrameT("HID Output Control Window"),
                            Child, (IPTR)Label((IPTR) "Open on startup:"),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)(nch->nch_HIDCtrlAutoObj = ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, nch->nch_CDC->cdc_HIDCtrlOpen,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                Child, (IPTR)HSpace(0),
                                Child, (IPTR)(nch->nch_HIDCtrlOpenObj = TextObject, ButtonFrame,
                                    MUIA_ShowMe, (IPTR)nch->nch_Interface,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_RelVerify,
                                    MUIA_Text_Contents, (IPTR)"\33c Open now ",
                                    End),
                                End,
                            Child, (IPTR)Label((IPTR) "Window Title:"),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)(nch->nch_HIDCtrlTitleObj = StringObject,
                                    StringFrame,
                                    MUIA_CycleChain, 1,
                                    MUIA_String_AdvanceOnCR, TRUE,
                                    MUIA_String_Contents, (IPTR)nch->nch_CDC->cdc_HIDCtrlTitle,
                                    MUIA_String_MaxLen, 31,
                                    End),
                                Child, (IPTR)Label((IPTR) "Rexx Port:"),
                                Child, (IPTR)(nch->nch_HIDCtrlRexxObj = StringObject,
                                    StringFrame,
                                    MUIA_CycleChain, 1,
                                    MUIA_String_AdvanceOnCR, TRUE,
                                    MUIA_String_Contents, (IPTR)nch->nch_CDC->cdc_HIDCtrlRexx,
                                    MUIA_String_MaxLen, 31,
                                    End),
                                End,
                            End,
                        Child, (IPTR)VSpace(0),
                        Child, (IPTR)ColGroup(4), GroupFrameT("LowLevel Library Joypad emulation"),
                            Child, (IPTR)Label((IPTR) "Port 0:"),
                            Child, (IPTR)(nch->nch_LLPortModeObj[0] = CycleObject,
                                MUIA_CycleChain, 1,
                                MUIA_Cycle_Entries, (IPTR)LLPortStrings,
                                MUIA_Cycle_Active, nch->nch_CDC->cdc_LLPortMode[0],
                                End),
                            Child, (IPTR)Label((IPTR) "Port 2:"),
                            Child, (IPTR)(nch->nch_LLPortModeObj[2] = CycleObject,
                                MUIA_CycleChain, 1,
                                MUIA_Cycle_Entries, (IPTR)LLPortStrings,
                                MUIA_Cycle_Active, nch->nch_CDC->cdc_LLPortMode[2],
                                End),
                            Child, (IPTR)Label((IPTR) "Port 1:"),
                            Child, (IPTR)(nch->nch_LLPortModeObj[1] = CycleObject,
                                MUIA_CycleChain, 1,
                                MUIA_Cycle_Entries, (IPTR)LLPortStrings,
                                MUIA_Cycle_Active, nch->nch_CDC->cdc_LLPortMode[1],
                                End),
                            Child, (IPTR)Label((IPTR) "Port 3:"),
                            Child, (IPTR)(nch->nch_LLPortModeObj[3] = CycleObject,
                                MUIA_CycleChain, 1,
                                MUIA_Cycle_Entries, (IPTR)LLPortStrings,
                                MUIA_Cycle_Active, nch->nch_CDC->cdc_LLPortMode[3],
                                End),
                            Child, (IPTR)Label((IPTR) "Rumble Port:"),
                            Child, (IPTR)(nch->nch_LLRumblePortObj = CycleObject,
                                MUIA_CycleChain, 1,
                                MUIA_Cycle_Entries, (IPTR)LLRumbleStrings,
                                MUIA_Cycle_Active, nch->nch_CDC->cdc_LLRumblePort,
                                End),
                            Child, (IPTR)HSpace(0),
                            Child, (IPTR)HSpace(0),
                            End,
                        Child, (IPTR)VSpace(0),
                        End,
                    Child, (IPTR)VGroup,
                        //Child, (IPTR)VSpace(0),
                        Child, (IPTR)HGroup, GroupFrameT((IPTR)(nch->nch_Interface ? "Keyboard mapping" : "Default Keyboard mapping")),
                            Child, (IPTR)VGroup,
                                Child, (IPTR)HGroup,
                                    Child, (IPTR)(nch->nch_USBKeymapLVObj = ListviewObject,
                                        MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
                                        MUIA_Listview_List, (IPTR)ListObject,
                                            MUIA_CycleChain, 1,
                                            InputListFrame,
                                            MUIA_List_SourceArray, (IPTR)nch->nch_USBKeyArray,
                                            MUIA_List_DisplayHook, (IPTR)&nch->nch_USBKeyListDisplayHook,
                                            MUIA_List_AutoVisible, TRUE,
                                            End,
                                        End),
                                    Child, (IPTR)VGroup,
                                        Child, (IPTR)VSpace(0),
                                        Child, (IPTR)Label((IPTR) "->"),
                                        Child, (IPTR)VSpace(0),
                                        End,
                                    Child, (IPTR)(nch->nch_RawKeymapLVObj = ListviewObject,
                                        MUIA_Listview_Input, TRUE,
                                        MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
                                        MUIA_Listview_List, (IPTR)ListObject,
                                            InputListFrame,
                                            MUIA_CycleChain, 1,
                                            MUIA_List_SourceArray, (IPTR)nch->nch_RawKeyArray,
                                            MUIA_List_AutoVisible, TRUE,
                                            End,
                                        End),
                                    End,
                                Child, (IPTR)HGroup,
                                    Child, (IPTR)(nch->nch_RestoreDefKeymapObj = TextObject, ButtonFrame,
                                        MUIA_Background, MUII_ButtonBack,
                                        MUIA_CycleChain, 1,
                                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                                        MUIA_Text_Contents, (IPTR)"\33c Restore default keymap ",
                                        End),
                                    Child, (IPTR)HSpace(0),
                                    Child, (IPTR)(nch->nch_TrackKeyEventsObj = ImageObject, ImageButtonFrame,
                                        MUIA_Background, MUII_ButtonBack,
                                        MUIA_CycleChain, 1,
                                        MUIA_InputMode, MUIV_InputMode_Toggle,
                                        MUIA_Image_Spec, MUII_CheckMark,
                                        MUIA_Image_FreeVert, TRUE,
                                        MUIA_Disabled, !nch->nch_Interface,
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
                                    Child, (IPTR)(nch->nch_ReportLVObj = ListviewObject,
                                        MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
                                        MUIA_Listview_List, (IPTR)ListObject,
                                            MUIA_CycleChain, 1,
                                            InputListFrame,
                                            MUIA_List_DisplayHook, (IPTR)&nch->nch_ReportListDisplayHook,
                                            MUIA_List_AutoVisible, TRUE,
                                            End,
                                        End),
                                    Child, (IPTR)HGroup,
                                        Child, (IPTR)(nch->nch_FillDefObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                                            MUIA_Text_Contents, (IPTR)"\33c Fill defaults ",
                                            End),
                                        Child, (IPTR)(nch->nch_ClearActionsObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                                            MUIA_Text_Contents, (IPTR)"\33c Clear actions ",
                                            End),
                                        End,
                                    End,
                                Child, (IPTR)VGroup, GroupFrameT((IPTR)"Usage items"),
                                    Child, (IPTR)(nch->nch_ItemLVObj = ListviewObject,
                                        MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
                                        MUIA_Listview_List, (IPTR)ListObject,
                                            InputListFrame,
                                            MUIA_CycleChain, 1,
                                            MUIA_List_DisplayHook, (IPTR)&nch->nch_ItemListDisplayHook,
                                            MUIA_List_Format, (IPTR)barbarbarbar,
                                            MUIA_List_Title, TRUE,
                                            MUIA_List_AutoVisible, TRUE,
                                            End,
                                        End),
                                    End,
                                End,
                            Child, (IPTR)HGroup,
                                Child, (IPTR)(nch->nch_TrackEventsObj = ImageObject, ImageButtonFrame,
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
                                Child, (IPTR)(nch->nch_ReportValuesObj = ImageObject, ImageButtonFrame,
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
                                Child, (IPTR)(nch->nch_DisableActionsObj = ImageObject, ImageButtonFrame,
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
                                    Child, (IPTR)(nch->nch_ActionLVObj = ListviewObject,
                                        MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
                                        MUIA_Listview_List, (IPTR)ListObject,
                                            InputListFrame,
                                            MUIA_CycleChain, 1,
                                            MUIA_List_DisplayHook, (IPTR)&nch->nch_ActionListDisplayHook,
                                            MUIA_List_Format, (IPTR)barbar,
                                            MUIA_List_Title, TRUE,
                                            MUIA_List_AutoVisible, TRUE,
                                            End,
                                        End),
                                    Child, (IPTR)HGroup,
                                        Child, (IPTR)(nch->nch_ActionNewObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                                            MUIA_Text_Contents, (IPTR)"\33c New ",
                                            End),
                                        Child, (IPTR)(nch->nch_ActionCopyObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                                            MUIA_Text_Contents, (IPTR)"\33c Copy ",
                                            End),
                                        Child, (IPTR)(nch->nch_ActionDelObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                                            MUIA_Text_Contents, (IPTR)"\33c Del ",
                                            End),
                                        Child, (IPTR)(nch->nch_ActionUpObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                                            MUIA_Text_Contents, (IPTR)"\33c Up ",
                                            End),
                                        Child, (IPTR)(nch->nch_ActionDownObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                                            MUIA_Text_Contents, (IPTR)"\33c Down ",
                                            End),
                                        End,
                                    End,
                                Child, (IPTR)(nch->nch_ActionAreaObj = VGroup,
                                    MUIA_Disabled, TRUE,
                                    Child, (IPTR)HGroup,
                                        Child, (IPTR)(nch->nch_ActionSelectorObj = CycleObject,
                                            MUIA_CycleChain, 1,
                                            MUIA_Cycle_Entries, (IPTR)ActionTypeStrings,
                                            MUIA_Cycle_Active, 0,
                                            End),
                                        Child, (IPTR)Label((IPTR) "Trigger:"),
                                        Child, (IPTR)(nch->nch_ActionTriggerObj = CycleObject,
                                            MUIA_CycleChain, 1,
                                            MUIA_Cycle_Entries, (IPTR)ActionTriggerStrings,
                                            MUIA_Cycle_Active, 0,
                                            End),
                                        End,
                                    Child, (IPTR)HGroup,
                                        Child, (IPTR)Label((IPTR) "Opts:"),
                                        Child, (IPTR)(nch->nch_ActionAbsToRelObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_Toggle,
                                            MUIA_Text_Contents, (IPTR)"\33c Abs->Rel ",
                                            End),
                                        Child, (IPTR)(nch->nch_ActionClipEnableObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_Toggle,
                                            MUIA_Text_Contents, (IPTR)"\33c Clip ",
                                            End),
                                        Child, (IPTR)(nch->nch_ActionScaleEnableObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_Toggle,
                                            MUIA_Text_Contents, (IPTR)"\33c Scale ",
                                            End),
                                        Child, (IPTR)(nch->nch_ActionCCEnableObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_Toggle,
                                            MUIA_Text_Contents, (IPTR)"\33c CC ",
                                            End),
                                        Child, (IPTR)(nch->nch_ActionValEnableObj = TextObject, ButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_Toggle,
                                            MUIA_Text_Contents, (IPTR)"\33c Val ",
                                            End),
                                        End,
                                    Child, (IPTR)(nch->nch_A_ClipGroupObj = HGroup, GroupFrameT("Clipping"),
                                        MUIA_ShowMe, FALSE,
                                        Child, (IPTR)Label((IPTR) "Min:"),
                                        Child, (IPTR)(nch->nch_A_ClipMinObj = SliderObject, SliderFrame,
                                            MUIA_CycleChain, 1,
                                            MUIA_Numeric_Min, 0,
                                            MUIA_Numeric_Max, 100,
                                            MUIA_Numeric_Format, (IPTR)"%ld%%",
                                            End),
                                        Child, (IPTR)Label((IPTR) "Max:"),
                                        Child, (IPTR)(nch->nch_A_ClipMaxObj = SliderObject, SliderFrame,
                                            MUIA_CycleChain, 1,
                                            MUIA_Numeric_Min, 0,
                                            MUIA_Numeric_Max, 100,
                                            MUIA_Numeric_Format, (IPTR)"%ld%%",
                                            End),
                                        Child, (IPTR)Label((IPTR) "Stretch:"),
                                        Child, (IPTR)(nch->nch_A_ClipStretchObj = ImageObject, ImageButtonFrame,
                                            MUIA_Background, MUII_ButtonBack,
                                            MUIA_CycleChain, 1,
                                            MUIA_InputMode, MUIV_InputMode_Toggle,
                                            MUIA_Image_Spec, MUII_CheckMark,
                                            MUIA_Image_FreeVert, TRUE,
                                            //MUIA_Selected,
                                            MUIA_ShowSelState, FALSE,
                                            End),
                                        End),
                                    Child, (IPTR)(nch->nch_A_ScaleGroupObj = HGroup, GroupFrameT("Scaling"),
                                        MUIA_ShowMe, FALSE,
                                        Child, (IPTR)Label((IPTR) "Min:"),
                                        Child, (IPTR)(nch->nch_A_ScaleMinObj = StringObject,
                                            StringFrame,
                                            MUIA_String_Accept, (IPTR)"0123456789-",
                                            MUIA_CycleChain, 1,
                                            MUIA_String_AdvanceOnCR, TRUE,
                                            MUIA_String_MaxLen, 10,
                                            End),
                                        Child, (IPTR)Label((IPTR) "Max:"),
                                        Child, (IPTR)(nch->nch_A_ScaleMaxObj = StringObject,
                                            StringFrame,
                                            MUIA_String_Accept, (IPTR)"0123456789-",
                                            MUIA_CycleChain, 1,
                                            MUIA_String_AdvanceOnCR, TRUE,
                                            MUIA_String_MaxLen, 10,
                                            End),
                                        End),
                                    Child, (IPTR)(nch->nch_A_CCGroupObj = VGroup, GroupFrameT("Pre-condition code"),
                                        MUIA_ShowMe, FALSE,
                                        Child, (IPTR)HGroup,
                                            Child, (IPTR)Label((IPTR) "If"),
                                            Child, (IPTR)(nch->nch_A_CCVar1Obj = CycleObject,
                                                MUIA_CycleChain, 1,
                                                MUIA_Cycle_Entries, (IPTR)A_CCVariableStrings,
                                                End),
                                            Child, (IPTR)(nch->nch_A_CCCondObj = CycleObject,
                                                MUIA_CycleChain, 1,
                                                MUIA_Cycle_Entries, (IPTR)A_CCCondStrings,
                                                End),
                                            Child, (IPTR)(nch->nch_A_CCVar2Obj = CycleObject,
                                                MUIA_CycleChain, 1,
                                                MUIA_Cycle_Entries, (IPTR)A_CCVariableStrings,
                                                End),
                                            End,
                                        Child, (IPTR)HGroup,
                                            Child, (IPTR)Label((IPTR) "Left constant:"),
                                            Child, (IPTR)(nch->nch_A_CCConst1Obj = StringObject,
                                                StringFrame,
                                                MUIA_String_Accept, (IPTR)"0123456789-",
                                                MUIA_CycleChain, 1,
                                                MUIA_String_AdvanceOnCR, TRUE,
                                                MUIA_String_MaxLen, 10,
                                                End),
                                            Child, (IPTR)Label((IPTR) "Right constant:"),
                                            Child, (IPTR)(nch->nch_A_CCConst2Obj = StringObject,
                                                StringFrame,
                                                MUIA_String_Accept, (IPTR)"0123456789-",
                                                MUIA_CycleChain, 1,
                                                MUIA_String_AdvanceOnCR, TRUE,
                                                MUIA_String_MaxLen, 10,
                                                End),
                                            End,
                                        End),
                                    Child, (IPTR)(nch->nch_A_ValGroupObj = VGroup, GroupFrameT("Input value redirection"),
                                        MUIA_ShowMe, FALSE,
                                        Child, (IPTR)HGroup,
                                            Child, (IPTR)Label((IPTR) "Take value for action from"),
                                            Child, (IPTR)(nch->nch_A_ValVarObj = CycleObject,
                                                MUIA_CycleChain, 1,
                                                MUIA_Cycle_Entries, (IPTR)A_CCVariableStrings,
                                                End),
                                            End,
                                        Child, (IPTR)HGroup,
                                            Child, (IPTR)Label((IPTR) "Constant:"),
                                            Child, (IPTR)(nch->nch_A_ValConstObj = StringObject,
                                                StringFrame,
                                                MUIA_String_Accept, (IPTR)"0123456789-",
                                                MUIA_CycleChain, 1,
                                                MUIA_String_AdvanceOnCR, TRUE,
                                                MUIA_String_MaxLen, 10,
                                                End),
                                            End,
                                        End),
                                    Child, (IPTR)(nch->nch_ActionPageObj = VGroup,
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
                                                Child, (IPTR)(nch->nch_A_KeyQualOpObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_QualOpStrings,
                                                    End),
                                                Child, (IPTR)Label((IPTR) "qualifier"),
                                                Child, (IPTR)(nch->nch_A_KeyQualObj = CycleObject,
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
                                            Child, (IPTR)(nch->nch_A_RawKeyObj = ListviewObject,
                                                MUIA_Listview_Input, TRUE,
                                                MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
                                                MUIA_Listview_List, (IPTR)ListObject,
                                                    InputListFrame,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_List_SourceArray, (IPTR)nch->nch_RawKeyArray,
                                                    MUIA_List_AutoVisible, TRUE,
                                                    End,
                                                End),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)(nch->nch_A_RawKeyUpObj = ImageObject, ImageButtonFrame,
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
                                                Child, (IPTR)(nch->nch_A_VanillaStrObj = StringObject,
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
                                                Child, (IPTR)(nch->nch_A_KeyStringObj = StringObject,
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
                                                Child, (IPTR)(nch->nch_A_MousePosOpObj = CycleObject,
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
                                                Child, (IPTR)(nch->nch_A_MouseButOpObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_MouseButOpStrings,
                                                    End),
                                                Child, (IPTR)(nch->nch_A_MouseButObj = CycleObject,
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
                                                Child, (IPTR)(nch->nch_A_TabletAxisObj = CycleObject,
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
                                                Child, (IPTR)(nch->nch_A_JoypadOpObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_JoypadOpStrings,
                                                    End),
                                                Child, (IPTR)(nch->nch_A_JoypadFeatObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_JoypadFeatStrings,
                                                    End),
                                                Child, (IPTR)Label((IPTR) "on"),
                                                Child, (IPTR)(nch->nch_A_JoypadPortObj = CycleObject,
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
                                                Child, (IPTR)(nch->nch_A_APadFeatObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_APadFeatStrings,
                                                    End),
                                                Child, (IPTR)Label((IPTR) "of"),
                                                Child, (IPTR)(nch->nch_A_APadPortObj = CycleObject,
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
                                                Child, (IPTR)(nch->nch_A_WheelOpObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_WheelOpStrings,
                                                    End),
                                                Child, (IPTR)Label((IPTR) "event"),
                                                Child, (IPTR)HSpace(0),
                                                End,
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)Label((IPTR) "Distance:"),
                                                Child, (IPTR)(nch->nch_A_WheelDistObj = SliderObject, SliderFrame,
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
                                                    MUIA_Popstring_String, (IPTR)(nch->nch_A_SoundFileObj = StringObject,
                                                        StringFrame,
                                                        MUIA_CycleChain, 1,
                                                        MUIA_String_AdvanceOnCR, TRUE,
                                                        End),
                                                    MUIA_Popstring_Button, (IPTR)PopButton(MUII_PopFile),
                                                    ASLFR_TitleText, (IPTR)"Select sound file...",
                                                    End,
                                                Child, (IPTR)Label((IPTR) "Volume:"),
                                                Child, (IPTR)(nch->nch_A_SoundVolObj = SliderObject, SliderFrame,
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
                                                    MUIA_Popstring_String, (IPTR)(nch->nch_A_ShellComObj = StringObject,
                                                        StringFrame,
                                                        MUIA_CycleChain, 1,
                                                        MUIA_String_AdvanceOnCR, TRUE,
                                                        End),
                                                    MUIA_Popstring_Button, (IPTR)PopButton(MUII_PopFile),
                                                    ASLFR_TitleText, (IPTR) "Select an executable...",
                                                    End,
                                                Child, (IPTR)Label((IPTR) "ASync:"),
                                                Child, (IPTR)HGroup,
                                                    Child, (IPTR)(nch->nch_A_ShellAsyncObj = ImageObject, ImageButtonFrame,
                                                        MUIA_Background, MUII_ButtonBack,
                                                        MUIA_CycleChain, 1,
                                                        MUIA_InputMode, MUIV_InputMode_Toggle,
                                                        MUIA_Image_Spec, MUII_CheckMark,
                                                        MUIA_Image_FreeVert, TRUE,
                                                        MUIA_Selected, nch->nch_CDC->cdc_EnableRH,
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
                                            Child, (IPTR)(nch->nch_A_OutItemLVObj = ListviewObject,
                                                MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
                                                MUIA_Listview_List, (IPTR)ListObject,
                                                    InputListFrame,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_List_DisplayHook, (IPTR)&nch->nch_ItemListDisplayHook,
                                                    MUIA_List_Format, (IPTR)barbarbarbar,
                                                    MUIA_List_Title, TRUE,
                                                    MUIA_List_AutoVisible, TRUE,
                                                    End,
                                                End),
                                            //Child, (IPTR)VSpace(0),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)Label((IPTR) "Array values:"),
                                                Child, (IPTR)(nch->nch_A_OutArrayObj = StringObject,
                                                    StringFrame,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_String_AdvanceOnCR, TRUE,
                                                    MUIA_String_MaxLen, 256,
                                                    End),
                                                End,
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)Label((IPTR) "Operation:"),
                                                Child, (IPTR)(nch->nch_A_OutOpObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_OutOpStrings,
                                                    End),
                                                End,
                                            End,
                                        Child, (IPTR)VGroup, GroupFrameT("HID feature variables"), /* HUA_FEATURE */
                                            Child, (IPTR)(nch->nch_A_FeatItemLVObj = ListviewObject,
                                                MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
                                                MUIA_Listview_List, (IPTR)ListObject,
                                                    InputListFrame,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_List_DisplayHook, (IPTR)&nch->nch_ItemListDisplayHook,
                                                    MUIA_List_Format, (IPTR)barbarbarbar,
                                                    MUIA_List_Title, TRUE,
                                                    MUIA_List_AutoVisible, TRUE,
                                                    End,
                                                End),
                                            //Child, (IPTR)VSpace(0),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)Label((IPTR) "Array values:"),
                                                Child, (IPTR)(nch->nch_A_FeatArrayObj = StringObject,
                                                    StringFrame,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_String_AdvanceOnCR, TRUE,
                                                    MUIA_String_MaxLen, 256,
                                                    End),
                                                End,
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)Label((IPTR) "Operation:"),
                                                Child, (IPTR)(nch->nch_A_FeatOpObj = CycleObject,
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
                                                Child, (IPTR)(nch->nch_A_MiscOpObj = CycleObject,
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
                                                Child, (IPTR)(nch->nch_A_TarVarObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_TarVariableStrings,
                                                    End),
                                                Child, (IPTR)(nch->nch_A_TarVarOpObj = CycleObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Cycle_Entries, (IPTR)A_TarVarOpStrings,
                                                    End),
                                                End,
                                            Child, (IPTR)VSpace(0),
                                            End,
                                        Child, (IPTR)VGroup, GroupFrameT("Extended Raw key"), /* HUA_EXTRAWKEY */
                                            //Child, (IPTR)VSpace(0),
                                            Child, (IPTR)Label((IPTR) "\33cSelect key to send"),
                                            Child, (IPTR)(nch->nch_A_ExtRawKeyObj = ListviewObject,
                                                MUIA_Listview_Input, TRUE,
                                                MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
                                                MUIA_Listview_List, (IPTR)ListObject,
                                                    InputListFrame,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_List_SourceArray, (IPTR)nch->nch_ExtRawKeyArray,
                                                    MUIA_List_AutoVisible, TRUE,
                                                    End,
                                                End),
                                            Child, (IPTR)HGroup,
                                                Child, (IPTR)(nch->nch_A_ExtRawKeyUpObj = ImageObject, ImageButtonFrame,
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
                    Child, (IPTR)(nch->nch_UseObj = TextObject, ButtonFrame,
                        MUIA_ShowMe, (IPTR)nch->nch_Interface,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, (IPTR)"\33c Save ",
                        End),
                    Child, (IPTR)(nch->nch_SetDefaultObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, (IPTR)(nch->nch_Interface ? "\33c Save as Default " : "\33c Save Defaults "),
                        End),
                    Child, (IPTR)(nch->nch_CloseObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, (IPTR)"\33c Use ",
                        End),
                    End,
                End,
            End),
        End;

    if(!nch->nch_App)
    {
        KPRINTF(10, ("Couldn't create application\n"));
        GM_UNIQUENAME(nGUITaskCleanup)(nch);
        return;
    }

    {
        struct ActionData *ad = INST_DATA(nch->nch_ActionClass->mcc_Class, nch->nch_ActionObj);
        ad->ad_NCH = nch;
    }

    nch->nch_GUICurrentColl = NULL;
    nch->nch_GUICurrentItem = NULL;
    nch->nch_QuitGUI = FALSE;

    DoMethod(nch->nch_MainWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             nch->nch_ActionObj, 1, MUIM_Action_UseConfig);
    DoMethod(nch->nch_UseObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nch->nch_ActionObj, 1, MUIM_Action_StoreConfig);
    DoMethod(nch->nch_SetDefaultObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nch->nch_ActionObj, 1, MUIM_Action_DefaultConfig);
    DoMethod(nch->nch_CloseObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nch->nch_ActionObj, 1, MUIM_Action_UseConfig);

    DoMethod(nch->nch_AboutMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_About);
    DoMethod(nch->nch_UseMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_StoreConfig);
    DoMethod(nch->nch_SetDefaultMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_DefaultConfig);
    DoMethod(nch->nch_MUIPrefsMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_App, 2, MUIM_Application_OpenConfigWindow, 0);
    DoMethod(nch->nch_SwapLMBRMBMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_SwapLMBRMB);
    DoMethod(nch->nch_MouseAccel100MI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_ActionObj, 2, MUIM_Action_SetMouseAccel, 100);
    DoMethod(nch->nch_MouseAccel150MI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_ActionObj, 2, MUIM_Action_SetMouseAccel, 150);
    DoMethod(nch->nch_MouseAccel200MI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_ActionObj, 2, MUIM_Action_SetMouseAccel, 200);
    DoMethod(nch->nch_JoyPort0MI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_ActionObj, 2, MUIM_Action_SetJoyPort, 0);
    DoMethod(nch->nch_JoyPort1MI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_ActionObj, 2, MUIM_Action_SetJoyPort, 1);
    DoMethod(nch->nch_JoyPort2MI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_ActionObj, 2, MUIM_Action_SetJoyPort, 2);
    DoMethod(nch->nch_JoyPort3MI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_ActionObj, 2, MUIM_Action_SetJoyPort, 3);
    DoMethod(nch->nch_JoyAutofireMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_AddAutofire);

    DoMethod(nch->nch_DebugReportMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_DebugReport);

    DoMethod(nch->nch_EnableKBResetObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateDevPrefs);
    DoMethod(nch->nch_EnableRHObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateDevPrefs);
    DoMethod(nch->nch_ResetDelayObj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateDevPrefs);
    DoMethod(nch->nch_ShellStackObj, MUIM_Notify, MUIA_String_Integer, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateDevPrefs);
    DoMethod(nch->nch_TurboMouseObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateDevPrefs);

    DoMethod(nch->nch_HIDCtrlAutoObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateDevPrefs);
    DoMethod(nch->nch_HIDCtrlOpenObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nch->nch_ActionObj, 1, MUIM_Action_ShowHIDControl);

    for(count = 0; count < 4; count++)
    {
        DoMethod(nch->nch_LLPortModeObj[count], MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
                 nch->nch_ActionObj, 1, MUIM_Action_UpdateDevPrefs);
    }
    DoMethod(nch->nch_LLRumblePortObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateDevPrefs);

    DoMethod(nch->nch_USBKeymapLVObj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_KeymapSelectUSB);
    DoMethod(nch->nch_RawKeymapLVObj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_KeymapSelectRaw);
    DoMethod(nch->nch_RestoreDefKeymapObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nch->nch_ActionObj, 1, MUIM_Action_RestDefKeymap);
    DoMethod(nch->nch_TrackKeyEventsObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_SetTracking);

    DoMethod(nch->nch_ActionSelectorObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionPageObj, 3, MUIM_Set, MUIA_Group_ActivePage, MUIV_TriggerValue);

    DoMethod(nch->nch_ReportLVObj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_SelectReport);

    DoMethod(nch->nch_FillDefObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nch->nch_ActionObj, 1, MUIM_Action_FillDefReport);
    DoMethod(nch->nch_ClearActionsObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nch->nch_ActionObj, 1, MUIM_Action_ClearReport);

    DoMethod(nch->nch_TrackEventsObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_SetTracking);
    DoMethod(nch->nch_ReportValuesObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_SetTracking);
    DoMethod(nch->nch_DisableActionsObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_SetTracking);

    DoMethod(nch->nch_ItemLVObj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_SelectItem);

    DoMethod(nch->nch_ActionLVObj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_SelectAction);

    DoMethod(nch->nch_ActionNewObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nch->nch_ActionObj, 1, MUIM_Action_NewAction);
    DoMethod(nch->nch_ActionCopyObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nch->nch_ActionObj, 1, MUIM_Action_CopyAction);
    DoMethod(nch->nch_ActionDelObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nch->nch_ActionObj, 1, MUIM_Action_DelAction);
    DoMethod(nch->nch_ActionUpObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nch->nch_ActionObj, 1, MUIM_Action_MoveActionUp);
    DoMethod(nch->nch_ActionDownObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nch->nch_ActionObj, 1, MUIM_Action_MoveActionDown);

    DoMethod(nch->nch_ActionClipEnableObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nch->nch_A_ClipGroupObj, 3, MUIM_Set, MUIA_ShowMe, MUIV_TriggerValue);
    DoMethod(nch->nch_ActionScaleEnableObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nch->nch_A_ScaleGroupObj, 3, MUIM_Set, MUIA_ShowMe, MUIV_TriggerValue);
    DoMethod(nch->nch_ActionCCEnableObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nch->nch_A_CCGroupObj, 3, MUIM_Set, MUIA_ShowMe, MUIV_TriggerValue);
    DoMethod(nch->nch_ActionValEnableObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nch->nch_A_ValGroupObj, 3, MUIM_Set, MUIA_ShowMe, MUIV_TriggerValue);

    DoMethod(nch->nch_ActionSelectorObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_SetActionType);
    DoMethod(nch->nch_ActionTriggerObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);

    DoMethod(nch->nch_ActionAbsToRelObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nch->nch_ActionClipEnableObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nch->nch_ActionScaleEnableObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nch->nch_ActionCCEnableObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nch->nch_ActionValEnableObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAOptions);

    DoMethod(nch->nch_A_ClipMinObj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nch->nch_A_ClipMaxObj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nch->nch_A_ClipStretchObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAOptions);

    DoMethod(nch->nch_A_ScaleMinObj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nch->nch_A_ScaleMaxObj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAOptions);

    DoMethod(nch->nch_A_CCVar1Obj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nch->nch_A_CCCondObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nch->nch_A_CCVar2Obj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nch->nch_A_CCConst1Obj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nch->nch_A_CCConst2Obj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAOptions);

    DoMethod(nch->nch_A_ValVarObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAOptions);
    DoMethod(nch->nch_A_ValConstObj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAOptions);

    DoMethod(nch->nch_A_KeyQualObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_KeyQualOpObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_RawKeyObj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_RawKeyUpObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_VanillaStrObj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_KeyStringObj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_MousePosOpObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_MouseButOpObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_MouseButObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_TabletAxisObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_WheelOpObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_WheelDistObj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_JoypadOpObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_JoypadFeatObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_JoypadPortObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_APadFeatObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_APadPortObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_SoundFileObj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_SoundVolObj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_ShellComObj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_ShellAsyncObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_MiscOpObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_TarVarObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_TarVarOpObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_OutOpObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_OutArrayObj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_OutItemLVObj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_FeatOpObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_FeatArrayObj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_FeatItemLVObj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_ExtRawKeyObj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);
    DoMethod(nch->nch_A_ExtRawKeyUpObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nch->nch_ActionObj, 1, MUIM_Action_UpdateAction);

    if(nch->nch_Interface)
    {
        struct NepHidCollection *nhc;
        struct NepHidReport *nhr = (struct NepHidReport *) nch->nch_HidReports.lh_Head;
        struct NepHidItem *nhi;
        struct NepHidGItem *nhgi;
        Object *outobj;

        nch->nch_GUICurrentColl = NULL;
        while(nhr->nhr_Node.ln_Succ)
        {
            nhc = (struct NepHidCollection *) nhr->nhr_Collections.lh_Head;
            while(nhc->nhc_Node.ln_Succ)
            {
                if(nhc->nhc_Items.lh_Head->ln_Succ)
                {
                    DoMethod(nch->nch_ReportLVObj, MUIM_List_InsertSingle, nhc, MUIV_List_Insert_Bottom);
                    if(!nch->nch_GUICurrentColl)
                    {
                        nch->nch_GUICurrentColl = nhc;
                        set(nch->nch_ReportLVObj, MUIA_List_Active, MUIV_List_Active_Top);
                    }
                }
                nhi = (struct NepHidItem *) nhc->nhc_Items.lh_Head;
                while(nhi->nhi_Node.ln_Succ)
                {
                    if(nhi->nhi_Type == REPORT_MAIN_OUTPUT)
                    {
                        outobj = nch->nch_A_OutItemLVObj;
                    }
                    else if(nhi->nhi_Type == REPORT_MAIN_FEATURE)
                    {
                        outobj = nch->nch_A_FeatItemLVObj;
                    } else {
                        outobj = NULL;
                    }
                    if(outobj)
                    {
                        if(nhi->nhi_Flags & RPF_MAIN_VARIABLE)
                        {
                            if((nhgi = nAllocGOutItem(nch, nhi, &nhi->nhi_ActionList, nhi->nhi_Usage)))
                            {
                                DoMethod(outobj, MUIM_List_InsertSingle, nhgi, MUIV_List_Insert_Bottom);
                            }
                        } else {
                            STRPTR newstr;
                            //ULONG acount;
                            //if(nhi->nhi_SameUsages)
                            {
                                if((nhgi = nAllocGOutItem(nch, nhi, &nhi->nhi_ActionList, nhi->nhi_Usage)))
                                {
                                    if((newstr = psdCopyStrFmt("%s Array [%ld]", nhgi->nhgi_Name, nhi->nhi_Count)))
                                    {
                                        psdFreeVec(nhgi->nhgi_Name);
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
                                    if((nhgi = nAllocGOutItem(nch, nhi, &nhi->nhi_ActionMap[acount], nhi->nhi_UsageMap[acount])))
                                    {
                                        if((newstr = psdCopyStrFmt(" +%s", nhgi->nhgi_Name)))
                                        {
                                            psdFreeVec(nhgi->nhgi_Name);
                                            nhgi->nhgi_Name = newstr;
                                        }

                                        DoMethod(outobj, MUIM_List_InsertSingle, nhgi, MUIV_List_Insert_Bottom);
                                    }
                                } while(++acount < (nhi->nhi_LogicalMax-nhi->nhi_LogicalMin+1));
                            }
#endif
                        }
                    }
                    nhi = (struct NepHidItem *) nhi->nhi_Node.ln_Succ;
                }
                nhc = (struct NepHidCollection *) nhc->nhc_Node.ln_Succ;
            }
            nhr = (struct NepHidReport *) nhr->nhr_Node.ln_Succ;
        }
    }

    {
        IPTR  isopen = 0;
        IPTR  iconify = 0;
        ULONG sigs;
        ULONG sigmask;
        LONG retid;

        get(nch->nch_App, MUIA_Application_Iconified, &iconify);
        set(nch->nch_MainWindow, MUIA_Window_Open, TRUE);
        get(nch->nch_MainWindow, MUIA_Window_Open, &isopen);
        if(!(isopen || iconify))
        {
            GM_UNIQUENAME(nGUITaskCleanup)(nch);
            return;
        }
        nch->nch_TrackingSignal = AllocSignal(-1);
        sigmask = (1<<nch->nch_TrackingSignal);
        do
        {
            retid = DoMethod(nch->nch_App, MUIM_Application_NewInput, &sigs);
            if(sigs)
            {
                sigs = Wait(sigs | sigmask | SIGBREAKF_CTRL_C);
                if(nch->nch_TrackEvents && (sigs & sigmask))
                {
                    BOOL track;
                    ULONG count;
                    struct NepHidItem *titem;
                    struct List *talist;
                    struct NepHidCollection *nhc;
                    struct NepHidGItem *nhgi;

                    talist = nch->nch_LastItemAList;
                    if((titem = nch->nch_LastItem))
                    {
                        track = TRUE;
                        if(nch->nch_GUICurrentItem)
                        {
                            if(nch->nch_GUICurrentItem->nhgi_ActionList == talist)
                            {
                                track = FALSE;
                            }
                        }
                        if(track)
                        {
                            if(nch->nch_GUICurrentColl == titem->nhi_Collection)
                            {
                                /* Already the right collection, find item */
                                count = 0;
                                nhgi = NULL;
                                do
                                {
                                    DoMethod(nch->nch_ItemLVObj, MUIM_List_GetEntry, count, &nhgi);
                                    if(!nhgi)
                                    {
                                        break;
                                    }
                                    if(nhgi->nhgi_ActionList == talist)
                                    {
                                        /* Heureka! */
                                        set(nch->nch_ItemLVObj, MUIA_List_Active, count);
                                        DoMethod(nch->nch_ItemLVObj, MUIM_List_Jump, count);
                                        nch->nch_LastItemAList = NULL;
                                        nch->nch_LastItem = NULL;
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
                                    DoMethod(nch->nch_ReportLVObj, MUIM_List_GetEntry, count, &nhc);
                                    if(!nhc)
                                    {
                                        break;
                                    }
                                    if(nhc == titem->nhi_Collection)
                                    {
                                        /* Heureka! */
                                        set(nch->nch_ReportLVObj, MUIA_List_Active, count);
                                        DoMethod(nch->nch_ReportLVObj, MUIM_List_Jump, count);
                                        break;
                                    }
                                    count++;
                                } while(TRUE);
                            }
                        }
                    }
                }
                if(nch->nch_TrackKeyEvents && (sigs & sigmask))
                {
                    if((nch->nch_CurrUSBKey != nch->nch_LastUSBKey) && nch->nch_LastUSBKey)
                    {
                        hum = (struct HidUsageIDMap *) hidusage07;
                        count = 0;
                        while(hum->hum_String)
                        {
                            if(hum->hum_ID == (nch->nch_LastUSBKey & 0xffff))
                            {
                                set(nch->nch_USBKeymapLVObj, MUIA_List_Active, count);
                                break;
                            }
                            hum++;
                            count++;
                        }
                        nch->nch_CurrUSBKey = nch->nch_LastUSBKey;
                    }
                }

                if(nch->nch_ReportValues && (sigs & sigmask) && nch->nch_ItemChanged)
                {
                    DoMethod(nch->nch_ItemLVObj, MUIM_List_Redraw, MUIV_List_Redraw_Active);
                    nch->nch_ItemChanged = FALSE;
                }

                if(sigs & SIGBREAKF_CTRL_C)
                {
                    break;
                }
            }
        } while((!nch->nch_QuitGUI) && (retid != MUIV_Application_ReturnID_Quit));
        FreeSignal(nch->nch_TrackingSignal);
        nch->nch_TrackingSignal = -1;
        set(nch->nch_MainWindow, MUIA_Window_Open, FALSE);
    }
    GM_UNIQUENAME(nGUITaskCleanup)(nch);

    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nGUITaskCleanup()" */
void GM_UNIQUENAME(nGUITaskCleanup)(struct NepClassHid *nch)
{
    UWORD count;
    struct NepHidGItem *nhgi;

    if(nch->nch_App)
    {
        MUI_DisposeObject(nch->nch_App);
        nch->nch_App = NULL;
    }

    for(count = 0; count < 128; count++)
    {
        psdFreeVec(nch->nch_RawKeyArray[count]);
        nch->nch_RawKeyArray[count] = NULL;
        psdFreeVec(nch->nch_ExtRawKeyArray[count]);
        nch->nch_ExtRawKeyArray[count] = NULL;
    }
    nhgi = (struct NepHidGItem *) nch->nch_GUIItems.lh_Head;
    while(nhgi->nhgi_Node.ln_Succ)
    {
        nFreeGItem(nch, nhgi);
        nhgi = (struct NepHidGItem *) nch->nch_GUIItems.lh_Head;
    }
    nhgi = (struct NepHidGItem *) nch->nch_GUIOutItems.lh_Head;
    while(nhgi->nhgi_Node.ln_Succ)
    {
        nFreeGItem(nch, nhgi);
        nhgi = (struct NepHidGItem *) nch->nch_GUIOutItems.lh_Head;
    }
    if(nch->nch_ActionClass)
    {
        MUI_DeleteCustomClass(nch->nch_ActionClass);
        nch->nch_ActionClass = NULL;
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
    if(ps)
    {
        CloseLibrary(ps);
        ps = NULL;
    }
    Forbid();
    nch->nch_GUITask = NULL;
    if(nch->nch_ReadySigTask)
    {
        Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
    }
    --nch->nch_ClsBase->nh_Library.lib_OpenCnt;
}
/* \\\ */

/* /// "nAllocGItem()" */
struct NepHidGItem * nAllocGItem(struct NepClassHid *nch, struct NepHidItem *nhi, struct List *actionlist, ULONG usageid)
{
    struct NepHidGItem *nhgi;

    if(!(nhgi = psdAllocVec(sizeof(struct NepHidGItem))))
    {
        return(NULL);
    }
    nhgi->nhgi_Item = nhi;
    nhgi->nhgi_ActionList = actionlist;
    if(usageid)
    {
        nhgi->nhgi_Name = nGetUsageName(nch, usageid);
    }
    AddTail(&nch->nch_GUIItems, &nhgi->nhgi_Node);
    return(nhgi);
}
/* \\\ */

/* /// "nAllocGOutItem()" */
struct NepHidGItem * nAllocGOutItem(struct NepClassHid *nch, struct NepHidItem *nhi, struct List *actionlist, ULONG usageid)
{
    struct NepHidGItem *nhgi;

    if(!(nhgi = psdAllocVec(sizeof(struct NepHidGItem))))
    {
        return(NULL);
    }
    nhgi->nhgi_Item = nhi;
    nhgi->nhgi_ActionList = actionlist;
    if(usageid)
    {
        nhgi->nhgi_Name = nGetUsageName(nch, usageid);
    }
    AddTail(&nch->nch_GUIOutItems, &nhgi->nhgi_Node);
    return(nhgi);
}
/* \\\ */

/* /// "nFreeGItem()" */
void nFreeGItem(struct NepClassHid *nch, struct NepHidGItem *nhgi)
{
    if(nhgi)
    {
        Remove(&nhgi->nhgi_Node);
        psdFreeVec(nhgi->nhgi_Name);
        psdFreeVec(nhgi);
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
          AROS_UFHA(struct NepHidCollection *, nhc, A1))
{
    AROS_USERFUNC_INIT

    struct NepClassHid *nch = (struct NepClassHid *) hook->h_Data;

    if(nhc)
    {
        if(!nhc->nhc_Parent)
        {
            *strarr = nhc->nhc_Name;
        } else {
            STRPTR srcptr;
            STRPTR tarptr;
            *strarr = tarptr = nch->nch_TmpStrBufReport;
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
          AROS_UFHA(struct NepHidGItem *, nhgi, A1))
{
    AROS_USERFUNC_INIT

    struct NepClassHid *nch = (struct NepClassHid *) hook->h_Data;
    struct NepHidItem *nhi;
    STRPTR buf;
    ULONG flags;
    if(nhgi)
    {
        nhi = nhgi->nhgi_Item;
        psdSafeRawDoFmt(nch->nch_TmpStrBufItem, 10, "%ld", nhi->nhi_LogicalMin);
        psdSafeRawDoFmt(&nch->nch_TmpStrBufItem[10], 10, "%ld", nhi->nhi_LogicalMax);
        buf = &nch->nch_TmpStrBufItem[30];
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
            psdSafeRawDoFmt(&nch->nch_TmpStrBufItem[20], 10, "%ld", nhi->nhi_OldValue);
            *strarr++ = &nch->nch_TmpStrBufItem[20];
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
        *strarr++ = nch->nch_TmpStrBufItem;
        *strarr++ = &nch->nch_TmpStrBufItem[10];
        *strarr = &nch->nch_TmpStrBufItem[30];
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
          AROS_UFHA(struct NepHidAction *, nha, A1))
{
    AROS_USERFUNC_INIT

    struct NepClassHid *nch = (struct NepClassHid *) hook->h_Data;
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

                psdSafeRawDoFmt(nch->nch_TmpStrBufAction, 80, "%s %s", p1str, A_QualifierStrings[nha->nha_Qualifier]);
                *strarr = nch->nch_TmpStrBufAction;
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
                psdSafeRawDoFmt(*strarr = nch->nch_TmpStrBufAction, 80, "%s %s", p1str, p2str);
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
                p1str = A_JoypadOpStrings[nRevLookup(nha->nha_JoypadOp, 0, A_JoypadOpVals)];
                p2str = A_JoypadFeatStrings[nRevLookup(nha->nha_JoypadFeat, 0, A_JoypadFeatVals)];
                psdSafeRawDoFmt(*strarr = nch->nch_TmpStrBufAction, 80, "%s %s (port %ld)", p1str, p2str, (ULONG) nha->nha_JoypadPort);
                break;

            case HUA_ANALOGJOY:
                p1str = A_APadFeatStrings[nRevLookup(nha->nha_APadFeat, 0, A_APadFeatVals)];
                psdSafeRawDoFmt(*strarr = nch->nch_TmpStrBufAction, 80, "Set %s (port %ld)", p1str, (ULONG) nha->nha_JoypadPort);
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
                psdSafeRawDoFmt(*strarr = nch->nch_TmpStrBufAction, 80, "%s %s",
                                (nha->nha_RawKey & IECODE_UP_PREFIX) ? "keyup" : "keydown",
                                nch->nch_RawKeyArray[nha->nha_RawKey & (~IECODE_UP_PREFIX)]);
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
                struct NepHidItem *nhi;
                ULONG dummy;
                BOOL freeit = FALSE;

                p1str = A_OutOpStrings[nRevLookup(nha->nha_OutOp, 0, A_OutOpVals)];
                p2str = "???";
                if((nhi = nFindItemID(nch, nha->nha_OutItem, REPORT_MAIN_OUTPUT, &dummy)))
                {
                    p2str = nGetUsageName(nch, nhi->nhi_Usage);
                    freeit =  TRUE;
                    if(!(nhi->nhi_Flags & RPF_MAIN_VARIABLE))
                    {
                        p1str = nha->nha_OutArray;
                    }
                }
                psdSafeRawDoFmt(*strarr = nch->nch_TmpStrBufAction, 80, "%s %s", p1str, p2str);
                if(freeit)
                {
                    psdFreeVec(p2str);
                }
                break;
            }

            case HUA_FEATURE:
            {
                struct NepHidItem *nhi;
                ULONG dummy;
                BOOL freeit = FALSE;

                p1str = A_OutOpStrings[nRevLookup(nha->nha_FeatOp, 0, A_OutOpVals)];
                p2str = "???";
                if((nhi = nFindItemID(nch, nha->nha_FeatItem, REPORT_MAIN_FEATURE, &dummy)))
                {
                    p2str = nGetUsageName(nch, nhi->nhi_Usage);
                    freeit =  TRUE;
                    if(!(nhi->nhi_Flags & RPF_MAIN_VARIABLE))
                    {
                        p1str = nha->nha_OutArray;
                    }
                }
                psdSafeRawDoFmt(*strarr = nch->nch_TmpStrBufAction, 80, "%s %s", p1str, p2str);
                if(freeit)
                {
                    psdFreeVec(p2str);
                }
                break;
            }

            case HUA_MISC:
                *strarr = A_MiscOpStrings[nRevLookup(nha->nha_MiscMode, 0, A_MiscOpVals)];
                break;

            case HUA_VARIABLES:
                p1str = A_TarVariableStrings[nRevLookup(nha->nha_TarVar, 0, A_TarVariableVals)];
                p2str = A_TarVarOpStrings[nRevLookup(nha->nha_TarVarOp, 0, A_TarVarOpVals)];
                psdSafeRawDoFmt(*strarr = nch->nch_TmpStrBufAction, 80, "%s %s", p1str, p2str);
                break;

            case HUA_EXTRAWKEY:
                psdSafeRawDoFmt(*strarr = nch->nch_TmpStrBufAction, 80, "%s %s",
                                (nha->nha_RawKey & IECODE_UP_PREFIX) ? "keyup" : "keydown",
                                nch->nch_ExtRawKeyArray[nha->nha_RawKey & (~IECODE_UP_PREFIX)]);
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
    struct NepClassHid *nch = NULL;
    struct HidUsageIDMap *hum;
    ULONG tmpval;
    STRPTR tmpstr;
    ULONG count;

    if(msg->MethodID != OM_NEW)
    {
        ad = INST_DATA(cl, obj);
        nch = ad->ad_NCH;
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
            struct PsdIFFContext *pic;
            struct PsdIFFContext *rppic;
            struct NepHidReport *nhr;
            struct NepHidCollection *nhc;
            struct NepHidItem *nhi;
            struct List *alistptr;
            ULONG *usageptr;
            ULONG count;
            ULONG newform[3];

            DoMethod(nch->nch_ActionObj, MUIM_Action_UpdateDevPrefs);
            DoMethod(nch->nch_ActionObj, MUIM_Action_UpdateAction);
            DoMethod(nch->nch_ActionObj, MUIM_Action_UpdateAOptions);

            if(msg->MethodID == MUIM_Action_DefaultConfig)
            {
                pic = psdGetClsCfg(GM_UNIQUENAME(libname));
                if(!pic)
                {
                    psdSetClsCfg(GM_UNIQUENAME(libname), NULL);
                    pic = psdGetClsCfg(GM_UNIQUENAME(libname));
                }
                if(pic)
                {
                    psdAddCfgEntry(pic, nch->nch_CDC);
                    psdAddCfgEntry(pic, &nch->nch_KeymapCfg);
                    psdSaveCfgToDisk(NULL, FALSE);
                }
            }
            if(nch->nch_Interface)
            {
                pic = psdGetUsbDevCfg(GM_UNIQUENAME(libname), nch->nch_DevIDString, nch->nch_IfIDString);
                if(!pic)
                {
                    psdSetUsbDevCfg(GM_UNIQUENAME(libname), nch->nch_DevIDString, nch->nch_IfIDString, NULL);
                    pic = psdGetUsbDevCfg(GM_UNIQUENAME(libname), nch->nch_DevIDString, nch->nch_IfIDString);
                }
                if(pic)
                {
                    psdAddCfgEntry(pic, nch->nch_CDC);
                    if(memcmp(&nch->nch_KeymapCfg, &nch->nch_ClsBase->nh_DummyNCH.nch_KeymapCfg, sizeof(struct KeymapCfg)))
                    {
                        psdAddCfgEntry(pic, &nch->nch_KeymapCfg);
                    } else {
                        psdRemCfgChunk(pic, AROS_LONG2BE(nch->nch_KeymapCfg.kmc_ChunkID));
                    }

                    /* Create config file */
                    newform[0] = AROS_LONG2BE(ID_FORM);
                    newform[1] = AROS_LONG2BE(4);
                    nhr = (struct NepHidReport *) nch->nch_HidReports.lh_Head;
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

                        rppic = psdFindCfgForm(pic, AROS_LONG2BE(newform[2]));
                        if(!rppic)
                        {
                            rppic = psdAddCfgEntry(pic, newform);
                            if(!rppic)
                            {
                                break;
                            }
                        } else {
                            psdRemCfgChunk(rppic, 0);
                        }

                        /* find out, if we can get rid of defaults before saving */
                        nhc = (struct NepHidCollection *) nhr->nhr_Collections.lh_Head;
                        while(nhc->nhc_Node.ln_Succ)
                        {
                            nhi = (struct NepHidItem *) nhc->nhc_Items.lh_Head;
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
                                    nCheckForDefaultAction(nch, nhi, alistptr++, nhc, *usageptr++);
                                } while(--count);
                                nhi = (struct NepHidItem *) nhi->nhi_Node.ln_Succ;
                            }
                            nhc = (struct NepHidCollection *) nhc->nhc_Node.ln_Succ;
                        }

                        nhc = (struct NepHidCollection *) nhr->nhr_Collections.lh_Head;
                        while(nhc->nhc_Node.ln_Succ)
                        {
                            nhi = (struct NepHidItem *) nhc->nhc_Items.lh_Head;
                            while(nhi->nhi_Node.ln_Succ)
                            {
                                nSaveItem(nch, rppic, &nhi->nhi_ActionList, nhr->nhr_ItemIDBase);
                                if((alistptr = nhi->nhi_ActionMap))
                                {
                                    count = nhi->nhi_MapSize;
                                    do
                                    {
                                        nSaveItem(nch, rppic, alistptr++, nhr->nhr_ItemIDBase);
                                    } while(--count);
                                }
                                nhi = (struct NepHidItem *) nhi->nhi_Node.ln_Succ;
                            }
                            nhc = (struct NepHidCollection *) nhc->nhc_Node.ln_Succ;
                        }
                        nhr = (struct NepHidReport *) nhr->nhr_Node.ln_Succ;
                    }
                    if(msg->MethodID != MUIM_Action_UseConfig)
                    {
                        psdSaveCfgToDisk(NULL, FALSE);
                    }
                    nch->nch_QuitGUI = TRUE;
                }
            } else {
                nch->nch_QuitGUI = TRUE;
            }
            return(TRUE);
        }

        case MUIM_Action_About:
            MUI_RequestA(nch->nch_App, nch->nch_MainWindow, 0, NULL, "Blimey!", VERSION_STRING, NULL);
            return(TRUE);

        case MUIM_Action_UpdateDevPrefs:
        {
            CONST_STRPTR tmpstr;
            tmpstr = "";
            get(nch->nch_ConWindowObj, MUIA_String_Contents, &tmpstr);
            strncpy(nch->nch_CDC->cdc_ShellCon, tmpstr, 127);

            get(nch->nch_EnableKBResetObj, MUIA_Selected, &nch->nch_CDC->cdc_EnableKBReset);
            get(nch->nch_EnableRHObj, MUIA_Selected, &nch->nch_CDC->cdc_EnableRH);
            get(nch->nch_ResetDelayObj, MUIA_Numeric_Value, &nch->nch_CDC->cdc_ResetDelay);
            get(nch->nch_ShellStackObj, MUIA_String_Integer, &nch->nch_CDC->cdc_ShellStack);
            get(nch->nch_TurboMouseObj, MUIA_Cycle_Active, &nch->nch_CDC->cdc_TurboMouse);

            get(nch->nch_HIDCtrlAutoObj, MUIA_Selected, &nch->nch_CDC->cdc_HIDCtrlOpen);
            tmpstr = "";
            get(nch->nch_HIDCtrlRexxObj, MUIA_String_Contents, &tmpstr);
            strncpy(nch->nch_CDC->cdc_HIDCtrlRexx, tmpstr, 31);
            tmpstr = "";
            get(nch->nch_HIDCtrlTitleObj, MUIA_String_Contents, &tmpstr);
            strncpy(nch->nch_CDC->cdc_HIDCtrlTitle, tmpstr, 31);

            for(count = 0; count < 4; count++)
            {
                get(nch->nch_LLPortModeObj[count], MUIA_Cycle_Active, &nch->nch_CDC->cdc_LLPortMode[count]);
            }
            get(nch->nch_LLRumblePortObj, MUIA_Cycle_Active, &nch->nch_CDC->cdc_LLRumblePort);

            if(nch->nch_EPInPipe)
            {
                psdSetAttrs(PGA_PIPE, nch->nch_EPInPipe,
                            PPA_Interval, nch->nch_CDC->cdc_TurboMouse ? 1<<nch->nch_CDC->cdc_TurboMouse : nch->nch_EPInInterval,
                            TAG_END);
            }

            return(TRUE);
        }

        case MUIM_Action_SelectReport:
        {
            struct NepHidGItem *nhgi = NULL;
            struct NepHidItem *nhi = NULL;
            ULONG acount;
            STRPTR newstr;
            STRPTR idstr1, idstr2;
            ULONG pos;
            ULONG jumppos = 0;

            nch->nch_SilentActionUpdate = TRUE;
            DoMethod(nch->nch_ReportLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nch->nch_GUICurrentColl);
            set(nch->nch_ItemLVObj, MUIA_List_Quiet, TRUE);
            DoMethod(nch->nch_ItemLVObj, MUIM_List_Clear);
            nhgi = (struct NepHidGItem *) nch->nch_GUIItems.lh_Head;
            while(nhgi->nhgi_Node.ln_Succ)
            {
                nFreeGItem(nch, nhgi);
                nhgi = (struct NepHidGItem *) nch->nch_GUIItems.lh_Head;
            }
            nch->nch_GUICurrentItem = NULL;
            if(nch->nch_GUICurrentColl)
            {
                pos = 0;
                nhi = (struct NepHidItem *) nch->nch_GUICurrentColl->nhc_Items.lh_Head;
                while(nhi->nhi_Node.ln_Succ)
                {
                    if(nhi->nhi_Type == REPORT_MAIN_INPUT)
                    {
                        if(nhi->nhi_Flags & RPF_MAIN_VARIABLE)
                        {
                            if((nhgi = nAllocGItem(nch, nhi, &nhi->nhi_ActionList, nhi->nhi_Usage)))
                            {
                                DoMethod(nch->nch_ItemLVObj, MUIM_List_InsertSingle, nhgi, MUIV_List_Insert_Bottom);
                                if((!nch->nch_GUICurrentItem) ||
                                   (nch->nch_TrackEvents && (nhgi->nhgi_ActionList == nch->nch_LastItemAList)))
                                {
                                    nch->nch_GUICurrentItem = nhgi;
                                    jumppos = pos;
                                }
                                pos++;
                            }
                        } else {
                            if((nhgi = nAllocGItem(nch, nhi, &nhi->nhi_ActionList, 0)))
                            {
                                idstr1 = nGetUsageName(nch, nhi->nhi_UsageMap[0]);
                                if(nhi->nhi_SameUsages)
                                {
                                    nhgi->nhgi_Name = psdCopyStrFmt("%s Array", idstr1);
                                } else {
                                    idstr2 = nGetUsageName(nch, nhi->nhi_UsageMap[nhi->nhi_LogicalMax-nhi->nhi_LogicalMin]);
                                    nhgi->nhgi_Name = psdCopyStrFmt("Default for %s->%s", idstr1, idstr2);
                                    psdFreeVec(idstr2);
                                }
                                psdFreeVec(idstr1);
                                DoMethod(nch->nch_ItemLVObj, MUIM_List_InsertSingle, nhgi, MUIV_List_Insert_Bottom);
                                if((!nch->nch_GUICurrentItem) ||
                                   (nch->nch_TrackEvents && (nhgi->nhgi_ActionList == nch->nch_LastItemAList)))
                                {
                                    nch->nch_GUICurrentItem = nhgi;
                                    jumppos = pos;
                                }
                                pos++;
                            }
                            if(!nhi->nhi_SameUsages)
                            {
                                acount = 0;
                                do
                                {
                                    if((nhgi = nAllocGItem(nch, nhi, &nhi->nhi_ActionMap[acount], nhi->nhi_UsageMap[acount])))
                                    {
                                        if((newstr = psdCopyStrFmt(" +%s", nhgi->nhgi_Name)))
                                        {
                                            psdFreeVec(nhgi->nhgi_Name);
                                            nhgi->nhgi_Name = newstr;
                                        }

                                        DoMethod(nch->nch_ItemLVObj, MUIM_List_InsertSingle, nhgi, MUIV_List_Insert_Bottom);
                                        if((!nch->nch_GUICurrentItem) ||
                                           (nch->nch_TrackEvents && (nhgi->nhgi_ActionList == nch->nch_LastItemAList)))
                                        {
                                            nch->nch_GUICurrentItem = nhgi;
                                            jumppos = pos;
                                        }
                                        pos++;
                                    }
                                } while(++acount < (nhi->nhi_LogicalMax-nhi->nhi_LogicalMin+1));
                            }
                        }
                    }
                    nhi = (struct NepHidItem *) nhi->nhi_Node.ln_Succ;
                }
                set(nch->nch_FillDefObj, MUIA_Disabled, FALSE);
                set(nch->nch_ClearActionsObj, MUIA_Disabled, FALSE);
                set(nch->nch_ItemLVObj, MUIA_Disabled, FALSE);
                set(nch->nch_ItemLVObj, MUIA_List_Quiet, FALSE);
                nch->nch_SilentActionUpdate = FALSE;
                set(nch->nch_ItemLVObj, MUIA_List_Active, jumppos);
                DoMethod(nch->nch_ItemLVObj, MUIM_List_Jump, jumppos);
            } else {
                set(nch->nch_FillDefObj, MUIA_Disabled, TRUE);
                set(nch->nch_ClearActionsObj, MUIA_Disabled, TRUE);
                nch->nch_SilentActionUpdate = FALSE;
                set(nch->nch_ItemLVObj, MUIA_Disabled, TRUE);
                set(nch->nch_ItemLVObj, MUIA_List_Quiet, FALSE);
            }
            if(!nch->nch_GUICurrentAction)
            {
                DoMethod(nch->nch_ActionObj, MUIM_Action_SelectAction);
            }
            return(TRUE);
        }

        case MUIM_Action_DebugReport:
            if(nch->nch_GUICurrentColl)
            {
                nDebugReport(nch, nch->nch_GUICurrentColl->nhc_Report);
            }
            return(TRUE);

        case MUIM_Action_SelectItem:
        {
            struct NepHidGItem *nhgi;
            struct NepHidAction *nha;

            nch->nch_SilentActionUpdate = TRUE;
            DoMethod(nch->nch_ItemLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nch->nch_GUICurrentItem);
            set(nch->nch_ActionLVObj, MUIA_List_Quiet, TRUE);
            DoMethod(nch->nch_ActionLVObj, MUIM_List_Clear);
            nch->nch_GUICurrentAction = NULL;
            nch->nch_SilentActionUpdate = FALSE;
            if((nhgi = nch->nch_GUICurrentItem))
            {
                nha = (struct NepHidAction *) nhgi->nhgi_ActionList->lh_Head;
                while(nha->nha_Node.ln_Succ)
                {
                    DoMethod(nch->nch_ActionLVObj, MUIM_List_InsertSingle, nha, MUIV_List_Insert_Bottom);
                    if(!nch->nch_GUICurrentAction)
                    {
                        nch->nch_GUICurrentAction = nha;
                        set(nch->nch_ActionLVObj, MUIA_List_Active, MUIV_List_Active_Top);
                    }
                    nha = (struct NepHidAction *) nha->nha_Node.ln_Succ;
                }
                set(nch->nch_ActionLVObj, MUIA_Disabled, FALSE);
                set(nch->nch_ActionNewObj, MUIA_Disabled, FALSE);
                //set(nch->nch_ActionLVObj, MUIA_List_Active, MUIV_List_Active_Off);
            } else {
                set(nch->nch_ActionLVObj, MUIA_Disabled, TRUE);
                set(nch->nch_ActionNewObj, MUIA_Disabled, TRUE);
                set(nch->nch_ActionCopyObj, MUIA_Disabled, TRUE);
                set(nch->nch_ActionDelObj, MUIA_Disabled, TRUE);
                set(nch->nch_ActionUpObj, MUIA_Disabled, TRUE);
                set(nch->nch_ActionDownObj, MUIA_Disabled, TRUE);
            }
            if(!nch->nch_GUICurrentAction)
            {
                DoMethod(nch->nch_ActionObj, MUIM_Action_SelectAction);
            }
            set(nch->nch_ActionLVObj, MUIA_List_Quiet, FALSE);

            return(TRUE);
        }

        case MUIM_Action_SelectAction:
        {
            struct NepHidAction *nha;
            if(!nch->nch_SilentActionUpdate)
            {
                DoMethod(nch->nch_ActionLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nch->nch_GUICurrentAction);
                if((nha = nch->nch_GUICurrentAction))
                {
                    set(nch->nch_ActionSelectorObj, MUIA_Cycle_Active, nha->nha_Type & HUA_ATYPEMASK);
                    set(nch->nch_ActionAreaObj, MUIA_Disabled, FALSE);
                    set(nch->nch_ActionCopyObj, MUIA_Disabled, FALSE);
                    set(nch->nch_ActionDelObj, MUIA_Disabled, FALSE);
                    set(nch->nch_ActionUpObj, MUIA_Disabled, FALSE);
                    set(nch->nch_ActionDownObj, MUIA_Disabled, FALSE);
                    DoMethod(nch->nch_ActionObj, MUIM_Action_SetActionType);
                } else {
                    set(nch->nch_ActionAreaObj, MUIA_Disabled, TRUE);
                    set(nch->nch_ActionCopyObj, MUIA_Disabled, TRUE);
                    set(nch->nch_ActionDelObj, MUIA_Disabled, TRUE);
                    set(nch->nch_ActionUpObj, MUIA_Disabled, TRUE);
                    set(nch->nch_ActionDownObj, MUIA_Disabled, TRUE);
                }
            }
            return(TRUE);
        }

        case MUIM_Action_FillDefReport:
        {
            struct NepHidItem *nhi;
            struct NepHidCollection *nhc;
            ULONG count;
            struct List *alistptr;
            ULONG *usageptr;

            DoMethod(nch->nch_ReportLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nhc);
            if(nhc)
            {
                if(MUI_Request(nch->nch_App, nch->nch_MainWindow, 0, NULL, "Fill with default|Cancel",
                               "Warning! This operation will erase\n"
                               "all of the actions defined for\n"
                               "the selected collection '%s'\n"
                               "replace it with default values.", nhc->nhc_Name))
                {
                    DoMethod(nch->nch_ActionLVObj, MUIM_List_Clear);
                    Forbid();
                    nCleanCollection(nch, nhc);
                    nhi = (struct NepHidItem *) nhc->nhc_Items.lh_Head;
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
                            nDetectDefaultAction(nch, nhi, alistptr++, nhc, *usageptr++);
                        } while(--count);
                        nhi = (struct NepHidItem *) nhi->nhi_Node.ln_Succ;
                    }
                    Permit();
                    nch->nch_GUICurrentAction = NULL;
                    set(nch->nch_ItemLVObj, MUIA_List_Active, MUIV_List_Active_Off);
                    set(nch->nch_ItemLVObj, MUIA_List_Active, MUIV_List_Active_Top);
                }
            }
            return(TRUE);
        }

        case MUIM_Action_SwapLMBRMB:
        case MUIM_Action_SetJoyPort:
        {
            struct NepHidItem *nhi;
            struct NepHidCollection *nhc;
            struct NepHidAction *nha;
            ULONG count;
            struct List *alistptr;

            DoMethod(nch->nch_ReportLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nhc);
            if(nhc)
            {
                nhi = (struct NepHidItem *) nhc->nhc_Items.lh_Head;
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
                        nha = (struct NepHidAction *) alistptr->lh_Head;
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
                            nha = (struct NepHidAction *) nha->nha_Node.ln_Succ;
                        }
                        alistptr++;
                    } while(--count);
                    nhi = (struct NepHidItem *) nhi->nhi_Node.ln_Succ;
                }
                DoMethod(nch->nch_ActionObj, MUIM_Action_SelectItem);
            }

            return(TRUE);
        }

        case MUIM_Action_ClearReport:
        {
            struct NepHidCollection *nhc;

            DoMethod(nch->nch_ReportLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nhc);
            if(nhc)
            {
                if(MUI_Request(nch->nch_App, nch->nch_MainWindow, 0, NULL, "Clear|Cancel",
                               "Warning! This operation will erase\n"
                               "all of the actions defined for\n"
                               "the selected collection '%s'\n"
                               "replace them with default values.", nhc->nhc_Name))
                {
                    DoMethod(nch->nch_ActionLVObj, MUIM_List_Clear);
                    Forbid();
                    nCleanCollection(nch, nhc);
                    Permit();
                    nch->nch_GUICurrentAction = NULL;
                    set(nch->nch_ItemLVObj, MUIA_List_Active, MUIV_List_Active_Off);
                    set(nch->nch_ItemLVObj, MUIA_List_Active, MUIV_List_Active_Top);
                }
            }
            return(TRUE);
        }

        case MUIM_Action_SetTracking:
        {
            ULONG state = 0;
            get(nch->nch_TrackKeyEventsObj, MUIA_Selected, &state);
            nch->nch_TrackKeyEvents = state;
            get(nch->nch_TrackEventsObj, MUIA_Selected, &state);
            nch->nch_TrackEvents = state;
            get(nch->nch_ReportValuesObj, MUIA_Selected, &state);
            nch->nch_ReportValues = state;
            get(nch->nch_DisableActionsObj, MUIA_Selected, &state);
            nch->nch_DisableActions = state;
            return(TRUE);
        }

        case MUIM_Action_NewAction:
        {
            struct NepHidAction *nha;
            if(nch->nch_GUICurrentItem)
            {
                Forbid();
                nha = nAllocAction(nch, nch->nch_GUICurrentItem->nhgi_ActionList, HUA_NOP|HUA_DOWNEVENT);
                Permit();
                if(nha)
                {
                    DoMethod(nch->nch_ActionLVObj, MUIM_List_InsertSingle, nha, MUIV_List_Insert_Bottom);
                    set(nch->nch_ActionLVObj, MUIA_List_Active, MUIV_List_Active_Bottom);
                }
            }
            return(TRUE);
        }

        case MUIM_Action_CopyAction:
        {
            struct NepHidAction *nha;
            struct NepHidAction *newnha;
            DoMethod(nch->nch_ActionLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nch->nch_GUICurrentAction);
            if((nha = nch->nch_GUICurrentAction))
            {
                newnha = psdAllocVec(sizeof(struct NepHidAction));
                if(newnha)
                {
                    *newnha = *nha;
                    Forbid();
                    AddTail(nch->nch_GUICurrentItem->nhgi_ActionList, &newnha->nha_Node);
                    Permit();
                    DoMethod(nch->nch_ActionLVObj, MUIM_List_InsertSingle, newnha, MUIV_List_Insert_Bottom);
                    set(nch->nch_ActionLVObj, MUIA_List_Active, MUIV_List_Active_Bottom);
                }
            }
            return(TRUE);
        }

        case MUIM_Action_DelAction:
        {
            struct NepHidAction *nha;
            DoMethod(nch->nch_ActionLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nch->nch_GUICurrentAction);
            if((nha = nch->nch_GUICurrentAction))
            {
                nch->nch_GUICurrentAction = NULL;
                DoMethod(nch->nch_ActionLVObj, MUIM_List_Remove, MUIV_List_Remove_Active);
                Forbid();
                Remove(&nha->nha_Node);
                psdFreeVec(nha);
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
            DoMethod(nch->nch_ActionLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nha);
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
                    DoMethod(nch->nch_ActionLVObj, MUIM_List_Move, MUIV_List_Move_Active, MUIV_List_Move_Previous);
                    set(nch->nch_ActionLVObj, MUIA_List_Active, MUIV_List_Active_Up);
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
            DoMethod(nch->nch_ActionLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nha);
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
                    DoMethod(nch->nch_ActionLVObj, MUIM_List_Move, MUIV_List_Move_Active, MUIV_List_Move_Next);
                    set(nch->nch_ActionLVObj, MUIA_List_Active, MUIV_List_Active_Down);
                }
            }
            return(TRUE);
        }

        case MUIM_Action_SetActionType:
        {
            struct NepHidAction *nha;
            if((nha = nch->nch_GUICurrentAction))
            {
                tmpval = 0;
                get(nch->nch_ActionSelectorObj, MUIA_Cycle_Active, &tmpval);
                if(tmpval != (nha->nha_Type & HUA_ATYPEMASK))
                {
                    nha->nha_IsDefault = FALSE;
                }
                nha->nha_Type = (nha->nha_Type & HUA_TRIGMASK) | tmpval;
                nnset(nch->nch_ActionTriggerObj, MUIA_Cycle_Active, nRevLookup(nha->nha_Type & HUA_TRIGMASK, 0, ActionTriggerVals));

                nnset(nch->nch_ActionAbsToRelObj, MUIA_Selected, nha->nha_AbsToRel);
                nnset(nch->nch_ActionClipEnableObj, MUIA_Selected, nha->nha_ClipEnable);
                nnset(nch->nch_A_ClipMinObj, MUIA_Numeric_Value, nha->nha_ClipMin);
                nnset(nch->nch_A_ClipMaxObj, MUIA_Numeric_Value, nha->nha_ClipMax);
                nnset(nch->nch_A_ClipStretchObj, MUIA_Selected, nha->nha_ClipStretch);

                nnset(nch->nch_ActionScaleEnableObj, MUIA_Selected, nha->nha_ScaleEnable);
                psdSafeRawDoFmt(nch->nch_TmpStrBuf0, 80, "%ld", nha->nha_ScaleMin);
                nnset(nch->nch_A_ScaleMinObj, MUIA_String_Contents, nch->nch_TmpStrBuf0);
                psdSafeRawDoFmt(nch->nch_TmpStrBuf0, 80, "%ld", nha->nha_ScaleMax);
                nnset(nch->nch_A_ScaleMaxObj, MUIA_String_Contents, nch->nch_TmpStrBuf0);

                nnset(nch->nch_ActionCCEnableObj, MUIA_Selected, nha->nha_CCEnable);
                nnset(nch->nch_A_CCVar1Obj, MUIA_Cycle_Active, nRevLookup(nha->nha_CCVar1, 0, A_CCVariableVals));
                nnset(nch->nch_A_CCCondObj, MUIA_Cycle_Active, nRevLookup(nha->nha_CCCond, 0, A_CCCondVals));
                nnset(nch->nch_A_CCVar2Obj, MUIA_Cycle_Active, nRevLookup(nha->nha_CCVar2, 0, A_CCVariableVals));
                psdSafeRawDoFmt(nch->nch_TmpStrBuf0, 80, "%ld", nha->nha_CCConst1);
                nnset(nch->nch_A_CCConst1Obj, MUIA_String_Contents, nch->nch_TmpStrBuf0);
                psdSafeRawDoFmt(nch->nch_TmpStrBuf0, 80, "%ld", nha->nha_CCConst2);
                nnset(nch->nch_A_CCConst2Obj, MUIA_String_Contents, nch->nch_TmpStrBuf0);

                nnset(nch->nch_ActionValEnableObj, MUIA_Selected, nha->nha_ValEnable);
                nnset(nch->nch_A_ValVarObj, MUIA_Cycle_Active, nRevLookup(nha->nha_ValVar, 0, A_CCVariableVals));
                psdSafeRawDoFmt(nch->nch_TmpStrBuf0, 80, "%ld", nha->nha_ValConst);
                nnset(nch->nch_A_ValConstObj, MUIA_String_Contents, nch->nch_TmpStrBuf0);

                nnset(nch->nch_A_ClipGroupObj, MUIA_ShowMe, nha->nha_ClipEnable);
                nnset(nch->nch_A_ScaleGroupObj, MUIA_ShowMe, nha->nha_ScaleEnable);
                nnset(nch->nch_A_CCGroupObj, MUIA_ShowMe, nha->nha_CCEnable);
                nnset(nch->nch_A_ValGroupObj, MUIA_ShowMe, nha->nha_ValEnable);

                nnset(nch->nch_A_KeyQualOpObj, MUIA_Cycle_Active, nRevLookup(nha->nha_QualMode, 0, A_QualOpVals));
                nnset(nch->nch_A_KeyQualObj, MUIA_Cycle_Active, nha->nha_Qualifier);
                nnset(nch->nch_A_RawKeyUpObj, MUIA_Selected, nha->nha_RawKey & IECODE_UP_PREFIX);
                nnset(nch->nch_A_RawKeyObj, MUIA_List_Active, nha->nha_RawKey & (~IECODE_UP_PREFIX));
                nnset(nch->nch_A_VanillaStrObj, MUIA_String_Contents, nha->nha_VanillaString);
                nnset(nch->nch_A_KeyStringObj, MUIA_String_Contents, nha->nha_KeyString);
                nnset(nch->nch_A_MousePosOpObj, MUIA_Cycle_Active, nRevLookup(nha->nha_MouseAxis, 0, A_MousePosOpVals));
                nnset(nch->nch_A_MouseButOpObj, MUIA_Cycle_Active, nRevLookup(nha->nha_ButtonMode, 0, A_MouseButOpVals));
                nnset(nch->nch_A_MouseButObj, MUIA_Cycle_Active, nha->nha_ButtonNo ? nha->nha_ButtonNo-1 : 0);
                nnset(nch->nch_A_TabletAxisObj, MUIA_Cycle_Active, nRevLookup(nha->nha_TabletAxis, 0, A_TabletAxisVals));
                nnset(nch->nch_A_WheelOpObj, MUIA_Cycle_Active, nRevLookup(nha->nha_WheelMode, 0, A_WheelOpVals));
                nnset(nch->nch_A_WheelDistObj, MUIA_Numeric_Value, nha->nha_WheelDist);
                nnset(nch->nch_A_JoypadOpObj, MUIA_Cycle_Active, nRevLookup(nha->nha_JoypadOp, 0, A_JoypadOpVals));
                nnset(nch->nch_A_JoypadFeatObj, MUIA_Cycle_Active, nRevLookup(nha->nha_JoypadFeat, 0, A_JoypadFeatVals));
                nnset(nch->nch_A_JoypadPortObj, MUIA_Cycle_Active, nha->nha_JoypadPort);
                nnset(nch->nch_A_APadFeatObj, MUIA_Cycle_Active, nRevLookup(nha->nha_APadFeat, 0, A_APadFeatVals));
                nnset(nch->nch_A_APadPortObj, MUIA_Cycle_Active, nha->nha_JoypadPort);
                nnset(nch->nch_A_SoundFileObj, MUIA_String_Contents, nha->nha_SoundFile);
                nnset(nch->nch_A_SoundVolObj, MUIA_Numeric_Value, nha->nha_SoundVolume);
                nnset(nch->nch_A_ShellComObj, MUIA_String_Contents, nha->nha_ExeString);
                nnset(nch->nch_A_ShellAsyncObj, MUIA_Selected, nha->nha_ShellAsync);
                nnset(nch->nch_A_OutOpObj, MUIA_Cycle_Active, nRevLookup(nha->nha_OutOp, 0, A_OutOpVals));
                nnset(nch->nch_A_OutArrayObj, MUIA_String_Contents, nha->nha_OutArray);
                {
                    struct NepHidItem *nhi;
                    struct NepHidGItem *nhgi;
                    ULONG pos;
                    if((nhi = nFindItemID(nch, nha->nha_OutItem, REPORT_MAIN_OUTPUT, &pos)))
                    {
                        pos = 0;
                        do
                        {
                            nhgi = NULL;
                            DoMethod(nch->nch_A_OutItemLVObj, MUIM_List_GetEntry, pos, &nhgi);
                            if(!nhgi)
                            {
                                break;
                            }
                            if(nhgi->nhgi_Item == nhi)
                            {
                                nnset(nch->nch_A_OutItemLVObj, MUIA_List_Active, pos);
                                break;
                            }
                            pos++;
                        }
                        while(TRUE);
                    }
                }
                nnset(nch->nch_A_FeatOpObj, MUIA_Cycle_Active, nRevLookup(nha->nha_FeatOp, 0, A_OutOpVals));
                nnset(nch->nch_A_FeatArrayObj, MUIA_String_Contents, nha->nha_OutArray);
                {
                    struct NepHidItem *nhi;
                    struct NepHidGItem *nhgi;
                    ULONG pos;
                    if((nhi = nFindItemID(nch, nha->nha_FeatItem, REPORT_MAIN_FEATURE, &pos)))
                    {
                        pos = 0;
                        do
                        {
                            nhgi = NULL;
                            DoMethod(nch->nch_A_FeatItemLVObj, MUIM_List_GetEntry, pos, &nhgi);
                            if(!nhgi)
                            {
                                break;
                            }
                            if(nhgi->nhgi_Item == nhi)
                            {
                                nnset(nch->nch_A_FeatItemLVObj, MUIA_List_Active, pos);
                                break;
                            }
                            pos++;
                        }
                        while(TRUE);
                    }
                }
                nnset(nch->nch_A_MiscOpObj, MUIA_Cycle_Active, nRevLookup(nha->nha_MiscMode, 0, A_MiscOpVals));

                nnset(nch->nch_A_TarVarObj, MUIA_Cycle_Active, nRevLookup(nha->nha_TarVar, 0, A_TarVariableVals));
                nnset(nch->nch_A_TarVarOpObj, MUIA_Cycle_Active, nRevLookup(nha->nha_TarVarOp, 0, A_TarVarOpVals));

                nnset(nch->nch_A_ExtRawKeyUpObj, MUIA_Selected, nha->nha_RawKey & IECODE_UP_PREFIX);
                nnset(nch->nch_A_ExtRawKeyObj, MUIA_List_Active, nha->nha_RawKey & (~IECODE_UP_PREFIX));

                DoMethod(nch->nch_ActionObj, MUIM_Action_UpdateAction);
                DoMethod(nch->nch_ActionObj, MUIM_Action_UpdateAOptions);
                DoMethod(nch->nch_ActionLVObj, MUIM_List_Redraw, MUIV_List_Redraw_Active);
            }
            return(TRUE);
        }

        case MUIM_Action_UpdateAOptions:
        {
            struct NepHidAction *nha;

            if((nha = nch->nch_GUICurrentAction))
            {
                nha->nha_IsDefault = FALSE;
                tmpval = 0;
                get(nch->nch_ActionAbsToRelObj, MUIA_Selected, &tmpval);
                nha->nha_AbsToRel = tmpval;

                tmpval = 0;
                get(nch->nch_ActionClipEnableObj, MUIA_Selected, &tmpval);
                nha->nha_ClipEnable = tmpval;
                if(tmpval)
                {
                    tmpval = 0;
                    get(nch->nch_A_ClipMinObj, MUIA_Numeric_Value, &tmpval);
                    nha->nha_ClipMin = tmpval;
                    tmpval = 0;
                    get(nch->nch_A_ClipMaxObj, MUIA_Numeric_Value, &tmpval);
                    nha->nha_ClipMax = tmpval;
                    tmpval = 0;
                    get(nch->nch_A_ClipStretchObj, MUIA_Selected, &tmpval);
                    nha->nha_ClipStretch = tmpval;
                }

                tmpval = 0;
                get(nch->nch_ActionScaleEnableObj, MUIA_Selected, &tmpval);
                nha->nha_ScaleEnable = tmpval;
                if(tmpval)
                {
                    tmpval = 0;
                    get(nch->nch_A_ScaleMinObj, MUIA_String_Integer, &tmpval);
                    nha->nha_ScaleMin = tmpval;
                    tmpval = 0;
                    get(nch->nch_A_ScaleMaxObj, MUIA_String_Integer, &tmpval);
                    nha->nha_ScaleMax = tmpval;
                }

                tmpval = 0;
                get(nch->nch_ActionCCEnableObj, MUIA_Selected, &tmpval);
                nha->nha_CCEnable = tmpval;
                if(tmpval)
                {
                    tmpval = 0;
                    get(nch->nch_A_CCVar1Obj, MUIA_Cycle_Active, &tmpval);
                    nha->nha_CCVar1 = A_CCVariableVals[tmpval];
                    tmpval = 0;
                    get(nch->nch_A_CCCondObj, MUIA_Cycle_Active, &tmpval);
                    nha->nha_CCCond = A_CCCondVals[tmpval];
                    tmpval = 0;
                    get(nch->nch_A_CCVar2Obj, MUIA_Cycle_Active, &tmpval);
                    nha->nha_CCVar2 = A_CCVariableVals[tmpval];
                    tmpval = 0;
                    get(nch->nch_A_CCConst1Obj, MUIA_String_Integer, &tmpval);
                    nha->nha_CCConst1 = tmpval;
                    tmpval = 0;
                    get(nch->nch_A_CCConst2Obj, MUIA_String_Integer, &tmpval);
                    nha->nha_CCConst2 = tmpval;
                }

                tmpval = 0;
                get(nch->nch_ActionValEnableObj, MUIA_Selected, &tmpval);
                nha->nha_ValEnable = tmpval;
                if(tmpval)
                {
                    tmpval = 0;
                    get(nch->nch_A_ValVarObj, MUIA_Cycle_Active, &tmpval);
                    nha->nha_ValVar = A_CCVariableVals[tmpval];
                    tmpval = 0;
                    get(nch->nch_A_ValConstObj, MUIA_String_Integer, &tmpval);
                    nha->nha_ValConst = tmpval;
                }
            }
            return(TRUE);
        }

        case MUIM_Action_UpdateAction:
        {
            struct NepHidAction *nha;
            struct NepHidGItem *nhgi;

            if((nha = nch->nch_GUICurrentAction))
            {
                nha->nha_IsDefault = FALSE;
                tmpval = 0;
                get(nch->nch_ActionTriggerObj, MUIA_Cycle_Active, &tmpval);
                nha->nha_Type = (nha->nha_Type & HUA_ATYPEMASK) | ActionTriggerVals[tmpval];
                switch(nha->nha_Type & HUA_ATYPEMASK)
                {
                    case HUA_QUALIFIER:
                        tmpval = 0;
                        get(nch->nch_A_KeyQualOpObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_QualMode = A_QualOpVals[tmpval];
                        tmpval = 0;
                        get(nch->nch_A_KeyQualObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_Qualifier = tmpval;
                        break;

                    case HUA_RAWKEY:
                        tmpval = 0;
                        get(nch->nch_A_RawKeyUpObj, MUIA_Selected, &tmpval);
                        if(tmpval)
                        {
                            tmpval = 0;
                            get(nch->nch_A_RawKeyObj, MUIA_List_Active, &tmpval);
                            nha->nha_RawKey = tmpval|IECODE_UP_PREFIX;
                        } else {
                            tmpval = 0;
                            get(nch->nch_A_RawKeyObj, MUIA_List_Active, &tmpval);
                            nha->nha_RawKey = tmpval;
                        }
                        break;

                    case HUA_VANILLA:
                        tmpstr = "";
                        get(nch->nch_A_VanillaStrObj, MUIA_String_Contents, &tmpstr);
                        strncpy(nha->nha_VanillaString, tmpstr, 79);
                        break;

                    case HUA_KEYSTRING:
                        tmpstr = "";
                        get(nch->nch_A_KeyStringObj, MUIA_String_Contents, &tmpstr);
                        strncpy(nha->nha_KeyString, tmpstr, 79);
                        break;

                    case HUA_MOUSEPOS:
                        tmpval = 0;
                        get(nch->nch_A_MousePosOpObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_MouseAxis = A_MousePosOpVals[tmpval];
                        break;

                    case HUA_BUTTONS:
                        tmpval = 0;
                        get(nch->nch_A_MouseButOpObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_ButtonMode = A_MouseButOpVals[tmpval];
                        tmpval = 0;
                        get(nch->nch_A_MouseButObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_ButtonNo = tmpval+1;
                        break;

                    case HUA_TABLET:
                        tmpval = 0;
                        get(nch->nch_A_TabletAxisObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_TabletAxis = A_TabletAxisVals[tmpval];
                        break;

                    case HUA_WHEEL:
                        tmpval = 0;
                        get(nch->nch_A_WheelOpObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_WheelMode = A_WheelOpVals[tmpval];
                        tmpval = 0;
                        get(nch->nch_A_WheelDistObj, MUIA_Numeric_Value, &tmpval);
                        nha->nha_WheelDist = tmpval;
                        break;

                    case HUA_DIGJOY:
                        tmpval = 0;
                        get(nch->nch_A_JoypadOpObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_JoypadOp = A_JoypadOpVals[tmpval];
                        tmpval = 0;
                        get(nch->nch_A_JoypadFeatObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_JoypadFeat = A_JoypadFeatVals[tmpval];
                        tmpval = 0;
                        get(nch->nch_A_JoypadPortObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_JoypadPort = tmpval;
                        break;

                    case HUA_ANALOGJOY:
                        tmpval = 0;
                        get(nch->nch_A_APadFeatObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_APadFeat = A_APadFeatVals[tmpval];
                        tmpval = 0;
                        get(nch->nch_A_APadPortObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_JoypadPort = tmpval;
                        break;

                    case HUA_SOUND:
                        tmpstr = "";
                        get(nch->nch_A_SoundFileObj, MUIA_String_Contents, &tmpstr);
                        strncpy(nha->nha_SoundFile, tmpstr, 255);
                        tmpval = 0;
                        get(nch->nch_A_SoundVolObj, MUIA_Numeric_Value, &tmpval);
                        nha->nha_SoundVolume = tmpval;
                        break;

                    case HUA_SHELL:
                        tmpstr = "";
                        get(nch->nch_A_ShellComObj, MUIA_String_Contents, &tmpstr);
                        strncpy(nha->nha_ExeString, tmpstr, 79);
                        tmpval = 0;
                        get(nch->nch_A_ShellAsyncObj, MUIA_Selected, &tmpval);
                        nha->nha_ShellAsync = tmpval;
                        break;

                    case HUA_OUTPUT:
                        DoMethod(nch->nch_A_OutItemLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nhgi);
                        tmpval = 0;
                        get(nch->nch_A_OutOpObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_OutOp = A_OutOpVals[tmpval];
                        tmpstr = "";
                        get(nch->nch_A_OutArrayObj, MUIA_String_Contents, &tmpstr);
                        strncpy(nha->nha_OutArray, tmpstr, 255);
                        if(nhgi)
                        {
                            nha->nha_OutItem = GET_WTYPE(nhgi->nhgi_ActionList);
                            if(nhgi->nhgi_Item->nhi_Flags & RPF_MAIN_VARIABLE)
                            {
                                set(nch->nch_A_OutArrayObj, MUIA_Disabled, TRUE);
                                set(nch->nch_A_OutOpObj, MUIA_Disabled, FALSE);
                            } else {
                                set(nch->nch_A_OutArrayObj, MUIA_Disabled, FALSE);
                                set(nch->nch_A_OutOpObj, MUIA_Disabled, TRUE);
                            }
                        } else {
                            nha->nha_OutItem = 0;
                        }
                        break;

                    case HUA_FEATURE:
                        DoMethod(nch->nch_A_FeatItemLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &nhgi);
                        tmpval = 0;
                        get(nch->nch_A_FeatOpObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_FeatOp = A_OutOpVals[tmpval];
                        tmpstr = "";
                        get(nch->nch_A_FeatArrayObj, MUIA_String_Contents, &tmpstr);
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
                        get(nch->nch_A_MiscOpObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_MiscMode = A_MiscOpVals[tmpval];
                        break;

                    case HUA_VARIABLES:
                        tmpval = 0;
                        get(nch->nch_A_TarVarObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_TarVar = A_TarVariableVals[tmpval];
                        tmpval = 0;
                        get(nch->nch_A_TarVarOpObj, MUIA_Cycle_Active, &tmpval);
                        nha->nha_TarVarOp = A_TarVarOpVals[tmpval];
                        break;

                   case HUA_EXTRAWKEY:
                        tmpval = 0;
                        get(nch->nch_A_ExtRawKeyUpObj, MUIA_Selected, &tmpval);
                        if(tmpval)
                        {
                            tmpval = 0;
                            get(nch->nch_A_ExtRawKeyObj, MUIA_List_Active, &tmpval);
                            nha->nha_RawKey = tmpval|IECODE_UP_PREFIX;
                        } else {
                            tmpval = 0;
                            get(nch->nch_A_ExtRawKeyObj, MUIA_List_Active, &tmpval);
                            nha->nha_RawKey = tmpval;
                        }
                        break;

                }
                DoMethod(nch->nch_ActionLVObj, MUIM_List_Redraw, MUIV_List_Redraw_Active);
            }
            return(TRUE);
        }

        case MUIM_Action_KeymapSelectUSB:
            DoMethod(nch->nch_USBKeymapLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &hum);
            if(hum)
            {
                nch->nch_CurrUSBKey = hum->hum_ID | 0x070000;
                set(nch->nch_RawKeymapLVObj, MUIA_List_Active, nch->nch_KeymapCfg.kmc_Keymap[hum->hum_ID]);
            }
            return(TRUE);

        case MUIM_Action_KeymapSelectRaw:
            DoMethod(nch->nch_USBKeymapLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &hum);
            if(hum)
            {
                tmpval = 0;
                get(nch->nch_RawKeymapLVObj, MUIA_List_Active, &tmpval);
                nch->nch_KeymapCfg.kmc_Keymap[hum->hum_ID] = tmpval;
            }
            return(TRUE);

        case MUIM_Action_RestDefKeymap:
            CopyMemQuick(usbkeymap, nch->nch_KeymapCfg.kmc_Keymap, 256);
            DoMethod(nch->nch_USBKeymapLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &hum);
            if(hum)
            {
                nch->nch_CurrUSBKey = hum->hum_ID | 0x070000;
                set(nch->nch_RawKeymapLVObj, MUIA_List_Active, nch->nch_KeymapCfg.kmc_Keymap[hum->hum_ID]);
            }
            return(TRUE);

        case MUIM_Action_ShowHIDControl:
            Forbid();
            if(nch->nch_HCApp)
            {
                DoMethod(nch->nch_HCApp, MUIM_Application_PushMethod,
                         nch->nch_HCActionObj, 1, MUIM_Action_ShowHIDControl);
            }
            Permit();
            return(TRUE);
    }
    return(DoSuperMethodA(cl,obj,msg));

    AROS_USERFUNC_EXIT
}
/* \\\ */

