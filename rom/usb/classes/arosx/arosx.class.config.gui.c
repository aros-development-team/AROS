/*
    Copyright © 2018-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Gamepad (XInput) USB class driver
    Lang: English
*/

#include "debug.h"
#include "arosx.class.h"

#include "arosx.class.config.gui.h"

#include "include/arosx.h"
#include <proto/arosx.h>

#undef ps
#undef MUIMasterBase
#define ps PsdBase
#define MUIMasterBase MUIBase

/* /// "nGUITask()" */
AROS_UFH0(void, nGUITask)
{
    AROS_USERFUNC_INIT

    struct Task *thistask;
    struct AROSXClassBase *arosxb;
    struct AROSXClassController *arosxc;

    struct Library *MUIBase;
    struct Library *PsdBase;
    struct Library *AROSXBase;

    struct AROSXClassConfigGUI *gui;

    thistask = FindTask(NULL);
    arosxc = thistask->tc_UserData;
    arosxb = arosxc->arosxb;

    struct AROSX_GAMEPAD *arosx_gamepad;
    arosx_gamepad = &arosxc->arosx_gamepad;

    /*
        TODO: Make use of OpenLibrary/CloseLibrary to keep track of lib open count...
    */
    ++arosxb->Library.lib_OpenCnt;

    struct AROSX_EventHook *arosx_eventhook;
    struct MsgPort         *arosx_eventport;

    if(gui = AllocVec(sizeof(struct AROSXClassConfigGUI), MEMF_CLEAR|MEMF_ANY)) {

        if(AROSXBase = OpenLibrary("arosx.library", 0)) {
            mybug(-1,("[AROSXClass GUI] arosx.library openened\n"));

            arosx_eventport = CreateMsgPort();
            arosx_eventhook = AROSX_AddEventHandler(arosx_eventport, (((1<<arosxc->id))<<28));
            /*
                Set to listen every controller
            */
            //arosx_eventhook = AROSX_AddEventHandler(arosx_eventport, (0xf<<28));

        if((MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)))
        {
            if((ps = OpenLibrary("poseidon.library", 4)))
            {

                gui->App = ApplicationObject,
                MUIA_Application_Title      , (IPTR)libname,
                MUIA_Application_Version    , (IPTR)VERSION_STRING,
                MUIA_Application_Copyright  , (IPTR)"©2018-2019 The AROS Development Team",
                MUIA_Application_Author     , (IPTR)"The AROS Development Team",
                MUIA_Application_Description, (IPTR)"Settings for the arosx.class",
                MUIA_Application_Base       , (IPTR)"AROSX",
                MUIA_Application_HelpFile   , (IPTR)"HELP:Poseidon.guide",
                MUIA_Application_Menustrip  , (IPTR)MenustripObject,
                    Child, (IPTR)MenuObjectT((IPTR)"Project"),
                        Child, (IPTR)(gui->AboutMI = MenuitemObject,
                            MUIA_Menuitem_Title, (IPTR)"About...",
                            MUIA_Menuitem_Shortcut, (IPTR)"?",
                            End),
                        End,
                    Child, (IPTR)MenuObjectT((IPTR)"Settings"),
                        Child, (IPTR)(gui->UseMI = MenuitemObject,
                            MUIA_Menuitem_Title, (IPTR)"Save",
                            MUIA_Menuitem_Shortcut, (IPTR)"S",
                            End),
                        Child, (IPTR)MenuitemObject,
                            MUIA_Menuitem_Title, (IPTR)NM_BARLABEL,
                            End,
                        Child, (IPTR)(gui->MUIPrefsMI = MenuitemObject,
                            MUIA_Menuitem_Title, (IPTR)"MUI Settings",
                            MUIA_Menuitem_Shortcut, (IPTR)"M",
                            End),
                        End,
                    End,

                SubWindow, (IPTR)(gui->MainWindow = WindowObject,
                    MUIA_Window_ID   , MAKE_ID('M','A','I','N'),
                    MUIA_Window_Title, (IPTR)arosxc->name,
                    MUIA_HelpNode, (IPTR)libname,

                    WindowContents, (IPTR)VGroup,
                        Child, (IPTR)(gui->GamepadGroupObject = ColGroup(2),
                            GroupFrameT("Gamepad"),
                            MUIA_Disabled, TRUE,

                            Child, (IPTR)HGroup,
                            Child, (IPTR)(gui->GamepadObject_button_a = ImageObject,
                                MUIA_Image_FontMatch, TRUE,
                                MUIA_Selected, FALSE,
                                MUIA_ShowSelState, FALSE,
                                MUIA_Image_Spec, MUII_RadioButton,
                                MUIA_Frame, MUIV_Frame_None,
                                End),

                            Child, (IPTR)(gui->GamepadObject_button_b = ImageObject,
                                MUIA_Image_FontMatch, TRUE,
                                MUIA_Selected, FALSE,
                                MUIA_ShowSelState, FALSE,
                                MUIA_Image_Spec, MUII_RadioButton,
                                MUIA_Frame, MUIV_Frame_None,
                                End),

                            Child, (IPTR)(gui->GamepadObject_button_x = ImageObject,
                                MUIA_Image_FontMatch, TRUE,
                                MUIA_Selected, FALSE,
                                MUIA_ShowSelState, FALSE,
                                MUIA_Image_Spec, MUII_RadioButton,
                                MUIA_Frame, MUIV_Frame_None,
                                End),

                            Child, (IPTR)(gui->GamepadObject_button_y = ImageObject,
                                MUIA_Image_FontMatch, TRUE,
                                MUIA_Selected, FALSE,
                                MUIA_ShowSelState, FALSE,
                                MUIA_Image_Spec, MUII_RadioButton,
                                MUIA_Frame, MUIV_Frame_None,
                                End),

                            Child, (IPTR)(gui->GamepadObject_button_ls = ImageObject,
                                MUIA_Image_FontMatch, TRUE,
                                MUIA_Selected, FALSE,
                                MUIA_ShowSelState, FALSE,
                                MUIA_Image_Spec, MUII_RadioButton,
                                MUIA_Frame, MUIV_Frame_None,
                                End),

                            Child, (IPTR)(gui->GamepadObject_button_rs = ImageObject,
                                MUIA_Image_FontMatch, TRUE,
                                MUIA_Selected, FALSE,
                                MUIA_ShowSelState, FALSE,
                                MUIA_Image_Spec, MUII_RadioButton,
                                MUIA_Frame, MUIV_Frame_None,
                                End),

                            Child, (IPTR)(gui->GamepadObject_left_thumb = ImageObject,
                                MUIA_Image_FontMatch, TRUE,
                                MUIA_Selected, FALSE,
                                MUIA_ShowSelState, FALSE,
                                MUIA_Image_Spec, MUII_RadioButton,
                                MUIA_Frame, MUIV_Frame_None,
                                End),

                            Child, (IPTR)(gui->GamepadObject_right_thumb = ImageObject,
                                MUIA_Image_FontMatch, TRUE,
                                MUIA_Selected, FALSE,
                                MUIA_ShowSelState, FALSE,
                                MUIA_Image_Spec, MUII_RadioButton,
                                MUIA_Frame, MUIV_Frame_None,
                                End),

                            Child, (IPTR)(gui->GamepadObject_dpad_left = ImageObject,
                                MUIA_Image_FontMatch, TRUE,
                                MUIA_Selected, FALSE,
                                MUIA_ShowSelState, FALSE,
                                MUIA_Image_Spec, MUII_RadioButton,
                                MUIA_Frame, MUIV_Frame_None,
                                End),

                            Child, (IPTR)(gui->GamepadObject_dpad_right = ImageObject,
                                MUIA_Image_FontMatch, TRUE,
                                MUIA_Selected, FALSE,
                                MUIA_ShowSelState, FALSE,
                                MUIA_Image_Spec, MUII_RadioButton,
                                MUIA_Frame, MUIV_Frame_None,
                                End),

                            Child, (IPTR)(gui->GamepadObject_dpad_up = ImageObject,
                                MUIA_Image_FontMatch, TRUE,
                                MUIA_Selected, FALSE,
                                MUIA_ShowSelState, FALSE,
                                MUIA_Image_Spec, MUII_RadioButton,
                                MUIA_Frame, MUIV_Frame_None,
                                End),

                            Child, (IPTR)(gui->GamepadObject_dpad_down = ImageObject,
                                MUIA_Image_FontMatch, TRUE,
                                MUIA_Selected, FALSE,
                                MUIA_ShowSelState, FALSE,
                                MUIA_Image_Spec, MUII_RadioButton,
                                MUIA_Frame, MUIV_Frame_None,
                                End),

                            Child, (IPTR)(gui->GamepadObject_button_back = ImageObject,
                                MUIA_Image_FontMatch, TRUE,
                                MUIA_Selected, FALSE,
                                MUIA_ShowSelState, FALSE,
                                MUIA_Image_Spec, MUII_RadioButton,
                                MUIA_Frame, MUIV_Frame_None,
                                End),

                            Child, (IPTR)(gui->GamepadObject_button_start = ImageObject,
                                MUIA_Image_FontMatch, TRUE,
                                MUIA_Selected, FALSE,
                                MUIA_ShowSelState, FALSE,
                                MUIA_Image_Spec, MUII_RadioButton,
                                MUIA_Frame, MUIV_Frame_None,
                                End),
                            End,

                            Child, (IPTR)(gui->GamepadObject_left_trigger = GaugeObject,
                                GaugeFrame,
                                MUIA_Gauge_Max, 0xff,
                                MUIA_Gauge_InfoText, (IPTR)"%lx",
                                MUIA_Gauge_Horiz, TRUE,
                                MUIA_Gauge_Current, 0,
                                End),

                            Child, (IPTR)(gui->GamepadObject_right_trigger = GaugeObject,
                                GaugeFrame,
                                MUIA_Gauge_Max, 0xff,
                                MUIA_Gauge_InfoText, (IPTR)"%lx",
                                MUIA_Gauge_Horiz, TRUE,
                                MUIA_Gauge_Current, 0,
                                End),

                            Child, (IPTR)(gui->GamepadObject_left_stick_x = GaugeObject,
                                GaugeFrame,
                                MUIA_Gauge_Max, 0xffff,
                                MUIA_Gauge_InfoText, (IPTR)"%lx",
                                MUIA_Gauge_Horiz, TRUE,
                                MUIA_Gauge_Current, 0,
                                End),

                            Child, (IPTR)(gui->GamepadObject_left_stick_y = GaugeObject,
                                GaugeFrame,
                                MUIA_Gauge_Max, 0xffff,
                                MUIA_Gauge_InfoText, (IPTR)"%lx",
                                MUIA_Gauge_Horiz, TRUE,
                                MUIA_Gauge_Current, 0,
                                End),

                            Child, (IPTR)(gui->GamepadObject_right_stick_x = GaugeObject,
                                GaugeFrame,
                                MUIA_Gauge_Max, 0xffff,
                                MUIA_Gauge_InfoText, (IPTR)"%lx",
                                MUIA_Gauge_Horiz, TRUE,
                                MUIA_Gauge_Current, 0,
                                End),

                            Child, (IPTR)(gui->GamepadObject_right_stick_y = GaugeObject,
                                GaugeFrame,
                                MUIA_Gauge_Max, 0xffff,
                                MUIA_Gauge_InfoText, (IPTR)"%lx",
                                MUIA_Gauge_Horiz, TRUE,
                                MUIA_Gauge_Current, 0,
                                End),

                            End),

                        Child, (IPTR)VSpace(0),
                        Child, (IPTR)HGroup,
                            MUIA_Group_SameWidth, TRUE,
                            Child, (IPTR)(gui->UseObj = TextObject, ButtonFrame,
                                MUIA_Background, MUII_ButtonBack,
                                MUIA_CycleChain, 1,
                                MUIA_InputMode, MUIV_InputMode_RelVerify,
                                MUIA_Text_Contents, (IPTR)"\33c Save ",
                                End),
                            Child, (IPTR)(gui->CloseObj = TextObject, ButtonFrame,
                                MUIA_Background, MUII_ButtonBack,
                                MUIA_CycleChain, 1,
                                MUIA_InputMode, MUIV_InputMode_RelVerify,
                                MUIA_Text_Contents, (IPTR)"\33c Use ",
                                End),
                            End,
                        End,
                    End),
                End;

                if(gui->App) 
                {
                    DoMethod(gui->MainWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
                             gui->App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
                    DoMethod(gui->UseObj, MUIM_Notify, MUIA_Pressed, FALSE,
                             gui->App, 2, MUIM_Application_ReturnID, ID_STORE_CONFIG);
                    DoMethod(gui->CloseObj, MUIM_Notify, MUIA_Pressed, FALSE,
                             gui->App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

                    DoMethod(gui->AboutMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
                             gui->App, 2, MUIM_Application_ReturnID, ID_ABOUT);
                    DoMethod(gui->UseMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
                             gui->App, 2, MUIM_Application_ReturnID, ID_STORE_CONFIG);
                    DoMethod(gui->MUIPrefsMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
                             gui->App, 2, MUIM_Application_OpenConfigWindow, 0);

                    IPTR  isopen = 0;
                    IPTR  iconify = 0;
                    ULONG sigs;
                    ULONG sigmask;
                    LONG retid;

                    get(gui->App, MUIA_Application_Iconified, &iconify);
                    set(gui->MainWindow, MUIA_Window_Open, TRUE);
                    get(gui->MainWindow, MUIA_Window_Open, &isopen);

                    if((isopen || (!iconify)))
                    {
                        sigmask = (1UL<<arosx_eventport->mp_SigBit);
                        do
                        {
                            retid = DoMethod(gui->App, MUIM_Application_NewInput, &sigs);
                            switch(retid)
                            {
                                case ID_ABOUT:
                                    MUI_RequestA(gui->App, gui->MainWindow, 0, NULL, "Fabulous!", VERSION_STRING, NULL);
                                    break;
                            }
                            if(retid == MUIV_Application_ReturnID_Quit)
                            {
                                break;
                            }
                            if(sigs)
                            {
                                sigs = Wait(sigs | sigmask | SIGBREAKF_CTRL_C);
                                if(sigs & SIGBREAKF_CTRL_C)
                                {
                                    break;
                                }

                                if(sigs & (1UL<<arosx_eventport->mp_SigBit)) {
                                    mybug(-1,("(%d) I may have received an event...\n", arosxc->id));
                                    struct AROSX_EventNote *en;
                                    while((en = (struct AROSX_EventNote *)GetMsg(arosx_eventport))) {
 
                                        mybug(-1,("    event %08lx\n", en->en_Event));

                                        /* TODO: GUI is disabled until we get the first message even on wired controllers... */
                                        if((arosxc->status.wireless)&&(arosxc->status.signallost)) {
                                            set(gui->GamepadGroupObject, MUIA_Disabled, TRUE);
                                        } else {
                                            set(gui->GamepadGroupObject, MUIA_Disabled, FALSE);

                                            set(gui->GamepadObject_button_a, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_A));
                                            set(gui->GamepadObject_button_b, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_B));
                                            set(gui->GamepadObject_button_x, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_X));
                                            set(gui->GamepadObject_button_y, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_Y));
                                            set(gui->GamepadObject_button_ls, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_LEFT_SHOULDER));
                                            set(gui->GamepadObject_button_rs, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_RIGHT_SHOULDER));
                                            set(gui->GamepadObject_left_thumb, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_LEFT_THUMB));
                                            set(gui->GamepadObject_right_thumb, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_RIGHT_THUMB));
                                            set(gui->GamepadObject_dpad_left, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_DPAD_LEFT));
                                            set(gui->GamepadObject_dpad_right, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_DPAD_RIGHT));
                                            set(gui->GamepadObject_dpad_up, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_DPAD_UP));
                                            set(gui->GamepadObject_dpad_down, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_DPAD_DOWN));
                                            set(gui->GamepadObject_button_back, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_BACK));
                                            set(gui->GamepadObject_button_start, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_START));

                                            set(gui->GamepadObject_left_trigger, MUIA_Gauge_Current, (arosx_gamepad->LeftTrigger));
                                            set(gui->GamepadObject_right_trigger, MUIA_Gauge_Current, (arosx_gamepad->RightTrigger));

                                            if(arosx_gamepad->ThumbLX>=0x8000) {
                                                set(gui->GamepadObject_left_stick_x, MUIA_Gauge_Current, (arosx_gamepad->ThumbLX-0x8000));
                                            } else {
                                                set(gui->GamepadObject_left_stick_x, MUIA_Gauge_Current, (0x8000+arosx_gamepad->ThumbLX));
                                            }

                                            if(arosx_gamepad->ThumbLY>=0x8000) {
                                                set(gui->GamepadObject_left_stick_y, MUIA_Gauge_Current, (arosx_gamepad->ThumbLY-0x8000));
                                            } else {
                                                set(gui->GamepadObject_left_stick_y, MUIA_Gauge_Current, (0x8000+arosx_gamepad->ThumbLY));
                                            }

                                            if(arosx_gamepad->ThumbRX>=0x8000) {
                                                set(gui->GamepadObject_right_stick_x, MUIA_Gauge_Current, (arosx_gamepad->ThumbRX-0x8000));
                                            } else {
                                                set(gui->GamepadObject_right_stick_x, MUIA_Gauge_Current, (0x8000+arosx_gamepad->ThumbRX));
                                            }

                                            if(arosx_gamepad->ThumbRY>=0x8000) {
                                                set(gui->GamepadObject_right_stick_y, MUIA_Gauge_Current, (arosx_gamepad->ThumbRY-0x8000));
                                            } else {
                                                set(gui->GamepadObject_right_stick_y, MUIA_Gauge_Current, (0x8000+arosx_gamepad->ThumbRY));
                                            }
                                        }

                                        ReplyMsg((struct Message *)en);
                                    }
                                }
                            }
                        } while(TRUE);
                        set(gui->MainWindow, MUIA_Window_Open, FALSE);
                    }

                    if(gui->App)
                    {
                        MUI_DisposeObject(gui->App);
                        gui->App = NULL;
                    }

                    if(MUIMasterBase)
                    {
                        CloseLibrary(MUIMasterBase);
                        MUIMasterBase = NULL;
                    }

                    if(ps)
                    {
                        CloseLibrary(ps);
                        ps = NULL;
                    }
                }
            }
        }
        }

        AROSX_RemEventHandler(arosx_eventhook);
        CloseLibrary(AROSXBase);

        FreeVec(gui);

        Forbid();

        arosxc->GUITask = NULL;
        --arosxb->Library.lib_OpenCnt;

    }

    AROS_USERFUNC_EXIT
}
