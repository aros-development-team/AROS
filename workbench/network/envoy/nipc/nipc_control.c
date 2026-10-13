/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: nipc.library - NIPCControlA, NIPCInquiryA, and the functions that
          belong to Envoy's own IP layer. On AROS the TCP/IP stack owns
          routes, interfaces, IP and UDP, so those functions are accepted
          and do nothing, as documented in each.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <string.h>

#include <proto/nipc.h>

#include "nipc_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH1(void, NIPCControlA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 79, NIPC)

/*  FUNCTION
        Get and set the library's tunables. The RDP settings take effect
        for all connections at once. The IP, ARP and fragment settings are
        stored and read back but have no effect, since the host stack
        handles those layers. NIPCTAG_GetMTUforIP asks the stack for the
        MTU of the interface that routes to an address.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tag, *tstate = taglist;

#define SETGET(settag, gettag, var) \
        case settag: var = tag->ti_Data; break; \
        case gettag: if (tag->ti_Data) *(ULONG *)tag->ti_Data = var; break;

    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
        SETGET(NIPCTAG_SetDefaultTTL, NIPCTAG_GetDefaultTTL, NIPCBase->DefaultTTL)
        SETGET(NIPCTAG_SetDefaultTOS, NIPCTAG_GetDefaultTOS, NIPCBase->DefaultTOS)
        SETGET(NIPCTAG_SetFragmentTO, NIPCTAG_GetFragmentTO, NIPCBase->FragmentTO)
        SETGET(NIPCTAG_SetARPResolveTO, NIPCTAG_GetARPResolveTO, NIPCBase->ArpResolveTO)
        SETGET(NIPCTAG_SetARPResolveRetries, NIPCTAG_GetARPResolveRetries, NIPCBase->ArpResolveRetries)
        SETGET(NIPCTAG_SetARPEntryTO, NIPCTAG_GetARPEntryTO, NIPCBase->ArpEntryTO)
        SETGET(NIPCTAG_SetRDPInactivityCheck, NIPCTAG_GetRDPInactivityCheck, NIPCBase->Rdp.InactivityCheck)
        SETGET(NIPCTAG_SetRDPInactivityLimit, NIPCTAG_GetRDPInactivityLimit, NIPCBase->Rdp.InactivityLimit)
        SETGET(NIPCTAG_SetRDPTransmitRetries, NIPCTAG_GetRDPTransmitRetries, NIPCBase->Rdp.TransmitRetries)
        SETGET(NIPCTAG_SetRDPTransmitMinTO, NIPCTAG_GetRDPTransmitMinTO, NIPCBase->Rdp.TransmitMinTO)
        SETGET(NIPCTAG_SetRDPTransmitMaxTO, NIPCTAG_GetRDPTransmitMaxTO, NIPCBase->Rdp.TransmitMaxTO)
        SETGET(NIPCTAG_SetRDPInitialRoundTripTO, NIPCTAG_GetRDPInitialRoundTripTO, NIPCBase->Rdp.InitialRoundTripTO)
        SETGET(NIPCTAG_SetRDPConnectTO, NIPCTAG_GetRDPConnectTO, NIPCBase->Rdp.ConnectTO)
        SETGET(NIPCTAG_SetRDPConnectRetries, NIPCTAG_GetRDPConnectRetries, NIPCBase->Rdp.ConnectRetries)

        case NIPCTAG_GetMTUforIP:
            if (tag->ti_Data)
            {
                struct SuperReq req;
                memset(&req, 0, sizeof(req));
                req.Type = SREQ_GETMTU;
                req.Value = *(ULONG *)tag->ti_Data;
                if (SendSuperReq(NIPCBase, &req))
                    *(ULONG *)tag->ti_Data = req.Error;
                else
                    *(ULONG *)tag->ti_Data = 0;
            }
            break;

        case NIPCTAG_ControlCommand:
            /* AS225-style ioctls: the stack owns interfaces, routes and ARP */
            {
                struct TagItem *r = FindTagItem(NIPCTAG_ControlResult, taglist);
                struct TagItem *r2 = FindTagItem(NIPCTAG_ControlResult2, taglist);
                if (r && r->ti_Data) *(LONG *)r->ti_Data = -1;
                if (r2 && r2->ti_Data) *(LONG *)r2->ti_Data = 22;    /* EINVAL */
            }
            break;
        }
    }
#undef SETGET

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH4(BOOL, NIPCInquiryA,

/*  SYNOPSIS */
        AROS_LHA(struct Hook *, hook, A0),
        AROS_LHA(ULONG, maxTime, D0),
        AROS_LHA(ULONG, maxResponses, D1),
        AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 35, NIPC)

/*  FUNCTION
        Start a network inquiry: the QUERY_#? / MATCH_#? tags are sent to
        the hosts of the local network (or, in a realm, through the Realm
        Server), repeated once a second for maxTime seconds. The hook is
        called for every answer with the answering host's tag list as the
        message and the calling task as the object, and once more with a
        NULL message when the inquiry ends (after maxTime seconds, after
        maxResponses answers, or when the hook returned FALSE). The hook
        runs in the library's own process. With maxTime 0 the oldest
        pending inquiry using the same hook is aborted.

    RESULT
        TRUE if the inquiry was started (or aborted).

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct SuperReq *req;
    BOOL ok;

    if (!hook || (!tagList && maxTime))
        return FALSE;
    if (!(req = AllocVec(sizeof(struct SuperReq), MEMF_CLEAR | MEMF_PUBLIC)))
        return FALSE;
    req->Type = maxTime ? SREQ_INQUIRY : SREQ_INQUIRYABORT;
    req->Hook = hook;
    req->MaxTime = maxTime;
    req->MaxResponses = maxResponses;
    req->Tags = tagList;
    /* synchronous: the supervisor validates the tags and queues the query */
    ok = SendSuperReq(NIPCBase, req) ? (req->Error == 0) : FALSE;
    FreeVec(req);
    return ok;

    AROS_LIBFUNC_EXIT
}

/* ---- routing and interface control: owned by the stack on AROS -------- */

AROS_LH4(BOOL, AddRoute,
         AROS_LHA(ULONG, network, D0), AROS_LHA(ULONG, gateway, D1), AROS_LHA(UWORD, hops, D2), AROS_LHA(WORD, ttl, D3),
         struct NIPCBase *, NIPCBase, 7, NIPC)
{
    AROS_LIBFUNC_INIT
    /* Routes belong to the TCP/IP stack; use its tools. */
    return FALSE;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(APTR, DeleteRoute,
         AROS_LHA(ULONG, network, D0),
         struct NIPCBase *, NIPCBase, 8, NIPC)
{
    AROS_LIBFUNC_INIT
    return NULL;
    AROS_LIBFUNC_EXIT
}

AROS_LH5(BOOL, SetDeviceIPAddressA,
         AROS_LHA(CONST_STRPTR, devicename, A0), AROS_LHA(ULONG, unit, D0), AROS_LHA(ULONG, ipaddress, D1),
         AROS_LHA(ULONG, mask, D2), AROS_LHA(struct TagItem *, tags, A1),
         struct NIPCBase *, NIPCBase, 55, NIPC)
{
    AROS_LIBFUNC_INIT
    return FALSE;
    AROS_LIBFUNC_EXIT
}

/* ---- protocol-author API: no private IP layer exists on AROS ---------- */

AROS_LH4(APTR, AllocateIPProtocolA,
         AROS_LHA(UWORD, protonr, D0), AROS_LHA(APTR, input, A0), AROS_LHA(APTR, cleanup, A1), AROS_LHA(struct TagItem *, tags, A2),
         struct NIPCBase *, NIPCBase, 56, NIPC)
{
    AROS_LIBFUNC_INIT
    return NULL;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(BOOL, RegisterIPProtocol,
         AROS_LHA(APTR, handle, A0),
         struct NIPCBase *, NIPCBase, 57, NIPC)
{
    AROS_LIBFUNC_INIT
    return FALSE;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, FreeIPProtocol,
         AROS_LHA(APTR, handle, A0),
         struct NIPCBase *, NIPCBase, 58, NIPC)
{
    AROS_LIBFUNC_INIT
    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, SendIPPacketA,
         AROS_LHA(struct NIPCBuff *, data, A0), AROS_LHA(ULONG, srcip, D0), AROS_LHA(ULONG, destip, D1),
         AROS_LHA(UBYTE, protocol, D2), AROS_LHA(struct TagItem *, iptags, A1),
         struct NIPCBase *, NIPCBase, 59, NIPC)
{
    AROS_LIBFUNC_INIT
    /* The function consumes the buffer. */
    FreeNIPCBuff(data);
    AROS_LIBFUNC_EXIT
}

AROS_LH8(ULONG, SendICMPMessageA,
         AROS_LHA(UWORD, typeandcode, D0), AROS_LHA(ULONG, magicid, D1), AROS_LHA(APTR, iphdr, A0),
         AROS_LHA(ULONG, sourceip, D2), AROS_LHA(ULONG, destip, D3), AROS_LHA(APTR, data, A1),
         AROS_LHA(ULONG, datalen, D4), AROS_LHA(struct TagItem *, iptags, A2),
         struct NIPCBase *, NIPCBase, 60, NIPC)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH3(APTR, AllocateUDPPortA,
         AROS_LHA(LONG, localport, D0), AROS_LHA(APTR, datain, A0), AROS_LHA(struct TagItem *, tags, A1),
         struct NIPCBase *, NIPCBase, 61, NIPC)
{
    AROS_LIBFUNC_INIT
    return NULL;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(BOOL, RegisterUDPPort,
         AROS_LHA(APTR, handle, A0),
         struct NIPCBase *, NIPCBase, 62, NIPC)
{
    AROS_LIBFUNC_INIT
    return FALSE;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, FreeUDPPort,
         AROS_LHA(APTR, handle, A0),
         struct NIPCBase *, NIPCBase, 63, NIPC)
{
    AROS_LIBFUNC_INIT
    AROS_LIBFUNC_EXIT
}

AROS_LH6(void, SendUDPPacketA,
         AROS_LHA(APTR, dataptr, A0), AROS_LHA(UWORD, length, D0), AROS_LHA(ULONG, destip, D1),
         AROS_LHA(UWORD, srcport, D2), AROS_LHA(UWORD, dstport, D3), AROS_LHA(struct TagItem *, iptags, A1),
         struct NIPCBase *, NIPCBase, 64, NIPC)
{
    AROS_LIBFUNC_INIT
    AROS_LIBFUNC_EXIT
}
