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

#include "desktopclass.h"

#include "desktop_intern_protos.h"

IPTR desktopNew(Class * cl, Object * obj, struct opSet * msg)
{
    IPTR            retval = 0;
    struct DesktopClassData *data;
    //struct TagItem *tag;

    retval = DoSuperMethodA(cl, obj, (Msg) msg);
    if (retval)
    {
        obj = (Object *) retval;
        data = INST_DATA(cl, obj);
    }

    return retval;
}

IPTR desktopSet(Class * cl, Object * obj, struct opSet * msg)
{
    struct DesktopClassData *data;
    IPTR            retval = 1;
    struct TagItem *tag,
                   *tstate = msg->ops_AttrList;

    data = (struct DesktopClassData *) INST_DATA(cl, obj);

    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
            case DA_ActiveWindow:
                data->activeWindow = (Object *) tag->ti_Data;
                break;
            default:
                break;
        }
    }

    retval = DoSuperMethodA(cl, obj, (Msg) msg);

    return retval;
}

IPTR desktopGet(Class * cl, Object * obj, struct opGet * msg)
{
    IPTR            retval = 1;
    struct DesktopClassData *data;

    data = (struct DesktopClassData *) INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
        case DA_ActiveWindow:
            *msg->opg_Storage = (IPTR) data->activeWindow;
            break;
        default:
            retval = DoSuperMethodA(cl, obj, (Msg) msg);
            break;
    }

    return retval;
}

IPTR desktopDispose(Class * cl, Object * obj, Msg msg)
{
    IPTR            retval;

    retval = DoSuperMethodA(cl, obj, msg);

    return retval;
}

BOOPSI_DISPATCHER(IPTR, desktopDispatcher, cl, obj, msg)
{
    ULONG           retval = 0;

    switch (msg->MethodID)
    {
        case OM_NEW:
            retval = desktopNew(cl, obj, (struct opSet *) msg);
            break;
        case OM_SET:
            retval = desktopSet(cl, obj, (struct opSet *) msg);
            break;
        case OM_GET:
            retval = desktopGet(cl, obj, (struct opGet *) msg);
            break;
        case OM_DISPOSE:
            retval = desktopDispose(cl, obj, msg);
            break;
        default:
            retval = DoSuperMethodA(cl, obj, msg);
            break;
    }

    return retval;
}
BOOPSI_DISPATCHER_END
