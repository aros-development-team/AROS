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

#include "internalwindowopsclass.h"
#include "iconcontainerclass.h"

#include "desktop_intern_protos.h"

IPTR internalWindowOpsNew(Class * cl, Object * obj, struct opSet * msg)
{
    IPTR            retval = 0;
    struct InternalWindowOpsClassData *data;
    //struct TagItem *tag;

    retval = DoSuperMethodA(cl, obj, (Msg) msg);
    if (retval)
    {
        obj = (Object *) retval;
        data = INST_DATA(cl, obj);
    }

    return retval;
}

IPTR internalWindowOpsSet(Class * cl, Object * obj, struct opSet * msg)
{
    struct InternalWindowOpsClassData *data;
    IPTR            retval = 1;
    struct TagItem *tag,
                   *tstate = msg->ops_AttrList;

    data = (struct InternalWindowOpsClassData *) INST_DATA(cl, obj);

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

IPTR internalWindowOpsGet(Class * cl, Object * obj, struct opGet * msg)
{
    IPTR            retval = 1;
    struct InternalWindowOpsClassData *data;

    data = (struct InternalWindowOpsClassData *) INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
        default:
            retval = DoSuperMethodA(cl, obj, (Msg) msg);
            break;
    }

    return retval;
}

IPTR internalWindowOpsDispose(Class * cl, Object * obj, Msg msg)
{
    IPTR            retval;

    retval = DoSuperMethodA(cl, obj, msg);

    return retval;
}

IPTR internalWindowOpsExecute(Class * cl, Object * obj,
                              struct opExecute * msg)
{
    IPTR            retval = 0;
    struct InternalWindowOpsClassData *data;
    Object         *iconcontainer = NULL;

    data = (struct InternalWindowOpsClassData *) INST_DATA(cl, obj);

    GetAttr(MUIA_Window_RootObject, msg->target, (IPTR *) &iconcontainer);

    switch (msg->operationCode)
    {
        // close
        case (DOC_WINDOWOP | 1):
        // SetAttrs(msg->target, ICA_Open, FALSE, TAG_END);
            SetAttrs(msg->target, MUIA_Window_Open, FALSE, TAG_END);
            break;
        // view by
        case (DOC_WINDOWOP | 2):
            break;
        // large icons
        case (DOC_WINDOWOP | 3):
            SetAttrs(iconcontainer, ICA_ViewMode, ICAVM_LARGE);
            break;
        // small icons
        case (DOC_WINDOWOP | 4):
            SetAttrs(iconcontainer, ICA_ViewMode, ICAVM_SMALL);
            break;
        // detail
        case (DOC_WINDOWOP | 5):
            SetAttrs(iconcontainer, ICA_ViewMode, ICAVM_DETAIL);
            break;
    }

    return retval;
}

BOOPSI_DISPATCHER(IPTR, internalWindowOpsDispatcher, cl, obj, msg)
{
    ULONG           retval = 0;

    switch (msg->MethodID)
    {
        case OM_NEW:
            retval = internalWindowOpsNew(cl, obj, (struct opSet *) msg);
            break;
        case OM_SET:
            retval = internalWindowOpsSet(cl, obj, (struct opSet *) msg);
            break;
        case OM_GET:
            retval = internalWindowOpsGet(cl, obj, (struct opGet *) msg);
            break;
        case OM_DISPOSE:
            retval = internalWindowOpsDispose(cl, obj, msg);
            break;
        case OPM_Execute:
            retval =
                internalWindowOpsExecute(cl, obj, (struct opExecute *) msg);
            break;
        default:
            retval = DoSuperMethodA(cl, obj, msg);
            break;
    }

    return retval;
}
BOOPSI_DISPATCHER_END
