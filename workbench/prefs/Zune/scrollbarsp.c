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

struct MUI_ScrollbarsPData
{
    Object *popframe;
    Object *arrow_up_popimage;
    Object *arrow_down_popimage;
    Object *arrow_left_popimage;
    Object *arrow_right_popimage;
    Object *gadget_type_cycle;
    Object *background_popimage;
    Object *knob_popimage;
};

static CONST_STRPTR gadget_type_labels[] =
{
    "standard",
    "newlook",
    "custom",
    NULL,
};

static ULONG DoSuperNew(struct IClass *cl, Object * obj, ULONG tag1,...)
{
    return (DoSuperMethod(cl, obj, OM_NEW, &tag1, NULL));
}

#define FindFont(id) (void*)DoMethod(msg->configdata,MUIM_Dataspace_Find,id)

Object *MakeArrowPopimage (CONST_STRPTR wintitle)
{
    return NewObject(CL_ImageClipboard->mcc_Class, NULL,
		     MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Image,
		     MUIA_Draggable, TRUE, 
		     MUIA_CycleChain, 1,
		     MUIA_MaxWidth, 28,
		     MUIA_MaxHeight, 28,
		     MUIA_Imagedisplay_FreeHoriz, FALSE,
		     MUIA_Imagedisplay_FreeVert, FALSE,
		     MUIA_Window_Title, wintitle,
		     TAG_DONE, 0);
}  

static IPTR ScrollbarsP_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ScrollbarsPData *data;
    struct MUI_ScrollbarsPData d;
    
    obj = (Object *)DoSuperNew(cl, obj,
			       MUIA_Group_Horiz, TRUE,
			       Child, VGroup,
			       Child, VGroup,
			       GroupFrameT("Arrows"),
  			       Child, HVSpace,
			       Child, ColGroup(6),
			       Child, HVSpace,
			       Child, FreeLabel("up"),
			       Child, d.arrow_up_popimage = MakeArrowPopimage("Arrow up"),
			       Child, d.arrow_down_popimage = MakeArrowPopimage("Arrow down"),
			       Child, FreeLLabel("down"),
			       Child, HVSpace,
			       Child, HVSpace,
			       Child, FreeLabel("left"),
			       Child, d.arrow_left_popimage = MakeArrowPopimage("Arrow left"),
			       Child, d.arrow_right_popimage = MakeArrowPopimage("Arrow right"),
			       Child, FreeLLabel("right"),
			       Child, HVSpace,
			       End, /* ColGroup(4) */
			       Child, HVSpace,
			       End, /* Arrows */
			       Child, VGroup,
			       GroupFrameT("Bar"),
			       Child, HGroup,
			       Child, MakeLabel("Gadget Type:"),
			       Child, d.gadget_type_cycle =
			       MUI_MakeObject(MUIO_Cycle, "Gadget Type:",
					      gadget_type_labels),
			       End, /* HGroup Gadget Type */
			       Child, HGroup,
			       MUIA_Group_SameWidth, TRUE,
			       Child, VGroup,
			       MUIA_Group_VertSpacing, 1,
			       Child, d.knob_popimage =
			       NewObject(CL_ImageClipboard->mcc_Class, NULL,
					 MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Image,
					 MUIA_Draggable, TRUE, 
					 MUIA_CycleChain, 1,
					 MUIA_Imagedisplay_FreeHoriz, FALSE,
					 MUIA_Imagedisplay_FreeVert, FALSE,
					 MUIA_Window_Title, "Knob",
					 TAG_DONE),
			       Child, CLabel("Knob"),
			       End, /* VGroup Knob */
			       Child, VGroup,
			       MUIA_Group_VertSpacing, 1,
			       Child, d.background_popimage = MakeBackgroundPopimage(),
			       Child, CLabel("Background"),
			       End, /* VGroup Background */
			       End, /* HGroup Images */
			       End, /* Bar VGroup*/	     
			       End, /* VGroup left */
			       Child, VGroup,
			       Child, VGroup,
			       GroupFrameT("Frame"),
			       Child, d.popframe = MakePopframe(),
			       End, /* Frame VGroup*/
			       Child, ColGroup(3),
			       GroupFrameT("Arrangement"),
			       ScrollbarObject, End,
			       ScrollbarObject, End,
			       ScrollbarObject, End,
			       ScrollbarObject, End,
			       ScrollbarObject, End,
			       ScrollbarObject, End,
			       End, /* Arrangement VGroup*/
			       End, /* VGroup right */
    	TAG_MORE, msg->ops_AttrList);

    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    *data = d;

    DoMethod(d.gadget_type_cycle, MUIM_Notify, MUIA_Cycle_Active, 2, obj,
	     6, MUIM_MultiSet, MUIA_Disabled, FALSE,
	     d.background_popimage, d.knob_popimage, NULL);
    DoMethod(d.gadget_type_cycle, MUIM_Notify, MUIA_Cycle_Active, 0, obj,
	     6, MUIM_MultiSet, MUIA_Disabled, TRUE,
	     d.background_popimage, d.knob_popimage, NULL);
    DoMethod(d.gadget_type_cycle, MUIM_Notify, MUIA_Cycle_Active, 1, obj,
	     6, MUIM_MultiSet, MUIA_Disabled, TRUE,
	     d.background_popimage, d.knob_popimage, NULL);

    return (IPTR)obj;
}


/*
 * MUIM_Settingsgroup_ConfigToGadgets
 */
static IPTR ScrollbarsP_ConfigToGadgets(struct IClass *cl, Object *obj,
				    struct MUIP_Settingsgroup_ConfigToGadgets *msg)
{
    struct MUI_ScrollbarsPData *data = INST_DATA(cl, obj);
    STRPTR spec;

/* Images */
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Image_ArrowUp);
    set(data->arrow_up_popimage,MUIA_Imagedisplay_Spec, (IPTR)spec);
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Image_ArrowDown);
    set(data->arrow_down_popimage,MUIA_Imagedisplay_Spec, (IPTR)spec);
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Image_ArrowLeft);
    set(data->arrow_left_popimage,MUIA_Imagedisplay_Spec, (IPTR)spec);
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Image_ArrowRight);
    set(data->arrow_right_popimage,MUIA_Imagedisplay_Spec, (IPTR)spec);

    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Image_PropKnob);
    set(data->knob_popimage,MUIA_Imagedisplay_Spec, (IPTR)spec);

    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Background_Prop);
    set(data->background_popimage,MUIA_Imagedisplay_Spec, (IPTR)spec);

/* Cycle */
    setcycle(data->gadget_type_cycle,
	     DoMethod(msg->configdata, MUIM_Configdata_GetULong,
		      MUICFG_Scrollbar_Type));

/* Frame */
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Frame_Prop);
    set(data->popframe, MUIA_Framedisplay_Spec, (IPTR)spec);

    return 1;    
}


/*
 * MUIM_Settingsgroup_ConfigToGadgets
 */
static IPTR ScrollbarsP_GadgetsToConfig(struct IClass *cl, Object *obj,
				    struct MUIP_Settingsgroup_GadgetsToConfig *msg)
{
    struct MUI_ScrollbarsPData *data = INST_DATA(cl, obj);
    STRPTR str;

/* Frame */
    str = (STRPTR)xget(data->popframe, MUIA_Framedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetFramespec, MUICFG_Frame_Prop,
	     (IPTR)str);

/* Cycles */
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Scrollbar_Type,
	     xget(data->gadget_type_cycle, MUIA_Cycle_Active));

/* Images */
    str = (STRPTR)xget(data->background_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_Prop,
	     (IPTR)str);
    str = (STRPTR)xget(data->knob_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Image_PropKnob,
	     (IPTR)str);
    str = (STRPTR)xget(data->arrow_up_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Image_ArrowUp,
	     (IPTR)str);
    str = (STRPTR)xget(data->arrow_down_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Image_ArrowDown,
	     (IPTR)str);
    str = (STRPTR)xget(data->arrow_left_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Image_ArrowLeft,
	     (IPTR)str);
    str = (STRPTR)xget(data->arrow_right_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Image_ArrowRight,
	     (IPTR)str);
    return TRUE;
}


BOOPSI_DISPATCHER(IPTR, ScrollbarsP_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return ScrollbarsP_New(cl, obj, (struct opSet *)msg);
	case MUIM_Settingsgroup_ConfigToGadgets: return ScrollbarsP_ConfigToGadgets(cl,obj,(APTR)msg);break;
	case MUIM_Settingsgroup_GadgetsToConfig: return ScrollbarsP_GadgetsToConfig(cl,obj,(APTR)msg);break;
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUIP_Scrollbars_desc = { 
    "Scrollbars",
    MUIC_Group,
    sizeof(struct MUI_ScrollbarsPData),
    (void*)ScrollbarsP_Dispatcher 
};
