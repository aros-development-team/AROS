/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    protocols.c - shared ProtocolAddress infrastructure:
      - MUI list hooks (construct / destruct / display)
      - ProtoAddr_FromInterface / ProtoAddr_ToInterface conversions
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <libraries/mui.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/alib.h>
#include <utility/hooks.h>
#include <string.h>

#include "protocols.h"
#include "locale.h"

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
