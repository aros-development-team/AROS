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
#include "worker.h"
#include "desktop_intern.h"

#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "presentation.h"
#include "diskiconclass.h"

#include "desktop_intern_protos.h"

IPTR diskIconNew(Class * cl, Object * obj, struct opSet * msg)
{
    IPTR            retval = 0;
    struct DiskIconClassData *data;
    //struct TagItem *tag;

    retval = DoSuperMethodA(cl, obj, (Msg) msg);

    if (retval)
    {
        obj = (Object *) retval;
        data = INST_DATA(cl, obj);
    }

    return retval;
}

IPTR diskIconSet(Class * cl, Object * obj, struct opSet * msg)
{
    struct DiskIconClassData *data;
    IPTR            retval = 1;
    struct TagItem *tag,
                   *tstate = msg->ops_AttrList;

    data = (struct DiskIconClassData *) INST_DATA(cl, obj);

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

IPTR diskIconGet(Class * cl, Object * obj, struct opGet * msg)
{
    IPTR            retval = 1;
    struct DiskIconClassData *data;

    data = (struct DiskIconClassData *) INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
        default:
            retval = DoSuperMethodA(cl, obj, (Msg) msg);
            break;
    }

    return retval;
}

IPTR diskIconDispose(Class * cl, Object * obj, Msg msg)
{
    IPTR            retval;

    retval = DoSuperMethodA(cl, obj, msg);

    return retval;
}

BOOPSI_DISPATCHER(IPTR, diskIconDispatcher, cl, obj, msg)
{
    ULONG           retval = 0;

    switch (msg->MethodID)
    {
        case OM_NEW:
            retval = diskIconNew(cl, obj, (struct opSet *) msg);
            break;
        case OM_SET:
            retval = diskIconSet(cl, obj, (struct opSet *) msg);
            break;
        case OM_GET:
            retval = diskIconGet(cl, obj, (struct opGet *) msg);
            break;
        case OM_DISPOSE:
            retval = diskIconDispose(cl, obj, msg);
            break;
        default:
            retval = DoSuperMethodA(cl, obj, msg);
            break;
    }

    return retval;
}
BOOPSI_DISPATCHER_END
