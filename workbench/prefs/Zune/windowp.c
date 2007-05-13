/*
    Copyright  2002-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#ifdef __AROS__
#include <proto/alib.h>
//#define DEBUG 1
//#include <aros/debug.h>
#endif

#include "zunestuff.h"
#include <string.h>

extern struct Library *MUIMasterBase;

struct MUI_WindowPData
{
    Object *positions_cycle;
    Object *refresh_cycle;
    Object *redraw_cycle;
    Object *font_normal_string;
    Object *font_tiny_string;
    Object *font_big_string;
    Object *background_window_popimage;
    Object *background_requester_popimage;
    Object *spacing_left_slider;
    Object *spacing_right_slider;
    Object *spacing_top_slider;
    Object *spacing_bottom_slider;
    Object *mui;
    Object *popup;
    Object *iconify;
    Object *snapshot;
};

static CONST_STRPTR positions_labels[4];
static CONST_STRPTR refresh_labels[3];
static CONST_STRPTR redraw_labels[3];

#define _Button(name)\
    TextObject,\
        ButtonFrame,\
        MUIA_Font, MUIV_Font_Button,\
        MUIA_Text_Contents, name,\
        MUIA_Text_PreParse, "\33c",\
        MUIA_InputMode    , MUIV_InputMode_Toggle,\
        MUIA_Background   , MUII_ButtonBack,\
        End

static IPTR WindowP_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_WindowPData *data;
    struct MUI_WindowPData d;

    positions_labels[0] = _(MSG_FORGET_ON_EXIT);
    positions_labels[1] = _(MSG_REMEMBER_ON_EXIT);
    positions_labels[2] = _(MSG_SAVE_ON_EXIT);

    refresh_labels[0] = _(MSG_SMART);
    refresh_labels[1] = _(MSG_SIMPLE);
    
    redraw_labels[0] = _(MSG_WITHOUT_CLEAR);
    redraw_labels[1] = _(MSG_WITH_CLEAR);

    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
	
        Child, (IPTR) HGroup,
	   Child, (IPTR) VGroup,
	       Child, (IPTR) VGroup,
                   GroupFrameT(_(MSG_CONTROL)),
		   Child, (IPTR) VSpace(0),
		   Child, (IPTR) ColGroup(2),
		      MUIA_Group_VertSpacing, 2,
		      Child, (IPTR) Label(_(MSG_POSITIONS)),
		      Child, (IPTR) (d.positions_cycle = MakeCycle(_(MSG_POSITIONS), positions_labels)),
		      Child, (IPTR) Label(_(MSG_REFRESH)),
		      Child, (IPTR) (d.refresh_cycle = MakeCycle(_(MSG_REFRESH), refresh_labels)),
		      Child, (IPTR) Label(_(MSG_REDRAW)),
		      Child, (IPTR) (d.redraw_cycle = MakeCycle(_(MSG_REDRAW), redraw_labels)),
              Child, (IPTR) Label(_(MSG_BUTTONS)),
                Child, HGroup,
                    NoFrame,
                    Child, ColGroup(8),
                     NoFrame,
                     Child, HVSpace,
                     Child, d.mui = _Button("M"),
                     Child, HVSpace,
                     Child, d.snapshot = _Button("S"),
                     Child, HVSpace,
                     Child, d.popup = _Button("P"),
                     Child, HVSpace,
                     Child, d.iconify = _Button("I"),
                    End,
                End,
		      End,
		   Child, (IPTR) VSpace(0),
		   End,
   	       Child, (IPTR) VGroup,
   		   GroupFrameT(_(MSG_FONTS)),
	           Child, (IPTR) VSpace(0),
		   Child, (IPTR) ColGroup(2),
		       MUIA_Group_VertSpacing, 2,
   		       Child, (IPTR) Label(_(MSG_NORMAL_COLON)),
   		       Child, (IPTR) (d.font_normal_string = MakePopfont(FALSE)),
   		       Child, (IPTR) Label(_(MSG_TINY)),
   		       Child, (IPTR) (d.font_tiny_string = MakePopfont(FALSE)),
   		       Child, (IPTR) Label(_(MSG_BIG)),
   		       Child, (IPTR) (d.font_big_string = MakePopfont(FALSE)),
                    End,
                    Child, (IPTR) VSpace(0),
   		   End,
	        End,
	     Child, (IPTR) VGroup,
                Child, (IPTR) HGroup,
		   GroupFrameT(_(MSG_BACKGROUND)),
		   MUIA_Group_SameWidth, TRUE,
		   Child, (IPTR) VGroup,
		      MUIA_Group_VertSpacing, 1,
		      Child, (IPTR) (d.background_window_popimage = MakeBackgroundPopimage()),
		      Child, (IPTR) CLabel(_(MSG_WINDOW)),
		      End,
		   Child, (IPTR) VGroup,
		      MUIA_Group_VertSpacing, 1,
		      Child, (IPTR) (d.background_requester_popimage = MakeBackgroundPopimage()),
		      Child, (IPTR) CLabel(_(MSG_REQUESTER)),
	              End,
		   End,
	        Child, (IPTR) ColGroup(4),
                   GroupFrameT(_(MSG_SPACING)),
			       MUIA_Group_Spacing, 2,
			       Child, (IPTR) Label(_(MSG_L)),
			       Child, (IPTR) (d.spacing_left_slider = MakeSpacingSlider()),
			       Child, (IPTR) (d.spacing_top_slider = MakeSpacingSlider()),
			       Child, (IPTR) Label(_(MSG_T)),
			       Child, (IPTR) Label(_(MSG_R)),
			       Child, (IPTR) (d.spacing_right_slider = MakeSpacingSlider()),
			       Child, (IPTR) (d.spacing_bottom_slider = MakeSpacingSlider()),
			       Child, (IPTR) Label(_(MSG_B)),
			       End,
		End,
	    End,
    	TAG_MORE, (IPTR) msg->ops_AttrList);

    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    *data = d;

    set(data->refresh_cycle, MUIA_Disabled, TRUE);
    //set(data->positions_cycle, MUIA_Disabled, TRUE);

    return (IPTR)obj;
}


static IPTR WindowP_ConfigToGadgets(struct IClass *cl, Object *obj,
				    struct MUIP_Settingsgroup_ConfigToGadgets *msg)
{
    struct MUI_WindowPData *data = INST_DATA(cl, obj);
    STRPTR spec;

/* Fonts */
    setstring(data->font_normal_string, (IPTR)FindFont(MUICFG_Font_Normal));
    setstring(data->font_tiny_string, (IPTR)FindFont(MUICFG_Font_Tiny));
    setstring(data->font_big_string, (IPTR)FindFont(MUICFG_Font_Big));

/* Backgrounds */
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Background_Window);
    set(data->background_window_popimage,MUIA_Imagedisplay_Spec, (IPTR)spec);

    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Background_Requester);
    set(data->background_requester_popimage,MUIA_Imagedisplay_Spec, (IPTR)spec);

/* Spacing */
    setslider(data->spacing_left_slider,
	      DoMethod(msg->configdata, MUIM_Configdata_GetULong,
		       MUICFG_Window_Spacing_Left));
    setslider(data->spacing_right_slider,
	      DoMethod(msg->configdata, MUIM_Configdata_GetULong,
		       MUICFG_Window_Spacing_Right));
    setslider(data->spacing_top_slider,
	      DoMethod(msg->configdata, MUIM_Configdata_GetULong,
		       MUICFG_Window_Spacing_Top));
    setslider(data->spacing_bottom_slider,
	      DoMethod(msg->configdata, MUIM_Configdata_GetULong,
		       MUICFG_Window_Spacing_Bottom));

/* Cycles */
    setcycle(data->redraw_cycle,
	     DoMethod(msg->configdata, MUIM_Configdata_GetULong,
		      MUICFG_Window_Redraw));
	setcycle(data->positions_cycle,
	     DoMethod(msg->configdata, MUIM_Configdata_GetULong,
		      MUICFG_Window_Positions));

    ULONG   buttons = DoMethod(msg->configdata, MUIM_Configdata_GetULong, MUICFG_Window_Buttons);

    if ((buttons & MUIV_Window_Button_MUI) != 0) set(data->mui, MUIA_Selected, TRUE); else set(data->mui, MUIA_Selected, FALSE);
    if ((buttons & MUIV_Window_Button_Popup) != 0) set(data->popup, MUIA_Selected, TRUE); else set(data->popup, MUIA_Selected, FALSE);
    if ((buttons & MUIV_Window_Button_Iconify) != 0) set(data->iconify, MUIA_Selected, TRUE); else set(data->iconify, MUIA_Selected, FALSE);
    if ((buttons & MUIV_Window_Button_Snapshot) != 0) set(data->snapshot, MUIA_Selected, TRUE); else set(data->snapshot, MUIA_Selected, FALSE);

    return 1;    
}


static IPTR WindowP_GadgetsToConfig(struct IClass *cl, Object *obj,
				    struct MUIP_Settingsgroup_GadgetsToConfig *msg)
{
    struct MUI_WindowPData *data = INST_DATA(cl, obj);
    STRPTR str;

/* Fonts */
    str = getstring(data->font_normal_string);
    DoMethod(msg->configdata, MUIM_Configdata_SetFont, MUICFG_Font_Normal, (IPTR)str);

    str = getstring(data->font_tiny_string);
    DoMethod(msg->configdata, MUIM_Configdata_SetFont, MUICFG_Font_Tiny, (IPTR)str);

    str = getstring(data->font_big_string);
    DoMethod(msg->configdata, MUIM_Configdata_SetFont, MUICFG_Font_Big, (IPTR)str);

/* Backgrounds */
    str = (STRPTR)XGET(data->background_window_popimage,MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_Window,
	     (IPTR)str);

    str = (STRPTR)XGET(data->background_requester_popimage,MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_Requester,
	     (IPTR)str);

/* Spacing */
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Window_Spacing_Left,
	     XGET(data->spacing_left_slider, MUIA_Numeric_Value));
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Window_Spacing_Right,
	     XGET(data->spacing_right_slider, MUIA_Numeric_Value));
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Window_Spacing_Top,
	     XGET(data->spacing_top_slider, MUIA_Numeric_Value));
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Window_Spacing_Bottom,
	     XGET(data->spacing_bottom_slider, MUIA_Numeric_Value));

/* Cycles */
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Window_Redraw,
	     XGET(data->redraw_cycle, MUIA_Cycle_Active));
	DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Window_Positions,
	     XGET(data->positions_cycle, MUIA_Cycle_Active));

    ULONG   buttons = 0;

    if (XGET(data->mui, MUIA_Selected) != FALSE) buttons |= MUIV_Window_Button_MUI;
    if (XGET(data->popup, MUIA_Selected) != FALSE) buttons |= MUIV_Window_Button_Popup;
    if (XGET(data->snapshot, MUIA_Selected) != FALSE) buttons |= MUIV_Window_Button_Snapshot;
    if (XGET(data->iconify, MUIA_Selected) != FALSE) buttons |= MUIV_Window_Button_Iconify;

    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Window_Buttons, buttons);
    
    return TRUE;
}

BOOPSI_DISPATCHER(IPTR, WindowP_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return WindowP_New(cl, obj, (struct opSet *)msg);
	case MUIM_Settingsgroup_ConfigToGadgets: return WindowP_ConfigToGadgets(cl,obj,(APTR)msg);break;
	case MUIM_Settingsgroup_GadgetsToConfig: return WindowP_GadgetsToConfig(cl,obj,(APTR)msg);break;
    }
    
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUIP_Windows_desc = { 
    "Windows",
    MUIC_Group, 
    sizeof(struct MUI_WindowPData),
    (void*)WindowP_Dispatcher 
};
