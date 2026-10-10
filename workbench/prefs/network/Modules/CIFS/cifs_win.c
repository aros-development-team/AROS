/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    cifs_win.c - CIFS/SMB mounted-share configuration window class:
      - Subclass of PAWinClass (protocols.c, passed in via the plugin API)
      - MUIM_PAWin_Show  : populate gadgets from a MountedShare
      - MUIM_PAWin_Apply : read gadgets back into a MountedShare

    This file is compiled as part of the cifs.netprefs plugin module.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <libraries/mui.h>
#include <intuition/classes.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/alib.h>
#include <utility/hooks.h>
#include <string.h>
#include <stdio.h>

#include "protocols.h"
#include "prefsdata.h"
#include "locale.h"
#include "cifs_intern.h"

struct MUI_CustomClass *CIFSWinClass = NULL;

struct CIFSWin_Data
{
    Object *cw_deviceObj;
    Object *cw_activeObj;
    Object *cw_hostObj;
    Object *cw_groupObj;
    Object *cw_shareObj;
    Object *cw_userObj;
    Object *cw_passObj;
};

/*---------------------------------------------------------------------------*/
static IPTR CIFSWin__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    Object *device, *active, *host, *group, *share, *user, *pass, *content;

    /* Build the CIFS-specific content group */
    content = (Object *)ColGroup(2),
        GroupFrame,
        Child, (IPTR)Label2(__(MSG_DEVICE)),
        Child, (IPTR)(device = (Object *)StringObject,
            StringFrame,
            MUIA_String_MaxLen, MOUNT_DEVBUFLEN,
            MUIA_CycleChain,    1,
        End),
        Child, (IPTR)HVSpace,
        Child, (IPTR)HGroup,
            Child, (IPTR)(active = MUI_MakeObject(MUIO_Checkmark, NULL)),
            Child, (IPTR)Label2(__(MSG_UP)),
            Child, (IPTR)HVSpace,
        End,
        Child, (IPTR)Label2(__(MSG_HOST_NAME)),
        Child, (IPTR)(host = (Object *)StringObject,
            StringFrame,
            MUIA_String_MaxLen, MOUNT_HOSTBUFLEN,
            MUIA_CycleChain,    1,
        End),
        Child, (IPTR)Label2(__(MSG_WORKGROUP)),
        Child, (IPTR)(group = (Object *)StringObject,
            StringFrame,
            MUIA_String_MaxLen, MOUNT_FIELDBUFLEN,
            MUIA_CycleChain,    1,
        End),
        Child, (IPTR)Label2(__(MSG_SERVICE)),
        Child, (IPTR)(share = (Object *)StringObject,
            StringFrame,
            MUIA_String_MaxLen, MOUNT_FIELDBUFLEN,
            MUIA_CycleChain,    1,
        End),
        Child, (IPTR)Label2(__(MSG_USERNAME)),
        Child, (IPTR)(user = (Object *)StringObject,
            StringFrame,
            MUIA_String_MaxLen, MOUNT_FIELDBUFLEN,
            MUIA_CycleChain,    1,
        End),
        Child, (IPTR)Label2(__(MSG_PASSWORD)),
        Child, (IPTR)(pass = (Object *)StringObject,
            StringFrame,
            MUIA_String_MaxLen, MOUNT_FIELDBUFLEN,
            MUIA_String_Secret, TRUE,
            MUIA_CycleChain,    1,
        End),
    End;

    if (!content)
        return 0;

    /* Pass content + type name to the PAWin base class */
    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
        MUIA_PAWin_ProtocolName, (IPTR)_(MSG_SERVICETYPE_CIFS),
        MUIA_PAWin_TypeLabel,    (IPTR)_(MSG_SERVICE_TYPES),
        MUIA_PAWin_Content,      (IPTR)content,
        MUIA_Window_Title,       __(MSG_SERVERWINDOW_TITLE),
        MUIA_Window_ID,          MAKE_ID('S','H','R','E'),
        TAG_MORE, (IPTR)msg->ops_AttrList);

    if (!obj)
        return 0;

    struct CIFSWin_Data *data = INST_DATA(cl, obj);
    data->cw_deviceObj = device;
    data->cw_activeObj = active;
    data->cw_hostObj   = host;
    data->cw_groupObj  = group;
    data->cw_shareObj  = share;
    data->cw_userObj   = user;
    data->cw_passObj   = pass;

    return (IPTR)obj;
}

/*---------------------------------------------------------------------------*/
static IPTR CIFSWin__MUIM_PAWin_Show(Class *cl, Object *obj,
    struct MUIP_PAWin_Show *msg)
{
    struct CIFSWin_Data *data = INST_DATA(cl, obj);
    struct MountedShare *ms = (struct MountedShare *)msg->pa;

    SET(data->cw_deviceObj, MUIA_String_Contents, (IPTR)ms->ms_device);
    SET(data->cw_activeObj, MUIA_Selected, ms->ms_active ? 1 : 0);
    SET(data->cw_hostObj,   MUIA_String_Contents, (IPTR)ms->ms_host);
    SET(data->cw_groupObj,  MUIA_String_Contents, (IPTR)ms->ms_extra);
    SET(data->cw_shareObj,  MUIA_String_Contents, (IPTR)ms->ms_volume);
    SET(data->cw_userObj,   MUIA_String_Contents, (IPTR)ms->ms_user);
    SET(data->cw_passObj,   MUIA_String_Contents, (IPTR)ms->ms_secret);
    return 0;
}

/*---------------------------------------------------------------------------*/
static IPTR CIFSWin__MUIM_PAWin_Apply(Class *cl, Object *obj,
    struct MUIP_PAWin_Apply *msg)
{
    struct CIFSWin_Data *data = INST_DATA(cl, obj);
    struct MountedShare *ms = (struct MountedShare *)msg->pa;

    strlcpy(ms->ms_device,
            (STRPTR)XGET(data->cw_deviceObj, MUIA_String_Contents),
            sizeof(ms->ms_device));
    ms->ms_active = XGET(data->cw_activeObj, MUIA_Selected) ? TRUE : FALSE;
    strlcpy(ms->ms_host,
            (STRPTR)XGET(data->cw_hostObj, MUIA_String_Contents),
            sizeof(ms->ms_host));
    strlcpy(ms->ms_extra,
            (STRPTR)XGET(data->cw_groupObj, MUIA_String_Contents),
            sizeof(ms->ms_extra));
    strlcpy(ms->ms_volume,
            (STRPTR)XGET(data->cw_shareObj, MUIA_String_Contents),
            sizeof(ms->ms_volume));
    strlcpy(ms->ms_user,
            (STRPTR)XGET(data->cw_userObj, MUIA_String_Contents),
            sizeof(ms->ms_user));
    strlcpy(ms->ms_secret,
            (STRPTR)XGET(data->cw_passObj, MUIA_String_Contents),
            sizeof(ms->ms_secret));
    return 0;
}

/*---------------------------------------------------------------------------*/
BOOPSI_DISPATCHER(IPTR, CIFSWin_Dispatch, cl, obj, msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW:
            return CIFSWin__OM_NEW(cl, obj, (struct opSet *)msg);
        case MUIM_PAWin_Show:
            return CIFSWin__MUIM_PAWin_Show(cl, obj,
                       (struct MUIP_PAWin_Show *)msg);
        case MUIM_PAWin_Apply:
            return CIFSWin__MUIM_PAWin_Apply(cl, obj,
                       (struct MUIP_PAWin_Apply *)msg);
        default:
            return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

/*---------------------------------------------------------------------------*/
BOOL CIFSWin_InitClass(struct MUI_CustomClass *PAWinCl)
{
    if (CIFSWinClass)
        return TRUE;
    if (!PAWinCl)
        return FALSE;
    CIFSWinClass = MUI_CreateCustomClass(NULL, NULL, PAWinCl,
                       sizeof(struct CIFSWin_Data), CIFSWin_Dispatch);
    return CIFSWinClass != NULL;
}

void CIFSWin_FreeClass(void)
{
    if (CIFSWinClass)
    {
        MUI_DeleteCustomClass(CIFSWinClass);
        CIFSWinClass = NULL;
    }
}
