/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * What the core keeps of the packet services (net/packet_ops.h) while
 * they are a module loaded on first use:
 *
 *  - the capture vectors of bsdsocket.library (61-68), with their places,
 *    names and arguments. bpf_open() has the module started - its library
 *    loaded - when it is not running; it fails with errno ENOENT when the
 *    module cannot be had (not installed, or its test switch
 *    ENV:AROSTCP/Without_packet is set). The others never load it: while
 *    it does not run no handle is open, and they answer EBADF, as they
 *    always did for a handle that is not open;
 *  - the capture tap of the SANA-II interfaces (nettap_set()).
 *
 * None of the vectors takes the socket calls' fence, so the demand slot
 * is entered as net/netdemand.h requires. Nothing here runs for a packet
 * but the tap, which the SANA-II paths call only for an interface that
 * has a capture open.
 */

#include <conf.h>

#include <aros/libcall.h>
#include <aros/atomic.h>
#include <exec/types.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/errno.h>

#include <net/if.h>
#include <api/amiga_api.h>
#include <net/netdemand.h>
#include <net/packet_ops.h>

/*
 * The demand slot: the module's table while it runs. Found by the
 * module's name when it puts its table in (netdemand_set()).
 */
static struct netdemand packet_demand = { PACKET_MODULE };
NETDEMAND_SLOT(packet_demand);

/*
 * THE CAPTURE TAP. Valid functions at all times: a tap called between a
 * capture's if_bpf and the module's table (which cannot happen while the
 * stop is refused with a capture open) does nothing rather than jump to
 * NULL.
 */
static void
nettap_none(struct ifnet *ifp, struct mbuf *m, int direction)
{
}

static void
nettap_none2(struct ifnet *ifp, const void *hdr, u_int hlen, struct mbuf *m, int direction)
{
}

nettap_mtap_fn  nettap_mtap = nettap_none;
nettap_mtap2_fn nettap_mtap2 = nettap_none2;

int
nettap_set(nettap_mtap_fn mtap, nettap_mtap2_fn mtap2)
{
    struct ifnet *ifp;

    if(mtap == NULL || mtap2 == NULL) {
        for(ifp = ifnet; ifp != NULL; ifp = ifp->if_next)
            if(ifp->if_bpf != NULL)
                return (EBUSY);
        nettap_mtap = nettap_none;
        nettap_mtap2 = nettap_none2;
        return (0);
    }
    nettap_mtap = mtap;
    nettap_mtap2 = mtap2;
    return (0);
}

#ifdef __CONFIG_ROADSHOW__

/*
 * The table, if the module runs - without starting it: the slot counted
 * first, the table read after, as netdemand_enter() does (a stop clears
 * the table first and reads the count after). NULL: nothing is open.
 */
static const struct packet_ops *
packet_running(void)
{
    const struct packet_ops *po;

    AROS_ATOMIC_INC(packet_demand.nd_busy);
    if((po = packet_demand.nd_ops) == NULL)
        AROS_ATOMIC_DEC(packet_demand.nd_busy);
    return (po);
}

AROS_LH1(long, bpf_open,
         AROS_LHA(long, channel, D0),
         struct SocketBase *, libPtr, 61, UL)
{
    AROS_LIBFUNC_INIT

    const struct packet_ops *po;
    long r;
    int error;

    if((error = netdemand_enter_error(&packet_demand, (const void **)&po)) != 0) {
        writeErrnoValue(libPtr, error);
        return -1;
    }
    r = po->po_bpf_open(channel, libPtr);
    netdemand_leave(&packet_demand);
    return r;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(long, bpf_close,
         AROS_LHA(long, handle, D0),
         struct SocketBase *, libPtr, 62, UL)
{
    AROS_LIBFUNC_INIT

    const struct packet_ops *po;
    long r;

    if((po = packet_running()) == NULL) {
        writeErrnoValue(libPtr, EBADF);
        return -1;
    }
    r = po->po_bpf_close(handle, libPtr);
    netdemand_leave(&packet_demand);
    return r;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(long, bpf_read,
         AROS_LHA(long, handle, D0),
         AROS_LHA(void *, buffer, A0),
         AROS_LHA(long, len, D1),
         struct SocketBase *, libPtr, 63, UL)
{
    AROS_LIBFUNC_INIT

    const struct packet_ops *po;
    long r;

    if((po = packet_running()) == NULL) {
        writeErrnoValue(libPtr, EBADF);
        return -1;
    }
    r = po->po_bpf_read(handle, buffer, len, libPtr);
    netdemand_leave(&packet_demand);
    return r;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(long, bpf_write,
         AROS_LHA(long, handle, D0),
         AROS_LHA(void *, buffer, A0),
         AROS_LHA(long, len, D1),
         struct SocketBase *, libPtr, 64, UL)
{
    AROS_LIBFUNC_INIT

    const struct packet_ops *po;
    long r;

    if((po = packet_running()) == NULL) {
        writeErrnoValue(libPtr, EBADF);
        return -1;
    }
    r = po->po_bpf_write(handle, buffer, len, libPtr);
    netdemand_leave(&packet_demand);
    return r;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(long, bpf_set_notify_mask,
         AROS_LHA(long, handle, D0),
         AROS_LHA(unsigned long, mask, D1),
         struct SocketBase *, libPtr, 65, UL)
{
    AROS_LIBFUNC_INIT

    const struct packet_ops *po;
    long r;

    if((po = packet_running()) == NULL) {
        writeErrnoValue(libPtr, EBADF);
        return -1;
    }
    r = po->po_bpf_set_notify_mask(handle, mask, libPtr);
    netdemand_leave(&packet_demand);
    return r;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(long, bpf_set_interrupt_mask,
         AROS_LHA(long, handle, D0),
         AROS_LHA(unsigned long, mask, D1),
         struct SocketBase *, libPtr, 66, UL)
{
    AROS_LIBFUNC_INIT

    const struct packet_ops *po;
    long r;

    if((po = packet_running()) == NULL) {
        writeErrnoValue(libPtr, EBADF);
        return -1;
    }
    r = po->po_bpf_set_interrupt_mask(handle, mask, libPtr);
    netdemand_leave(&packet_demand);
    return r;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(long, bpf_ioctl,
         AROS_LHA(long, handle, D0),
         AROS_LHA(unsigned long, request, D1),
         AROS_LHA(char *, argp, A0),
         struct SocketBase *, libPtr, 67, UL)
{
    AROS_LIBFUNC_INIT

    const struct packet_ops *po;
    long r;

    if((po = packet_running()) == NULL) {
        writeErrnoValue(libPtr, EBADF);
        return -1;
    }
    r = po->po_bpf_ioctl(handle, request, argp, libPtr);
    netdemand_leave(&packet_demand);
    return r;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(long, bpf_data_waiting,
         AROS_LHA(long, handle, D0),
         struct SocketBase *, libPtr, 68, UL)
{
    AROS_LIBFUNC_INIT

    const struct packet_ops *po;
    long r;

    if((po = packet_running()) == NULL) {
        writeErrnoValue(libPtr, EBADF);
        return -1;
    }
    r = po->po_bpf_data_waiting(handle, libPtr);
    netdemand_leave(&packet_demand);
    return r;

    AROS_LIBFUNC_EXIT
}

#endif /* __CONFIG_ROADSHOW__ */
