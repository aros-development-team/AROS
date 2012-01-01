/*
    Copyright © 2002-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 0
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

// like strlen(), but \n ends string, too.
static long MyStrLen(const char *ptr)
{
    const char * start = ptr;

    while (*ptr && (*ptr != '\n')) ptr ++;

    return (((long)ptr) - ((long)start));
}

static void SetText(Object *obj, struct Floattext_DATA *data)
{
    DoMethod(obj, MUIM_List_Clear);
    
    if (data->text)
    {
	STRPTR pos = data->text;
	for (;;)
	{
	    LONG len = MyStrLen(pos); 
	    DoMethod(obj, MUIM_List_InsertSingle, pos, MUIV_List_Insert_Bottom);
	    pos += len;
	    if (*pos == '\0')
		break;
	    pos++;
	}
    }
}

AROS_UFH3S(APTR, construct_func,
AROS_UFHA(struct Hook *, hook, A0),
AROS_UFHA(APTR, pool, A2),
AROS_UFHA(STRPTR, entry, A1))
{
    AROS_USERFUNC_INIT

    // TODO: MUIA_Floattext_Justify

    struct Floattext_DATA *data = hook->h_Data;

    STRPTR new;
    ULONG tabs = 0;
    ULONG i;
    ULONG slen = MyStrLen(entry);
    ULONG size = slen + 1;

    // Count tabulators
    for (i = 0; i < slen; i++)
    {
	if (entry[i] == '\t')
	    tabs++;
    }
    size += tabs * data->tabsize; // Worst case

    if ((new = AllocVecPooled(pool, size)))
    {
	ULONG oldpos = 0;
	ULONG newpos = 0;
	for (; oldpos < slen; oldpos++)
	{
	    if (data->skipchars)
	    {
		if (strchr(data->skipchars, entry[oldpos]))
		{
		    continue;
		}
	    }

	    if (entry[oldpos] == '\t')
	    {
		LONG spaces = data->tabsize - (newpos % data->tabsize);
		for ( ; spaces > 0 ; spaces --)
		{
		    new[newpos++] = ' ';
		}
	    }
	    else
	    {
		new[newpos++] = entry[oldpos];
	    }
	}
	new[newpos] = '\0';
    }
    return new;

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, destruct_func,
AROS_UFHA(struct Hook *, hook, A0),
AROS_UFHA(APTR, pool, A2),
AROS_UFHA(STRPTR, entry, A1))
{
    AROS_USERFUNC_INIT

    FreeVecPooled(pool, entry);

    AROS_USERFUNC_EXIT
}

IPTR Floattext__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Floattext_DATA *data;
    struct TagItem        *tag;
    struct TagItem        *tags;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);

    if (!obj)
    {
	return 0;
    }

    data = INST_DATA(cl, obj);

    data->construct_hook.h_Entry = (HOOKFUNC)construct_func;
    data->construct_hook.h_Data  = data;
    data->destruct_hook.h_Entry  = (HOOKFUNC)destruct_func;

    SetAttrs(obj, MUIA_List_ConstructHook, (IPTR)&data->construct_hook,
		  MUIA_List_DestructHook,  (IPTR)&data->destruct_hook,
		  TAG_DONE);

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

    if (data->tabsize == 0)
	data->tabsize = 8;
    else if (data->tabsize > 20)
	data->tabsize = 20;

    SetText(obj, data);
    
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
    struct TagItem  *tags;
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
	if (data->tabsize == 0)
	    data->tabsize = 8;
	else if (data->tabsize > 20)
	    data->tabsize = 20;

	SetText(obj, data);
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
