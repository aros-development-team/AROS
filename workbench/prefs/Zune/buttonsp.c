/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>
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
                    GroupFrameT("General"),                    
	            MUIA_Group_SameHeight, TRUE,
	            MUIA_Group_VertSpacing, 1,
                    Child, (IPTR) FreeLabel("Background:"),
                    Child, (IPTR) (d.text_background_popimage = MakeBackgroundPopimage()),
	Child, RectangleObject, MUIA_MaxHeight, 0, End,
	Child, HGroup,
	Child, RectangleObject, MUIA_MaxHeight, 0, MUIA_FixWidth, 28, End,
	Child, RectangleObject, MUIA_MaxHeight, 0, End,
	End,
                    Child, (IPTR) FreeLabel("Background in\npressed state:"),
                    Child, (IPTR) (d.text_selbackground_popimage = MakeBackgroundPopimage()),
                    End,
                Child, (IPTR) ColGroup(2),
                    GroupFrameT("Text Buttons"),
	            MUIA_Group_VertSpacing, 2,
                    Child, (IPTR) FreeLabel("Frame:"),
                    Child, (IPTR) (d.button_popframe = MakePopframe()),
                    Child, (IPTR) Label("Font:"),
                    Child, (IPTR) (d.text_font_string = MakePopfont(FALSE)),
                End,
            End, /* Text Buttons */
            Child, (IPTR) VGroup, /* other buttons */
                Child, (IPTR) HGroup, /* Image Buttons */
                    GroupFrameT("Image Buttons"),
                    
                    Child, (IPTR) HSpace(0),
                    Child, (IPTR) FreeLabel("Frame:"),
                    Child, (IPTR) (d.imagebutton_popframe = MUI_NewObject
                    (
                        MUIC_Popframe,
                        MUIA_CycleChain,          1,
                        MUIA_FixWidth,            28,
                        MUIA_Window_Title, (IPTR) "Adjust Frame",
                        TAG_DONE
                    )),
                    Child, (IPTR) HSpace(0),
                End, /* Image Buttons */
                Child, (IPTR) HGroup, /* Checkmarks */
                    GroupFrameT("Checkmarks"),
                    Child, (IPTR) HSpace(0),
                    Child, (IPTR) FreeLabel("Look:"),
                    Child, (IPTR) (d.checkmark_look_popimage = MUI_NewObject
                    (
                        MUIC_Popimage,
                        MUIA_Imageadjust_Type,       MUIV_Imageadjust_Type_Image,
                        MUIA_CycleChain,             1,
                        MUIA_FixWidth,               28,
                        MUIA_MaxHeight,              28,
                        MUIA_Imagedisplay_FreeHoriz, FALSE,
                        MUIA_Imagedisplay_FreeVert,  FALSE,
                        MUIA_Window_Title,           (IPTR) "Checkmark",
                        TAG_DONE
                    )),
                    Child, (IPTR) HSpace(0),
                    End, /* Checkmarks */
                Child, (IPTR) HGroup, /* Radio Buttons */
                    GroupFrameT("Radio Buttons"),
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
                            MUIA_Window_Title, (IPTR)"Radiobutton",
                            TAG_DONE
                        )),
	                Child, (IPTR) CLabel("Look"),
                    End,
                    Child, (IPTR) HSpace(4),
                    Child, (IPTR) ColGroup(2),
                        MUIA_Group_VertSpacing, 1,
                        MUIA_Group_HorizSpacing, 2,
                        Child, (IPTR) Label("H"),
                        Child, (IPTR) (d.spacing_horiz_slider = MakeSpacingSlider()),
                        Child, (IPTR) Label("V"),
                        Child, (IPTR) (d.spacing_vert_slider = MakeSpacingSlider()),
                        Child, (IPTR) HVSpace,
                        Child, (IPTR) CLabel("Spacing"),
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
