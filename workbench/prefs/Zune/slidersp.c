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

struct MUI_SlidersPData
{
    Object *container_background_popimage;
    Object *container_popframe;
    Object *knob_background_popimage;
    Object *knob_popframe;
    Object *knob_font_string;
};


static Object *MakeSmallHorizSlider(void)
{
    Object *obj = MUI_MakeObject(MUIO_Slider, (IPTR)"", 0, 9);
    set(obj, MUIA_CycleChain, 1);
    set(obj, MUIA_Numeric_Value, 5);
    return obj;
}

static Object *MakeBigHorizSlider(void)
{
    Object *obj = MUI_MakeObject(MUIO_Slider, (IPTR)"", 0, 99);
    set(obj, MUIA_CycleChain, 1);
    set(obj, MUIA_Numeric_Value, 42);
    return obj;
}

static Object *MakeSmallVertSlider(void)
{
    return SliderObject,
	MUIA_Slider_Horiz, FALSE,
	MUIA_CycleChain, 1,
	MUIA_Numeric_Min, 0,
	MUIA_Numeric_Max, 9,
	MUIA_Numeric_Value, 5,
	End;
}

static Object *MakeBigVertSlider(void)
{
    return SliderObject,
	MUIA_Slider_Horiz, FALSE,
	MUIA_CycleChain, 1,
	MUIA_Numeric_Min, 0,
	MUIA_Numeric_Max, 99,
	MUIA_Numeric_Value, 42,
	End;
}

static IPTR SlidersP_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_SlidersPData *data;
    struct MUI_SlidersPData d;
    
    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
        
        MUIA_Group_Horiz, FALSE,
        
        Child, (IPTR) HGroup,
            MUIA_VertWeight, 800,
            Child, (IPTR) HGroup,
                GroupFrameT("Container Design"),
                Child, (IPTR) HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    Child, (IPTR) VGroup,
                        MUIA_Group_VertSpacing, 1,
                        Child, (IPTR) (d.container_background_popimage = MakeBackgroundPopimage()),
                        Child, (IPTR) CLabel("Background"),
                    End, /* VGroup BG */
                    Child, (IPTR) VGroup,
                        MUIA_Group_VertSpacing, 1,
                        Child, (IPTR) (d.container_popframe = MakePopframe()),
                        Child, (IPTR) CLabel("Frame"),
                    End, /* VGroup Frame */
                End, /* HGroup Frame/BG */
            End, /* HGroup Container Design */
            Child, (IPTR) VGroup,
                GroupFrameT("Knob Design"),
                Child, (IPTR) HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    Child, (IPTR) VGroup,
                        MUIA_Group_VertSpacing, 1,
                        Child, (IPTR) (d.knob_background_popimage = MakeBackgroundPopimage()),
                        Child, (IPTR) CLabel("Background"),
                    End, /* VGroup BG */
                    Child, (IPTR) VGroup,
                        MUIA_Group_VertSpacing, 1,
                        Child, (IPTR) (d.knob_popframe = MakePopframe()),
                        Child, (IPTR) CLabel("Frame"),
                    End, /* VGroup Frame */
                End, /* HGroup Frame/BG */
                Child, (IPTR) HGroup,
                    Child, (IPTR) Label2("Font:"),
                    Child, (IPTR) (d.knob_font_string = MakePopfont(FALSE)),
                End, /* HGroup font */
            End, /* VGroup Knob Design */
        End, /* HGroup Container/Knob design */
        
        Child, (IPTR) VGroup,
            GroupFrameT("Example Sliders"),
            Child, (IPTR) VSpace(0),
            Child, (IPTR) HGroup,
                Child, (IPTR) VGroup,
                    Child, (IPTR) MakeSmallHorizSlider(),
                    Child, (IPTR) MakeBigHorizSlider(),
                End, /* VGroup horiz sliders */
                Child, (IPTR) MakeSmallVertSlider(),
                Child, (IPTR) MakeBigVertSlider(),
            End, /* HGroup */
            Child, (IPTR) HVSpace,
        End, /* VGroup Slider examples */
        
        TAG_MORE, (IPTR) msg->ops_AttrList
    );

    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    *data = d;

    return (IPTR)obj;
}


/*
 * MUIM_Settingsgroup_ConfigToGadgets
 */
static IPTR SlidersP_ConfigToGadgets(struct IClass *cl, Object *obj,
				    struct MUIP_Settingsgroup_ConfigToGadgets *msg)
{
    struct MUI_SlidersPData *data = INST_DATA(cl, obj);
    STRPTR spec;

/* Frame */
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Frame_Slider);
    set(data->container_popframe, MUIA_Framedisplay_Spec, (IPTR)spec);

    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Frame_Knob);
    set(data->knob_popframe, MUIA_Framedisplay_Spec, (IPTR)spec);

/* Images */
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Background_Slider);
    set(data->container_background_popimage,MUIA_Imagedisplay_Spec, (IPTR)spec);
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Background_SliderKnob);
    set(data->knob_background_popimage,MUIA_Imagedisplay_Spec, (IPTR)spec);

/* Fonts */
    setstring(data->knob_font_string, (IPTR)FindFont(MUICFG_Font_Knob));

    return 1;
}


/*
 * MUIM_Settingsgroup_ConfigToGadgets
 */
static IPTR SlidersP_GadgetsToConfig(struct IClass *cl, Object *obj,
				    struct MUIP_Settingsgroup_GadgetsToConfig *msg)
{
    struct MUI_SlidersPData *data = INST_DATA(cl, obj);
    STRPTR str;

/* Fonts */
    str = getstring(data->knob_font_string);
    DoMethod(msg->configdata, MUIM_Configdata_SetFont, MUICFG_Font_Knob, (IPTR)str);

/* Backgrounds */
    str = (STRPTR)XGET(data->container_background_popimage,MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_Slider,
	     (IPTR)str);

    str = (STRPTR)XGET(data->knob_background_popimage,MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_SliderKnob,
	     (IPTR)str);

/* Frame */
    str = (STRPTR)XGET(data->container_popframe, MUIA_Framedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetFramespec, MUICFG_Frame_Slider,
	     (IPTR)str);
    str = (STRPTR)XGET(data->knob_popframe, MUIA_Framedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetFramespec, MUICFG_Frame_Knob,
	     (IPTR)str);
    return TRUE;
}


BOOPSI_DISPATCHER(IPTR, SlidersP_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return SlidersP_New(cl, obj, (struct opSet *)msg);
	case MUIM_Settingsgroup_ConfigToGadgets: return SlidersP_ConfigToGadgets(cl,obj,(APTR)msg);break;
	case MUIM_Settingsgroup_GadgetsToConfig: return SlidersP_GadgetsToConfig(cl,obj,(APTR)msg);break;
    }
    
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUIP_Sliders_desc = { 
    "Sliders",
    MUIC_Group,
    sizeof(struct MUI_SlidersPData),
    (void*)SlidersP_Dispatcher 
};
