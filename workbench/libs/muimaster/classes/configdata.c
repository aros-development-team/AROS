/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
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

extern struct Library *MUIMasterBase;

struct MUI_ConfigdataData
{
    Object             *app;
    CONST_STRPTR        appbase;
    struct ZunePrefsNew prefs;
};


static CONST_STRPTR GetConfigString(Object *obj, ULONG id)
{
    return (CONST_STRPTR)DoMethod(obj, MUIM_Configdata_GetString, id);
}

static ULONG GetConfigULong(Object *obj, ULONG id)
{
    return (ULONG)DoMethod(obj, MUIM_Configdata_GetULong, id);
}


/**************************************************************************
 Default ImageSpec values
**************************************************************************/

struct spec_cfg {
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
static void init_imspecs (Object *obj, struct MUI_ConfigdataData *data)
{
    int i;

    for (i = 0; DefImspecValues[i].defspec; i++)
    {
	CONST_STRPTR imspec;
	struct spec_cfg *img = DefImspecValues + i;

	imspec = GetConfigString(obj, img->cfgid);
/*  	D(bug("init_imspecs: %ld %lx %s ...\n", img->muiv, img->cfgid, imspec)); */
	data->prefs.imagespecs[img->muiv] = imspec;
	if (!data->prefs.imagespecs[img->muiv])
	{
/*  	    D(bug("*** init_imspecs: null imagespec %ld\n", img->muiv)); */
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
static void init_framespecs (Object *obj, struct MUI_ConfigdataData *data)
{
    int i;

    for (i = 0; DefFramespecValues[i].defspec; i++)
    {
	CONST_STRPTR framespec;
	struct spec_cfg *fcfg = DefFramespecValues + i;

	framespec = GetConfigString(obj, fcfg->cfgid);
	zune_frame_spec_to_intern(framespec,
				  &data->prefs.frames[fcfg->muiv]);
    }
}

/**************************************************************************
 Default ULONG values
**************************************************************************/

struct def_ulval {
    ULONG id;
    ULONG val;
};

const static struct def_ulval DefULValues[] =
{
    { MUICFG_Window_Spacing_Left,     4 },
    { MUICFG_Window_Spacing_Right,    4 },
    { MUICFG_Window_Spacing_Top,      3 },
    { MUICFG_Window_Spacing_Bottom,   3 },
    { MUICFG_Window_Positions,        WINDOW_POSITION_FORGET_ON_EXIT },
    { MUICFG_Window_Redraw,           WINDOW_REDRAW_WITHOUT_CLEAR },
    { MUICFG_Radio_HSpacing,          4 },
    { MUICFG_Radio_VSpacing,          1 },
    { MUICFG_Group_HSpacing,          6 },
    { MUICFG_Group_VSpacing,          3 },    
    { MUICFG_Cycle_MenuCtrl_Position, CYCLE_MENU_POSITION_BELOW },
    { MUICFG_Cycle_MenuCtrl_Level,    2 },
    { MUICFG_Cycle_MenuCtrl_Speed,    0 },
    { MUICFG_Cycle_Menu_Recessed,     FALSE },
    { MUICFG_Listview_Font_Leading,   1 },
    { MUICFG_Listview_Smoothed,       FALSE },
    { MUICFG_Listview_SmoothVal,      0 },
    { MUICFG_Listview_Refresh,        LISTVIEW_REFRESH_MIXED },
    { MUICFG_Listview_Multi,          LISTVIEW_MULTI_SHIFTED },
    { MUICFG_GroupTitle_Position,     GROUP_TITLE_POSITION_CENTERED },
    { MUICFG_GroupTitle_Color,        GROUP_TITLE_COLOR_HILITE },
    { MUICFG_Scrollbar_Type,          SCROLLBAR_TYPE_STANDARD },
    { MUICFG_Scrollbar_Arrangement,   SCROLLBAR_ARRANGEMENT_TOP },
    { MUICFG_Balance_Look,            BALANCING_SHOW_FRAMES },
    { MUICFG_Dragndrop_Look,          DND_LOOK_GHOSTED_ON_BOX },
    { MUICFG_Drag_Autostart,          TRUE },
    { MUICFG_Drag_Autostart_Length,   3 },
    { MUICFG_Drag_LeftButton,         TRUE },
    { MUICFG_Drag_MiddleButton,       TRUE },
    { 0, 0 },
};

/**************************************************************************
 Default string values
**************************************************************************/

struct def_strval {
    ULONG id;
    CONST_STRPTR val;
};

/* NULL values not allowed */
const static struct def_strval DefStrValues[] =
{
    { MUICFG_Font_Normal,   "" },
    { MUICFG_Font_List,     "" },
    { MUICFG_Font_Tiny,     "" },
    { MUICFG_Font_Fixed,    "" },
    { MUICFG_Font_Title,    "" },
    { MUICFG_Font_Big,      "" },
    { MUICFG_Font_Button,   "" },
    { MUICFG_Font_Knob,     "" },
    { MUICFG_String_Background,         "2:m2" },
    { MUICFG_String_Text,               "m5" },
    { MUICFG_String_ActiveBackground,   "2:m1" },
    { MUICFG_String_ActiveText,         "m5" },
    { MUICFG_String_Cursor,             "m7" },
    { MUICFG_ActiveObject_Color,        "m0" },
    { MUICFG_Keyboard_Press,            "-upstroke return" },
    { MUICFG_Keyboard_Toggle,           "-repeat space" },
    { MUICFG_Keyboard_Up,               "-repeat up" },
    { MUICFG_Keyboard_Down,             "-repeat down" },
    { MUICFG_Keyboard_PageUp,           "-repeat shift up" },
    { MUICFG_Keyboard_PageDown,         "-repeat shift down" },
    { MUICFG_Keyboard_Top,              "control up" },
    { MUICFG_Keyboard_Bottom,           "control down" },
    { MUICFG_Keyboard_Left,             "-repeat left" },
    { MUICFG_Keyboard_Right,            "-repeat right" },
    { MUICFG_Keyboard_WordLeft,         "-repeat control left" },
    { MUICFG_Keyboard_WordRight,        "-repeat control right" },
    { MUICFG_Keyboard_LineStart,        "shift left" },
    { MUICFG_Keyboard_LineEnd,          "shift right" },
    { MUICFG_Keyboard_NextGadget,       "-repeat tab" },
    { MUICFG_Keyboard_PrevGadget,       "-repeat shift tab" },
    { MUICFG_Keyboard_GadgetOff,        "control tab" },
    { MUICFG_Keyboard_CloseWindow,      "esc" },
    { MUICFG_Keyboard_NextWindow,       "-repeat alt tab" },
    { MUICFG_Keyboard_PrevWindow,       "-repeat alt shift tab" },
    { MUICFG_Keyboard_Help,             "help" },
    { MUICFG_Keyboard_Popup,            "control p" },
    { MUICFG_Drag_LMBModifier,          "control" },
    { MUICFG_Drag_MMBModifier,          "" },
    { 0, 0 },
};


/**************************************************************************
 OM_NEW
 Load global (and maybe application-specific) prefs files into the dataspace,
 then fill the prefs struct with dataspace or default values
**************************************************************************/
static ULONG Configdata_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ConfigdataData *data;
    struct TagItem *tags,*tag;
    //APTR cdata;
    int i;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return NULL;

/*      D(bug("Configdata_New(%p)\n", obj)); */

    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Configdata_Application:
		data->app = (Object *)tag->ti_Data;
		break;
	    case MUIA_Configdata_ApplicationBase:
		data->appbase = (CONST_STRPTR)tag->ti_Data;
		break;
	}
    }

    if (!DoMethod(obj, MUIM_Configdata_Load, (IPTR)"ENV:zune/global.prefs"))
    {
	DoMethod(obj, MUIM_Configdata_Load, (IPTR)"ENVARC:zune/global.prefs");
    }

    if (data->app && !data->appbase)
    {
	get(data->app, MUIA_Application_Base, &data->appbase);
    }

    if (data->appbase)
    {
	char filename[255];
	snprintf(filename, 255, "ENV:zune/%s.prefs", data->appbase);
	if (!DoMethod(obj, MUIM_Configdata_Load, (IPTR)filename))
	{
	    snprintf(filename, 255, "ENVARC:zune/%s.prefs", data->appbase);
	    DoMethod(obj, MUIM_Configdata_Load, (IPTR)filename);
	}
    }

    /*---------- fonts stuff ----------*/

    data->prefs.fonts[-MUIV_Font_Normal] = GetConfigString(obj, MUICFG_Font_Normal);
    data->prefs.fonts[-MUIV_Font_List] =   GetConfigString(obj, MUICFG_Font_List);
    data->prefs.fonts[-MUIV_Font_Tiny] =   GetConfigString(obj, MUICFG_Font_Tiny);
    data->prefs.fonts[-MUIV_Font_Fixed] =  GetConfigString(obj, MUICFG_Font_Fixed);
    data->prefs.fonts[-MUIV_Font_Title] =  GetConfigString(obj, MUICFG_Font_Title);
    data->prefs.fonts[-MUIV_Font_Big] =    GetConfigString(obj, MUICFG_Font_Big);
    data->prefs.fonts[-MUIV_Font_Button] = GetConfigString(obj, MUICFG_Font_Button);
    data->prefs.fonts[-MUIV_Font_Knob] =   GetConfigString(obj, MUICFG_Font_Knob);

    /*---------- images stuff ----------*/

    init_imspecs(obj, data);

    /*---------- frame stuff ----------*/

    init_framespecs(obj, data);

    /*---------- window stuff ----------*/

    data->prefs.window_inner_left = GetConfigULong(obj, MUICFG_Window_Spacing_Left);
    data->prefs.window_inner_right = GetConfigULong(obj, MUICFG_Window_Spacing_Right);
    data->prefs.window_inner_top = GetConfigULong(obj, MUICFG_Window_Spacing_Top);
    data->prefs.window_inner_bottom = GetConfigULong(obj, MUICFG_Window_Spacing_Bottom);
    data->prefs.window_position = GetConfigULong(obj, MUICFG_Window_Positions);
    data->prefs.window_redraw = GetConfigULong(obj, MUICFG_Window_Redraw);

    /*---------- group stuff ----------*/

    data->prefs.group_title_position = GetConfigULong(obj, MUICFG_GroupTitle_Position);
    data->prefs.group_title_color = GetConfigULong(obj, MUICFG_GroupTitle_Color);
    data->prefs.group_hspacing = GetConfigULong(obj, MUICFG_Group_HSpacing);
    data->prefs.group_vspacing = GetConfigULong(obj, MUICFG_Group_VSpacing);

    /*---------- registers ----------*/

    data->prefs.register_look = REGISTER_LOOK_TRADITIONAL;
    data->prefs.register_truncate_titles = FALSE; /* loosers want full titles */

    /*---------- Buttons ----------*/

    data->prefs.radiobutton_hspacing = GetConfigULong(obj, MUICFG_Radio_HSpacing);
    data->prefs.radiobutton_vspacing = GetConfigULong(obj, MUICFG_Radio_VSpacing);

    /*---------- Cycles ----------*/

    data->prefs.cycle_menu_position = GetConfigULong(obj, MUICFG_Cycle_MenuCtrl_Position);
    data->prefs.cycle_menu_min_entries = GetConfigULong(obj, MUICFG_Cycle_MenuCtrl_Level);
    data->prefs.cycle_menu_speed = GetConfigULong(obj, MUICFG_Cycle_MenuCtrl_Speed);
    data->prefs.cycle_menu_recessed_entries = GetConfigULong(obj, MUICFG_Cycle_Menu_Recessed);

    /*---------- Sliders ----------*/
    /* all taken care of in frames and images */

    /*---------- Scrollbars ----------*/

    data->prefs.scrollbar_type = GetConfigULong(obj, MUICFG_Scrollbar_Type);
    data->prefs.scrollbar_arrangement = GetConfigULong(obj, MUICFG_Scrollbar_Arrangement);

    /*---------- Lists ----------*/

    data->prefs.list_linespacing = GetConfigULong(obj, MUICFG_Listview_Font_Leading);
    data->prefs.list_smoothed = GetConfigULong(obj, MUICFG_Listview_Smoothed);
    data->prefs.list_smoothval = GetConfigULong(obj, MUICFG_Listview_SmoothVal);
    data->prefs.list_multi = GetConfigULong(obj, MUICFG_Listview_Multi);
    data->prefs.list_refresh = GetConfigULong(obj, MUICFG_Listview_Refresh);

    /*---------- Strings ----------*/
    data->prefs.string_bg_inactive = GetConfigString(obj, MUICFG_String_Background);
    data->prefs.string_text_inactive = GetConfigString(obj, MUICFG_String_Text);
    data->prefs.string_bg_active = GetConfigString(obj, MUICFG_String_ActiveBackground);
    data->prefs.string_text_active = GetConfigString(obj, MUICFG_String_ActiveText);
    data->prefs.string_cursor = GetConfigString(obj, MUICFG_String_Cursor);

    /*---------- Navigation ----------*/

    data->prefs.drag_left_button = GetConfigULong(obj, MUICFG_Drag_LeftButton);
    data->prefs.drag_left_modifier.readable_hotkey = GetConfigString(obj, MUICFG_Drag_LMBModifier);
    data->prefs.drag_middle_button = GetConfigULong(obj, MUICFG_Drag_MiddleButton);
    data->prefs.drag_middle_modifier.readable_hotkey = GetConfigString(obj, MUICFG_Drag_MMBModifier);
    data->prefs.drag_autostart = GetConfigULong(obj, MUICFG_Drag_Autostart);
    data->prefs.drag_autostart_length = GetConfigULong(obj, MUICFG_Drag_Autostart_Length);
    data->prefs.drag_look = GetConfigULong(obj, MUICFG_Dragndrop_Look);
    data->prefs.balancing_look = GetConfigULong(obj, MUICFG_Balance_Look);

    if (data->prefs.drag_left_modifier.readable_hotkey != NULL)
	data->prefs.drag_left_modifier.ix_well = 
	    !ParseIX(data->prefs.drag_left_modifier.readable_hotkey,
		     &data->prefs.drag_left_modifier.ix);
    else
	data->prefs.drag_left_modifier.ix_well = 0;

    if (data->prefs.drag_middle_modifier.readable_hotkey != NULL)
	data->prefs.drag_middle_modifier.ix_well = 
	    !ParseIX(data->prefs.drag_middle_modifier.readable_hotkey,
		     &data->prefs.drag_middle_modifier.ix);
    else
	data->prefs.drag_middle_modifier.ix_well = 0;

    data->prefs.active_object_color = GetConfigString(obj, MUICFG_ActiveObject_Color);
    /*---------- mui keys ----------*/

    data->prefs.muikeys[MUIKEY_PRESS].readable_hotkey = GetConfigString(obj, MUICFG_Keyboard_Press);
    data->prefs.muikeys[MUIKEY_TOGGLE].readable_hotkey = GetConfigString(obj, MUICFG_Keyboard_Toggle);
    data->prefs.muikeys[MUIKEY_UP].readable_hotkey = GetConfigString(obj, MUICFG_Keyboard_Up);
    data->prefs.muikeys[MUIKEY_DOWN].readable_hotkey = GetConfigString(obj, MUICFG_Keyboard_Down);
    data->prefs.muikeys[MUIKEY_PAGEUP].readable_hotkey = GetConfigString(obj, MUICFG_Keyboard_PageUp);
    data->prefs.muikeys[MUIKEY_PAGEDOWN].readable_hotkey = GetConfigString(obj, MUICFG_Keyboard_PageDown);
    data->prefs.muikeys[MUIKEY_TOP].readable_hotkey = GetConfigString(obj, MUICFG_Keyboard_Top);
    data->prefs.muikeys[MUIKEY_BOTTOM].readable_hotkey = GetConfigString(obj, MUICFG_Keyboard_Bottom);
    data->prefs.muikeys[MUIKEY_LEFT].readable_hotkey = GetConfigString(obj, MUICFG_Keyboard_Left);
    data->prefs.muikeys[MUIKEY_RIGHT].readable_hotkey = GetConfigString(obj, MUICFG_Keyboard_Right);
    data->prefs.muikeys[MUIKEY_WORDLEFT].readable_hotkey = GetConfigString(obj, MUICFG_Keyboard_WordLeft);
    data->prefs.muikeys[MUIKEY_WORDRIGHT].readable_hotkey = GetConfigString(obj, MUICFG_Keyboard_WordRight);
    data->prefs.muikeys[MUIKEY_LINESTART].readable_hotkey = GetConfigString(obj, MUICFG_Keyboard_LineStart);
    data->prefs.muikeys[MUIKEY_LINEEND].readable_hotkey = GetConfigString(obj, MUICFG_Keyboard_LineEnd);
    data->prefs.muikeys[MUIKEY_GADGET_NEXT].readable_hotkey = GetConfigString(obj, MUICFG_Keyboard_NextGadget);
    data->prefs.muikeys[MUIKEY_GADGET_PREV].readable_hotkey = GetConfigString(obj, MUICFG_Keyboard_PrevGadget);
    data->prefs.muikeys[MUIKEY_GADGET_OFF].readable_hotkey = GetConfigString(obj, MUICFG_Keyboard_GadgetOff);
    data->prefs.muikeys[MUIKEY_WINDOW_CLOSE].readable_hotkey = GetConfigString(obj, MUICFG_Keyboard_CloseWindow);
    data->prefs.muikeys[MUIKEY_WINDOW_NEXT].readable_hotkey = GetConfigString(obj, MUICFG_Keyboard_NextWindow);
    data->prefs.muikeys[MUIKEY_WINDOW_PREV].readable_hotkey = GetConfigString(obj, MUICFG_Keyboard_PrevWindow);
    data->prefs.muikeys[MUIKEY_HELP].readable_hotkey = GetConfigString(obj, MUICFG_Keyboard_Help);
    data->prefs.muikeys[MUIKEY_POPUP].readable_hotkey = GetConfigString(obj, MUICFG_Keyboard_Popup);

    for (i = 0; i < MUIKEY_COUNT; i++)
    {
    	if (data->prefs.muikeys[i].readable_hotkey)
	    data->prefs.muikeys[i].ix_well = !ParseIX(data->prefs.muikeys[i].readable_hotkey, &data->prefs.muikeys[i].ix);
	else data->prefs.muikeys[i].ix_well = 0;
    }

    /*---------- Special ----------*/
    /* all taken care of in frames and images */

    return (ULONG)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static ULONG Configdata_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
/*      struct MUI_ConfigdataData *data = INST_DATA(cl, obj); */
/*      int i; */

    return DoSuperMethodA(cl,obj,msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
static ULONG  Configdata_Get(struct IClass *cl, Object * obj, struct opGet *msg)
{
    struct MUI_ConfigdataData *data = INST_DATA(cl, obj);
    ULONG *store = msg->opg_Storage;
    ULONG    tag = msg->opg_AttrID;

    switch (tag)
    {
	case 	MUIA_Configdata_ZunePrefs:
		*store = (ULONG)&data->prefs;
		return 1;
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}


/**************************************************************************
 MUIM_Configdata_GetString
 Check if string is found in dataspace, then if not found, search each
 builtin array
**************************************************************************/
static IPTR Configdata_GetString(struct IClass *cl, Object * obj,
				 struct MUIP_Configdata_GetString *msg)
{
    CONST_STRPTR s;

    s = (CONST_STRPTR)DoMethod(obj, MUIM_Dataspace_Find, msg->id);
    if (!s)
    {
	int i;

	for (i = 0; DefStrValues[i].id; i++)
	{
	    if (DefStrValues[i].id == msg->id)
		return (IPTR)DefStrValues[i].val;
	}
	for (i = 0; DefImspecValues[i].defspec; i++)
	{
	    if (DefImspecValues[i].cfgid == msg->id)
		return (IPTR)DefImspecValues[i].defspec;
	}
	for (i = 0; DefFramespecValues[i].defspec; i++)
	{
	    if (DefFramespecValues[i].cfgid == msg->id)
		return (IPTR)DefFramespecValues[i].defspec;
	}
	return (IPTR)0;
    }
    else
    {
	return (IPTR)s;
    }
}

/**************************************************************************
 MUIM_Configdata_SetImspec
 search in builtin array first, to not not have in dataspace the default
 value (would be redundant)
**************************************************************************/
static IPTR Configdata_SetImspec(struct IClass *cl, Object * obj,
				 struct MUIP_Configdata_SetImspec *msg)
{
    int i;

    if (!msg->imspec || !*msg->imspec || *msg->imspec == '6')
    {
/*  	D(bug("Configdata_SetImspec(%p) : id %08lx, val invalid\n", */
/*  	      obj, msg->id)); */
	return 0;
    }

    for (i = 0; DefImspecValues[i].defspec; i++)
    {
	if (DefImspecValues[i].cfgid == msg->id)
	    if (!strcmp(DefImspecValues[i].defspec, msg->imspec))
	    {
/*  		D(bug("Configdata_SetImspec(%p) : set to def, id %08lx, val %s\n", */
/*  		      obj, msg->id, msg->imspec)); */
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

    DoMethod(obj, MUIM_Dataspace_Add, (IPTR)msg->imspec, strlen(msg->imspec) + 1, msg->id);
    return 0;
}

/**************************************************************************
 MUIM_Configdata_SetFramespec
**************************************************************************/
static IPTR Configdata_SetFramespec(struct IClass *cl, Object * obj,
				    struct MUIP_Configdata_SetFramespec *msg)
{
    int i;

    if (!msg->framespec || !*msg->framespec)
    {
/*  	D(bug("Configdata_SetFramespec(%p) : id %08lx, val invalid\n", */
/*  	      obj, msg->id)); */
	return 0;
    }

    for (i = 0; DefFramespecValues[i].defspec; i++)
    {
	if (DefFramespecValues[i].cfgid == msg->id)
	    if (!strcmp(DefFramespecValues[i].defspec, msg->framespec))
	    {
/*  		D(bug("Configdata_SetFramespec(%p) : set to def, id %08lx, val %s\n", */
/*  		      obj, msg->id, msg->framespec)); */
		DoMethod(obj, MUIM_Dataspace_Remove, msg->id);		
		return 0;
	    }
    }

    DoMethod(obj, MUIM_Dataspace_Add, (IPTR)msg->framespec,
	     strlen(msg->framespec) + 1, msg->id);
    return 0;
}

/**************************************************************************
 MUIM_Configdata_SetPenspec
**************************************************************************/
static IPTR Configdata_SetPenspec(struct IClass *cl, Object * obj,
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

    DoMethod(obj, MUIM_Dataspace_Add, (IPTR)msg->penspec,
	     strlen(msg->penspec) + 1, msg->id);
    return 0;
}

/**************************************************************************
 MUIM_Configdata_SetFont
**************************************************************************/
static IPTR Configdata_SetFont(struct IClass *cl, Object * obj,
				 struct MUIP_Configdata_SetFont *msg)
{
    if (!msg->font || !*msg->font)
    {
/*  	D(bug("Configdata_SetFont(%p) : id %08lx, val invalid\n", */
/*  	      obj, msg->id)); */
	DoMethod(obj, MUIM_Dataspace_Remove, msg->id);
	return 0;
    }

    DoMethod(obj, MUIM_Dataspace_Add, (IPTR)msg->font, strlen(msg->font) + 1, msg->id);
    return 0;
}

/**************************************************************************
 MUIM_Configdata_SetString
**************************************************************************/
static IPTR Configdata_SetString(struct IClass *cl, Object * obj,
				 struct MUIP_Configdata_SetString *msg)
{
    int i;

    for (i = 0; DefStrValues[i].id; i++)
    {
	if (DefStrValues[i].id == msg->id)
	    if (!strcmp(DefStrValues[i].val, msg->string))
	    {
		DoMethod(obj, MUIM_Dataspace_Remove, msg->id);		
		return 0;
	    }
    }

    DoMethod(obj, MUIM_Dataspace_Add, (IPTR)msg->string, strlen(msg->string) + 1, msg->id);
    return 0;
}

/**************************************************************************
 MUIM_Configdata_GetULong
**************************************************************************/
static ULONG Configdata_GetULong(struct IClass *cl, Object * obj,
				 struct MUIP_Configdata_GetULong *msg)
{
    ULONG *vp;

    vp = (ULONG *)DoMethod(obj, MUIM_Dataspace_Find, msg->id);
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
static ULONG Configdata_SetULong(struct IClass *cl, Object * obj,
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
/*  		D(bug("Configdata_SetULong(%p) : set to def, id %08lx, val %ld\n", */
/*  		      obj, msg->id, v)); */
		DoMethod(obj, MUIM_Dataspace_Remove, msg->id);		
		return 0;
	    }
    }

    v = AROS_LONG2BE(v);
/*      D(bug("Configdata_SetULong(%p): adding %08lx to %08lx chunk\n", obj, v, msg->id)); */
    DoMethod(obj, MUIM_Dataspace_Add, (IPTR)vp, 4, msg->id);
    return 0;
}


/**************************************************************************
 SavePrefsHeader: Write a PRHD chunk
**************************************************************************/
static int SavePrefsHeader(struct IFFHandle *iff)
{
    if (!PushChunk( iff, 0, MAKE_ID('P','R','H','D'), IFFSIZE_UNKNOWN))
    {
	struct PrefHeader ph;
	ph.ph_Version = 0;
	ph.ph_Type = 0;
	ph.ph_Flags = 0;

	if (WriteChunkBytes(iff, &ph, sizeof(struct PrefHeader)))
	    if (!PopChunk(iff)) return 1;
	PopChunk(iff);
    }
    return 0;
}

/**************************************************************************
 MUIM_Configdata_Save
**************************************************************************/
static ULONG Configdata_Save(struct IClass *cl, Object * obj,
			     struct MUIP_Configdata_Save *msg)
{
    struct IFFHandle *iff;
    if ((iff = AllocIFF()))
    {
    	if (!(iff->iff_Stream = (IPTR)Open(msg->filename,MODE_NEWFILE)))
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
			iff->iff_Stream = (IPTR)Open(msg->filename,MODE_NEWFILE);
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
		if (!PushChunk(iff, MAKE_ID('P','R','E','F'), ID_FORM, IFFSIZE_UNKNOWN))
		{
		    if (SavePrefsHeader(iff))
		    {
		    	DoMethod(obj,MUIM_Dataspace_WriteIFF, (IPTR)iff, 0, MAKE_ID('M','U','I','C'));
		    }
		    PopChunk(iff);
		}

		CloseIFF(iff);
	    }
	    Close((BPTR)iff->iff_Stream);
	}
	FreeIFF(iff);
    }
    return 0;
}


/**************************************************************************
 MUIM_Configdata_Load
 Get the content of the file into the object.
**************************************************************************/
static ULONG Configdata_Load(struct IClass *cl, Object * obj,
			     struct MUIP_Configdata_Load *msg)
{
    struct IFFHandle *iff;
    ULONG res = TRUE;

    if ((iff = AllocIFF()))
    {
	D(bug("loading prefs from %s\n", msg->filename));
	if ((iff->iff_Stream = (IPTR)Open(msg->filename,MODE_OLDFILE)))
	{
	    InitIFFasDOS(iff);

	    if (!OpenIFF(iff, IFFF_READ))
	    {
		StopChunk( iff, MAKE_ID('P','R','E','F'), MAKE_ID('M','U','I','C'));

		while (!ParseIFF(iff, IFFPARSE_SCAN))
		{
		    struct ContextNode *cn;
		    if (!(cn = CurrentChunk(iff))) continue;
		    if (cn->cn_ID == MAKE_ID('M','U','I','C'))
			DoMethod(obj, MUIM_Dataspace_ReadIFF, (IPTR)iff);
		}

		CloseIFF(iff);
	    }
	    else
	    {
		res = FALSE;
	    }
	    Close((BPTR)iff->iff_Stream);
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
    return TRUE;
}


/*
 * The class dispatcher
 */
BOOPSI_DISPATCHER(IPTR, Configdata_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	/* Whenever an object shall be created using NewObject(), it will be
	** sent a OM_NEW method.
	*/
	case OM_NEW: return Configdata_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return Configdata_Dispose(cl, obj, (APTR)msg);
	case OM_GET: return Configdata_Get(cl, obj, (APTR)msg);
	case MUIM_Configdata_GetString: return Configdata_GetString(cl, obj, (APTR)msg);
	case MUIM_Configdata_GetULong: return Configdata_GetULong(cl, obj, (APTR)msg);
	case MUIM_Configdata_SetULong: return Configdata_SetULong(cl, obj, (APTR)msg);
	case MUIM_Configdata_SetImspec: return Configdata_SetImspec(cl, obj, (APTR)msg);
	case MUIM_Configdata_SetFramespec: return Configdata_SetFramespec(cl, obj, (APTR)msg);
	case MUIM_Configdata_SetPenspec: return Configdata_SetPenspec(cl, obj, (APTR)msg);
	case MUIM_Configdata_SetFont: return Configdata_SetFont(cl, obj, (APTR)msg);
	case MUIM_Configdata_SetString: return Configdata_SetString(cl, obj, (APTR)msg);
	case MUIM_Configdata_Save: return Configdata_Save(cl, obj, (APTR)msg);
	case MUIM_Configdata_Load: return Configdata_Load(cl, obj, (APTR)msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Configdata_desc = {
    MUIC_Configdata,                        /* Class name */
    MUIC_Dataspace,                         /* super class name */
    sizeof(struct MUI_ConfigdataData),      /* size of class own datas */
    (void*)Configdata_Dispatcher            /* class dispatcher */
};
