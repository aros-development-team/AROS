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
#include <libraries/gadtools.h>
#include <libraries/mui.h>
#include <libraries/desktop.h>

#include "support.h"
#include "worker.h"
#include "desktop_intern.h"

#include <proto/desktop.h>
#include <proto/dos.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "presentation.h"
#include "observer.h"
#include "iconobserver.h"
#include "drawericonobserver.h"
#include "desktopobserver.h"
#include "iconcontainerclass.h"
#include "iconclass.h"

#include "desktop_intern_protos.h"

#include <string.h>

IPTR containerIconObserverExecute(Class * cl, Object * obj, Msg msg)
{
    UBYTE          *name,
                   *directory,
                   *newDir;
    ULONG           dirNameLen = 0;
    ULONG           length = 0;
    struct ContainerIconObserverClassData *data;
    IPTR            retval = 1;
    Object         *horiz,
                   *vert,
                   *dirWindow,
                   *iconcontainer,
                   *strip;
    struct TagItem *icTags;
    BYTE            terminator;
    struct NewMenu *menuDat;
    Object         *desktop = NULL;
    STRPTR          title = NULL;
    
    data = (struct ContainerIconObserverClassData *) INST_DATA(cl, obj);
    retval = DoSuperMethodA(cl, obj, msg);

    name = _name(obj);
    directory = _directory(obj);

// directory is NULL if this is a disk icon
    if (directory)
        dirNameLen = strlen(directory);

    length = strlen(name) + dirNameLen + 1 /* ':' or '/' */ + 1;
    newDir = AllocVec(length, MEMF_ANY);

    if (directory)
    {
        strlcpy(newDir, directory, length);
        strlcat(newDir, name, length);
        strlcat(newDir, "/", length);
        title = name;
    }
    else
    {
        strlcpy(newDir, name, length);
        strlcat(newDir, ":", length);
        title = newDir;
    }

    horiz = PropObject,
        MUIA_Prop_Horiz, TRUE,
        MUIA_Prop_Entries, 0,
        MUIA_Prop_UseWinBorder, MUIV_Prop_UseWinBorder_Bottom, End;
    vert = PropObject,
        MUIA_Prop_Horiz, FALSE,
        MUIA_Prop_UseWinBorder, MUIV_Prop_UseWinBorder_Right, End;

    menuDat = BuildDesktopMenus();

    GetAttr(IA_Desktop, _presentation(obj), &desktop);

    icTags = AllocVec(sizeof(struct TagItem) * 10, MEMF_ANY);
    icTags[0].ti_Tag = MUIA_FillArea;
    icTags[0].ti_Data = FALSE;
    icTags[1].ti_Tag = ICOA_Directory;
    icTags[1].ti_Data = newDir;
    icTags[2].ti_Tag = ICA_VertScroller;
    icTags[2].ti_Data = vert;
    icTags[3].ti_Tag = ICA_HorizScroller;
    icTags[3].ti_Data = horiz;
    icTags[4].ti_Tag = MUIA_InnerLeft;
    icTags[4].ti_Data = 0;
    icTags[5].ti_Tag = MUIA_InnerTop;
    icTags[5].ti_Data = 0;
    icTags[6].ti_Tag = MUIA_InnerRight;
    icTags[6].ti_Data = 0;
    icTags[7].ti_Tag = MUIA_InnerBottom;
    icTags[7].ti_Data = 0;
    icTags[8].ti_Tag = ICA_Desktop;
    icTags[8].ti_Data = desktop;
    icTags[9].ti_Tag = TAG_END;
    icTags[9].ti_Data = 0;

    iconcontainer = CreateDesktopObjectA(CDO_IconContainer, icTags);

// TEMPORARY!!!!! Use CreateDesktopObjectA(CDO_Window.....) instead!
    dirWindow = WindowObject,
        MUIA_Window_Width, 300,
        MUIA_Window_Height, 300,
        MUIA_Window_Title,  title,
        MUIA_Window_Menustrip, strip = MUI_MakeObject(MUIO_MenustripNM, menuDat, 0),
        MUIA_Window_UseBottomBorderScroller, TRUE,
        MUIA_Window_UseRightBorderScroller,  TRUE,
        MUIA_Window_EraseArea,               FALSE,
        
        WindowContents, iconcontainer,
    End;

    DoMethod(_app(_presentation(obj)), OM_ADDMEMBER, dirWindow);

    DoMethod(dirWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             dirWindow, 3, MUIM_Set, MUIA_Window_Open, FALSE);

    DoMethod(vert, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime,
             iconcontainer, 3, MUIM_Set, ICA_ScrollToVert, MUIV_TriggerValue);
    DoMethod(horiz, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime,
             iconcontainer, 3, MUIM_Set, ICA_ScrollToHoriz,
             MUIV_TriggerValue);
    DoMethod(dirWindow, MUIM_Notify, MUIA_Window_Activate, TRUE, desktop, 3,
             MUIM_Set, DA_ActiveWindow, dirWindow);
    SetAttrs(dirWindow, MUIA_Window_Open, TRUE, TAG_END);

    return 1;
}

IPTR containerIconObserverNew(Class * cl, Object * obj, struct opSet * msg)
{
    IPTR            retval = 0;
    struct ContainerIconObserverClassData *data;
    struct TagItem *tag;

    retval = DoSuperMethodA(cl, obj, (Msg) msg);
    if (retval)
    {
        obj = (Object *) retval;
        data = INST_DATA(cl, obj);
    }

    return retval;
}

IPTR containerIconObserverSet(Class * cl, Object * obj, struct opSet * msg)
{
    struct ContainerIconObserverClassData *data;
    IPTR            retval = 1;
    struct TagItem *tag,
                   *tstate = msg->ops_AttrList;

    data = (struct ContainerIconObserverClassData *) INST_DATA(cl, obj);

    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
            default:
                retval = DoSuperMethodA(cl, obj, (Msg) msg);
                break;
        }
    }

    return retval;
}

IPTR containerIconObserverGet(Class * cl, Object * obj, struct opGet * msg)
{
    IPTR            retval = 1;
    struct ContainerIconObserverClassData *data;

    data = (struct ContainerIconObserverClassData *) INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
        default:
            retval = DoSuperMethodA(cl, obj, (Msg) msg);
            break;
    }

    return retval;
}

IPTR containerIconObserverDispose(Class * cl, Object * obj, Msg msg)
{
    IPTR            retval;

    retval = DoSuperMethodA(cl, obj, msg);

    return retval;
}

BOOPSI_DISPATCHER(IPTR, containerIconObserverDispatcher, cl, obj, msg)
{
    ULONG           retval = 0;

    switch (msg->MethodID)
    {
        case OM_NEW:
            retval = containerIconObserverNew(cl, obj, (struct opSet *) msg);
            break;
        case OM_SET:
            retval = containerIconObserverSet(cl, obj, (struct opSet *) msg);
            break;
        case OM_GET:
            retval = containerIconObserverGet(cl, obj, (struct opGet *) msg);
            break;
        case OM_DISPOSE:
            retval = containerIconObserverDispose(cl, obj, msg);
            break;
        case IOM_Execute:
            retval = containerIconObserverExecute(cl, obj, msg);
            break;
        default:
            retval = DoSuperMethodA(cl, obj, msg);
            break;
    }

    return retval;
}
