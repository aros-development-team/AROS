/*
   Copyright © 1995-2002, The AROS Development Team. All rights reserved.
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

#include "desktop_intern_protos.h"

#include "presentation.h"

IPTR presentationNew(Class * cl, Object * obj, struct opSet * msg)
{
    IPTR            retval = 0;
    struct PresentationClassData *data;
    struct TagItem *tag,
                   *tstate = msg->ops_AttrList;
    Object         *observer = NULL;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case PA_Observer:
                observer = (Object *) tag->ti_Data;
                break;

            default:
                continue;       /* Don't supress non-processed tags */
        }

        tag->ti_Tag = TAG_IGNORE;
    }

    retval = DoSuperMethodA(cl, obj, (Msg) msg);
    if (retval)
    {
        obj = (Object *) retval;
        data = INST_DATA(cl, obj);
        data->observer = observer;
    }

    return retval;
}

IPTR presentationSet(Class * cl, Object * obj, struct opSet * msg)
{
    struct PresentationClassData *data;
    IPTR            retval = 1;
    struct TagItem *tag,
                   *tstate = msg->ops_AttrList;

    data = (struct PresentationClassData *) INST_DATA(cl, obj);

    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
            case PA_Observer:
                data->observer = (Object *) tag->ti_Data;
                break;
            default:
                break;
        }
    }

    retval = DoSuperMethodA(cl, obj, (Msg) msg);

    return retval;
}

IPTR presentationGet(Class * cl, Object * obj, struct opGet * msg)
{
    IPTR            retval = 1;
    struct PresentationClassData *data;

    data = (struct PresentationClassData *) INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
        default:
            retval = DoSuperMethodA(cl, obj, (Msg) msg);
            break;
    }

    return retval;
}

IPTR presentationDispose(Class * cl, Object * obj, Msg msg)
{
    IPTR            retval;

    SetAttrs(obj, PA_Disused, TRUE, TAG_END);

    retval = DoSuperMethodA(cl, obj, msg);

    return retval;
}

IPTR parentConnectParent(Class * cl, Object * obj,
                         struct MUIP_ConnectParent * msg)
{
    IPTR            retval;

    retval = DoSuperMethodA(cl, obj, (Msg) msg);
    SetAttrs(obj, PA_InTree, TRUE, TAG_END);
    return retval;
}

BOOPSI_DISPATCHER(IPTR, presentationDispatcher, cl, obj, msg)
{
    ULONG           retval = 0;

    switch (msg->MethodID)
    {
        case OM_NEW:
            retval = presentationNew(cl, obj, (struct opSet *) msg);
            break;
        case OM_SET:
            retval = presentationSet(cl, obj, (struct opSet *) msg);
            break;
        case OM_GET:
            retval = presentationGet(cl, obj, (struct opGet *) msg);
            break;
        case OM_DISPOSE:
            retval = presentationDispose(cl, obj, msg);
            break;
        case MUIM_ConnectParent:
            retval =
                parentConnectParent(cl, obj,
                                    (struct MUIP_ConnectParent *) msg);
            break;
        default:
            retval = DoSuperMethodA(cl, obj, msg);
            break;
    }

    return retval;
}
BOOPSI_DISPATCHER_END
