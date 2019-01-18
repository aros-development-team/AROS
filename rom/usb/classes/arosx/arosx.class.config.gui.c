/*
    Copyright © 2018-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Gamepad (XInput) USB class driver
    Lang: English
*/

#include "debug.h"

#include "arosx.class.h"

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

    thistask = FindTask(NULL);
    arosxc = thistask->tc_UserData;
    arosxb = arosxc->arosxb;

    struct AROSX_GAMEPAD *arosx_gamepad;
    arosx_gamepad = &arosxc->arosx_gamepad;

    ++arosxb->Library.lib_OpenCnt;
    if((MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)))
    {
        if((ps = OpenLibrary("poseidon.library", 4)))
        {

            arosxc->App = ApplicationObject,
            MUIA_Application_Title      , (IPTR)libname,
            MUIA_Application_Version    , (IPTR)VERSION_STRING,
            MUIA_Application_Copyright  , (IPTR)"©2018-2019 The AROS Development Team",
            MUIA_Application_Author     , (IPTR)"The AROS Development Team",
            MUIA_Application_Description, (IPTR)"Settings for the arosx.class",
            MUIA_Application_Base       , (IPTR)"AROSX",
            MUIA_Application_HelpFile   , (IPTR)"HELP:Poseidon.guide",
            MUIA_Application_Menustrip  , (IPTR)MenustripObject,
                Child, (IPTR)MenuObjectT((IPTR)"Project"),
                    Child, (IPTR)(arosxc->AboutMI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"About...",
                        MUIA_Menuitem_Shortcut, (IPTR)"?",
                        End),
                    End,
                Child, (IPTR)MenuObjectT((IPTR)"Settings"),
                    Child, (IPTR)(arosxc->UseMI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"Save",
                        MUIA_Menuitem_Shortcut, (IPTR)"S",
                        End),
                    Child, (IPTR)MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)NM_BARLABEL,
                        End,
                    Child, (IPTR)(arosxc->MUIPrefsMI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"MUI Settings",
                        MUIA_Menuitem_Shortcut, (IPTR)"M",
                        End),
                    End,
                End,

            SubWindow, (IPTR)(arosxc->MainWindow = WindowObject,
                MUIA_Window_ID   , MAKE_ID('M','A','I','N'),
                MUIA_Window_Title, (IPTR)arosxc->name,
                MUIA_HelpNode, (IPTR)libname,

                WindowContents, (IPTR)VGroup,
                    Child, (IPTR)(arosxc->GamepadGroupObject = ColGroup(2),
                        GroupFrameT("Gamepad"),
                        MUIA_Disabled, TRUE,


                        Child, (IPTR)HGroup,
                        Child, (IPTR)(arosxc->GamepadObject_button_a = ImageObject,
                            MUIA_Image_FontMatch, TRUE,
                            MUIA_Selected, FALSE,
                            MUIA_ShowSelState, FALSE,
                            MUIA_Image_Spec, MUII_RadioButton,
                            MUIA_Frame, MUIV_Frame_None,
                            End),

                        Child, (IPTR)(arosxc->GamepadObject_button_b = ImageObject,
                            MUIA_Image_FontMatch, TRUE,
                            MUIA_Selected, FALSE,
                            MUIA_ShowSelState, FALSE,
                            MUIA_Image_Spec, MUII_RadioButton,
                            MUIA_Frame, MUIV_Frame_None,
                            End),

                        Child, (IPTR)(arosxc->GamepadObject_button_x = ImageObject,
                            MUIA_Image_FontMatch, TRUE,
                            MUIA_Selected, FALSE,
                            MUIA_ShowSelState, FALSE,
                            MUIA_Image_Spec, MUII_RadioButton,
                            MUIA_Frame, MUIV_Frame_None,
                            End),

                        Child, (IPTR)(arosxc->GamepadObject_button_y = ImageObject,
                            MUIA_Image_FontMatch, TRUE,
                            MUIA_Selected, FALSE,
                            MUIA_ShowSelState, FALSE,
                            MUIA_Image_Spec, MUII_RadioButton,
                            MUIA_Frame, MUIV_Frame_None,
                            End),

                        Child, (IPTR)(arosxc->GamepadObject_button_ls = ImageObject,
                            MUIA_Image_FontMatch, TRUE,
                            MUIA_Selected, FALSE,
                            MUIA_ShowSelState, FALSE,
                            MUIA_Image_Spec, MUII_RadioButton,
                            MUIA_Frame, MUIV_Frame_None,
                            End),

                        Child, (IPTR)(arosxc->GamepadObject_button_rs = ImageObject,
                            MUIA_Image_FontMatch, TRUE,
                            MUIA_Selected, FALSE,
                            MUIA_ShowSelState, FALSE,
                            MUIA_Image_Spec, MUII_RadioButton,
                            MUIA_Frame, MUIV_Frame_None,
                            End),

                        Child, (IPTR)(arosxc->GamepadObject_left_thumb = ImageObject,
                            MUIA_Image_FontMatch, TRUE,
                            MUIA_Selected, FALSE,
                            MUIA_ShowSelState, FALSE,
                            MUIA_Image_Spec, MUII_RadioButton,
                            MUIA_Frame, MUIV_Frame_None,
                            End),

                        Child, (IPTR)(arosxc->GamepadObject_right_thumb = ImageObject,
                            MUIA_Image_FontMatch, TRUE,
                            MUIA_Selected, FALSE,
                            MUIA_ShowSelState, FALSE,
                            MUIA_Image_Spec, MUII_RadioButton,
                            MUIA_Frame, MUIV_Frame_None,
                            End),

                        Child, (IPTR)(arosxc->GamepadObject_dpad_left = ImageObject,
                            MUIA_Image_FontMatch, TRUE,
                            MUIA_Selected, FALSE,
                            MUIA_ShowSelState, FALSE,
                            MUIA_Image_Spec, MUII_RadioButton,
                            MUIA_Frame, MUIV_Frame_None,
                            End),

                        Child, (IPTR)(arosxc->GamepadObject_dpad_right = ImageObject,
                            MUIA_Image_FontMatch, TRUE,
                            MUIA_Selected, FALSE,
                            MUIA_ShowSelState, FALSE,
                            MUIA_Image_Spec, MUII_RadioButton,
                            MUIA_Frame, MUIV_Frame_None,
                            End),

                        Child, (IPTR)(arosxc->GamepadObject_dpad_up = ImageObject,
                            MUIA_Image_FontMatch, TRUE,
                            MUIA_Selected, FALSE,
                            MUIA_ShowSelState, FALSE,
                            MUIA_Image_Spec, MUII_RadioButton,
                            MUIA_Frame, MUIV_Frame_None,
                            End),

                        Child, (IPTR)(arosxc->GamepadObject_dpad_down = ImageObject,
                            MUIA_Image_FontMatch, TRUE,
                            MUIA_Selected, FALSE,
                            MUIA_ShowSelState, FALSE,
                            MUIA_Image_Spec, MUII_RadioButton,
                            MUIA_Frame, MUIV_Frame_None,
                            End),

                        Child, (IPTR)(arosxc->GamepadObject_button_back = ImageObject,
                            MUIA_Image_FontMatch, TRUE,
                            MUIA_Selected, FALSE,
                            MUIA_ShowSelState, FALSE,
                            MUIA_Image_Spec, MUII_RadioButton,
                            MUIA_Frame, MUIV_Frame_None,
                            End),

                        Child, (IPTR)(arosxc->GamepadObject_button_start = ImageObject,
                            MUIA_Image_FontMatch, TRUE,
                            MUIA_Selected, FALSE,
                            MUIA_ShowSelState, FALSE,
                            MUIA_Image_Spec, MUII_RadioButton,
                            MUIA_Frame, MUIV_Frame_None,
                            End),
                        End,

                        Child, (IPTR)(arosxc->GamepadObject_left_trigger = GaugeObject,
                            GaugeFrame,
                            MUIA_Gauge_Max, 0xff,
                            MUIA_Gauge_InfoText, (IPTR)"%lx",
                            MUIA_Gauge_Horiz, TRUE,
                            MUIA_Gauge_Current, 0,
                            End),

                        Child, (IPTR)(arosxc->GamepadObject_right_trigger = GaugeObject,
                            GaugeFrame,
                            MUIA_Gauge_Max, 0xff,
                            MUIA_Gauge_InfoText, (IPTR)"%lx",
                            MUIA_Gauge_Horiz, TRUE,
                            MUIA_Gauge_Current, 0,
                            End),

                        Child, (IPTR)(arosxc->GamepadObject_left_stick_x = GaugeObject,
                            GaugeFrame,
                            MUIA_Gauge_Max, 0xffff,
                            MUIA_Gauge_InfoText, (IPTR)"%lx",
                            MUIA_Gauge_Horiz, TRUE,
                            MUIA_Gauge_Current, 0,
                            End),

                        Child, (IPTR)(arosxc->GamepadObject_left_stick_y = GaugeObject,
                            GaugeFrame,
                            MUIA_Gauge_Max, 0xffff,
                            MUIA_Gauge_InfoText, (IPTR)"%lx",
                            MUIA_Gauge_Horiz, TRUE,
                            MUIA_Gauge_Current, 0,
                            End),

                        Child, (IPTR)(arosxc->GamepadObject_right_stick_x = GaugeObject,
                            GaugeFrame,
                            MUIA_Gauge_Max, 0xffff,
                            MUIA_Gauge_InfoText, (IPTR)"%lx",
                            MUIA_Gauge_Horiz, TRUE,
                            MUIA_Gauge_Current, 0,
                            End),

                        Child, (IPTR)(arosxc->GamepadObject_right_stick_y = GaugeObject,
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
                        Child, (IPTR)(arosxc->UseObj = TextObject, ButtonFrame,
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                            MUIA_Text_Contents, (IPTR)"\33c Save ",
                            End),
                        Child, (IPTR)(arosxc->CloseObj = TextObject, ButtonFrame,
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                            MUIA_Text_Contents, (IPTR)"\33c Use ",
                            End),
                        End,
                    End,
                End),
            End;

            if(arosxc->App) 
            {
                DoMethod(arosxc->MainWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
                         arosxc->App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
                DoMethod(arosxc->UseObj, MUIM_Notify, MUIA_Pressed, FALSE,
                         arosxc->App, 2, MUIM_Application_ReturnID, ID_STORE_CONFIG);
                DoMethod(arosxc->CloseObj, MUIM_Notify, MUIA_Pressed, FALSE,
                         arosxc->App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

                DoMethod(arosxc->AboutMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
                         arosxc->App, 2, MUIM_Application_ReturnID, ID_ABOUT);
                DoMethod(arosxc->UseMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
                         arosxc->App, 2, MUIM_Application_ReturnID, ID_STORE_CONFIG);
                DoMethod(arosxc->MUIPrefsMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
                         arosxc->App, 2, MUIM_Application_OpenConfigWindow, 0);


                IPTR  isopen = 0;
                IPTR  iconify = 0;
                ULONG sigs;
                ULONG sigmask;
                LONG retid;

                get(arosxc->App, MUIA_Application_Iconified, &iconify);
                set(arosxc->MainWindow, MUIA_Window_Open, TRUE);
                get(arosxc->MainWindow, MUIA_Window_Open, &isopen);

                if((isopen || (!iconify)))
                {
                    arosxc->TrackingSignal = AllocSignal(-1);
                    sigmask = (1<<arosxc->TrackingSignal);
                    do
                    {
                        retid = DoMethod(arosxc->App, MUIM_Application_NewInput, &sigs);
                        switch(retid)
                        {
                            case ID_ABOUT:
                                MUI_RequestA(arosxc->App, arosxc->MainWindow, 0, NULL, "Fabulous!", VERSION_STRING, NULL);
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

                            if((ULONG)(1<<arosxc->TrackingSignal)) {

                                /* TODO: Check if the GUI goes to sleep when the controller says it's sleepy */
                                if((arosxc->status.wireless)&&(arosxc->status.signallost)) {
                                    set(arosxc->GamepadGroupObject, MUIA_Disabled, TRUE);
                                    //psdDelayMS(10);
                                } else {
                                    set(arosxc->GamepadGroupObject, MUIA_Disabled, FALSE);

                                    set(arosxc->GamepadObject_button_a, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_A));
                                    set(arosxc->GamepadObject_button_b, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_B));
                                    set(arosxc->GamepadObject_button_x, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_X));
                                    set(arosxc->GamepadObject_button_y, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_Y));
                                    set(arosxc->GamepadObject_button_ls, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_LEFT_SHOULDER));
                                    set(arosxc->GamepadObject_button_rs, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_RIGHT_SHOULDER));
                                    set(arosxc->GamepadObject_left_thumb, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_LEFT_THUMB));
                                    set(arosxc->GamepadObject_right_thumb, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_RIGHT_THUMB));
                                    set(arosxc->GamepadObject_dpad_left, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_DPAD_LEFT));
                                    set(arosxc->GamepadObject_dpad_right, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_DPAD_RIGHT));
                                    set(arosxc->GamepadObject_dpad_up, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_DPAD_UP));
                                    set(arosxc->GamepadObject_dpad_down, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_DPAD_DOWN));
                                    set(arosxc->GamepadObject_button_back, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_BACK));
                                    set(arosxc->GamepadObject_button_start, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_START));

                                    set(arosxc->GamepadObject_left_trigger, MUIA_Gauge_Current, (arosx_gamepad->LeftTrigger));
                                    set(arosxc->GamepadObject_right_trigger, MUIA_Gauge_Current, (arosx_gamepad->RightTrigger));

                                    if(arosx_gamepad->ThumbLX>=0x8000) {
                                        set(arosxc->GamepadObject_left_stick_x, MUIA_Gauge_Current, (arosx_gamepad->ThumbLX-0x8000));
                                    } else {
                                        set(arosxc->GamepadObject_left_stick_x, MUIA_Gauge_Current, (0x8000+arosx_gamepad->ThumbLX));
                                    }

                                    if(arosx_gamepad->ThumbLY>=0x8000) {
                                        set(arosxc->GamepadObject_left_stick_y, MUIA_Gauge_Current, (arosx_gamepad->ThumbLY-0x8000));
                                    } else {
                                        set(arosxc->GamepadObject_left_stick_y, MUIA_Gauge_Current, (0x8000+arosx_gamepad->ThumbLY));
                                    }

                                    if(arosx_gamepad->ThumbRX>=0x8000) {
                                        set(arosxc->GamepadObject_right_stick_x, MUIA_Gauge_Current, (arosx_gamepad->ThumbRX-0x8000));
                                    } else {
                                        set(arosxc->GamepadObject_right_stick_x, MUIA_Gauge_Current, (0x8000+arosx_gamepad->ThumbRX));
                                    }

                                    if(arosx_gamepad->ThumbRY>=0x8000) {
                                        set(arosxc->GamepadObject_right_stick_y, MUIA_Gauge_Current, (arosx_gamepad->ThumbRY-0x8000));
                                    } else {
                                        set(arosxc->GamepadObject_right_stick_y, MUIA_Gauge_Current, (0x8000+arosx_gamepad->ThumbRY));
                                    }

                                    /* 100Hz max. GUI update frequency should be enough for everyone... */
                                    //psdDelayMS(10);
                                }
                            }
                        }
                    } while(TRUE);
                    set(arosxc->MainWindow, MUIA_Window_Open, FALSE);
                }

                if(arosxc->App)
                {
                    MUI_DisposeObject(arosxc->App);
                    arosxc->App = NULL;
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

    Forbid();
    FreeSignal(arosxc->TrackingSignal);
    arosxc->TrackingSignal = -1;
    arosxc->GUITask = NULL;
    --arosxb->Library.lib_OpenCnt;

    AROS_USERFUNC_EXIT
}
