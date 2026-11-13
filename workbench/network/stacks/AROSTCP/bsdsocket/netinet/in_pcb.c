/*
 * Copyright (C) 1982, 1986, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (C) 2006 Pavel Fedin
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
 *	@(#)in_pcb.c	8.2 (Berkeley) 1/4/94
 * $Id$
 */

#include <conf.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <sys/synch.h>

#include <net/route.h>
#include <net/if.h>
#include <net/if_protos.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/inetfamily.h>
#include <netinet/inet_transport.h>
#include <protos/kern/kern_random_protos.h>
#include <protos/net/route_protos.h>
#include <protos/net/rtsock_protos.h>
#include <protos/kern/uipc_socket_protos.h>
#include <net/netcore_map.h>        /* always the last include */

/*
 * Ephemeral ports (RFC 6056): a random one from the IANA dynamic range
 * (RFC 6335 section 6), else the next free one after it - algorithm 1,
 * section 3.3.1 - so that an off-path attacker cannot guess the port of
 * the next connection or query. The range keeps clear of the registered
 * ports services listen on.
 */
#define EPHEMERAL_FIRST	49152
#define EPHEMERAL_LAST	65535

/* ------------------------------------------------------------------ *
 * Addresses, for every family: inf_addrlen bytes, all zero meaning "any";
 * compared and tested with inpcb_addreq() and inpcb_addrany()
 * (netinet/in_pcb.h).
 * ------------------------------------------------------------------ */

/* Store an address in a PCB; NULL is "any". (src may be the place itself.) */
static __inline void
inpcb_addrset(union inp_addr *dst, const void *src, u_int len)
{
    if(src == NULL)
        bzero((caddr_t)dst, sizeof(*dst));
    else if(src != (const void *)dst) {
        const inp_word_t *w = (const inp_word_t *)src;
        inp_word_t *d = (inp_word_t *)dst;

        for(; len != 0; len -= sizeof(inp_word_t))
            *d++ = *w++;
    }
}

/*
 * The families the transports can serve (netinet/inetfamily.h).
 */
static const struct inetfamily *inetfamilies[INETFAMILY_MAX];

int
inetfamily_register(const struct inetfamily *inf)
{
    int i, slot = -1;

    if(inf == NULL || inf->inf_size < sizeof(struct inetfamily))
        return (EINVAL);
    /*
     * What the PCB layer and the transports call without a test. A family
     * that lacks any of it is refused here - its sockets then cannot be
     * made - instead of failing at the first call through a missing
     * operation. (What may be NULL is said in netinet/inetfamily.h.)
     */
    if(inf->inf_addrlen <= 0 || inf->inf_addrlen > INP_ADDRMAX ||
            (inf->inf_addrlen & (sizeof(inp_word_t) - 1)) != 0 ||
            inf->inf_salen <= 0 ||
            inf->inf_salen > (int)sizeof(struct sockaddr_storage) ||
            inf->inf_hdrlen <= 0 || inf->inf_mssdflt == NULL ||
            inf->inf_ctlerrmap == NULL)
        return (EINVAL);
    /* every socket */
    if(inf->inf_ctloutput == NULL || inf->inf_sa_multicast == NULL ||
            inf->inf_peer_local == NULL || inf->inf_pcbfree == NULL ||
            inf->inf_pcbinit == NULL || inf->inf_pcbanyfamily == NULL ||
            inf->inf_sockaddr_parse == NULL || inf->inf_mksockaddr == NULL ||
            inf->inf_addr_from == NULL || inf->inf_pcbzone == NULL ||
            inf->inf_ownaddr == NULL || inf->inf_selectsrc == NULL ||
            inf->inf_pcbbindaddr == NULL || inf->inf_pcbladdr == NULL)
        return (EINVAL);
    /* per packet, and connections */
    if(inf->inf_rcvinfo == NULL || inf->inf_cksum == NULL ||
            inf->inf_output == NULL || inf->inf_addr_multicast == NULL ||
            inf->inf_acceptok == NULL || inf->inf_mtu == NULL ||
            inf->inf_pcbroute == NULL)
        return (EINVAL);
    /* datagrams; ancillary data if any socket option asks for it */
    if(inf->inf_rcvfrom == NULL || inf->inf_unreach == NULL ||
            inf->inf_dgoutput == NULL ||
            (inf->inf_rcvopt_mask != 0 && inf->inf_savecontrol == NULL))
        return (EINVAL);
    for(i = 0; i < INETFAMILY_MAX; i++) {
        if(inetfamilies[i] == NULL) {
            if(slot < 0)
                slot = i;
        } else if(inetfamilies[i]->inf_family == inf->inf_family)
            return (EEXIST);
    }
    if(slot < 0)
        return (ENOSPC);
    inetfamilies[slot] = inf;
    return (0);
}

int
inetfamily_unregister(const struct inetfamily *inf)
{
    int i;

    for(i = 0; i < INETFAMILY_MAX; i++)
        if(inetfamilies[i] == inf) {
            inetfamilies[i] = NULL;
            return (0);
        }
    return (ENOENT);
}

/* The first registered family, or NULL: is any left? */
const struct inetfamily *
inetfamily_any(void)
{
    int i;

    for(i = 0; i < INETFAMILY_MAX; i++)
        if(inetfamilies[i] != NULL)
            return (inetfamilies[i]);
    return (NULL);
}

/* The transports stop: the table is as the file has it (it is empty by then). */
void
inetfamily_forget(void)
{
    int i;

    for(i = 0; i < INETFAMILY_MAX; i++)
        inetfamilies[i] = NULL;
}

/*
 * How many PCBs of a list still use family inf: are of it, or carry
 * something of its own (holds(), the family's to say; may be NULL). For a
 * family that is asked to stop (ti_family_pcbs, netinet/inet_transport.h).
 */
long
in_pcbcount(const struct inpcbhead *head, const struct inetfamily *inf,
            int (*holds)(const struct inpcb *inp))
{
    const struct inpcb *inp;
    long n = 0;

    for(inp = head->lh_first; inp != NULL; inp = inp->inp_list.le_next)
        if(inp->inp_family == inf || (holds != NULL && (*holds)(inp)))
            n++;
    return (n);
}

const struct inetfamily *
inetfamily_find(int family)
{
    int i;

    for(i = 0; i < INETFAMILY_MAX; i++)
        if(inetfamilies[i] != NULL && inetfamilies[i]->inf_family == family)
            return (inetfamilies[i]);
    return (NULL);
}

const struct inetfamily *
inetfamily_bylevel(int optlevel)
{
    int i;

    for(i = 0; i < INETFAMILY_MAX; i++)
        if(inetfamilies[i] != NULL && inetfamilies[i]->inf_optlevel == optlevel)
            return (inetfamilies[i]);
    return (NULL);
}

/*
 * Make a PCB one of another family than its socket's: the IPv4 connection
 * accepted on an IPv6 socket that listens for both. Its addresses start
 * out as "any"; the socket's own family presents them to the application
 * (in IPv4-mapped form). Refused when that family is not part of the
 * running stack.
 */
int
in_pcbsetfamily(struct inpcb *inp, const struct inetfamily *inf)
{
    int s;

    if(inf == NULL)
        return (EAFNOSUPPORT);
    s = splnet();
    inp->inp_family = inf;
    inp->inp_af = inf->inf_family;
    bzero((caddr_t)&inp->inp_la, sizeof(inp->inp_la));
    bzero((caddr_t)&inp->inp_fa, sizeof(inp->inp_fa));
    /* what a PCB of that family starts with (IPv4: its TTL) */
    (*inf->inf_pcbinit)(inp);
    splx(s);
    return (0);
}

/*
 * May a look-up on behalf of family inf consider this PCB? One of the
 * family, or one that stands for every family (inpcb_anyfamily(),
 * netinet/in_pcb.h).
 */
#define INPCB_VISIBLE(inp, inf) \
    ((inp)->inp_family == (inf) || inpcb_anyfamily(inp))

int
in_pcballoc(so, pcbinfo)
struct socket *so;
struct inpcbinfo *pcbinfo;
{
    register struct inpcb *inp;
    int s;

    const struct inetfamily *inf = NULL;

    /* a PCB belongs to the family of its socket, which must be there */
    if(so != NULL && so->so_proto != NULL && so->so_proto->pr_domain != NULL)
        inf = inetfamily_find(so->so_proto->pr_domain->dom_family);
    if(inf == NULL)
        return (EAFNOSUPPORT);
    MALLOC(inp, struct inpcb *, sizeof(*inp), M_PCB, M_NOWAIT);
    if(inp == NULL)
        return (ENOBUFS);
    bzero((caddr_t)inp, sizeof(*inp));
    inp->inp_pcbinfo = pcbinfo;
    inp->inp_socket = so;
    inp->inp_family = inf;
    inp->inp_af = inf->inf_family;
    /* the family's defaults (IPv6: restricted to its own family until
     * IPV6_V6ONLY is cleared) */
    (*inf->inf_pcbinit)(inp);
    s = splnet();
    LIST_INSERT_HEAD(pcbinfo->listhead, inp, inp_list);
    inpcb_inshash(inp);
    splx(s);
    so->so_pcb = (caddr_t)inp;
    return (0);
}

/* ------------------------------------------------------------------ *
 * The hash: over the bytes of the foreign address and both ports, for
 * PCBs of every family. (For an IPv4 address this is the sum it always
 * was.) A PCB is filed under its own family's address length; one that
 * stands for every family has no address, so it hashes alike for all.
 * ------------------------------------------------------------------ */

/* Expanded in the look-up, which every packet goes through. */
INP_ALWAYS_INLINE struct inpcbhead *
inpcb_hashhead(struct inpcbinfo *pcbinfo, const void *faddr, u_int alen,
               u_int fport, u_int lport)
{
    u_int32_t h = (u_short)fport + (u_short)lport;
    const inp_word_t *w = (const inp_word_t *)faddr;

    for(; alen != 0; alen -= sizeof(inp_word_t))
        h += *w++;
    return (&pcbinfo->hashbase[h % pcbinfo->hashsize]);
}

struct inpcbhead *
inpcb_hash(struct inpcbinfo *pcbinfo, const struct inetfamily *inf,
           const void *faddr, u_int fport, u_int lport)
{
    return (inpcb_hashhead(pcbinfo, faddr, faddr != NULL ? inf->inf_addrlen : 0,
                           fport, lport));
}

/*
 * Insert PCB into hash chain. Must be called at splnet.
 */
void
inpcb_inshash(struct inpcb *inp)
{
    struct inpcbhead *head;

    head = inpcb_hashhead(inp->inp_pcbinfo, &inp->inp_fa,
                          inp->inp_family->inf_addrlen,
                          inp->inp_fport, inp->inp_lport);
    LIST_INSERT_HEAD(head, inp, inp_hash);
}

/* The PCB's foreign address or one of its ports has changed. */
void
inpcb_rehash(struct inpcb *inp)
{
    int s;

    s = splnet();
    LIST_REMOVE(inp, inp_hash);
    inpcb_inshash(inp);
    splx(s);
}

/* ------------------------------------------------------------------ *
 * Look-up.
 * ------------------------------------------------------------------ */

/*
 * Does the interface a packet came in by rule this PCB out? A socket bound
 * to an interface (SO_BINDTODEVICE, or a zone) only hears that interface,
 * and one whose interface went away hears nothing. rcvif NULL: not asked.
 */
#define INPCB_DEAF(inp, rcvif) \
    ((rcvif) != NULL && \
     (((inp)->inp_boundif != NULL && (inp)->inp_boundif != (rcvif)) || \
      ((inp)->inp_flags & INP_BOUNDGONE)))

/*
 * The best PCB for these addresses and ports by a walk of the list: each
 * address that is "any" on one side only counts as one wildcard, the PCB
 * with the fewest wins, and without INPLOOKUP_WILDCARD only one with none
 * will do. A PCB of another family is considered when it stands for every
 * family (inpcb_anyfamily()). Must be called at splnet.
 */
static struct inpcb *
inpcb_walklist(struct inpcbhead *head, const struct inetfamily *inf,
               const void *faddr, u_int fport_arg,
               const void *laddr, u_int lport_arg,
               const struct ifnet *rcvif, int flags)
{
    register struct inpcb *inp, *match = NULL;
    int matchwild = 3, wildcard;
    u_short fport = fport_arg, lport = lport_arg;
    u_int alen = inf->inf_addrlen;
    int fany, lany;

    fany = (faddr == NULL || inpcb_addrany(faddr, alen));
    lany = (laddr == NULL || inpcb_addrany(laddr, alen));

    for(inp = head->lh_first; inp != NULL; inp = inp->inp_list.le_next) {
        if(inp->inp_lport != lport)
            continue;
        if(!INPCB_VISIBLE(inp, inf) || INPCB_DEAF(inp, rcvif))
            continue;
        wildcard = 0;
        if(!inpcb_addrany(&inp->inp_fa, alen)) {
            if(fany)
                wildcard++;
            else if(inp->inp_fport != fport ||
                    !inpcb_addreq(&inp->inp_fa, faddr, alen))
                continue;
        } else {
            if(!fany)
                wildcard++;
        }
        if(!inpcb_addrany(&inp->inp_la, alen)) {
            if(lany)
                wildcard++;
            else if(!inpcb_addreq(&inp->inp_la, laddr, alen))
                continue;
        } else {
            if(!lany)
                wildcard++;
        }
        if(wildcard && (flags & INPLOOKUP_WILDCARD) == 0)
            continue;
        if(wildcard < matchwild) {
            match = inp;
            matchwild = wildcard;
            if(matchwild == 0) {
                break;
            }
        }
    }
    return (match);
}

/* The same for those that are not at splnet: bind and connect. */
static struct inpcb *
inpcb_lookuplist(struct inpcbhead *head, const struct inetfamily *inf,
                 const void *faddr, u_int fport,
                 const void *laddr, u_int lport,
                 const struct ifnet *rcvif, int flags)
{
    struct inpcb *inp;
    int s;

    s = splnet();
    inp = inpcb_walklist(head, inf, faddr, fport, laddr, lport, rcvif, flags);
    splx(s);
    return (inp);
}

/*
 * inpcb_lookup - the PCB a packet of family inf belongs to, or that holds
 * the given addresses and ports: the one with exactly these four through
 * the hash, else the best one of the list (inpcb_walklist(); flags:
 * INPLOOKUP_WILDCARD) - or, with INPLOOKUP_HASHED, the exact one through
 * the hash or none.
 * rcvif: the interface the packet came in by - PCBs bound to another one
 * are passed over - or NULL when that is not to be looked at.
 *
 * Every segment and datagram that arrives comes through here, so the way
 * to a connection's PCB is kept short: the hash and the comparisons are
 * expanded in place - for a 4-byte address, one addition and two
 * comparisons - and the interrupt level is raised once for the whole
 * look-up, or not at all for a caller that says it is at splnet already
 * (INPLOOKUP_ATSPLNET: the input of a packet, which sana_poll() runs at
 * splnet as a whole). Each change of the level is two calls into exec.
 */
struct inpcb *
inpcb_lookup(struct inpcbinfo *pcbinfo, const struct inetfamily *inf,
             const void *faddr, u_int fport_arg, const void *laddr,
             u_int lport_arg, const struct ifnet *rcvif, int flags)
{
    static const union inp_addr any;
    struct inpcbhead *head;
    register struct inpcb *inp;
    u_short fport = fport_arg, lport = lport_arg;
    u_int alen = inf->inf_addrlen;
    int s;

    if(faddr == NULL)
        faddr = &any;
    if(laddr == NULL)
        laddr = &any;
    s = (flags & INPLOOKUP_ATSPLNET) ? 0 : splnet();
    head = inpcb_hashhead(pcbinfo, faddr, alen, fport, lport);
    for(inp = head->lh_first; inp != NULL; inp = inp->inp_hash.le_next) {
        if(inp->inp_fport != fport || inp->inp_lport != lport ||
                !inpcb_addreq(&inp->inp_fa, faddr, alen) ||
                !inpcb_addreq(&inp->inp_la, laddr, alen))
            continue;
        if(!INPCB_VISIBLE(inp, inf) || INPCB_DEAF(inp, rcvif))
            continue;
        /*
         * Move PCB to head of this hash chain so that it can be
         * found more quickly in the future.
         */
        if(inp != head->lh_first) {
            LIST_REMOVE(inp, inp_hash);
            LIST_INSERT_HEAD(head, inp, inp_hash);
        }
        break;
    }
    if(inp == NULL && !(flags & INPLOOKUP_HASHED)) {
        inp = inpcb_walklist(pcbinfo->listhead, inf, faddr, fport,
                             laddr, lport, rcvif, flags);
        /*
         * A PCB with both its addresses that the hash did not give: they
         * were set without telling the hash (code that writes the fields
         * and does not call inpcb_rehash()). File it where it belongs, at
         * the head, so that the next packet finds it at once. (One whose
         * local address is "any" is never an exact match, wherever it is
         * filed: it is left alone, and found by the list every time.)
         */
        if(inp != NULL && inp->inp_family == inf &&
                !inpcb_addrany(&inp->inp_fa, alen) &&
                !inpcb_addrany(&inp->inp_la, alen)) {
            LIST_REMOVE(inp, inp_hash);
            inpcb_inshash(inp);
        }
    }
    if(!(flags & INPLOOKUP_ATSPLNET))
        splx(s);
    return (inp);
}

/* ------------------------------------------------------------------ *
 * Local and foreign address of a PCB.
 * ------------------------------------------------------------------ */

/*
 * inpcb_setconn - give a PCB its addresses and ports, all four at once,
 * and file it under them. An address that is NULL is "any" (it may also
 * be the PCB's own field: unchanged). zone: the interface the PCB is to
 * be bound to with them, or NULL to leave that as it is.
 */
void
inpcb_setconn(struct inpcb *inp, const void *laddr, u_int lport,
              const void *faddr, u_int fport, struct ifnet *zone)
{
    u_int alen = inp->inp_family->inf_addrlen;
    int s;

    s = splnet();
    inpcb_addrset(&inp->inp_la, laddr, alen);
    inp->inp_lport = lport;
    inpcb_addrset(&inp->inp_fa, faddr, alen);
    inp->inp_fport = fport;
    if(zone != NULL)
        inp->inp_boundif = zone;
    inpcb_rehash(inp);
    splx(s);
}

/*
 * Pick an ephemeral port for inp on the local address laddr (NULL: any).
 * A port is taken when any PCB the family can see has it with a local
 * address that could be the same one (either is "any", or they are equal),
 * connected or not, whatever its reuse options: connections - those in
 * TIME_WAIT too - must be seen, or a new connection to the same peer could
 * be given a port one of them still holds. The port is stored in network
 * byte order in *lportp. Returns EADDRNOTAVAIL if every port in the range
 * is taken.
 */
static int
inpcb_pickport(struct inpcb *inp, const void *laddr, u_short *lportp)
{
    u_int32_t num = EPHEMERAL_LAST - EPHEMERAL_FIRST + 1, offset, count;

    offset = kern_random32() % num;
    for(count = num; count > 0; count--) {
        u_short lport = htons((u_short)(EPHEMERAL_FIRST + offset));

        if(inpcb_lookuplist(inp->inp_pcbinfo->listhead, inp->inp_family,
                            NULL, 0, laddr, lport, NULL,
                            INPLOOKUP_WILDCARD) == NULL) {
            *lportp = lport;
            return (0);
        }
        offset = (offset + 1) % num;
    }
    return (EADDRNOTAVAIL);
}

/*
 * inpcb_bind - bind a PCB to the local address and port in nam, or with
 * nam NULL to an ephemeral port on "any". The family says whether the
 * address will do (inf_pcbbindaddr); who may share an address and port is
 * decided here, for every family alike:
 * a port given explicitly is refused (EADDRINUSE) when another PCB has it
 * on the same address - or, if this socket set neither SO_REUSEADDR nor
 * SO_REUSEPORT, on an address that could be the same one - unless both
 * sockets set SO_REUSEPORT; for a multicast address SO_REUSEADDR counts
 * as SO_REUSEPORT.
 */
int
inpcb_bind(struct inpcb *inp, struct mbuf *nam)
{
    const struct inetfamily *inf = inp->inp_family;
    register struct socket *so = inp->inp_socket;
    const void *laddr = NULL;           /* NULL: any */
    struct ifnet *zone = NULL;
    u_short lport = 0;
    int wild = 0, reuseport = (so->so_options & SO_REUSEPORT);
    int error;

    if(inp->inp_lport || !inpcb_addrany(&inp->inp_la, inf->inf_addrlen))
        return (EINVAL);
    if((so->so_options & (SO_REUSEADDR | SO_REUSEPORT)) == 0 &&
            ((so->so_proto->pr_flags & PR_CONNREQUIRED) == 0 ||
             (so->so_options & SO_ACCEPTCONN) == 0))
        wild = INPLOOKUP_WILDCARD;
    error = (*inf->inf_pcbbindaddr)(inp, nam, &laddr, &lport, &zone);
    if(error)
        return (error);
    if(nam != NULL && laddr != NULL) {
        if((*inf->inf_addr_multicast)(laddr)) {
            /*
             * Treat SO_REUSEADDR as SO_REUSEPORT for multicast;
             * allow complete duplication of binding if
             * SO_REUSEPORT is set, or if SO_REUSEADDR is set
             * and a multicast address is bound on both
             * new and duplicated sockets.
             */
            if(so->so_options & SO_REUSEADDR)
                reuseport = SO_REUSEADDR | SO_REUSEPORT;
        }
        if(lport) {
            struct inpcb *t;

            t = inpcb_lookuplist(inp->inp_pcbinfo->listhead, inf, NULL, 0,
                                 laddr, lport, NULL, wild);
            if(t && (reuseport & t->inp_socket->so_options) == 0)
                return (EADDRINUSE);
        }
    }
    /* the port is picked before the PCB changes: an error leaves it unbound */
    if(lport == 0 && (error = inpcb_pickport(inp, laddr, &lport)) != 0)
        return (error);
    inpcb_setconn(inp, laddr, lport, &inp->inp_fa, inp->inp_fport, zone);
    return (0);
}

/*
 * inpcb_connect - connect a PCB to the address and port in nam. The family
 * checks the address and finds the local address to use if the PCB has
 * none (inf_pcbladdr); a PCB without a port gets an ephemeral one. Refused
 * with EADDRINUSE when another PCB already has the same four addresses
 * and ports - unless the caller's `inuse' (NULL: none), asked about that
 * PCB, returns not 0: it has then disposed of it (TCP: an earlier
 * incarnation of the connection whose TIME_WAIT may be cut short) and the
 * connect goes on.
 */
int
inpcb_connect(struct inpcb *inp, struct mbuf *nam,
              int (*inuse) __P((struct inpcb *)))
{
    const struct inetfamily *inf = inp->inp_family;
    union inp_addr laddr;
    const void *faddr;
    struct ifnet *zone = NULL, *named;
    struct inpcb *t;
    u_short fport, lport;
    int error;

    if(nam == NULL)
        return (EINVAL);
    bzero((caddr_t)&laddr, sizeof(laddr));
    /*
     *   Call inner routine, to assign local interface address.
     */
    if((error = (*inf->inf_pcbladdr)(inp, nam, &laddr, &zone)) != 0)
        return (error);
    /* where the (possibly rewritten) address and the port are in nam */
    if((error = (*inf->inf_sockaddr_parse)(nam, &faddr, &fport, &named)) != 0)
        return (error);

    t = inpcb_lookup(inp->inp_pcbinfo, inf, faddr, fport,
                     &laddr, inp->inp_lport, NULL, INPLOOKUP_HASHED);
    if(t != NULL && t != inp && (inuse == NULL || !(*inuse)(t)))
        return (EADDRINUSE);
    lport = inp->inp_lport;
    if(lport == 0) {
        if(inpcb_addrany(&inp->inp_la, inf->inf_addrlen)) {
            if((error = inpcb_bind(inp, (struct mbuf *)0)) != 0)
                return (error);
            lport = inp->inp_lport;
        } else if((error = inpcb_pickport(inp, &inp->inp_la, &lport)) != 0)
            return (error);
    }
    inpcb_setconn(inp, &laddr, lport, faddr, fport, zone);
    return (0);
}

void
inpcb_disconnect(struct inpcb *inp)
{
    inpcb_setconn(inp, &inp->inp_la, inp->inp_lport, NULL, 0, NULL);
    if(inp->inp_socket->so_state & SS_NOFDREF)
        in_pcbdetach(inp);
}

/*
 * Write one of the PCB's addresses into nam as a socket address of the
 * family of its socket. That is the PCB's own family, except for the
 * connection of one family that a socket of another accepted (the IPv4
 * connection of an IPv6 socket): there the socket's family says how
 * it presents the address.
 */
static void
inpcb_mkname(struct inpcb *inp, struct mbuf *nam, const union inp_addr *addr,
             u_int port)
{
    const struct inetfamily *inf = inp->inp_family, *sof = inf;
    struct socket *so = inp->inp_socket;
    union inp_addr other;
    const struct ifnet *zone = NULL;

    if(so != NULL && so->so_proto != NULL && so->so_proto->pr_domain != NULL &&
            so->so_proto->pr_domain->dom_family != inf->inf_family) {
        sof = inetfamily_find(so->so_proto->pr_domain->dom_family);
        if(sof == NULL)
            sof = inf;
    }
    if(sof != inf) {
        bzero((caddr_t)&other, sizeof(other));
        if((*sof->inf_addr_from)(&other, inf, addr) != 0)
            bzero((caddr_t)&other, sizeof(other));
        addr = &other;
    } else
        zone = (*inf->inf_pcbzone)(inp);
    nam->m_len = sof->inf_salen;
    (*sof->inf_mksockaddr)(mtod(nam, struct sockaddr *), addr, port, zone);
}

void
inpcb_setsockaddr(struct inpcb *inp, struct mbuf *nam)
{
    inpcb_mkname(inp, nam, &inp->inp_la, inp->inp_lport);
}

void
inpcb_setpeeraddr(struct inpcb *inp, struct mbuf *nam)
{
    inpcb_mkname(inp, nam, &inp->inp_fa, inp->inp_fport);
}

/*
 * in_pcbifgone - interface ifp is going away (if_detach()), and with it
 * the struct ifnet the PCBs of head may name: no socket may go on doing
 * so. A socket bound to it (SO_BINDTODEVICE, or an IPv6 zone) stays bound
 * to it gone, INP_BOUNDGONE: what it sends fails with ENXIO and it hears
 * nothing, as a Linux socket bound to a device that went away does, until
 * it is bound again. A multicast interface chosen for it is forgotten: it
 * was a preference, and the default applies again. That is in what the
 * families hung on the PCB, and every family is asked: a socket's options
 * are found by their level, so a PCB may carry those of a family that is
 * not its own.
 */
void
in_pcbifgone(const struct inpcbhead *head, const struct ifnet *ifp)
{
    struct inpcb *inp;
    int i;

    for(inp = head->lh_first; inp != NULL; inp = inp->inp_list.le_next) {
        if(inp->inp_boundif == ifp) {
            inp->inp_boundif = NULL;
            inp->inp_flags |= INP_BOUNDGONE;
        }
        for(i = 0; i < INETFAMILY_MAX; i++)
            if(inetfamilies[i] != NULL && inetfamilies[i]->inf_pcbifgone != NULL)
                (*inetfamilies[i]->inf_pcbifgone)(inp, ifp);
    }
}

void
in_pcbdetach(inp)
struct inpcb *inp;
{
    struct socket *so = inp->inp_socket;
    int s;

    so->so_pcb = 0;
    sofree(so);
    if(inp->inp_options)
        (void)m_free(inp->inp_options);
    if(inp->inp_route.ro_rt)
        rtfree(inp->inp_route.ro_rt);
    /*
     * What the families hung on it (their multicast options): its own
     * family's, and - a socket's options are found by their level, so a
     * PCB may carry those of a family that is not its own - every other
     * registered family's. (Only the PCB's own family used to be asked:
     * the group memberships an AF_INET6 socket had taken with IPv4's
     * options were never left. A family that is asked to stop refuses
     * while a PCB carries something of it - ti_family_pcbs - so the one
     * that hung something here is still registered.)
     */
    (*inp->inp_family->inf_pcbfree)(inp);
    {
        int i;

        for(i = 0; i < INETFAMILY_MAX; i++)
            if(inetfamilies[i] != NULL && inetfamilies[i] != inp->inp_family)
                (*inetfamilies[i]->inf_pcbfree)(inp);
    }
    s = splnet();
    LIST_REMOVE(inp, inp_hash);
    LIST_REMOVE(inp, inp_list);
    splx(s);
    FREE(inp, M_PCB);
}

/*
 * inpcb_notify - pass some notification to all connections of a protocol
 * associated with the address faddr of family inf. The local address
 * and/or port numbers may be specified to limit the search (laddr NULL or
 * "any", ports 0: not specified). The "usual action" will be taken,
 * depending on the ctlinput cmd. The caller must filter any cmds that are
 * uninteresting (e.g., no error in the family's map: notify is then called
 * with 0). Call the protocol specific routine (if any) to report any
 * errors for each matching socket.
 */
void
inpcb_notify(struct inpcbhead *head, const struct inetfamily *inf,
             const void *faddr, u_int fport_arg,
             const void *laddr, u_int lport_arg, int cmd,
             void (*notify) __P((struct inpcb *, int)))
{
    register struct inpcb *inp, *oinp;
    u_short fport = fport_arg, lport = lport_arg;
    u_int alen;
    int _errno, s;

    if((unsigned)cmd >= PRC_NCMDS || inf == NULL || faddr == NULL)
        return;
    alen = inf->inf_addrlen;
    if(inpcb_addrany(faddr, alen))
        return;
    if(laddr != NULL && inpcb_addrany(laddr, alen))
        laddr = NULL;

    /*
     * Redirects go to all references to the destination,
     * and use in_rtchange to invalidate the route cache.
     * Dead host indications: notify all references to the destination.
     * Otherwise, if we have knowledge of the local port and address,
     * deliver only to that socket.
     */
    if(PRC_IS_REDIRECT(cmd) || cmd == PRC_HOSTDEAD) {
        fport = 0;
        lport = 0;
        laddr = NULL;
        if(cmd != PRC_HOSTDEAD)
            notify = in_rtchange;
    }
    _errno = inf->inf_ctlerrmap[cmd];
    s = splnet();
    for(inp = head->lh_first; inp != NULL;) {
        oinp = inp;
        inp = inp->inp_list.le_next;    /* notify may free oinp */
        if(oinp->inp_family != inf || oinp->inp_socket == 0 ||
                !inpcb_addreq(&oinp->inp_fa, faddr, alen) ||
                (lport && oinp->inp_lport != lport) ||
                (laddr != NULL && !inpcb_addrany(&oinp->inp_la, alen) &&
                 !inpcb_addreq(&oinp->inp_la, laddr, alen)) ||
                (fport && oinp->inp_fport != fport))
            continue;
        if(notify)
            (*notify)(oinp, _errno);
    }
    splx(s);
}

/*
 * Check for alternatives when higher level complains
 * about service problems.  For now, invalidate cached
 * routing information.  If the route was created dynamically
 * (by a redirect), time to try a default gateway again.
 */
void
in_losing(inp)
struct inpcb *inp;
{
    register struct rtentry *rt;

    if((rt = inp->inp_route.ro_rt)) {
        inp->inp_route.ro_rt = 0;
        rt_missmsg(RTM_LOSING, &inp->inp_route.ro_dst,
                   rt->rt_gateway, (struct sockaddr *)rt_mask(rt),
                   (struct sockaddr *)0, rt->rt_flags, 0);
        if(rt->rt_flags & RTF_DYNAMIC)
            (void) rtrequest(RTM_DELETE, rt_key(rt),
                             rt->rt_gateway, rt_mask(rt), rt->rt_flags,
                             (struct rtentry **)0);
        else
            /*
             * A new route can be allocated
             * the next time output is attempted.
             */
            rtfree(rt);
    }
}

/*
 * After a routing change, flush old routing
 * and allocate a (hopefully) better one.
 */
void
in_rtchange(inp, _errno)
struct inpcb *inp;
int _errno;
{
    if(inp->inp_route.ro_rt) {
        rtfree(inp->inp_route.ro_rt);
        inp->inp_route.ro_rt = 0;
        /*
         * A new route can be allocated the next time
         * output is attempted.
         */
    }
}
