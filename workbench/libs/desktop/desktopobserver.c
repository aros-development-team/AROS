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
#include "desktopobserver.h"
#include "presentation.h"
#include "iconcontainerclass.h"
#include "observer.h"
#include "iconclass.h"

#include "desktop_intern_protos.h"

IPTR desktopObsNew(Class * cl, Object * obj, struct opSet *msg)
{
    IPTR            retval = 0;
    struct DesktopObserverClassData *data;
    struct TagItem *tag;
    Class   *defaultWindowClass = NULL;
    struct TagItem *defaultWindowArgs = NULL;

    tag = FindTagItem(DOA_DefaultWindowClass, msg->ops_AttrList);
    if (tag)
    {
        defaultWindowClass = (Class *) tag->ti_Data;
    // this will change, save the variable in a new
    // desktopcontext area
        DesktopBase->db_DefaultWindow = defaultWindowClass;
        tag->ti_Tag = TAG_IGNORE;
    }

    tag = FindTagItem(DOA_DefaultWindowArguments, msg->ops_AttrList);
    if (tag)
    {
        defaultWindowArgs = (struct TagItem *) tag->ti_Data;
    // this will change, save the variable in a new
    // desktopcontext area
        DesktopBase->db_DefaultWindowArguments = defaultWindowArgs;
        tag->ti_Tag = TAG_IGNORE;
    }

    retval = DoSuperMethodA(cl, obj, (Msg) msg);
    if (retval)
    {
        obj = (Object *) retval;
        data = INST_DATA(cl, obj);
        data->defaultWindow = defaultWindowClass;
        data->defaultWindowArgs = defaultWindowArgs;
    }

    return retval;
}

IPTR desktopObsSet(Class * cl, Object * obj, struct opSet * msg)
{
    struct DesktopObserverClassData *data;
    IPTR            retval = 1;
    struct TagItem *tag,
                   *tstate = msg->ops_AttrList;

    data = (struct DesktopObserverClassData *) INST_DATA(cl, obj);

    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
            case OA_InTree:
                {
                    struct HandlerTopLevelRequest *htl;

                    htl =
                        createTLScanMessage(DIMC_TOPLEVEL, NULL, LDF_VOLUMES,
                                            obj, _app(_presentation(obj)));
                    PutMsg(DesktopBase->db_HandlerPort,
                           (struct Message *) htl);

                    break;
                }
            default:
                break;
        }
    }

    retval = DoSuperMethodA(cl, obj, (Msg) msg);

    return retval;
}

IPTR desktopObsGet(Class * cl, Object * obj, struct opGet * msg)
{
    IPTR            retval = 1;
    struct DesktopObserverClassData *data;

    data = (struct DesktopObserverClassData *) INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
        default:
            retval = DoSuperMethodA(cl, obj, (Msg) msg);
            break;
    }

    return retval;
}

IPTR desktopObsDispose(Class * cl, Object * obj, Msg msg)
{
    IPTR            retval;
    struct DesktopObserverClassData *data;

    data = (struct DesktopObserverClassData *) INST_DATA(cl, obj);
    retval = DoSuperMethodA(cl, obj, msg);

    return retval;
}

IPTR desktopObsAddIcons(Class * cl, Object * obj, struct icoAddIcon * msg)
{
    IPTR            retval = 0;
    ULONG           i;
    Object         *newIcon;
    ULONG           kind;

    // struct DesktopObserverClassData *data;

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
                continue; /* skip unknown diskobject types */
        }

        newIcon = CreateDesktopObject
        (
            kind, 
        
            IA_DiskObject,  (IPTR) msg->wsr_ResultsArray[i].sr_DiskObject,
            IA_Label,       (IPTR) msg->wsr_ResultsArray[i].sr_Name,
            IA_Desktop,     (IPTR) _presentation(obj),
            // IA_Directory,   data->directory,
            MUIA_Draggable, TRUE,
            
            TAG_DONE
        );
        
        DoMethod(_presentation(obj), OM_ADDMEMBER, (IPTR) newIcon);
    }

    FreeVec(msg->wsr_ResultsArray);

    return retval;
}

BOOPSI_DISPATCHER(IPTR, desktopObserverDispatcher, cl, obj, msg)
{
    ULONG           retval = 0;

    switch (msg->MethodID)
    {
        case OM_NEW:
            retval = desktopObsNew(cl, obj, (struct opSet *) msg);
            break;
        case OM_SET:
            retval = desktopObsSet(cl, obj, (struct opSet *) msg);
            break;
        case OM_GET:
            retval = desktopObsGet(cl, obj, (struct opGet *) msg);
            break;
        case OM_DISPOSE:
            retval = desktopObsDispose(cl, obj, msg);
            break;
        case ICOM_AddIcons:
            retval = desktopObsAddIcons(cl, obj, (struct icoAddIcon *) msg);
            break;
        default:
            retval = DoSuperMethodA(cl, obj, msg);
            break;
    }

    return retval;
}
BOOPSI_DISPATCHER_END
