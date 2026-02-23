/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    net4.c - IPv4 ProtocolAddress implementation:
      - Net4_CreateWindow   : build the IPv4 configuration sub-window
      - Net4_ShowWindow     : populate gadgets from a ProtocolAddress
      - Net4_ApplyWindow    : read gadgets back into a ProtocolAddress
      - Net4_ModeChanged    : handle the mode-cycle notification
      - Net4_WriteTokens    : write IP= / NETMASK= / GW= to a FILE
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <libraries/mui.h>
#include <libraries/asl.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/alib.h>
#include <utility/hooks.h>
#include <string.h>
#include <stdio.h>

#include "protocols.h"
#include "locale.h"

static CONST_STRPTR IPv4ModeCycle[] = { NULL, NULL, NULL, NULL };

/* Widest possible IPv4 string, used to fix gadget width */
static const TEXT ipv4_max_str[] = "255.255.255.255 ";

/*---------------------------------------------------------------------------*/
Object *Net4_CreateWindow(Object **modeOut,
                          Object **addrOut,
                          Object **maskOut,
                          Object **gateOut,
                          Object **applyOut,
                          Object **closeOut)
{
    Object *win, *mode, *addr, *mask, *gate, *apply, *close;

    IPv4ModeCycle[0] = _(MSG_IP_MODE_DHCP);
    IPv4ModeCycle[1] = _(MSG_IP_MODE_AUTO);
    IPv4ModeCycle[2] = _(MSG_IP_MODE_MANUAL);

    win = (Object *)WindowObject,
        MUIA_Window_Title,      (IPTR)"IPv4 Configuration",
        MUIA_Window_ID,         MAKE_ID('I','P','4','W'),
        MUIA_Window_CloseGadget, FALSE,
        WindowContents, (IPTR)VGroup,
            GroupFrame,
            Child, (IPTR)HGroup,
                Child, (IPTR)HVSpace,
                Child, (IPTR)ImageObject,
                    MUIA_Image_Spec,  (IPTR)"3:Images:protocol",
                    MUIA_FixWidth,    52,
                    MUIA_FixHeight,   48,
                End,
                Child, (IPTR)HVSpace,
            End,
            Child, (IPTR)ColGroup(2),
                GroupFrame,
                Child, (IPTR)Label2(__(MSG_IP_MODE)),
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
            End,
            Child, (IPTR)HGroup,
                Child, (IPTR)(apply = ImageButton(_(MSG_BUTTON_OK),
                    "THEME:Images/Gadgets/Prefs/Save")),
                Child, (IPTR)(close = ImageButton(_(MSG_BUTTON_CANCEL),
                    "THEME:Images/Gadgets/Prefs/Cancel")),
            End,
        End,
    End;

    if (win)
    {
        *modeOut  = mode;
        *addrOut  = addr;
        *maskOut  = mask;
        *gateOut  = gate;
        *applyOut = apply;
        *closeOut = close;
    }
    return win;
}

/*---------------------------------------------------------------------------*/
void Net4_ShowWindow(struct ProtocolAddress *pa,
                     Object *mode, Object *addr, Object *mask, Object *gate)
{
    SET(mode, MUIA_Cycle_Active, (IPTR)pa->pa_mode);

    if (pa->pa_mode == IP_MODE_MANUAL)
    {
        SET(addr, MUIA_Disabled, FALSE);
        SET(addr, MUIA_String_Contents, pa->pa_addr);
        SET(mask, MUIA_Disabled, FALSE);
        SET(mask, MUIA_String_Contents, pa->pa_mask);
    }
    else
    {
        SET(addr, MUIA_Disabled, TRUE);
        SET(addr, MUIA_String_Contents, "");
        SET(mask, MUIA_Disabled, TRUE);
        SET(mask, MUIA_String_Contents, "");
    }

    /* Gateway is always editable regardless of mode */
    SET(gate, MUIA_String_Contents, pa->pa_gate);
}

/*---------------------------------------------------------------------------*/
void Net4_ApplyWindow(struct ProtocolAddress *pa,
                      Object *mode, Object *addr, Object *mask, Object *gate)
{
    pa->pa_mode = (enum IPMode)XGET(mode, MUIA_Cycle_Active);

    if (pa->pa_mode == IP_MODE_MANUAL)
    {
        strncpy(pa->pa_addr,
                (STRPTR)XGET(addr, MUIA_String_Contents),
                sizeof(pa->pa_addr) - 1);
        strncpy(pa->pa_mask,
                (STRPTR)XGET(mask, MUIA_String_Contents),
                sizeof(pa->pa_mask) - 1);
    }
    else
    {
        /* Keep saved values in pa_addr / pa_mask but clear for non-manual */
        pa->pa_addr[0] = '\0';
        pa->pa_mask[0] = '\0';
    }

    strncpy(pa->pa_gate,
            (STRPTR)XGET(gate, MUIA_String_Contents),
            sizeof(pa->pa_gate) - 1);

    pa->pa_addr[sizeof(pa->pa_addr) - 1] = '\0';
    pa->pa_mask[sizeof(pa->pa_mask) - 1] = '\0';
    pa->pa_gate[sizeof(pa->pa_gate) - 1] = '\0';
}

/*---------------------------------------------------------------------------*/
void Net4_ModeChanged(struct ProtocolAddress *pa, ULONG newMode,
                      Object *addr, Object *mask)
{
    STRPTR str = NULL;

    if (newMode == IP_MODE_MANUAL)
    {
        /* Restore previously saved address/mask */
        SET(addr, MUIA_Disabled, FALSE);
        SET(addr, MUIA_String_Contents, pa->pa_addr);
        SET(mask, MUIA_Disabled, FALSE);
        SET(mask, MUIA_String_Contents, pa->pa_mask);
    }
    else
    {
        /* Save current contents before clearing */
        GET(addr, MUIA_String_Contents, &str);
        if (str && str[0])
            strncpy(pa->pa_addr, str, sizeof(pa->pa_addr) - 1);
        SET(addr, MUIA_Disabled, TRUE);
        SET(addr, MUIA_String_Contents, "");

        GET(mask, MUIA_String_Contents, &str);
        if (str && str[0])
            strncpy(pa->pa_mask, str, sizeof(pa->pa_mask) - 1);
        SET(mask, MUIA_Disabled, TRUE);
        SET(mask, MUIA_String_Contents, "");
    }

    pa->pa_mode = (enum IPMode)newMode;
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
