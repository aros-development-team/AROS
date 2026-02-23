/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    net6.c - IPv6 ProtocolAddress implementation:
      - Net6_CreateWindow   : build the IPv6 configuration sub-window
      - Net6_ShowWindow     : populate gadgets from a ProtocolAddress
      - Net6_ApplyWindow    : read gadgets back into a ProtocolAddress
      - Net6_ModeChanged    : handle the mode-cycle notification
      - Net6_WriteTokens    : write IP6= / GW6= to a FILE
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

static CONST_STRPTR IPv6ModeCycle[] = { NULL, NULL, NULL, NULL };

/*---------------------------------------------------------------------------*/
Object *Net6_CreateWindow(Object **modeOut,
                          Object **addrOut,
                          Object **prefixOut,
                          Object **gateOut,
                          Object **applyOut,
                          Object **closeOut)
{
    Object *win, *mode, *addr, *prefix, *gate, *apply, *close;

    IPv6ModeCycle[0] = _(MSG_IP_MODE_DHCP);
    IPv6ModeCycle[1] = _(MSG_IP6_MODE_AUTO);
    IPv6ModeCycle[2] = _(MSG_IP_MODE_MANUAL);

    win = (Object *)WindowObject,
        MUIA_Window_Title,       (IPTR)"IPv6 Configuration",
        MUIA_Window_ID,          MAKE_ID('I','P','6','W'),
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
                Child, (IPTR)Label2(__(MSG_IP6_MODE)),
                Child, (IPTR)(mode = (Object *)CycleObject,
                    MUIA_Cycle_Entries, (IPTR)IPv6ModeCycle,
                End),
                Child, (IPTR)Label2(__(MSG_IP6)),
                Child, (IPTR)(addr = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_Accept, (IPTR)IP6CHARS,
                    MUIA_CycleChain,    1,
                End),
                Child, (IPTR)Label2(__(MSG_IP6_PREFIX)),
                Child, (IPTR)HGroup,
                    Child, (IPTR)(prefix = (Object *)StringObject,
                        StringFrame,
                        MUIA_String_Accept,  (IPTR)"0123456789",
                        MUIA_CycleChain,     1,
                        MUIA_FixWidthTxt,    (IPTR)"128 ",
                    End),
                    Child, (IPTR)HVSpace,
                End,
                Child, (IPTR)Label2(__(MSG_GATE6)),
                Child, (IPTR)(gate = (Object *)StringObject,
                    StringFrame,
                    MUIA_String_Accept, (IPTR)IP6CHARS,
                    MUIA_CycleChain,    1,
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
        *modeOut   = mode;
        *addrOut   = addr;
        *prefixOut = prefix;
        *gateOut   = gate;
        *applyOut  = apply;
        *closeOut  = close;
    }
    return win;
}

/*---------------------------------------------------------------------------*/
void Net6_ShowWindow(struct ProtocolAddress *pa,
                     Object *mode, Object *addr, Object *prefix, Object *gate)
{
    SET(mode, MUIA_Cycle_Active, (IPTR)pa->pa_mode);

    if (pa->pa_mode == IP_MODE_MANUAL)
    {
        LONG plen = pa->pa_prefix ? pa->pa_prefix : 64;
        SET(addr,   MUIA_Disabled, FALSE);
        SET(addr,   MUIA_String_Contents, pa->pa_addr);
        SET(prefix, MUIA_Disabled, FALSE);
        SET(prefix, MUIA_String_Integer, plen);
    }
    else
    {
        SET(addr,   MUIA_Disabled, TRUE);
        SET(addr,   MUIA_String_Contents, "");
        SET(prefix, MUIA_Disabled, TRUE);
        SET(prefix, MUIA_String_Contents, "");
    }

    /* Gateway is always editable regardless of mode */
    SET(gate, MUIA_String_Contents, pa->pa_gate);
}

/*---------------------------------------------------------------------------*/
void Net6_ApplyWindow(struct ProtocolAddress *pa,
                      Object *mode, Object *addr, Object *prefix, Object *gate)
{
    pa->pa_mode = (enum IPMode)XGET(mode, MUIA_Cycle_Active);

    if (pa->pa_mode == IP_MODE_MANUAL)
    {
        strncpy(pa->pa_addr,
                (STRPTR)XGET(addr, MUIA_String_Contents),
                sizeof(pa->pa_addr) - 1);
        pa->pa_prefix = (LONG)XGET(prefix, MUIA_String_Integer);
    }
    else
    {
        pa->pa_addr[0] = '\0';
        pa->pa_prefix  = 0;
    }

    strncpy(pa->pa_gate,
            (STRPTR)XGET(gate, MUIA_String_Contents),
            sizeof(pa->pa_gate) - 1);

    pa->pa_addr[sizeof(pa->pa_addr) - 1] = '\0';
    pa->pa_gate[sizeof(pa->pa_gate) - 1] = '\0';
}

/*---------------------------------------------------------------------------*/
void Net6_ModeChanged(struct ProtocolAddress *pa, ULONG newMode,
                      Object *addr, Object *prefix)
{
    STRPTR str = NULL;

    if (newMode == IP_MODE_MANUAL)
    {
        LONG plen = pa->pa_prefix ? pa->pa_prefix : 64;
        SET(addr,   MUIA_Disabled, FALSE);
        SET(addr,   MUIA_String_Contents, pa->pa_addr);
        SET(prefix, MUIA_Disabled, FALSE);
        SET(prefix, MUIA_String_Integer,  plen);
    }
    else
    {
        GET(addr, MUIA_String_Contents, &str);
        if (str && str[0])
            strncpy(pa->pa_addr, str, sizeof(pa->pa_addr) - 1);
        SET(addr,   MUIA_Disabled, TRUE);
        SET(addr,   MUIA_String_Contents, "");
        SET(prefix, MUIA_Disabled, TRUE);
        SET(prefix, MUIA_String_Contents, "");
    }

    pa->pa_mode = (enum IPMode)newMode;
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
