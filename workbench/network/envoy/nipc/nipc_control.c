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

/*****************************************************************************

    NAME */
        AROS_LH4(BOOL, AddRoute,

/*  SYNOPSIS */
        AROS_LHA(ULONG, network, D0),
        AROS_LHA(ULONG, gateway, D1),
        AROS_LHA(UWORD, hops, D2),
        AROS_LHA(WORD, ttl, D3),

/*  LOCATION */
         struct NIPCBase *, NIPCBase, 7, NIPC)

/*  FUNCTION
        Route creation through this interface is not supported on AROS.
        The call leaves the routing table unchanged.

    INPUTS
        network - Ignored.
        gateway - Ignored.
        hops - Ignored.
        ttl - Ignored.

    RESULT
        Always FALSE.

    NOTES
        Configure routes through the TCP/IP stack.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    /* Routes belong to the TCP/IP stack; use its tools. */
    return FALSE;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(APTR, DeleteRoute,

/*  SYNOPSIS */
        AROS_LHA(ULONG, network, D0),

/*  LOCATION */
         struct NIPCBase *, NIPCBase, 8, NIPC)

/*  FUNCTION
        Route deletion through this interface is not supported on AROS.
        The call leaves the routing table unchanged.

    INPUTS
        network - Ignored.

    RESULT
        Always NULL. No route object is returned.

    NOTES
        Configure routes through the TCP/IP stack.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return NULL;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH5(BOOL, SetDeviceIPAddressA,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, devicename, A0),
        AROS_LHA(ULONG, unit, D0),
        AROS_LHA(ULONG, ipaddress, D1),
        AROS_LHA(ULONG, mask, D2),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
         struct NIPCBase *, NIPCBase, 55, NIPC)

/*  FUNCTION
        Interface configuration through this interface is not supported
        on AROS. No device is opened and no address or mask is changed.

    INPUTS
        devicename - Ignored; may be NULL.
        unit - Ignored.
        ipaddress - Ignored.
        mask - Ignored.
        tags - Ignored; may be NULL.

    RESULT
        Always FALSE.

    NOTES
        The device name and tag list are neither read nor retained.
        Configure interfaces through the TCP/IP stack.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return FALSE;
    AROS_LIBFUNC_EXIT
}

/* ---- protocol-author API: no private IP layer exists on AROS ---------- */

/*****************************************************************************

    NAME */
        AROS_LH4(APTR, AllocateIPProtocolA,

/*  SYNOPSIS */
        AROS_LHA(UWORD, protonr, D0),
        AROS_LHA(APTR, input, A0),
        AROS_LHA(APTR, cleanup, A1),
        AROS_LHA(struct TagItem *, tags, A2),

/*  LOCATION */
         struct NIPCBase *, NIPCBase, 56, NIPC)

/*  FUNCTION
        Private IP protocol allocation is not supported on AROS.
        No handler is created and no callback is installed.

    INPUTS
        protonr - Ignored.
        input - Ignored; may be NULL.
        cleanup - Ignored; may be NULL.
        tags - Ignored; may be NULL.

    RESULT
        Always NULL.

    NOTES
        Callback pointers are neither invoked nor retained. The tag
        list is not read. No storage is allocated.

    SEE ALSO
        RegisterIPProtocol(), FreeIPProtocol()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return NULL;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(BOOL, RegisterIPProtocol,

/*  SYNOPSIS */
        AROS_LHA(APTR, handle, A0),

/*  LOCATION */
         struct NIPCBase *, NIPCBase, 57, NIPC)

/*  FUNCTION
        Private IP protocol registration is not supported on AROS.
        No protocol handler is registered.

    INPUTS
        handle - Ignored; may be NULL.

    RESULT
        Always FALSE.

    NOTES
        The handle is neither dereferenced nor retained.

    SEE ALSO
        AllocateIPProtocolA(), FreeIPProtocol()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return FALSE;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, FreeIPProtocol,

/*  SYNOPSIS */
        AROS_LHA(APTR, handle, A0),

/*  LOCATION */
         struct NIPCBase *, NIPCBase, 58, NIPC)

/*  FUNCTION
        Perform no operation. AROS does not allocate private IP
        protocol handles through this interface.

    INPUTS
        handle - Ignored; may be NULL.

    RESULT
        None. No storage is freed and no callback is invoked.

    NOTES
        The handle is not dereferenced. This call cannot release
        storage allocated through another interface.

    SEE ALSO
        AllocateIPProtocolA()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH5(void, SendIPPacketA,

/*  SYNOPSIS */
        AROS_LHA(struct NIPCBuff *, data, A0),
        AROS_LHA(ULONG, srcip, D0),
        AROS_LHA(ULONG, destip, D1),
        AROS_LHA(UBYTE, protocol, D2),
        AROS_LHA(struct TagItem *, iptags, A1),

/*  LOCATION */
         struct NIPCBase *, NIPCBase, 59, NIPC)

/*  FUNCTION
        Discard an IP packet by calling FreeNIPCBuff() on its buffer.
        No packet is queued or transmitted.

    INPUTS
        data - Buffer obtained from AllocNIPCBuff(), or NULL.
        srcip - Ignored.
        destip - Ignored.
        protocol - Ignored.
        iptags - Ignored; may be NULL.

    RESULT
        None. The call provides no transmission status.

    NOTES
        An unlocked buffer and its entries are freed before return;
        do not access them afterward. Data allocated with an entry
        is freed with that entry. Separately supplied entry data
        remains caller-owned.

        If the buffer has outstanding LockNIPCBuff() calls, one lock
        is consumed and the buffer survives this call. These locks
        defer freeing; they do not provide mutual exclusion. Prevent
        concurrent access to the buffer while this call runs.

        NULL has no effect. The tag list is not read or retained.
        Call from task context.

    SEE ALSO
        FreeNIPCBuff(), LockNIPCBuff(), AllocNIPCBuff()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    /* The function consumes the buffer. */
    FreeNIPCBuff(data);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH8(ULONG, SendICMPMessageA,

/*  SYNOPSIS */
        AROS_LHA(UWORD, typeandcode, D0),
        AROS_LHA(ULONG, magicid, D1),
        AROS_LHA(APTR, iphdr, A0),
        AROS_LHA(ULONG, sourceip, D2),
        AROS_LHA(ULONG, destip, D3),
        AROS_LHA(APTR, data, A1),
        AROS_LHA(ULONG, datalen, D4),
        AROS_LHA(struct TagItem *, iptags, A2),

/*  LOCATION */
         struct NIPCBase *, NIPCBase, 60, NIPC)

/*  FUNCTION
        ICMP transmission through this interface is not supported
        on AROS. No message is queued or transmitted.

    INPUTS
        typeandcode - Ignored.
        magicid - Ignored.
        iphdr - Ignored; may be NULL.
        sourceip - Ignored.
        destip - Ignored.
        data - Ignored; may be NULL.
        datalen - Ignored.
        iptags - Ignored; may be NULL.

    RESULT
        Always zero. This value does not indicate that a message
        was sent.

    NOTES
        The header, payload and tag list are neither read nor retained.
        Their storage is not freed; ownership remains with the caller.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(APTR, AllocateUDPPortA,

/*  SYNOPSIS */
        AROS_LHA(LONG, localport, D0),
        AROS_LHA(APTR, datain, A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
         struct NIPCBase *, NIPCBase, 61, NIPC)

/*  FUNCTION
        Private UDP port allocation is not supported on AROS.
        No port is reserved and no receive callback is installed.

    INPUTS
        localport - Ignored.
        datain - Ignored; may be NULL.
        tags - Ignored; may be NULL.

    RESULT
        Always NULL.

    NOTES
        The callback is neither invoked nor retained. The tag list
        is not read. No storage is allocated.

    SEE ALSO
        RegisterUDPPort(), FreeUDPPort()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return NULL;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(BOOL, RegisterUDPPort,

/*  SYNOPSIS */
        AROS_LHA(APTR, handle, A0),

/*  LOCATION */
         struct NIPCBase *, NIPCBase, 62, NIPC)

/*  FUNCTION
        Private UDP port registration is not supported on AROS.
        No port is registered.

    INPUTS
        handle - Ignored; may be NULL.

    RESULT
        Always FALSE.

    NOTES
        The handle is neither dereferenced nor retained.

    SEE ALSO
        AllocateUDPPortA(), FreeUDPPort()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return FALSE;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, FreeUDPPort,

/*  SYNOPSIS */
        AROS_LHA(APTR, handle, A0),

/*  LOCATION */
         struct NIPCBase *, NIPCBase, 63, NIPC)

/*  FUNCTION
        Perform no operation. AROS does not allocate private UDP
        port handles through this interface.

    INPUTS
        handle - Ignored; may be NULL.

    RESULT
        None. No storage is freed and no callback is invoked.

    NOTES
        The handle is not dereferenced. This call cannot release
        storage allocated through another interface.

    SEE ALSO
        AllocateUDPPortA()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH6(void, SendUDPPacketA,

/*  SYNOPSIS */
        AROS_LHA(APTR, dataptr, A0),
        AROS_LHA(UWORD, length, D0),
        AROS_LHA(ULONG, destip, D1),
        AROS_LHA(UWORD, srcport, D2),
        AROS_LHA(UWORD, dstport, D3),
        AROS_LHA(struct TagItem *, iptags, A1),

/*  LOCATION */
         struct NIPCBase *, NIPCBase, 64, NIPC)

/*  FUNCTION
        UDP transmission through this interface is not supported
        on AROS. No packet is queued or transmitted.

    INPUTS
        dataptr - Ignored; may be NULL.
        length - Ignored.
        destip - Ignored.
        srcport - Ignored.
        dstport - Ignored.
        iptags - Ignored; may be NULL.

    RESULT
        None. The call provides no transmission status.

    NOTES
        The payload and tag list are neither read nor retained.
        Their storage is not freed; ownership remains with the caller.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBFUNC_EXIT
}
