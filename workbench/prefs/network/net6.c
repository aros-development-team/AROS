/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    net6.c - IPv6 protocol-address configuration window class (Net6WinClass):
      - Subclass of PAWinClass (protocols.c)
      - MUIM_PAWin_Show        : populate gadgets from a ProtocolAddress
      - MUIM_PAWin_Apply       : read gadgets back into a ProtocolAddress
      - MUIM_PAWin_ModeChanged : enable/disable addr+prefix for manual mode
      - Net6_WriteTokens       : write IP6= / GW6= to a FILE
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

struct MUI_CustomClass *Net6WinClass = NULL;

struct Net6Win_Data
{
    Object *n6_modeObj;
    Object *n6_addrObj;
    Object *n6_prefixObj;
    Object *n6_gateObj;
};

static CONST_STRPTR IPv6ModeCycle[] = { NULL, NULL, NULL, NULL };

/*---------------------------------------------------------------------------*/
static IPTR Net6Win__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    Object *mode, *addr, *prefix, *gate, *content;

    IPv6ModeCycle[0] = _(MSG_IP_MODE_DHCP);
    IPv6ModeCycle[1] = _(MSG_IP6_MODE_AUTO);
    IPv6ModeCycle[2] = _(MSG_IP_MODE_MANUAL);

    /* Build the IPv6-specific content group */
    content = (Object *)ColGroup(2),
        GroupFrame,
        Child, (IPTR)Label2(__(MSG_ADDR_MODE)),
        Child, (IPTR)(mode = (Object *)CycleObject,
            MUIA_Cycle_Entries, (IPTR)IPv6ModeCycle,
        End),
        Child, (IPTR)Label2(__(MSG_IP)),
        Child, (IPTR)(addr = (Object *)StringObject,
            StringFrame,
            MUIA_String_Accept, (IPTR)IP6CHARS,
            MUIA_CycleChain,    1,
        End),
        Child, (IPTR)Label2(__(MSG_PREFIX_LEN)),
        Child, (IPTR)HGroup,
            Child, (IPTR)(prefix = (Object *)StringObject,
                StringFrame,
                MUIA_String_Accept,  (IPTR)"0123456789",
                MUIA_CycleChain,     1,
                MUIA_FixWidthTxt,    (IPTR)"128 ",
            End),
            Child, (IPTR)HVSpace,
        End,
        Child, (IPTR)Label2(__(MSG_GATE)),
        Child, (IPTR)(gate = (Object *)StringObject,
            StringFrame,
            MUIA_String_Accept, (IPTR)IP6CHARS,
            MUIA_CycleChain,    1,
        End),
    End;

    if (!content)
        return 0;

    /* Pass content + protocol name to the PAWin base class */
    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
        MUIA_PAWin_ProtocolName, (IPTR)"IPv6",
        MUIA_PAWin_Content,      (IPTR)content,
        MUIA_Window_ID,          MAKE_ID('I','P','6','W'),
        TAG_MORE, (IPTR)msg->ops_AttrList);

    if (!obj)
        return 0;

    struct Net6Win_Data *data = INST_DATA(cl, obj);
    data->n6_modeObj   = mode;
    data->n6_addrObj   = addr;
    data->n6_prefixObj = prefix;
    data->n6_gateObj   = gate;

    /* Mode cycle notifies the window to update gadget states */
    DoMethod(mode, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             obj, 2, MUIM_PAWin_ModeChanged, MUIV_TriggerValue);

    return (IPTR)obj;
}

/*---------------------------------------------------------------------------*/
static IPTR Net6Win__MUIM_PAWin_Show(Class *cl, Object *obj,
    struct MUIP_PAWin_Show *msg)
{
    struct Net6Win_Data *data = INST_DATA(cl, obj);
    struct ProtocolAddress *pa = msg->pa;

    SET(data->n6_modeObj, MUIA_Cycle_Active, (IPTR)pa->pa_mode);

    if (pa->pa_mode == IP_MODE_MANUAL)
    {
        LONG plen = pa->pa_prefix ? pa->pa_prefix : 64;
        SET(data->n6_addrObj,   MUIA_Disabled, FALSE);
        SET(data->n6_addrObj,   MUIA_String_Contents, pa->pa_addr);
        SET(data->n6_prefixObj, MUIA_Disabled, FALSE);
        SET(data->n6_prefixObj, MUIA_String_Integer, plen);
    }
    else
    {
        SET(data->n6_addrObj,   MUIA_Disabled, TRUE);
        SET(data->n6_addrObj,   MUIA_String_Contents, "");
        SET(data->n6_prefixObj, MUIA_Disabled, TRUE);
        SET(data->n6_prefixObj, MUIA_String_Contents, "");
    }
    SET(data->n6_gateObj, MUIA_String_Contents, pa->pa_gate);
    return 0;
}

/*---------------------------------------------------------------------------*/
static IPTR Net6Win__MUIM_PAWin_Apply(Class *cl, Object *obj,
    struct MUIP_PAWin_Apply *msg)
{
    struct Net6Win_Data *data = INST_DATA(cl, obj);
    struct ProtocolAddress *pa = msg->pa;

    pa->pa_mode = (enum IPMode)XGET(data->n6_modeObj, MUIA_Cycle_Active);

    if (pa->pa_mode == IP_MODE_MANUAL)
    {
        strncpy(pa->pa_addr,
                (STRPTR)XGET(data->n6_addrObj, MUIA_String_Contents),
                sizeof(pa->pa_addr) - 1);
        pa->pa_prefix = (LONG)XGET(data->n6_prefixObj, MUIA_String_Integer);
    }
    else
    {
        pa->pa_addr[0] = '\0';
        pa->pa_prefix  = 0;
    }
    strncpy(pa->pa_gate,
            (STRPTR)XGET(data->n6_gateObj, MUIA_String_Contents),
            sizeof(pa->pa_gate) - 1);

    pa->pa_addr[sizeof(pa->pa_addr) - 1] = '\0';
    pa->pa_gate[sizeof(pa->pa_gate) - 1] = '\0';
    return 0;
}

/*---------------------------------------------------------------------------*/
static IPTR Net6Win__MUIM_PAWin_ModeChanged(Class *cl, Object *obj,
    struct MUIP_PAWin_ModeChanged *msg)
{
    struct Net6Win_Data *data = INST_DATA(cl, obj);
    ULONG newMode = msg->newMode;

    if (newMode == IP_MODE_MANUAL)
    {
        SET(data->n6_addrObj,   MUIA_Disabled, FALSE);
        SET(data->n6_prefixObj, MUIA_Disabled, FALSE);
    }
    else
    {
        SET(data->n6_addrObj,   MUIA_Disabled, TRUE);
        SET(data->n6_addrObj,   MUIA_String_Contents, "");
        SET(data->n6_prefixObj, MUIA_Disabled, TRUE);
        SET(data->n6_prefixObj, MUIA_String_Contents, "");
    }
    return 0;
}

/*---------------------------------------------------------------------------*/
static IPTR Net6Win_Dispatch(Class *cl, Object *obj, Msg msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW:
            return Net6Win__OM_NEW(cl, obj, (struct opSet *)msg);
        case MUIM_PAWin_Show:
            return Net6Win__MUIM_PAWin_Show(cl, obj,
                       (struct MUIP_PAWin_Show *)msg);
        case MUIM_PAWin_Apply:
            return Net6Win__MUIM_PAWin_Apply(cl, obj,
                       (struct MUIP_PAWin_Apply *)msg);
        case MUIM_PAWin_ModeChanged:
            return Net6Win__MUIM_PAWin_ModeChanged(cl, obj,
                       (struct MUIP_PAWin_ModeChanged *)msg);
        default:
            return DoSuperMethodA(cl, obj, msg);
    }
}

/*---------------------------------------------------------------------------*/
BOOL Net6Win_InitClass(void)
{
    if (Net6WinClass)
        return TRUE;
    if (!PAWinClass && !PAWin_InitClass())
        return FALSE;
    Net6WinClass = MUI_CreateCustomClass(NULL, NULL, PAWinClass,
                       sizeof(struct Net6Win_Data), Net6Win_Dispatch);
    return Net6WinClass != NULL;
}

void Net6Win_FreeClass(void)
{
    if (Net6WinClass)
    {
        MUI_DeleteCustomClass(Net6WinClass);
        Net6WinClass = NULL;
    }
}

/*---------------------------------------------------------------------------*/
void Net6_WriteTokens(FILE *f, struct ProtocolAddress *pa)
{
    switch (pa->pa_mode)
    {
        case IP_MODE_DHCP:
            fprintf(f, "IP6=DHCP ");
            break;
        case IP_MODE_AUTO:
            fprintf(f, "IP6=AUTO ");
            break;
        default:
            if (pa->pa_addr[0])
                fprintf(f, "IP6=%s PREFIXLEN=%ld ",
                    pa->pa_addr,
                    pa->pa_prefix ? pa->pa_prefix : 64L);
            else
                fprintf(f, "IP6=AUTO ");
            break;
    }
    if (pa->pa_gate[0])
        fprintf(f, "GW6=%s ", pa->pa_gate);
}
