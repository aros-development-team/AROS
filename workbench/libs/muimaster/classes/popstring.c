#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

/********** Popstring ***********/

struct MUI_PopstringData
{
    int dummy;
};

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Popstring_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_PopstringData   *data;
    struct TagItem  	    *tag, *tags;
    
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
    	}
    }
    
    return (IPTR)obj;
}

#ifndef _AROS
__asm IPTR Popstring_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,Popstring_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW:
	    return Popstring_New(cl, obj, (struct opSet *)msg);
	    
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Popstring_desc = { 
    MUIC_Popstring, 
    MUIC_Group, 
    sizeof(struct MUI_PopstringData), 
    (void*)Popstring_Dispatcher 
};


/********** Popasl ***********/

struct MUI_PopaslData
{
    int dummy;
};

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Popasl_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_PopaslData   *data;
    struct TagItem  	    *tag, *tags;
    
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
    	}
    }
    
    return (IPTR)obj;
}

#ifndef _AROS
__asm IPTR Popasl_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,Popasl_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW:
	    return Popasl_New(cl, obj, (struct opSet *)msg);
	    
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Popasl_desc = { 
    MUIC_Popasl, 
    MUIC_Group, 
    sizeof(struct MUI_PopaslData), 
    (void*)Popasl_Dispatcher 
};
