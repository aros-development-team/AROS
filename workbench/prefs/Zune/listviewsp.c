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

struct MUI_ListviewsPData
{
    Object *multi_cycle;
    Object *refresh_cycle;
    Object *smooth_checkmark;
    Object *smooth_slider;

    Object *fonts_normal_string;
    Object *fonts_fixed_string;
    Object *fonts_leading_slider;

    Object *input_popframe;
    Object *input_popimage;
    Object *readonly_popframe;
    Object *readonly_popimage;

    Object *active_popimage;
    Object *selected_popimage;
    Object *activeselected_popimage;
};

static CONST_STRPTR multi_labels[] =
{
    "shifted",
    "always",
    NULL,
};

static CONST_STRPTR refresh_labels[] =
{
    "linear",
    "mixed",
    NULL,
};

static Object*MakeListviewspSlider (void)
{
    Object *obj = MUI_MakeObject(MUIO_Slider, (IPTR)"", 0, 9);
    set(obj, MUIA_CycleChain, 1);
    return obj;
}

static IPTR ListviewsP_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ListviewsPData *data;
    struct MUI_ListviewsPData d;
    
    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
        
        MUIA_Group_Columns, 2,
        MUIA_Group_SameSize, TRUE,
        Child, (IPTR) VGroup,
            GroupFrameT("Control"),
            MUIA_Group_VertSpacing, 0,
            Child, (IPTR) VSpace(0),
            Child, (IPTR) ColGroup(2),
                MUIA_Group_VertSpacing, 2,
                Child, (IPTR) Label("Multi:"),
                Child, (IPTR) d.multi_cycle = MakeCycle("Multi:", multi_labels),
                Child, (IPTR) Label("Refresh:"),
                Child, (IPTR) d.refresh_cycle = MakeCycle("Refresh:", refresh_labels),
                Child, (IPTR) Label("Smooth:"),
                Child, (IPTR) HGroup,
                    MUIA_Group_HorizSpacing, 4,
                    Child, (IPTR) d.smooth_checkmark = MakeCheck(NULL),
                    Child, (IPTR) d.smooth_slider = MakeListviewspSlider(),
                End, /* HGroup */
            End, /* ColGroup */
            Child, (IPTR) VSpace(0),
        End, /* Control VGroup */
    
        Child, (IPTR) VGroup,
            GroupFrameT("Fonts"),
            MUIA_Group_VertSpacing, 0,
            Child, (IPTR) VSpace(0),
            Child, (IPTR) ColGroup(2),
                MUIA_Group_VertSpacing, 2,
                Child, (IPTR) Label("Normal:"),
                Child, (IPTR) d.fonts_normal_string = MakePopfont(FALSE),
                Child, (IPTR) Label("Fixed:"),
                Child, (IPTR) d.fonts_fixed_string = MakePopfont(TRUE),
                Child, (IPTR) Label("Leading:"),
                Child, (IPTR) d.fonts_leading_slider = MakeListviewspSlider(),
            
            End, /* ColGroup */
            Child, (IPTR) VSpace(0),
        End, /* Fonts */
            
        Child, (IPTR) ColGroup(3),
            GroupFrameT("Design"),
            MUIA_Group_VertSpacing, 2,
            Child, (IPTR) FreeLabel("Input \nLists:"),
            Child, (IPTR) d.input_popframe = MakePopframe(),                  
            Child, (IPTR) d.input_popimage = MakeBackgroundPopimage(),
            Child, (IPTR) FreeLabel("Readonly \nLists:"),
            Child, (IPTR) d.readonly_popframe = MakePopframe(),                  
            Child, (IPTR) d.readonly_popimage = MakeBackgroundPopimage(),
            Child, (IPTR) VSpace(0),
            Child, (IPTR) CLabel("Frame"),
            Child, (IPTR) CLabel("Background"),
        End, /* Design ColGroup(3) */
            
        Child, (IPTR) ColGroup(3),
            GroupFrameT("Cursor"),
            MUIA_Group_VertSpacing, 2,
            MUIA_Group_SameSize, TRUE,
            Child, (IPTR) FreeLabel("Active:"),
            Child, (IPTR) d.active_popimage = MakeBackgroundPopimage(),
            Child, (IPTR) VGroup,
                Child, (IPTR) HVSpace,
                Child, (IPTR) CLabel("Active &\nSelected:"),
            End, /* VGroup */
            Child, (IPTR) FreeLabel("Selected:"),
            Child, (IPTR) d.selected_popimage = MakeBackgroundPopimage(),
            Child, (IPTR) d.activeselected_popimage = MakeBackgroundPopimage(),
        End, /* Cursor ColGroup */
            
        TAG_MORE, (IPTR) msg->ops_AttrList
    );

    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    *data = d;
    set(data->refresh_cycle, MUIA_CycleChain, 1);
    set(data->multi_cycle, MUIA_CycleChain, 1);

    DoMethod
    (
        d.smooth_checkmark, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        (IPTR) d.smooth_slider, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue
    );

    return (IPTR)obj;
}


static IPTR ListviewsP_ConfigToGadgets(struct IClass *cl, Object *obj,
				    struct MUIP_Settingsgroup_ConfigToGadgets *msg)
{
    struct MUI_ListviewsPData *data = INST_DATA(cl, obj);
    STRPTR spec;

/* Backgrounds */
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Background_List);
    set(data->input_popimage, MUIA_Imagedisplay_Spec, (IPTR)spec);

    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Background_ReadList);
    set(data->readonly_popimage, MUIA_Imagedisplay_Spec, (IPTR)spec);

    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Background_ListCursor);
    set(data->active_popimage, MUIA_Imagedisplay_Spec, (IPTR)spec);

    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Background_ListSelect);
    set(data->selected_popimage, MUIA_Imagedisplay_Spec, (IPTR)spec);

    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Background_ListSelCur);
    set(data->activeselected_popimage, MUIA_Imagedisplay_Spec, (IPTR)spec);

/* Frames */
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Frame_InputList);
    set(data->input_popframe, MUIA_Framedisplay_Spec, (IPTR)spec);

    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Frame_ReadList);
    set(data->readonly_popframe, MUIA_Framedisplay_Spec, (IPTR)spec);

/* Font */
    setstring(data->fonts_normal_string, (IPTR)FindFont(MUICFG_Font_List));
    setstring(data->fonts_fixed_string, (IPTR)FindFont(MUICFG_Font_Fixed));

/* Sliders */
    setslider(data->smooth_slider,
	      DoMethod(msg->configdata, MUIM_Configdata_GetULong,
		       MUICFG_Listview_SmoothVal));
    setslider(data->fonts_leading_slider,
	      DoMethod(msg->configdata, MUIM_Configdata_GetULong,
		       MUICFG_Listview_Font_Leading));

/* Checkmarks */
    setcheckmark(data->smooth_checkmark,
		 DoMethod(msg->configdata, MUIM_Configdata_GetULong,
			  MUICFG_Listview_Smoothed));
/* Cycles */
    setcycle(data->multi_cycle,
	     DoMethod(msg->configdata, MUIM_Configdata_GetULong,
		      MUICFG_Listview_Multi));

    setcycle(data->refresh_cycle,
	     DoMethod(msg->configdata, MUIM_Configdata_GetULong,
		      MUICFG_Listview_Refresh));

    return 1;    
}


static IPTR ListviewsP_GadgetsToConfig(struct IClass *cl, Object *obj,
				    struct MUIP_Settingsgroup_GadgetsToConfig *msg)
{
    struct MUI_ListviewsPData *data = INST_DATA(cl, obj);
    STRPTR str;

/* Font */
    str = getstring(data->fonts_normal_string);
    DoMethod(msg->configdata, MUIM_Configdata_SetFont, MUICFG_Font_List, (IPTR)str);

    str = getstring(data->fonts_fixed_string);
    DoMethod(msg->configdata, MUIM_Configdata_SetFont, MUICFG_Font_Fixed, (IPTR)str);

/* Frames */
    str = (STRPTR)XGET(data->input_popframe, MUIA_Framedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetFramespec, MUICFG_Frame_InputList,
	     (IPTR)str);

    str = (STRPTR)XGET(data->readonly_popframe, MUIA_Framedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetFramespec, MUICFG_Frame_ReadList,
	     (IPTR)str);

/* Backgrounds */
    str = (STRPTR)XGET(data->input_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_List,
	     (IPTR)str);

    str = (STRPTR)XGET(data->readonly_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_ReadList,
	     (IPTR)str);

    str = (STRPTR)XGET(data->active_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_ListCursor,
	     (IPTR)str);

    str = (STRPTR)XGET(data->selected_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_ListSelect,
	     (IPTR)str);

    str = (STRPTR)XGET(data->activeselected_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_ListSelCur,
	     (IPTR)str);

/* Sliders */
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Listview_SmoothVal,
	     XGET(data->smooth_slider, MUIA_Numeric_Value));

    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Listview_Font_Leading,
	     XGET(data->fonts_leading_slider, MUIA_Numeric_Value));

/* Checkmarks */
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Listview_Smoothed,
	     XGET(data->smooth_checkmark, MUIA_Selected));

/* Cycles */
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Listview_Multi,
	     XGET(data->multi_cycle, MUIA_Cycle_Active));

    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Listview_Refresh,
	     XGET(data->refresh_cycle, MUIA_Cycle_Active));

    return TRUE;
}


BOOPSI_DISPATCHER(IPTR, ListviewsP_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return ListviewsP_New(cl, obj, (struct opSet *)msg);
	case MUIM_Settingsgroup_ConfigToGadgets: return ListviewsP_ConfigToGadgets(cl,obj,(APTR)msg);break;
	case MUIM_Settingsgroup_GadgetsToConfig: return ListviewsP_GadgetsToConfig(cl,obj,(APTR)msg);break;
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUIP_Listviews_desc = { 
    "Listviews",
    MUIC_Group,
    sizeof(struct MUI_ListviewsPData),
    (void*)ListviewsP_Dispatcher 
};
