/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <exec/types.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

extern struct Library *MUIMasterBase;

#include "muimaster_intern.h"
#include "mui.h"
#include "support.h"

//#define MYDEBUG 1
#include "debug.h"

struct MUI_StringData
{
    int dummy;
};

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

    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
	MUIA_Text_Editable, TRUE,
	MUIA_Text_SetMin, TRUE,
	MUIA_Text_SetMax, FALSE,
	TAG_MORE, (IPTR) msg->ops_AttrList);

    if (!obj)
    {
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
    //struct MUI_StringData *data = INST_DATA(cl, obj);
    return DoSuperMethodA(cl,obj,(Msg)msg);
}



BOOPSI_DISPATCHER(IPTR, String_Dispatcher, cl, obj, msg)
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
    (void*)String_Dispatcher 
};
