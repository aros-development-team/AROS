/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
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
#endif

#include "zunestuff.h"
#include <string.h>

/*  #define DEBUG 1 */
/*  #include <aros/debug.h> */

extern struct Library *MUIMasterBase;

struct MUI_GroupsPData
{
    Object *background_register_popimage;
    Object *background_framed_popimage;
    Object *background_page_popimage;
    Object *spacing_vert_slider;
    Object *spacing_horiz_slider;
    Object *font_title_string;
    Object *normal_popframe;
    Object *virtual_popframe;
    Object *title_position_cycle;
    Object *title_color_cycle;
};

static CONST_STRPTR positions_labels[] =
{
    "above",
    "centered",
    NULL,
};

static CONST_STRPTR color_labels[] =
{
    "standard",
    "hilite",
    "3d",
    "outline",
    NULL,
};


static IPTR GroupsP_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_GroupsPData *data;
    struct MUI_GroupsPData d;
    
    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
	    MUIA_Group_SameWidth, TRUE,
	    MUIA_Group_Columns, 2,
	    Child, VGroup,
		GroupFrameT("Title"),
		Child, VSpace(0),
		Child, ColGroup(2),
		    MUIA_Group_VertSpacing, 2,
		    Child, Label("Position:"),
		    Child, d.title_position_cycle =
			       MakeCycle("Position:", positions_labels),
	            Child, Label("Color:"),
	    	    Child, d.title_color_cycle =
			       MakeCycle("Color:", color_labels),
   		    Child, Label("Font:"),
   		    Child, PopaslObject,
   		       MUIA_Popasl_Type, ASL_FontRequest,
   		       MUIA_Popstring_String, d.font_title_string = StringObject,
			       StringFrame, End,
   		       MUIA_Popstring_Button, PopButton(MUII_PopUp),
   		       End,
		    End, /* Title */
		Child, VSpace(0),
		End,
	    Child, HGroup,
		GroupFrameT("Frame"),
		MUIA_Group_SameWidth, TRUE,
		Child, VGroup,
		   MUIA_Group_VertSpacing, 1,
		   Child, d.normal_popframe = MakePopframe(),
		   Child, CLabel("Normal"),
	           End,
       	       Child, VGroup,
		   MUIA_Group_VertSpacing, 1,
		   Child, d.virtual_popframe = MakePopframe(),
		   Child, CLabel("Virtual"),
	           End,
							 End, /* Frame */
	    Child, VGroup,
		GroupFrameT("Spacing"),
		Child, VSpace(0),
		Child, ColGroup(2),
		    MUIA_Group_VertSpacing, 2,
		    Child, Label("Horizontal:"),
	            Child, d.spacing_horiz_slider = (Object*)MakeSpacingSlider(),
	            Child, Label("Vertical:"),
		    Child, d.spacing_vert_slider = (Object*)MakeSpacingSlider(),
		    End, /* Spacing */
		Child, VSpace(0),
		End,
	    Child, HGroup,
		GroupFrameT("Background"),
                MUIA_Group_SameWidth, TRUE,
		Child, VGroup,
		    MUIA_Group_VertSpacing, 1,
		    Child, d.background_framed_popimage = MakeBackgroundPopimage(),
		    Child, CLabel("Framed"),
		    End,
		Child, VGroup,
		    MUIA_Group_VertSpacing, 1,
		    Child, d.background_page_popimage = MakeBackgroundPopimage(),
		    Child, CLabel("Page"),
		    End,
		Child, VGroup,
		    MUIA_Group_VertSpacing, 1,
		    Child, d.background_register_popimage = MakeBackgroundPopimage(),
		    Child, CLabel("Register"),
		    End,
		End, /* Background */
    	TAG_MORE, msg->ops_AttrList);

    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    *data = d;
    set(data->font_title_string, MUIA_CycleChain, 1);

    return (IPTR)obj;
}


static IPTR GroupsP_ConfigToGadgets(struct IClass *cl, Object *obj,
				    struct MUIP_Settingsgroup_ConfigToGadgets *msg)
{
    struct MUI_GroupsPData *data = INST_DATA(cl, obj);
    STRPTR spec;

/* Fonts */
    setstring(data->font_title_string, (IPTR)FindFont(MUICFG_Font_Title));

/* Backgrounds */
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Background_Framed);
    set(data->background_framed_popimage,MUIA_Imagedisplay_Spec, (IPTR)spec);

    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Background_Register);
    set(data->background_register_popimage,MUIA_Imagedisplay_Spec, (IPTR)spec);

    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Background_Page);
    set(data->background_page_popimage,MUIA_Imagedisplay_Spec, (IPTR)spec);

/* Frames */
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Frame_Group);
    set(data->normal_popframe, MUIA_Framedisplay_Spec, (IPTR)spec);

    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Frame_Virtual);
    set(data->virtual_popframe, MUIA_Framedisplay_Spec, (IPTR)spec);

/* Spacing */
    setslider(data->spacing_horiz_slider,
	      DoMethod(msg->configdata, MUIM_Configdata_GetULong,
		       MUICFG_Group_HSpacing));
    setslider(data->spacing_vert_slider,
	      DoMethod(msg->configdata, MUIM_Configdata_GetULong,
		       MUICFG_Group_VSpacing));

/* Title (Cycles) */
    set(data->title_position_cycle, MUIA_Cycle_Active,
	DoMethod(msg->configdata, MUIM_Configdata_GetULong, MUICFG_GroupTitle_Position));
    set(data->title_color_cycle, MUIA_Cycle_Active,
	DoMethod(msg->configdata, MUIM_Configdata_GetULong, MUICFG_GroupTitle_Color));

    return 1;    
}


static IPTR GroupsP_GadgetsToConfig(struct IClass *cl, Object *obj,
				    struct MUIP_Settingsgroup_GadgetsToConfig *msg)
{
    struct MUI_GroupsPData *data = INST_DATA(cl, obj);
    STRPTR str;

/* Fonts */
    str = getstring(data->font_title_string);
    DoMethod(msg->configdata, MUIM_Configdata_SetFont, MUICFG_Font_Title, (IPTR)str);

/* Backgrounds */
    str = (STRPTR)xget(data->background_framed_popimage,MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_Framed,
	     (IPTR)str);

    str = (STRPTR)xget(data->background_register_popimage,MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_Register,
	     (IPTR)str);

    str = (STRPTR)xget(data->background_page_popimage,MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_Page,
	     (IPTR)str);

/* Frames */
    str = (STRPTR)xget(data->normal_popframe, MUIA_Framedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetFramespec, MUICFG_Frame_Group,
	     (IPTR)str);
    str = (STRPTR)xget(data->virtual_popframe, MUIA_Framedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetFramespec, MUICFG_Frame_Virtual,
	     (IPTR)str);

/* Spacing */
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Group_HSpacing,
	     xget(data->spacing_horiz_slider, MUIA_Numeric_Value));
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Group_VSpacing,
	     xget(data->spacing_vert_slider, MUIA_Numeric_Value));

/* Title (Cycles) */
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_GroupTitle_Position,
	     xget(data->title_position_cycle, MUIA_Cycle_Active));
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_GroupTitle_Color,
	     xget(data->title_color_cycle, MUIA_Cycle_Active));

    return TRUE;
}


BOOPSI_DISPATCHER(IPTR, GroupsP_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return GroupsP_New(cl, obj, (struct opSet *)msg);
	case MUIM_Settingsgroup_ConfigToGadgets: return GroupsP_ConfigToGadgets(cl,obj,(APTR)msg);break;
	case MUIM_Settingsgroup_GadgetsToConfig: return GroupsP_GadgetsToConfig(cl,obj,(APTR)msg);break;
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUIP_Groups_desc = { 
    "Groups",
    MUIC_Group, 
    sizeof(struct MUI_GroupsPData),
    (void*)GroupsP_Dispatcher 
};
