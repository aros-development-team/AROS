/*
 * Copyright (C) 1982, 1986, 1991, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * IPv6 PCB management, derived from netinet/in_pcb.c
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
#include <sys/queue.h>
#include <sys/synch.h>

#include <net/route.h>
#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet6/in6_var.h>

/* Defined later in this file. */
struct inpcb *in6_pcblookup(struct inpcbhead *, struct in6_addr *, u_int,
                            struct in6_addr *, u_int);

/* netinet/in_pcb.c - stack internal, not part of the public in_pcb.h */
int in_pcbisipv6(struct inpcb *);

extern struct in6_ifaddr *in6_ifawithifp(struct ifnet *, struct in6_addr *);
extern struct ifnet *ifnet;

/* ------------------------------------------------------------------ *
 * Zones (RFC 4007, Readme section 6.6).
 *
 * A link-scope address names a link only together with a zone, which is
 * an interface index: sin6_scope_id in the API, never embedded in the
 * address. A socket's zone is inp_boundif, the interface SO_BINDTODEVICE
 * also sets (as Linux keeps it in sk_bound_dev_if): bind() and connect()
 * set it from a link-scope address, and getsockname()/getpeername()
 * report it.
 * ------------------------------------------------------------------ */
#define IN6_IS_ADDR_ZONED(a) \
    (IN6_IS_ADDR_LINKLOCAL(a) || IN6_IS_ADDR_MC_LINKLOCAL(a) || \
     IN6_IS_ADDR_MC_NODELOCAL(a))

static struct ifnet *
in6_ifbyindex(u_int32_t index)
{
    struct ifnet *ifp;

    for(ifp = ifnet; ifp; ifp = ifp->if_next)
        if((u_int32_t)ifp->if_index == index)
            return ifp;
    return NULL;
}

/*
 * Our address addr, on ifp if that is not NULL, usable as a local address
 * (not tentative or duplicated). NULL if there is none.
 */
static struct in6_ifaddr *
in6_ourifa(const struct in6_addr *addr, struct ifnet *ifp)
{
    struct in6_ifaddr *ia;

    for(ia = in6_ifaddr; ia; ia = ia->ia_next)
        if((ifp == NULL || ia->ia6_ifp == ifp) &&
                IN6_ARE_ADDR_EQUAL(&ia->ia_addr.sin6_addr, addr) &&
                !(ia->ia6_ifaflags & (IN6_IFF_TENTATIVE | IN6_IFF_DUPLICATED)))
            return ia;
    return NULL;
}

/* The zone to report with addr, a local or peer address of inp. */
static u_int32_t
in6_pcbzone(struct inpcb *inp, const struct in6_addr *addr)
{
    struct in6_ifaddr *ia;

    if(!IN6_IS_ADDR_ZONED(addr))
        return 0;
    if(inp->inp_boundif)
        return inp->inp_boundif->if_index;
    /* an accepted connection: the link its link-local address is on */
    if(IN6_IS_ADDR_LINKLOCAL(&inp->inp_laddr6) &&
            (ia = in6_ourifa(&inp->inp_laddr6, NULL)) != NULL)
        return ia->ia6_ifp->if_index;
    return 0;
}

/* The zone a received packet's link-scope address belongs to (0 if none). */
u_int32_t
in6_rcvzone(struct mbuf *m, const struct in6_addr *addr)
{
    if(!IN6_IS_ADDR_ZONED(addr) || m->m_pkthdr.rcvif == NULL)
        return 0;
    return m->m_pkthdr.rcvif->if_index;
}

int in_pcbpickport(struct inpcb *, int (*)(struct inpcb *, u_short, void *),
                   void *, u_short *);

/* Whether lport is in use for the local address arg (in6_pcblookup()). */
static int
in6_porttaken(struct inpcb *inp, u_short lport, void *arg)
{
    struct in6_addr wild;

    bzero(&wild, sizeof(wild));
    return in6_pcblookup(inp->inp_pcbinfo->listhead, &wild, 0,
                         (struct in6_addr *)arg, lport) != NULL;
}

/* ------------------------------------------------------------------ *
 * in6_pcballoclport - pick an unused ephemeral local port for inp on the
 * local address laddr (it may be the unspecified/wildcard address), as
 * in_pcbbind() does for IPv4 (RFC 6056, in_pcbpickport()). The port goes
 * to *lportp in network byte order; EADDRNOTAVAIL if none is free.
 * ------------------------------------------------------------------ */
static int
in6_pcballoclport(struct inpcb *inp, struct in6_addr *laddr, u_short *lportp)
{
    return in_pcbpickport(inp, in6_porttaken, laddr, lportp);
}

/* ------------------------------------------------------------------ *
 * in6_pcbbind - bind an IPv6 PCB to a local address and/or port.
 *
 * If nam is NULL, a wildcard bind with an ephemeral port is performed.
 * Otherwise nam must point to a sockaddr_in6.
 * ------------------------------------------------------------------ */
int
in6_pcbbind(struct inpcb *inp, struct mbuf *nam)
{
    struct ifnet *zone = NULL;

    if(inp->inp_lport || !IN6_IS_ADDR_UNSPECIFIED(&inp->inp_laddr6))
        return EINVAL;

    if(nam == NULL) {
        /* wildcard bind: ephemeral port, wildcard (unspecified) address */
        return in6_pcballoclport(inp, &inp->inp_laddr6, &inp->inp_lport);
    }

    if(nam->m_len < (int)sizeof(struct sockaddr_in6))
        return EINVAL;

    {
        struct sockaddr_in6 *sin6 = mtod(nam, struct sockaddr_in6 *);
        if(sin6->sin6_family != AF_INET6)
            return EAFNOSUPPORT;

        /*
         * The address must be one of ours (or a group, or unspecified). A
         * link-scope one is looked for in its zone: sin6_scope_id, else the
         * interface the socket is bound to, else any interface that has it.
         */
        if(IN6_IS_ADDR_ZONED(&sin6->sin6_addr)) {
            zone = inp->inp_boundif;
            if(sin6->sin6_scope_id != 0) {
                zone = in6_ifbyindex(sin6->sin6_scope_id);
                if(zone == NULL ||
                        (inp->inp_boundif && inp->inp_boundif != zone))
                    return EADDRNOTAVAIL;
            }
        }
        if(!IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr) &&
                !IN6_IS_ADDR_MULTICAST(&sin6->sin6_addr)) {
            struct in6_ifaddr *ia = in6_ourifa(&sin6->sin6_addr, zone);

            if(ia == NULL)
                return EADDRNOTAVAIL;
            if(IN6_IS_ADDR_ZONED(&sin6->sin6_addr))
                zone = ia->ia6_ifp;
        }

        /*
         * For an explicit local port, reject a conflicting bind unless the
         * socket permits address/port reuse.  Done before mutating inp so an
         * error leaves the PCB unchanged (mirrors in_pcbbind()).
         */
        if(sin6->sin6_port != 0) {
            struct socket *so = inp->inp_socket;
            if((so->so_options & (SO_REUSEADDR | SO_REUSEPORT)) == 0) {
                struct inpcbhead *head = inp->inp_pcbinfo->listhead;
                struct in6_addr wildf;

                bzero(&wildf, sizeof(wildf));
                if(in6_pcblookup(head, &wildf, 0,
                                 &sin6->sin6_addr, sin6->sin6_port) != NULL)
                    return EADDRINUSE;
            }
        }

        if(sin6->sin6_port == 0) {
            /* explicit address, but port 0 means "pick an ephemeral one";
             * picked before inp changes, so an error leaves it unbound */
            u_short lport;
            int error = in6_pcballoclport(inp, &sin6->sin6_addr, &lport);

            if(error)
                return error;
            inp->inp_lport = lport;
        } else
            inp->inp_lport = sin6->sin6_port; /* already in network byte order */
        inp->inp_laddr6 = sin6->sin6_addr;
        if(zone != NULL)
            inp->inp_boundif = zone;
    }

    return 0;
}

/* ------------------------------------------------------------------ *
 * in6_pcbconnect - connect an IPv6 PCB to a remote address and port.
 * ------------------------------------------------------------------ */
int
in6_pcbconnect(struct inpcb *inp, struct mbuf *nam)
{
    struct sockaddr_in6 *sin6;
    struct ifnet *zone = NULL;

    if(nam == NULL || nam->m_len < (int)sizeof(struct sockaddr_in6))
        return EINVAL;

    sin6 = mtod(nam, struct sockaddr_in6 *);
    if(sin6->sin6_family != AF_INET6)
        return EAFNOSUPPORT;

    if(IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr))
        return EADDRNOTAVAIL;

    /*
     * A link-scope peer is on the link its zone names: sin6_scope_id, else
     * the socket's. With neither, the routing table picks the link, as
     * before (the default zone, RFC 4007 section 6).
     */
    if(IN6_IS_ADDR_ZONED(&sin6->sin6_addr)) {
        zone = inp->inp_boundif;
        if(sin6->sin6_scope_id != 0) {
            zone = in6_ifbyindex(sin6->sin6_scope_id);
            if(zone == NULL)
                return ENXIO;
            if(inp->inp_boundif && inp->inp_boundif != zone)
                return EINVAL;
        }
    }

    /* Bind a local port if not already bound */
    if(inp->inp_lport == 0) {
        int error = in6_pcbbind(inp, NULL);
        if(error)
            return error;
    }

    /*
     * Select a source address if the socket is not bound to a specific local
     * address.  Without this the connection would use the unspecified source
     * (::), which the peer's ip6_input() discards as an invalid scope - so the
     * SYN (and every later segment) would never be accepted.  Mirrors the
     * source selection udp6_output() performs for unbound datagram sockets.
     */
    if(IN6_IS_ADDR_UNSPECIFIED(&inp->inp_laddr6) && zone != NULL) {
        struct in6_ifaddr *ia = in6_ifawithifp(zone, &sin6->sin6_addr);

        if(ia == NULL)
            return EADDRNOTAVAIL;
        inp->inp_laddr6 = ia->ia_addr.sin6_addr;
    } else if(IN6_IS_ADDR_UNSPECIFIED(&inp->inp_laddr6)) {
        struct route_in6 ro;
        struct in6_ifaddr *ia;

        bzero((caddr_t)&ro, sizeof(ro));
        ro.ro_dst.sin6_family = AF_INET6;
        ro.ro_dst.sin6_len    = sizeof(ro.ro_dst);
        ro.ro_dst.sin6_addr   = sin6->sin6_addr;
        rtalloc((struct route *)&ro);
        if(ro.ro_rt != NULL) {
            ia = in6_ifawithifp(ro.ro_rt->rt_ifp, &sin6->sin6_addr);
            if(ia)
                inp->inp_laddr6 = ia->ia_addr.sin6_addr;
            RTFREE(ro.ro_rt);
        }
        if(IN6_IS_ADDR_UNSPECIFIED(&inp->inp_laddr6))
            return EADDRNOTAVAIL;
    }

    inp->inp_faddr6 = sin6->sin6_addr;
    inp->inp_fport  = sin6->sin6_port;
    if(zone != NULL)
        inp->inp_boundif = zone;

    return 0;
}

/* ------------------------------------------------------------------ *
 * in6_pcbdisconnect - disconnect an IPv6 PCB (clear foreign address).
 * ------------------------------------------------------------------ */
void
in6_pcbdisconnect(struct inpcb *inp)
{
    bzero(&inp->inp_faddr6, sizeof(inp->inp_faddr6));
    inp->inp_fport = 0;
    if(inp->inp_socket->so_state & SS_NOFDREF)
        in_pcbdetach(inp);
}

/* ------------------------------------------------------------------ *
 * in6_setsockaddr - fill nam with the local IPv6 address/port.
 * ------------------------------------------------------------------ */
void
in6_setsockaddr(struct inpcb *inp, struct mbuf *nam)
{
    struct sockaddr_in6 *sin6;

    nam->m_len = sizeof(struct sockaddr_in6);
    sin6 = mtod(nam, struct sockaddr_in6 *);
    bzero(sin6, sizeof(*sin6));
    sin6->sin6_family = AF_INET6;
    sin6->sin6_len    = sizeof(*sin6);
    sin6->sin6_port   = inp->inp_lport;
    sin6->sin6_addr   = inp->inp_laddr6;
    sin6->sin6_scope_id = in6_pcbzone(inp, &inp->inp_laddr6);
}

/* ------------------------------------------------------------------ *
 * in6_setpeeraddr - fill nam with the foreign IPv6 address/port.
 * ------------------------------------------------------------------ */
void
in6_setpeeraddr(struct inpcb *inp, struct mbuf *nam)
{
    struct sockaddr_in6 *sin6;

    nam->m_len = sizeof(struct sockaddr_in6);
    sin6 = mtod(nam, struct sockaddr_in6 *);
    bzero(sin6, sizeof(*sin6));
    sin6->sin6_family = AF_INET6;
    sin6->sin6_len    = sizeof(*sin6);
    sin6->sin6_port   = inp->inp_fport;
    sin6->sin6_addr   = inp->inp_faddr6;
    sin6->sin6_scope_id = in6_pcbzone(inp, &inp->inp_faddr6);
}

/* ------------------------------------------------------------------ *
 * in6_pcblookup - find a matching IPv6 PCB.
 *
 * Searches the given PCB list for an entry matching the four-tuple
 * (foreign addr, foreign port, local addr, local port).  Wildcard
 * entries (all-zeros local or foreign address) are lower priority.
 * ------------------------------------------------------------------ */
struct inpcb *
in6_pcblookup(struct inpcbhead *head,
              struct in6_addr *faddr, u_int fport_arg,
              struct in6_addr *laddr, u_int lport_arg)
{
    struct inpcb *inp, *match = NULL;
    int matchwild = 3;
    u_short fport = fport_arg, lport = lport_arg;

    LIST_FOREACH(inp, head, inp_list) {
        if(inp->inp_lport != lport)
            continue;
        /* IPv4 PCBs share this list and leave the IPv6 addresses zeroed,
           which would make them look like wildcard IPv6 sockets here */
        if(!in_pcbisipv6(inp))
            continue;

        int wildcard = 0;

        /* Check local address */
        if(!IN6_IS_ADDR_UNSPECIFIED(&inp->inp_laddr6)) {
            if(!IN6_ARE_ADDR_EQUAL(&inp->inp_laddr6, laddr))
                continue;
        } else {
            wildcard++;
        }

        /* Check foreign address */
        if(!IN6_IS_ADDR_UNSPECIFIED(&inp->inp_faddr6)) {
            if(inp->inp_fport != fport ||
                    !IN6_ARE_ADDR_EQUAL(&inp->inp_faddr6, faddr))
                continue;
        } else {
            if(fport != 0)
                wildcard++;
        }

        if(wildcard < matchwild) {
            match = inp;
            matchwild = wildcard;
            if(matchwild == 0)
                break;
        }
    }

    return match;
}

#endif /* INET6 */
