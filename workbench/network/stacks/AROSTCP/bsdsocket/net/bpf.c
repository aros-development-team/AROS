/*
 * Copyright (c) 1990, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (C) 2005-2026 The AROS Dev Team
 *
 * BPF core: descriptor management, packet capture, and tap functions.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/synch.h>

#include <kern/amiga_includes.h>

#include <net/if.h>
#include <net/bpf.h>
#include <net/if_sana.h>

#include <string.h>
#include <stdio.h>
#include <syslog.h>

extern struct ifnet *ifnet;

/* Global descriptor table */
static struct bpf_d bpf_dtab[BPF_MAXDEVICES];
static int bpf_initialized = 0;

/*
 * Initialize the BPF subsystem.
 */
void
bpf_init(void)
{
    memset(bpf_dtab, 0, sizeof(bpf_dtab));
    bpf_initialized = 1;
}

/*
 * Clean up the BPF subsystem.
 */
void
bpf_cleanup(void)
{
    int i;

    for(i = 0; i < BPF_MAXDEVICES; i++) {
        struct bpf_d *d = &bpf_dtab[i];
        if(d->bd_inuse) {
            if(d->bd_sbuf)
                bsd_free(d->bd_sbuf, NULL);
            if(d->bd_hbuf)
                bsd_free(d->bd_hbuf, NULL);
            if(d->bd_filter)
                bsd_free(d->bd_filter, NULL);
            d->bd_inuse = 0;
        }
    }
    bpf_initialized = 0;
}

/*
 * Allocate a BPF descriptor.
 * Returns descriptor index (0..BPF_MAXDEVICES-1) or -1 on failure.
 */
int
bpf_allocate(void)
{
    int i;
    spl_t s;

    if(!bpf_initialized)
        bpf_init();

    s = splnet();
    for(i = 0; i < BPF_MAXDEVICES; i++) {
        if(!bpf_dtab[i].bd_inuse) {
            struct bpf_d *d = &bpf_dtab[i];
            memset(d, 0, sizeof(*d));
            d->bd_inuse = 1;
            d->bd_bufsize = BPF_DFLTBUFSIZE;
            d->bd_seesent = 1;
            splx(s);
            return i;
        }
    }
    splx(s);
    return -1;
}

/*
 * Look up a BPF descriptor by handle.
 * Returns pointer to descriptor, or NULL if invalid.
 */
struct bpf_d *
bpf_lookup(int handle)
{
    if(handle < 0 || handle >= BPF_MAXDEVICES)
        return NULL;
    if(!bpf_dtab[handle].bd_inuse)
        return NULL;
    return &bpf_dtab[handle];
}

/*
 * Free a BPF descriptor and all its resources.
 */
void
bpf_freed(int handle)
{
    struct bpf_d *d;
    spl_t s;

    d = bpf_lookup(handle);
    if(d == NULL)
        return;

    s = splnet();

    /* Detach from interface */
    if(d->bd_ifp != NULL) {
        d->bd_ifp->if_bpf = NULL;
        if(d->bd_promisc) {
            d->bd_ifp->if_pcount--;
            d->bd_ifp->if_flags &= ~IFF_PROMISC;
        }
        d->bd_ifp = NULL;
    }

    /* Free buffers */
    if(d->bd_sbuf) {
        bsd_free(d->bd_sbuf, NULL);
        d->bd_sbuf = NULL;
    }
    if(d->bd_hbuf) {
        bsd_free(d->bd_hbuf, NULL);
        d->bd_hbuf = NULL;
    }

    /* Free filter */
    if(d->bd_filter) {
        bsd_free(d->bd_filter, NULL);
        d->bd_filter = NULL;
    }

    d->bd_inuse = 0;
    splx(s);
}

/*
 * Allocate the capture buffers for a descriptor.
 * Must be called before attaching to an interface.
 * Returns 0 on success, errno on failure.
 */
static int
bpf_allocbufs(struct bpf_d *d)
{
    if(d->bd_sbuf)
        bsd_free(d->bd_sbuf, NULL);
    if(d->bd_hbuf)
        bsd_free(d->bd_hbuf, NULL);

    d->bd_sbuf = bsd_malloc(d->bd_bufsize, NULL, NULL);
    d->bd_hbuf = bsd_malloc(d->bd_bufsize, NULL, NULL);

    if(d->bd_sbuf == NULL || d->bd_hbuf == NULL) {
        if(d->bd_sbuf) {
            bsd_free(d->bd_sbuf, NULL);
            d->bd_sbuf = NULL;
        }
        if(d->bd_hbuf) {
            bsd_free(d->bd_hbuf, NULL);
            d->bd_hbuf = NULL;
        }
        return ENOMEM;
    }

    d->bd_slen = 0;
    d->bd_hlen = 0;
    return 0;
}

/*
 * Rotate buffers: move store to hold, reset store.
 */
void
bpf_rotate(struct bpf_d *d)
{
    caddr_t tmp;

    tmp = d->bd_hbuf;
    d->bd_hbuf = d->bd_sbuf;
    d->bd_hlen = d->bd_slen;
    d->bd_sbuf = tmp;
    d->bd_slen = 0;
}

/*
 * Determine the DLT type for an interface based on its hardware type.
 */
static int
bpf_dlt_for_ifp(struct ifnet *ifp)
{
    struct sana_softc *ssc = (struct sana_softc *)ifp;

    switch(ssc->ss_hwtype) {
    case 1:		/* IFT_ETHER */
        return DLT_EN10MB;
    case 24:	/* IFT_LOOP / loopback */
        return DLT_NULL;
    case 23:	/* IFT_PPP */
        return DLT_PPP;
    case 28:	/* IFT_SLIP */
        return DLT_SLIP;
    default:
        return DLT_EN10MB;	/* default to Ethernet */
    }
}

/*
 * Attach a descriptor to a network interface.
 * Returns 0 on success, errno on failure.
 */
int
bpf_setif(struct bpf_d *d, const char *ifname)
{
    struct ifnet *ifp;
    spl_t s;

    /* Find the interface by name */
    for(ifp = ifnet; ifp != NULL; ifp = ifp->if_next) {
        char name[IFNAMSIZ];
        snprintf(name, sizeof(name), "%s%d", ifp->if_name, ifp->if_unit);
        if(strcmp(name, ifname) == 0)
            break;
    }
    if(ifp == NULL)
        return ENXIO;

    s = splnet();

    /* Detach from current interface if any */
    if(d->bd_ifp != NULL) {
        /* Only clear if_bpf if we own it */
        if(d->bd_ifp->if_bpf == (caddr_t)d)
            d->bd_ifp->if_bpf = NULL;
        if(d->bd_promisc) {
            d->bd_ifp->if_pcount--;
            d->bd_ifp->if_flags &= ~IFF_PROMISC;
            d->bd_promisc = 0;
        }
    }

    /* Allocate buffers if not yet done */
    if(d->bd_sbuf == NULL) {
        int error = bpf_allocbufs(d);
        if(error) {
            splx(s);
            return error;
        }
    }

    /* Attach to new interface */
    d->bd_ifp = ifp;
    d->bd_dlt = bpf_dlt_for_ifp(ifp);

    /*
     * Set if_bpf to point to us.  In a full BSD BPF this would be
     * a linked list; here we support one active BPF listener per
     * interface.  Multiple descriptors can still be open — they just
     * capture from different interfaces.
     */
    ifp->if_bpf = (caddr_t)d;

    /* Flush existing data */
    d->bd_slen = 0;
    d->bd_hlen = 0;

    splx(s);
    return 0;
}

/*
 * Set the filter program on a descriptor.
 * Returns 0 on success, errno on failure.
 */
int
bpf_setf(struct bpf_d *d, struct bpf_program *fp)
{
    struct bpf_insn *fcode;
    u_int flen;
    spl_t s;

    flen = fp->bf_len;

    /* A zero-length filter accepts everything */
    if(flen == 0) {
        s = splnet();
        if(d->bd_filter) {
            bsd_free(d->bd_filter, NULL);
            d->bd_filter = NULL;
        }
        d->bd_flen = 0;
        splx(s);
        return 0;
    }

    if(flen > BPF_MAXINSNS)
        return EINVAL;

    /* Validate the program */
    if(!bpf_validate(fp->bf_insns, (int)flen))
        return EINVAL;

    /* Copy the filter program */
    fcode = bsd_malloc(flen * sizeof(struct bpf_insn), NULL, NULL);
    if(fcode == NULL)
        return ENOMEM;

    memcpy(fcode, fp->bf_insns, flen * sizeof(struct bpf_insn));

    s = splnet();
    if(d->bd_filter)
        bsd_free(d->bd_filter, NULL);
    d->bd_filter = fcode;
    d->bd_flen = flen;
    splx(s);

    return 0;
}

/*
 * catchpacket: store a packet in the descriptor's store buffer.
 * Called from bpf_tap/bpf_mtap with splnet held.
 */
static void
catchpacket(struct bpf_d *d, const u_char *pkt, u_int pktlen, u_int snaplen)
{
    struct bpf_hdr *hp;
    int totlen, curlen, hdrlen;

    hdrlen = BPF_WORDALIGN(sizeof(struct bpf_hdr));
    if(snaplen > pktlen)
        snaplen = pktlen;
    totlen = BPF_WORDALIGN(hdrlen + snaplen);

    curlen = d->bd_slen;

    /* If store buffer is full, rotate */
    if(curlen + totlen > d->bd_bufsize) {
        if(d->bd_hlen != 0) {
            /* Hold buffer still has unread data — drop */
            d->bd_dcount++;
            return;
        }
        bpf_rotate(d);
        curlen = 0;
    }

    /* Append packet to store buffer */
    hp = (struct bpf_hdr *)(d->bd_sbuf + curlen);
    GetSysTime(&hp->bh_tstamp);
    hp->bh_datalen = pktlen;
    hp->bh_caplen = snaplen;
    hp->bh_hdrlen = hdrlen;

    memcpy((char *)hp + hdrlen, pkt, snaplen);

    d->bd_slen = curlen + totlen;
    d->bd_ccount++;

    /* In immediate mode, rotate immediately so data is readable */
    if(d->bd_immediate && d->bd_hlen == 0)
        bpf_rotate(d);

    /* Signal the waiting task if notification mask is set */
    if(d->bd_notifymask && d->bd_sigtask)
        Signal(d->bd_sigtask, d->bd_notifymask);
}

/*
 * bpf_tap: called from network interface input/output path for
 * contiguous packet buffers (non-mbuf).
 *
 * ifp      — interface the packet arrived on / is being sent from
 * pkt      — pointer to packet data
 * pktlen   — length of packet data
 * direction — BPF_D_IN for incoming, BPF_D_OUT for outgoing
 */
void
bpf_tap(struct ifnet *ifp, u_char *pkt, u_int pktlen, int direction)
{
    struct bpf_d *d;
    u_int slen;
    spl_t s;

    if(ifp->if_bpf == NULL)
        return;

    d = (struct bpf_d *)ifp->if_bpf;
    if(!d->bd_inuse)
        return;

    /* Skip outgoing packets if not configured to see them */
    if(direction == BPF_D_OUT && !d->bd_seesent)
        return;

    d->bd_rcount++;

    s = splnet();
    slen = bpf_filter(d->bd_filter, pkt, pktlen, pktlen);
    if(slen != 0)
        catchpacket(d, pkt, pktlen, slen);
    splx(s);
}

/*
 * bpf_mtap: called from network interface input/output path for
 * mbuf chain packets.
 *
 * ifp       — interface
 * m         — mbuf chain
 * direction — BPF_D_IN or BPF_D_OUT
 */
void
bpf_mtap(struct ifnet *ifp, struct mbuf *m, int direction)
{
    struct bpf_d *d;
    u_int pktlen, slen;
    u_char *buf;
    spl_t s;

    if(ifp->if_bpf == NULL)
        return;

    d = (struct bpf_d *)ifp->if_bpf;
    if(!d->bd_inuse)
        return;

    if(direction == BPF_D_OUT && !d->bd_seesent)
        return;

    d->bd_rcount++;

    pktlen = m->m_pkthdr.len;
    if(pktlen == 0)
        return;

    s = splnet();

    /*
     * If the mbuf is contiguous (single mbuf or data fits in first),
     * filter directly.  Otherwise, linearize into a temporary buffer.
     */
    if(m->m_next == NULL) {
        buf = mtod(m, u_char *);
        slen = bpf_filter(d->bd_filter, buf, pktlen, m->m_len);
        if(slen != 0)
            catchpacket(d, buf, pktlen, slen);
    } else {
        /* Linearize: copy mbuf chain into contiguous buffer */
        buf = bsd_malloc(pktlen, NULL, NULL);
        if(buf != NULL) {
            m_copydata(m, 0, (int)pktlen, (caddr_t)buf);
            slen = bpf_filter(d->bd_filter, buf, pktlen, pktlen);
            if(slen != 0)
                catchpacket(d, buf, pktlen, slen);
            bsd_free(buf, NULL);
        }
    }
    splx(s);
}
