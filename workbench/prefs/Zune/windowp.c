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
/*  #define DEBUG 1 */
/*  #include <aros/debug.h> */
#endif

#include "zunestuff.h"
#include <string.h>

extern struct Library *MUIMasterBase;

struct MUI_WindowPData
{
    Object *font_normal_string;
    Object *font_tiny_string;
    Object *font_big_string;
    Object *background_window_popimage;
    Object *background_requester_popimage;
    Object *spacing_left_slider;
    Object *spacing_right_slider;
    Object *spacing_top_slider;
    Object *spacing_bottom_slider;
};

static CONST_STRPTR positions_labels[] =
{
    "forget on exit",
    "remember on exit",
    "save on exit",
    NULL,
};

static CONST_STRPTR refresh_labels[] =
{
    "smart",
    "simple",
    NULL,
};

static CONST_STRPTR redraw_labels[] =
{
    "without clear",
    "with clear",
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


static IPTR WindowP_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_WindowPData *data;
    struct MUI_WindowPData d;

    obj = (Object *)DoSuperNew(cl, obj,
	Child, HGroup,
	   Child, VGroup,
	       Child, ColGroup(2),
                   GroupFrameT("Control"),
			       MUIA_Disabled, TRUE,
                   Child, HVSpace,
                   Child, HVSpace,
		   Child, MakeLabel("Positions:"),
		   Child, MUI_MakeObject(MUIO_Cycle, "Positions:", positions_labels),
		   Child, MakeLabel("Refresh:"),
		   Child, MUI_MakeObject(MUIO_Cycle, "Refresh:", refresh_labels),
		   Child, MakeLabel("Redraw:"),
		   Child, MUI_MakeObject(MUIO_Cycle, "Redraw:", redraw_labels),
                   Child, HVSpace,
                   Child, HVSpace,
		   End,
   	       Child, ColGroup(2),
   		   GroupFrameT("Fonts"),
	           Child, HVSpace,
	           Child, HVSpace,
   		   Child, MakeLabel("Normal"),
   		   Child, PopaslObject,
   		       MUIA_Popasl_Type, ASL_FontRequest,
   		       MUIA_Popstring_String, d.font_normal_string = StringObject,
			       StringFrame, End,
   		       MUIA_Popstring_Button, PopButton(MUII_PopUp),
   		       End,
   
   		   Child, MakeLabel("Tiny"),
   		   Child, PopaslObject,
   		       MUIA_Popasl_Type, ASL_FontRequest,
   		       MUIA_Popstring_String, d.font_tiny_string = StringObject,
			       StringFrame, End,
   		       MUIA_Popstring_Button, PopButton(MUII_PopUp),
   		       End,
   
   		   Child, MakeLabel("Big"),
   		   Child, PopaslObject,
   		       MUIA_Popasl_Type, ASL_FontRequest,
   		       MUIA_Popstring_String, d.font_big_string = StringObject,
			       StringFrame, End,
   		       MUIA_Popstring_Button, PopButton(MUII_PopUp),
   		       End,
	           Child, HVSpace,
	           Child, HVSpace,
   		   End,
	        End,
	     Child, VGroup,
                Child, ColGroup(2),
		   GroupFrameT("Background"),
		   Child, VGroup,
		      MUIA_Group_VertSpacing, 1,
		      Child, d.background_window_popimage =
			       NewObject(CL_ImageClipboard->mcc_Class, NULL,
					 MUIA_Draggable, TRUE,
					 MUIA_Window_Title, "Adjust Background",
					 End,
		      Child, MUI_MakeObject(MUIO_Label, "Window",
					    MUIO_Label_Centered),
		      End,
		   Child, VGroup,
		      MUIA_Group_VertSpacing, 1,
		      Child, d.background_requester_popimage =
			     NewObject(CL_ImageClipboard->mcc_Class, NULL,
				       MUIA_Draggable, TRUE,
				       MUIA_Window_Title, "Adjust Background",
				       End,
		      Child, MUI_MakeObject(MUIO_Label, "Requester",
					    MUIO_Label_Centered),
	              End,
		   End,
	        Child, ColGroup(4),
                   GroupFrameT("Spacing"),
			       MUIA_Group_Spacing, 2,
			       Child, MakeLabel("L"),
			       Child, d.spacing_left_slider = MakeSpacingSlider(),
			       Child, d.spacing_top_slider = MakeSpacingSlider(),
			       Child, MakeLabel("T"),
			       Child, MakeLabel("R"),
			       Child, d.spacing_right_slider = MakeSpacingSlider(),
			       Child, d.spacing_bottom_slider = MakeSpacingSlider(),
			       Child, MakeLabel("B"),
			       End,
		End,
	    End,
    	TAG_MORE, msg->ops_AttrList);

    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    *data = d;

    return (IPTR)obj;
}


static IPTR WindowP_ConfigToGadgets(struct IClass *cl, Object *obj,
				    struct MUIP_Settingsgroup_ConfigToGadgets *msg)
{
    struct MUI_WindowPData *data = INST_DATA(cl, obj);
    STRPTR spec;

/* Fonts */
    setstring(data->font_normal_string, FindFont(MUICFG_Font_Normal));
    setstring(data->font_tiny_string, FindFont(MUICFG_Font_Tiny));
    setstring(data->font_big_string, FindFont(MUICFG_Font_Big));

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
    str = (STRPTR)xget(data->background_window_popimage,MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_Window,
	     (IPTR)str);

    str = (STRPTR)xget(data->background_requester_popimage,MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_Requester,
	     (IPTR)str);

/* Spacing */
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Window_Spacing_Left,
	     xget(data->spacing_left_slider, MUIA_Numeric_Value));
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Window_Spacing_Right,
	     xget(data->spacing_right_slider, MUIA_Numeric_Value));
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Window_Spacing_Top,
	     xget(data->spacing_top_slider, MUIA_Numeric_Value));
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Window_Spacing_Bottom,
	     xget(data->spacing_bottom_slider, MUIA_Numeric_Value));

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

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUIP_Windows_desc = { 
    "Windows",
    MUIC_Group, 
    sizeof(struct MUI_WindowPData),
    (void*)WindowP_Dispatcher 
};
