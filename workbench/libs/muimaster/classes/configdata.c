/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/dos.h>
#include <proto/commodities.h>

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
    char *appname;
    struct ZunePrefsNew prefs;
    int test;
};

static APTR GetConfigData(Object *obj, ULONG id, APTR def)
{
    APTR f = (APTR)DoMethod(obj, MUIM_Dataspace_Find, id);
    if (!f)
	return def;
    return f;
}

static void LoadPrefs(STRPTR filename, Object *obj)
{
    struct IFFHandle *iff;
    if ((iff = AllocIFF()))
    {
	if ((iff->iff_Stream = (IPTR)Open(filename, MODE_OLDFILE)))
	{
	    InitIFFasDOS(iff);

	    if (!OpenIFF(iff, IFFF_READ))
	    {
		StopChunk( iff, MAKE_ID('P','R','E','F'), MAKE_ID('M','U','I','C'));

		while (!ParseIFF(iff, IFFPARSE_SCAN))
		{
		    struct ContextNode *cn;
		    if (!(cn = CurrentChunk(iff)))
			continue;
		    if (cn->cn_ID == MAKE_ID('M','U','I','C'))
			DoMethod(obj, MUIM_Dataspace_ReadIFF, (IPTR)iff);
		}

		CloseIFF(iff);
	    }
	    Close((BPTR)iff->iff_Stream);
	}
	FreeIFF(iff);
    }
}

/*------------ imspec config stuff --------------*/

struct spec_cfg {
    ULONG muiv;
    ULONG cfgid;
    char *defspec;
};

struct spec_cfg imspec_defaults[] =
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
    { MUII_PropKnob,      MUICFG_Image_PropKnob,        "0:128" },
    { MUII_Drawer,        MUICFG_Image_Drawer,          "0:128" },
    { MUII_HardDisk,      MUICFG_Image_HardDisk,        "0:128" },
    { MUII_Disk,          MUICFG_Image_Disk,            "0:128" },
    { MUII_Chip,          MUICFG_Image_Chip,            "0:128" },
    { MUII_Volume,        MUICFG_Image_Volume,          "0:128" },
    { MUII_RegisterBack,  MUICFG_Background_Register,   "0:128" },
    { MUII_Network,       MUICFG_Image_Network,         "0:128" },
    { MUII_Assign,        MUICFG_Image_Assign,          "0:128" },
    { MUII_TapePlay,      MUICFG_Image_TapePlay,        "0:128" },
    { MUII_TapePlayBack,  MUICFG_Image_TapePlayBack,    "0:128" },
    { MUII_TapePause,     MUICFG_Image_TapePause,       "0:128" },
    { MUII_TapeStop,      MUICFG_Image_TapeStop,        "0:128" },
    { MUII_TapeRecord,    MUICFG_Image_TapeRecord,      "0:128" },
    { MUII_GroupBack,     MUICFG_Background_Framed,     "0:128" },
    { MUII_SliderBack,    MUICFG_Background_Slider,     "0:128" },
    { MUII_SliderKnob,    MUICFG_Background_SliderKnob, "0:128" },
    { MUII_TapeUp,        MUICFG_Image_TapeUp,          "0:128" },
    { MUII_TapeDown,      MUICFG_Image_TapeDown,        "0:128" },
    { MUII_PageBack,      MUICFG_Background_Page,       "0:128" },
    { MUII_ReadListBack,  MUICFG_Background_ReadList,   "0:128" },
};

static int Num_Imspec_Defaults = sizeof(imspec_defaults) / sizeof(struct spec_cfg);

/* called by Configdata_New */
static void init_imspecs (Object *obj, struct MUI_ConfigdataData *data)
{
    int i;

    for (i = 0; i < Num_Imspec_Defaults; i++)
    {
	struct spec_cfg *img = imspec_defaults + i;
	CONST_STRPTR imspec = GetConfigData(obj, img->cfgid, img->defspec);

	D(bug("init_imspecs: %ld %lx %s ...\n", img->muiv, img->cfgid, imspec));
	data->prefs.imagespecs[img->muiv] = imspec;
	if (!data->prefs.imagespecs[img->muiv])
	{
	    D(bug("*** init_imspecs: null imagespec\n"));
	}
    }
}

/*------------ framespec config stuff --------------*/

/* spec format : type, recessed, left, right, up, down spacing */
struct spec_cfg framespec_defaults[] =
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
};

static int Num_Framespec_Defaults = sizeof(framespec_defaults) / sizeof(struct spec_cfg);

/* called by Configdata_New */
static void init_framespecs (Object *obj, struct MUI_ConfigdataData *data)
{
    int i;

    for (i = 0; i < Num_Framespec_Defaults; i++)
    {
	struct spec_cfg *fcfg = framespec_defaults + i;
	CONST_STRPTR framespec = GetConfigData(obj, fcfg->cfgid, fcfg->defspec);
	zune_frame_spec_to_intern(framespec,
				  &data->prefs.frames[fcfg->muiv]);
    }
}


/**************************************************************************
 OM_NEW
**************************************************************************/
static ULONG Configdata_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ConfigdataData *data;
    struct TagItem *tags,*tag;
    //APTR cdata;
    int i;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return NULL;

    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Configdata_Application:
		    data->appname = (char*)tag->ti_Data;
		    break;
	}
    }

    LoadPrefs("env:zune/global.prefs",obj);

    /*---------- fonts stuff ----------*/

    data->prefs.fonts[-MUIV_Font_Normal] = (char*)DoMethod(obj, MUIM_Dataspace_Find, MUICFG_Font_Normal);
    data->prefs.fonts[-MUIV_Font_List] = (char*)DoMethod(obj, MUIM_Dataspace_Find, MUICFG_Font_List);
    data->prefs.fonts[-MUIV_Font_Tiny] = (char*)DoMethod(obj, MUIM_Dataspace_Find, MUICFG_Font_Tiny);
    data->prefs.fonts[-MUIV_Font_Fixed] = (char*)DoMethod(obj, MUIM_Dataspace_Find, MUICFG_Font_Fixed);
    data->prefs.fonts[-MUIV_Font_Title] = (char*)DoMethod(obj, MUIM_Dataspace_Find, MUICFG_Font_Title);
    data->prefs.fonts[-MUIV_Font_Big] = (char*)DoMethod(obj, MUIM_Dataspace_Find, MUICFG_Font_Big);
    data->prefs.fonts[-MUIV_Font_Button] = (char*)DoMethod(obj, MUIM_Dataspace_Find, MUICFG_Font_Button);
    data->prefs.fonts[-MUIV_Font_Knob] = (char*)DoMethod(obj, MUIM_Dataspace_Find, MUICFG_Font_Knob);

    /*---------- images stuff ----------*/

    init_imspecs(obj, data);

    /*---------- frame stuff ----------*/

    init_framespecs(obj, data);

    /*---------- window stuff ----------*/

    data->prefs.window_inner_left = 4;
    data->prefs.window_inner_right = 4;
    data->prefs.window_inner_top = 4;
    data->prefs.window_inner_bottom = 4;
    data->prefs.window_position = WINDOW_POSITION_FORGET_ON_EXIT;

    /*---------- group stuff ----------*/

    data->prefs.group_title_position = GROUP_TITLE_POSITION_CENTERED;
    data->prefs.group_title_color = GROUP_TITLE_COLOR_BLACK;
    data->prefs.group_hspacing = 4;
    data->prefs.group_vspacing = 4;

    /*---------- registers ----------*/

    data->prefs.register_look = REGISTER_LOOK_TRADITIONAL;
    data->prefs.register_truncate_titles = FALSE; /* loosers want full titles */

    /*---------- Buttons ----------*/

    data->prefs.radiobutton_hspacing = 4;
    data->prefs.radiobutton_vspacing = 2;

    /*---------- Cycles ----------*/

    data->prefs.cycle_menu_position = CYCLE_MENU_POSITION_CENTERED;
    data->prefs.cycle_menu_min_entries = 2;
    data->prefs.cycle_menu_speed = 0;
    data->prefs.cycle_menu_recessed_entries = TRUE;

    /*---------- Sliders ----------*/
    /* all taken care of in frames and images */

    /*---------- Scrollbars ----------*/

    data->prefs.sb_look = SB_LOOK_TOP;

    /*---------- Lists ----------*/

    data->prefs.list_linespacing = 2;

    /*---------- Strings ----------*/
    /* all taken care of in frames and images */

    /*---------- Navigation ----------*/

    data->prefs.dragndrop_left_button = FALSE;
    data->prefs.dragndrop_left_modifier.readable_hotkey = StrDup("control");
    data->prefs.dragndrop_middle_button = FALSE;
    data->prefs.dragndrop_middle_modifier.readable_hotkey = StrDup("control");
    data->prefs.dragndrop_autostart = -1;
    data->prefs.dragndrop_look = DND_LOOK_GHOSTED_ON_BOX;
    data->prefs.balancing_look = BALANCING_SHOW_FRAMES;

    if (data->prefs.dragndrop_left_modifier.readable_hotkey)
	data->prefs.dragndrop_left_modifier.ix_well = 
	    !ParseIX(data->prefs.dragndrop_left_modifier.readable_hotkey,
		     &data->prefs.dragndrop_left_modifier.ix);
    else
	data->prefs.dragndrop_left_modifier.ix_well = 0;

    if (data->prefs.dragndrop_middle_modifier.readable_hotkey)
	data->prefs.dragndrop_middle_modifier.ix_well = 
	    !ParseIX(data->prefs.dragndrop_middle_modifier.readable_hotkey,
		     &data->prefs.dragndrop_middle_modifier.ix);
    else
	data->prefs.dragndrop_middle_modifier.ix_well = 0;

    /*---------- mui keys ----------*/

    data->prefs.muikeys[MUIKEY_PRESS].readable_hotkey = StrDup("-upstroke return");
    data->prefs.muikeys[MUIKEY_TOGGLE].readable_hotkey = StrDup("-repeat space");
    data->prefs.muikeys[MUIKEY_UP].readable_hotkey = StrDup("-repeat up");
    data->prefs.muikeys[MUIKEY_DOWN].readable_hotkey = StrDup("-repeat down");
    data->prefs.muikeys[MUIKEY_PAGEUP].readable_hotkey = StrDup("-repeat shift up");
    data->prefs.muikeys[MUIKEY_PAGEDOWN].readable_hotkey = StrDup("-repeat shift down");
    data->prefs.muikeys[MUIKEY_TOP].readable_hotkey = StrDup("control up");
    data->prefs.muikeys[MUIKEY_BOTTOM].readable_hotkey = StrDup("control down");
    data->prefs.muikeys[MUIKEY_LEFT].readable_hotkey = StrDup("-repeat left");
    data->prefs.muikeys[MUIKEY_RIGHT].readable_hotkey = StrDup("-repeat right");
    data->prefs.muikeys[MUIKEY_WORDLEFT].readable_hotkey = StrDup("-repeat control left");
    data->prefs.muikeys[MUIKEY_WORDRIGHT].readable_hotkey = StrDup("-repeat control right");
    data->prefs.muikeys[MUIKEY_LINESTART].readable_hotkey = StrDup("shift left");
    data->prefs.muikeys[MUIKEY_LINEEND].readable_hotkey = StrDup("shift right");
    data->prefs.muikeys[MUIKEY_GADGET_NEXT].readable_hotkey = StrDup("-repeat tab");
    data->prefs.muikeys[MUIKEY_GADGET_PREV].readable_hotkey = StrDup("-repeat shift tab");
    data->prefs.muikeys[MUIKEY_GADGET_OFF].readable_hotkey = StrDup("control tab");
    data->prefs.muikeys[MUIKEY_WINDOW_CLOSE].readable_hotkey = StrDup("esc");
    data->prefs.muikeys[MUIKEY_WINDOW_NEXT].readable_hotkey = StrDup("-repeat alt tab");
    data->prefs.muikeys[MUIKEY_WINDOW_PREV].readable_hotkey = StrDup("-repeat alt shift tab");
    data->prefs.muikeys[MUIKEY_HELP].readable_hotkey = StrDup("help");
    data->prefs.muikeys[MUIKEY_POPUP].readable_hotkey = StrDup("control p");

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
    struct MUI_ConfigdataData *data = INST_DATA(cl, obj);
    int i;

    if (data->prefs.dragndrop_left_modifier.readable_hotkey)
	FreeVec(data->prefs.dragndrop_left_modifier.readable_hotkey);
    if (data->prefs.dragndrop_middle_modifier.readable_hotkey)
	FreeVec(data->prefs.dragndrop_middle_modifier.readable_hotkey);

    for (i = 0; i < MUIKEY_COUNT; i++)
    {
    	if (data->prefs.muikeys[i].readable_hotkey)
	    FreeVec(data->prefs.muikeys[i].readable_hotkey);
    }

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
