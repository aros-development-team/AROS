/*
   Copyright © 1995-2002, The AROS Development Team. All rights reserved.
   $Id$
 */

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <libraries/mui.h>

#include "abstracticon.h"
#include "desktop_intern.h"
#include "iconclass.h"

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "desktop_intern_protos.h"

IPTR abstractIconNew(Class *cl, Object *obj, struct opSet *msg)
{
    IPTR retval=0;
    struct AbstractIconClassData *data;
    struct TagItem *tag, *tstate = msg->ops_AttrList;
    BOOL script=FALSE, pure=FALSE, archived=FALSE, readable=FALSE, writeable=FALSE, executable=FALSE, deleteable=FALSE;
    UBYTE *comment=NULL;

    while((tag = NextTagItem(&tstate)) != NULL)
    {
        switch(tag->ti_Tag)
        {
            case AIA_Comment:
                comment = (UBYTE *) tag->ti_Data;
                break;
            case AIA_Script:
                script = (BOOL) tag->ti_Data;
                break;
            case AIA_Pure:
                pure = (BOOL) tag->ti_Data;
                break;
            case AIA_Archived:
                archived = (BOOL) tag->ti_Data;
                break;
            case AIA_Readable:
                readable = (BOOL) tag->ti_Data;
                break;
            case AIA_Writeable:
                writeable = (BOOL) tag->ti_Data;
                break;
            case AIA_Executable:
                executable = (BOOL) tag->ti_Data;
                break;
            case AIA_Deleteable:
                deleteable = (BOOL) tag->ti_Data;
                break;
        }
    }

    retval = DoSuperMethodA(cl, obj, (Msg) msg);

    if (retval)
    {
        obj=(Object*)retval;
        data=INST_DATA(cl, obj);

        data->script=script;
        data->pure=pure;
        data->archived=archived;
        data->readable=readable;
        data->writeable=writeable;
        data->executable=executable;
        data->deleteable=deleteable;
        data->comment=comment;
    }

    return retval;
}

IPTR abstractIconDispose(Class *cl, Object *obj, Msg msg)
{
    IPTR retval;

    retval=DoSuperMethodA(cl, obj, msg);

    return retval;
}

IPTR abstractIconGet(Class *cl, Object *obj, struct opGet *msg)
{
    IPTR retval=1;
    struct AbstractIconClassData *data;

    data=(struct AbstractIconClassData*)INST_DATA(cl, obj);

    switch(msg->opg_AttrID)
    {
        case AIA_Comment:
            *msg->opg_Storage=(ULONG)data->comment;
            break;
        case AIA_Script:
            *msg->opg_Storage=(ULONG)data->script;
            break;
        case AIA_Pure:
            *msg->opg_Storage=(ULONG)data->pure;
            break;
        case AIA_Archived:
            *msg->opg_Storage=(ULONG)data->archived;
            break;
        case AIA_Readable:
            *msg->opg_Storage=(ULONG)data->readable;
            break;
        case AIA_Writeable:
            *msg->opg_Storage=(ULONG)data->writeable;
            break;
        case AIA_Executable:
            *msg->opg_Storage=(ULONG)data->executable;
            break;
        case AIA_Deleteable:
            *msg->opg_Storage=(ULONG)data->deleteable;
            break;
        default:
            retval=DoSuperMethodA(cl, obj, (Msg)msg);
            break;
    }

    return retval;
}

IPTR abstractIconSet(Class *cl, Object *obj, struct opSet *msg)
{
    struct AbstractIconClassData *data;
    IPTR retval=1;
    struct TagItem *tag, *tstate=msg->ops_AttrList;
    BOOL doSuper=TRUE;

    data=(struct AbstractIconClassData*)INST_DATA(cl, obj);

    while((tag=NextTagItem(&tstate)))
    {
        switch(tag->ti_Tag)
        {
            case AIA_Comment:
                data->comment=(UBYTE*)tag->ti_Data;
                break;
            case AIA_Script:
                data->script=(BOOL)tag->ti_Data;
                break;
            case AIA_Pure:
                data->pure=(BOOL)tag->ti_Data;
                break;
            case AIA_Archived:
                data->archived=(BOOL)tag->ti_Data;
                break;
            case AIA_Readable:
                data->readable=(BOOL)tag->ti_Data;
                break;
            case AIA_Writeable:
                data->writeable=(BOOL)tag->ti_Data;
                break;
            case AIA_Executable:
                data->executable=(BOOL)tag->ti_Data;
                break;
            case AIA_Deleteable:
                data->deleteable=(BOOL)tag->ti_Data;
                break;
        }
    }

    if(doSuper)
        retval=DoSuperMethodA(cl, obj, (Msg) msg);

    return retval;
}

BOOPSI_DISPATCHER(IPTR, abstractIconDispatcher, cl, obj, msg)
{
    IPTR retval = 0;

    switch (msg->MethodID)
    {
        case OM_NEW:
            retval=abstractIconNew(cl, obj, (struct opSet*)msg);
            break;
        case OM_SET:
            retval=abstractIconSet(cl, obj, (struct opSet*)msg);
            break;
        case OM_GET:
            retval=abstractIconGet(cl, obj, (struct opGet*)msg);
            break;
        case OM_DISPOSE:
            retval=abstractIconDispose(cl, obj, msg);
            break;
        default:
            retval=DoSuperMethodA(cl, obj, msg);
            break;
    }

    return retval;
}
BOOPSI_DISPATCHER_END

