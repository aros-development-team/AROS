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
        
        Child, HGroup,
            Child, VGroup, /* Text Buttons */
                Child, ColGroup(2),
                    GroupFrameT("General"),                    
	            MUIA_Group_SameHeight, TRUE,
	            MUIA_Group_VertSpacing, 2,
                    Child, FreeLabel("Background:"),
                    Child, d.text_background_popimage = MakeBackgroundPopimage(),
                    Child, FreeLabel("Background in\npressed state:"),
                    Child, d.text_selbackground_popimage = MakeBackgroundPopimage(),
                    End,
                Child, ColGroup(2),
                    GroupFrameT("Text Buttons"),
	            MUIA_Group_VertSpacing, 2,
                    Child, FreeLabel("Frame:"),
                    Child, d.button_popframe = MakePopframe(),                  
                    Child, Label("Font:"),
                    Child, PopaslObject,
                        MUIA_Popasl_Type, ASL_FontRequest,
                        MUIA_Popstring_String, d.text_font_string = StringObject,
                            MUIA_CycleChain, 1,
                            StringFrame, 
                            End,
                        MUIA_Popstring_Button, PopButton(MUII_PopUp),
                        End,
                    End,
                End, /* Text Buttons */
            Child, VGroup, /* other buttons */
                Child, HGroup, /* Image Buttons */
                    GroupFrameT("Image Buttons"),
                    
                    Child, HSpace(0),
                    Child, FreeLabel("Frame:"),
                    Child, d.imagebutton_popframe = NewObject(CL_FrameClipboard->mcc_Class, NULL,
                            MUIA_Draggable, TRUE, 
                            MUIA_CycleChain, 1,
                            MUIA_MaxWidth, 28,
                            MUIA_Window_Title, (IPTR)"Adjust Frame",
                            TAG_DONE),
                    Child, HSpace(0),
                    End, /* Image Buttons */
                Child, HGroup, /* Checkmarks */
                    GroupFrameT("Checkmarks"),
                        Child, HSpace(0),
                        Child, FreeLabel("Look:"),
                        Child, d.checkmark_look_popimage = NewObject(CL_ImageClipboard->mcc_Class, NULL,
                            MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Image,
                            MUIA_Draggable, TRUE,
                            MUIA_CycleChain, 1,
                            MUIA_FixWidth, 28,
                            MUIA_MaxHeight, 28,
                            MUIA_Imagedisplay_FreeHoriz, FALSE,
                            MUIA_Imagedisplay_FreeVert, FALSE,
                            MUIA_Window_Title, (IPTR)"Checkmark",
                            TAG_DONE),
                        Child, HSpace(0),
                    End, /* Checkmarks */
                Child, HGroup, /* Radio Buttons */
                    GroupFrameT("Radio Buttons"),
                    /* MUIA_Group_SameHeight, TRUE, */
                    
	            Child, HSpace(0),
                    Child, VGroup,
                        MUIA_Group_VertSpacing, 1,
                        Child, d.radio_look_popimage = NewObject(CL_ImageClipboard->mcc_Class, NULL,
                            MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Image,
                            MUIA_Draggable, TRUE, 
                            MUIA_CycleChain, 1,
                            MUIA_MaxWidth, 28,
                            MUIA_FixHeight, 28,
			    MUIA_Weight, 300,
                            MUIA_Imagedisplay_FreeHoriz, FALSE,
                            MUIA_Imagedisplay_FreeVert, FALSE,
                            MUIA_Window_Title, (IPTR)"Radiobutton",
                            TAG_DONE),
	                Child, CLabel("Look"),
                        End,
                    Child, HSpace(4),
                    Child, ColGroup(2),
                        MUIA_Group_VertSpacing, 1,
                        MUIA_Group_HorizSpacing, 2,
                        Child, Label("H"),
                        Child, d.spacing_horiz_slider = MakeSpacingSlider(),
                        Child, Label("V"),
                        Child, d.spacing_vert_slider = MakeSpacingSlider(),
                        Child, HVSpace,
                        Child, CLabel("Spacing"),
                        End, /* ColGroup */
                    Child, HSpace(0),
                    End, /* Radio Buttons */
                End, /* other buttons */
            End, /* obj */
        TAG_MORE, msg->ops_AttrList
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
    str = (STRPTR)xget(data->text_background_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_Button,
	     (IPTR)str);

    str = (STRPTR)xget(data->text_selbackground_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_Selected,
	     (IPTR)str);

/* Frames */
    str = (STRPTR)xget(data->button_popframe, MUIA_Framedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetFramespec, MUICFG_Frame_Button,
	     (IPTR)str);
    str = (STRPTR)xget(data->imagebutton_popframe, MUIA_Framedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetFramespec, MUICFG_Frame_ImageButton,
	     (IPTR)str);

/* Looks */
    str = (STRPTR)xget(data->radio_look_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Image_RadioButton,
	     (IPTR)str);

    str = (STRPTR)xget(data->checkmark_look_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Image_CheckMark,
	     (IPTR)str);

/* Spacing */
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Radio_HSpacing,
	     xget(data->spacing_horiz_slider, MUIA_Numeric_Value));
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Radio_VSpacing,
	     xget(data->spacing_vert_slider, MUIA_Numeric_Value));

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

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUIP_Buttons_desc = { 
    "Buttons",
    MUIC_Group, 
    sizeof(struct MUI_ButtonsPData),
    (void*)ButtonsP_Dispatcher 
};
