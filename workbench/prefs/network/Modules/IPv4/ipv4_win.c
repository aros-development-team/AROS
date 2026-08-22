/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    net4.c - IPv4 protocol-address configuration window class (Net4WinClass):
      - Subclass of PAWinClass (protocols.c, passed in via the plugin API)
      - MUIM_PAWin_Show        : populate gadgets from a ProtocolAddress
      - MUIM_PAWin_Apply       : read gadgets back into a ProtocolAddress
      - MUIM_PAWin_ModeChanged : enable/disable addr+mask for manual mode
      - Net4_WriteTokens       : write IP= / NETMASK= / GW= to a FILE

    This file is compiled as part of the net4.netprefs plugin module.
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

struct MUI_CustomClass *Net4WinClass = NULL;

struct Net4Win_Data
{
    Object *n4_modeObj;
    Object *n4_addrObj;
    Object *n4_maskObj;
    Object *n4_gateObj;
};

static CONST_STRPTR IPv4ModeCycle[] = { NULL, NULL, NULL, NULL };
static const TEXT   ipv4_max_str[]  = "255.255.255.255 ";

/*---------------------------------------------------------------------------*/
static IPTR Net4Win__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    Object *mode, *addr, *mask, *gate, *content;

    IPv4ModeCycle[0] = _(MSG_IP_MODE_DHCP);
    IPv4ModeCycle[1] = _(MSG_IP_MODE_AUTO);
    IPv4ModeCycle[2] = _(MSG_IP_MODE_MANUAL);

    /* Build the IPv4-specific content group */
    content = (Object *)ColGroup(2),
        GroupFrame,
        Child, (IPTR)Label2(__(MSG_ADDR_MODE)),
        Child, (IPTR)(mode = (Object *)CycleObject,
            MUIA_Cycle_Entries, (IPTR)IPv4ModeCycle,
        End),
        Child, (IPTR)Label2(__(MSG_IP)),
        Child, (IPTR)(addr = (Object *)StringObject,
            StringFrame,
            MUIA_String_Accept,  (IPTR)IPCHARS,
            MUIA_CycleChain,     1,
            MUIA_FixWidthTxt,    (IPTR)ipv4_max_str,
        End),
        Child, (IPTR)Label2(__(MSG_MASK)),
        Child, (IPTR)(mask = (Object *)StringObject,
            StringFrame,
            MUIA_String_Accept,  (IPTR)IPCHARS,
            MUIA_CycleChain,     1,
            MUIA_FixWidthTxt,    (IPTR)ipv4_max_str,
        End),
        Child, (IPTR)Label2(__(MSG_GATE)),
        Child, (IPTR)(gate = (Object *)StringObject,
            StringFrame,
            MUIA_String_Accept,  (IPTR)IPCHARS,
            MUIA_CycleChain,     1,
            MUIA_FixWidthTxt,    (IPTR)ipv4_max_str,
        End),
    End;

    if (!content)
        return 0;

    /* Pass content + protocol name to the PAWin base class */
    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
        MUIA_PAWin_ProtocolName, (IPTR)"IPv4",
        MUIA_PAWin_Content,      (IPTR)content,
        MUIA_Window_ID,          MAKE_ID('I','P','4','W'),
        TAG_MORE, (IPTR)msg->ops_AttrList);

    if (!obj)
        return 0;

    struct Net4Win_Data *data = INST_DATA(cl, obj);
    data->n4_modeObj = mode;
    data->n4_addrObj = addr;
    data->n4_maskObj = mask;
    data->n4_gateObj = gate;

    /* Mode cycle notifies the window to update gadget states */
    DoMethod(mode, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             obj, 2, MUIM_PAWin_ModeChanged, MUIV_TriggerValue);

    return (IPTR)obj;
}

/*---------------------------------------------------------------------------*/
static IPTR Net4Win__MUIM_PAWin_Show(Class *cl, Object *obj,
    struct MUIP_PAWin_Show *msg)
{
    struct Net4Win_Data *data = INST_DATA(cl, obj);
    struct ProtocolAddress *pa = msg->pa;

    SET(data->n4_modeObj, MUIA_Cycle_Active, (IPTR)pa->pa_mode);

    if (pa->pa_mode == IP_MODE_MANUAL)
    {
        SET(data->n4_addrObj, MUIA_Disabled, FALSE);
        SET(data->n4_addrObj, MUIA_String_Contents, pa->pa_addr);
        SET(data->n4_maskObj, MUIA_Disabled, FALSE);
        SET(data->n4_maskObj, MUIA_String_Contents, pa->pa_mask);
    }
    else
    {
        SET(data->n4_addrObj, MUIA_Disabled, TRUE);
        SET(data->n4_addrObj, MUIA_String_Contents, "");
        SET(data->n4_maskObj, MUIA_Disabled, TRUE);
        SET(data->n4_maskObj, MUIA_String_Contents, "");
    }
    SET(data->n4_gateObj, MUIA_String_Contents, pa->pa_gate);
    return 0;
}

/*---------------------------------------------------------------------------*/
static IPTR Net4Win__MUIM_PAWin_Apply(Class *cl, Object *obj,
    struct MUIP_PAWin_Apply *msg)
{
    struct Net4Win_Data *data = INST_DATA(cl, obj);
    struct ProtocolAddress *pa = msg->pa;

    pa->pa_mode = (enum IPMode)XGET(data->n4_modeObj, MUIA_Cycle_Active);

    if (pa->pa_mode == IP_MODE_MANUAL)
    {
        strncpy(pa->pa_addr,
                (STRPTR)XGET(data->n4_addrObj, MUIA_String_Contents),
                sizeof(pa->pa_addr) - 1);
        strncpy(pa->pa_mask,
                (STRPTR)XGET(data->n4_maskObj, MUIA_String_Contents),
                sizeof(pa->pa_mask) - 1);
    }
    else
    {
        pa->pa_addr[0] = '\0';
        pa->pa_mask[0] = '\0';
    }
    strncpy(pa->pa_gate,
            (STRPTR)XGET(data->n4_gateObj, MUIA_String_Contents),
            sizeof(pa->pa_gate) - 1);

    pa->pa_addr[sizeof(pa->pa_addr) - 1] = '\0';
    pa->pa_mask[sizeof(pa->pa_mask) - 1] = '\0';
    pa->pa_gate[sizeof(pa->pa_gate) - 1] = '\0';
    return 0;
}

/*---------------------------------------------------------------------------*/
static IPTR Net4Win__MUIM_PAWin_ModeChanged(Class *cl, Object *obj,
    struct MUIP_PAWin_ModeChanged *msg)
{
    struct Net4Win_Data *data = INST_DATA(cl, obj);
    ULONG newMode = msg->newMode;
    STRPTR str = NULL;

    if (newMode == IP_MODE_MANUAL)
    {
        SET(data->n4_addrObj, MUIA_Disabled, FALSE);
        SET(data->n4_maskObj, MUIA_Disabled, FALSE);
    }
    else
    {
        GET(data->n4_addrObj, MUIA_String_Contents, &str);
        if (str && str[0])
        {
            /* save current addr before clearing */
        }
        SET(data->n4_addrObj, MUIA_Disabled, TRUE);
        SET(data->n4_addrObj, MUIA_String_Contents, "");
        GET(data->n4_maskObj, MUIA_String_Contents, &str);
        SET(data->n4_maskObj, MUIA_Disabled, TRUE);
        SET(data->n4_maskObj, MUIA_String_Contents, "");
    }
    return 0;
}

/*---------------------------------------------------------------------------*/
BOOPSI_DISPATCHER(IPTR, Net4Win_Dispatch, cl, obj, msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW:
            return Net4Win__OM_NEW(cl, obj, (struct opSet *)msg);
        case MUIM_PAWin_Show:
            return Net4Win__MUIM_PAWin_Show(cl, obj,
                       (struct MUIP_PAWin_Show *)msg);
        case MUIM_PAWin_Apply:
            return Net4Win__MUIM_PAWin_Apply(cl, obj,
                       (struct MUIP_PAWin_Apply *)msg);
        case MUIM_PAWin_ModeChanged:
            return Net4Win__MUIM_PAWin_ModeChanged(cl, obj,
                       (struct MUIP_PAWin_ModeChanged *)msg);
        default:
            return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

/*---------------------------------------------------------------------------*/
BOOL Net4Win_InitClass(struct MUI_CustomClass *PAWinCl)
{
    if (Net4WinClass)
        return TRUE;
    if (!PAWinCl)
        return FALSE;
    Net4WinClass = MUI_CreateCustomClass(NULL, NULL, PAWinCl,
                       sizeof(struct Net4Win_Data), Net4Win_Dispatch);
    return Net4WinClass != NULL;
}

void Net4Win_FreeClass(void)
{
    if (Net4WinClass)
    {
        MUI_DeleteCustomClass(Net4WinClass);
        Net4WinClass = NULL;
    }
}

/*---------------------------------------------------------------------------*/
void Net4_WriteTokens(FILE *f, struct ProtocolAddress *pa)
{
    switch (pa->pa_mode)
    {
        case IP_MODE_DHCP:
            fprintf(f, "IP=DHCP ");
            break;
        case IP_MODE_AUTO:
            fprintf(f, "IP=AUTO ");
            break;
        case IP_MODE_MANUAL:
            if (pa->pa_addr[0])
                fprintf(f, "IP=%s NETMASK=%s ",
                    (const char *)pa->pa_addr,
                    pa->pa_mask[0] ? (const char *)pa->pa_mask : "255.255.255.0");
            break;
        default:
            break;
    }
    if (pa->pa_gate[0])
        fprintf(f, "GW=%s ", pa->pa_gate);
}

/* Find (or create and append) this plugin's address node on an interface's
 * protocol list, tagged with our id in ln_Type. */
static struct ProtocolAddress *net4_node(struct List *list, UBYTE id)
{
    struct Node *n;

    for (n = list->lh_Head; n->ln_Succ; n = n->ln_Succ)
        if (n->ln_Type == id)
            return (struct ProtocolAddress *)n;

    struct ProtocolAddress *pa = AllocVec(sizeof(*pa), MEMF_CLEAR);
    if (pa)
    {
        pa->pa_node.ln_Type = id;
        pa->pa_family       = PROTO_FAMILY_IPV4;
        AddTail(list, &pa->pa_node);
    }
    return pa;
}

/* Claim and parse one IPv4 token (IP= / NETMASK= / GW=).  IP= must not swallow
 * IP6= and GW= must not swallow GW6= - the '6' breaks the 3-char prefix. */
struct Node *Net4_ReadTokens(struct List *protoList, CONST_STRPTR token, UBYTE id)
{
    struct ProtocolAddress *pa;
    CONST_STRPTR val;

    if (strncmp(token, "IP=", 3) == 0)
    {
        if (!(pa = net4_node(protoList, id))) return NULL;
        val = token + 3;
        if (strncmp(val, "DHCP", 4) == 0)
        {
            pa->pa_mode = IP_MODE_DHCP; pa->pa_addr[0] = '\0';
        }
        else if (strncmp(val, "AUTO", 4) == 0)
        {
            pa->pa_mode = IP_MODE_AUTO; pa->pa_addr[0] = '\0';
        }
        else
        {
            pa->pa_mode = IP_MODE_MANUAL;
            strlcpy(pa->pa_addr, val, sizeof(pa->pa_addr));
        }
        return &pa->pa_node;
    }
    if (strncmp(token, "NETMASK=", 8) == 0)
    {
        if (!(pa = net4_node(protoList, id))) return NULL;
        strlcpy(pa->pa_mask, token + 8, sizeof(pa->pa_mask));
        return &pa->pa_node;
    }
    if (strncmp(token, "GW=", 3) == 0)
    {
        if (!(pa = net4_node(protoList, id))) return NULL;
        strlcpy(pa->pa_gate, token + 3, sizeof(pa->pa_gate));
        return &pa->pa_node;
    }
    return NULL;
}

/* Format the address column for the interface list. */
void Net4_Display(struct ProtocolAddress *pa, STRPTR buf, ULONG buflen)
{
    switch (pa->pa_mode)
    {
        case IP_MODE_DHCP:
            strlcpy(buf, _(MSG_IP_MODE_DHCP), buflen);
            break;
        case IP_MODE_AUTO:
            strlcpy(buf, _(MSG_IP_MODE_AUTO), buflen);
            break;
        default:
            strlcpy(buf, pa->pa_addr[0] ? pa->pa_addr : _(MSG_IP_MODE_MANUAL),
                    buflen);
            break;
    }
}
