/*
    Copyright � 2002, The AROS Development Team. 
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
#include <proto/muimaster.h>

#ifdef __AROS__
#include <proto/alib.h>
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
    Object *pos_radios[3];
};

static CONST_STRPTR gadget_type_labels[] =
{
    "standard",
    "newlook",
    "custom",
    NULL,
};

#define FindFont(id) (void*)DoMethod(msg->configdata,MUIM_Dataspace_Find,id)

Object *MakeArrowPopimage (CONST_STRPTR wintitle)
{
    return NewObject(CL_ImageClipboard->mcc_Class, NULL,
		     MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Image,
		     MUIA_Draggable, TRUE, 
		     MUIA_CycleChain, 1,
		     MUIA_MaxWidth, 30,
		     MUIA_MaxHeight, 30,
		     MUIA_Imagedisplay_FreeHoriz, FALSE,
		     MUIA_Imagedisplay_FreeVert, FALSE,
		     MUIA_Window_Title, (IPTR)wintitle,
		     TAG_DONE, 0);
}  

Object *MakeSingleRadio (void)
{
    return ImageObject,
	MUIA_Image_FontMatch, TRUE,
	MUIA_InputMode, MUIV_InputMode_Immediate,
	MUIA_Selected, FALSE,
	MUIA_ShowSelState, FALSE,
	MUIA_Image_Spec, MUII_RadioButton,
	MUIA_Frame, MUIV_Frame_None,
	End;
}

Object *MakeScrollbar(IPTR type)
{
    return ScrollbarObject,
	MUIA_Scrollbar_Type, type,
	MUIA_Prop_Entries, 10,
	MUIA_Prop_Visible, 4,
	MUIA_Prop_First, 3,
	End;
}

Object *MakeSpacer(void)
{
    return RectangleObject, MUIA_FixWidth, 0, MUIA_FixHeight, 0, MUIA_Weight, 1, End;
}

static IPTR ScrollbarsP_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ScrollbarsPData *data;
    struct MUI_ScrollbarsPData d;

    Object * radiochilds[] = {
	MakeScrollbar(MUIV_Scrollbar_Type_Top),
	MakeScrollbar(MUIV_Scrollbar_Type_Sym),
	MakeScrollbar(MUIV_Scrollbar_Type_Bottom),
	NULL,
    };

    obj = (Object *)DoSuperNew(cl, obj,
	MUIA_Group_Horiz, TRUE,
	Child, VGroup, /* left */
	
	    Child, VGroup,
	        GroupFrameT("Arrows"),
	        MUIA_Group_Spacing, 0,
  	        Child, MakeSpacer(),
	        Child, HGroup,
	            MUIA_Group_Spacing, 0,
  	            Child, MakeSpacer(),
	            Child, ColGroup(4),
	                MUIA_Group_SameHeight, TRUE,
	                Child, FreeLabel("up"),
	                Child, d.arrow_up_popimage = MakeArrowPopimage("Arrow up"),
	                Child, d.arrow_down_popimage = MakeArrowPopimage("Arrow down"),
	                Child, FreeLLabel("down"),
	                Child, FreeLabel("left"),
	                Child, d.arrow_left_popimage = MakeArrowPopimage("Arrow left"),
	                Child, d.arrow_right_popimage = MakeArrowPopimage("Arrow right"),
	                Child, FreeLLabel("right"),
	                End, /* ColGroup(6) */
  	            Child, MakeSpacer(),
	            End, /* HGroup */
	        Child, MakeSpacer(),
	        End, /* Arrows */
	
	    Child, VGroup,
	        GroupFrameT("Bar"),
	        Child, HGroup,
	            Child, Label("Gadget Type:"),
	            Child, d.gadget_type_cycle = MakeCycle("Gadget Type:", gadget_type_labels),
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
			         MUIA_Window_Title, (IPTR)"Scroller",
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
		MUIA_CycleChain, 1,
		Child, d.pos_radios[0] = MakeSingleRadio(),
		Child, d.pos_radios[1] = MakeSingleRadio(),
	        Child, d.pos_radios[2] = MakeSingleRadio(),
		Child, MakeScrollbar(MUIV_Scrollbar_Type_Top),
		Child, MakeScrollbar(MUIV_Scrollbar_Type_Sym),
		Child, MakeScrollbar(MUIV_Scrollbar_Type_Bottom),
		End,
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

    DoMethod(d.pos_radios[0], MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
	     d.pos_radios[1], 3, MUIM_NoNotifySet, MUIA_Selected, FALSE);
    DoMethod(d.pos_radios[0], MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
	     d.pos_radios[2], 3, MUIM_NoNotifySet, MUIA_Selected, FALSE);
    DoMethod(d.pos_radios[1], MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
	     d.pos_radios[0], 3, MUIM_NoNotifySet, MUIA_Selected, FALSE);
    DoMethod(d.pos_radios[1], MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
	     d.pos_radios[2], 3, MUIM_NoNotifySet, MUIA_Selected, FALSE);
    DoMethod(d.pos_radios[2], MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
	     d.pos_radios[0], 3, MUIM_NoNotifySet, MUIA_Selected, FALSE);
    DoMethod(d.pos_radios[2], MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
	     d.pos_radios[1], 3, MUIM_NoNotifySet, MUIA_Selected, FALSE);

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
    int pos;

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

/* Radio (Arrangement) */
    pos = DoMethod(msg->configdata, MUIM_Configdata_GetULong,
		   MUICFG_Scrollbar_Arrangement);
    if (pos < 0 || pos > 2)
	pos = 0;
    set(data->pos_radios[pos], MUIA_Selected, TRUE);

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
    int pos;
    int i;

/* Frame */
    str = (STRPTR)xget(data->popframe, MUIA_Framedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetFramespec, MUICFG_Frame_Prop,
	     (IPTR)str);

/* Cycles */
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Scrollbar_Type,
	     xget(data->gadget_type_cycle, MUIA_Cycle_Active));

/* Radio */
    for (i = 0; i < 3; i++)
    {
	if (xget(data->pos_radios[i], MUIA_Selected))
	    pos = i;
    }

    DoMethod(msg->configdata, MUIM_Configdata_SetULong,
	     MUICFG_Scrollbar_Arrangement, pos);

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
