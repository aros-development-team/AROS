/*
    Copyright © 2003, The AROS Development Team. 
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

struct MUI_StringsPData
{
    Object *string_popframe;
    Object *popup_popimage;
    Object *popfile_popimage;
    Object *popdrawer_popimage;
    Object *inactive_bg_poppen;
    Object *inactive_text_poppen;
    Object *active_bg_poppen;
    Object *active_text_poppen;
};

Object *MakePopupPopimage(CONST_STRPTR title)
{
    return NewObject
	(
	    CL_ImageClipboard->mcc_Class, NULL,
	    MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Image,
	    MUIA_Draggable, TRUE, 
	    MUIA_CycleChain, 1,
	    MUIA_FixWidth, 28,
	    MUIA_MaxHeight, 28,
	    MUIA_Imagedisplay_FreeHoriz, FALSE,
	    MUIA_Imagedisplay_FreeVert, FALSE,
	    MUIA_Window_Title, (IPTR)title,
	    TAG_DONE
	    );
}

static IPTR StringsP_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_StringsPData *data;
    struct MUI_StringsPData d;
    
    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
	Child, ColGroup(2),
	Child, HGroup,
	GroupFrameT("String Frame"),
	Child, d.string_popframe = MakePopframe(),
	End, // String Frame
	Child, HGroup,
	GroupFrameT("Special Popup Buttons"),
        MUIA_Group_SameWidth, TRUE,
	Child, (IPTR) VGroup,
                MUIA_Group_VertSpacing, 1,
                Child, (IPTR) d.popup_popimage = MakePopupPopimage("Popup"),
                Child, (IPTR) CLabel("Default"),
            End,
            Child, (IPTR) VGroup,
                MUIA_Group_VertSpacing, 1,
                Child, (IPTR) d.popfile_popimage = MakePopupPopimage("Popup file"),
                Child, (IPTR) CLabel("File"),
            End,
            Child, (IPTR) VGroup,
                MUIA_Group_VertSpacing, 1,
                Child, (IPTR) d.popdrawer_popimage = MakePopupPopimage("Popup drawer"),
                Child, (IPTR) CLabel("Drawer"),
            End,
	End, // Special Popup Buttons
	Child, HGroup,
	GroupFrameT("Inactive String Colors"),
        MUIA_Group_SameWidth, TRUE,
	Child, (IPTR) VGroup,
                MUIA_Group_VertSpacing, 1,
                Child, (IPTR) d.inactive_bg_poppen = MakePoppen(),
                Child, (IPTR) CLabel("Background"),
            End,
            Child, (IPTR) VGroup,
                MUIA_Group_VertSpacing, 1,
                Child, (IPTR) d.inactive_text_poppen = MakePoppen(),
                Child, (IPTR) CLabel("Text"),
            End,
	End, // Inactive String Colors
	Child, HGroup,
	GroupFrameT("Active String Colors"),
        MUIA_Group_SameWidth, TRUE,
	Child, (IPTR) VGroup,
                MUIA_Group_VertSpacing, 1,
                Child, (IPTR) d.active_bg_poppen = MakePoppen(),
                Child, (IPTR) CLabel("Background"),
            End,
            Child, (IPTR) VGroup,
                MUIA_Group_VertSpacing, 1,
                Child, (IPTR) d.active_text_poppen = MakePoppen(),
                Child, (IPTR) CLabel("Text"),
            End,
	End, // Inactive String Colors
	End, // ColGroup(2)
	Child, StringObject,
	StringFrame,
	MUIA_String_Contents, "Example String Gadget",
	MUIA_String_Format, MUIV_String_Format_Center,
	End,
    	TAG_MORE, (IPTR) msg->ops_AttrList);
	
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    *data = d;

    return (IPTR)obj;
}


/*
 * MUIM_Settingsgroup_ConfigToGadgets
 */
static IPTR StringsP_ConfigToGadgets(struct IClass *cl, Object *obj,
				    struct MUIP_Settingsgroup_ConfigToGadgets *msg)
{
    struct MUI_StringsPData *data = INST_DATA(cl, obj);
    STRPTR spec;

/* Frames */
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Frame_String);
    set(data->string_popframe, MUIA_Framedisplay_Spec, (IPTR)spec);

/* Looks */
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Image_PopUp);
    set(data->popup_popimage, MUIA_Imagedisplay_Spec, (IPTR)spec);
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Image_PopFile);
    set(data->popfile_popimage, MUIA_Imagedisplay_Spec, (IPTR)spec);
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Image_PopDrawer);
    set(data->popdrawer_popimage, MUIA_Imagedisplay_Spec, (IPTR)spec);

/* pens */
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_String_Background);
    set(data->inactive_bg_poppen, MUIA_Pendisplay_Spec, (IPTR)spec);
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_String_Text);
    set(data->inactive_text_poppen, MUIA_Pendisplay_Spec, (IPTR)spec);

    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_String_ActiveBackground);
    set(data->active_bg_poppen, MUIA_Pendisplay_Spec, (IPTR)spec);
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_String_ActiveText);
    set(data->active_text_poppen, MUIA_Pendisplay_Spec, (IPTR)spec);

    return 1;    
}


/*
 * MUIM_Settingsgroup_ConfigToGadgets
 */
static IPTR StringsP_GadgetsToConfig(struct IClass *cl, Object *obj,
				    struct MUIP_Settingsgroup_GadgetsToConfig *msg)
{
    struct MUI_StringsPData *data = INST_DATA(cl, obj);
    STRPTR str;

/* Frames */
    str = (STRPTR)XGET(data->string_popframe, MUIA_Framedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetFramespec, MUICFG_Frame_String,
	     (IPTR)str);

/* Looks */
    str = (STRPTR)XGET(data->popup_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Image_PopUp,
	     (IPTR)str);
    str = (STRPTR)XGET(data->popfile_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Image_PopFile,
	     (IPTR)str);
    str = (STRPTR)XGET(data->popdrawer_popimage, MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Image_PopDrawer,
	     (IPTR)str);

/* pens */
    str = (STRPTR)XGET(data->inactive_bg_poppen, MUIA_Pendisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetPenspec,
	     MUICFG_String_Background, (IPTR)str);
    str = (STRPTR)XGET(data->inactive_text_poppen, MUIA_Pendisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetPenspec,
	     MUICFG_String_Text, (IPTR)str);

    str = (STRPTR)XGET(data->active_bg_poppen, MUIA_Pendisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetPenspec,
	     MUICFG_String_ActiveBackground, (IPTR)str);
    str = (STRPTR)XGET(data->active_text_poppen, MUIA_Pendisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetPenspec,
	     MUICFG_String_ActiveText, (IPTR)str);

    return TRUE;
}


BOOPSI_DISPATCHER(IPTR, StringsP_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return StringsP_New(cl, obj, (struct opSet *)msg);
	case MUIM_Settingsgroup_ConfigToGadgets: return StringsP_ConfigToGadgets(cl,obj,(APTR)msg);break;
	case MUIM_Settingsgroup_GadgetsToConfig: return StringsP_GadgetsToConfig(cl,obj,(APTR)msg);break;
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUIP_Strings_desc = { 
    "Strings",
    MUIC_Group,
    sizeof(struct MUI_StringsPData),
    (void*)StringsP_Dispatcher 
};
