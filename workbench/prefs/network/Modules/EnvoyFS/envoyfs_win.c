/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    envoyfs_win.c - Envoy Filesystem mounted-share configuration window class:
      - Subclass of PAWinClass (protocols.c, passed in via the plugin API)
      - MUIM_PAWin_Show  : populate gadgets from a MountedShare
      - MUIM_PAWin_Apply : read gadgets back into a MountedShare
      - "Select..." (host)   : envoy.library HostRequest     [needs EnvoyBase]
      - "List..."   (export) : Imp_ListExports host query    [needs nipc+services]

    Degradation with missing Envoy libraries: the dependent buttons are
    disabled, hand-entry keeps working.  The password gadget never shows the
    stored "$hash"; a non-empty entry is hashed on Apply (needs accounts -
    without it the gadget is disabled and the stored hash is preserved).

    This file is compiled as part of the envoyfs.netprefs plugin module.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <libraries/mui.h>
#include <intuition/classes.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/envoy.h>
#include <proto/alib.h>
#include <utility/hooks.h>
#include <envoy/envoy.h>
#include <string.h>
#include <stdio.h>

#include "protocols.h"
#include "prefsdata.h"
#include "locale.h"
#include "envoyfs_intern.h"
#include "envoyfs_imports.h"

struct MUI_CustomClass *EFSWinClass = NULL;

/* Private window methods (button actions) */
#define MUIB_EFSWin                 (TAG_USER | 0x11200000)
#define MUIM_EFSWin_SelectHost      (MUIB_EFSWin | 0x0001)
#define MUIM_EFSWin_ListExports     (MUIB_EFSWin | 0x0002)

/* At most this many exports fit a MUI_Request button row */
#define EFS_MAXCHOICES  8

struct EFSWin_Data
{
    Object *ew_deviceObj;
    Object *ew_activeObj;
    Object *ew_hostObj;
    Object *ew_selectObj;
    Object *ew_exportObj;
    Object *ew_listObj;
    Object *ew_userObj;
    Object *ew_passObj;
};

/*---------------------------------------------------------------------------*/
static IPTR EFSWin__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    Object *device, *active, *host, *selectBtn, *export, *listBtn,
           *user, *pass, *content;

    /* Build the Envoy-specific content group */
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
        Child, (IPTR)HGroup,
            Child, (IPTR)(host = (Object *)StringObject,
                StringFrame,
                MUIA_String_MaxLen, MOUNT_HOSTBUFLEN,
                MUIA_CycleChain,    1,
            End),
            Child, (IPTR)(selectBtn = SimpleButton(_(MSG_ENVOY_SELECT_HOST))),
        End,
        Child, (IPTR)Label2(__(MSG_ENVOY_EXPORT)),
        Child, (IPTR)HGroup,
            Child, (IPTR)(export = (Object *)StringObject,
                StringFrame,
                MUIA_String_MaxLen, MOUNT_FIELDBUFLEN,
                MUIA_CycleChain,    1,
            End),
            Child, (IPTR)(listBtn = SimpleButton(_(MSG_ENVOY_LIST_EXPORTS))),
        End,
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
            MUIA_ShortHelp,     (IPTR)_(MSG_ENVOY_KEEPPASS_HELP),
        End),
    End;

    if (!content)
        return 0;

    /* Pass content + type name to the PAWin base class */
    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
        MUIA_PAWin_ProtocolName, (IPTR)_(MSG_SERVICETYPE_ENVOY),
        MUIA_PAWin_TypeLabel,    (IPTR)_(MSG_SERVICE_TYPES),
        MUIA_PAWin_Content,      (IPTR)content,
        MUIA_Window_Title,       __(MSG_SERVERWINDOW_TITLE),
        MUIA_Window_ID,          MAKE_ID('E','F','S','W'),
        TAG_MORE, (IPTR)msg->ops_AttrList);

    if (!obj)
        return 0;

    struct EFSWin_Data *data = INST_DATA(cl, obj);
    data->ew_deviceObj = device;
    data->ew_activeObj = active;
    data->ew_hostObj   = host;
    data->ew_selectObj = selectBtn;
    data->ew_exportObj = export;
    data->ew_listObj   = listBtn;
    data->ew_userObj   = user;
    data->ew_passObj   = pass;

    /* Degrade: disable what the missing Envoy libraries cannot serve */
    if (!EnvoyBase)
        SET(selectBtn, MUIA_Disabled, TRUE);
    if (!NIPCBase || !ServicesBase)
        SET(listBtn, MUIA_Disabled, TRUE);
    if (!AccountsBase)
    {
        SET(pass, MUIA_Disabled, TRUE);
        SET(pass, MUIA_ShortHelp, (IPTR)_(MSG_ENVOY_NOACCOUNTS_HELP));
    }

    DoMethod(selectBtn, MUIM_Notify, MUIA_Pressed, FALSE,
             obj, 1, MUIM_EFSWin_SelectHost);
    DoMethod(listBtn, MUIM_Notify, MUIA_Pressed, FALSE,
             obj, 1, MUIM_EFSWin_ListExports);

    return (IPTR)obj;
}

/*---------------------------------------------------------------------------*/
static IPTR EFSWin__MUIM_PAWin_Show(Class *cl, Object *obj,
    struct MUIP_PAWin_Show *msg)
{
    struct EFSWin_Data *data = INST_DATA(cl, obj);
    struct MountedShare *ms = (struct MountedShare *)msg->pa;

    SET(data->ew_deviceObj, MUIA_String_Contents, (IPTR)ms->ms_device);
    SET(data->ew_activeObj, MUIA_Selected, ms->ms_active ? 1 : 0);
    SET(data->ew_hostObj,   MUIA_String_Contents, (IPTR)ms->ms_host);
    SET(data->ew_exportObj, MUIA_String_Contents, (IPTR)ms->ms_volume);
    SET(data->ew_userObj,   MUIA_String_Contents, (IPTR)ms->ms_user);
    /* never show the stored hash; empty = "keep the stored login" */
    SET(data->ew_passObj,   MUIA_String_Contents, (IPTR)"");
    return 0;
}

/*---------------------------------------------------------------------------*/
static IPTR EFSWin__MUIM_PAWin_Apply(Class *cl, Object *obj,
    struct MUIP_PAWin_Apply *msg)
{
    struct EFSWin_Data *data = INST_DATA(cl, obj);
    struct MountedShare *ms = (struct MountedShare *)msg->pa;
    CONST_STRPTR pw;

    strlcpy(ms->ms_device,
            (STRPTR)XGET(data->ew_deviceObj, MUIA_String_Contents),
            sizeof(ms->ms_device));
    ms->ms_active = XGET(data->ew_activeObj, MUIA_Selected) ? TRUE : FALSE;
    strlcpy(ms->ms_host,
            (STRPTR)XGET(data->ew_hostObj, MUIA_String_Contents),
            sizeof(ms->ms_host));
    strlcpy(ms->ms_volume,
            (STRPTR)XGET(data->ew_exportObj, MUIA_String_Contents),
            sizeof(ms->ms_volume));
    strlcpy(ms->ms_user,
            (STRPTR)XGET(data->ew_userObj, MUIA_String_Contents),
            sizeof(ms->ms_user));

    /* A non-empty password is hashed into the new secret; an empty one keeps
     * the stored secret (ms arrives as a copy of the current entry). */
    pw = (CONST_STRPTR)XGET(data->ew_passObj, MUIA_String_Contents);
    if (pw != NULL && pw[0] != '\0')
    {
        struct ImpLogin login;

        if (Imp_MakeLogin(&login, ms->ms_user, pw))
            strlcpy(ms->ms_secret, login.Hash, sizeof(ms->ms_secret));
        SET(data->ew_passObj, MUIA_String_Contents, (IPTR)"");
    }

    /* An empty device name is derived from host + export */
    if (ms->ms_device[0] == '\0' && ms->ms_host[0] != '\0'
        && ms->ms_volume[0] != '\0')
    {
        Imp_DeviceName(ms->ms_device, sizeof(ms->ms_device),
                       ms->ms_host, ms->ms_volume);
        SET(data->ew_deviceObj, MUIA_String_Contents, (IPTR)ms->ms_device);
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
static IPTR EFSWin__MUIM_EFSWin_SelectHost(Class *cl, Object *obj, Msg msg)
{
    struct EFSWin_Data *data = INST_DATA(cl, obj);
    char buf[MOUNT_HOSTBUFLEN];

    if (!EnvoyBase)
        return 0;

    buf[0] = '\0';
    if (HostRequest(HREQ_Buffer,   (IPTR)buf,
                    HREQ_BuffSize, sizeof(buf),
                    HREQ_Title,    (IPTR)_(MSG_HOST_NAME),
                    TAG_DONE) && buf[0] != '\0')
    {
        SET(data->ew_hostObj, MUIA_String_Contents, (IPTR)buf);
    }
    return 0;
}

/*---------------------------------------------------------------------------*/
static IPTR EFSWin__MUIM_EFSWin_ListExports(Class *cl, Object *obj, Msg msg)
{
    struct EFSWin_Data *data = INST_DATA(cl, obj);
    CONST_STRPTR host, user, pw;
    struct List exports;
    struct Node *n;
    ULONG err;

    host = (CONST_STRPTR)XGET(data->ew_hostObj, MUIA_String_Contents);
    if (host == NULL || host[0] == '\0')
        return 0;
    user = (CONST_STRPTR)XGET(data->ew_userObj, MUIA_String_Contents);
    pw   = (CONST_STRPTR)XGET(data->ew_passObj, MUIA_String_Contents);

    NEWLIST(&exports);
    err = Imp_ListExports(host, user, pw, &exports);
    if (err != 0)
    {
        char buf[80];

        MUI_Request(_app(obj), obj, 0,
                    _(MSG_ENVOY_LIST_EXPORTS), _(MSG_BUTTON_CANCEL),
                    "%s", (IPTR)Imp_ErrorText(err, buf, sizeof(buf)));
    }
    else
    {
        /* Offer the first few exports as requester choices */
        TEXT gadgets[EFS_MAXCHOICES * IMP_NAMELEN + 16];
        LONG count = 0, res;

        gadgets[0] = '\0';
        ForeachNode(&exports, n)
        {
            if (count >= EFS_MAXCHOICES)
                break;
            strlcat(gadgets, n->ln_Name, sizeof(gadgets));
            strlcat(gadgets, "|", sizeof(gadgets));
            count++;
        }
        strlcat(gadgets, _(MSG_BUTTON_CANCEL), sizeof(gadgets));

        if (count > 0)
        {
            res = MUI_Request(_app(obj), obj, 0,
                              _(MSG_ENVOY_LIST_EXPORTS), gadgets,
                              "%s", (IPTR)host);
            if (res > 0)
            {
                LONG i = 1;

                ForeachNode(&exports, n)
                {
                    if (i++ == res)
                    {
                        SET(data->ew_exportObj, MUIA_String_Contents,
                            (IPTR)n->ln_Name);
                        break;
                    }
                }
            }
        }
    }
    Imp_FreeList(&exports);
    return 0;
}

/*---------------------------------------------------------------------------*/
BOOPSI_DISPATCHER(IPTR, EFSWin_Dispatch, cl, obj, msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW:
            return EFSWin__OM_NEW(cl, obj, (struct opSet *)msg);
        case MUIM_PAWin_Show:
            return EFSWin__MUIM_PAWin_Show(cl, obj,
                       (struct MUIP_PAWin_Show *)msg);
        case MUIM_PAWin_Apply:
            return EFSWin__MUIM_PAWin_Apply(cl, obj,
                       (struct MUIP_PAWin_Apply *)msg);
        case MUIM_EFSWin_SelectHost:
            return EFSWin__MUIM_EFSWin_SelectHost(cl, obj, msg);
        case MUIM_EFSWin_ListExports:
            return EFSWin__MUIM_EFSWin_ListExports(cl, obj, msg);
        default:
            return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

/*---------------------------------------------------------------------------*/
BOOL EFSWin_InitClass(struct MUI_CustomClass *PAWinCl)
{
    if (EFSWinClass)
        return TRUE;
    if (!PAWinCl)
        return FALSE;
    EFSWinClass = MUI_CreateCustomClass(NULL, NULL, PAWinCl,
                      sizeof(struct EFSWin_Data), EFSWin_Dispatch);
    return EFSWinClass != NULL;
}

void EFSWin_FreeClass(void)
{
    if (EFSWinClass)
    {
        MUI_DeleteCustomClass(EFSWinClass);
        EFSWinClass = NULL;
    }
}
