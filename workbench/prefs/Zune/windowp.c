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

extern struct Library *MUIMasterBase;

struct MUI_WindowPData
{
    Object *font_normal_string;
    Object *font_tiny_string;
    Object *font_big_string;
    Object *background_window_string;
};

static ULONG DoSuperNew(struct IClass *cl, Object * obj, ULONG tag1,...)
{
    return (DoSuperMethod(cl, obj, OM_NEW, &tag1, NULL));
}

#define FindConfig(id) (void*)DoMethod(msg->configdata,MUIM_Dataspace_Find,id)
#define AddConfig(data,len,id) DoMethod(msg->configdata,MUIM_Dataspace_Add,data,len,id)
#define AddConfigStr(str,id) if(str && *str){DoMethod(msg->configdata,MUIM_Dataspace_Add,str,strlen(str)+1,id);}

static IPTR WindowP_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_WindowPData *data;
    struct MUI_WindowPData d;
    
    obj = (Object *)DoSuperNew(cl, obj,
	Child, VGroup,
	    Child, ColGroup(2),
		GroupFrameT("Fonts"),
		Child, MakeLabel("Normal"),
		Child, PopaslObject,
		    MUIA_Popasl_Type, ASL_FontRequest,
		    MUIA_Popstring_String, d.font_normal_string = StringObject, StringFrame, End,
		    MUIA_Popstring_Button, PopButton(MUII_PopUp),
		    End,

		Child, MakeLabel("Tiny"),
		Child, PopaslObject,
		    MUIA_Popasl_Type, ASL_FontRequest,
		    MUIA_Popstring_String, d.font_tiny_string = StringObject, StringFrame, End,
		    MUIA_Popstring_Button, PopButton(MUII_PopUp),
		    End,

		Child, MakeLabel("Big"),
		Child, PopaslObject,
		    MUIA_Popasl_Type, ASL_FontRequest,
		    MUIA_Popstring_String, d.font_big_string = StringObject, StringFrame, End,
		    MUIA_Popstring_Button, PopButton(MUII_PopUp),
		    End,
		End,

	    Child, ColGroup(2),
		GroupFrameT("Background"),
		Child, MakeLabel("Window"),
		Child, PopaslObject,
		    MUIA_Popstring_String, d.background_window_string = StringObject, StringFrame, End,
		    MUIA_Popstring_Button, PopButton(MUII_PopFile),
		    End,
		End,
	    End,
    	TAG_MORE, msg->ops_AttrList);

    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    *data = d;

    return (IPTR)obj;
}

static IPTR WindowP_ConfigToGadgets(struct IClass *cl, Object *obj, struct MUIP_Settingsgroup_ConfigToGadgets *msg)
{
    struct MUI_WindowPData *data = INST_DATA(cl, obj);
    setstring(data->font_normal_string,FindConfig(MUICFG_Font_Normal));
    setstring(data->font_tiny_string,FindConfig(MUICFG_Font_Tiny));
    setstring(data->font_big_string,FindConfig(MUICFG_Font_Big));
    setstring(data->background_window_string,((char*)FindConfig(MUICFG_Background_Window))+2);
    return 1;    
}

static IPTR WindowP_GadgetsToConfig(struct IClass *cl, Object *obj, struct MUIP_Settingsgroup_GadgetsToConfig *msg)
{
    struct MUI_WindowPData *data = INST_DATA(cl, obj);
    char *buf;
    char *str = getstring(data->font_normal_string);
    AddConfigStr(str,MUICFG_Font_Normal);

    str = getstring(data->font_tiny_string);
    AddConfigStr(str,MUICFG_Font_Tiny);

    str = getstring(data->font_big_string);
    AddConfigStr(str,MUICFG_Font_Big);

    if ((str = getstring(data->background_window_string)))
    {
    	if (*str)
    	{
	    if ((buf = AllocVec(strlen(str)+10,0)))
	    {
		strcpy(buf,"5:");
		strcat(buf,str);
	        AddConfigStr(buf,MUICFG_Background_Window);
	        FreeVec(buf);
	    }
	}
    }
    return TRUE;
}

#ifndef _AROS
__asm IPTR WindowP_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,WindowP_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return WindowP_New(cl, obj, (struct opSet *)msg);
	case MUIM_Settingsgroup_ConfigToGadgets: return WindowP_ConfigToGadgets(cl,obj,(APTR)msg);break;
	case MUIM_Settingsgroup_GadgetsToConfig: return WindowP_GadgetsToConfig(cl,obj,(APTR)msg);break;
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUIP_Windows_desc = { 
    "Windows",
    MUIC_Settingsgroup, 
    sizeof(struct MUI_WindowPData),
    (void*)WindowP_Dispatcher 
};
