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

#ifdef __AROS__
#include <proto/alib.h>
#include <proto/intuition.h>
#endif

#include "zunestuff.h"
#include <string.h>

/*  #define DEBUG 1 */
/*  #include <aros/debug.h> */

extern struct Library *MUIMasterBase;

struct MUI_CyclesPData
{
    Object *cycle_popimage;
    Object *menu_position_cycle;
    Object *menu_level_slider;
    Object *menu_speed_slider;
    Object *menu_popframe;
    Object *background_menu_popimage;
    Object *recessed_entries_checkmark;
};

static CONST_STRPTR positions_labels[] =
{
    "below",
    "on active",
    NULL,
};


static Object*MakeLevelSlider (void)
{
    Object *obj = MUI_MakeObject(MUIO_Slider, (IPTR)"", 2, 20);
    set(obj, MUIA_CycleChain, 1);
    return obj;
}


static Object*MakeSpeedSlider (void)
{
    Object *obj = MUI_MakeObject(MUIO_Slider, (IPTR)"", 0, 50);
    set(obj, MUIA_CycleChain, 1);
    return obj;
}


static IPTR CyclesP_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_CyclesPData *data;
    struct MUI_CyclesPData d;
    
    obj = (Object *)DoSuperNew(cl, obj,
			       MUIA_Group_Horiz, FALSE,
			       Child, HGroup,
			       GroupFrameT("Cycle Gadget Design"),
			       Child, HVSpace,
			       Child, d.cycle_popimage =
			       NewObject(CL_ImageClipboard->mcc_Class, NULL,
					 MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Image,
					 MUIA_Draggable, TRUE, 
					 MUIA_CycleChain, 1,
					 MUIA_MaxWidth, 28,
					 MUIA_MaxHeight, 28,
					 MUIA_Imagedisplay_FreeHoriz, FALSE,
					 MUIA_Imagedisplay_FreeVert, FALSE,
					 MUIA_Window_Title, (IPTR)"Cycle",
					 TAG_DONE),
			       Child, HVSpace,
			       End, /* Cycle Gadget Design */
			       Child, HGroup,
			       Child, VGroup,
			       GroupFrameT("Popup Menu Control"),
			       Child, VSpace(0),
			       Child, ColGroup(2),
			       MUIA_Group_VertSpacing, 2,
			       Child, Label("Position:"),
			       Child, d.menu_position_cycle =
			       MakeCycle("Position:", positions_labels),
			       Child, Label("Level:"),
			       Child, d.menu_level_slider = MakeLevelSlider(),
			       Child, Label("Speed:"),
			       Child, d.menu_speed_slider = MakeSpeedSlider(),
			       End,
			       Child, VSpace(0),
			       End, /* Popup Menu Control */
			       Child, VGroup,
			         MUIA_Group_VertSpacing, 2,
			         GroupFrameT("Popup Menu Design"),
			         Child, HGroup,
			           MUIA_Group_SameWidth, TRUE,
			           Child, VGroup,
			               MUIA_Group_VertSpacing, 1,
			               Child, d.menu_popframe = MakePopframe(),
			               Child, CLabel("Frame"),
			               End, /* VGroup */
			           Child, VGroup,
			               MUIA_Group_VertSpacing, 1,
			               Child, d.background_menu_popimage =
			                 NewObject(CL_ImageClipboard->mcc_Class, NULL,
					 MUIA_Draggable, TRUE,
					 MUIA_CycleChain, 1,
					 MUIA_Window_Title, (IPTR)"Adjust Background",
					 TAG_DONE),
			               Child, CLabel((IPTR)"Background"),
			               End, /* VGroup */
			         End, /* HGroup */
			         Child, HGroup,
			           Child, HSpace(0),
			           Child, Label1("Recessed Entries:"),
			           Child, d.recessed_entries_checkmark =
			           MakeCheck(NULL),
			           End, /* HGroup recessed CM */
			       End, /* Popup Menu Design */
			     End, /* HGroup Popup Menu */			       
    	TAG_MORE, msg->ops_AttrList);

    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    *data = d;
    set(data->menu_position_cycle, MUIA_CycleChain, 1);

    return (IPTR)obj;
}


/*
 * MUIM_Settingsgroup_ConfigToGadgets
 */
static IPTR CyclesP_ConfigToGadgets(struct IClass *cl, Object *obj,
				    struct MUIP_Settingsgroup_ConfigToGadgets *msg)
{
    struct MUI_CyclesPData *data = INST_DATA(cl, obj);
    STRPTR spec;

/* Frame */
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Frame_PopUp);
    set(data->menu_popframe, MUIA_Framedisplay_Spec, (IPTR)spec);


/* Images */
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Background_PopUp);
    set(data->background_menu_popimage,MUIA_Imagedisplay_Spec, (IPTR)spec);
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Image_Cycle);
    set(data->cycle_popimage,MUIA_Imagedisplay_Spec, (IPTR)spec);

/* Sliders */
    setslider(data->menu_level_slider,
	      DoMethod(msg->configdata, MUIM_Configdata_GetULong,
		       MUICFG_Cycle_MenuCtrl_Level));
    setslider(data->menu_speed_slider,
	      DoMethod(msg->configdata, MUIM_Configdata_GetULong,
		       MUICFG_Cycle_MenuCtrl_Speed));
/* Checkmark */
    setcheckmark(data->recessed_entries_checkmark,
		 DoMethod(msg->configdata, MUIM_Configdata_GetULong,
			  MUICFG_Cycle_Menu_Recessed));

/* Cycles */
    setcycle(data->menu_position_cycle,
	     DoMethod(msg->configdata, MUIM_Configdata_GetULong,
		      MUICFG_Cycle_MenuCtrl_Position));

    return 1;    
}


/*
 * MUIM_Settingsgroup_ConfigToGadgets
 */
static IPTR CyclesP_GadgetsToConfig(struct IClass *cl, Object *obj,
				    struct MUIP_Settingsgroup_GadgetsToConfig *msg)
{
    struct MUI_CyclesPData *data = INST_DATA(cl, obj);
    STRPTR str;

/* Frame */
    str = (STRPTR)xget(data->menu_popframe, MUIA_Framedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetFramespec, MUICFG_Frame_PopUp,
	     (IPTR)str);
/* Images */
    str = (STRPTR)xget(data->background_menu_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_PopUp,
	     (IPTR)str);
    str = (STRPTR)xget(data->cycle_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Image_Cycle,
	     (IPTR)str);
/* Sliders */
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Cycle_MenuCtrl_Level,
	     xget(data->menu_level_slider, MUIA_Numeric_Value));
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Cycle_MenuCtrl_Speed,
	     xget(data->menu_speed_slider, MUIA_Numeric_Value));
/* Checkmark */
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Cycle_Menu_Recessed,
	     xget(data->recessed_entries_checkmark, MUIA_Selected));
/* Cycles */
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Cycle_MenuCtrl_Position,
	     xget(data->menu_position_cycle, MUIA_Cycle_Active));

    return TRUE;
}


BOOPSI_DISPATCHER(IPTR, CyclesP_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return CyclesP_New(cl, obj, (struct opSet *)msg);
	case MUIM_Settingsgroup_ConfigToGadgets: return CyclesP_ConfigToGadgets(cl,obj,(APTR)msg);break;
	case MUIM_Settingsgroup_GadgetsToConfig: return CyclesP_GadgetsToConfig(cl,obj,(APTR)msg);break;
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUIP_Cycles_desc = { 
    "Cycles",
    MUIC_Group,
    sizeof(struct MUI_CyclesPData),
    (void*)CyclesP_Dispatcher 
};
