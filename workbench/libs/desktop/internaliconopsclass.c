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
#include <libraries/desktop.h>

#include "support.h"
#include "desktop_intern.h"

#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "internaliconopsclass.h"
#include "iconclass.h"

#include "desktop_intern_protos.h"

IPTR internalIconOpsNew(Class * cl, Object * obj, struct opSet * msg)
{
    IPTR            retval = 0;
    struct InternalIconOpsClassData *data;
    //struct TagItem *tag;

    retval = DoSuperMethodA(cl, obj, (Msg) msg);
    if (retval)
    {
        obj = (Object *) retval;
        data = INST_DATA(cl, obj);
    }

    return retval;
}

IPTR internalIconOpsSet(Class * cl, Object * obj, struct opSet * msg)
{
    struct InternalIconOpsClassData *data;
    IPTR            retval = 1;
    struct TagItem *tag,
                   *tstate = msg->ops_AttrList;

    data = (struct InternalIconOpsClassData *) INST_DATA(cl, obj);

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

IPTR internalIconOpsGet(Class * cl, Object * obj, struct opGet * msg)
{
    IPTR            retval = 1;
    struct InternalIconOpsClassData *data;

    data = (struct InternalIconOpsClassData *) INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
        default:
            retval = DoSuperMethodA(cl, obj, (Msg) msg);
            break;
    }

    return retval;
}

IPTR internalIconOpsDispose(Class * cl, Object * obj, Msg msg)
{
    IPTR            retval;

    retval = DoSuperMethodA(cl, obj, msg);

    return retval;
}

IPTR internalIconOpsExecute(Class * cl, Object * obj, struct opExecute * msg)
{
    IPTR            retval = 0;
    struct InternalIconOpsClassData *data;

    data = (struct InternalIconOpsClassData *) INST_DATA(cl, obj);

    switch (msg->operationCode)
    {
        // open
        case (DOC_ICONOP | 1):
            SetAttrs(msg->target, IA_Executed, TRUE, TAG_END);
            break;
    }

    return retval;
}

BOOPSI_DISPATCHER(IPTR, internalIconOpsDispatcher, cl, obj, msg)
{
    ULONG           retval = 0;

    switch (msg->MethodID)
    {
        case OM_NEW:
            retval = internalIconOpsNew(cl, obj, (struct opSet *) msg);
            break;
        case OM_SET:
            retval = internalIconOpsSet(cl, obj, (struct opSet *) msg);
            break;
        case OM_GET:
            retval = internalIconOpsGet(cl, obj, (struct opGet *) msg);
            break;
        case OM_DISPOSE:
            retval = internalIconOpsDispose(cl, obj, msg);
            break;
        case OPM_Execute:
            retval =
                internalIconOpsExecute(cl, obj, (struct opExecute *) msg);
            break;
        default:
            retval = DoSuperMethodA(cl, obj, msg);
            break;
    }

    return retval;
}
BOOPSI_DISPATCHER_END
