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

#ifdef _AROS
#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#endif

#include "zunestuff.h"

extern struct Library *MUIMasterBase;

struct MUI_WindowPData
{
    int dummy;
};

static ULONG DoSuperNew(struct IClass *cl, Object * obj, ULONG tag1,...)
{
  return (DoSuperMethod(cl, obj, OM_NEW, &tag1, NULL));
}



static IPTR WindowP_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_WindowPData   *data;
    struct TagItem  	    *tag, *tags;
    Object *app = NULL;
    Object *ok_button;
    
    obj = (Object *)DoSuperNew(cl, obj,
	Child, VGroup,
	    Child, ColGroup(2),
		GroupFrameT("Fonts"),
		Child, MakeLabel("Normal"),
		Child, PopaslObject,
		    MUIA_Popasl_Type, ASL_FontRequest,
		    MUIA_Popstring_String, StringObject, StringFrame, End,
		    MUIA_Popstring_Button, PopButton(MUII_PopUp),
		    End,

		Child, MakeLabel("Small"),
		Child, PopaslObject,
		    MUIA_Popasl_Type, ASL_FontRequest,
		    MUIA_Popstring_String, StringObject, StringFrame, End,
		    MUIA_Popstring_Button, PopButton(MUII_PopUp),
		    End,

		Child, MakeLabel("Big"),
		Child, PopaslObject,
		    MUIA_Popasl_Type, ASL_FontRequest,
		    MUIA_Popstring_String, StringObject, StringFrame, End,
		    MUIA_Popstring_Button, PopButton(MUII_PopUp),
		    End,
		End,
	    End,
    	TAG_MORE, msg->ops_AttrList);

    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
    }

    return (IPTR)obj;
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
