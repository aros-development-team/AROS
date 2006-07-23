/*
    Copyright © 2003-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <clib/alib_protos.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

/*  #define MYDEBUG 1 */
#include "debug.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct MUI_MccprefsData
{
    LONG    dummy;
};

/**************************************************************************
 OM_NEW
**************************************************************************/
IPTR Mccprefs__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_MccprefsData   *data;
    struct TagItem  	       *tag, *tags;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);

    if (!obj)
	return FALSE;

    data = INST_DATA(cl, obj);

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem((const struct TagItem**)&tags)); )
    {
	switch (tag->ti_Tag)
	{
   	}
    }

    return (IPTR)obj;
}


BOOPSI_DISPATCHER(IPTR, Mccprefs_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Mccprefs__OM_NEW(cl, obj, (struct opSet *)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Mccprefs_desc = { 
    MUIC_Mccprefs, 
    MUIC_Group,
    sizeof(struct MUI_MccprefsData), 
    (void*)Mccprefs_Dispatcher 
};

