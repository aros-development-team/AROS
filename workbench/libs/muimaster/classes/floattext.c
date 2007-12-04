/*
    Copyright © 2002-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 1
#include <aros/debug.h>

#include <exec/memory.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#include <string.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "floattext_private.h"

extern struct Library *MUIMasterBase;

IPTR Floattext__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);

    if (obj)
    {
    	struct Floattext_DATA *data = INST_DATA(cl, obj);
	struct TagItem *tag;

	if ((tag = FindTagItem(MUIA_Floattext_Text, msg->ops_AttrList)))
	    data->text = (char *)tag->ti_Data;
	else
	    data->text = NULL;

	if (data->text)
	    DoMethod(obj, MUIM_List_InsertSingle, data->text, MUIV_List_Insert_Top);
    }
    
    return (IPTR)obj;
}

IPTR Floattext__OM_DISPOSE(struct IClass *cl, Object *obj, struct opSet *msg)
{
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR Floattext__OM_GET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR Floattext__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Floattext_DATA *data = INST_DATA(cl, obj);

    const struct TagItem *tags;
    const struct TagItem *tag;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	case MUIA_Floattext_Text:
	    DoMethod(obj, MUIM_List_Clear, NULL);
	    //if (data->text) free
	    data->text = (char *)tag->ti_Data;
	    if (data->text)
		DoMethod(obj, MUIM_List_InsertSingle, data->text, MUIV_List_Insert_Top);
	    break;
	}
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

#if ZUNE_BUILTIN_FLOATTEXT
BOOPSI_DISPATCHER(IPTR, Floattext_Dispatcher, cl, obj, msg)
{
    struct opSet* omsg = (struct opSet *)msg;

    switch (msg->MethodID)
    {
	case OM_NEW:	 return Floattext__OM_NEW(cl, obj, omsg);
	case OM_DISPOSE: return Floattext__OM_DISPOSE(cl, obj, omsg);
	case OM_GET:	 return Floattext__OM_GET(cl, obj, omsg);
	case OM_SET:	 return Floattext__OM_SET(cl, obj, omsg);
	
        default:	 return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Floattext_desc =
{ 
    MUIC_Floattext, 
    MUIC_List, 
    sizeof(struct Floattext_DATA), 
    (void*)Floattext_Dispatcher 
};
#endif /* ZUNE_BUILTIN_FLOATTEXT */
