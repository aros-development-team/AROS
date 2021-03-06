/*
    Copyright (C) 2002-2006, The AROS Development Team. All rights reserved.
*/

#include <string.h>
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

extern struct Library *MUIMasterBase;

struct MUI_ButtonsPData
{
    Object *text_font_string;
    Object *text_background_popimage;
    Object *text_selbackground_popimage;
    Object *spacing_vert_slider;
    Object *spacing_horiz_slider;
    Object *radio_look_popimage;
    Object *checkmark_look_popimage;
    Object *button_popframe;
    Object *imagebutton_popframe;
};

static IPTR ButtonsP_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ButtonsPData *data;
    struct MUI_ButtonsPData d;
    
    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
        
        Child, (IPTR) HGroup,
            Child, (IPTR) VGroup, /* Text Buttons */
                MUIA_Weight, 130,
                Child, (IPTR) ColGroup(2),
                    GroupFrameT(_(MSG_GENERAL)),
                    MUIA_Group_SameHeight, TRUE,
                    MUIA_Group_VertSpacing, 1,
                    Child, (IPTR) FreeLabel(_(MSG_BACKGROUND_COLON)),
                    Child, (IPTR) (d.text_background_popimage = MakeBackgroundPopimage()),
        Child, RectangleObject, MUIA_MaxHeight, 0, End,
        Child, HGroup,
        Child, RectangleObject, MUIA_MaxHeight, 0, MUIA_FixWidth, 28, End,
        Child, RectangleObject, MUIA_MaxHeight, 0, End,
        End,
                    Child, (IPTR) FreeLabel(_(MSG_BACKGROUND_PRESSED)),
                    Child, (IPTR) (d.text_selbackground_popimage = MakeBackgroundPopimage()),
                    End,
                Child, (IPTR) ColGroup(2),
                    GroupFrameT(_(MSG_TEXT_BUTTONS)),
                    MUIA_Group_VertSpacing, 2,
                    Child, (IPTR) FreeLabel(_(MSG_FRAME_COLON)),
                    Child, (IPTR) (d.button_popframe = MakePopframe()),
                    Child, (IPTR) Label(_(MSG_FONT)),
                    Child, (IPTR) (d.text_font_string = MakePopfont(FALSE)),
                End,
            End, /* Text Buttons */
            Child, (IPTR) VGroup, /* other buttons */
                Child, (IPTR) HGroup, /* Image Buttons */
                    GroupFrameT(_(MSG_IMAGE_BUTTONS)),
                    
                    Child, (IPTR) HSpace(0),
                    Child, (IPTR) FreeLabel(_(MSG_FRAME_COLON)),
                    Child, (IPTR) (d.imagebutton_popframe = MUI_NewObject
                    (
                        MUIC_Popframe,
                        MUIA_CycleChain,          1,
                        MUIA_FixWidth,            28,
                        MUIA_Window_Title, (IPTR) _(MSG_ADJUST_FRAME),
                        TAG_DONE
                    )),
                    Child, (IPTR) HSpace(0),
                End, /* Image Buttons */
                Child, (IPTR) HGroup, /* Checkmarks */
                    GroupFrameT(_(MSG_CHECKMARKS)),
                    Child, (IPTR) HSpace(0),
                    Child, (IPTR) FreeLabel(_(MSG_LOOK_COLON)),
                    Child, (IPTR) (d.checkmark_look_popimage = MUI_NewObject
                    (
                        MUIC_Popimage,
                        MUIA_Imageadjust_Type,       MUIV_Imageadjust_Type_Image,
                        MUIA_CycleChain,             1,
                        MUIA_FixWidth,               28,
                        MUIA_MaxHeight,              28,
                        MUIA_Imagedisplay_FreeHoriz, FALSE,
                        MUIA_Imagedisplay_FreeVert,  FALSE,
                        MUIA_Window_Title,           (IPTR) _(MSG_CHECKMARK),
                        TAG_DONE
                    )),
                    Child, (IPTR) HSpace(0),
                    End, /* Checkmarks */
                Child, (IPTR) HGroup, /* Radio Buttons */
                    GroupFrameT(_(MSG_RADIO_BUTTONS)),
                    /* MUIA_Group_SameHeight, TRUE, */
                    
                    Child, (IPTR) HSpace(0),
                    Child, (IPTR) VGroup,
                        MUIA_Group_VertSpacing, 1,
                        Child, (IPTR) (d.radio_look_popimage = MUI_NewObject
                        (
                            MUIC_Popimage,
                            MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Image,
                            MUIA_CycleChain, 1,
                            MUIA_MaxWidth, 28,
                            MUIA_FixHeight, 28,
                            MUIA_Weight, 300,
                            MUIA_Imagedisplay_FreeHoriz, FALSE,
                            MUIA_Imagedisplay_FreeVert, FALSE,
                            MUIA_Window_Title, (IPTR) _(MSG_RADIOBUTTON),
                            TAG_DONE
                        )),
                        Child, (IPTR) CLabel(_(MSG_LOOK)),
                    End,
                    Child, (IPTR) HSpace(4),
                    Child, (IPTR) ColGroup(2),
                        MUIA_Group_VertSpacing, 1,
                        MUIA_Group_HorizSpacing, 2,
                        Child, (IPTR) Label(_(MSG_H)),
                        Child, (IPTR) (d.spacing_horiz_slider = MakeSpacingSlider()),
                        Child, (IPTR) Label(_(MSG_V)),
                        Child, (IPTR) (d.spacing_vert_slider = MakeSpacingSlider()),
                        Child, (IPTR) HVSpace,
                        Child, (IPTR) CLabel(_(MSG_SPACING)),
                        End, /* ColGroup */
                    Child, (IPTR) HSpace(0),
                    End, /* Radio Buttons */
                End, /* other buttons */
            End, /* obj */
        TAG_MORE, (IPTR) msg->ops_AttrList
    );

    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    *data = d;

    return (IPTR)obj;
}


static IPTR ButtonsP_ConfigToGadgets(struct IClass *cl, Object *obj,
                                     struct MUIP_Settingsgroup_ConfigToGadgets *msg)
{
    struct MUI_ButtonsPData *data = INST_DATA(cl, obj);
    STRPTR spec;

/* Font */
    setstring(data->text_font_string, (IPTR)FindFont(MUICFG_Font_Button));

/* Backgrounds */
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
                            MUICFG_Background_Button);
    set(data->text_background_popimage, MUIA_Imagedisplay_Spec, (IPTR)spec);

    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
                            MUICFG_Background_Selected);
    set(data->text_selbackground_popimage, MUIA_Imagedisplay_Spec, (IPTR)spec);

/* Frames */
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
                            MUICFG_Frame_Button);
    set(data->button_popframe, MUIA_Framedisplay_Spec, (IPTR)spec);
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
                            MUICFG_Frame_ImageButton);
    set(data->imagebutton_popframe, MUIA_Framedisplay_Spec, (IPTR)spec);

/* Looks */
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
                            MUICFG_Image_RadioButton);
    set(data->radio_look_popimage, MUIA_Imagedisplay_Spec, (IPTR)spec);

    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
                            MUICFG_Image_CheckMark);
    set(data->checkmark_look_popimage, MUIA_Imagedisplay_Spec, (IPTR)spec);

/* Spacing */
    setslider(data->spacing_horiz_slider,
              DoMethod(msg->configdata, MUIM_Configdata_GetULong,
                       MUICFG_Radio_HSpacing));
    setslider(data->spacing_vert_slider,
              DoMethod(msg->configdata, MUIM_Configdata_GetULong,
                       MUICFG_Radio_VSpacing));
    return 1;
}


static IPTR ButtonsP_GadgetsToConfig(struct IClass *cl, Object *obj,
                                     struct MUIP_Settingsgroup_GadgetsToConfig *msg)
{
    struct MUI_ButtonsPData *data = INST_DATA(cl, obj);
    STRPTR str;

/* Font */
    str = getstring(data->text_font_string);
    DoMethod(msg->configdata, MUIM_Configdata_SetFont, MUICFG_Font_Button, (IPTR)str);

/* Backgrounds */
    str = (STRPTR)XGET(data->text_background_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_Button,
             (IPTR)str);

    str = (STRPTR)XGET(data->text_selbackground_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_Selected,
             (IPTR)str);

/* Frames */
    str = (STRPTR)XGET(data->button_popframe, MUIA_Framedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetFramespec, MUICFG_Frame_Button,
             (IPTR)str);
    str = (STRPTR)XGET(data->imagebutton_popframe, MUIA_Framedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetFramespec, MUICFG_Frame_ImageButton,
             (IPTR)str);

/* Looks */
    str = (STRPTR)XGET(data->radio_look_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Image_RadioButton,
             (IPTR)str);

    str = (STRPTR)XGET(data->checkmark_look_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Image_CheckMark,
             (IPTR)str);

/* Spacing */
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Radio_HSpacing,
             XGET(data->spacing_horiz_slider, MUIA_Numeric_Value));
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Radio_VSpacing,
             XGET(data->spacing_vert_slider, MUIA_Numeric_Value));

    return TRUE;
}


BOOPSI_DISPATCHER(IPTR, ButtonsP_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW: return ButtonsP_New(cl, obj, (struct opSet *)msg);
        case MUIM_Settingsgroup_ConfigToGadgets: return ButtonsP_ConfigToGadgets(cl,obj,(APTR)msg);break;
        case MUIM_Settingsgroup_GadgetsToConfig: return ButtonsP_GadgetsToConfig(cl,obj,(APTR)msg);break;
    }
    
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUIP_Buttons_desc = {
    "Buttons",
    MUIC_Group,
    sizeof(struct MUI_ButtonsPData),
    (void*)ButtonsP_Dispatcher
};


static unsigned char default_icon[] =
{
    0x00, 0x00, 0x00, 0x18,  // width
    0x00, 0x00, 0x00, 0x13,  // height
    'B', 'Z', '2', '\0',
    0x00, 0x00, 0x00, 0x80,  // number of bytes

    0x42, 0x5a, 0x68, 0x39, 0x31, 0x41, 0x59, 0x26, 0x53, 0x59, 0x15, 0xbf,
    0x2e, 0x12, 0x00, 0x02, 0x94, 0xfa, 0x46, 0xa2, 0x22, 0x22, 0x00, 0x20,
    0x00, 0x00, 0x02, 0x00, 0x01, 0x00, 0x01, 0x00, 0x08, 0x42, 0x40, 0x00,
    0x00, 0xb0, 0x00, 0x9b, 0x58, 0x42, 0xa8, 0x0d, 0x06, 0x80, 0xa5, 0x48,
    0x00, 0x00, 0xa8, 0xa6, 0xa3, 0x44, 0x0d, 0x3c, 0x24, 0x31, 0x7b, 0xa7,
    0x17, 0xac, 0xdf, 0x3d, 0x71, 0x99, 0xc8, 0x6d, 0xa8, 0xd4, 0x6a, 0x2b,
    0xa5, 0xed, 0x4e, 0x4a, 0x96, 0x12, 0xe8, 0x96, 0x12, 0xec, 0x97, 0xaa,
    0x93, 0xe5, 0x49, 0x9a, 0x42, 0x79, 0x36, 0x84, 0xb6, 0xec, 0x6e, 0xe5,
    0xac, 0x8b, 0x29, 0x90, 0xc0, 0xc4, 0x68, 0x16, 0xa0, 0xb4, 0x12, 0x4d,
    0x35, 0x4c, 0x44, 0x40, 0x8b, 0x6c, 0x02, 0xc0, 0xbf, 0x8b, 0xb9, 0x22,
    0x9c, 0x28, 0x48, 0x0a, 0xdf, 0x97, 0x09, 0x00,
};


Object *buttonsclass_get_icon(void)
{
    return RawimageObject,
        MUIA_Rawimage_Data, default_icon,
    End;
}
