/*
   Copyright © 1995-2003, The AROS Development Team. All rights reserved.
   $Id$ 
*/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <libraries/mui.h>

#include "support.h"
#include "desktop_intern.h"

#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "operationclass.h"

#include "desktop_intern_protos.h"

IPTR operationNew(Class * cl, Object * obj, struct opSet * msg)
{
    IPTR            retval = 0;
    struct OperationClassData *data;
    //struct TagItem *tag;

    retval = DoSuperMethodA(cl, obj, (Msg) msg);
    if (retval)
    {
        obj = (Object *) retval;
        data = INST_DATA(cl, obj);
    }

    return retval;
}

IPTR operationSet(Class * cl, Object * obj, struct opSet * msg)
{
    struct OperationClassData *data;
    IPTR            retval = 1;
    struct TagItem *tag,
                   *tstate = msg->ops_AttrList;

    data = (struct OperationClassData *) INST_DATA(cl, obj);

    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
            default:
                break;
        }
    }

    retval = DoSuperMethodA(cl, obj, (Msg) msg);

    return retval;
}

IPTR operationGet(Class * cl, Object * obj, struct opGet * msg)
{
    IPTR            retval = 1;
    struct OperationClassData *data;

    data = (struct OperationClassData *) INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
        default:
            retval = DoSuperMethodA(cl, obj, (Msg) msg);
            break;
    }

    return retval;
}

IPTR operationDispose(Class * cl, Object * obj, Msg msg)
{
    IPTR            retval;

    retval = DoSuperMethodA(cl, obj, msg);

    return retval;
}

BOOPSI_DISPATCHER(IPTR, operationDispatcher, cl, obj, msg)
{
    ULONG           retval = 0;

    switch (msg->MethodID)
    {
        case OM_NEW:
            retval = operationNew(cl, obj, (struct opSet *) msg);
            break;
        case OM_SET:
            retval = operationSet(cl, obj, (struct opSet *) msg);
            break;
        case OM_GET:
            retval = operationGet(cl, obj, (struct opGet *) msg);
            break;
        case OM_DISPOSE:
            retval = operationDispose(cl, obj, msg);
            break;
        default:
            retval = DoSuperMethodA(cl, obj, msg);
            break;
    }

    return retval;
}
BOOPSI_DISPATCHER_END
