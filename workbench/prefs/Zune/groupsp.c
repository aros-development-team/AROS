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

/*  #define DEBUG 1 */
#include <aros/debug.h>

extern struct Library *MUIMasterBase;

struct MUI_GroupsPData
{
    Object *background_register_popimage;
    Object *background_framed_popimage;
    Object *background_page_popimage;
};

static ULONG DoSuperNew(struct IClass *cl, Object * obj, ULONG tag1,...)
{
    return (DoSuperMethod(cl, obj, OM_NEW, &tag1, NULL));
}

#define FindConfig(id) (void*)DoMethod(msg->configdata,MUIM_Dataspace_Find,id)
#define AddConfig(data,len,id) DoMethod(msg->configdata,MUIM_Dataspace_Add,data,len,id)
#define AddConfigStr(str,id) if(str && *str){DoMethod(msg->configdata,MUIM_Dataspace_Add,str,strlen(str)+1,id);}
#define AddConfigImgStr(str,id) if(str && *str && *str != '6'){DoMethod(msg->configdata,MUIM_Dataspace_Add,str,strlen(str)+1,id);}

static IPTR GroupsP_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_GroupsPData *data;
    struct MUI_GroupsPData d;
    
    obj = (Object *)DoSuperNew(cl, obj,
			       Child, ColGroup(2),
			       Child, VGroup,
			       GroupFrameT("Title"),
			       Child, HVSpace,
			       End,
	    Child, ColGroup(3),
		GroupFrameT("Background"),
		Child, MakeLabel("Framed"),
		Child, MakeLabel("Page"),
		Child, MakeLabel("Register"),
		Child, d.background_framed_popimage = PopimageObject, End,
		Child, d.background_page_popimage = PopimageObject, End,
		Child, d.background_register_popimage = PopimageObject, End,
		End,
	    End,
    	TAG_MORE, msg->ops_AttrList);

    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    *data = d;

    return (IPTR)obj;
}

static IPTR GroupsP_ConfigToGadgets(struct IClass *cl, Object *obj, struct MUIP_Settingsgroup_ConfigToGadgets *msg)
{
    struct MUI_GroupsPData *data = INST_DATA(cl, obj);
    char *spec = FindConfig(MUICFG_Background_Framed);
    D(bug("GroupsP_ConfigToGadgets : spec = %p\n", spec));
    D(bug("GroupsP_ConfigToGadgets : spec = %s\n", spec));
    set(data->background_framed_popimage,MUIA_Imagedisplay_Spec,
	spec ? spec : MUII_GroupBack);
    spec = FindConfig(MUICFG_Background_Register);
    set(data->background_register_popimage,MUIA_Imagedisplay_Spec,
	spec ? spec : MUII_RegisterBack);
    spec = FindConfig(MUICFG_Background_Page);
    set(data->background_page_popimage,MUIA_Imagedisplay_Spec,
	spec ? spec  : MUII_PageBack);
    return 1;    
}

static IPTR GroupsP_GadgetsToConfig(struct IClass *cl, Object *obj, struct MUIP_Settingsgroup_GadgetsToConfig *msg)
{
    struct MUI_GroupsPData *data = INST_DATA(cl, obj);
    char *buf;
    char *str;

    str = (char*)xget(data->background_framed_popimage,MUIA_Imagedisplay_Spec);
    AddConfigImgStr(str,MUICFG_Background_Framed);

    str = (char*)xget(data->background_register_popimage,MUIA_Imagedisplay_Spec);
    AddConfigImgStr(str,MUICFG_Background_Register);

    str = (char*)xget(data->background_page_popimage,MUIA_Imagedisplay_Spec);
    AddConfigImgStr(str,MUICFG_Background_Page);
    return TRUE;
}

#ifndef __AROS__
__asm IPTR GroupsP_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,GroupsP_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return GroupsP_New(cl, obj, (struct opSet *)msg);
	case MUIM_Settingsgroup_ConfigToGadgets: return GroupsP_ConfigToGadgets(cl,obj,(APTR)msg);break;
	case MUIM_Settingsgroup_GadgetsToConfig: return GroupsP_GadgetsToConfig(cl,obj,(APTR)msg);break;
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUIP_Groups_desc = { 
    "Groups",
    MUIC_Settingsgroup, 
    sizeof(struct MUI_GroupsPData),
    (void*)GroupsP_Dispatcher 
};
