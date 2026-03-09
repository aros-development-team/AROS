/*
 * Copyright (C) 2005-2026, The AROS Development Team.
 *
 * This file contains Roadshow-specific functions which are not required for MorphOS build
 */

#include <conf.h>

#ifdef __CONFIG_ROADSHOW__

#include <aros/libcall.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <libraries/bsdsocket.h>
#include <utility/tagitem.h>
#include <proto/utility.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/synch.h>
#include <netinet/in.h>
#include <net/route.h>
#include <net/if.h>
#include <net/radix.h>
#include <net/bpf.h>
#include <net/if_sana.h>
#include <sys/ioctl.h>
#include <protos/net/route_protos.h>
#include <syslog.h>
#include <api/amiga_api.h>
#include <api/apicalls.h>
#include <kern/amiga_netdb.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <netdb.h>

#include <api/gethtbynamadr.h>

#include <sys/errno.h>

/* BPF kernel functions (from net/bpf.c) */
extern int  bpf_allocate(void);
extern struct bpf_d *bpf_lookup(int handle);
extern void bpf_freed(int handle);
extern void bpf_rotate(struct bpf_d *d);
extern int  bpf_setif(struct bpf_d *d, const char *ifname);
extern int  bpf_setf(struct bpf_d *d, struct bpf_program *fp);

extern struct ifnet *ifnet;

extern struct hostent *__gethostbyname(const char *name, struct SocketBase *);
extern struct hostent *__gethostbyname2(const char *name, int af, struct SocketBase *);
extern struct hostent *__gethostbyname_r(const char *name,
    struct hostent *result, char *buf, LONG buflen, LONG *h_errnop,
    struct SocketBase *libPtr);
extern struct hostent *__gethostbyaddr_r(const char *addr, LONG len, LONG type,
    struct hostent *result, char *buf, LONG buflen, LONG *h_errnop,
    struct SocketBase *libPtr);

/* Internal implementation functions */
extern const char *__inet_ntop(int af, const void *src, char *dst,
                               socklen_t size, struct SocketBase *SocketBase);
extern int __inet_pton(int af, const char *src, void *dst,
                       struct SocketBase *SocketBase);

/* Kernel network functions */
extern int in_localaddr(struct in_addr);
extern int in_canforward(struct in_addr);

extern struct ServentNode *findServentNode(struct NetDataBase *ndb,
                                           const char *name, const char *proto);

/*
 * Helper: fill a sockaddr_in from a network-byte-order IPv4 address.
 */
static inline void
fill_sockaddr_in(struct sockaddr_in *sa, ULONG addr)
{
    memset(sa, 0, sizeof(*sa));
    sa->sin_family = AF_INET;
    sa->sin_len    = sizeof(struct sockaddr_in);
    sa->sin_addr.s_addr = addr;
}

/*
 * Parse RTA_* route tags into destination, gateway, netmask, and flags.
 * Returns 0 on success, or an errno value on failure.
 */
static int
parse_route_tags(struct TagItem *tags, ULONG *dst_out, ULONG *gw_out,
                 ULONG *mask_out, ULONG *flags_out)
{
    struct TagItem *tag;
    struct TagItem *tstate = tags;
    ULONG dst = 0, gw = 0, mask = 0;
    ULONG flags = RTF_UP | RTF_STATIC;
    int have_dst = 0;

    if(tags == NULL)
        return EINVAL;

    while((tag = NextTagItem(&tstate)) != NULL) {
        switch(tag->ti_Tag) {
        case RTA_Destination:
            dst = (ULONG)tag->ti_Data;
            have_dst = 1;
            break;
        case RTA_DestinationHost:
            dst = (ULONG)tag->ti_Data;
            flags |= RTF_HOST;
            mask = 0xFFFFFFFF;
            have_dst = 1;
            break;
        case RTA_DestinationNet:
            dst = (ULONG)tag->ti_Data;
            flags &= ~RTF_HOST;
            /* Derive classful netmask from destination address */
            {
                ULONG ha = ntohl(dst);
                if((ha & 0x80000000) == 0)
                    mask = htonl(0xFF000000);       /* Class A */
                else if((ha & 0xC0000000) == 0x80000000)
                    mask = htonl(0xFFFF0000);       /* Class B */
                else
                    mask = htonl(0xFFFFFF00);       /* Class C */
            }
            have_dst = 1;
            break;
        case RTA_Gateway:
            gw = (ULONG)tag->ti_Data;
            if(gw != 0)
                flags |= RTF_GATEWAY;
            break;
        case RTA_DefaultGateway:
            dst = INADDR_ANY;
            mask = INADDR_ANY;
            gw = (ULONG)tag->ti_Data;
            flags |= RTF_GATEWAY;
            flags &= ~RTF_HOST;
            have_dst = 1;
            break;
        }
    }

    if(!have_dst)
        return EINVAL;

    /* Default to host route if no mask specified and not a default route */
    if(!(flags & RTF_HOST) && mask == 0 && dst != INADDR_ANY) {
        flags |= RTF_HOST;
        mask = 0xFFFFFFFF;
    }

    *dst_out   = dst;
    *gw_out    = gw;
    *mask_out  = mask;
    *flags_out = flags;
    return 0;
}

/* Format an in_addr as a dotted-decimal string into buf (must be >= 16 bytes) */
static void fmt_ipaddr(char *buf, struct in_addr addr)
{
    unsigned char *p = (unsigned char *)&addr.s_addr;
    snprintf(buf, 16, "%u.%u.%u.%u", p[0], p[1], p[2], p[3]);
}

AROS_LH1(long, bpf_open,
         AROS_LHA(long, channel, D0),
         struct SocketBase *, libPtr, 61, UL)
{
    AROS_LIBFUNC_INIT

    int handle;
    struct bpf_d *d;

    handle = bpf_allocate();
    if(handle < 0) {
        writeErrnoValue(libPtr, ENOMEM);
        return -1;
    }

    d = bpf_lookup(handle);
    if(d != NULL) {
        d->bd_sigtask = libPtr->thisTask;
    }

    return handle;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(long, bpf_close,
         AROS_LHA(long, handle, D0),
         struct SocketBase *, libPtr, 62, UL)
{
    AROS_LIBFUNC_INIT

    if(bpf_lookup(handle) == NULL) {
        writeErrnoValue(libPtr, EBADF);
        return -1;
    }

    bpf_freed(handle);
    return 0;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(long, bpf_read,
         AROS_LHA(long, handle, D0),
         AROS_LHA(void *, buffer, A0),
         AROS_LHA(long, len, D1),
         struct SocketBase *, libPtr, 63, UL)
{
    AROS_LIBFUNC_INIT

    struct bpf_d *d;
    int copylen;
    spl_t s;

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

    AROS_LIBFUNC_EXIT
}

AROS_LH3(long, bpf_write,
         AROS_LHA(long, handle, D0),
         AROS_LHA(void *, buffer, A0),
         AROS_LHA(long, len, D1),
         struct SocketBase *, libPtr, 64, UL)
{
    AROS_LIBFUNC_INIT

    struct bpf_d *d;
    struct mbuf *m;
    struct ifnet *ifp;
    struct sockaddr dst;
    int error;

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

    if(len > (long)ifp->if_mtu) {
        writeErrnoValue(libPtr, EMSGSIZE);
        return -1;
    }

    /* Allocate mbuf with header and copy user data */
    m = m_gethdr(M_DONTWAIT, MT_DATA);
    if(m == NULL) {
        writeErrnoValue(libPtr, ENOBUFS);
        return -1;
    }
    if(len > (int)MHLEN) {
        MCLGET(m, M_DONTWAIT);
        if((m->m_flags & M_EXT) == 0) {
            m_freem(m);
            writeErrnoValue(libPtr, ENOBUFS);
            return -1;
        }
    }
    memcpy(mtod(m, char *), buffer, len);
    m->m_len = len;
    m->m_pkthdr.len = len;
    m->m_pkthdr.rcvif = NULL;

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

    AROS_LIBFUNC_EXIT
}

AROS_LH2(long, bpf_set_notify_mask,
         AROS_LHA(long, handle, D0),
         AROS_LHA(unsigned long, mask, D1),
         struct SocketBase *, libPtr, 65, UL)
{
    AROS_LIBFUNC_INIT

    struct bpf_d *d;

    d = bpf_lookup(handle);
    if(d == NULL) {
        writeErrnoValue(libPtr, EBADF);
        return -1;
    }

    d->bd_notifymask = mask;
    d->bd_sigtask = libPtr->thisTask;
    return 0;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(long, bpf_set_interrupt_mask,
         AROS_LHA(long, handle, D0),
         AROS_LHA(unsigned long, mask, D1),
         struct SocketBase *, libPtr, 66, UL)
{
    AROS_LIBFUNC_INIT

    struct bpf_d *d;

    d = bpf_lookup(handle);
    if(d == NULL) {
        writeErrnoValue(libPtr, EBADF);
        return -1;
    }

    d->bd_intrmask = mask;
    d->bd_sigtask = libPtr->thisTask;
    return 0;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(long, bpf_ioctl,
         AROS_LHA(long, handle, D0),
         AROS_LHA(unsigned long, request, D1),
         AROS_LHA(char *, argp, A0),
         struct SocketBase *, libPtr, 67, UL)
{
    AROS_LIBFUNC_INIT

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
        snprintf(argp, IFNAMSIZ, "%s%d",
                 d->bd_ifp->if_name, d->bd_ifp->if_unit);
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

    AROS_LIBFUNC_EXIT
}

AROS_LH1(long, bpf_data_waiting,
         AROS_LHA(long, handle, D0),
         struct SocketBase *, libPtr, 68, UL)
{
    AROS_LIBFUNC_INIT

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

    AROS_LIBFUNC_EXIT
}

AROS_LH1(long, AddRouteTagList,
         AROS_LHA(struct TagItem *, tags, A0),
         struct SocketBase *, libPtr, 69, UL)
{
    AROS_LIBFUNC_INIT

    struct sockaddr_in dst, gw, mask;
    ULONG dst_addr, gw_addr, mask_addr, flags;
    int error;

    error = parse_route_tags(tags, &dst_addr, &gw_addr, &mask_addr, &flags);
    if(error) {
        writeErrnoValue(libPtr, error);
        return -1;
    }

    fill_sockaddr_in(&dst, dst_addr);
    fill_sockaddr_in(&gw,  gw_addr);
    fill_sockaddr_in(&mask, mask_addr);

    error = rtrequest(RTM_ADD,
                      (struct sockaddr *)&dst,
                      (struct sockaddr *)&gw,
                      (struct sockaddr *)&mask,
                      (int)flags, NULL);
    if(error) {
        writeErrnoValue(libPtr, error);
        return -1;
    }
    return 0;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(long, DeleteRouteTagList,
         AROS_LHA(struct TagItem *, tags, A0),
         struct SocketBase *, libPtr, 70, UL)
{
    AROS_LIBFUNC_INIT

    struct sockaddr_in dst, mask;
    ULONG dst_addr, gw_addr, mask_addr, flags;
    int error;

    error = parse_route_tags(tags, &dst_addr, &gw_addr, &mask_addr, &flags);
    if(error) {
        writeErrnoValue(libPtr, error);
        return -1;
    }

    fill_sockaddr_in(&dst, dst_addr);
    fill_sockaddr_in(&mask, mask_addr);

    error = rtrequest(RTM_DELETE,
                      (struct sockaddr *)&dst,
                      NULL,
                      (struct sockaddr *)&mask,
                      0, NULL);
    if(error) {
        writeErrnoValue(libPtr, error);
        return -1;
    }
    return 0;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(long, ChangeRouteTagList,
         AROS_LHA(struct TagItem *, tags, A0),
         struct SocketBase *, libPtr, 71, UL)
{
    AROS_LIBFUNC_INIT

    struct sockaddr_in dst, gw, mask;
    ULONG dst_addr, gw_addr, mask_addr, flags;
    int error;

    error = parse_route_tags(tags, &dst_addr, &gw_addr, &mask_addr, &flags);
    if(error) {
        writeErrnoValue(libPtr, error);
        return -1;
    }

    fill_sockaddr_in(&dst, dst_addr);
    fill_sockaddr_in(&gw,  gw_addr);
    fill_sockaddr_in(&mask, mask_addr);

    /*
     * RTM_CHANGE is not supported by rtrequest(), so we perform
     * an atomic delete + add under a single splnet section.
     */
    {
        spl_t s = splnet();

        /* Delete the existing route (ignore ESRCH — it may not exist) */
        (void)rtrequest(RTM_DELETE,
                        (struct sockaddr *)&dst,
                        NULL,
                        (struct sockaddr *)&mask,
                        0, NULL);

        /* Re-add with the new parameters */
        error = rtrequest(RTM_ADD,
                          (struct sockaddr *)&dst,
                          (struct sockaddr *)&gw,
                          (struct sockaddr *)&mask,
                          (int)flags, NULL);

        splx(s);
    }

    if(error) {
        writeErrnoValue(libPtr, error);
        return -1;
    }
    return 0;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, FreeRouteInfo,
         AROS_LHA(struct rt_msghdr *, table, A0),
         struct SocketBase *, libPtr, 72, UL)
{
    AROS_LIBFUNC_INIT

    if(table != NULL)
        bsd_free(table, NULL);

    AROS_LIBFUNC_EXIT
}

/*
 * Walk radix sub-tree calling func for each leaf node.
 * Returns 0 on success or the first non-zero return from func.
 */
static int
route_walk(struct radix_node *rn,
           int (*func)(struct radix_node *, void *), void *arg)
{
    int error;
    for(;;) {
        while(rn->rn_b >= 0)
            rn = rn->rn_l;
        if((error = (*func)(rn, arg)) != 0)
            return error;
        while(rn->rn_p->rn_r == rn) {
            rn = rn->rn_p;
            if(rn->rn_flags & RNF_ROOT)
                return 0;
        }
        rn = rn->rn_p->rn_r;
    }
}

#define RT_ROUNDUP(a) \
    ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

/*
 * Callback context for GetRouteInfo routing table dump.
 */
struct getri_ctx {
    LONG  filter_flags;   /* RTF_* flags to filter by (0 = all) */
    int   pass;           /* 0 = size computation, 1 = data copy */
    int   size;           /* accumulated/needed size */
    char *buf;            /* output buffer (pass 1 only) */
};

static int
getri_callback(struct radix_node *rn, void *arg)
{
    struct getri_ctx *ctx = (struct getri_ctx *)arg;
    struct rtentry *rt;
    struct sockaddr *sa;
    int entry_size;

    for(; rn; rn = rn->rn_dupedkey) {
        if(rn->rn_flags & RNF_ROOT)
            continue;
        rt = (struct rtentry *)rn;
        if(ctx->filter_flags && !(rt->rt_flags & ctx->filter_flags))
            continue;

        /* Compute entry size: rt_msghdr + packed sockaddrs */
        entry_size = sizeof(struct rt_msghdr);
        if((sa = rt_key(rt)) != NULL)
            entry_size += RT_ROUNDUP(sa->sa_len);
        if((sa = rt->rt_gateway) != NULL)
            entry_size += RT_ROUNDUP(sa->sa_len);
        if((sa = rt_mask(rt)) != NULL)
            entry_size += RT_ROUNDUP(sa->sa_len);

        if(ctx->pass == 0) {
            ctx->size += entry_size;
        } else {
            struct rt_msghdr *rtm = (struct rt_msghdr *)ctx->buf;
            char *cp = (char *)(rtm + 1);
            int addrs = 0;

            memset(rtm, 0, sizeof(*rtm));
            rtm->rtm_version = RTM_VERSION;
            rtm->rtm_type    = RTM_GET;
            rtm->rtm_flags   = rt->rt_flags;
            rtm->rtm_use     = rt->rt_use;
            rtm->rtm_rmx     = rt->rt_rmx;
            if(rt->rt_ifp)
                rtm->rtm_index = rt->rt_ifp->if_index;

            if((sa = rt_key(rt)) != NULL) {
                int n = RT_ROUNDUP(sa->sa_len);
                memcpy(cp, sa, sa->sa_len);
                cp += n;
                addrs |= RTA_DST;
            }
            if((sa = rt->rt_gateway) != NULL) {
                int n = RT_ROUNDUP(sa->sa_len);
                memcpy(cp, sa, sa->sa_len);
                cp += n;
                addrs |= RTA_GATEWAY;
            }
            if((sa = rt_mask(rt)) != NULL) {
                int n = RT_ROUNDUP(sa->sa_len);
                memcpy(cp, sa, sa->sa_len);
                cp += n;
                addrs |= RTA_NETMASK;
            }

            rtm->rtm_addrs  = addrs;
            rtm->rtm_msglen = entry_size;
            ctx->buf += entry_size;
        }
    }
    return 0;
}

AROS_LH2(struct rt_msghdr *, GetRouteInfo,
         AROS_LHA(LONG, address_family, D0),
         AROS_LHA(LONG, flags, D1),
         struct SocketBase *, libPtr, 73, UL)
{
    AROS_LIBFUNC_INIT

    struct radix_node_head *rnh;
    struct getri_ctx ctx;
    struct rt_msghdr *result;
    spl_t s;
    int total_size;

    /* Pass 0: compute total buffer size under lock */
    ctx.filter_flags = flags;
    ctx.pass = 0;
    ctx.size = 0;
    ctx.buf  = NULL;

    s = splnet();
    for(rnh = radix_node_head; rnh; rnh = rnh->rnh_next) {
        if(rnh->rnh_af == 0)
            continue;
        if(address_family && rnh->rnh_af != (u_char)address_family)
            continue;
        if(rnh->rnh_treetop)
            route_walk(rnh->rnh_treetop, getri_callback, &ctx);
    }

    total_size = ctx.size;
    if(total_size == 0) {
        splx(s);
        writeErrnoValue(libPtr, ESRCH);
        return NULL;
    }

    /*
     * Allocate buffer with space for a zero-length sentinel.
     * Note: bsd_malloc under splnet is acceptable here — AROSTCP
     * uses a simple Forbid/Permit model and the allocator is safe
     * to call at any spl level.
     */
    result = (struct rt_msghdr *)bsd_malloc(total_size + sizeof(struct rt_msghdr),
                                            NULL, NULL);
    if(result == NULL) {
        splx(s);
        writeErrnoValue(libPtr, ENOMEM);
        return NULL;
    }

    /* Pass 1: fill the buffer (still under lock — table cannot change) */
    ctx.pass = 1;
    ctx.size = 0;
    ctx.buf  = (char *)result;

    for(rnh = radix_node_head; rnh; rnh = rnh->rnh_next) {
        if(rnh->rnh_af == 0)
            continue;
        if(address_family && rnh->rnh_af != (u_char)address_family)
            continue;
        if(rnh->rnh_treetop)
            route_walk(rnh->rnh_treetop, getri_callback, &ctx);
    }
    splx(s);

    /* Write sentinel: zero-length rt_msghdr marks end of list */
    memset(ctx.buf, 0, sizeof(struct rt_msghdr));

    return result;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(long, AddInterfaceTagList,
         AROS_LHA(STRPTR, name, A0),
         AROS_LHA(STRPTR, device, A1),
         AROS_LHA(long, unit, D0),
         AROS_LHA(struct TagItem *, tags, A2),
         struct SocketBase *, libPtr, 74, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "AddInterfaceTagList() is not implemented\n");
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH2(long, ConfigureInterfaceTagList,
         AROS_LHA(STRPTR, name, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         struct SocketBase *, libPtr, 75, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "ConfigureInterfaceTagList() is not implemented\n");
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, ReleaseInterfaceList,
         AROS_LHA(struct List *, list, A0),
         struct SocketBase *, libPtr, 76, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "ReleaseInterfaceList() is not implemented");
    AROS_LIBFUNC_EXIT
}

AROS_LH0(struct List *, ObtainInterfaceList,
         struct SocketBase *, libPtr, 77, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "ObtainInterfaceList() is not implemented");
    return NULL;
    AROS_LIBFUNC_EXIT
}

// QueryInterfaceTagList in amiga_netstat.c

AROS_LH5(LONG, CreateAddrAllocMessageA,
         AROS_LHA(LONG, version, D0),
         AROS_LHA(LONG, protocol, D1),
         AROS_LHA(STRPTR, interface_name, A0),
         AROS_LHA(struct AddressAllocationMessage *, result_ptr, A1),
         AROS_LHA(struct TagItem *, tags, A2),
         struct SocketBase *, libPtr, 79, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "CreateAddrAllocMessageA() is not implemented\n");
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, DeleteAddrAllocMessage,
         AROS_LHA(struct AddressAllocationMessage *, message, A0),
         struct SocketBase *, libPtr, 80, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "DeleteAddrAllocMessage() is not implemented");
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, BeginInterfaceConfig,
         AROS_LHA(struct AddressAllocationMessage *, message, A0),
         struct SocketBase *, libPtr, 81, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "BeginInterfaceConfig() is not implemented");
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, AbortInterfaceConfig,
         AROS_LHA(struct AddressAllocationMessage *, message, A0),
         struct SocketBase *, libPtr, 82, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "AbortInterfaceConfig() is not implemented");
    AROS_LIBFUNC_EXIT
}

AROS_LH3(long, AddNetMonitorHookTagList,
         AROS_LHA(long, type, D0),
         AROS_LHA(struct Hook *, hook, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         struct SocketBase *, libPtr, 83, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "AddNetMonitorHookTagList() is not implemented");
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, RemoveNetMonitorHook,
         AROS_LHA(struct Hook *, hook, A0),
         struct SocketBase *, libPtr, 84, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "RemoveNetMonitorHook() is not implemented");
    AROS_LIBFUNC_EXIT
}

AROS_LH4(LONG, GetNetworkStatistics,
         AROS_LHA(LONG, type, D0),
         AROS_LHA(LONG, version, D1),
         AROS_LHA(APTR, destination, A0),
         AROS_LHA(LONG, size, D2),
         struct SocketBase *, libPtr, 85, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "GetNetworkStatistics() is not implemented");
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AddDomainNameServer,
         AROS_LHA(STRPTR, address, A0),
         struct SocketBase *, libPtr, 86, UL)
{
    AROS_LIBFUNC_INIT

    struct sockaddr_in sa;
    struct NameserventNode *nsn;

    if(address == NULL) {
        writeErrnoValue(libPtr, EINVAL);
        return -1;
    }

    /* Parse the dotted-decimal address string */
    if(!__inet_aton(address, &sa.sin_addr)) {
        writeErrnoValue(libPtr, EINVAL);
        return -1;
    }

    if((nsn = bsd_malloc(sizeof(struct NameserventNode), NULL, NULL)) == NULL) {
        writeErrnoValue(libPtr, ENOMEM);
        return -1;
    }

    nsn->nsn_EntSize = sizeof(nsn->nsn_Ent);
    nsn->nsn_Ent.ns_addr.s_addr = sa.sin_addr.s_addr;

    ObtainSemaphore(&DynDB.dyn_Lock);
    AddTail((struct List *)&DynDB.dyn_NameServers, (struct Node *)nsn);
    ndb_Serial++;
    ReleaseSemaphore(&DynDB.dyn_Lock);

    return 0;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, RemoveDomainNameServer,
         AROS_LHA(STRPTR, address, A0),
         struct SocketBase *, libPtr, 87, UL)
{
    AROS_LIBFUNC_INIT

    struct in_addr target;
    struct MinNode *node, *nnode;
    int found = 0;

    if(address == NULL) {
        writeErrnoValue(libPtr, EINVAL);
        return -1;
    }

    if(!__inet_aton(address, &target)) {
        writeErrnoValue(libPtr, EINVAL);
        return -1;
    }

    ObtainSemaphore(&DynDB.dyn_Lock);
    for(node = DynDB.dyn_NameServers.mlh_Head; node->mln_Succ;) {
        struct NameserventNode *nsn = (struct NameserventNode *)node;
        nnode = node->mln_Succ;
        if(nsn->nsn_Ent.ns_addr.s_addr == target.s_addr) {
            Remove((struct Node *)node);
            bsd_free(node, NULL);
            found = 1;
            break;
        }
        node = nnode;
    }
    if(found)
        ndb_Serial++;
    ReleaseSemaphore(&DynDB.dyn_Lock);

    if(!found) {
        writeErrnoValue(libPtr, ENOENT);
        return -1;
    }
    return 0;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, ReleaseDomainNameServerList,
         AROS_LHA(struct List *, list, A0),
         struct SocketBase *, libPtr, 88, UL)
{
    AROS_LIBFUNC_INIT

    struct Node *node, *nnode;

    if(list == NULL)
        return;

    for(node = list->lh_Head; node->ln_Succ;) {
        nnode = node->ln_Succ;
        Remove(node);
        bsd_free(node, NULL);
        node = nnode;
    }
    bsd_free(list, NULL);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(struct List *, ObtainDomainNameServerList,
         struct SocketBase *, libPtr, 89, UL)
{
    AROS_LIBFUNC_INIT

    struct List *list;
    struct MinNode *node;

    list = bsd_malloc(sizeof(struct List), NULL, NULL);
    if(list == NULL) {
        writeErrnoValue(libPtr, ENOMEM);
        return NULL;
    }
    NewList(list);

    ObtainSemaphoreShared(&ndb_Lock);

    /* Walk static nameservers from the NetDB */
    if(NDB) {
        for(node = NDB->ndb_NameServers.mlh_Head; node->mln_Succ;
                node = node->mln_Succ) {
            struct NameserventNode *nsn = (struct NameserventNode *)node;
            char ipstr[16];
            struct DomainNameServerNode *dnsn;
            int slen;

            fmt_ipaddr(ipstr, nsn->nsn_Ent.ns_addr);
            slen = strnlen(ipstr, sizeof(ipstr)) + 1;

            dnsn = bsd_malloc(sizeof(struct DomainNameServerNode) + slen,
                              NULL, NULL);
            if(dnsn == NULL)
                continue;
            dnsn->dnsn_Size = sizeof(struct DomainNameServerNode) + slen;
            dnsn->dnsn_Address = (STRPTR)(dnsn + 1);
            memcpy(dnsn->dnsn_Address, ipstr, slen);
            dnsn->dnsn_UseCount = -1;  /* static entry */
            AddTail(list, (struct Node *)dnsn);
        }
    }

    /* Walk dynamic nameservers from DynDB */
    ObtainSemaphoreShared(&DynDB.dyn_Lock);
    for(node = DynDB.dyn_NameServers.mlh_Head; node->mln_Succ;
            node = node->mln_Succ) {
        struct NameserventNode *nsn = (struct NameserventNode *)node;
        char ipstr[16];
        struct DomainNameServerNode *dnsn;
        int slen;

        fmt_ipaddr(ipstr, nsn->nsn_Ent.ns_addr);
        slen = strnlen(ipstr, sizeof(ipstr)) + 1;

        dnsn = bsd_malloc(sizeof(struct DomainNameServerNode) + slen,
                          NULL, NULL);
        if(dnsn == NULL)
            continue;
        dnsn->dnsn_Size = sizeof(struct DomainNameServerNode) + slen;
        dnsn->dnsn_Address = (STRPTR)(dnsn + 1);
        memcpy(dnsn->dnsn_Address, ipstr, slen);
        dnsn->dnsn_UseCount = 1;  /* dynamic entry */
        AddTail(list, (struct Node *)dnsn);
    }
    ReleaseSemaphore(&DynDB.dyn_Lock);

    ReleaseSemaphore(&ndb_Lock);

    return list;

    AROS_LIBFUNC_EXIT
}

// setnetent/endnetent/getnetent defined in amiga_ndbent.c
// setprotoent defined in amiga_ndbent.c
// endprotoent defined in amiga_ndbent.c
// getprotoent defined in amiga_ndbent.c
// setservent/endservent/getservent defined in amiga_ndbent.c

// inet_aton defined in amiga_libcalls.c

/* ------------------------------------------------------------------ */
/* Slot 100: inet_ntop                                                */
/* ------------------------------------------------------------------ */
AROS_LH4(STRPTR, RS_inet_ntop,
         AROS_LHA(LONG, af, D0),
         AROS_LHA(void *, src, A0),
         AROS_LHA(char *, dst, A1),
         AROS_LHA(LONG, size, D1),
         struct SocketBase *, libPtr, 100, UL)
{
    AROS_LIBFUNC_INIT
    return (STRPTR)__inet_ntop((int)af, src, dst, (socklen_t)size, libPtr);
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 101: inet_pton                                                */
/* ------------------------------------------------------------------ */
AROS_LH3(LONG, RS_inet_pton,
         AROS_LHA(LONG, af, D0),
         AROS_LHA(const char *, src, A0),
         AROS_LHA(void *, dst, A1),
         struct SocketBase *, libPtr, 101, UL)
{
    AROS_LIBFUNC_INIT
    return (LONG)__inet_pton((int)af, src, dst, libPtr);
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 102: In_LocalAddr                                             */
/* ------------------------------------------------------------------ */
AROS_LH1(LONG, In_LocalAddr,
         AROS_LHA(ULONG, address, D0),
         struct SocketBase *, libPtr, 102, UL)
{
    AROS_LIBFUNC_INIT
    struct in_addr in;
    in.s_addr = address;
    return (LONG)in_localaddr(in);
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 103: In_CanForward                                            */
/* ------------------------------------------------------------------ */
AROS_LH1(LONG, In_CanForward,
         AROS_LHA(ULONG, address, D0),
         struct SocketBase *, libPtr, 103, UL)
{
    AROS_LIBFUNC_INIT
    struct in_addr in;
    in.s_addr = address;
    return (LONG)in_canforward(in);
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slots 104-114: mbuf kernel memory API                              */
/* ------------------------------------------------------------------ */

AROS_LH3(struct mbuf *, mbuf_copym,
         AROS_LHA(struct mbuf *, buffer, A0),
         AROS_LHA(LONG, offset, D0),
         AROS_LHA(LONG, length, D1),
         struct SocketBase *, libPtr, 104, UL)
{
    AROS_LIBFUNC_INIT
    return m_copym(buffer, (int)offset, (int)length, M_DONTWAIT);
    AROS_LIBFUNC_EXIT
}

AROS_LH4(LONG, mbuf_copydata,
         AROS_LHA(struct mbuf *, buffer, A0),
         AROS_LHA(LONG, offset, D0),
         AROS_LHA(LONG, length, D1),
         AROS_LHA(void *, data, A1),
         struct SocketBase *, libPtr, 105, UL)
{
    AROS_LIBFUNC_INIT
    m_copydata(buffer, (int)offset, (int)length, (caddr_t)data);
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH4(LONG, mbuf_copyback,
         AROS_LHA(struct mbuf *, buffer, A0),
         AROS_LHA(LONG, offset, D0),
         AROS_LHA(LONG, length, D1),
         AROS_LHA(void *, data, A1),
         struct SocketBase *, libPtr, 106, UL)
{
    AROS_LIBFUNC_INIT
    m_copyback(buffer, (int)offset, (int)length, (caddr_t)data);
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(struct mbuf *, mbuf_free,
         AROS_LHA(struct mbuf *, buffer, A0),
         struct SocketBase *, libPtr, 107, UL)
{
    AROS_LIBFUNC_INIT
    return m_free(buffer);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, mbuf_freem,
         AROS_LHA(struct mbuf *, chain, A0),
         struct SocketBase *, libPtr, 108, UL)
{
    AROS_LIBFUNC_INIT
    m_freem(chain);
    AROS_LIBFUNC_EXIT
}

AROS_LH0(struct mbuf *, mbuf_get,
         struct SocketBase *, libPtr, 109, UL)
{
    AROS_LIBFUNC_INIT
    return m_get(M_DONTWAIT, MT_DATA);
    AROS_LIBFUNC_EXIT
}

AROS_LH0(struct mbuf *, mbuf_gethdr,
         struct SocketBase *, libPtr, 110, UL)
{
    AROS_LIBFUNC_INIT
    return m_gethdr(M_DONTWAIT, MT_DATA);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(struct mbuf *, mbuf_prepend,
         AROS_LHA(struct mbuf *, buffer, A0),
         AROS_LHA(LONG, length, D0),
         struct SocketBase *, libPtr, 111, UL)
{
    AROS_LIBFUNC_INIT
    return m_prepend(buffer, (int)length, M_DONTWAIT);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(LONG, mbuf_adj,
         AROS_LHA(struct mbuf *, buffer, A0),
         AROS_LHA(LONG, length, D0),
         struct SocketBase *, libPtr, 112, UL)
{
    AROS_LIBFUNC_INIT
    m_adj(buffer, (int)length);
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH2(LONG, mbuf_cat,
         AROS_LHA(struct mbuf *, first_chain, A0),
         AROS_LHA(struct mbuf *, second_chain, A1),
         struct SocketBase *, libPtr, 113, UL)
{
    AROS_LIBFUNC_INIT
    m_cat(first_chain, second_chain);
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH2(struct mbuf *, mbuf_pullup,
         AROS_LHA(struct mbuf *, buffer, A0),
         AROS_LHA(LONG, length, D0),
         struct SocketBase *, libPtr, 114, UL)
{
    AROS_LIBFUNC_INIT
    return m_pullup(buffer, (int)length);
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 115: ProcessIsServer                                          */
/* ------------------------------------------------------------------ */
AROS_LH1(LONG, ProcessIsServer,
         AROS_LHA(struct Process *, process, A0),
         struct SocketBase *, libPtr, 115, UL)
{
    AROS_LIBFUNC_INIT
    /* Not implemented -- always returns FALSE */
    return 0;
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 116: ObtainServerSocket                                       */
/* ------------------------------------------------------------------ */
AROS_LH0(LONG, ObtainServerSocket,
         struct SocketBase *, libPtr, 116, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 117: GetDefaultDomainName                                     */
/* ------------------------------------------------------------------ */
AROS_LH2(LONG, GetDefaultDomainName,
         AROS_LHA(STRPTR, buffer, A0),
         AROS_LHA(LONG, buffer_size, D0),
         struct SocketBase *, libPtr, 117, UL)
{
    AROS_LIBFUNC_INIT

    struct DomainentNode *dn;

    if (buffer == NULL || buffer_size <= 0)
        return 0;

    LOCK_R_NDB(NDB);
    dn = (struct DomainentNode *)NDB->ndb_Domains.mlh_Head;
    if (dn->dn_Node.mln_Succ != NULL && dn->dn_Ent.d_name != NULL) {
        size_t len = strnlen(dn->dn_Ent.d_name, (size_t)buffer_size - 1);
        memcpy(buffer, dn->dn_Ent.d_name, len);
        buffer[len] = '\0';
        UNLOCK_NDB(NDB);
        return 1; /* TRUE */
    }
    UNLOCK_NDB(NDB);

    buffer[0] = '\0';
    return 0; /* FALSE */

    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 118: SetDefaultDomainName                                     */
/* ------------------------------------------------------------------ */
AROS_LH1(void, SetDefaultDomainName,
         AROS_LHA(STRPTR, name, A0),
         struct SocketBase *, libPtr, 118, UL)
{
    AROS_LIBFUNC_INIT
    /* Not fully implemented -- would need to rebuild domain list */
    (void)name;
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 119: ObtainRoadshowData                                       */
/* ------------------------------------------------------------------ */
AROS_LH1(struct List *, ObtainRoadshowData,
         AROS_LHA(LONG, access, D0),
         struct SocketBase *, libPtr, 119, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return NULL;
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 120: ReleaseRoadshowData                                      */
/* NOTE: Roadshow passes pointer in D0 (unusual)                      */
/* ------------------------------------------------------------------ */
AROS_LH1(void, ReleaseRoadshowData,
         AROS_LHA(APTR, list, D0),
         struct SocketBase *, libPtr, 120, UL)
{
    AROS_LIBFUNC_INIT
    (void)list;
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 121: ChangeRoadshowData                                       */
/* ------------------------------------------------------------------ */
AROS_LH4(LONG, ChangeRoadshowData,
         AROS_LHA(struct List *, list, A0),
         AROS_LHA(STRPTR, name, A1),
         AROS_LHA(LONG, length, D0),
         AROS_LHA(APTR, data, A2),
         struct SocketBase *, libPtr, 121, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return 0; /* FALSE */
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 122: RemoveInterface                                          */
/* ------------------------------------------------------------------ */
AROS_LH2(LONG, RemoveInterface,
         AROS_LHA(STRPTR, name, A0),
         AROS_LHA(LONG, force, D0),
         struct SocketBase *, libPtr, 122, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return 0; /* FALSE */
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 123: gethostbyname_r                                          */
/* ------------------------------------------------------------------ */
AROS_LH5(struct hostent *, RS_gethostbyname_r,
         AROS_LHA(const char *, name, A0),
         AROS_LHA(struct hostent *, result, A1),
         AROS_LHA(char *, buf, A2),
         AROS_LHA(LONG, buflen, D0),
         AROS_LHA(LONG *, h_errnop, A3),
         struct SocketBase *, libPtr, 123, UL)
{
    AROS_LIBFUNC_INIT
    return __gethostbyname_r(name, result, buf, buflen, h_errnop, libPtr);
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 124: gethostbyaddr_r                                          */
/* ------------------------------------------------------------------ */
AROS_LH7(struct hostent *, RS_gethostbyaddr_r,
         AROS_LHA(const char *, addr, A0),
         AROS_LHA(LONG, len, D0),
         AROS_LHA(LONG, type, D1),
         AROS_LHA(struct hostent *, result, A1),
         AROS_LHA(char *, buf, A2),
         AROS_LHA(LONG, buflen, D2),
         AROS_LHA(LONG *, h_errnop, A3),
         struct SocketBase *, libPtr, 124, UL)
{
    AROS_LIBFUNC_INIT
    return __gethostbyaddr_r(addr, len, type, result, buf, buflen,
                             h_errnop, libPtr);
    AROS_LIBFUNC_EXIT
}

/* Slots 125-127: Reserved (Null entries in function table) */

/* ------------------------------------------------------------------ */
/* Slots 128-134: IPF (IP Filter) API                                 */
/* Intentionally undocumented per Roadshow spec.                      */
/* ------------------------------------------------------------------ */

AROS_LH3(LONG, ipf_open,
         AROS_LHA(LONG, channel, D0),
         AROS_LHA(STRPTR, name, A0),
         AROS_LHA(LONG, flags, D1),
         struct SocketBase *, libPtr, 128, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, ipf_close,
         AROS_LHA(LONG, handle, D0),
         struct SocketBase *, libPtr, 129, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH3(LONG, ipf_read,
         AROS_LHA(LONG, handle, D0),
         AROS_LHA(void *, buffer, A0),
         AROS_LHA(LONG, len, D1),
         struct SocketBase *, libPtr, 130, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH3(LONG, ipf_write,
         AROS_LHA(LONG, handle, D0),
         AROS_LHA(const void *, buffer, A0),
         AROS_LHA(LONG, len, D1),
         struct SocketBase *, libPtr, 131, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH3(LONG, ipf_ioctl,
         AROS_LHA(LONG, handle, D0),
         AROS_LHA(ULONG, request, D1),
         AROS_LHA(char *, argp, A0),
         struct SocketBase *, libPtr, 132, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH2(LONG, ipf_set_notify_mask,
         AROS_LHA(LONG, handle, D0),
         AROS_LHA(ULONG, mask, D1),
         struct SocketBase *, libPtr, 133, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH2(LONG, ipf_set_interrupt_mask,
         AROS_LHA(LONG, handle, D0),
         AROS_LHA(ULONG, mask, D1),
         struct SocketBase *, libPtr, 134, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slots 135-138: getaddrinfo family (Roadshow extension)             */
/* These are kernel-side implementations that use internal APIs       */
/* directly, parallel to the Miami-side versions in miami_api.c.      */
/* ------------------------------------------------------------------ */

static char *rs_ai_errors[] = {
    "No error",
    "Address family for host not supported",
    "Temporary failure in name resolution",
    "Invalid flags value",
    "Non-recoverable failure in name resolution",
    "Address family not supported",
    "Memory allocation failure",
    "No address associated with host",
    "Host nor service provided, or not known",
    "Service not supported for socket type",
    "Socket type not supported",
    "System error",
    "Unknown error"
};

/* Internal: find service by port number in NDB */
static struct servent *
rs_findservbyport(LONG port, const char *proto)
{
    struct ServentNode *entNode;
    LOCK_R_NDB(NDB);
    for (entNode = (struct ServentNode *)NDB->ndb_Services.mlh_Head;
         entNode->sn_Node.mln_Succ;
         entNode = (struct ServentNode *)entNode->sn_Node.mln_Succ)
    {
        if (entNode->sn_Ent.s_port == port &&
            (proto == NULL || strcmp(entNode->sn_Ent.s_proto, proto) == 0))
        {
            UNLOCK_NDB(NDB);
            return &entNode->sn_Ent;
        }
    }
    UNLOCK_NDB(NDB);
    return NULL;
}

/* Internal: find service by name in NDB */
static struct servent *
rs_findservbyname(const char *name, const char *proto)
{
    struct ServentNode *entNode;
    LOCK_R_NDB(NDB);
    entNode = findServentNode(NDB, name, proto);
    if (entNode != NULL) {
        UNLOCK_NDB(NDB);
        return &entNode->sn_Ent;
    }
    UNLOCK_NDB(NDB);
    return NULL;
}

AROS_LH1(void, RS_freeaddrinfo,
         AROS_LHA(struct addrinfo *, ai, A0),
         struct SocketBase *, libPtr, 135, UL)
{
    AROS_LIBFUNC_INIT
    struct addrinfo *cur, *next;
    for (cur = ai; cur != NULL; cur = next) {
        next = cur->ai_next;
        if (cur->ai_canonname)
            FreeVec(cur->ai_canonname);
        FreeVec(cur);
    }
    AROS_LIBFUNC_EXIT
}

AROS_LH4(LONG, RS_getaddrinfo,
         AROS_LHA(const char *, hostname, A0),
         AROS_LHA(const char *, servname, A1),
         AROS_LHA(const struct addrinfo *, hints, A2),
         AROS_LHA(struct addrinfo **, result, A3),
         struct SocketBase *, libPtr, 136, UL)
{
    AROS_LIBFUNC_INIT

    struct addrinfo sentinel;
    struct addrinfo *cur;
    int family = PF_UNSPEC;
    int socktype = 0;
    int protocol = 0;
    int flags = 0;
    int port = 0;
    int error;

    *result = NULL;
    memset(&sentinel, 0, sizeof(sentinel));
    cur = &sentinel;

    if (hostname == NULL && servname == NULL)
        return EAI_NONAME;

    if (hints) {
        if (hints->ai_addrlen || hints->ai_canonname ||
            hints->ai_addr || hints->ai_next)
            return EAI_BADFLAGS;
        family = hints->ai_family;
        socktype = hints->ai_socktype;
        protocol = hints->ai_protocol;
        flags = hints->ai_flags;
        switch (family) {
        case PF_UNSPEC: case PF_INET: case PF_INET6: break;
        default: return EAI_FAMILY;
        }
        switch (socktype) {
        case 0: case SOCK_STREAM: case SOCK_DGRAM: case SOCK_RAW: break;
        default: return EAI_SOCKTYPE;
        }
    }

    if (servname != NULL && servname[0] != '\0') {
        char *ep;
        long pval;
        const char *proto_name = NULL;
        if (socktype == SOCK_STREAM) proto_name = "tcp";
        else if (socktype == SOCK_DGRAM) proto_name = "udp";
        pval = strtol(servname, &ep, 10);
        if (ep != servname && *ep == '\0' && pval >= 0 && pval <= 65535) {
            port = htons((unsigned short)pval);
        } else {
            struct servent *sp;
            if (flags & AI_NUMERICSERV)
                return EAI_NONAME;
            sp = rs_findservbyname(servname, proto_name);
            if (sp == NULL) {
                if (proto_name == NULL) {
                    sp = rs_findservbyname(servname, "tcp");
                    if (sp == NULL)
                        sp = rs_findservbyname(servname, "udp");
                }
                if (sp == NULL) return EAI_SERVICE;
            }
            port = sp->s_port;
        }
    }

    int stypes[3];
    int nstypes = 0;
    if (socktype != 0) { stypes[0] = socktype; nstypes = 1; }
    else { stypes[0] = SOCK_STREAM; stypes[1] = SOCK_DGRAM; nstypes = 2; }

    int families[2];
    int nfamilies = 0;
    if (family == PF_INET || family == PF_INET6) {
        families[0] = family; nfamilies = 1;
    } else {
        families[0] = PF_INET6; families[1] = PF_INET; nfamilies = 2;
    }

    for (int fi = 0; fi < nfamilies; fi++) {
        int af = families[fi];
        struct sockaddr_storage addrs[16];
        char *canonname = NULL;
        int naddrs = 0;

        if (hostname == NULL) {
            if (af == PF_INET) {
                struct sockaddr_in *sin = (struct sockaddr_in *)&addrs[0];
                memset(sin, 0, sizeof(*sin));
                sin->sin_len = sizeof(*sin);
                sin->sin_family = AF_INET;
                sin->sin_addr.s_addr = (flags & AI_PASSIVE) ? htonl(INADDR_ANY) : htonl(INADDR_LOOPBACK);
                naddrs = 1;
            } else {
                struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&addrs[0];
                memset(sin6, 0, sizeof(*sin6));
                sin6->sin6_len = sizeof(*sin6);
                sin6->sin6_family = AF_INET6;
                if (!(flags & AI_PASSIVE))
                    sin6->sin6_addr.s6_addr[15] = 1;
                naddrs = 1;
            }
        } else if (flags & AI_NUMERICHOST) {
            if (af == PF_INET) {
                struct sockaddr_in *sin = (struct sockaddr_in *)&addrs[0];
                memset(sin, 0, sizeof(*sin));
                sin->sin_len = sizeof(*sin);
                sin->sin_family = AF_INET;
                if (__inet_pton(AF_INET, hostname, &sin->sin_addr, libPtr) == 1)
                    naddrs = 1;
            } else {
                struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&addrs[0];
                memset(sin6, 0, sizeof(*sin6));
                sin6->sin6_len = sizeof(*sin6);
                sin6->sin6_family = AF_INET6;
                if (__inet_pton(AF_INET6, hostname, &sin6->sin6_addr, libPtr) == 1)
                    naddrs = 1;
            }
        } else {
            int numeric_ok = 0;
            if (af == PF_INET) {
                struct sockaddr_in *sin = (struct sockaddr_in *)&addrs[0];
                memset(sin, 0, sizeof(*sin));
                sin->sin_len = sizeof(*sin);
                sin->sin_family = AF_INET;
                if (__inet_pton(AF_INET, hostname, &sin->sin_addr, libPtr) == 1) {
                    naddrs = 1; numeric_ok = 1;
                }
            } else {
                struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&addrs[0];
                memset(sin6, 0, sizeof(*sin6));
                sin6->sin6_len = sizeof(*sin6);
                sin6->sin6_family = AF_INET6;
                if (__inet_pton(AF_INET6, hostname, &sin6->sin6_addr, libPtr) == 1) {
                    naddrs = 1; numeric_ok = 1;
                }
            }
            if (!numeric_ok) {
                struct hostent *hp;
                if (af == PF_INET)
                    hp = __gethostbyname(hostname, libPtr);
                else
                    hp = __gethostbyname2(hostname, AF_INET6, libPtr);
                if (hp != NULL && hp->h_addr_list != NULL) {
                    if (flags & AI_CANONNAME) canonname = hp->h_name;
                    for (int i = 0; hp->h_addr_list[i] && naddrs < 16; i++) {
                        if (af == PF_INET) {
                            struct sockaddr_in *sin = (struct sockaddr_in *)&addrs[naddrs];
                            memset(sin, 0, sizeof(*sin));
                            sin->sin_len = sizeof(*sin);
                            sin->sin_family = AF_INET;
                            memcpy(&sin->sin_addr, hp->h_addr_list[i], hp->h_length);
                        } else {
                            struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&addrs[naddrs];
                            memset(sin6, 0, sizeof(*sin6));
                            sin6->sin6_len = sizeof(*sin6);
                            sin6->sin6_family = AF_INET6;
                            memcpy(&sin6->sin6_addr, hp->h_addr_list[i], hp->h_length);
                        }
                        naddrs++;
                    }
                }
            }
        }

        if (naddrs == 0) continue;

        for (int ai = 0; ai < naddrs; ai++) {
            struct sockaddr *sa = (struct sockaddr *)&addrs[ai];
            int addrlen = (af == PF_INET)
                          ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
            if (af == PF_INET)
                ((struct sockaddr_in *)sa)->sin_port = port;
            else
                ((struct sockaddr_in6 *)sa)->sin6_port = port;

            for (int si = 0; si < nstypes; si++) {
                struct addrinfo *ai_new;
                int proto;
                if (stypes[si] == SOCK_STREAM) proto = IPPROTO_TCP;
                else if (stypes[si] == SOCK_DGRAM) proto = IPPROTO_UDP;
                else proto = protocol;

                ai_new = (struct addrinfo *)AllocVec(
                             sizeof(struct addrinfo) + addrlen,
                             MEMF_PUBLIC | MEMF_CLEAR);
                if (ai_new == NULL) { error = EAI_MEMORY; goto fail; }

                ai_new->ai_flags = flags;
                ai_new->ai_family = af;
                ai_new->ai_socktype = stypes[si];
                ai_new->ai_protocol = proto;
                ai_new->ai_addrlen = addrlen;
                ai_new->ai_addr = (struct sockaddr *)(ai_new + 1);
                memcpy(ai_new->ai_addr, sa, addrlen);

                if (sentinel.ai_next == NULL && canonname != NULL) {
                    int clen = strnlen(canonname, MAXHOSTNAMELEN) + 1;
                    ai_new->ai_canonname = (char *)AllocVec(clen, MEMF_PUBLIC);
                    if (ai_new->ai_canonname)
                        memcpy(ai_new->ai_canonname, canonname, clen);
                } else if (sentinel.ai_next == NULL && (flags & AI_CANONNAME) && hostname != NULL) {
                    int clen = strnlen(hostname, MAXHOSTNAMELEN) + 1;
                    ai_new->ai_canonname = (char *)AllocVec(clen, MEMF_PUBLIC);
                    if (ai_new->ai_canonname)
                        memcpy(ai_new->ai_canonname, hostname, clen);
                }

                cur->ai_next = ai_new;
                cur = ai_new;
            }
        }
    }

    if (sentinel.ai_next == NULL)
        return EAI_NONAME;

    *result = sentinel.ai_next;
    return 0;

fail:
    {
        struct addrinfo *ai_f, *next;
        for (ai_f = sentinel.ai_next; ai_f != NULL; ai_f = next) {
            next = ai_f->ai_next;
            if (ai_f->ai_canonname)
                FreeVec(ai_f->ai_canonname);
            FreeVec(ai_f);
        }
    }
    return error;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(STRPTR, RS_gai_strerror,
         AROS_LHA(LONG, ecode, D0),
         struct SocketBase *, libPtr, 137, UL)
{
    AROS_LIBFUNC_INIT
    if (ecode < 0 || ecode > 11)
        return (STRPTR)"Unknown error";
    return (STRPTR)rs_ai_errors[ecode];
    AROS_LIBFUNC_EXIT
}

AROS_LH7(LONG, RS_getnameinfo,
         AROS_LHA(struct sockaddr *, sa, A0),
         AROS_LHA(LONG, salen, D0),
         AROS_LHA(char *, hostname, A1),
         AROS_LHA(LONG, hostlen, D1),
         AROS_LHA(char *, servicename, A2),
         AROS_LHA(LONG, servicelen, D2),
         AROS_LHA(LONG, flags, D3),
         struct SocketBase *, libPtr, 138, UL)
{
    AROS_LIBFUNC_INIT

    if (sa == NULL)
        return EAI_FAIL;

    if (hostname != NULL && hostlen > 0) {
        if (flags & NI_NUMERICHOST) {
            const char *p = NULL;
            if (sa->sa_family == AF_INET) {
                struct sockaddr_in *sin = (struct sockaddr_in *)sa;
                p = __inet_ntop(AF_INET, &sin->sin_addr,
                                hostname, hostlen, libPtr);
            } else if (sa->sa_family == AF_INET6) {
                struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
                p = __inet_ntop(AF_INET6, &sin6->sin6_addr,
                                hostname, hostlen, libPtr);
            } else {
                return EAI_FAMILY;
            }
            if (p == NULL)
                return EAI_FAIL;
        } else {
            struct hostent *hp = NULL;

            if (sa->sa_family == AF_INET) {
                struct sockaddr_in *sin = (struct sockaddr_in *)sa;
                hp = __gethostbyaddr((UBYTE *)&sin->sin_addr,
                                     sizeof(sin->sin_addr),
                                     AF_INET, libPtr);
            } else if (sa->sa_family == AF_INET6) {
                struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
                hp = __gethostbyaddr((UBYTE *)&sin6->sin6_addr,
                                     sizeof(sin6->sin6_addr),
                                     AF_INET6, libPtr);
            } else {
                return EAI_FAMILY;
            }

            if (hp != NULL && hp->h_name != NULL) {
                char *name = hp->h_name;
                if (flags & NI_NOFQDN) {
                    char *dot = strchr(name, '.');
                    if (dot) {
                        int dlen = dot - name;
                        if (dlen >= hostlen)
                            return EAI_FAIL;
                        memcpy(hostname, name, dlen);
                        hostname[dlen] = '\0';
                        goto rs_host_done;
                    }
                }
                if ((int)strnlen(name, hostlen) >= hostlen)
                    return EAI_FAIL;
                strcpy(hostname, name);
            } else {
                const char *numaddr = NULL;
                if (flags & NI_NAMEREQD)
                    return EAI_NONAME;
                if (sa->sa_family == AF_INET) {
                    struct sockaddr_in *sin = (struct sockaddr_in *)sa;
                    numaddr = __inet_ntop(AF_INET, &sin->sin_addr,
                                          hostname, hostlen, libPtr);
                } else {
                    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
                    numaddr = __inet_ntop(AF_INET6, &sin6->sin6_addr,
                                          hostname, hostlen, libPtr);
                }
                if (numaddr == NULL)
                    return EAI_FAIL;
            }
        }
    }
rs_host_done:

    if (servicename != NULL && servicelen > 0) {
        int port;
        if (sa->sa_family == AF_INET)
            port = ((struct sockaddr_in *)sa)->sin_port;
        else if (sa->sa_family == AF_INET6)
            port = ((struct sockaddr_in6 *)sa)->sin6_port;
        else
            return EAI_FAMILY;

        if (!(flags & NI_NUMERICSERV)) {
            const char *proto = (flags & NI_DGRAM) ? "udp" : "tcp";
            struct servent *sp = rs_findservbyport(port, proto);
            if (sp != NULL) {
                if ((int)strnlen(sp->s_name, servicelen) >= servicelen)
                    return EAI_FAIL;
                strcpy(servicename, sp->s_name);
                goto rs_serv_done;
            }
        }
        snprintf(servicename, servicelen, "%d", ntohs(port));
    }
rs_serv_done:

    return 0;

    AROS_LIBFUNC_EXIT
}

#endif

