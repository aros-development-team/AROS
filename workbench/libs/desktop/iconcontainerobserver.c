/*
   Copyright © 1995-2003, The AROS Development Team. All rights reserved.
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
#include <workbench/workbench.h>

#include <proto/dos.h>
#include <proto/desktop.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "support.h"
#include "worker.h"
#include "desktop_intern.h"
#include "iconcontainerobserver.h"
#include "presentation.h"
#include "iconcontainerclass.h"
#include "iconobserver.h"
#include "observer.h"
#include "iconclass.h"

#include "desktop_intern_protos.h"

IPTR iconConObsNew(Class * cl, Object * obj, struct opSet * msg)
{
    IPTR            retval = 0;
    struct IconContainerObserverClassData *data;
    struct TagItem *tag,
                   *tstate = msg->ops_AttrList;
    UBYTE          *directory = NULL;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case ICOA_Directory:
                directory = (UBYTE *) tag->ti_Data;
                break;

            default:
                continue; /* Don't supress non-processed tags */
        }

        tag->ti_Tag = TAG_IGNORE;
    }

    retval = DoSuperMethodA(cl, obj, (Msg) msg);
    if (retval)
    {
        obj = (Object *) retval;
        data = INST_DATA(cl, obj);
        data->directory = directory;
    // this is unlocked by the scanner worker
        data->dirLock = Lock(directory, ACCESS_READ);
    }

    return retval;
}

IPTR iconConObsSet(Class * cl, Object * obj, struct opSet * msg)
{
    struct IconContainerObserverClassData *data;
    IPTR            retval = 1;
    struct TagItem *tag,
                   *tstate = msg->ops_AttrList;

    data = (struct IconContainerObserverClassData *) INST_DATA(cl, obj);

    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
            case ICOA_Directory:
                data->directory = (UBYTE *) tag->ti_Data;
                break;
            case OA_InTree:
                {
                    struct HandlerScanRequest *hsr;

                    hsr =
                        createScanMessage(DIMC_SCANDIRECTORY, NULL,
                                          data->dirLock, obj,
                                          _app(_presentation(obj)));
                    PutMsg(DesktopBase->db_HandlerPort,
                           (struct Message *) hsr);

                    DoMethod(_presentation(obj), MUIM_KillNotify, PA_InTree);

                    break;
                }
            default:
                break;
        }
    }

    retval = DoSuperMethodA(cl, obj, (Msg) msg);

    return retval;
}

IPTR iconConObsGet(Class * cl, Object * obj, struct opGet * msg)
{
    IPTR            retval = 1;
    struct IconContainerObserverClassData *data;

    data = (struct IconContainerObserverClassData *) INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
        case ICOA_Directory:
            *msg->opg_Storage = (ULONG) data->directory;
            break;
        default:
            retval = DoSuperMethodA(cl, obj, (Msg) msg);
            break;
    }

    return retval;
}

IPTR iconConObsDispose(Class * cl, Object * obj, Msg msg)
{
    IPTR            retval;
    struct IconContainerObserverClassData *data;

    data = (struct IconContainerObserverClassData *) INST_DATA(cl, obj);
    UnLock(data->dirLock);
    retval = DoSuperMethodA(cl, obj, msg);

    return retval;
}

IPTR iconConObsAddIcons(Class * cl, Object * obj, struct icoAddIcon * msg)
{
    IPTR            retval = 0;
    ULONG           i;
    Object         *newIcon;
    ULONG           kind;
    struct IconContainerObserverClassData *data;
    Object         *desktop = NULL;

    data = (struct IconContainerObserverClassData *) INST_DATA(cl, obj);

    GetAttr(AICA_Desktop, _presentation(obj), (IPTR *) &desktop);

    for (i = 0; i < msg->wsr_Results; i++)
    {
        switch (msg->wsr_ResultsArray[i].sr_DiskObject->do_Type)
        {
            case WBDISK:
                kind = CDO_DiskIcon;
                break;
            
            case WBDRAWER:
                kind = CDO_DrawerIcon;
                break;
            
            case WBTOOL:
                kind = CDO_ToolIcon;
                break;
            
            case WBPROJECT:
                kind = CDO_ProjectIcon;
                break;
            
            case WBGARBAGE:
                kind = CDO_TrashcanIcon;
                break;
            
            case WBDEVICE:
            case WBKICK:
            case WBAPPICON:
            default:
                continue; /* skip unknown disk object types */
        }
        
        newIcon = CreateDesktopObject
        (
            kind,
            
            IOA_Name,         (IPTR) msg->wsr_ResultsArray[i].sr_Name,
            IOA_Directory,    (IPTR) data->directory,
            IOA_Comment,      (IPTR) msg->wsr_ResultsArray[i].sr_Comment,
            IOA_Script,       (IPTR) msg->wsr_ResultsArray[i].sr_Script,
            IOA_Pure,         (IPTR) msg->wsr_ResultsArray[i].sr_Pure,
            IOA_Readable,     (IPTR) msg->wsr_ResultsArray[i].sr_Read,
            IOA_Writeable,    (IPTR) msg->wsr_ResultsArray[i].sr_Write,
            IOA_Archived,     (IPTR) msg->wsr_ResultsArray[i].sr_Archive,
            IOA_Executable,   (IPTR) msg->wsr_ResultsArray[i].sr_Execute,
            IOA_Deleteable,   (IPTR) msg->wsr_ResultsArray[i].sr_Delete,
            IA_DiskObject,    (IPTR) msg->wsr_ResultsArray[i].sr_DiskObject,
            IA_Label,         (IPTR) msg->wsr_ResultsArray[i].sr_Name,
            IA_Size,          (IPTR) msg->wsr_ResultsArray[i].sr_Size,
            IA_LastModified,  (IPTR) &msg->wsr_ResultsArray[i].sr_LastModified,
            IA_Type,          (IPTR) msg->wsr_ResultsArray[i].sr_Type,
            IA_Desktop,       (IPTR) desktop,
            MUIA_Draggable,   TRUE,
            
            TAG_DONE
        );

        DoMethod(_presentation(obj), OM_ADDMEMBER, (IPTR) newIcon);
    }

    FreeVec(msg->wsr_ResultsArray);


    return retval;
}

BOOPSI_DISPATCHER(IPTR, iconContainerObserverDispatcher, cl, obj, msg)
{
    ULONG           retval = 0;

    switch (msg->MethodID)
    {
        case OM_NEW:
            retval = iconConObsNew(cl, obj, (struct opSet *) msg);
            break;
        case OM_SET:
            retval = iconConObsSet(cl, obj, (struct opSet *) msg);
            break;
        case OM_GET:
            retval = iconConObsGet(cl, obj, (struct opGet *) msg);
            break;
        case OM_DISPOSE:
            retval = iconConObsDispose(cl, obj, msg);
            break;
        case ICOM_AddIcons:
            retval = iconConObsAddIcons(cl, obj, (struct icoAddIcon *) msg);
            break;
        default:
            retval = DoSuperMethodA(cl, obj, msg);
            break;
    }

    return retval;
}
BOOPSI_DISPATCHER_END
