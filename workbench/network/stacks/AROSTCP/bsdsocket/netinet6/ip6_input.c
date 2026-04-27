/*
 * Copyright (C) 2020-2026 The AROS Development Team.  All rights reserved.
 *
 * Based on KAME IPv6 implementation:
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * ip6_input.c - IPv6 input processing.
 *
 * Handles inbound IPv6 packets: basic header validation, hop-limit
 * decrement (when forwarding), extension-header parsing, reassembly
 * dispatch, and demultiplexing to the correct upper-layer protocol.
 */

#include <conf.h>

#if INET6

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/synch.h>

#include <net/if.h>
#include <net/route.h>
#include <net/pfil.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_var.h>
#include <netinet/in_pcb.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>

#include <kern/uipc_domain_protos.h>

/* ------------------------------------------------------------------
 * Globals
 * ------------------------------------------------------------------ */
struct ip6stat ip6stat = {0};

extern struct domain inet6domain;
extern struct protosw inet6sw[];
u_char ip6_protox[IPPROTO_MAX] = {0};

/*
 * forward declarations (defined in icmp6.c).
 */
void icmp6_error(struct mbuf *, int, int, int);
void ip6_input(struct mbuf *m);

/* ------------------------------------------------------------------
 * IPv6 fragment reassembly (RFC 8200 §4.5).
 * ------------------------------------------------------------------ */
#define IP6_MAXFRAGS      64   /* max fragments held system-wide */
#define IP6_FRAGTTL       60   /* fragment lifetime (slow ticks = 30s) */
#define IP6_MAXFRAGPACKETS 16  /* max concurrent reassembly queues */

struct ip6q {
    struct ip6q  *next;
    struct ip6q  *prev;
    struct ip6asfrag *ip6q_down;  /* fragment chain */
    u_int32_t     ip6q_ident;     /* fragment identification */
    u_int8_t      ip6q_nxt;       /* next header of original */
    struct in6_addr ip6q_src;
    struct in6_addr ip6q_dst;
    int           ip6q_ttl;       /* time to live (ticks) */
    int           ip6q_nfrag;     /* # fragments received */
    int           ip6q_unfrglen;  /* unfragmentable part length */
};

struct ip6asfrag {
    struct ip6asfrag *next;
    struct ip6asfrag *prev;
    struct mbuf      *m;
    int               off;       /* offset in original datagram */
    int               frglen;    /* fragment payload length */
    int               mff;       /* more fragments flag */
};

static struct ip6q ip6q = { &ip6q, &ip6q, NULL, 0, 0, {{0}}, {{0}}, 0, 0, 0 };
static int ip6q_nfragpackets = 0;
static int ip6_nfrags = 0;

static void frag6_freef(struct ip6q *);
static struct mbuf *frag6_input(struct mbuf *, int);

/*
 * frag6_freef - free an entire reassembly queue.
 */
static void
frag6_freef(struct ip6q *q6)
{
    struct ip6asfrag *af6, *naf6;

    for(af6 = q6->ip6q_down; af6; af6 = naf6) {
        naf6 = af6->next;
        m_freem(af6->m);
        bsd_free(af6, M_FTABLE);
        ip6_nfrags--;
    }
    q6->prev->next = q6->next;
    q6->next->prev = q6->prev;
    ip6q_nfragpackets--;
    bsd_free(q6, M_FTABLE);
}

/*
 * frag6_input - process an IPv6 fragment.
 *
 * m contains the full packet with ip6 header.
 * off is the offset to the Fragment header within the mbuf.
 * Returns a reassembled mbuf or NULL if reassembly is not yet complete.
 */
static struct mbuf *
frag6_input(struct mbuf *m, int off)
{
    struct ip6_hdr  *ip6;
    struct ip6_frag *fh;
    struct ip6q     *q6;
    struct ip6asfrag *af6, *naf6;
    int fragoff, frglen, mff;
    spl_t s;

    ip6 = mtod(m, struct ip6_hdr *);

    /* pull up fragment header */
    if(m->m_len < off + sizeof(struct ip6_frag) &&
       (m = m_pullup(m, off + sizeof(struct ip6_frag))) == NULL) {
        ip6stat.ip6s_toosmall++;
        return NULL;
    }
    ip6 = mtod(m, struct ip6_hdr *);
    fh  = (struct ip6_frag *)(mtod(m, u_int8_t *) + off);

    fragoff = ntohs(fh->ip6f_offlg & IP6F_OFF_MASK);
    mff     = fh->ip6f_offlg & IP6F_MORE_FRAG;
    frglen  = m->m_pkthdr.len - off - sizeof(struct ip6_frag);

    /* fragment length must be multiple of 8, except for last fragment */
    if(mff && (frglen & 7)) {
        icmp6_error(m, ICMP6_PARAM_PROB,
                    ICMP6_PARAMPROB_HEADER,
                    offsetof(struct ip6_hdr, ip6_plen));
        return NULL;
    }

    /* limit total fragments in system */
    if(ip6_nfrags >= IP6_MAXFRAGS) {
        ip6stat.ip6s_fragoverflow++;
        m_freem(m);
        return NULL;
    }

    s = splimp();

    /* find existing reassembly queue */
    for(q6 = ip6q.next; q6 != &ip6q; q6 = q6->next) {
        if(q6->ip6q_ident == fh->ip6f_ident &&
           IN6_ARE_ADDR_EQUAL(&q6->ip6q_src, &ip6->ip6_src) &&
           IN6_ARE_ADDR_EQUAL(&q6->ip6q_dst, &ip6->ip6_dst))
            break;
    }

    if(q6 == &ip6q) {
        /* new reassembly queue */
        if(ip6q_nfragpackets >= IP6_MAXFRAGPACKETS) {
            /* drop oldest */
            struct ip6q *oldest = ip6q.prev;
            if(oldest != &ip6q)
                frag6_freef(oldest);
        }

        q6 = (struct ip6q *)bsd_malloc(sizeof(*q6), M_FTABLE, M_NOWAIT);
        if(q6 == NULL) {
            splx(s);
            m_freem(m);
            return NULL;
        }
        bzero(q6, sizeof(*q6));

        q6->ip6q_ident = fh->ip6f_ident;
        q6->ip6q_src   = ip6->ip6_src;
        q6->ip6q_dst   = ip6->ip6_dst;
        q6->ip6q_nxt   = fh->ip6f_nxt;
        q6->ip6q_ttl   = IP6_FRAGTTL;
        q6->ip6q_unfrglen = off - sizeof(struct ip6_hdr);

        /* insert at head of queue list */
        q6->next = ip6q.next;
        q6->prev = &ip6q;
        ip6q.next->prev = q6;
        ip6q.next = q6;
        ip6q_nfragpackets++;
    }

    /* allocate fragment descriptor */
    af6 = (struct ip6asfrag *)bsd_malloc(sizeof(*af6), M_FTABLE, M_NOWAIT);
    if(af6 == NULL) {
        splx(s);
        m_freem(m);
        return NULL;
    }
    af6->m      = m;
    af6->off    = fragoff;
    af6->frglen = frglen;
    af6->mff    = mff ? 1 : 0;

    /* insert in offset-sorted order */
    {
        struct ip6asfrag *prev_af = NULL;
        struct ip6asfrag *cur;
        for(cur = q6->ip6q_down; cur; cur = cur->next) {
            if(cur->off > af6->off)
                break;
            prev_af = cur;
        }
        af6->next = cur;
        af6->prev = prev_af;
        if(prev_af)
            prev_af->next = af6;
        else
            q6->ip6q_down = af6;
        if(cur)
            cur->prev = af6;
    }

    q6->ip6q_nfrag++;
    ip6_nfrags++;

    /* check if reassembly is complete */
    {
        int next_expected = 0;
        int have_last = 0;
        for(af6 = q6->ip6q_down; af6; af6 = af6->next) {
            if(af6->off != next_expected) {
                splx(s);
                return NULL;  /* gap - not yet complete */
            }
            next_expected = af6->off + af6->frglen;
            if(!af6->mff)
                have_last = 1;
        }
        if(!have_last) {
            splx(s);
            return NULL;  /* haven't seen last fragment */
        }
    }

    /* reassembly complete - concatenate fragments */
    {
        struct mbuf *result;
        struct ip6_hdr *rip6;

        /* take first fragment's mbuf as the base */
        af6 = q6->ip6q_down;
        result = af6->m;

        rip6 = mtod(result, struct ip6_hdr *);

        /* Remove the fragment header from the first mbuf:
         * shift data after frag hdr back over the frag hdr */
        {
            int fhoff = sizeof(struct ip6_hdr) + q6->ip6q_unfrglen;
            int fhsize = sizeof(struct ip6_frag);
            if(result->m_len > fhoff + fhsize) {
                ovbcopy(mtod(result, caddr_t) + fhoff + fhsize,
                        mtod(result, caddr_t) + fhoff,
                        result->m_len - fhoff - fhsize);
            }
            result->m_len -= fhsize;
            result->m_pkthdr.len = af6->frglen + sizeof(struct ip6_hdr)
                                   + q6->ip6q_unfrglen;
        }

        /* append remaining fragments' data */
        {
            struct mbuf *tail = result;
            while(tail->m_next)
                tail = tail->m_next;

            naf6 = af6->next;
            while(naf6) {
                struct ip6asfrag *next = naf6->next;
                struct mbuf *fm = naf6->m;
                int strip = sizeof(struct ip6_hdr) + q6->ip6q_unfrglen
                            + sizeof(struct ip6_frag);
                m_adj(fm, strip);
                result->m_pkthdr.len += fm->m_pkthdr.len;
                tail->m_next = fm;
                while(tail->m_next)
                    tail = tail->m_next;
                bsd_free(naf6, M_FTABLE);
                ip6_nfrags--;
                naf6 = next;
            }
        }

        /* fix up the IPv6 header */
        rip6 = mtod(result, struct ip6_hdr *);
        rip6->ip6_plen = htons(result->m_pkthdr.len - sizeof(struct ip6_hdr));
        rip6->ip6_nxt  = q6->ip6q_nxt;

        /* free the first fragment descriptor (mbuf is now result) */
        bsd_free(af6, M_FTABLE);
        ip6_nfrags--;

        /* unlink and free queue header */
        q6->prev->next = q6->next;
        q6->next->prev = q6->prev;
        ip6q_nfragpackets--;
        bsd_free(q6, M_FTABLE);

        ip6stat.ip6s_reassembled++;
        splx(s);
        return result;
    }
}

/*
 * frag6_slowtimo - age out fragment reassembly queues.
 *
 * Called from icmp6_slowtimo() (once per slow timeout tick).
 */
void
frag6_slowtimo(void)
{
    struct ip6q *q6, *nq6;
    spl_t s = splimp();

    for(q6 = ip6q.next; q6 != &ip6q; q6 = nq6) {
        nq6 = q6->next;
        if(--q6->ip6q_ttl <= 0) {
            ip6stat.ip6s_fragtimeout++;
            /* send ICMPv6 Time Exceeded if we have the first fragment */
            if(q6->ip6q_down && q6->ip6q_down->off == 0) {
                icmp6_error(q6->ip6q_down->m, ICMP6_TIME_EXCEEDED,
                            ICMP6_TIME_EXCEED_REASSEMBLY, 0);
                q6->ip6q_down->m = NULL; /* consumed by icmp6_error */
            }
            frag6_freef(q6);
        }
    }
    splx(s);
}

/*
 * ip6_forward - forward an IPv6 packet to its destination.
 *
 * Performs route lookup, scope checks, and hands off to the
 * outgoing interface. Generates ICMPv6 errors as needed.
 */
static void
ip6_forward(struct mbuf *m, struct ifnet *srcifp)
{
    struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
    struct route_in6 ip6route;
    struct route *ro;
    struct sockaddr_in6 *dst;
    struct rtentry *rt;
    struct ifnet *ifp;
    int error;
    spl_t s;

    bzero(&ip6route, sizeof(ip6route));
    ro = (struct route *)&ip6route;
    dst = (struct sockaddr_in6 *)&ro->ro_dst;
    dst->sin6_family = AF_INET6;
    dst->sin6_len    = sizeof(*dst);
    dst->sin6_addr   = ip6->ip6_dst;

    rtalloc(ro);
    if(ro->ro_rt == NULL) {
        icmp6_error(m, ICMP6_DST_UNREACH,
                    ICMP6_DST_UNREACH_NOROUTE, 0);
        ip6stat.ip6s_noroute++;
        return;
    }
    rt  = ro->ro_rt;
    ifp = rt->rt_ifp;

    /* Packet too big? Send ICMPv6 Packet Too Big */
    if(m->m_pkthdr.len > ifp->if_mtu) {
        icmp6_error(m, ICMP6_PACKET_TOO_BIG, 0,
                    ifp->if_mtu);
        RTFREE(rt);
        ip6stat.ip6s_cantfrag++;
        return;
    }

    /* Update the payload length (trimming was done in ip6_input) */
    ip6->ip6_plen = htons((u_int16_t)(m->m_pkthdr.len
                                       - sizeof(struct ip6_hdr)));

    /* send redirect if forwarding on same interface */
    if(ifp == srcifp) {
        /* TODO: send ICMPv6 redirect */
        ip6stat.ip6s_redirectsent++;
    }

    /* output via the outgoing interface */
    s = splimp();
    error = (*ifp->if_output)(ifp, m,
                              (rt->rt_flags & RTF_GATEWAY)
                              ? rt->rt_gateway
                              : (struct sockaddr *)dst,
                              rt);
    splx(s);

    if(error)
        ip6stat.ip6s_odropped++;
    else
        ip6stat.ip6s_forward++;

    RTFREE(rt);
}

/* ------------------------------------------------------------------
 * ip6_init - called once during domain initialisation.
 * ------------------------------------------------------------------ */
void
ip6_init(void)
{
    struct protosw *pr;
    int i;

    pr = pffindproto(AF_INET6, IPPROTO_RAW, SOCK_RAW);
    if(pr == NULL)
        panic("ip6_init: no raw IPv6");

    /* build protocol dispatch table */
    for(i = 0; i < IPPROTO_MAX; i++)
        ip6_protox[i] = (u_char)(pr - inet6sw);

    for(pr = inet6domain.dom_protosw;
            pr < inet6domain.dom_protoswNPROTOSW;
            pr++) {
        if(pr->pr_protocol && pr->pr_protocol != IPPROTO_RAW)
            ip6_protox[pr->pr_protocol] = (u_char)(pr - inet6sw);
    }

    nd6_init();

    ip6intrq.ifq_maxlen = IFQ_MAXLEN;
}

/* ------------------------------------------------------------------
 * ip6intr - software interrupt: drain ip6intrq.
 * ------------------------------------------------------------------ */
void
ip6intr(void)
{
    struct mbuf *m;
    spl_t s;

    for(;;) {
        s = splimp();
        IF_DEQUEUE(&ip6intrq, m);
        splx(s);
        if(m == NULL)
            break;
        ip6_input(m);
    }
}

/* ------------------------------------------------------------------
 * ip6_input - process a single inbound IPv6 packet.
 *
 * m->m_pkthdr.rcvif must be set to the receiving interface.
 * ------------------------------------------------------------------ */
void
ip6_input(struct mbuf *m)
{
    struct ip6_hdr *ip6;
    struct ifnet   *ifp = m->m_pkthdr.rcvif;
    struct in6_ifaddr *ia = NULL;
    struct protosw *pr;
    u_int8_t nxt;
    int off, plen;

    ip6stat.ip6s_total++;

    D({
        struct ip6_hdr *_ip6 = mtod(m, struct ip6_hdr *);
        u_int8_t *s = (u_int8_t *)&_ip6->ip6_src;
        u_int8_t *d = (u_int8_t *)&_ip6->ip6_dst;
        bug("[AROSTCP:IP6] %s: pktlen=%d nxt=%d\n"
            "  src=%02x%02x:%02x%02x:%02x%02x:%02x%02x:"
            "%02x%02x:%02x%02x:%02x%02x:%02x%02x\n"
            "  dst=%02x%02x:%02x%02x:%02x%02x:%02x%02x:"
            "%02x%02x:%02x%02x:%02x%02x:%02x%02x\n",
            __func__, m->m_pkthdr.len, _ip6->ip6_nxt,
            s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7],
            s[8], s[9], s[10], s[11], s[12], s[13], s[14], s[15],
            d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7],
            d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);
    });

    /* pull up to minimum header size */
    if(m->m_len < sizeof(struct ip6_hdr) &&
            (m = m_pullup(m, sizeof(struct ip6_hdr))) == NULL) {
        ip6stat.ip6s_toosmall++;
        return;
    }
    ip6 = mtod(m, struct ip6_hdr *);

    /* version check */
    if((ip6->ip6_vfc & IPV6_VERSION_MASK) != IPV6_VERSION) {
        ip6stat.ip6s_badvers++;
        goto bad;
    }

    plen = ntohs(ip6->ip6_plen);
    if(m->m_pkthdr.len < sizeof(struct ip6_hdr) + plen) {
        ip6stat.ip6s_tooshort++;
        goto bad;
    }
    /* trim any trailing junk */
    if(m->m_pkthdr.len > sizeof(struct ip6_hdr) + plen)
        m_adj(m, sizeof(struct ip6_hdr) + plen - m->m_pkthdr.len);

    /* discard unspecified source except when link-local */
    if(IN6_IS_ADDR_UNSPECIFIED(&ip6->ip6_src) &&
            !IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst)) {
        ip6stat.ip6s_badscope++;
        goto bad;
    }

    /* check if destination is one of our addresses */
    for(ia = in6_ifaddr; ia; ia = ia->ia_next) {
        if(IN6_ARE_ADDR_EQUAL(&ip6->ip6_dst, &ia->ia_addr.sin6_addr))
            goto ours;
        /* also accept solicited-node multicast for our addr */
        if(IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst)) {
            /* ff02::1:ffXX:XXXX */
            if(ip6->ip6_dst.s6_addr[0] == 0xff &&
                    ip6->ip6_dst.s6_addr[1] == 0x02 &&
                    ip6->ip6_dst.s6_addr[11] == 0x01 &&
                    ip6->ip6_dst.s6_addr[12] == 0xff &&
                    ip6->ip6_dst.s6_addr[13] == ia->ia_addr.sin6_addr.s6_addr[13] &&
                    ip6->ip6_dst.s6_addr[14] == ia->ia_addr.sin6_addr.s6_addr[14] &&
                    ip6->ip6_dst.s6_addr[15] == ia->ia_addr.sin6_addr.s6_addr[15])
                goto ours;
        }
    }

    /* accept all-nodes multicast ff02::1 */
    if(ip6->ip6_dst.s6_addr[0] == 0xff &&
            ip6->ip6_dst.s6_addr[1] == 0x02 &&
            ip6->ip6_dst.s6_addr[15] == 0x01)
        goto ours;

    /* forwarding */
    if(ip6_forwarding) {
        /* Don't forward link-local scope */
        if(IN6_IS_ADDR_LINKLOCAL(&ip6->ip6_src) ||
           IN6_IS_ADDR_LINKLOCAL(&ip6->ip6_dst)) {
            ip6stat.ip6s_cantforward++;
            goto bad;
        }

        if(ip6->ip6_hlim <= 1) {
            icmp6_error(m, ICMP6_TIME_EXCEEDED,
                        ICMP6_TIME_EXCEED_TRANSIT, 0);
            return;
        }
        ip6->ip6_hlim--;

        ip6_forward(m, ifp);
        return;
    } else {
        ip6stat.ip6s_cantforward++;
    }
    m_freem(m);
    return;

ours:
    ip6stat.ip6s_delivered++;

    /* walk extension headers and dispatch */
    off = sizeof(struct ip6_hdr);
    nxt = ip6->ip6_nxt;

    /* simple extension header skip loop */
    while(1) {
        struct ip6_ext *ext;
        int extlen;

        switch(nxt) {
        case IPPROTO_HOPOPTS:
        case IPPROTO_ROUTING:
        case IPPROTO_DSTOPTS:
            if(m->m_len < off + sizeof(struct ip6_ext) &&
                    (m = m_pullup(m, off + sizeof(struct ip6_ext))) == NULL) {
                ip6stat.ip6s_tooshort++;
                return;
            }
            ext = (struct ip6_ext *)(mtod(m, u_int8_t *) + off);
            extlen = (ext->ip6e_len + 1) * 8;
            nxt = ext->ip6e_nxt;
            off += extlen;
            break;

        case IPPROTO_FRAGMENT:
        {
            struct mbuf *reasm;
            ip6stat.ip6s_fragments++;
            reasm = frag6_input(m, off);
            if(reasm == NULL)
                return;  /* not yet complete or error */
            /* reassembly complete, process the full packet */
            m = reasm;
            ip6 = mtod(m, struct ip6_hdr *);
            nxt = ip6->ip6_nxt;
            off = sizeof(struct ip6_hdr);
            /* continue extension header processing with reassembled packet */
            break;
        }

        case IPPROTO_NONE:
            m_freem(m);
            return;

        default:
            goto deliver;
        }
    }

deliver:
    /* dispatch to upper-layer protocol */
    pr = &inet6sw[ip6_protox[nxt]];
    if(pr->pr_input)
        (*pr->pr_input)(m, off, nxt);
    else
        m_freem(m);
    return;

bad:
    m_freem(m);
}

#endif /* INET6 */
