/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef NET_PACKET_OPS_H
#define NET_PACKET_OPS_H

/*
 * THE PACKET SERVICES MODULE ("packet", packet.atcp): the
 * filter and NAT engine (net/ipfilter.c), packet capture (net/bpf.c,
 * net/bpf_filter.c) and the capture calls of bsdsocket.library
 * (api/rs_bpf.c). Loaded on first use: by the first filter or NAT request
 * (the ioctl range 'r' 60-95, kern/net_demand.c) or the first bpf_open().
 *
 * What stays in the core (net/packet_stubs.c), whatever is loaded:
 *  - the eight capture vectors (bpf_open() ... bpf_data_waiting(), 61-68),
 *    which call through the table below - bpf_open() loads the module, the
 *    others answer EBADF while it does not run (no handle can be open);
 *  - the capture tap of the SANA-II interfaces: two pointers the module
 *    sets while it runs (nettap_set()); an interface's if_bpf is set only
 *    while a capture is open on it, and the module cannot be stopped while
 *    one is, so the pointers are good whenever if_bpf is not NULL.
 */

#include <exec/types.h>
#include <sys/types.h>

#define PACKET_MODULE   "packet"

struct SocketBase;
struct ifnet;
struct mbuf;

/* the capture calls, as the vectors' C bodies (api/rs_bpf.c) */
struct packet_ops {
    unsigned short po_size;     /* sizeof(struct packet_ops) */
    long (*po_bpf_open)(long channel, struct SocketBase *libPtr);
    long (*po_bpf_close)(long handle, struct SocketBase *libPtr);
    long (*po_bpf_read)(long handle, void *buffer, long len, struct SocketBase *libPtr);
    long (*po_bpf_write)(long handle, void *buffer, long len, struct SocketBase *libPtr);
    long (*po_bpf_set_notify_mask)(long handle, unsigned long mask, struct SocketBase *libPtr);
    long (*po_bpf_set_interrupt_mask)(long handle, unsigned long mask,
                                      struct SocketBase *libPtr);
    long (*po_bpf_ioctl)(long handle, unsigned long request, char *argp,
                         struct SocketBase *libPtr);
    long (*po_bpf_data_waiting)(long handle, struct SocketBase *libPtr);
};

/* api/rs_bpf.c: the bodies */
long pkt_bpf_open(long channel, struct SocketBase *libPtr);
long pkt_bpf_close(long handle, struct SocketBase *libPtr);
long pkt_bpf_read(long handle, void *buffer, long len, struct SocketBase *libPtr);
long pkt_bpf_write(long handle, void *buffer, long len, struct SocketBase *libPtr);
long pkt_bpf_set_notify_mask(long handle, unsigned long mask, struct SocketBase *libPtr);
long pkt_bpf_set_interrupt_mask(long handle, unsigned long mask, struct SocketBase *libPtr);
long pkt_bpf_ioctl(long handle, unsigned long request, char *argp, struct SocketBase *libPtr);
long pkt_bpf_data_waiting(long handle, struct SocketBase *libPtr);

/* net/bpf.c: how many capture handles are open (a module that has one cannot stop) */
int  bpf_busy(void);

/*
 * THE CAPTURE TAP (net/packet_stubs.c, core). Called by the SANA-II paths
 * after they found if_bpf set; they do nothing while no module set them.
 * nettap_set(): the module, in its start (the two functions) and its stop
 * (NULL, NULL: EBUSY while an interface still has a capture on it).
 */
typedef void (*nettap_mtap_fn)(struct ifnet *ifp, struct mbuf *m, int direction);
typedef void (*nettap_mtap2_fn)(struct ifnet *ifp, const void *hdr, u_int hlen,
                                struct mbuf *m, int direction);

extern nettap_mtap_fn   nettap_mtap;
extern nettap_mtap2_fn  nettap_mtap2;

int  nettap_set(nettap_mtap_fn mtap, nettap_mtap2_fn mtap2);

#endif /* !NET_PACKET_OPS_H */
