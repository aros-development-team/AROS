/*
   Copyright © 1995-2002, The AROS Development Team. All rights reserved.
   $Id$
 */

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <libraries/mui.h>

#include <proto/dos.h>
#include <proto/desktop.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "desktop_intern.h"
#include "presentation.h"
// FIXME: for IA_Selected, but it should be in abstracticon
#include "iconclass.h"

#include "desktop_intern_protos.h"

#include "abstracticoncontainer.h"

void broadcastMessage(Class * cl, Object * obj, Msg msg)
{
    struct MemberNode *mn;
    struct AbstractIconContainerData *data;

    data=(struct AbstractIconContainerData*)INST_DATA(cl, obj);

    mn=(struct MemberNode *) data->memberList.mlh_Head;
    while(mn->m_Node.mln_Succ)
    {
        DoMethodA(mn->m_Object, msg);
        mn=(struct MemberNode*)mn->m_Node.mln_Succ;
    }
}

struct MemberNode* findMember(struct MinList *list, Object *object)
{
    struct MemberNode *mn=(struct MemberNode*)list->mlh_Head;
    BOOL found=FALSE;

    while(!found && mn->m_Node.mln_Succ)
    {
        if(mn->m_Object==object)
            found=TRUE;
        else
            mn=(struct MemberNode*)mn->m_Node.mln_Succ;
    }

    return mn;
}

IPTR abstractIconConNew(Class * cl, Object * obj, struct opSet *ops)
{
    IPTR retval = 0;
    struct AbstractIconContainerData *data;
    struct TagItem *tag, *tstate = ops->ops_AttrList;
    Object *desktop = NULL;
    BOOL applyMethodsToMembers=TRUE;

    while((tag=NextTagItem(&tstate)))
    {
        switch(tag->ti_Tag)
        {
            case AICA_Desktop:
                desktop=(Object*)tag->ti_Data;
                break;
            case AICA_ApplyMethodsToMembers:
                applyMethodsToMembers=(BOOL)tag->ti_Data;
                break;
            default:
                continue; // Don't supress non-processed tags
        }

        tag->ti_Tag=TAG_IGNORE;
    }

    retval=DoSuperMethodA(cl, obj, (Msg)ops);

    if(retval)
    {
        obj=(Object*)retval;
        data=(struct AbstractIconContainerData*)INST_DATA(cl, obj);

        // FIXME: this doesn't look right
        if(desktop==NULL)
            desktop=obj;

        NEWLIST((struct List*)&data->memberList);
        NEWLIST((struct List*)&data->selectedList);

        data->memberCount=0;
        data->applyMethodsToMembers=applyMethodsToMembers;
        data->desktop=desktop;
    }

    return retval;
}

IPTR abstractIconConAdd(Class *cl, Object *obj, struct opMember *msg)
{
    struct AbstractIconContainerData *data;
    struct MemberNode *mn;
    ULONG  retval=1;

    data=(struct AbstractIconContainerData*)INST_DATA(cl, obj);

    mn=AllocVec(sizeof(struct MemberNode), MEMF_ANY);
    mn->m_Object=msg->opam_Object;

    muiNotifyData(msg->opam_Object)->mnd_ParentObject=obj;
    DoMethod(msg->opam_Object, MUIM_ConnectParent, (IPTR) obj);

    DoMethod(msg->opam_Object, MUIM_Notify, IA_Selected, MUIV_EveryTime, 
    (IPTR) obj, 3, AICM_UpdateSelectList, (IPTR) msg->opam_Object, MUIV_TriggerValue);

    data->memberCount++;
    AddTail((struct List*)&data->memberList, (struct Node*)mn);

    return retval;
}

IPTR abstractIconConRemove(Class *cl, Object *obj, struct opMember *msg)
{
    struct AbstractIconContainerData *data;
    struct MemberNode *mn;
    ULONG  retval=-1;

    data=(struct AbstractIconContainerData*)INST_DATA(cl, obj);

    mn=findMember(&data->memberList, msg->opam_Object);
    if(mn)
    {
        data->memberCount--;
        Remove((struct Node*)mn);

        if(muiNotifyData(obj)->mnd_GlobalInfo)
        {
            DoMethod(msg->opam_Object, MUIM_DisconnectParent);
            muiNotifyData(msg->opam_Object)->mnd_ParentObject=NULL;
        }

        FreeVec(mn);
        retval=1;
    }

    return retval;
}

IPTR abstractIconConSet(Class* cl, Object* obj, struct opSet* msg)
{
    struct AbstractIconContainerData *data;
    IPTR retval;
    struct TagItem *tag, *tstate=msg->ops_AttrList;

    data=(struct AbstractIconContainerData*)INST_DATA(cl, obj);

    while((tag=NextTagItem(&tstate)))
    {
        switch(tag->ti_Tag)
        {
            case AICA_Desktop:
                data->desktop=(Object*)tag->ti_Data;
                tag->ti_Tag=TAG_IGNORE;
                break;
            case AICA_ApplyMethodsToMembers:
                data->applyMethodsToMembers=(BOOL)tag->ti_Data;
                tag->ti_Tag=TAG_IGNORE;
                break;
        }
    }

    retval=DoSuperMethodA(cl, obj, (Msg) msg);
    broadcastMessage(cl, obj, (Msg) msg);

    return 0;
}

struct MinList* createObjectList(struct MinList *copyFrom, struct AbstractIconContainerData *data)
{
    struct MinList *list;
    struct MemberNode *mn;

    list=AllocVec(sizeof(struct MinList), MEMF_ANY);
    NEWLIST(list);

    mn=(struct MemberNode*)data->selectedList.mlh_Head;
    while(mn->m_Node.mln_Succ)
    {
        AddTail((struct List*)list, (struct Node *)_OBJECT(mn->m_Object));
        mn=(struct MemberNode*)mn->m_Node.mln_Succ;
    }

    return list;
}

IPTR abstractIconConGet(Class * cl, Object * obj, struct opGet * msg)
{
    IPTR retval=1;
    struct AbstractIconContainerData *data;

    data=(struct AbstractIconContainerData*)INST_DATA(cl, obj);

    switch(msg->opg_AttrID)
    {
        case AICA_ApplyMethodsToMembers:
            *msg->opg_Storage=(ULONG)data->applyMethodsToMembers;
            break;
        // FIXME: change this to a Lock/Unlock method pair, to
        // deallocate objectList
        case AICA_SelectedIcons:
        {
            struct MinList *objectList;

            objectList=createObjectList(&data->selectedList, data);
            *msg->opg_Storage=(ULONG)objectList;
            break;
        }
        // FIXME: change this to a Lock/Unlock method pair, to
        // deallocate objectList
        case AICA_MemberList:
        {
            struct MinList *objectList;

            objectList=createObjectList(&data->memberList, data);
            *msg->opg_Storage=(ULONG)objectList;
            break;
        }
        case AICA_Desktop:
            *msg->opg_Storage=(ULONG)data->desktop;
            break;
        default:
            retval=DoSuperMethodA(cl, obj, (Msg)msg);
            break;
    }

    return retval;
}

// FIXME: finish me
IPTR abstractIconConUnselectAll(Class * cl, Object * obj, Msg msg)
{
    IPTR retval = 0;
//    struct MemberNode *mn;
//    struct AbstractIconContainerData *data;

//    data=(struct AbstractIconContainerData*)INST_DATA(cl, obj);

// mn=(struct MemberNode*)data->memberList.mlh_Head;
// while(mn->m_Node.mln_Succ)
// {
// if(_selected(mn->m_Object) && )
// {
// SetAttrs(mn->m_Object, IA_Selected, FALSE, TAG_END);
// }
// mn=(struct MemberNode*)mn->m_Node.mln_Succ;
// }

    return retval;
}

IPTR abstractIconConUpdateSelectList(Class *cl, Object *obj, struct opUpdateSelectList * msg)
{
    IPTR retval=0;
    struct AbstractIconContainerData *data;

    data=(struct AbstractIconContainerData*)INST_DATA(cl, obj);

    if(msg->selectState==TRUE)
        AddTail((struct List*)&(data->selectedList), (struct Node*)_OBJECT(msg->target));
    else if(msg->selectState==FALSE)
        Remove((struct Node *)_OBJECT(msg->target));

    return retval;
}

IPTR abstractIconConDispose(Class* cl, Object* obj, Msg msg)
{
    IPTR retval;

    broadcastMessage(cl, obj, msg);
    retval=DoSuperMethodA(cl, obj, msg);

    return retval;
}

BOOPSI_DISPATCHER(IPTR, abstractIconContainerDispatcher, cl, obj, msg)
{

    ULONG retval=0;

    switch(msg->MethodID)
    {
        case OM_NEW:
            retval=abstractIconConNew(cl, obj, (struct opSet*)msg);
            break;
        case OM_ADDMEMBER:
            retval=abstractIconConAdd(cl, obj, (struct opMember*)msg);
            break;
        case OM_REMMEMBER:
            retval=abstractIconConRemove(cl, obj, (struct opMember*)msg);
            break;
        case OM_SET:
            retval=abstractIconConSet(cl, obj, (struct opSet*)msg);
            break;
        case OM_GET:
            retval=abstractIconConGet(cl, obj, (struct opGet*)msg);
            break;
        case AICM_UnselectAll:
            retval=abstractIconConUnselectAll(cl, obj, msg);
            break;
        case AICM_UpdateSelectList:
            retval=abstractIconConUpdateSelectList(cl, obj, (struct opUpdateSelectList*)msg);
            break;
        case OM_DISPOSE:
            retval=abstractIconConDispose(cl, obj, msg);
            break;
        default:
        {
            struct AbstractIconContainerData *data;

            data=(struct AbstractIconContainerData*)INST_DATA(cl, obj);

            if(data->applyMethodsToMembers)
                broadcastMessage(cl, obj, msg);
            retval=DoSuperMethodA(cl, obj, msg);
            break;
        }
    }

    return retval;
}
BOOPSI_DISPATCHER_END



