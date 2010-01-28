/*
    Copyright © 2002-2010, The AROS Development Team. All rights reserved.
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

static void SetText(Object *obj, STRPTR text)
{
    DoMethod(obj, MUIM_List_Clear);
    
    // TODO: split the text in single lines, handling of attributes
    // like tabwith etc.
    if (text)
    {
	DoMethod(obj, MUIM_List_InsertSingle, text, MUIV_List_Insert_Top);
    }
}

IPTR Floattext__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Floattext_DATA *data;
    struct TagItem        *tag;
    const struct TagItem  *tags;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);

    if (!obj)
    {
	return 0;
    }

    data = INST_DATA(cl, obj);
    data->tabsize = 8;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Floattext_Justify:
		    data->justify = tag->ti_Data;
		    break;


	    case    MUIA_Floattext_SkipChars:
		    data->skipchars = (STRPTR)tag->ti_Data;
		    break;

	    case    MUIA_Floattext_TabSize:
		    data->tabsize = tag->ti_Data;
		    break;

	    case    MUIA_Floattext_Text:
		    data->text = StrDup((STRPTR)tag->ti_Data);
		    break;
	}
    }
    
    SetText(obj, data->text);
    
    return (IPTR)obj;
}

IPTR Floattext__OM_DISPOSE(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Floattext_DATA *data = INST_DATA(cl, obj);

    FreeVec(data->text);

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR Floattext__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Floattext_DATA *data = INST_DATA(cl, obj);
#define STORE *(msg->opg_Storage)

    switch(msg->opg_AttrID)
    {
	case	MUIA_Floattext_Justify:
		STORE = data->justify;
		return 1;

	case	MUIA_Floattext_Text:
		STORE = (IPTR)data->text;
		return 1;

    }

#undef STORE

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR Floattext__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Floattext_DATA *data = INST_DATA(cl, obj);
    struct TagItem        *tag;
    const struct TagItem  *tags;
    BOOL                   changed = FALSE;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Floattext_Justify:
		    data->justify = tag->ti_Data;
		    changed = TRUE;
		    break;

	    case    MUIA_Floattext_SkipChars:
		    data->skipchars = (STRPTR)tag->ti_Data;
		    changed = TRUE;
		    break;

	    case    MUIA_Floattext_TabSize:
		    data->tabsize = tag->ti_Data;
		    changed = TRUE;
		    break;

	    case    MUIA_Floattext_Text:
		    FreeVec(data->text);
		    data->text = StrDup((STRPTR)tag->ti_Data);
		    changed = TRUE;
		    break;

	}
    }

    if (changed) // To avoid recursion
    {
	SetText(obj, data->text);
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

#if ZUNE_BUILTIN_FLOATTEXT
BOOPSI_DISPATCHER(IPTR, Floattext_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW:	 return Floattext__OM_NEW(cl, obj, msg);
	case OM_DISPOSE: return Floattext__OM_DISPOSE(cl, obj, msg);
	case OM_GET:	 return Floattext__OM_GET(cl, obj, msg);
	case OM_SET:	 return Floattext__OM_SET(cl, obj, msg);
	
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
