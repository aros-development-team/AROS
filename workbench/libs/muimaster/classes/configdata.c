/*
    Copyright (C) 2002-2025, The AROS Development Team. All rights reserved.

*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <exec/types.h>
#include <prefs/prefhdr.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/dos.h>
#include <proto/commodities.h>
#include <proto/muimaster.h>

/*  #define MYDEBUG 1 */
#include "debug.h"

#include "muimaster_intern.h"
#include "mui.h"
#include "support.h"
#include "prefs.h"
#include "imspec.h"

#include <proto/muiscreen.h>

#define PUBSCREEN_FILE "env:Zune/PublicScreens.iff"

#define DFSN(x)
#define DPSD(x)

extern struct Library *MUIMasterBase;

struct MUI_ConfigdataData
{
    Object                      *app;
    CONST_STRPTR                appbase;
    struct ZunePrefsNew         prefs;
    struct SignalSemaphore      psLock;
    struct MsgPort             *fsNotifyPort;
    struct MUI_InputHandlerNode fsNotifyIHN;
    struct NotifyRequest        fsNotifyRequest;
    struct List                 pubscreens;
};

static inline CONST_STRPTR GetConfigString(Object *obj, ULONG id)
{
    return (CONST_STRPTR) DoMethod(obj, MUIM_Configdata_GetString, id);
}

static inline ULONG GetConfigULong(Object *obj, ULONG id)
{
    return (ULONG) DoMethod(obj, MUIM_Configdata_GetULong, id);
}


/**************************************************************************
 Default ImageSpec values
**************************************************************************/

struct spec_cfg
{
    ULONG muiv;
    ULONG cfgid;
    CONST_STRPTR defspec;
};

const static struct spec_cfg DefImspecValues[] =
{
    { MUII_WindowBack,    MUICFG_Background_Window,     "0:128" },
    { MUII_RequesterBack, MUICFG_Background_Requester,  "0:137" },
    { MUII_ButtonBack,    MUICFG_Background_Button,     "0:128" },
    { MUII_ListBack,      MUICFG_Background_List,       "0:128" },
    { MUII_TextBack,      MUICFG_Background_Text,       "0:128" },
    { MUII_PropBack,      MUICFG_Background_Prop,       "0:128" },
    { MUII_PopupBack,     MUICFG_Background_PopUp,      "0:128" },
    { MUII_SelectedBack,  MUICFG_Background_Selected,   "0:131" },
    { MUII_ListCursor,    MUICFG_Background_ListCursor, "0:131" },
    { MUII_ListSelect,    MUICFG_Background_ListSelect, "0:135" },
    { MUII_ListSelCur,    MUICFG_Background_ListSelCur, "0:138" },
    { MUII_ArrowUp,       MUICFG_Image_ArrowUp,         "1:0" },
    { MUII_ArrowDown,     MUICFG_Image_ArrowDown,       "1:1" },
    { MUII_ArrowLeft,     MUICFG_Image_ArrowLeft,       "1:2" },
    { MUII_ArrowRight,    MUICFG_Image_ArrowRight,      "1:3" },
    { MUII_CheckMark,     MUICFG_Image_CheckMark,       "1:4" },
    { MUII_RadioButton,   MUICFG_Image_RadioButton,     "1:5" },
    { MUII_Cycle,         MUICFG_Image_Cycle,           "1:6" },
    { MUII_PopUp,         MUICFG_Image_PopUp,           "1:7" },
    { MUII_PopFile,       MUICFG_Image_PopFile,         "1:8" },
    { MUII_PopDrawer,     MUICFG_Image_PopDrawer,       "1:9" },
    { MUII_PropKnob,      MUICFG_Image_PropKnob,        "0:129" },
    { MUII_Drawer,        MUICFG_Image_Drawer,          "1:10" },
    { MUII_HardDisk,      MUICFG_Image_HardDisk,        "1:11" },
    { MUII_Disk,          MUICFG_Image_Disk,            "1:12" },
    { MUII_Chip,          MUICFG_Image_Chip,            "1:13" },
    { MUII_Volume,        MUICFG_Image_Volume,          "1:14" },
    { MUII_RegisterBack,  MUICFG_Background_Register,   "0:128" },
    { MUII_Network,       MUICFG_Image_Network,         "1:15" },
    { MUII_Assign,        MUICFG_Image_Assign,          "1:16" },
    { MUII_TapePlay,      MUICFG_Image_TapePlay,        "1:17" },
    { MUII_TapePlayBack,  MUICFG_Image_TapePlayBack,    "1:18" },
    { MUII_TapePause,     MUICFG_Image_TapePause,       "1:19" },
    { MUII_TapeStop,      MUICFG_Image_TapeStop,        "1:20" },
    { MUII_TapeRecord,    MUICFG_Image_TapeRecord,      "1:21" },
    { MUII_GroupBack,     MUICFG_Background_Framed,     "0:128" },
    { MUII_SliderBack,    MUICFG_Background_Slider,     "0:128" },
    { MUII_SliderKnob,    MUICFG_Background_SliderKnob, "0:128" },
    { MUII_TapeUp,        MUICFG_Image_TapeUp,          "1:22" },
    { MUII_TapeDown,      MUICFG_Image_TapeDown,        "1:23" },
    { MUII_PageBack,      MUICFG_Background_Page,       "0:128" },
    { MUII_ReadListBack,  MUICFG_Background_ReadList,   "0:128" },
    { 0,                  0,                            NULL },
};

/* called by Configdata_New */
static void init_imspecs(Object *obj, struct MUI_ConfigdataData *data)
{
    int i;

    for (i = 0; DefImspecValues[i].defspec; i++)
    {
        CONST_STRPTR imspec;
        const struct spec_cfg *img = DefImspecValues + i;

        imspec = GetConfigString(obj, img->cfgid);
/*          D(bug("init_imspecs: %ld %lx %s ...\n", img->muiv, img->cfgid, imspec)); */
        data->prefs.imagespecs[img->muiv] = imspec;
        if (!data->prefs.imagespecs[img->muiv])
        {
/*              D(bug("*** init_imspecs: null imagespec %ld\n", img->muiv)); */
        }
    }
}

/**************************************************************************
 Default FrameSpec values
**************************************************************************/

/* spec format : type, recessed, left, right, up, down spacing */
const static struct spec_cfg DefFramespecValues[] =
{
    { MUIV_Frame_None,        MUICFG_Invalid,           "000000" }, /* invisible frame          */
    { MUIV_Frame_Button,      MUICFG_Frame_Button,      "202211" }, /* text button              */
    { MUIV_Frame_ImageButton, MUICFG_Frame_ImageButton, "202211" }, /* image button             */
    { MUIV_Frame_Text,        MUICFG_Frame_Text,        "212211" }, /* textfield without input  */
    { MUIV_Frame_String,      MUICFG_Frame_String,      "302211" }, /* string gadget            */
    { MUIV_Frame_ReadList,    MUICFG_Frame_ReadList,    "212211" }, /* list without input       */
    { MUIV_Frame_InputList,   MUICFG_Frame_InputList,   "202211" }, /* list with input          */
    { MUIV_Frame_Prop,        MUICFG_Frame_Prop,        "202211" }, /* scrollbar container      */
    { MUIV_Frame_Gauge,       MUICFG_Frame_Gauge,       "210000" }, /* gauge                    */
    { MUIV_Frame_Group,       MUICFG_Frame_Group,       "314444" }, /* normal group             */
    { MUIV_Frame_PopUp,       MUICFG_Frame_PopUp,       "112211" }, /* cycle menu, popup window */
    { MUIV_Frame_Virtual,     MUICFG_Frame_Virtual,     "212211" }, /* virt group               */
    { MUIV_Frame_Slider,      MUICFG_Frame_Slider,      "400000" }, /* slider container         */
    { MUIV_Frame_Knob,        MUICFG_Frame_Knob,        "202211" }, /* slider knob              */
    { MUIV_Frame_Drag,        MUICFG_Frame_Drag,        "300000" }, /* dnd frame                */
    { -1,                     -1,                       NULL     },
};

/* called by Configdata_New */
static void init_framespecs(Object *obj, struct MUI_ConfigdataData *data)
{
    int i;

    for (i = 0; DefFramespecValues[i].defspec; i++)
    {
        CONST_STRPTR framespec;
        const struct spec_cfg *fcfg = DefFramespecValues + i;

        framespec = GetConfigString(obj, fcfg->cfgid);
        zune_frame_spec_to_intern(framespec,
            &data->prefs.frames[fcfg->muiv]);
    }
}

/**************************************************************************
 Default ULONG values
**************************************************************************/

struct def_ulval
{
    ULONG id;
    ULONG val;
};

const static struct def_ulval DefULValues[] = {
    {MUICFG_Window_Spacing_Left, 4},
    {MUICFG_Window_Spacing_Right, 4},
    {MUICFG_Window_Spacing_Top, 3},
    {MUICFG_Window_Buttons, 0},
    {MUICFG_Window_Spacing_Bottom, 3},
    {MUICFG_Window_Positions, WINDOW_POSITION_FORGET_ON_EXIT},
    {MUICFG_Window_Redraw, WINDOW_REDRAW_WITHOUT_CLEAR},
    {MUICFG_Window_Refresh, WINDOW_REFRESH_SIMPLE},
    {MUICFG_Radio_HSpacing, 4},
    {MUICFG_Radio_VSpacing, 1},
    {MUICFG_Group_HSpacing, 6},
    {MUICFG_Group_VSpacing, 3},
    {MUICFG_Cycle_MenuCtrl_Position, CYCLE_MENU_POSITION_BELOW},
    {MUICFG_Cycle_MenuCtrl_Level, 2},
    {MUICFG_Cycle_MenuCtrl_Speed, 0},
    {MUICFG_Cycle_Menu_Recessed, FALSE},
    {MUICFG_Listview_Font_Leading, 1},
    {MUICFG_Listview_Smoothed, FALSE},
    {MUICFG_Listview_SmoothVal, 0},
    {MUICFG_Listview_Refresh, LISTVIEW_REFRESH_MIXED},
    {MUICFG_Listview_Multi, LISTVIEW_MULTI_SHIFTED},
    {MUICFG_GroupTitle_Position, GROUP_TITLE_POSITION_CENTERED},
    {MUICFG_GroupTitle_Color, GROUP_TITLE_COLOR_HILITE},
    {MUICFG_Scrollbar_Type, SCROLLBAR_TYPE_STANDARD},
    {MUICFG_Scrollbar_Arrangement, SCROLLBAR_ARRANGEMENT_TOP},
    {MUICFG_Balance_Look, BALANCING_SHOW_FRAMES},
    {MUICFG_Dragndrop_Look, DND_LOOK_GHOSTED_ON_BOX},
    {MUICFG_Drag_Autostart, TRUE},
    {MUICFG_Drag_Autostart_Length, 3},
    {MUICFG_Drag_LeftButton, TRUE},
    {MUICFG_Drag_MiddleButton, FALSE},
    {MUICFG_Register_TruncateTitles, FALSE},
    {MUICFG_Screen_Mode, 0},
    {MUICFG_Screen_Mode_ID, -1},
    {MUICFG_Screen_Width, 0},
    {MUICFG_Screen_Height, 0},
    {MUICFG_PublicScreen_PopToFront, TRUE},
    {MUICFG_Iconification_ShowIcon, TRUE},
    {MUICFG_Iconification_ShowMenu, FALSE},
    {MUICFG_Iconification_OnStartup, FALSE},
    {MUICFG_Interfaces_EnableARexx, TRUE},
    {MUICFG_BubbleHelp_FirstDelay, 30},
    {MUICFG_BubbleHelp_NextDelay, 10},
    {0, 0},
};

/**************************************************************************
 Default string values
**************************************************************************/

struct def_strval
{
    ULONG id;
    CONST_STRPTR val;
};

/* NULL values not allowed */
const static struct def_strval DefStrValues[] = {
    {MUICFG_Font_Normal, ""},
    {MUICFG_Font_List, ""},
    {MUICFG_Font_Tiny, ""},
    {MUICFG_Font_Fixed, ""},
    {MUICFG_Font_Title, ""},
    {MUICFG_Font_Big, ""},
    {MUICFG_Font_Button, ""},
    {MUICFG_Font_Knob, ""},
    {MUICFG_String_Background, "2:m2"},
    {MUICFG_String_Text, "m5"},
    {MUICFG_String_ActiveBackground, "2:m1"},
    {MUICFG_String_ActiveText, "m5"},
    {MUICFG_String_Cursor, "m7"},
    {MUICFG_String_MarkedBackground, "m6"},
    {MUICFG_String_MarkedText, "m0"},
    {MUICFG_ActiveObject_Color, "m0"},
    {MUICFG_Keyboard_Press, "-upstroke return"},
    {MUICFG_Keyboard_Toggle, "-repeat space"},
    {MUICFG_Keyboard_Up, "-repeat up"},
    {MUICFG_Keyboard_Down, "-repeat down"},
    {MUICFG_Keyboard_PageUp, "-repeat shift up"},
    {MUICFG_Keyboard_PageDown, "-repeat shift down"},
    {MUICFG_Keyboard_Top, "control up"},
    {MUICFG_Keyboard_Bottom, "control down"},
    {MUICFG_Keyboard_Left, "-repeat left"},
    {MUICFG_Keyboard_Right, "-repeat right"},
    {MUICFG_Keyboard_WordLeft, "-repeat control left"},
    {MUICFG_Keyboard_WordRight, "-repeat control right"},
    {MUICFG_Keyboard_LineStart, "shift left"},
    {MUICFG_Keyboard_LineEnd, "shift right"},
    {MUICFG_Keyboard_NextGadget, "-repeat tab"},
    {MUICFG_Keyboard_PrevGadget, "-repeat shift tab"},
    {MUICFG_Keyboard_GadgetOff, "control tab"},
    {MUICFG_Keyboard_CloseWindow, "esc"},
    {MUICFG_Keyboard_NextWindow, "-repeat alt tab"},
    {MUICFG_Keyboard_PrevWindow, "-repeat alt shift tab"},
    {MUICFG_Keyboard_Help, "help"},
    {MUICFG_Keyboard_Popup, "control p"},
    {MUICFG_Drag_LMBModifier, "control"},
    {MUICFG_Drag_MMBModifier, ""},
    {MUICFG_PublicScreen, ""},
    {MUICFG_Iconification_Hotkey, ""},
    {0, 0},
};


/**************************************************************************
 OM_NEW
 Load global (and maybe application-specific) prefs files into the dataspace,
 then fill the prefs struct with dataspace or default values
**************************************************************************/
IPTR Configdata__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ConfigdataData *data;
    struct TagItem *tags;
    struct TagItem *tag;
    //APTR cdata;
    int i, res = 0;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return (IPTR) NULL;

/*      D(bug("Configdata_New(%p)\n", obj)); */

    data = INST_DATA(cl, obj);

    NEWLIST(&data->pubscreens);
    InitSemaphore(&data->psLock);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Configdata_Application:
            data->app = (Object *) tag->ti_Data;
            break;
        case MUIA_Configdata_ApplicationBase:
            data->appbase = (CONST_STRPTR) tag->ti_Data;
            break;
        }
    }

    if (data->app && !data->appbase)
    {
        get(data->app, MUIA_Application_Base, &data->appbase);
    }

    if (data->appbase)
    {
        char filename[255];
        snprintf(filename, 255, "ENV:zune/%s.prefs", data->appbase);
        res = DoMethod(obj, MUIM_Configdata_Load, (IPTR) filename);
        if (!res)
        {
            snprintf(filename, 255, "ENVARC:zune/%s.prefs", data->appbase);
            res = DoMethod(obj, MUIM_Configdata_Load, (IPTR) filename);
        }
    }

    // load only global prefs if no local app pref is found
    if (!res)
    {
        if (!DoMethod(obj, MUIM_Configdata_Load,
                (IPTR) "ENV:zune/global.prefs"))
        {
            DoMethod(obj, MUIM_Configdata_Load,
                (IPTR) "ENVARC:zune/global.prefs");
        }
    }

    data->fsNotifyPort = CreateMsgPort();
    if (data->fsNotifyPort) {
        /* Setup filesystem notification handler ---------------------------*/
        data->fsNotifyIHN.ihn_Signals = 1UL << data->fsNotifyPort->mp_SigBit;
        data->fsNotifyIHN.ihn_Object  = obj;
        data->fsNotifyIHN.ihn_Method  = MUIM_Configdata_LoadPubScreens;

        DoMethod(data->app, MUIM_Application_AddInputHandler, (IPTR)&data->fsNotifyIHN);

        data->fsNotifyRequest.nr_Name                 = PUBSCREEN_FILE;
        data->fsNotifyRequest.nr_Flags                = NRF_SEND_MESSAGE;
        data->fsNotifyRequest.nr_stuff.nr_Msg.nr_Port = data->fsNotifyPort;
        if (StartNotify(&data->fsNotifyRequest)) {
            DFSN(bug("[MUI:Cfg] %s: FileSystem-Notification setup for '%s'\n", __func__, data->fsNotifyRequest.nr_Name));
        } else {
            DFSN(bug("[MUI:Cfg] %s: FAILED to setup FileSystem-Notification for '%s'\n", __func__, data->fsNotifyRequest.nr_Name));
            DoMethod(data->app, MUIM_Application_RemInputHandler, (IPTR)&data->fsNotifyIHN);
            DeleteMsgPort(data->fsNotifyPort);
            data->fsNotifyPort = NULL;
        }
    }
    
    /*---------- init strings ----------*/
    for (i = 0; DefStrValues[i].id; i++)
    {
        DoMethod(obj, MUIM_Configdata_SetString, DefStrValues[i].id, DefStrValues[i].val);
    }

    /*---------- images stuff ----------*/

    init_imspecs(obj, data);

    /*---------- frame stuff ----------*/

    init_framespecs(obj, data);

    /*---------- system stuff ----------*/
    data->prefs.publicscreen_pop_to_front =
        GetConfigULong(obj, MUICFG_PublicScreen_PopToFront);
    data->prefs.iconification_show_icon =
        GetConfigULong(obj, MUICFG_Iconification_ShowIcon);
    data->prefs.iconification_show_menu =
        GetConfigULong(obj, MUICFG_Iconification_ShowMenu);
    data->prefs.iconification_on_startup =
        GetConfigULong(obj, MUICFG_Iconification_OnStartup);
    data->prefs.interfaces_enable_arexx =
        GetConfigULong(obj, MUICFG_Interfaces_EnableARexx);
    data->prefs.bubblehelp_first_delay =
        GetConfigULong(obj, MUICFG_BubbleHelp_FirstDelay);
    data->prefs.bubblehelp_next_delay =
        GetConfigULong(obj, MUICFG_BubbleHelp_NextDelay);

    /*---------- window stuff ----------*/

    data->prefs.window_inner_left =
        GetConfigULong(obj, MUICFG_Window_Spacing_Left);
    data->prefs.window_inner_right =
        GetConfigULong(obj, MUICFG_Window_Spacing_Right);
    data->prefs.window_inner_top =
        GetConfigULong(obj, MUICFG_Window_Spacing_Top);
    data->prefs.window_inner_bottom =
        GetConfigULong(obj, MUICFG_Window_Spacing_Bottom);
    data->prefs.window_position =
        GetConfigULong(obj, MUICFG_Window_Positions);
    data->prefs.window_redraw = GetConfigULong(obj, MUICFG_Window_Redraw);
    data->prefs.window_refresh = GetConfigULong(obj, MUICFG_Window_Refresh);
    data->prefs.screenmode = GetConfigULong(obj, MUICFG_Screen_Mode);
    data->prefs.screenmodeid = GetConfigULong(obj, MUICFG_Screen_Mode_ID);
    data->prefs.screen_width = GetConfigULong(obj, MUICFG_Screen_Width);
    data->prefs.screen_height = GetConfigULong(obj, MUICFG_Screen_Height);
    data->prefs.window_buttons = GetConfigULong(obj, MUICFG_Window_Buttons);

    /*---------- group stuff ----------*/

    data->prefs.group_title_position =
        GetConfigULong(obj, MUICFG_GroupTitle_Position);
    data->prefs.group_title_color =
        GetConfigULong(obj, MUICFG_GroupTitle_Color);
    data->prefs.group_hspacing = GetConfigULong(obj, MUICFG_Group_HSpacing);
    data->prefs.group_vspacing = GetConfigULong(obj, MUICFG_Group_VSpacing);

    /*---------- registers ----------*/

    data->prefs.register_look = REGISTER_LOOK_TRADITIONAL;
    data->prefs.register_truncate_titles =
        GetConfigULong(obj, MUICFG_Register_TruncateTitles);

    /*---------- Buttons ----------*/

    data->prefs.radiobutton_hspacing =
        GetConfigULong(obj, MUICFG_Radio_HSpacing);
    data->prefs.radiobutton_vspacing =
        GetConfigULong(obj, MUICFG_Radio_VSpacing);

    /*---------- Cycles ----------*/

    data->prefs.cycle_menu_position =
        GetConfigULong(obj, MUICFG_Cycle_MenuCtrl_Position);
    data->prefs.cycle_menu_min_entries =
        GetConfigULong(obj, MUICFG_Cycle_MenuCtrl_Level);
    data->prefs.cycle_menu_speed =
        GetConfigULong(obj, MUICFG_Cycle_MenuCtrl_Speed);
    data->prefs.cycle_menu_recessed_entries =
        GetConfigULong(obj, MUICFG_Cycle_Menu_Recessed);

    /*---------- Sliders ----------*/
    /* all taken care of in frames and images */

    /*---------- Scrollbars ----------*/

    data->prefs.scrollbar_type = GetConfigULong(obj, MUICFG_Scrollbar_Type);
    data->prefs.scrollbar_arrangement =
        GetConfigULong(obj, MUICFG_Scrollbar_Arrangement);

    /*---------- Lists ----------*/

    data->prefs.list_linespacing =
        GetConfigULong(obj, MUICFG_Listview_Font_Leading);
    data->prefs.list_smoothed =
        GetConfigULong(obj, MUICFG_Listview_Smoothed);
    data->prefs.list_smoothval =
        GetConfigULong(obj, MUICFG_Listview_SmoothVal);
    data->prefs.list_multi = GetConfigULong(obj, MUICFG_Listview_Multi);
    data->prefs.list_refresh = GetConfigULong(obj, MUICFG_Listview_Refresh);

    /*---------- Navigation ----------*/

    data->prefs.drag_left_button =
        GetConfigULong(obj, MUICFG_Drag_LeftButton);
    data->prefs.drag_middle_button =
        GetConfigULong(obj, MUICFG_Drag_MiddleButton);
    data->prefs.drag_autostart = GetConfigULong(obj, MUICFG_Drag_Autostart);
    data->prefs.drag_autostart_length =
        GetConfigULong(obj, MUICFG_Drag_Autostart_Length);
    data->prefs.drag_look = GetConfigULong(obj, MUICFG_Dragndrop_Look);
    data->prefs.balancing_look = GetConfigULong(obj, MUICFG_Balance_Look);

    /*---------- CustomFrames ----------*/
    data->prefs.customframe_config_1 =
        GetConfigString(obj, MUICFG_CustomFrame_1);
    data->prefs.customframe_config_2 =
        GetConfigString(obj, MUICFG_CustomFrame_2);
    data->prefs.customframe_config_3 =
        GetConfigString(obj, MUICFG_CustomFrame_3);
    data->prefs.customframe_config_4 =
        GetConfigString(obj, MUICFG_CustomFrame_4);
    data->prefs.customframe_config_5 =
        GetConfigString(obj, MUICFG_CustomFrame_5);
    data->prefs.customframe_config_6 =
        GetConfigString(obj, MUICFG_CustomFrame_6);
    data->prefs.customframe_config_7 =
        GetConfigString(obj, MUICFG_CustomFrame_7);
    data->prefs.customframe_config_8 =
        GetConfigString(obj, MUICFG_CustomFrame_8);
    data->prefs.customframe_config_9 =
        GetConfigString(obj, MUICFG_CustomFrame_9);
    data->prefs.customframe_config_10 =
        GetConfigString(obj, MUICFG_CustomFrame_10);
    data->prefs.customframe_config_11 =
        GetConfigString(obj, MUICFG_CustomFrame_11);
    data->prefs.customframe_config_12 =
        GetConfigString(obj, MUICFG_CustomFrame_12);
    data->prefs.customframe_config_13 =
        GetConfigString(obj, MUICFG_CustomFrame_13);
    data->prefs.customframe_config_14 =
        GetConfigString(obj, MUICFG_CustomFrame_14);
    data->prefs.customframe_config_15 =
        GetConfigString(obj, MUICFG_CustomFrame_15);
    data->prefs.customframe_config_16 =
        GetConfigString(obj, MUICFG_CustomFrame_16);

    /*---------- Special ----------*/
    /* all taken care of in frames and images */

    return (IPTR) obj;
}

static void Configdata__DisposeDescriptors(struct MUI_ConfigdataData *data)
{
    struct Node *psNode, *tmp;

    DPSD(bug("[MUI:Cfg] %s()\n", __func__);)

    ObtainSemaphore(&data->psLock);
    ForeachNodeSafe(&data->pubscreens, psNode, tmp) {
        Remove(psNode);
        MUIS_FreePubScreenDesc((struct MUI_PubScreenDesc *)psNode->ln_Name);
        FreeVec(psNode);
    }
    ReleaseSemaphore(&data->psLock);
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
IPTR Configdata__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_ConfigdataData *data = INST_DATA(cl, obj);

    if (data->fsNotifyPort) {
        DoMethod(data->app, MUIM_Application_RemInputHandler, (IPTR) &data->fsNotifyIHN);
        EndNotify(&data->fsNotifyRequest);
        DeleteMsgPort(data->fsNotifyPort);
    }

    if (MUIScreenBase)
        Configdata__DisposeDescriptors(data);

    return DoSuperMethodA(cl, obj, msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
IPTR Configdata__OM_GET(struct IClass *cl, Object *obj,
    struct opGet *msg)
{
    struct MUI_ConfigdataData *data = INST_DATA(cl, obj);
    IPTR *store = msg->opg_Storage;
    ULONG tag = msg->opg_AttrID;

    switch (tag)
    {
    case MUIA_Configdata_ZunePrefs:
        *store = (IPTR) & data->prefs;
        DoMethod(obj, MUIM_Configdata_GetWindowPos, 0);
        return 1;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

#ifndef AROS_BIG_ENDIAN
#define AROS_BIG_ENDIAN 0
#endif

#if (!AROS_BIG_ENDIAN)
static LONG windowpos_swapbytes(IPTR data, LONG size)
{
    LONG items;
    WORD cnt, i, j;
    void *p = (void *)data;

    *((LONG *) p) = AROS_SWAP_BYTES_LONG(size);

    cnt = sizeof(LONG);
    items = (size - sizeof(LONG)) / sizeof(struct windowpos);
    D(bug("[MUI:Cfg] %s: size = %d items = %d\n", __func__, size, items));
    for (i = 0; i < items; i++) {
        *((LONG *) ((IPTR)p + cnt)) =
            AROS_SWAP_BYTES_LONG(*((LONG *) ((IPTR)p + cnt)));
        cnt += sizeof(LONG);
        for (j = 0; j < 8; j++) {
            *((WORD *) ((IPTR)p + cnt)) =
                AROS_SWAP_BYTES_WORD(*((WORD *) ((IPTR)p + cnt)));
            cnt += sizeof(WORD);
        }
    }
    D(bug("[MUI:Cfg] %s: actual = %d\n", __func__, cnt));
    return cnt;
}
#endif

static IPTR Configdata_GetWindowPos(struct IClass *cl, Object *obj,
    struct MUIP_Configdata_GetString *msg)
{
    struct MUI_ConfigdataData *data;
    IPTR s;
    data = INST_DATA(cl, obj);
    //kprintf ("getwindowpos\n");
    s = (IPTR) DoMethod(obj, MUIM_Dataspace_Find, MUICFG_WindowPos);
    if (s && data->app) {
#if (!AROS_BIG_ENDIAN)
        windowpos_swapbytes(s, AROS_BE2LONG(*((LONG *)s)));
#endif
        set(data->app, MUIA_Application_CopyWinPosToApp, s);
#if (!AROS_BIG_ENDIAN)
        windowpos_swapbytes(s, *((LONG *)s));
#endif
    }
    return s;
}

static IPTR Configdata_SetWindowPos(struct IClass *cl, Object *obj,
    struct MUIP_Configdata_GetString *msg)
{
    struct MUI_ConfigdataData *data;
    //kprintf ("setwindowpos\n");
    data = INST_DATA(cl, obj);
    IPTR addr = 0;
    LONG size = 0;

    if (data->app)
    {
        get(data->app, MUIA_Application_GetWinPosAddr, &addr);
        get(data->app, MUIA_Application_GetWinPosSize, &size);

        /* We can ignore size-variable because
         * MUIA_Application_GetWinPosSize updates *((LONG*)addr) */
#if (AROS_BIG_ENDIAN)
        size = *((LONG *)addr);
#else
        size = windowpos_swapbytes(addr, *((LONG *)addr));
#endif
        DoMethod(obj, MUIM_Dataspace_Add, addr, size, MUICFG_WindowPos);
#if (!AROS_BIG_ENDIAN)
        windowpos_swapbytes(addr, AROS_BE2LONG(*((LONG *)addr)));
#endif
    }
    return 0;
}

/**************************************************************************
 MUIM_Configdata_GetString
 Check if string is found in dataspace, then if not found, search each
 builtin array
**************************************************************************/
IPTR Configdata__MUIM_GetString(struct IClass *cl, Object *obj,
    struct MUIP_Configdata_GetString *msg)
{
    CONST_STRPTR s;

    s = (CONST_STRPTR) DoMethod(obj, MUIM_Dataspace_Find, msg->id);
    if (!s)
    {
        int i;

        for (i = 0; DefStrValues[i].id; i++)
        {
            if (DefStrValues[i].id == msg->id)
                return (IPTR) DefStrValues[i].val;
        }
        for (i = 0; DefImspecValues[i].defspec; i++)
        {
            if (DefImspecValues[i].cfgid == msg->id)
                return (IPTR) DefImspecValues[i].defspec;
        }
        for (i = 0; DefFramespecValues[i].defspec; i++)
        {
            if (DefFramespecValues[i].cfgid == msg->id)
                return (IPTR) DefFramespecValues[i].defspec;
        }
        return (IPTR) 0;
    }
    else
    {
        return (IPTR) s;
    }
}

/**************************************************************************
 MUIM_Configdata_SetImspec
 search in builtin array first, to not not have in dataspace the default
 value (would be redundant)
**************************************************************************/
IPTR Configdata__MUIM_SetImspec(struct IClass *cl, Object *obj,
    struct MUIP_Configdata_SetImspec *msg)
{
    int i;

    if (!msg->imspec || !*msg->imspec || *msg->imspec == '6')
    {
/*          D(bug("Configdata_SetImspec(%p) : id %08lx, val invalid\n", */
/*                obj, msg->id)); */
        return 0;
    }

    for (i = 0; DefImspecValues[i].defspec; i++)
    {
        if (DefImspecValues[i].cfgid == msg->id)
            if (!strcmp(DefImspecValues[i].defspec, msg->imspec))
            {
/*                  D(bug("Configdata_SetImspec(%p) : set to def, id %08lx, val %s\n", */
/*                        obj, msg->id, msg->imspec)); */
                DoMethod(obj, MUIM_Dataspace_Remove, msg->id);
                return 0;
            }
    }

    for (i = 0; DefStrValues[i].id; i++)
    {
        if (DefStrValues[i].id == msg->id)
            if (!strcmp(DefStrValues[i].val, msg->imspec))
            {
                DoMethod(obj, MUIM_Dataspace_Remove, msg->id);
                return 0;
            }
    }

    DoMethod(obj, MUIM_Dataspace_Add, (IPTR) msg->imspec,
        strlen(msg->imspec) + 1, msg->id);
    return 0;
}

/**************************************************************************
 MUIM_Configdata_SetFramespec
**************************************************************************/
IPTR Configdata__MUIM_SetFramespec(struct IClass *cl, Object *obj,
    struct MUIP_Configdata_SetFramespec *msg)
{
    int i;

    if (!msg->framespec || !*msg->framespec)
    {
/*          D(bug("Configdata_SetFramespec(%p) : id %08lx, val invalid\n", */
/*                obj, msg->id)); */
        return 0;
    }

    for (i = 0; DefFramespecValues[i].defspec; i++)
    {
        if (DefFramespecValues[i].cfgid == msg->id)
            if (!strcmp(DefFramespecValues[i].defspec, msg->framespec))
            {
/*                D(bug("Configdata_SetFramespec(%p): " */
/*                    "set to def, id %08lx, val %s\n", */
/*                    obj, msg->id, msg->framespec)); */
                DoMethod(obj, MUIM_Dataspace_Remove, msg->id);
                return 0;
            }
    }

    DoMethod(obj, MUIM_Dataspace_Add, (IPTR) msg->framespec,
        strlen(msg->framespec) + 1, msg->id);
    return 0;
}

/**************************************************************************
 MUIM_Configdata_SetPenspec
**************************************************************************/
IPTR Configdata__MUIM_SetPenspec(struct IClass *cl, Object *obj,
    struct MUIP_Configdata_SetPenspec *msg)
{
    int i;

    if (!msg->penspec || !*msg->penspec)
        return 0;

    for (i = 0; DefStrValues[i].id; i++)
    {
        if (DefStrValues[i].id == msg->id)
            if (!strcmp(DefStrValues[i].val, msg->penspec))
            {
                DoMethod(obj, MUIM_Dataspace_Remove, msg->id);
                return 0;
            }
    }

    DoMethod(obj, MUIM_Dataspace_Add, (IPTR) msg->penspec,
        strlen(msg->penspec) + 1, msg->id);
    return 0;
}

/**************************************************************************
 MUIM_Configdata_SetFont
**************************************************************************/
IPTR Configdata__MUIM_SetFont(struct IClass *cl, Object *obj,
    struct MUIP_Configdata_SetFont *msg)
{
    if (!msg->font || !*msg->font)
    {
/*          D(bug("Configdata_SetFont(%p) : id %08lx, val invalid\n", */
/*                obj, msg->id)); */
        DoMethod(obj, MUIM_Dataspace_Remove, msg->id);
        return 0;
    }

    DoMethod(obj, MUIM_Dataspace_Add, (IPTR) msg->font,
        strlen(msg->font) + 1, msg->id);
    return 0;
}

static inline void Configdata__SetKey(ZuneKeySpec *key, CONST_STRPTR val)
{
    if ((key->readable_hotkey = val) != NULL)
        key->ix_well =
            !ParseIX(key->readable_hotkey,
            &key->ix);
    else
        key->ix_well = 0;
}

/**************************************************************************
 MUIM_Configdata_SetString
**************************************************************************/
IPTR Configdata__MUIM_SetString(struct IClass *cl, Object *obj,
    struct MUIP_Configdata_SetString *msg)
{
    int i, def = 0;

    for (i = 0; DefStrValues[i].id; i++)
    {
        if (DefStrValues[i].id == msg->id)
            if (!strcmp(DefStrValues[i].val, msg->string))
            {
                DoMethod(obj, MUIM_Dataspace_Remove, msg->id);
                def = 1;
                break;
            }
    }

    if (def == 0) {
        DoMethod(obj, MUIM_Dataspace_Add, (IPTR) msg->string,
            strlen(msg->string) + 1, msg->id);
    }
    struct MUI_ConfigdataData *data = INST_DATA(cl, obj);
    switch (msg->id) {
    case MUICFG_Font_Normal:
        data->prefs.fonts[-MUIV_Font_Normal] = msg->string;
        break;

    case MUICFG_Font_List:
        data->prefs.fonts[-MUIV_Font_List] = msg->string;
        break;

    case MUICFG_Font_Tiny:
        data->prefs.fonts[-MUIV_Font_Tiny] = msg->string;
        break;

    case MUICFG_Font_Fixed:
        data->prefs.fonts[-MUIV_Font_Fixed] = msg->string;
        break;

    case MUICFG_Font_Title:
        data->prefs.fonts[-MUIV_Font_Title] = msg->string;
        break;

    case MUICFG_Font_Big:
        data->prefs.fonts[-MUIV_Font_Big] = msg->string;
        break;

    case MUICFG_Font_Button:
        data->prefs.fonts[-MUIV_Font_Button] = msg->string;
        break;

    case MUICFG_Font_Knob:
        data->prefs.fonts[-MUIV_Font_Knob] = msg->string;
        break;

    /*---------- system stuff ----------*/
    case MUICFG_PublicScreen:
        data->prefs.publicscreen_name = msg->string;
        break;

    case MUICFG_Iconification_Hotkey:
        data->prefs.iconification_hotkey = msg->string;
        break;

    /*---------- Strings ----------*/
    case MUICFG_String_Background:
        data->prefs.string_bg_inactive = msg->string;
        break;

    case MUICFG_String_Text:
        data->prefs.string_text_inactive = msg->string;
        break;

    case MUICFG_String_ActiveBackground:
        data->prefs.string_bg_active = msg->string;
        break;

    case MUICFG_String_ActiveText:
        data->prefs.string_text_active = msg->string;
        break;

    case MUICFG_String_Cursor:
        data->prefs.string_cursor = msg->string;
        break;

    case MUICFG_String_MarkedBackground:
        data->prefs.string_bg_marked = msg->string;
        break;

    case MUICFG_String_MarkedText:
        data->prefs.string_text_marked = msg->string;
        break;

    /*---------- Navigation ----------*/

    case MUICFG_Drag_LMBModifier:
        Configdata__SetKey(&data->prefs.drag_left_modifier, msg->string);
        break;

    case MUICFG_Drag_MMBModifier:
        Configdata__SetKey(&data->prefs.drag_middle_modifier, msg->string);
        break;

    case MUICFG_ActiveObject_Color:
        data->prefs.active_object_color = msg->string;
        break;

    /*---------- mui keys ----------*/
    case MUICFG_Keyboard_Press:
        Configdata__SetKey(&data->prefs.muikeys[MUIKEY_PRESS], msg->string);
        break;

    case MUICFG_Keyboard_Toggle:
        Configdata__SetKey(&data->prefs.muikeys[MUIKEY_TOGGLE], msg->string);
        break;

    case MUICFG_Keyboard_Up:
        Configdata__SetKey(&data->prefs.muikeys[MUIKEY_UP], msg->string);
        break;

    case MUICFG_Keyboard_Down:
        Configdata__SetKey(&data->prefs.muikeys[MUIKEY_DOWN], msg->string);
        break;

    case MUICFG_Keyboard_PageUp:
        Configdata__SetKey(&data->prefs.muikeys[MUIKEY_PAGEUP], msg->string);
        break;

    case MUICFG_Keyboard_PageDown:
        Configdata__SetKey(&data->prefs.muikeys[MUIKEY_PAGEDOWN], msg->string);
        break;

    case MUICFG_Keyboard_Top:
        Configdata__SetKey(&data->prefs.muikeys[MUIKEY_TOP], msg->string);
        break;

    case MUICFG_Keyboard_Bottom:
        Configdata__SetKey(&data->prefs.muikeys[MUIKEY_BOTTOM], msg->string);
        break;

    case MUICFG_Keyboard_Left:
        Configdata__SetKey(&data->prefs.muikeys[MUIKEY_LEFT], msg->string);
        break;

    case MUICFG_Keyboard_Right:
        Configdata__SetKey(&data->prefs.muikeys[MUIKEY_RIGHT], msg->string);
        break;

    case MUICFG_Keyboard_WordLeft:
        Configdata__SetKey(&data->prefs.muikeys[MUIKEY_WORDLEFT], msg->string);
        break;

    case MUICFG_Keyboard_WordRight:
        Configdata__SetKey(&data->prefs.muikeys[MUIKEY_WORDRIGHT], msg->string);
        break;

    case MUICFG_Keyboard_LineStart:
        Configdata__SetKey(&data->prefs.muikeys[MUIKEY_LINESTART], msg->string);
        break;

    case MUICFG_Keyboard_LineEnd:
        Configdata__SetKey(&data->prefs.muikeys[MUIKEY_LINEEND], msg->string);
        break;

    case MUICFG_Keyboard_NextGadget:
        Configdata__SetKey(&data->prefs.muikeys[MUIKEY_GADGET_NEXT], msg->string);
        break;

    case MUICFG_Keyboard_PrevGadget:
        Configdata__SetKey(&data->prefs.muikeys[MUIKEY_GADGET_PREV], msg->string);
        break;

    case MUICFG_Keyboard_GadgetOff:
        Configdata__SetKey(&data->prefs.muikeys[MUIKEY_GADGET_OFF], msg->string);
        break;

    case MUICFG_Keyboard_CloseWindow:
        Configdata__SetKey(&data->prefs.muikeys[MUIKEY_WINDOW_CLOSE], msg->string);
        break;

    case MUICFG_Keyboard_NextWindow:
        Configdata__SetKey(&data->prefs.muikeys[MUIKEY_WINDOW_NEXT], msg->string);
        break;

    case MUICFG_Keyboard_PrevWindow:
        Configdata__SetKey(&data->prefs.muikeys[MUIKEY_WINDOW_PREV], msg->string);
        break;

    case MUICFG_Keyboard_Help:
        Configdata__SetKey(&data->prefs.muikeys[MUIKEY_HELP], msg->string);
        break;

    case MUICFG_Keyboard_Popup:
        Configdata__SetKey(&data->prefs.muikeys[MUIKEY_POPUP], msg->string);
        break;

    /*---------- CustomFrames ----------*/
    case MUICFG_CustomFrame_1:
        data->prefs.customframe_config_1 = msg->string;
        break;
    case MUICFG_CustomFrame_2:
        data->prefs.customframe_config_2 = msg->string;
        break;
    case MUICFG_CustomFrame_3:
        data->prefs.customframe_config_3 = msg->string;
        break;
    case MUICFG_CustomFrame_4:
        data->prefs.customframe_config_4 = msg->string;
        break;
    case MUICFG_CustomFrame_5:
        data->prefs.customframe_config_5 = msg->string;
        break;
    case MUICFG_CustomFrame_6:
        data->prefs.customframe_config_6 = msg->string;
        break;
    case MUICFG_CustomFrame_7:
        data->prefs.customframe_config_7 = msg->string;
        break;
    case MUICFG_CustomFrame_8:
        data->prefs.customframe_config_8 = msg->string;
        break;
    case MUICFG_CustomFrame_9:
        data->prefs.customframe_config_9 = msg->string;
        break;
    case MUICFG_CustomFrame_10:
        data->prefs.customframe_config_10 = msg->string;
        break;
    case MUICFG_CustomFrame_11:
        data->prefs.customframe_config_11 = msg->string;
        break;
    case MUICFG_CustomFrame_12:
        data->prefs.customframe_config_12 = msg->string;
        break;
    case MUICFG_CustomFrame_13:
        data->prefs.customframe_config_13 = msg->string;
        break;
    case MUICFG_CustomFrame_14:
        data->prefs.customframe_config_14 = msg->string;
        break;
    case MUICFG_CustomFrame_15:
        data->prefs.customframe_config_15 = msg->string;
        break;
    case MUICFG_CustomFrame_16:
        data->prefs.customframe_config_16 = msg->string;
        break;
    }
    return 0;
}

/**************************************************************************
 MUIM_Configdata_GetULong
**************************************************************************/
IPTR Configdata__MUIM_GetULong(struct IClass *cl, Object *obj,
    struct MUIP_Configdata_GetULong *msg)
{
    ULONG *vp;

    vp = (ULONG *) DoMethod(obj, MUIM_Dataspace_Find, msg->id);
    if (!vp)
    {
        int i;

        for (i = 0; DefULValues[i].id != 0; i++)
        {
            if (DefULValues[i].id == msg->id)
                return DefULValues[i].val;
        }
        return 0;
    }
    else
    {
        return AROS_BE2LONG(*vp);
    }
}

/**************************************************************************
 MUIM_Configdata_SetULong
**************************************************************************/
IPTR Configdata__MUIM_SetULong(struct IClass *cl, Object *obj,
    struct MUIP_Configdata_SetULong *msg)
{
    ULONG v = msg->val;
    ULONG *vp = &v;
    int i;

    for (i = 0; DefULValues[i].id != 0; i++)
    {
        if (DefULValues[i].id == msg->id)
            if (DefULValues[i].val == v)
            {
/*                D(bug("Configdata_SetULong(%p):" */
/*                    " set to def, id %08lx, val %ld\n", */
/*                    obj, msg->id, v)); */
                DoMethod(obj, MUIM_Dataspace_Remove, msg->id);
                return 0;
            }
    }

    v = AROS_LONG2BE(v);
/*      D(bug("Configdata_SetULong(%p): adding %08lx to %08lx chunk\n", */
/*          obj, v, msg->id)); */
    DoMethod(obj, MUIM_Dataspace_Add, (IPTR) vp, 4, msg->id);
    return 0;
}


/**************************************************************************
 SavePrefsHeader: Write a PRHD chunk
**************************************************************************/
static int SavePrefsHeader(struct IFFHandle *iff)
{
    if (!PushChunk(iff, 0, MAKE_ID('P', 'R', 'H', 'D'), IFFSIZE_UNKNOWN))
    {
        struct PrefHeader ph;
        ph.ph_Version = 0;
        ph.ph_Type = 0;
        ph.ph_Flags = 0;

        if (WriteChunkBytes(iff, &ph, sizeof(struct PrefHeader)))
            if (!PopChunk(iff))
                return 1;
        PopChunk(iff);
    }
    return 0;
}

/**************************************************************************
 MUIM_Configdata_Save
**************************************************************************/
IPTR Configdata__MUIM_Save(struct IClass *cl, Object *obj,
    struct MUIP_Configdata_Save *msg)
{
    struct IFFHandle *iff;
    if ((iff = AllocIFF()))
    {
        if (!(iff->iff_Stream = (IPTR) Open(msg->filename, MODE_NEWFILE)))
        {
            /* Try to Create the directory where the file is located */
            char *path = StrDup(msg->filename);
            if (path)
            {
                char *path_end = PathPart(path);
                if (path_end != path)
                {
                    BPTR lock;
                    *path_end = 0;
                    if ((lock = CreateDir(path)))
                    {
                        UnLock(lock);
                        iff->iff_Stream =
                            (IPTR) Open(msg->filename, MODE_NEWFILE);
                    }
                }
                FreeVec(path);
            }
        }

        if (iff->iff_Stream)
        {
            InitIFFasDOS(iff);

            if (!OpenIFF(iff, IFFF_WRITE))
            {
                if (!PushChunk(iff, MAKE_ID('P', 'R', 'E', 'F'), ID_FORM,
                        IFFSIZE_UNKNOWN))
                {
                    Configdata_SetWindowPos(cl, obj, (APTR) msg);
                    if (SavePrefsHeader(iff))
                    {
                        DoMethod(obj, MUIM_Dataspace_WriteIFF, (IPTR) iff,
                            0, MAKE_ID('M', 'U', 'I', 'C'));
                    }
                    PopChunk(iff);
                }

                CloseIFF(iff);
            }
            Close((BPTR) iff->iff_Stream);
        }
        FreeIFF(iff);
    }
    return 0;
}


/**************************************************************************
 MUIM_Configdata_Load
 Get the content of the file into the object.
**************************************************************************/
IPTR Configdata__MUIM_Load(struct IClass *cl, Object *obj,
    struct MUIP_Configdata_Load *msg)
{
    struct IFFHandle *iff;
    IPTR res = TRUE;

    if ((iff = AllocIFF()))
    {
        D(bug("loading prefs from %s\n", msg->filename));
        if ((iff->iff_Stream = (IPTR) Open(msg->filename, MODE_OLDFILE)))
        {
            InitIFFasDOS(iff);

            if (!OpenIFF(iff, IFFF_READ))
            {
                StopChunk(iff, MAKE_ID('P', 'R', 'E', 'F'), MAKE_ID('M',
                        'U', 'I', 'C'));

                while (!ParseIFF(iff, IFFPARSE_SCAN))
                {
                    struct ContextNode *cn;
                    if (!(cn = CurrentChunk(iff)))
                        continue;
                    if (cn->cn_ID == MAKE_ID('M', 'U', 'I', 'C'))
                        DoMethod(obj, MUIM_Dataspace_ReadIFF, (IPTR) iff);
                }

                CloseIFF(iff);
            }
            else
            {
                res = FALSE;
            }
            Close((BPTR) iff->iff_Stream);
        }
        else
        {
            res = FALSE;
        }
        FreeIFF(iff);
    }
    else
    {
        res = FALSE;
    }

    if (res) {
        DoMethod(obj, MUIM_Configdata_LoadPubScreens);
    }

    return res;
}

static IPTR Configdata_GetPubScrnDesc(struct IClass *cl, Object *obj,
    struct MUIP_Configdata_GetPubScrnDesc *msg)
{
    struct MUI_ConfigdataData *data = INST_DATA(cl, obj);
    struct MUI_PubScreenDesc *desc = NULL;

    DPSD(bug("[MUI:Cfg] %s()\n", __func__);)

    if (MUIScreenBase) {
        struct Node *psNode;
        ObtainSemaphoreShared(&data->psLock);
        ForeachNode(&data->pubscreens, psNode) {
            if (!strcmp(((struct MUI_PubScreenDesc *)(psNode->ln_Name))->Name, msg->name)) {
                desc = (struct MUI_PubScreenDesc *)psNode->ln_Name;
                break;
            }
        }
        ReleaseSemaphore(&data->psLock);
    }
    return (IPTR)desc;
}

static IPTR Configdata_LoadPubScreens(struct IClass *cl, Object *obj,
    Msg msg)
{
    DPSD(bug("[MUI:Cfg] %s()\n", __func__);)

    if (MUIScreenBase) {
        struct MUI_ConfigdataData *data = INST_DATA(cl, obj);
        struct Node *psNode, *tmpName;
        APTR pfh;

        Configdata__DisposeDescriptors(data);

        DPSD(bug("[MUI:Cfg] %s: loading public screen details from '%s'\n", __func__, PUBSCREEN_FILE);)

        ObtainSemaphore(&data->psLock);
        if ((pfh = MUIS_OpenPubFile(PUBSCREEN_FILE, MODE_OLDFILE))) {
            struct MUI_PubScreenDesc *desc;
            while ((desc = MUIS_ReadPubFile(pfh))) {
                DPSD(bug("[MUI:Cfg] %s: descriptor @ 0x%p\n", __func__, desc);)
                psNode = AllocVec(sizeof(struct Node), MEMF_CLEAR);
                psNode->ln_Name = (char *)desc;
                AddTail(&data->pubscreens, psNode);
            }
            MUIS_ClosePubFile(pfh);
        }
        ReleaseSemaphore(&data->psLock);
    }
    return (IPTR)TRUE;
}

/*
 * The class dispatcher
 */
BOOPSI_DISPATCHER(IPTR, Configdata_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Configdata__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Configdata__OM_DISPOSE(cl, obj, (APTR) msg);
    case OM_GET:
        return Configdata__OM_GET(cl, obj, (APTR) msg);
    case MUIM_Configdata_GetString:
        return Configdata__MUIM_GetString(cl, obj, (APTR) msg);
    case MUIM_Configdata_GetULong:
        return Configdata__MUIM_GetULong(cl, obj, (APTR) msg);
    case MUIM_Configdata_SetULong:
        return Configdata__MUIM_SetULong(cl, obj, (APTR) msg);
    case MUIM_Configdata_SetImspec:
        return Configdata__MUIM_SetImspec(cl, obj, (APTR) msg);
    case MUIM_Configdata_SetFramespec:
        return Configdata__MUIM_SetFramespec(cl, obj, (APTR) msg);
    case MUIM_Configdata_SetPenspec:
        return Configdata__MUIM_SetPenspec(cl, obj, (APTR) msg);
    case MUIM_Configdata_SetFont:
        return Configdata__MUIM_SetFont(cl, obj, (APTR) msg);
    case MUIM_Configdata_SetString:
        return Configdata__MUIM_SetString(cl, obj, (APTR) msg);
    case MUIM_Configdata_Save:
        return Configdata__MUIM_Save(cl, obj, (APTR) msg);
    case MUIM_Configdata_Load:
        return Configdata__MUIM_Load(cl, obj, (APTR) msg);
    case MUIM_Configdata_SetWindowPos:
        return Configdata_SetWindowPos(cl, obj, (APTR) msg);
    case MUIM_Configdata_GetWindowPos:
        return Configdata_GetWindowPos(cl, obj, (APTR) msg);
    case MUIM_Configdata_GetPubScrnDesc:
        return Configdata_GetPubScrnDesc(cl, obj, (APTR) msg);
    case MUIM_Configdata_LoadPubScreens:
        return Configdata_LoadPubScreens(cl, obj, (APTR) msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Configdata_desc =
{
    MUIC_Configdata,            /* Class name */
    MUIC_Dataspace,             /* super class name */
    sizeof(struct MUI_ConfigdataData),  /* size of class own datas */
    (void *) Configdata_Dispatcher       /* class dispatcher */
};
