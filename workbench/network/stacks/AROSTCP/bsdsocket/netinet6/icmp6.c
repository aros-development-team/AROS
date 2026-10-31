/*
 * Copyright (C) 2020-2026 The AROS Development Team.  All rights reserved.
 *
 * Based on KAME IPv6 / FreeBSD icmp6 implementation.
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * icmp6.c - ICMPv6 protocol implementation.
 *
 * Handles inbound ICMPv6: echo request/reply, error messages,
 * and dispatches Neighbor Discovery messages to nd6.c.
 */

#include <conf.h>

#if INET6

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/synch.h>
#include <stdarg.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>
#include <netinet6/mld6.h>

#include <protos/net/route_protos.h>
#include <kern/uipc_socket2_protos.h>
#include <protos/netinet/in_cksum_protos.h>

/* IP6_HDR_OFF macro: ip6 header is at offset 0 in mbuf after ip6_input */
#define ICMPv6_HDRLEN sizeof(struct icmp6_hdr)

struct icmp6stat icmp6stat = {0};

/* tcp_now: 500ms tick counter incremented by tcp_slowtimo() */
extern u_long tcp_now;

/* ICMPv6 rate limiting (token bucket) */
static int icmp6_ratelimit_tokens = 10;    /* current tokens */
static int icmp6_ratelimit_max = 10;       /* max burst */
static int icmp6_ratelimit_rate = 5;       /* tokens per slow-tick (500ms) */
static long icmp6_ratelimit_last = 0;      /* last refill time (tcp_now) */

/*
 * Rate-limit ICMPv6 error generation using a token bucket.
 * Returns 1 if allowed, 0 if rate-limited.
 */
static int
icmp6_ratelimit(void)
{
    long elapsed;

    elapsed = (long)(tcp_now - icmp6_ratelimit_last);
    if (elapsed > 0) {
        icmp6_ratelimit_last = tcp_now;
        icmp6_ratelimit_tokens += elapsed * icmp6_ratelimit_rate;
        if (icmp6_ratelimit_tokens > icmp6_ratelimit_max)
            icmp6_ratelimit_tokens = icmp6_ratelimit_max;
    }

    if (icmp6_ratelimit_tokens > 0) {
        icmp6_ratelimit_tokens--;
        return 1;
    }
    icmp6stat.icp6s_toofreq++;
    return 0;
}

/* forward */
static void icmp6_reflect(struct mbuf *, size_t);
void rip6_input(void *args, ...);

/* ------------------------------------------------------------------
 * icmp6_init
 * ------------------------------------------------------------------ */
void
icmp6_init(void)
{
    mld6_init();
}

void icmp6_fasttimo(void)
{
    mld6_fasttimeo();
}

extern void frag6_slowtimo(void);
extern void in6_tmpaddrtimer(void);

void
icmp6_slowtimo(void)
{
    static int icmp6_slow_ticks = 0;
    icmp6_slow_ticks++;
    if(icmp6_slow_ticks <= 10 || (icmp6_slow_ticks % 60) == 0) {
        D(bug("[AROSTCP:ICMP6] %s: tick=%d\n", __func__, icmp6_slow_ticks));
    }
    /* Refill ICMPv6 rate-limit tokens each slow tick */
    icmp6_ratelimit_tokens += icmp6_ratelimit_rate;
    if (icmp6_ratelimit_tokens > icmp6_ratelimit_max)
        icmp6_ratelimit_tokens = icmp6_ratelimit_max;
    icmp6_ratelimit_last = tcp_now;
    nd6_timer(NULL);
    frag6_slowtimo();
    in6_tmpaddrtimer();
    if(icmp6_slow_ticks <= 10) {
        D(bug("[AROSTCP:ICMP6] %s: nd6_timer returned (tick=%d)\n",
              __func__, icmp6_slow_ticks));
    }
}

void icmp6_drain(void)    {}

/* ------------------------------------------------------------------
 * icmp6_update_pmtu - record a discovered path MTU (RFC 8201).
 *
 * On receiving an ICMPv6 Packet Too Big, the reported next-hop MTU is stored
 * on the route to the destination that provoked the error, so ip6_output()
 * sizes subsequent datagrams to fit the bottleneck link instead of having them
 * black-holed (IPv6 routers never fragment).  The MTU is clamped to the IPv6
 * minimum and only ever lowered.
 * ------------------------------------------------------------------ */
static void
icmp6_update_pmtu(struct in6_addr *dst, u_int32_t mtu)
{
    struct sockaddr_in6 sin6;
    struct rtentry *rt;

    if(mtu < IPV6_MMTU)
        mtu = IPV6_MMTU;

    bzero(&sin6, sizeof(sin6));
    sin6.sin6_family = AF_INET6;
    sin6.sin6_len    = sizeof(sin6);
    sin6.sin6_addr   = *dst;

    rt = rtalloc1((struct sockaddr *)&sin6, 0);
    if(rt == NULL)
        return;
    if(rt->rt_rmx.rmx_mtu == 0 || mtu < rt->rt_rmx.rmx_mtu)
        rt->rt_rmx.rmx_mtu = mtu;
    rtfree(rt);
}

/* ------------------------------------------------------------------
 * icmp6_echo_copy - the packet to turn into the Echo Reply for m.
 *
 * m stays as it came, for the raw sockets: the headers up to and
 * including the ICMPv6 header are copied into a new mbuf, and only the
 * data after them is shared with m, read but never written.
 * ------------------------------------------------------------------ */
static struct mbuf *
icmp6_echo_copy(struct mbuf *m, int off)
{
    struct mbuf *n;
    int hlen = off + sizeof(struct icmp6_hdr);

    if(hlen > MHLEN || m->m_pkthdr.len < hlen)
        return NULL;
    n = m_gethdr(M_DONTWAIT, MT_DATA);
    if(n == NULL)
        return NULL;
    M_COPY_PKTHDR(n, m);
    m_copydata(m, 0, hlen, mtod(n, caddr_t));
    n->m_len = hlen;
    if(m->m_pkthdr.len > hlen &&
            (n->m_next = m_copym(m, hlen, M_COPYALL, M_DONTWAIT)) == NULL) {
        m_freem(n);
        return NULL;
    }
    return n;
}

/* ------------------------------------------------------------------
 * icmp6_notify_error - pass an ICMPv6 error to the upper-layer protocol
 * of the packet it quotes (RFC 4443 section 2.4), as KAME's
 * icmp6_notify_error() does: that protocol's pr_ctlinput gets the quoted
 * destination as a sockaddr_in6 and, in an ip6ctlparam, the quoted source
 * and ports. Only an upper-layer header that follows the IPv6 header
 * directly is found. The quoted headers are copied out, so m is left as
 * it is for the raw sockets.
 * ------------------------------------------------------------------ */
static void
icmp6_notify_error(struct mbuf *m, int off, int cmd)
{
    extern struct protosw inet6sw[];
    extern u_char ip6_protox[];
    struct ip6_hdr oip6;
    struct sockaddr_in6 sa6;
    struct ip6ctlparam ip6cp;
    struct protosw *pr;
    int qoff = off + sizeof(struct icmp6_hdr);  /* the quoted IPv6 header */

    if(m->m_pkthdr.len < qoff + (int)sizeof(oip6)) {
        icmp6stat.icp6s_tooshort++;
        return;
    }
    m_copydata(m, qoff, sizeof(oip6), (caddr_t)&oip6);

    bzero(&ip6cp, sizeof(ip6cp));
    ip6cp.ip6c_src = oip6.ip6_src;
    if((oip6.ip6_nxt == IPPROTO_TCP || oip6.ip6_nxt == IPPROTO_UDP) &&
            m->m_pkthdr.len >= qoff + (int)sizeof(oip6) + 4) {
        u_int16_t ports[2];     /* TCP and UDP both begin with them */

        m_copydata(m, qoff + sizeof(oip6), sizeof(ports), (caddr_t)ports);
        ip6cp.ip6c_sport = ports[0];
        ip6cp.ip6c_dport = ports[1];
    }

    bzero(&sa6, sizeof(sa6));
    sa6.sin6_len    = sizeof(sa6);
    sa6.sin6_family = AF_INET6;
    sa6.sin6_addr   = oip6.ip6_dst;

    pr = &inet6sw[ip6_protox[oip6.ip6_nxt]];
    if(pr->pr_protocol == oip6.ip6_nxt && pr->pr_ctlinput != NULL)
        (*pr->pr_ctlinput)(cmd, (struct sockaddr *)&sa6, &ip6cp);
}

/* ------------------------------------------------------------------
 * icmp6_input - process an inbound ICMPv6 message.
 *
 * Called via protosw pr_input: void *args is struct mbuf *,
 * va_arg gives int off (offset to ICMPv6 header within mbuf).
 * ------------------------------------------------------------------ */
void
icmp6_input(void *args, ...)
{
    struct mbuf  *m = args;
    struct ip6_hdr   *ip6;
    struct icmp6_hdr *icmp6;
    struct mbuf *n;
    int off, icmp6len;
    va_list va;

    va_start(va, args);
    off = va_arg(va, int);
    va_end(va);

    ip6 = mtod(m, struct ip6_hdr *);

    /* pull up icmp6 header */
    if(m->m_len < off + ICMPv6_HDRLEN &&
            (m = m_pullup(m, off + ICMPv6_HDRLEN)) == NULL) {
        icmp6stat.icp6s_tooshort++;
        return;
    }
    ip6   = mtod(m, struct ip6_hdr *);
    icmp6 = (struct icmp6_hdr *)(mtod(m, u_int8_t *) + off);
    icmp6len = m->m_pkthdr.len - off;

    icmp6stat.icp6s_inhist[icmp6->icmp6_type]++;

    /* verify checksum using IPv6 pseudo-header */
    if(in6_cksum(m, IPPROTO_ICMPV6, off, icmp6len)) {
        icmp6stat.icp6s_checksum++;
        m_freem(m);
        return;
    }

    /*
     * The kernel's handlers get a copy; m itself goes to the raw sockets
     * at the end, untouched (RFC 3542 section 3.2). m_copym() shares
     * clusters, which the handlers only read.
     */
    switch(icmp6->icmp6_type) {
    /* ----- Multicast Listener Discovery (delegated to mld6.c) ----- */
    case MLD_LISTENER_QUERY:
    case MLD_LISTENER_REPORT:
    case MLD_LISTENER_DONE:
        if((n = m_copym(m, 0, M_COPYALL, M_DONTWAIT)) != NULL)
            mld6_input(n, off, icmp6len);
        break;

    /* ----- Neighbor Discovery (delegated to nd6.c) ----- */
    case ND_NEIGHBOR_SOLICIT:
        if((n = m_copym(m, 0, M_COPYALL, M_DONTWAIT)) != NULL)
            nd6_ns_input(n, off, icmp6len);
        break;

    case ND_NEIGHBOR_ADVERT:
        if((n = m_copym(m, 0, M_COPYALL, M_DONTWAIT)) != NULL)
            nd6_na_input(n, off, icmp6len);
        break;

    case ND_ROUTER_SOLICIT:
        if((n = m_copym(m, 0, M_COPYALL, M_DONTWAIT)) != NULL)
            nd6_rs_input(n, off, icmp6len);
        break;

    case ND_ROUTER_ADVERT:
        if((n = m_copym(m, 0, M_COPYALL, M_DONTWAIT)) != NULL)
            nd6_ra_input(n, off, icmp6len);
        break;

    case ND_REDIRECT:
        if((n = m_copym(m, 0, M_COPYALL, M_DONTWAIT)) != NULL)
            nd6_redirect_input(n, off, icmp6len);
        break;

    /* ----- Echo ----- */
    case ICMP6_ECHO_REQUEST:
        /* the reply is written into headers of its own */
        if((n = icmp6_echo_copy(m, off)) != NULL) {
            struct icmp6_hdr *nicmp6 =
                (struct icmp6_hdr *)(mtod(n, u_int8_t *) + off);

            icmp6stat.icp6s_reflect++;
            nicmp6->icmp6_type  = ICMP6_ECHO_REPLY;
            nicmp6->icmp6_code  = 0;
            nicmp6->icmp6_cksum = 0;
            icmp6_reflect(n, off);
        }
        break;

    case ICMP6_ECHO_REPLY:
        break;          /* for the raw sockets only */

    /* ----- Error messages ----- */
    case ICMP6_PACKET_TOO_BIG:
        /*
         * Update the path MTU for the destination of the quoted packet.
         * The original IPv6 header immediately follows the 8-byte ICMPv6
         * header; pull it up before reading the destination.
         */
        if(m->m_len >= off + (int)(sizeof(struct icmp6_hdr) + sizeof(struct ip6_hdr))
                || (m = m_pullup(m, off + sizeof(struct icmp6_hdr)
                                 + sizeof(struct ip6_hdr))) != NULL) {
            struct icmp6_hdr *ic6 = (struct icmp6_hdr *)(mtod(m, u_int8_t *) + off);
            struct ip6_hdr *oip6 = (struct ip6_hdr *)((u_int8_t *)ic6
                                                      + sizeof(struct icmp6_hdr));
            struct in6_addr odst;
            u_int32_t omtu;

            /* Copy out of the packed headers into aligned locals so no
             * unaligned access occurs on strict-alignment architectures. */
            bcopy(&oip6->ip6_dst, &odst, sizeof(odst));
            bcopy(&ic6->icmp6_mtu, &omtu, sizeof(omtu));
            /* the route keeps the path MTU, which ip6_output() and
             * tcp_mss() use: no socket needs to be told */
            icmp6_update_pmtu(&odst, ntohl(omtu));
            break;
        }
        return;			/* m_pullup freed the mbuf */

    /*
     * Tell the protocol of the quoted packet, with the destination it was
     * for: not every protocol of every domain (pfctlinput()), whose IPv4
     * handlers know no IPv6 address. The codes map as KAME's do.
     */
    case ICMP6_DST_UNREACH:
        switch(icmp6->icmp6_code) {
        case ICMP6_DST_UNREACH_NOROUTE:
        case ICMP6_DST_UNREACH_REJECT:
            icmp6_notify_error(m, off, PRC_UNREACH_NET);
            break;
        case ICMP6_DST_UNREACH_ADMIN:
        case ICMP6_DST_UNREACH_POLICY:
            icmp6_notify_error(m, off, PRC_UNREACH_PROTOCOL);
            break;
        case ICMP6_DST_UNREACH_BEYONDSCOPE:
        case ICMP6_DST_UNREACH_SRCROUTE:
            icmp6_notify_error(m, off, PRC_UNREACH_SRCFAIL);
            break;
        case ICMP6_DST_UNREACH_ADDR:
            icmp6_notify_error(m, off, PRC_HOSTDEAD);
            break;
        case ICMP6_DST_UNREACH_NOPORT:
            icmp6_notify_error(m, off, PRC_UNREACH_PORT);
            break;
        }
        break;

    case ICMP6_TIME_EXCEEDED:
        icmp6_notify_error(m, off, icmp6->icmp6_code == ICMP6_TIME_EXCEED_TRANSIT
                           ? PRC_TIMXCEED_INTRANS : PRC_TIMXCEED_REASS);
        break;

    case ICMP6_PARAM_PROB:
        icmp6_notify_error(m, off, PRC_PARAMPROB);
        break;

    default:
        break;
    }

    rip6_input(m, off, IPPROTO_ICMPV6);
}

/* ------------------------------------------------------------------
 * icmp6_error - generate an ICMPv6 error in response to m.
 *
 * type/code - ICMPv6 type and code
 * param     - depends on type (e.g. MTU for TOO_BIG, pointer for PARAM_PROB)
 * ------------------------------------------------------------------ */
void
icmp6_error(struct mbuf *m, int type, int code, int param)
{
    struct ip6_hdr *oip6, *nip6;
    struct icmp6_hdr *icmp6;
    int hlen, prepend;

    icmp6stat.icp6s_error++;

    if(m->m_len < sizeof(struct ip6_hdr) &&
            (m = m_pullup(m, sizeof(struct ip6_hdr))) == NULL)
        return;

    oip6 = mtod(m, struct ip6_hdr *);

    /* never send ICMPv6 errors in response to multicast */
    if(IN6_IS_ADDR_MULTICAST(&oip6->ip6_dst)) {
        m_freem(m);
        return;
    }
    /* never send ICMPv6 errors in response to ICMPv6 errors */
    if(oip6->ip6_nxt == IPPROTO_ICMPV6) {
        struct icmp6_hdr *oicmp6 = (struct icmp6_hdr *)(oip6 + 1);
        if(m->m_len >= sizeof(*oip6) + sizeof(*oicmp6) &&
                oicmp6->icmp6_type < ICMP6_ECHO_REQUEST) {
            m_freem(m);
            return;
        }
    }

    /* Rate-limit ICMPv6 error generation */
    if (!icmp6_ratelimit()) {
        m_freem(m);
        return;
    }

    /* include as much of the offending packet as possible (max 1280 - hdrs) */
    hlen = sizeof(struct ip6_hdr) + sizeof(struct icmp6_hdr);
    prepend = hlen;
    if(m->m_pkthdr.len + prepend > IPV6_MMTU)
        m_adj(m, IPV6_MMTU - prepend - m->m_pkthdr.len);

    /* prepend new IPv6 + ICMPv6 headers */
    M_PREPEND(m, prepend, M_DONTWAIT);
    if(m == NULL)
        return;

    nip6  = mtod(m, struct ip6_hdr *);
    icmp6 = (struct icmp6_hdr *)(nip6 + 1);

    /* copy original ip6 src -> new dst */
    bzero(nip6, sizeof(*nip6));
    nip6->ip6_vfc  = IPV6_VERSION;
    nip6->ip6_nxt  = IPPROTO_ICMPV6;
    nip6->ip6_hlim = ip6_defhlim;
    nip6->ip6_dst  = oip6->ip6_src;
    /* source: find appropriate local address */
    {
        struct in6_ifaddr *ia = NULL;
        if(m->m_pkthdr.rcvif) {
            struct in6_addr dst_copy = nip6->ip6_dst;
            ia = in6_ifawithifp(m->m_pkthdr.rcvif, &dst_copy);
        }
        if(ia)
            nip6->ip6_src = ia->ia_addr.sin6_addr;
        /* else: stays 0 - routing will fix it */
    }

    icmp6->icmp6_type  = (u_int8_t)type;
    icmp6->icmp6_code  = (u_int8_t)code;
    icmp6->icmp6_cksum = 0;
    icmp6->icmp6_dataun.icmp6_un_data32[0] = htonl(param);

    nip6->ip6_plen = htons(m->m_pkthdr.len - sizeof(struct ip6_hdr));

    /* compute ICMPv6 checksum with pseudo-header */
    icmp6->icmp6_cksum = 0;
    icmp6->icmp6_cksum = in6_cksum(m, IPPROTO_ICMPV6,
                                   sizeof(struct ip6_hdr),
                                   m->m_pkthdr.len - sizeof(struct ip6_hdr));

    icmp6stat.icp6s_outhist[type]++;

    ip6_output(m, (struct mbuf *)NULL, (struct route *)NULL, 0,
               (struct ip6_moptions *)NULL, (struct ifnet **)NULL,
               (struct inpcb *)NULL);
}

/* ------------------------------------------------------------------
 * icmp6_reflect - swap src/dst and send an ICMPv6 reply.
 * ------------------------------------------------------------------ */
static void
icmp6_reflect(struct mbuf *m, size_t off)
{
    struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
    struct in6_addr t;
    struct in6_ifaddr *ia;

    /* swap addresses: reply goes to original sender */
    t = ip6->ip6_dst;               /* save our address (original dst) */
    ip6->ip6_dst = ip6->ip6_src;    /* new dst = original src (remote peer) */
    /* find the best local source address for the reply */
    {
        struct in6_addr dst_copy = ip6->ip6_dst;
        ia = in6_ifawithifp(m->m_pkthdr.rcvif, &dst_copy);
    }
    if(ia)
        ip6->ip6_src = ia->ia_addr.sin6_addr;
    else
        ip6->ip6_src = t;           /* fallback: our original dst address */

    ip6->ip6_hlim = ip6_defhlim;

    /* compute ICMPv6 checksum with pseudo-header */
    {
        struct icmp6_hdr *icmp6r;
        icmp6r = (struct icmp6_hdr *)((caddr_t)ip6 + off);
        icmp6r->icmp6_cksum = 0;
        icmp6r->icmp6_cksum = in6_cksum(m, IPPROTO_ICMPV6,
                                        off, m->m_pkthdr.len - off);
    }

    ip6_output(m, (struct mbuf *)NULL, (struct route *)NULL, 0,
               (struct ip6_moptions *)NULL, (struct ifnet **)NULL,
               (struct inpcb *)NULL);
}

#endif /* INET6 */
