/*
   Copyright © 1995-2002, The AROS Development Team. All rights reserved.
   $Id$ 
 */

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/memory.h>
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
#include "observer.h"

#include "desktop_intern_protos.h"

IPTR observerNew(Class * cl, Object * obj, struct opSet * msg)
{
    IPTR            retval = 0;
    struct ObserverClassData *data,
                   *tstate = msg->ops_AttrList;
    struct TagItem *tag;
    Object         *presentation = NULL,
        *parent = NULL;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case OA_Presentation:
                presentation = (Object *) tag->ti_Data;
                break;

            case OA_Parent:
                parent = (Object *) tag->ti_Data;
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
        data->presentation = presentation;
        data->parent = parent;
        NewList((struct List *) &data->freeList);
        data->inTree = FALSE;

        DoMethod(presentation, MUIM_Notify, PA_InTree, MUIV_EveryTime, obj, 3,
                 MUIM_Set, OA_InTree, TRUE);
    }

    return retval;
}

IPTR observerSet(Class * cl, Object * obj, struct opSet * msg)
{
    struct ObserverClassData *data;
    IPTR            retval = 1;
    struct TagItem *tag,
                   *tstate = msg->ops_AttrList;

    data = (struct ObserverClassData *) INST_DATA(cl, obj);

    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
            case OA_InTree:
                {
                    Object         *pparent = NULL;

                    data->inTree = tag->ti_Data;
                    GetAttr(MUIA_Parent, data->presentation, &pparent);
                // something has to be at the top of the tree...
                // the top object won't have a parent
                    if (data->parent)
                    {
                        data->parent = _observer(pparent);
                        if (data->parent)
                            DoMethod(obj, MUIM_Notify, OA_Disused, TRUE,
                                     MUIV_EveryTime, data->parent, 2,
                                     OM_Delete, obj);
                    }
                    DoMethod(data->presentation, MUIM_Notify, PA_Disused,
                             TRUE, MUIV_EveryTime, obj, 3, MUIM_Set,
                             OA_Disused, TRUE);
                    break;
                }
            case OA_Parent:
                data->parent = tag->ti_Data;
                if (data->inTree)
                    DoMethod(obj, MUIM_Notify, OA_Disused, TRUE,
                             MUIV_EveryTime, data->parent, 2, OM_Delete, obj);
                break;
            default:
                retval = DoSuperMethodA(cl, obj, (Msg) msg);
                break;
        }
    }

    return retval;
}

IPTR observerGet(Class * cl, Object * obj, struct opGet * msg)
{
    IPTR            retval = 1;
    struct ObserverClassData *data;

    data = (struct ObserverClassData *) INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
        default:
            retval = DoSuperMethodA(cl, obj, (Msg) msg);
            break;
    }

    return retval;
}

IPTR observerDispose(Class * cl, Object * obj, Msg msg)
{
    IPTR            retval;
    struct ObserverClassData *data;
    struct FreeNode *fn;

    data = (struct ObserverClassData *) INST_DATA(cl, obj);

    fn = (struct FreeNode *) RemHead((struct List *) &data->freeList);
    while (fn)
    {
        FreeVec(fn->f_mem);
        FreeVec(fn);
    }

    retval = DoSuperMethodA(cl, obj, msg);

    return retval;
}

IPTR observerFreeListAdd(Class * cl, Object * obj,
                         struct ObsFreeListAddMsg * msg)
{
    IPTR            retval = 0;
    struct FreeNode *fn;
    struct ObserverClassData *data;

    data = (struct ObserverClassData *) INST_DATA(cl, obj);

    fn = AllocVec(sizeof(struct FreeNode), MEMF_ANY);
    fn->f_mem = msg->free;

    AddTail((struct List *) &data->freeList, (struct Node *) fn);

    return retval;
}

IPTR observerDelete(Class * cl, Object * obj, struct ObsDeleteMsg * msg)
{
    IPTR            retval = 0;
    struct ObserverClassData *data;

    data = (struct ObserverClassData *) INST_DATA(cl, obj);

    DisposeObject(msg->obj);

    return retval;
}

AROS_UFH3(IPTR, observerDispatcher,
          AROS_UFHA(Class *, cl, A0),
          AROS_UFHA(Object *, obj, A2), AROS_UFHA(Msg, msg, A1))
{
    ULONG           retval = 0;

    switch (msg->MethodID)
    {
        case OM_NEW:
            retval = observerNew(cl, obj, (struct opSet *) msg);
            break;
        case OM_SET:
            retval = observerSet(cl, obj, (struct opSet *) msg);
            break;
        case OM_GET:
            retval = observerGet(cl, obj, (struct opGet *) msg);
            break;
        case OM_DISPOSE:
            retval = observerDispose(cl, obj, msg);
            break;
        case OM_FreeList_Add:
            retval =
                observerFreeListAdd(cl, obj,
                                    (struct ObsFreeListAddMsg *) msg);
            break;
        case OM_Delete:
            retval = observerDelete(cl, obj, (struct ObsDeleteMsg *) msg);
            break;
        default:
            retval = DoSuperMethodA(cl, obj, msg);
            break;
    }

    return retval;
}
