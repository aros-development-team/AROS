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

#ifdef _AROS
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
};

static ULONG DoSuperNew(struct IClass *cl, Object * obj, ULONG tag1,...)
{
    return (DoSuperMethod(cl, obj, OM_NEW, &tag1, NULL));
}

#define FindConfig(id) (void*)DoMethod(msg->configdata,MUIM_Dataspace_Find,id)
#define AddConfig(data,len,id) DoMethod(msg->configdata,MUIM_Dataspace_Add,data,len,id)
#define AddConfigStr(str,id) if(str && *str){DoMethod(msg->configdata,MUIM_Dataspace_Add,str,strlen(str)+1,id);}

static IPTR ButtonsP_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ButtonsPData *data;
    struct MUI_ButtonsPData d;
    
    obj = (Object *)DoSuperNew(cl, obj,
	Child, VGroup,
	    Child, ColGroup(2),
		GroupFrameT("Text Buttons"),
		Child, MakeLabel("Background:"),
		Child, d.text_background_popimage = PopimageObject, End,
		Child, MakeLabel("Background in\npressed state:"),
		Child, d.text_selbackground_popimage = PopimageObject, End,
		Child, MakeLabel("Font:"),
		Child, PopaslObject,
		    MUIA_Popasl_Type, ASL_FontRequest,
		    MUIA_Popstring_String, d.text_font_string = StringObject, StringFrame, End,
		    MUIA_Popstring_Button, PopButton(MUII_PopUp),
		    End,

		End,
	    End,
    	TAG_MORE, msg->ops_AttrList);

    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    *data = d;

    return (IPTR)obj;
}

static IPTR ButtonsP_ConfigToGadgets(struct IClass *cl, Object *obj, struct MUIP_Settingsgroup_ConfigToGadgets *msg)
{
    struct MUI_ButtonsPData *data = INST_DATA(cl, obj);
//    setstring(data->text_font_string,FindConfig(MUICFG_Buttons_Font));
    set(data->text_background_popimage,MUIA_Imagedisplay_Spec,FindConfig(MUICFG_Buttons_Background));
    set(data->text_selbackground_popimage,MUIA_Imagedisplay_Spec,FindConfig(MUICFG_Buttons_SelBackground));
    return 1;    
}

static IPTR ButtonsP_GadgetsToConfig(struct IClass *cl, Object *obj, struct MUIP_Settingsgroup_GadgetsToConfig *msg)
{
    struct MUI_ButtonsPData *data = INST_DATA(cl, obj);
    char *buf;
    char *str;/* = getstring(data->font_normal_string);
    AddConfigStr(str,MUICFG_Font_Normal);

    str = getstring(data->font_tiny_string);
    AddConfigStr(str,MUICFG_Font_Tiny);

    str = getstring(data->font_big_string);
    AddConfigStr(str,MUICFG_Font_Big);*/

    str = (char*)xget(data->text_background_popimage,MUIA_Imagedisplay_Spec);
    AddConfigStr(str,MUICFG_Buttons_Background);

    str = (char*)xget(data->text_selbackground_popimage,MUIA_Imagedisplay_Spec);
    AddConfigStr(str,MUICFG_Buttons_SelBackground);

    return TRUE;
}

#ifndef _AROS
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
