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
};

static ULONG DoSuperNew(struct IClass *cl, Object * obj, ULONG tag1,...)
{
    return (DoSuperMethod(cl, obj, OM_NEW, &tag1, NULL));
}

#define FindConfig(id) (void*)DoMethod(msg->configdata,MUIM_Dataspace_Find,id)

static Object*MakeSpacingSlider (void)
{
    return MUI_MakeObject(MUIO_Slider, "", 0, 9);
}

static IPTR ButtonsP_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ButtonsPData *data;
    struct MUI_ButtonsPData d;
    
    obj = (Object *)DoSuperNew(cl, obj,
	Child, HGroup,
	  Child, VGroup, /* Text Buttons */
	    Child, ColGroup(2),
		GroupFrameT("Text Buttons"),
		Child, VGroup,
			       Child, HVSpace,
			       Child, MakeLabel("Frame:"),
			       Child, HVSpace,
			       End,
		Child, PopimageObject, MUIA_Draggable, TRUE, End,
		Child, VGroup,
			       Child, HVSpace,
			       Child, MakeLabel("Background:"),
			       Child, HVSpace,
			       End,
		Child, d.text_background_popimage = PopimageObject, MUIA_Draggable, TRUE, End,
		Child, VGroup,
			       Child, HVSpace,
			       Child, MakeLabel("Background in\npressed state:"),
			       Child, HVSpace,
			       End,
		Child, d.text_selbackground_popimage = PopimageObject, MUIA_Draggable, TRUE, End,
		Child, MakeLabel("Font:"),
		Child, PopaslObject,
		    MUIA_Popasl_Type, ASL_FontRequest,
		    MUIA_Popstring_String, d.text_font_string = StringObject, StringFrame, End,
		    MUIA_Popstring_Button, PopButton(MUII_PopUp),
		    End,

		End,
	    End, /* VGroup */
	  Child, VGroup, /* other buttons */
	      Child, HGroup, /* ImageButtons */
	          GroupFrameT("Image Buttons"),
		  Child, VGroup,
		      Child, HVSpace,
		      Child, MakeLabel("Frame:"),
		      Child, HVSpace,
		      End,
	          Child, HGroup,
		      Child, PopimageObject,
			   MUIA_Draggable, TRUE, 
		           MUIA_FixWidth, 20,
		           End,
	              Child, HVSpace,
	 	      End,
	          End, /* ImageButtons */
	      Child, HGroup, /* Checkmarks */
	          GroupFrameT("Checkmarks"),
		  Child, VGroup,
		      Child, HVSpace,
		      Child, MakeLabel("Look:"),
		      Child, HVSpace,
		      End,
	          Child, HGroup,
	  	   Child, d.checkmark_look_popimage = PopimageObject,
		     MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Image,
		     MUIA_Draggable, TRUE, 
		     MUIA_FixWidth, 20,
		     MUIA_FixHeight, 20,
		     MUIA_Imagedisplay_FreeHoriz, FALSE,
		     MUIA_Imagedisplay_FreeVert, FALSE,
		     End,
	           Child, HVSpace,
	           End, /* HGroup */
	          End, /* Checkmarks */
	      Child, HGroup, /* Radio Buttons */
		  GroupFrameT("Radio Buttons"),
		  Child, HVSpace,
		  Child, VGroup,
		      Child, d.radio_look_popimage = PopimageObject,
		         MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Image,
			 MUIA_Draggable, TRUE, 
		         MUIA_FixWidth, 20,
		         MUIA_FixHeight, 20,
			 MUIA_Imagedisplay_FreeHoriz, FALSE,
			 MUIA_Imagedisplay_FreeVert, FALSE,
		         End,
		      Child, MUI_MakeObject(MUIO_Label, "Look", MUIO_Label_Centered),
               	      End,
		  Child, VGroup,
		      Child, ColGroup(2),
			  MUIA_Group_HorizSpacing, 2,
		          Child, MakeLabel("H"),
		          Child, d.spacing_horiz_slider = MakeSpacingSlider(),
		          Child, MakeLabel("V"),
		          Child, d.spacing_vert_slider = MakeSpacingSlider(),
		          End,
		      Child, MUI_MakeObject(MUIO_Label, "Spacing", MUIO_Label_Centered),
		      End,
	          Child, HVSpace,
	          End, /* Radio Buttons */
	      End, /* other buttons */
	    End, /* obj */
    	TAG_MORE, msg->ops_AttrList);

    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    *data = d;

    return (IPTR)obj;
}

static IPTR ButtonsP_ConfigToGadgets(struct IClass *cl, Object *obj, struct MUIP_Settingsgroup_ConfigToGadgets *msg)
{
    struct MUI_ButtonsPData *data = INST_DATA(cl, obj);
    char *spec;

/* Font */
    setstring(data->text_font_string, FindConfig(MUICFG_Font_Button));

/* Backgrounds */
    spec = FindConfig(MUICFG_Background_Button);
    set(data->text_background_popimage, MUIA_Imagedisplay_Spec,
	spec ? spec : MUII_ButtonBack);

    spec = FindConfig(MUICFG_Background_Selected);
    set(data->text_selbackground_popimage, MUIA_Imagedisplay_Spec,
	spec ? spec : MUII_SelectedBack);

/* Looks */
    spec = FindConfig(MUICFG_Image_RadioButton);
    set(data->radio_look_popimage, MUIA_Imagedisplay_Spec,
	spec ? spec : MUII_RadioButton);

    spec = FindConfig(MUICFG_Image_CheckMark);
    set(data->checkmark_look_popimage, MUIA_Imagedisplay_Spec,
	spec ? spec : MUII_CheckMark);

/* Spacing */
    setslider(data->spacing_horiz_slider,
	      DoMethod(msg->configdata, MUIM_Configdata_GetULong,
		       MUICFG_Radio_HSpacing));
    setslider(data->spacing_vert_slider,
	      DoMethod(msg->configdata, MUIM_Configdata_GetULong,
		       MUICFG_Radio_VSpacing));
    return 1;
}

static IPTR ButtonsP_GadgetsToConfig(struct IClass *cl, Object *obj, struct MUIP_Settingsgroup_GadgetsToConfig *msg)
{
    struct MUI_ButtonsPData *data = INST_DATA(cl, obj);
    char *buf;
    char *str;

/* Font */
    str = getstring(data->text_font_string);
    DoMethod(msg->configdata, MUIM_Configdata_SetFont, MUICFG_Font_Button, (IPTR)str);

/* Backgrounds */
    str = (char*)xget(data->text_background_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_Button,
	     (IPTR)str);

    str = (char*)xget(data->text_selbackground_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_Selected,
	     (IPTR)str);

/* Looks */
    str = (char*)xget(data->radio_look_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Image_RadioButton,
	     (IPTR)str);

    str = (char*)xget(data->checkmark_look_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Image_CheckMark,
	     (IPTR)str);

/* Spacing */
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Radio_HSpacing,
	     xget(data->spacing_horiz_slider, MUIA_Numeric_Value));
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Radio_VSpacing,
	     xget(data->spacing_vert_slider, MUIA_Numeric_Value));

    return TRUE;
}

#ifndef __AROS__
__asm IPTR ButtonsP_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,ButtonsP_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
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
    MUIC_Settingsgroup, 
    sizeof(struct MUI_ButtonsPData),
    (void*)ButtonsP_Dispatcher 
};
