/*
 * Copyright (C) 2005-2026, The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * bsdsocket.library: the packet capture calls of the Roadshow interface
 * (bpf_open() ... bpf_data_waiting(), vectors 61-68) - their bodies, part
 * of the packet services module (net/packet_ops.h). The vectors stay in the
 * core (net/packet_stubs.c) and call these through the module's table; each
 * answers as the vector always did.
 */

#include <conf.h>

#ifdef __CONFIG_ROADSHOW__

#include <exec/types.h>
#include <proto/exec.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/synch.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <net/if.h>
#include "../net/bpf.h"
#include <net/if_sana.h>
#include <api/amiga_api.h>
#include <net/packet_ops.h>

#include <string.h>

#include <sys/systm.h>
#include <net/netcore_map.h>        /* always the last include */

/* BPF kernel functions (from net/bpf.c) */
extern int  bpf_allocate(void);
extern struct bpf_d *bpf_lookup(int handle);
extern void bpf_freed(int handle);
extern void bpf_rotate(struct bpf_d *d);
extern int  bpf_setif(struct bpf_d *d, const char *ifname);
extern int  bpf_setf(struct bpf_d *d, struct bpf_program *fp);

long
pkt_bpf_open(long channel, struct SocketBase *libPtr)
{
    int handle;
    struct bpf_d *d;

    handle = bpf_allocate();
    if(handle < 0) {
        writeErrnoValue(libPtr, ENOMEM);
        return -1;
    }

    d = bpf_lookup(handle);
    if(d != NULL) {
        d->bd_sigtask = FindTask(NULL);	/* the caller: any task of a shared base */
    }

    return handle;
}

long
pkt_bpf_close(long handle, struct SocketBase *libPtr)
{
    if(bpf_lookup(handle) == NULL) {
        writeErrnoValue(libPtr, EBADF);
        return -1;
    }

    bpf_freed(handle);
    return 0;
}

long
pkt_bpf_read(long handle, void *buffer, long len, struct SocketBase *libPtr)
{
    struct bpf_d *d;
    int copylen;
    spl_t s;

    if(len < 0) {
        writeErrnoValue(libPtr, EINVAL);
        return -1;
    }

    d = bpf_lookup(handle);
    if(d == NULL) {
        writeErrnoValue(libPtr, EBADF);
        return -1;
    }
    if(d->bd_ifp == NULL) {
        writeErrnoValue(libPtr, ENXIO);
        return -1;
    }

    s = splnet();

    /*
     * If hold buffer is empty but store has data, rotate.
     * This makes accumulated packets available for reading.
     */
    if(d->bd_hlen == 0 && d->bd_slen > 0)
        bpf_rotate(d);

    if(d->bd_hlen == 0) {
        splx(s);
        /* No data available */
        writeErrnoValue(libPtr, EWOULDBLOCK);
        return -1;
    }

    /* Copy from hold buffer to user buffer */
    copylen = d->bd_hlen;
    if(copylen > len)
        copylen = len;

    memcpy(buffer, d->bd_hbuf, copylen);

    /* Mark hold buffer as consumed */
    d->bd_hlen = 0;
    splx(s);

    return copylen;
}

long
pkt_bpf_write(long handle, void *buffer, long len, struct SocketBase *libPtr)
{
    struct bpf_d *d;
    struct mbuf *m;
    struct ifnet *ifp;
    struct sockaddr dst;
    int error;

    if(len < 0) {
        writeErrnoValue(libPtr, EINVAL);
        return -1;
    }

    d = bpf_lookup(handle);
    if(d == NULL) {
        writeErrnoValue(libPtr, EBADF);
        return -1;
    }
    if(d->bd_ifp == NULL) {
        writeErrnoValue(libPtr, ENXIO);
        return -1;
    }

    ifp = d->bd_ifp;

    /*
     * The buffer is a whole frame, link header included, sent as it is
     * (SANA2IOF_RAW): what an Ethernet descriptor captures (DLT_EN10MB).
     * A DLT_RAW capture has no link header to write one with.
     */
    if(d->bd_dlt != DLT_EN10MB) {
        writeErrnoValue(libPtr, EAFNOSUPPORT);
        return -1;
    }
    if(len < SANA_ETHER_HDRLEN) {
        writeErrnoValue(libPtr, EINVAL);
        return -1;
    }
    if(len > (long)ifp->if_mtu + SANA_ETHER_HDRLEN) {
        writeErrnoValue(libPtr, EMSGSIZE);
        return -1;
    }

    /* Copy the frame into an mbuf chain: a large MTU takes several clusters */
    m = m_gethdr(M_DONTWAIT, MT_DATA);
    if(m == NULL) {
        writeErrnoValue(libPtr, ENOBUFS);
        return -1;
    }
    m->m_len = 0;
    m->m_pkthdr.len = len;
    m->m_pkthdr.rcvif = NULL;
    {
        struct mbuf *n = m;
        long done = 0;

        for(;;) {
            long room, chunk;

            if(len - done > (long)(n == m ? MHLEN : MLEN)) {
                MCLGET(n, M_DONTWAIT);
                if((n->m_flags & M_EXT) == 0) {
                    m_freem(m);
                    writeErrnoValue(libPtr, ENOBUFS);
                    return -1;
                }
                room = MCLBYTES;
            } else {
                room = (n == m) ? MHLEN : MLEN;
            }
            chunk = (len - done < room) ? len - done : room;
            memcpy(mtod(n, char *), (char *)buffer + done, chunk);
            n->m_len = chunk;
            done += chunk;
            if(done == len)
                break;
            MGET(n->m_next, M_DONTWAIT, MT_DATA);
            if(n->m_next == NULL) {
                m_freem(m);
                writeErrnoValue(libPtr, ENOBUFS);
                return -1;
            }
            n = n->m_next;
        }
    }

    /* Send as raw packet via AF_UNSPEC */
    memset(&dst, 0, sizeof(dst));
    dst.sa_family = AF_UNSPEC;
    dst.sa_len = sizeof(dst);

    error = (*ifp->if_output)(ifp, m, &dst, NULL);
    if(error) {
        writeErrnoValue(libPtr, error);
        return -1;
    }

    return len;
}

long
pkt_bpf_set_notify_mask(long handle, unsigned long mask, struct SocketBase *libPtr)
{
    struct bpf_d *d;

    d = bpf_lookup(handle);
    if(d == NULL) {
        writeErrnoValue(libPtr, EBADF);
        return -1;
    }

    d->bd_notifymask = mask;
    d->bd_sigtask = FindTask(NULL);	/* the caller: any task of a shared base */
    return 0;
}

long
pkt_bpf_set_interrupt_mask(long handle, unsigned long mask, struct SocketBase *libPtr)
{
    struct bpf_d *d;

    d = bpf_lookup(handle);
    if(d == NULL) {
        writeErrnoValue(libPtr, EBADF);
        return -1;
    }

    d->bd_intrmask = mask;
    d->bd_sigtask = FindTask(NULL);	/* the caller: any task of a shared base */
    return 0;
}

long
pkt_bpf_ioctl(long handle, unsigned long request, char *argp, struct SocketBase *libPtr)
{
    struct bpf_d *d;
    int error = 0;

    d = bpf_lookup(handle);
    if(d == NULL) {
        writeErrnoValue(libPtr, EBADF);
        return -1;
    }

    switch(request) {

    /*
     * Roadshow-compatible: query bytes available for reading.
     */
    case FIONREAD:
        *(long *)argp = (d->bd_hlen > 0) ? d->bd_hlen : d->bd_slen;
        break;

    /*
     * Roadshow-compatible: get interface address.
     */
    case SIOCGIFADDR: {
        struct sockaddr_in *sin = (struct sockaddr_in *)argp;
        struct ifaddr *ifa;

        if(d->bd_ifp == NULL) {
            error = ENXIO;
            break;
        }
        /* Find the first AF_INET address on this interface */
        for(ifa = d->bd_ifp->if_addrlist; ifa != NULL; ifa = ifa->ifa_next) {
            if(ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
                memcpy(sin, ifa->ifa_addr, sizeof(struct sockaddr_in));
                break;
            }
        }
        if(ifa == NULL)
            error = EADDRNOTAVAIL;
        break;
    }

    case BIOCGBLEN:
        *(u_int *)argp = d->bd_bufsize;
        break;

    case BIOCSBLEN: {
        u_int size = *(u_int *)argp;

        if(d->bd_sbuf != NULL) {
            error = EINVAL;	/* already allocated */
            break;
        }
        if(size < BPF_MINBUFSIZE)
            size = BPF_MINBUFSIZE;
        if(size > BPF_MAXBUFSIZE)
            size = BPF_MAXBUFSIZE;
        d->bd_bufsize = size;
        *(u_int *)argp = size;
        break;
    }

    case BIOCSETF:
        error = bpf_setf(d, (struct bpf_program *)argp);
        break;

    case BIOCFLUSH: {
        spl_t s = splnet();
        d->bd_slen = 0;
        d->bd_hlen = 0;
        splx(s);
        break;
    }

    case BIOCPROMISC:
        if(d->bd_ifp == NULL) {
            error = ENXIO;
            break;
        }
        if(!d->bd_promisc) {
            d->bd_promisc = 1;
            d->bd_ifp->if_pcount++;
            d->bd_ifp->if_flags |= IFF_PROMISC;
        }
        break;

    case BIOCGDLT:
        if(d->bd_ifp == NULL) {
            error = ENXIO;
            break;
        }
        *(u_int *)argp = d->bd_dlt;
        break;

    case BIOCGETIF:
        if(d->bd_ifp == NULL) {
            error = ENXIO;
            break;
        }
        if_fullname(d->bd_ifp, argp, IFNAMSIZ);
        break;

    case BIOCSETIF:
        error = bpf_setif(d, argp);
        break;

    case BIOCGSTATS: {
        struct bpf_stat *bs = (struct bpf_stat *)argp;
        bs->bs_recv = d->bd_rcount;
        bs->bs_drop = d->bd_dcount;
        break;
    }

    case BIOCIMMEDIATE:
        d->bd_immediate = *(u_int *)argp ? 1 : 0;
        break;

    case BIOCVERSION: {
        struct bpf_version *bv = (struct bpf_version *)argp;
        bv->bv_major = BPF_MAJOR_VERSION;
        bv->bv_minor = BPF_MINOR_VERSION;
        break;
    }

    case BIOCGHDRCMPLT:
        *(u_int *)argp = d->bd_hdrcmplt;
        break;

    case BIOCSHDRCMPLT:
        d->bd_hdrcmplt = *(u_int *)argp ? 1 : 0;
        break;

    case BIOCGSEESENT:
        *(u_int *)argp = d->bd_seesent;
        break;

    case BIOCSSEESENT:
        d->bd_seesent = *(u_int *)argp ? 1 : 0;
        break;

    case BIOCSRTIMEOUT:
        d->bd_rtimeout = *(struct timeval *)argp;
        break;

    case BIOCGRTIMEOUT:
        *(struct timeval *)argp = d->bd_rtimeout;
        break;

    default:
        error = EINVAL;
        break;
    }

    if(error) {
        writeErrnoValue(libPtr, error);
        return -1;
    }
    return 0;
}

long
pkt_bpf_data_waiting(long handle, struct SocketBase *libPtr)
{
    struct bpf_d *d;

    d = bpf_lookup(handle);
    if(d == NULL) {
        writeErrnoValue(libPtr, EBADF);
        return -1;
    }

    /* Return number of bytes available for reading */
    if(d->bd_hlen > 0)
        return d->bd_hlen;
    return d->bd_slen;
}

#endif
