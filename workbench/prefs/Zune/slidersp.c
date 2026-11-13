/*
    Copyright (C) 2002-2006, The AROS Development Team. All rights reserved.
*/

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <mui/Rawimage_mcc.h>
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
                GroupFrameT(_(MSG_CONTAINER_DESIGN)),
                Child, (IPTR) HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    Child, (IPTR) VGroup,
                        MUIA_Group_VertSpacing, 1,
                        Child, (IPTR) (d.container_background_popimage = MakeBackgroundPopimage()),
                        Child, (IPTR) CLabel(_(MSG_BACKGROUND)),
                    End, /* VGroup BG */
                    Child, (IPTR) VGroup,
                        MUIA_Group_VertSpacing, 1,
                        Child, (IPTR) (d.container_popframe = MakePopframe()),
                        Child, (IPTR) CLabel(_(MSG_FRAME)),
                    End, /* VGroup Frame */
                End, /* HGroup Frame/BG */
            End, /* HGroup Container Design */
            Child, (IPTR) VGroup,
                GroupFrameT(_(MSG_KNOB_DESIGN)),
                Child, (IPTR) HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    Child, (IPTR) VGroup,
                        MUIA_Group_VertSpacing, 1,
                        Child, (IPTR) (d.knob_background_popimage = MakeBackgroundPopimage()),
                        Child, (IPTR) CLabel(_(MSG_BACKGROUND)),
                    End, /* VGroup BG */
                    Child, (IPTR) VGroup,
                        MUIA_Group_VertSpacing, 1,
                        Child, (IPTR) (d.knob_popframe = MakePopframe()),
                        Child, (IPTR) CLabel(_(MSG_FRAME)),
                    End, /* VGroup Frame */
                End, /* HGroup Frame/BG */
                Child, (IPTR) HGroup,
                    Child, (IPTR) Label2(_(MSG_FONT)),
                    Child, (IPTR) (d.knob_font_string = MakePopfont(FALSE)),
                End, /* HGroup font */
            End, /* VGroup Knob Design */
        End, /* HGroup Container/Knob design */
        
        Child, (IPTR) VGroup,
            GroupFrameT(_(MSG_EXAMPLE_SLIDERS)),
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


static const UBYTE icon32[] =
{
    0x00, 0x00, 0x00, 0x18,  // width
    0x00, 0x00, 0x00, 0x13,  // height
    'B', 'Z', '2', '\0',
    0x00, 0x00, 0x00, 0x73,  // number of bytes

    0x42, 0x5a, 0x68, 0x39, 0x31, 0x41, 0x59, 0x26, 0x53, 0x59, 0x78, 0x5d,
    0x6a, 0xd9, 0x00, 0x03, 0x1e, 0x61, 0x12, 0xa0, 0x00, 0x02, 0x00, 0x00,
    0x04, 0x00, 0x01, 0x42, 0x40, 0x00, 0x00, 0xb0, 0x00, 0xb9, 0x21, 0x09,
    0x53, 0x20, 0x43, 0x02, 0x02, 0x92, 0x8a, 0x1e, 0xa7, 0x82, 0xe1, 0xcd,
    0xb3, 0x96, 0xd4, 0xba, 0x28, 0xd3, 0xba, 0x9b, 0x28, 0xf8, 0x15, 0xf8,
    0x21, 0xe0, 0x87, 0x4a, 0x87, 0x67, 0xd5, 0x5b, 0x4d, 0xe5, 0x64, 0x92,
    0x81, 0x80, 0x30, 0x94, 0x35, 0x65, 0x12, 0x25, 0xc4, 0xad, 0x6d, 0x65,
    0xc4, 0x86, 0x9a, 0x1c, 0x4b, 0xac, 0x49, 0x4a, 0x11, 0x28, 0x9a, 0x1d,
    0x22, 0x66, 0xdb, 0x55, 0x37, 0x45, 0x9d, 0x4f, 0xe2, 0xee, 0x48, 0xa7,
    0x0a, 0x12, 0x0f, 0x0b, 0xad, 0x5b, 0x20,
};


Object *slidersclass_get_icon(void)
{
    return RawimageObject,
        MUIA_Rawimage_Data, icon32,
    End;
}
