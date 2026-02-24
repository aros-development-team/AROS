/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    protocols.c - shared ProtocolAddress infrastructure:
      - MUI list hooks (construct / destruct / display)
      - ProtoAddr_FromInterface / ProtoAddr_ToInterface conversions
      - PAWinClass: common protocol-address configuration window base class
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <libraries/mui.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/alib.h>
#include <utility/hooks.h>
#include <string.h>

#include "protocols.h"
#include "locale.h"

#define USE_NET_PROTOICON_COLORS
#define USE_NET_PROTOICON_BODY
#include "net_protoicon.h"

/*--- MUI list hook implementations -----------------------------------------*/

AROS_UFH3S(APTR, protoConstructFunc,
    AROS_UFHA(struct Hook *,          hook,  A0),
    AROS_UFHA(APTR,                   pool,  A2),
    AROS_UFHA(struct ProtocolAddress *, entry, A1))
{
    AROS_USERFUNC_INIT

    struct ProtocolAddress *new;
    if ((new = AllocPooled(pool, sizeof(*new))))
        *new = *entry;
    return new;

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, protoDestructFunc,
    AROS_UFHA(struct Hook *,          hook,  A0),
    AROS_UFHA(APTR,                   pool,  A2),
    AROS_UFHA(struct ProtocolAddress *, entry, A1))
{
    AROS_USERFUNC_INIT

    FreePooled(pool, entry, sizeof(struct ProtocolAddress));

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(LONG, protoDisplayFunc,
    AROS_UFHA(struct Hook *,          hook,  A0),
    AROS_UFHA(char **,                array, A2),
    AROS_UFHA(struct ProtocolAddress *, entry, A1))
{
    AROS_USERFUNC_INIT

    if (entry)
    {
        /*
         * Two static buffers are safe here because the display hook is called
         * once per entry before MUI renders the row; no re-entrancy concern.
         */
        static char famBuf[8];
        static char addrBuf[IP6BUFLEN + 4];

        if (entry->pa_family == PROTO_FAMILY_IPV4)
            strcpy(famBuf, "IPv4");
        else
            strcpy(famBuf, "IPv6");

        switch (entry->pa_mode)
        {
            case IP_MODE_DHCP:
                strcpy(addrBuf, _(MSG_IP_MODE_DHCP));
                break;
            case IP_MODE_AUTO:
                strcpy(addrBuf,
                    (entry->pa_family == PROTO_FAMILY_IPV6)
                        ? _(MSG_IP6_MODE_AUTO)
                        : _(MSG_IP_MODE_AUTO));
                break;
            default:
                if (entry->pa_addr[0])
                    strcpy(addrBuf, entry->pa_addr);
                else
                    strcpy(addrBuf, _(MSG_IP_MODE_MANUAL));
                break;
        }

        *array++ = famBuf;
        *array   = addrBuf;
    }
    else
    {
        /* Column header row */
        *array++ = (STRPTR)"Protocol";
        *array   = (STRPTR)_(MSG_IP);
    }

    return 0;

    AROS_USERFUNC_EXIT
}

struct Hook proto_constructHook = { {0}, (HOOKFUNC)protoConstructFunc, NULL, NULL };
struct Hook proto_destructHook  = { {0}, (HOOKFUNC)protoDestructFunc,  NULL, NULL };
struct Hook proto_displayHook   = { {0}, (HOOKFUNC)protoDisplayFunc,   NULL, NULL };

/*--- Conversion functions --------------------------------------------------*/

void ProtoAddr_FromInterface(struct ProtocolAddress *pa,
                             struct Interface *iface,
                             enum ProtocolFamily fam)
{
    pa->pa_family = fam;

    if (fam == PROTO_FAMILY_IPV4)
    {
        pa->pa_mode = GetIPMode(iface);
        strncpy(pa->pa_addr, GetIP(iface),   sizeof(pa->pa_addr) - 1);
        strncpy(pa->pa_mask, GetMask(iface), sizeof(pa->pa_mask) - 1);
        pa->pa_prefix    = 0;
        strncpy(pa->pa_gate, GetGate(iface), sizeof(pa->pa_gate) - 1);
    }
    else
    {
        pa->pa_mode = GetIP6Mode(iface);
        strncpy(pa->pa_addr, GetIP6(iface),   sizeof(pa->pa_addr) - 1);
        pa->pa_mask[0] = '\0';
        pa->pa_prefix  = GetIP6Prefix(iface);
        strncpy(pa->pa_gate, GetGate6(iface), sizeof(pa->pa_gate) - 1);
    }

    /* Guarantee NUL termination regardless of source length */
    pa->pa_addr[sizeof(pa->pa_addr) - 1] = '\0';
    pa->pa_mask[sizeof(pa->pa_mask) - 1] = '\0';
    pa->pa_gate[sizeof(pa->pa_gate) - 1] = '\0';
}

void ProtoAddr_ToInterface(struct Interface *iface,
                           struct ProtocolAddress *ipv4,
                           struct ProtocolAddress *ipv6)
{
    /* IPv4 */
    SetIPMode(iface, ipv4->pa_mode);
    SetIP(iface,     ipv4->pa_addr);
    SetMask(iface,   ipv4->pa_mask);
    SetGate(iface,   ipv4->pa_gate);

    /* IPv6 */
    SetIP6Mode(iface,   ipv6->pa_mode);
    SetIP6(iface,       ipv6->pa_addr);
    SetIP6Prefix(iface, ipv6->pa_prefix);
    SetGate6(iface,     ipv6->pa_gate);
}

/*===========================================================================
 * PAWinClass - common protocol-address configuration window base class.
 * Subclassed by Net4WinClass (net4.c) and Net6WinClass (net6.c).
 *
 * Tags accepted at OM_NEW:
 *   MUIA_PAWin_ProtocolName  (STRPTR)  - shown as "Address Protocol: <name>"
 *   MUIA_PAWin_Content       (Object*) - gadget group from subclass
 *   Any MUIA_Window_* tag    - passed through to Window.mui via TAG_MORE
 *
 * Read-only attributes:
 *   MUIA_PAWin_UseButton     (Object*) - the Use ImageButton
 *   MUIA_PAWin_CancelButton  (Object*) - the Cancel ImageButton
 *
 * Methods overridden by subclasses:
 *   MUIM_PAWin_Show(pa)        - populate gadgets from ProtocolAddress
 *   MUIM_PAWin_Apply(pa)       - read gadgets back into ProtocolAddress
 *   MUIM_PAWin_ModeChanged(m)  - handle mode-cycle notification
 *===========================================================================*/

struct MUI_CustomClass *PAWinClass = NULL;

struct PAWin_Data
{
    Object *pwd_UseButton;
    Object *pwd_CancelButton;
};

static IPTR PAWin__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    STRPTR   protoName = (STRPTR)GetTagData(MUIA_PAWin_ProtocolName,
                                            (IPTR)"", msg->ops_AttrList);
    Object  *content   = (Object *)GetTagData(MUIA_PAWin_Content,
                                              (IPTR)NULL, msg->ops_AttrList);
    Object  *useBtn, *cancelBtn;

    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
        MUIA_Window_Title,       (IPTR)_(MSG_CFG_ADDR),
        MUIA_Window_CloseGadget, FALSE,
        WindowContents, (IPTR)VGroup,
            GroupFrame,
            Child, (IPTR)HGroup,
                Child, (IPTR)HVSpace,
                Child, (IPTR)BodychunkObject,
                    MUIA_Bitmap_SourceColors,   (IPTR)net_protoicon_colors,
                    MUIA_FixWidth,              NET_PROTOICON_WIDTH,
                    MUIA_FixHeight,             NET_PROTOICON_HEIGHT,
                    MUIA_Bitmap_Width,          NET_PROTOICON_WIDTH,
                    MUIA_Bitmap_Height,         NET_PROTOICON_HEIGHT,
                    MUIA_Bodychunk_Depth,       NET_PROTOICON_DEPTH,
                    MUIA_Bodychunk_Body,        (IPTR)net_protoicon_body,
                    MUIA_Bodychunk_Compression, NET_PROTOICON_COMPRESSION,
                    MUIA_Bodychunk_Masking,     NET_PROTOICON_MASKING,
                    MUIA_Bitmap_Transparent,    NET_PROTOICON_TRANSPARENT,
                End,
                Child, (IPTR)HVSpace,
            End,
            Child, (IPTR)ColGroup(2),
                Child, (IPTR)Label2(__(MSG_PROTO_LABEL)),
                Child, (IPTR)TextObject,
                    MUIA_Text_Contents, (IPTR)protoName,
                End,
            End,
            Child, (IPTR)(content != NULL ? content : HVSpace),
            Child, (IPTR)HGroup,
                Child, (IPTR)(useBtn    = ImageButton(_(MSG_BUTTON_USE),
                                "THEME:Images/Gadgets/Use")),
                Child, (IPTR)(cancelBtn = ImageButton(_(MSG_BUTTON_CANCEL),
                                "THEME:Images/Gadgets/Cancel")),
            End,
        End,
        TAG_MORE, (IPTR)msg->ops_AttrList);

    if (!obj)
        return 0;

    struct PAWin_Data *data = INST_DATA(cl, obj);
    data->pwd_UseButton    = useBtn;
    data->pwd_CancelButton = cancelBtn;

    /* Cancel button closes this window */
    DoMethod(cancelBtn, MUIM_Notify, MUIA_Pressed, FALSE,
             obj, 3, MUIM_Set, MUIA_Window_Open, FALSE);

    return (IPTR)obj;
}

static IPTR PAWin__OM_GET(Class *cl, Object *obj, struct opGet *msg)
{
    struct PAWin_Data *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
        case MUIA_PAWin_UseButton:
            *msg->opg_Storage = (IPTR)data->pwd_UseButton;
            return TRUE;
        case MUIA_PAWin_CancelButton:
            *msg->opg_Storage = (IPTR)data->pwd_CancelButton;
            return TRUE;
    }
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/* Base implementations — subclasses override these */
static IPTR PAWin__MUIM_PAWin_Show(Class *cl, Object *obj,
    struct MUIP_PAWin_Show *msg)
{
    (void)msg;
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

static IPTR PAWin__MUIM_PAWin_Apply(Class *cl, Object *obj,
    struct MUIP_PAWin_Apply *msg)
{
    (void)msg;
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

static IPTR PAWin__MUIM_PAWin_ModeChanged(Class *cl, Object *obj,
    struct MUIP_PAWin_ModeChanged *msg)
{
    (void)msg;
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

static IPTR PAWin_Dispatch(Class *cl, Object *obj, Msg msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW:
            return PAWin__OM_NEW(cl, obj, (struct opSet *)msg);
        case OM_GET:
            return PAWin__OM_GET(cl, obj, (struct opGet *)msg);
        case MUIM_PAWin_Show:
            return PAWin__MUIM_PAWin_Show(cl, obj,
                       (struct MUIP_PAWin_Show *)msg);
        case MUIM_PAWin_Apply:
            return PAWin__MUIM_PAWin_Apply(cl, obj,
                       (struct MUIP_PAWin_Apply *)msg);
        case MUIM_PAWin_ModeChanged:
            return PAWin__MUIM_PAWin_ModeChanged(cl, obj,
                       (struct MUIP_PAWin_ModeChanged *)msg);
        default:
            return DoSuperMethodA(cl, obj, msg);
    }
}

BOOL PAWin_InitClass(void)
{
    if (PAWinClass)
        return TRUE;
    PAWinClass = MUI_CreateCustomClass(NULL, MUIC_Window, NULL,
                     sizeof(struct PAWin_Data), PAWin_Dispatch);
    return PAWinClass != NULL;
}

void PAWin_FreeClass(void)
{
    if (PAWinClass)
    {
        MUI_DeleteCustomClass(PAWinClass);
        PAWinClass = NULL;
    }
}
