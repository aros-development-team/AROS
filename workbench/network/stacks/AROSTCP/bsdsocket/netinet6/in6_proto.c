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
 * in6_proto.c - IPv6 protocol domain and switch table.
 *
 * Defines the inet6 domain and the inet6sw[] protocol switch,
 * registering ICMPv6, UDP-over-IPv6, TCP-over-IPv6 and raw IPv6.
 *
 * Forward declarations use K&R unprototyped form (matching the pattern
 * used by netinet/in_proto.c) so that function pointer assignments into
 * the generic void/int (*)(void *, ...) protosw fields compile cleanly.
 */

#include <conf.h>

#if INET6

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_pcb.h>
#include <netinet6/in6_var.h>
#include <kern/amiga_dhcp.h>

#if DHCP6
/* IPv6 DHCP client state - one global instance for the IPv6 protocol family */
struct aros_dhcp_state aros_dhcpv6;
#endif

/* --- ANSI function declarations matching actual signatures ---
 * pr_input  must be void (*)(void *args, ...)
 * pr_output must be int  (*)(void *args, ...)
 * pr_init / pr_fasttimo / pr_slowtimo / pr_drain: void (*)(void)
 */
void ip6_init(void);
void icmp6_input(void *args, ...);
void icmp6_init(void);
void icmp6_fasttimo(void);
void icmp6_slowtimo(void);
void icmp6_drain(void);

int  ip6_output(void *args, ...);
void rip6_input(void *args, ...);
int  rip6_output(void *args, ...);
int  rip6_usrreq(struct socket *, int, struct mbuf *, struct mbuf *,
                 struct mbuf *);

/* TCP/UDP shared with IPv4 */
#include <netinet/tcp_usrreq_protos.h>
#include <netinet/udp_usrreq_protos.h>
void tcp_input(void *args, ...);
void udp_input(void *args, ...);
#include <netinet/ip_input_protos.h>

/* UDP over IPv6 */
void udp6_input(void *args, ...);
int  udp6_usrreq(struct socket *, int, struct mbuf *, struct mbuf *,
                 struct mbuf *);

/* ctloutput stubs: old BSD 5-arg signature */
int rip6_ctloutput(int op, struct socket *so, int level, int optname,
                   struct mbuf **m);
int icmp6_ctloutput(int op, struct socket *so, int level, int optname,
                    struct mbuf **m);
int ip6_ctloutput(int op, struct socket *so, int level, int optname,
                  struct mbuf **m);

extern struct domain inet6domain;

struct protosw inet6sw[] = {
    /* base IPv6 entry: handles outbound path and timers */
    { 0,            &inet6domain,   0,              0,
      NULL,         ip6_output,     NULL,           NULL,
      NULL,
      ip6_init,     NULL,           NULL,           NULL,
    },
    /* UDP over IPv6 */
    { SOCK_DGRAM,   &inet6domain,   IPPROTO_UDP,    PR_ATOMIC|PR_ADDR,
      udp6_input,
      NULL,
      udp_ctlinput,
      ip6_ctloutput,
      udp6_usrreq,
      NULL,         NULL,           NULL,           NULL,
    },
    /* TCP over IPv6 */
    { SOCK_STREAM,  &inet6domain,   IPPROTO_TCP,
      PR_CONNREQUIRED|PR_IMPLOPCL|PR_WANTRCVD,
      tcp_input,    NULL,           tcp_ctlinput,   tcp_ctloutput,
      tcp_usrreq,
      NULL,         tcp_fasttimo,   tcp_slowtimo,   tcp_drain,
    },
    /* ICMPv6 */
    { SOCK_RAW,     &inet6domain,   IPPROTO_ICMPV6, PR_ATOMIC|PR_ADDR,
      icmp6_input,
      rip6_output,
      NULL,
      icmp6_ctloutput,
      rip6_usrreq,
      icmp6_init,   icmp6_fasttimo, icmp6_slowtimo, icmp6_drain,
    },
    /* raw IPv6 */
    { SOCK_RAW,     &inet6domain,   IPPROTO_RAW,    PR_ATOMIC|PR_ADDR,
      rip6_input,   rip6_output,    NULL,           rip6_ctloutput,
      rip6_usrreq,
      NULL,         NULL,           NULL,           NULL,
    },
    /* raw IPv6 wildcard */
    { SOCK_RAW,     &inet6domain,   0,              PR_ATOMIC|PR_ADDR,
      rip6_input,   rip6_output,    NULL,           rip6_ctloutput,
      rip6_usrreq,
      NULL,         NULL,           NULL,           NULL,
    },
};

struct domain inet6domain = {
    AF_INET6, "internet6", NULL, NULL, NULL,
    inet6sw, &inet6sw[sizeof(inet6sw)/sizeof(inet6sw[0])], NULL
};

/* ------------------------------------------------------------------
 * Helper: get or allocate ip6_moptions for a socket.
 * ------------------------------------------------------------------ */
static struct ip6_moptions *
ip6_getmoptions(struct ip6_moptions **imop)
{
    struct ip6_moptions *im6o = *imop;

    if (im6o == NULL) {
        MALLOC(im6o, struct ip6_moptions *, sizeof(*im6o),
               M_SOOPTS, M_WAITOK);
        if (im6o == NULL)
            return NULL;
        im6o->im6o_multicast_ifp  = NULL;
        im6o->im6o_multicast_hlim = IP6_DEFAULT_MULTICAST_HLIM;
        im6o->im6o_multicast_loop = IP6_DEFAULT_MULTICAST_LOOP;
        im6o->im6o_num_memberships = 0;
        *imop = im6o;
    }
    return im6o;
}

/* ------------------------------------------------------------------
 * Helper: join a multicast group on an inpcb socket.
 * ------------------------------------------------------------------ */
static int
in6_joingroup_inp(struct inpcb *inp, struct ipv6_mreq *mreq)
{
    struct ip6_moptions *im6o;
    struct ifnet *ifp;
    struct in6_multi *in6m;
    int i;

    im6o = ip6_getmoptions(&inp->in6p_moptions);
    if (im6o == NULL)
        return ENOBUFS;

    if (im6o->im6o_num_memberships >= IPV6_MAX_MEMBERSHIPS)
        return ETOOMANYREFS;

    /* find interface by index; 0 means any */
    if (mreq->ipv6mr_interface == 0) {
        /* pick first non-loopback interface */
        extern struct ifnet *ifnet;
        for (ifp = ifnet; ifp; ifp = ifp->if_next) {
            if ((ifp->if_flags & (IFF_UP | IFF_LOOPBACK)) == IFF_UP)
                break;
        }
        if (ifp == NULL)
            return EADDRNOTAVAIL;
    } else {
        extern struct ifnet *ifnet;
        for (ifp = ifnet; ifp; ifp = ifp->if_next)
            if (ifp->if_index == mreq->ipv6mr_interface)
                break;
        if (ifp == NULL)
            return ENXIO;
    }

    /* check not already a member */
    for (i = 0; i < im6o->im6o_num_memberships; i++) {
        if (im6o->im6o_membership[i]->in6m_ifp == ifp &&
            IN6_ARE_ADDR_EQUAL(&im6o->im6o_membership[i]->in6m_addr,
                               &mreq->ipv6mr_multiaddr))
            return EADDRINUSE;
    }

    in6m = in6_addmulti(&mreq->ipv6mr_multiaddr, ifp);
    if (in6m == NULL)
        return ENOBUFS;

    im6o->im6o_membership[im6o->im6o_num_memberships++] = in6m;
    return 0;
}

/* ------------------------------------------------------------------
 * Helper: leave a multicast group on an inpcb socket.
 * ------------------------------------------------------------------ */
static int
in6_leavegroup_inp(struct inpcb *inp, struct ipv6_mreq *mreq)
{
    struct ip6_moptions *im6o = inp->in6p_moptions;
    struct ifnet *ifp;
    int i;

    if (im6o == NULL)
        return EADDRNOTAVAIL;

    /* find interface */
    if (mreq->ipv6mr_interface == 0) {
        ifp = NULL;
    } else {
        extern struct ifnet *ifnet;
        for (ifp = ifnet; ifp; ifp = ifp->if_next)
            if (ifp->if_index == mreq->ipv6mr_interface)
                break;
        if (ifp == NULL)
            return ENXIO;
    }

    for (i = 0; i < im6o->im6o_num_memberships; i++) {
        if ((ifp == NULL || im6o->im6o_membership[i]->in6m_ifp == ifp) &&
            IN6_ARE_ADDR_EQUAL(&im6o->im6o_membership[i]->in6m_addr,
                               &mreq->ipv6mr_multiaddr)) {
            in6_delmulti(im6o->im6o_membership[i]);
            /* compact array */
            im6o->im6o_num_memberships--;
            for (; i < im6o->im6o_num_memberships; i++)
                im6o->im6o_membership[i] = im6o->im6o_membership[i + 1];
            return 0;
        }
    }
    return EADDRNOTAVAIL;
}

/*
 * ip6_ctloutput - SOL_IPV6 socket option handler (for inpcb-based sockets).
 */
int
ip6_ctloutput(int op, struct socket *so, int level, int optname,
              struct mbuf **m)
{
    struct inpcb *inp;
    struct ip6_moptions *im6o;
    int error = 0;

    if (level != IPPROTO_IPV6)
        return EINVAL;

    inp = sotoinpcb(so);
    if (inp == NULL)
        return EINVAL;

    switch (op) {

    case PRCO_SETOPT:
        switch (optname) {

        case IPV6_UNICAST_HOPS:
            if (*m == NULL || (*m)->m_len != sizeof(int)) {
                error = EINVAL; break;
            }
            inp->in6p_hops = *mtod(*m, int *);
            break;

        case IPV6_V6ONLY:
            if (*m == NULL || (*m)->m_len != sizeof(int)) {
                error = EINVAL; break;
            }
            inp->in6p_v6only = (*mtod(*m, int *) != 0);
            break;

        case IPV6_TCLASS:
            if (*m == NULL || (*m)->m_len != sizeof(int)) {
                error = EINVAL; break;
            }
            inp->in6p_tclass = *mtod(*m, int *);
            break;

        case IPV6_RECVPKTINFO:
            if (*m == NULL || (*m)->m_len != sizeof(int)) {
                error = EINVAL; break;
            }
            if (*mtod(*m, int *))
                inp->in6p_flags |= IN6P_PKTINFO;
            else
                inp->in6p_flags &= ~IN6P_PKTINFO;
            break;

        case IPV6_MULTICAST_IF:
            {
                unsigned int ifindex;
                struct ifnet *ifp;
                extern struct ifnet *ifnet;

                im6o = ip6_getmoptions(&inp->in6p_moptions);
                if (im6o == NULL) { error = ENOBUFS; break; }

                if (*m == NULL || (*m)->m_len != sizeof(unsigned int)) {
                    error = EINVAL; break;
                }
                ifindex = *mtod(*m, unsigned int *);
                if (ifindex == 0) {
                    im6o->im6o_multicast_ifp = NULL;
                    break;
                }
                for (ifp = ifnet; ifp; ifp = ifp->if_next)
                    if (ifp->if_index == ifindex)
                        break;
                if (ifp == NULL) { error = ENXIO; break; }
                im6o->im6o_multicast_ifp = ifp;
            }
            break;

        case IPV6_MULTICAST_HOPS:
            {
                int hlim;
                im6o = ip6_getmoptions(&inp->in6p_moptions);
                if (im6o == NULL) { error = ENOBUFS; break; }
                if (*m == NULL || (*m)->m_len != sizeof(int)) {
                    error = EINVAL; break;
                }
                hlim = *mtod(*m, int *);
                if (hlim < -1 || hlim > 255) { error = EINVAL; break; }
                im6o->im6o_multicast_hlim =
                    (hlim < 0) ? IP6_DEFAULT_MULTICAST_HLIM : (u_int8_t)hlim;
            }
            break;

        case IPV6_MULTICAST_LOOP:
            {
                unsigned int loop;
                im6o = ip6_getmoptions(&inp->in6p_moptions);
                if (im6o == NULL) { error = ENOBUFS; break; }
                if (*m == NULL || (*m)->m_len != sizeof(unsigned int)) {
                    error = EINVAL; break;
                }
                loop = *mtod(*m, unsigned int *);
                if (loop > 1) { error = EINVAL; break; }
                im6o->im6o_multicast_loop = (u_int8_t)loop;
            }
            break;

        case IPV6_JOIN_GROUP:
            if (*m == NULL || (*m)->m_len != sizeof(struct ipv6_mreq)) {
                error = EINVAL; break;
            }
            error = in6_joingroup_inp(inp, mtod(*m, struct ipv6_mreq *));
            break;

        case IPV6_LEAVE_GROUP:
            if (*m == NULL || (*m)->m_len != sizeof(struct ipv6_mreq)) {
                error = EINVAL; break;
            }
            error = in6_leavegroup_inp(inp, mtod(*m, struct ipv6_mreq *));
            break;

        default:
            error = ENOPROTOOPT;
            break;
        }
        if (*m) { m_freem(*m); *m = NULL; }
        break;

    case PRCO_GETOPT:
        switch (optname) {

        case IPV6_UNICAST_HOPS:
            *m = m_get(M_WAIT, MT_SOOPTS);
            if (*m == NULL) { error = ENOBUFS; break; }
            (*m)->m_len = sizeof(int);
            *mtod(*m, int *) = inp->in6p_hops;
            break;

        case IPV6_V6ONLY:
            *m = m_get(M_WAIT, MT_SOOPTS);
            if (*m == NULL) { error = ENOBUFS; break; }
            (*m)->m_len = sizeof(int);
            *mtod(*m, int *) = inp->in6p_v6only;
            break;

        case IPV6_TCLASS:
            *m = m_get(M_WAIT, MT_SOOPTS);
            if (*m == NULL) { error = ENOBUFS; break; }
            (*m)->m_len = sizeof(int);
            *mtod(*m, int *) = inp->in6p_tclass;
            break;

        case IPV6_RECVPKTINFO:
            *m = m_get(M_WAIT, MT_SOOPTS);
            if (*m == NULL) { error = ENOBUFS; break; }
            (*m)->m_len = sizeof(int);
            *mtod(*m, int *) = (inp->in6p_flags & IN6P_PKTINFO) ? 1 : 0;
            break;

        case IPV6_MULTICAST_IF:
            *m = m_get(M_WAIT, MT_SOOPTS);
            if (*m == NULL) { error = ENOBUFS; break; }
            (*m)->m_len = sizeof(unsigned int);
            im6o = inp->in6p_moptions;
            *mtod(*m, unsigned int *) =
                (im6o && im6o->im6o_multicast_ifp)
                    ? im6o->im6o_multicast_ifp->if_index : 0;
            break;

        case IPV6_MULTICAST_HOPS:
            *m = m_get(M_WAIT, MT_SOOPTS);
            if (*m == NULL) { error = ENOBUFS; break; }
            (*m)->m_len = sizeof(int);
            im6o = inp->in6p_moptions;
            *mtod(*m, int *) = im6o
                ? (int)im6o->im6o_multicast_hlim
                : IP6_DEFAULT_MULTICAST_HLIM;
            break;

        case IPV6_MULTICAST_LOOP:
            *m = m_get(M_WAIT, MT_SOOPTS);
            if (*m == NULL) { error = ENOBUFS; break; }
            (*m)->m_len = sizeof(unsigned int);
            im6o = inp->in6p_moptions;
            *mtod(*m, unsigned int *) = im6o
                ? (unsigned int)im6o->im6o_multicast_loop
                : IP6_DEFAULT_MULTICAST_LOOP;
            break;

        default:
            error = ENOPROTOOPT;
            break;
        }
        break;

    default:
        error = EINVAL;
        break;
    }

    return error;
}


#endif /* INET6 */

