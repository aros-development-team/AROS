/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    net4.c - IPv4 protocol-address configuration window class (Net4WinClass):
      - Subclass of PAWinClass (protocols.c)
      - MUIM_PAWin_Show        : populate gadgets from a ProtocolAddress
      - MUIM_PAWin_Apply       : read gadgets back into a ProtocolAddress
      - MUIM_PAWin_ModeChanged : enable/disable addr+mask for manual mode
      - Net4_WriteTokens       : write IP= / NETMASK= / GW= to a FILE
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <libraries/mui.h>
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
static IPTR Net4Win_Dispatch(Class *cl, Object *obj, Msg msg)
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

/*---------------------------------------------------------------------------*/
BOOL Net4Win_InitClass(void)
{
    if (Net4WinClass)
        return TRUE;
    if (!PAWinClass && !PAWin_InitClass())
        return FALSE;
    Net4WinClass = MUI_CreateCustomClass(NULL, NULL, PAWinClass,
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
        default:
            fprintf(f, "IP=%s NETMASK=%s ",
                pa->pa_addr[0] ? (const char *)pa->pa_addr : "0.0.0.0",
                pa->pa_mask[0] ? (const char *)pa->pa_mask : "255.255.255.0");
            break;
    }
    if (pa->pa_gate[0])
        fprintf(f, "GW=%s ", pa->pa_gate);
}
