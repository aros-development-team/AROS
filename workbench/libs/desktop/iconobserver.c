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
#include <proto/icon.h>
#include <proto/utility.h>

#include "presentation.h"
#include "observer.h"
#include "iconclass.h"
#include "iconobserver.h"
#include "abstracticon.h"

#include "desktop_intern_protos.h"

#include <string.h>

IPTR iconObserverNew(Class * cl, Object * obj, struct opSet * msg)
{
    IPTR            retval = 0;
    struct IconObserverClassData *data;
    struct TagItem *tag,
                   *tstate = msg->ops_AttrList;
    UBYTE          *name = NULL,
        *directory = NULL;
    BOOL            selected = FALSE;
    UBYTE          *comment = NULL;
    BOOL            script = FALSE,
                    pure = FALSE,
                    archived = FALSE,
                    readable = TRUE,
                    writeable = TRUE,
                    executable = FALSE,
                    deleteable = TRUE;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case IOA_Selected:
                selected = (BOOL) tag->ti_Data;
                break;

            case IOA_Name:
                name = (UBYTE *) tag->ti_Data;
                break;

            case IOA_Directory:
                directory = (UBYTE *) tag->ti_Data;
                break;

            case IOA_Comment:
                comment = (UBYTE *) tag->ti_Data;
                break;

            case IOA_Script:
                script = (BOOL) tag->ti_Data;
                break;

            case IOA_Pure:
                pure = (BOOL) tag->ti_Data;
                break;

            case IOA_Archived:
                archived = (BOOL) tag->ti_Data;
                break;

            case IOA_Readable:
                readable = (BOOL) tag->ti_Data;
                break;

            case IOA_Writeable:
                writeable = (BOOL) tag->ti_Data;
                break;

            case IOA_Executable:
                executable = (BOOL) tag->ti_Data;
                break;

            case IOA_Deleteable:
                deleteable = (BOOL) tag->ti_Data;
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
        data->selected = selected;
        data->name = name;
        data->directory = directory;
        data->comment = comment;
        data->script = script;
        data->pure = pure;
        data->archived = archived;
        data->readable = readable;
        data->writeable = writeable;
        data->executable = executable;
        data->deleteable = deleteable;

        DoMethod(_presentation(obj), MUIM_Notify, IA_Executed, TRUE, (IPTR) obj, 1,
                 IOM_Execute);
        DoMethod(_presentation(obj), MUIM_Notify, IA_Selected, MUIV_EveryTime,
                 (IPTR) obj, 3, MUIM_Set, IOA_Selected, MUIV_TriggerValue);
        DoMethod(_presentation(obj), MUIM_Notify, IA_Directory,
                 MUIV_EveryTime, (IPTR) obj, 3, MUIM_Set, IOA_Directory,
                 MUIV_TriggerValue);
        DoMethod(_presentation(obj), MUIM_Notify, AIA_Comment, MUIV_EveryTime,
                 (IPTR) obj, 3, MUIM_Set, IOA_Comment, MUIV_TriggerValue);

        DoMethod(_presentation(obj), MUIM_Notify, AIA_Script, MUIV_EveryTime,
                 (IPTR) obj, 3, MUIM_Set, IOA_Script, MUIV_TriggerValue);
        DoMethod(_presentation(obj), MUIM_Notify, AIA_Pure, MUIV_EveryTime,
                 (IPTR) obj, 3, MUIM_Set, IOA_Pure, MUIV_TriggerValue);
        DoMethod(_presentation(obj), MUIM_Notify, AIA_Archived, MUIV_EveryTime,
                 (IPTR) obj, 3, MUIM_Set, IOA_Archived, MUIV_TriggerValue);
        DoMethod(_presentation(obj), MUIM_Notify, AIA_Readable, MUIV_EveryTime,
                 (IPTR) obj, 3, MUIM_Set, IOA_Readable, MUIV_TriggerValue);
        DoMethod(_presentation(obj), MUIM_Notify, AIA_Writeable,
                 MUIV_EveryTime, (IPTR) obj, 3, MUIM_Set, IOA_Writeable,
                 MUIV_TriggerValue);
        DoMethod(_presentation(obj), MUIM_Notify, AIA_Executable,
                 MUIV_EveryTime, (IPTR) obj, 3, MUIM_Set, IOA_Executable,
                 MUIV_TriggerValue);
        DoMethod(_presentation(obj), MUIM_Notify, AIA_Deleteable,
                 MUIV_EveryTime, (IPTR) obj, 3, MUIM_Set, IOA_Deleteable,
                 MUIV_TriggerValue);
	}

    return retval;
}

IPTR iconObserverSet(Class * cl, Object * obj, struct opSet * msg)
{
    struct IconObserverClassData *data;
    IPTR            retval = 1;
    struct TagItem *tag,
                   *tstate = msg->ops_AttrList;

    data = (struct IconObserverClassData *) INST_DATA(cl, obj);

    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
            case IOA_Comment:
                data->comment = (UBYTE *) tag->ti_Data;
            /*
               was this OM_SET triggered by a notify? 
             */
				if (strcmp(_comment(_presentation(obj)), data->comment))
                    DoMethod(_presentation(obj), MUIM_NoNotifySet, AIA_Comment,
                             (IPTR) data->comment);
				break;
            // TODO: When one of these bits is set, send a request to the
            // handler to do the change
            case IOA_Script:
                data->script = tag->ti_Data;
            /*
               was this OM_SET triggered by a notify?
             */
                if (_script(_presentation(obj)) != data->script)
                    DoMethod(_presentation(obj), MUIM_NoNotifySet, AIA_Script,
                             data->script);
                break;
            case IOA_Pure:
                data->pure = tag->ti_Data;
            /*
               was this OM_SET triggered by a notify?
             */
                if (_pure(_presentation(obj)) != data->pure)
                    DoMethod(_presentation(obj), MUIM_NoNotifySet, AIA_Pure,
                             data->pure);
                break;
            case IOA_Archived:
                data->archived = tag->ti_Data;
            /*
               was this OM_SET triggered by a notify?
             */
                if (_archived(_presentation(obj)) != data->archived)
                    DoMethod(_presentation(obj), MUIM_NoNotifySet,
                             AIA_Archived, data->archived);
                break;
            case IOA_Readable:
                data->readable = tag->ti_Data;
            /*
               was this OM_SET triggered by a notify?
             */
                if (_readable(_presentation(obj)) != data->readable)
                    DoMethod(_presentation(obj), MUIM_NoNotifySet,
                             AIA_Readable, data->readable);
                break;
            case IOA_Writeable:
                data->writeable = tag->ti_Data;
            /*
               was this OM_SET triggered by a notify?
             */
                if (_writeable(_presentation(obj)) != data->writeable)
                    DoMethod(_presentation(obj), MUIM_NoNotifySet,
                             AIA_Writeable, data->writeable);
                break;
            case IOA_Executable:
                data->comment = (UBYTE *) tag->ti_Data;
            /*
               was this OM_SET triggered by a notify?
             */
                if (_executable(_presentation(obj)) != data->executable)
                    DoMethod(_presentation(obj), MUIM_NoNotifySet,
                             AIA_Executable, data->executable);
                break;
            case IOA_Deleteable:
                data->comment = (UBYTE *) tag->ti_Data;
            /*
               was this OM_SET triggered by a notify?
             */
                if (_deleteable(_presentation(obj)) != data->deleteable)
                    DoMethod(_presentation(obj), MUIM_NoNotifySet,
                             AIA_Deleteable, data->deleteable);
                break;
            default:
                break;
        }
    }

    retval = DoSuperMethodA(cl, obj, (Msg) msg);

    return retval;
}

IPTR iconObserverGet(Class * cl, Object * obj, struct opGet * msg)
{
    IPTR            retval = 1;
    struct IconObserverClassData *data;

    data = (struct IconObserverClassData *) INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
        case IOA_Name:
            *msg->opg_Storage = (IPTR) data->name;
            break;
        case IOA_Script:
            *msg->opg_Storage = data->script;
            break;
        case IOA_Pure:
            *msg->opg_Storage = data->pure;
            break;
        case IOA_Archived:
            *msg->opg_Storage = data->archived;
            break;
        case IOA_Readable:
            *msg->opg_Storage = data->readable;
            break;
        case IOA_Writeable:
            *msg->opg_Storage = data->writeable;
            break;
        case IOA_Executable:
            *msg->opg_Storage = data->executable;
            break;
        case IOA_Deleteable:
            *msg->opg_Storage = data->deleteable;
            break;
        default:
            retval = DoSuperMethodA(cl, obj, (Msg) msg);
            break;
    }

    return retval;
}

IPTR iconObserverDispose(Class * cl, Object * obj, Msg msg)
{
    IPTR            retval;
    struct IconObserverClassData *data;

    data = (struct IconObserverClassData *) INST_DATA(cl, obj);

    FreeDiskObject(_diskobject(_presentation(obj)));
// note:
// IconObserverClassData.name is part of the ExAllBuffer and is freed
// by this object's parent
// IconObserverClassData.directory belongs to this object's parent, and
// is freed by it

    retval = DoSuperMethodA(cl, obj, msg);

    return retval;
}

IPTR iconObserverExecute(Class * cl, Object * obj, Msg msg)
{
    return 0;
}

BOOPSI_DISPATCHER(IPTR, iconObserverDispatcher, cl, obj, msg)
{
    ULONG retval = 0;

    switch (msg->MethodID)
    {
        case OM_NEW:
            retval = iconObserverNew(cl, obj, (struct opSet *) msg);
            break;
        case OM_SET:
            retval = iconObserverSet(cl, obj, (struct opSet *) msg);
            break;
        case OM_GET:
            retval = iconObserverGet(cl, obj, (struct opGet *) msg);
            break;
        case OM_DISPOSE:
            retval = iconObserverDispose(cl, obj, msg);
            break;
        case IOM_Execute:
            retval = iconObserverExecute(cl, obj, msg);
            break;
        default:
            retval = DoSuperMethodA(cl, obj, msg);
            break;
    }

    return retval;
}
BOOPSI_DISPATCHER_END
