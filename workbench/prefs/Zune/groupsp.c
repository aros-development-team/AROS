/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
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

#ifdef __AROS__
#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
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
    NULL,
};


static ULONG DoSuperNew(struct IClass *cl, Object * obj, ULONG tag1,...)
{
    return (DoSuperMethod(cl, obj, OM_NEW, &tag1, NULL));
}

#define FindFont(id) (void*)DoMethod(msg->configdata,MUIM_Dataspace_Find,id)

static Object*MakeSpacingSlider (void)
{
    return MUI_MakeObject(MUIO_Slider, "", 0, 9);
}

static IPTR GroupsP_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_GroupsPData *data;
    struct MUI_GroupsPData d;
    
    obj = (Object *)DoSuperNew(cl, obj,
        Child, ColGroup(2),
	    Child, ColGroup(2),
		GroupFrameT("Title"),
		    Child, HVSpace,
		    Child, HVSpace,
		    Child, MakeLabel("Position:"),
		    Child, d.title_position_cycle = MUI_MakeObject(MUIO_Cycle, "Position:", positions_labels),
	            Child, MakeLabel("Color:"),
	    	    Child, d.title_color_cycle = MUI_MakeObject(MUIO_Cycle, "Color:", color_labels),
   		    Child, MakeLabel("Font:"),
   		    Child, PopaslObject,
   		       MUIA_Popasl_Type, ASL_FontRequest,
   		       MUIA_Popstring_String, d.font_title_string = StringObject, StringFrame, End,
   		       MUIA_Popstring_Button, PopButton(MUII_PopUp),
   		       End,
		    Child, HVSpace,
		    Child, HVSpace,
		End,
	    Child, HGroup,
		GroupFrameT("Frame"),
		Child, VGroup,
		   Child, d.normal_popframe = NewObject(CL_FrameClipboard->mcc_Class, NULL,
			       MUIA_Draggable, TRUE,
			       MUIA_Window_Title, "Adjust Frame",
			       End,
		   Child, MUI_MakeObject(MUIO_Label, "Normal", MUIO_Label_Centered),
	           End,
       	       Child, VGroup,
		   Child, d.virtual_popframe = NewObject(CL_FrameClipboard->mcc_Class, NULL,
			       MUIA_Draggable, TRUE,
			       MUIA_Window_Title, "Adjust Frame",
			       End,
		   Child, MUI_MakeObject(MUIO_Label, "Virtual", MUIO_Label_Centered),
	           End,
		End,
	    Child, ColGroup(2),
		GroupFrameT("Spacing"),
	        Child, HVSpace,
	        Child, HVSpace,
		Child, MakeLabel("Horizontal:"),
	        Child, (IPTR)d.spacing_horiz_slider = MakeSpacingSlider(),
	        Child, MakeLabel("Vertical:"),
		Child, (IPTR)d.spacing_vert_slider = MakeSpacingSlider(),
	        Child, HVSpace,
	        Child, HVSpace,
		End,
	    Child, ColGroup(3),
		GroupFrameT("Background"),
		Child, d.background_framed_popimage =
		    NewObject(CL_ImageClipboard->mcc_Class, NULL,
			      MUIA_Draggable, TRUE,
			      MUIA_Window_Title, "Adjust Background",
			      End,
		Child, d.background_page_popimage =
		    NewObject(CL_ImageClipboard->mcc_Class, NULL,
			      MUIA_Draggable, TRUE,
			      MUIA_Window_Title, "Adjust Background",
			      End,
		Child, d.background_register_popimage =
		    NewObject(CL_ImageClipboard->mcc_Class, NULL,
			      MUIA_Draggable, TRUE,
			      MUIA_Window_Title, "Adjust Background",
			      End,
		Child, MakeLabel("Framed"),
		Child, MakeLabel("Page"),
		Child, MakeLabel("Register"),
		End,
	    End,
    	TAG_MORE, msg->ops_AttrList);

    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    *data = d;

    return (IPTR)obj;
}

static IPTR GroupsP_ConfigToGadgets(struct IClass *cl, Object *obj, struct MUIP_Settingsgroup_ConfigToGadgets *msg)
{
    struct MUI_GroupsPData *data = INST_DATA(cl, obj);
    STRPTR spec;

/* Fonts */
    setstring(data->font_title_string, FindFont(MUICFG_Font_Title));

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

static IPTR GroupsP_GadgetsToConfig(struct IClass *cl, Object *obj, struct MUIP_Settingsgroup_GadgetsToConfig *msg)
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

#ifndef __AROS__
__asm IPTR GroupsP_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,GroupsP_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
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
    MUIC_Settingsgroup, 
    sizeof(struct MUI_GroupsPData),
    (void*)GroupsP_Dispatcher 
};
