#include <exec/types.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#ifdef _AROS
#include <proto/muimaster.h>
#endif

extern struct Library *MUIMasterBase;

#include "muimaster_intern.h"
#include "mui.h"
#include "support.h"

#define MYDEBUG 1
#include "debug.h"

/**************************************************************************
 OM_NEW
**************************************************************************/
static ULONG String_New(struct IClass *cl, Object * obj, struct opSet *msg)
{
    struct MUI_SliderData *data;
    struct TagItem *tags, *tag;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
	switch (tag->ti_Tag)
	{
	}
    }

    obj = (Object *)DoSuperNew(cl, obj,
	MUIA_Text_Editable, TRUE,
	TAG_MORE, msg->ops_AttrList);

    if (!obj)
    {
    	CoerceMethod(cl,obj,OM_DISPOSE);
	return NULL;
    }

    data = INST_DATA(cl, obj);
    return (ULONG)obj;
}

/**************************************************************************
 OM_SET
**************************************************************************/
static ULONG String_Set(struct IClass *cl, Object * obj, struct opSet *msg)
{
    struct MUI_StringData *data = INST_DATA(cl, obj);
    return DoSuperMethodA(cl,obj,(Msg)msg);
}


#ifndef _AROS
__asm IPTR String_Dispatcher(register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR, String_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return String_New(cl, obj, (struct opSet *)msg);
	case OM_SET: return String_Set(cl, obj, (struct opSet *)msg);
    }
    return DoSuperMethodA(cl, obj, msg);
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_String_desc = { 
    MUIC_String, 
    MUIC_Text, 
    sizeof(struct MUI_StringData), 
    String_Dispatcher 
};
