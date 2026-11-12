/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005-2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 */

/*
 * Address Resolution Protocol.
 * TODO:
 *	add "inuse/lock" bit (or ref. count) along with valid bit
 */

#include <conf.h>

#include <libraries/miami.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/synch.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_protos.h>
#include <net/pfil.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
/* the stack's own, not the public <netinet/in_var.h>: it declares
 * in_mcast_macaddr() */
#include "../netinet/in_var.h"
#ifndef AROSTCP_NETINET_IN_VAR_H
#error "the stack's netinet/in_var.h was not the one included"
#endif
#include <netinet/ip.h>

#include <net/if_sana.h>
#include <net/sana2arp.h>
#include <net/netmodule.h>

#include <net/if_loop_protos.h>
#include <protos/kern/kern_random_protos.h>
#include <protos/net/rtsock_protos.h>
#include <protos/netinet/in_protos.h>
#include <net/netcore_map.h>        /* always the last include */

/* ARP's packet hook point, registered by IPv4 (netinet/in_proto.c) */
extern struct pfil_point arp_pfil;

/*
 * Internet to hardware address resolution table entry
 */
struct	arptab {
    struct arptab *at_succ;	/* doubly linked list */
    struct arptab *at_pred;
    struct in_addr at_iaddr;	/* internet address */
    u_char         at_hwaddr[MAXADDRSANA]; /* hardware address */
    u_char         at_timer;	/* minutes since last reference */
    u_char         at_flags;	/* flags */
    u_char         at_holdlen;	/* packets in at_hold */
    struct mbuf   *at_hold;	/* packets until resolved/timeout (m_nextpkt) */
    ULONG          at_asked;	/* when the last request went out (seconds) */
    u_char         at_nud;	/* reachability, ARP_NUD_* (complete entries) */
    u_char         at_probes;	/* unicast probes sent (ARP_NUD_PROBE) */
    ULONG          at_nudtime;	/* arp_ticks when the state's time is up */
};

/*
 * Neighbour reachability for ARP entries, as IPv6's neighbour
 * unreachability detection (RFC 4861 section 7.3; RFC 1122 section
 * 2.3.2.1's unicast poll): an entry confirmed by an ARP reply is REACHABLE
 * for 30 s, then STALE; used while STALE it waits 5 s (DELAY) for a
 * confirmation, then is asked with up to 3 ARP requests sent to its
 * hardware address, a second apart (PROBE); unanswered, the entry goes and
 * the next packet resolves the address again. A neighbour that changed
 * its hardware address without telling was otherwise sent to at the old
 * one for as long as the entry was used: arpresolve() keeps a used entry.
 * Times in fast ticks (ARP_HZ), a monotonic clock.
 */
#define ARP_NUD_NONE       0    /* incomplete, or permanent */
#define ARP_NUD_REACHABLE  1
#define ARP_NUD_STALE      2
#define ARP_NUD_DELAY      3
#define ARP_NUD_PROBE      4
#define ARP_NUD_REACHABLE_TIME  (30 * ARP_HZ)
#define ARP_NUD_DELAY_TIME      (5 * ARP_HZ)
#define ARP_NUD_RETRANS_TIME    (1 * ARP_HZ)
#define ARP_NUD_MAX_PROBES      3


/*
 * Global constant for ARP entry allocation
 */
unsigned long arpentries = ARPENTRIES;

/*
 * General per interface hash table
 */
struct arptable {
    struct SignalSemaphore atb_lock;
    struct arptab         *atb_free;
    struct arptab         *atb_block;   /* the entries, as they were allocated */
    struct MinList         atb_entries[ARPTAB_HSIZE];
};

#define ARPTAB_LOCK(atb) (ObtainSemaphore(&atb->atb_lock))
#define ARPTAB_UNLOCK(atb) (ReleaseSemaphore(&atb->atb_lock))
#define	ARPTAB_HASH(a) ((u_long)(a) % ARPTAB_HSIZE)


int	useloopback = 0;	/* use loopback interface for local traffic */

static void arpwhohas(register struct sana_softc *ssc, struct in_addr *addr, const u_char *to);
static u_long arp_ticks;        /* ARP's monotonic clock: fast ticks (ARP_HZ)
                                   since the stack started; conflict detection's
                                   and reachability's times */
static void arp_nud_timer(void);

/*
 * While an address is resolved, up to ARP_MAXHOLD packets are kept for it,
 * oldest dropped first, as FreeBSD's net.link.ether.inet.maxhold: one is
 * enough for RFC 1122 section 2.3.2.2, but with one only the last fragment
 * of a datagram fragmented on its way out was sent. Requests for it go
 * out at most once a second (section 2.3.2.1: a guard against ARP floods
 * MUST be there), not once for every packet.
 */
#define ARP_MAXHOLD	16

static ULONG
arp_now(void)
{
    struct timeval tv;

    GetSysTime(&tv);
    return tv.tv_secs;
}

/* Queue m for at; ARP TABLE MUST BE LOCKED. */
static void
arp_hold(struct arptab *at, struct mbuf *m)
{
    struct mbuf **mp;

    if(at->at_holdlen >= ARP_MAXHOLD && at->at_hold != NULL) {
        struct mbuf *old = at->at_hold;

        at->at_hold = old->m_nextpkt;
        old->m_nextpkt = NULL;
        m_freem(old);
        at->at_holdlen--;
    }
    m->m_nextpkt = NULL;
    for(mp = &at->at_hold; *mp != NULL; mp = &(*mp)->m_nextpkt)
        ;
    *mp = m;
    at->at_holdlen++;
}

/* Drop at's queued packets; ARP TABLE MUST BE LOCKED. */
static void
arp_hold_free(struct arptab *at)
{
    struct mbuf *m, *next;

    for(m = at->at_hold; m != NULL; m = next) {
        next = m->m_nextpkt;
        m->m_nextpkt = NULL;
        m_freem(m);
    }
    at->at_hold = NULL;
    at->at_holdlen = 0;
}

/* Ask for at's address, unless that was done less than a second ago. */
static void
arp_ask(struct sana_softc *ssc, struct arptab *at, struct in_addr *destip)
{
    ULONG now = arp_now();

    if(at->at_asked != 0 && now - at->at_asked < 1)
        return;
    at->at_asked = now;
    arpwhohas(ssc, destip, NULL);
}
static void in_arpinput(register struct sana_softc *ssc, struct mbuf *m);
static char *sana_sprintf(register u_char *ap, int len);

/*
 * Initialization routine. Allocate ARP entries.
 * MUST BE CALLED AT SPLIMP.
 */
void
alloc_arptable(struct sana_softc *ssc, int to_allocate)
{
    struct arptab *at;
    struct arptable *atab;
    int i;

    if(ssc->ss_arp_table)
        return /* (void)ssc->ss_arp_table */;

#if 0
    if(to_allocate < arpentries)
#endif
        to_allocate = arpentries;

    atab = bsd_malloc(sizeof(*atab), M_ARPENT, M_WAITOK);
    at = bsd_malloc(sizeof(*at) * to_allocate, M_ARPENT, M_WAITOK);

    if(atab && at) {
        InitSemaphore(&atab->atb_lock);
        for(i = 0; i < ARPTAB_HSIZE; i++)
            NEWLIST((struct List *)(atab->atb_entries + i));

        aligned_bzero(at, sizeof(*at) * to_allocate);

        at[0].at_succ = NULL;
        for(i = 1; i < to_allocate; i++) {
            at[i].at_succ = &at[i - 1];
        }
        atab->atb_free = &at[to_allocate - 1];
        atab->atb_block = at;
    } else {
        if(atab) bsd_free(atab, M_ARPENT);
        if(at) bsd_free(at, M_ARPENT);
        __log(LOG_ERR, "Could not allocate ARP table for %s\n", ssc->ss_name);
    }

    ssc->ss_arp_table = atab;
}

/*
 * The reverse of alloc_arptable(): IPv4 stops, and what it kept with the
 * interface goes back to the pool - the table, its entries with the
 * packets that waited for an address, and the permanent entries, which
 * were allocated one by one. MUST BE CALLED AT SPLIMP, by the stack's task
 * with the stack fenced: nobody holds the table's lock then.
 */
static void
free_arptable(struct sana_softc *ssc)
{
    struct arptable *atab = ssc->ss_arp_table;
    struct arptab *at, *next;
    int i;

    if(atab == NULL)
        return;
    ssc->ss_arp_table = NULL;
    for(i = 0; i < ARPTAB_HSIZE; i++)
        for(at = (struct arptab *)atab->atb_entries[i].mlh_Head;
                (next = at->at_succ) != NULL; at = next) {
            arp_hold_free(at);
            if(at->at_flags & ATF_PERM) {
                Remove((struct Node *)at);
                bsd_free(at, M_ARPENT);
            }
        }
    bsd_free(atab->atb_block, M_ARPENT);
    bsd_free(atab, M_ARPENT);
}

/*
 * Notification function for arp entries
 */
LONG
arpentries_notify(void *dummy, LONG value)
{
    return (ULONG)value > ARPENTRIES_MIN;
}

/*
 * Free an arptab entry. ARP TABLE MUST BE LOCKED
 */
static void
arptfree(register struct arptable *atb, register struct arptab *at)
{
    arp_hold_free(at);

    Remove((struct Node *)at);

    if(at->at_flags & ATF_PERM) {
        bsd_free(at, M_ARPENT);
    } else {
        at->at_asked = 0;
        at->at_timer = at->at_flags = 0;
        at->at_nud = at->at_probes = 0;
        at->at_iaddr.s_addr = 0;
        at->at_succ = atb->atb_free;
        atb->atb_free = at;
    }
}

/*
 * Enter a new address in arptab. ARP TABLE MUST BE LOCKED
 */
static struct arptab *
arptnew(u_long addr, struct arptable *atb, int permanent)
{
    struct arptab *at = NULL;

    if(permanent) {
        at = bsd_malloc(sizeof(*at), M_ARPENT, M_WAITOK);
        bzero((caddr_t)at, sizeof(*at));
    } else {
        at = atb->atb_free;
        if(at) {
            atb->atb_free = at->at_succ;
        } else {
            /*
             * The oldest entry is pushed out from the
             * interface table if there is no free entry.
             * This should always succeed since all
             * entries can not be permanent
             */
            struct arptab *oldest = NULL;
            int i;

            for(i = 0; i < ARPTAB_HSIZE; i++) {
                for(at = (struct arptab *)atb->atb_entries[i].mlh_Head;
                        at->at_succ;
                        at = at->at_succ) {
                    if(at->at_flags == 0 || (at->at_flags & ATF_PERM))
                        continue;
                    if(!oldest || oldest->at_timer < at->at_timer)
                        oldest = at;
                }
            }
            if(oldest) {
                Remove((struct Node *)oldest);
                at = oldest;
            } else {
                at = NULL;
            }
        }
    }
    if(at) {
        at->at_iaddr.s_addr = addr;
        at->at_flags = ATF_INUSE;
        AddHead((struct List *)(&atb->atb_entries[ARPTAB_HASH(addr)]),
                (struct Node *)at);
    }
    return (at);
}

/*
 * Locate an IP address in the ARP table
 * Assume looker have locked the table
 */
static struct arptab *
arptab_look(struct arptable *table, u_long addr)
{
    register struct arptab *at = (struct arptab *)
                                 table->atb_entries[ARPTAB_HASH(addr)].mlh_Head;

    for(; at->at_succ; at = at->at_succ)
        if(at->at_iaddr.s_addr == addr)
            return at;

    return NULL;
}

/*
 * Timeout routine.  Age arp_tab entries once a minute.
 */
void
arptimer()
{
    struct sana_softc *ssc;
    register struct arptable *atab;
    register struct arptab *at, *next;
    register int i;

    for(ssc = ssq; ssc; ssc = ssc->ss_next) {
        if(!(atab = ssc->ss_arp_table))
            continue;
        /* Lock the table */
        ARPTAB_LOCK(atab);

        for(i = 0; i < ARPTAB_HSIZE; i++) {
            /* next is taken first: arptfree() moves at onto the free list */
            for(at = (struct arptab *)atab->atb_entries[i].mlh_Head;
                    (next = at->at_succ) != NULL;
                    at = next) {
                if(at->at_flags == 0 || (at->at_flags & ATF_PERM))
                    continue;
                if(++at->at_timer < ((at->at_flags & ATF_COM) ?
                                     ARPT_KILLC : ARPT_KILLI))
                    continue;
                /* timer has expired, clear entry */
                arptfree(atab, at);
            }
        }
        ARPTAB_UNLOCK(atab);
    }
}

/* =========================================================
 * IPv4LL / RFC 3927 implementation
 * ========================================================= */

/* Generate a pseudo-random 169.254.x.x address seeded by MAC + conflict count */
static void
autoip_randaddr(struct sana_softc *ssc, struct in_addr *out)
{
    u_char *hw = ssc->ss_hwaddr;
    u_int32_t r;

    r  = ((u_int32_t)hw[2] << 24) | ((u_int32_t)hw[3] << 16)
         | ((u_int32_t)hw[4] <<  8) | (u_int32_t)hw[5];
    r ^= (u_int32_t)ssc->ss_autoip.conflicts * 1664525UL;
    r  = r * 1664525UL + 1013904223UL;
    /* Range 169.254.1.0 - 169.254.254.255  (RFC 3927 §2.1) */
    r  = (r % (254 * 256)) + 256;
    out->s_addr = htonl(0xA9FE0000UL | r);
}

/* Send an ARP probe: sender IP = 0.0.0.0, target IP = candidate (RFC 3927 §2.1) */
static void
arp_probe(struct sana_softc *ssc, struct in_addr *candidate)
{
    struct mbuf      *m;
    struct s2_arppkt *s2a;
    struct sockaddr_sana2 ss2;
    struct in_addr    zero;

    zero.s_addr = INADDR_ANY;

    if((m = m_gethdr(M_DONTWAIT, MT_DATA)) == NULL)
        return;
    m->m_len         = sizeof(*s2a);
    m->m_pkthdr.len  = sizeof(*s2a);
    MH_ALIGN(m, sizeof(*s2a));
    s2a = mtod(m, struct s2_arppkt *);
    aligned_bzero_const((caddr_t)s2a, sizeof(*s2a));
    m->m_flags |= M_BCAST;

    s2a->arp_hrd = htons(ssc->ss_arp_hrd);
    s2a->arp_pro = htons(ssc->ss_ip.type);
    s2a->arp_pln = sizeof(struct in_addr);
    s2a->arp_hln = ssc->ss_if.if_addrlen;
    s2a->arp_op  = htons(ARPOP_REQUEST);

    bcopy((caddr_t)ssc->ss_hwaddr,
          (caddr_t)&s2a->arpdata, s2a->arp_hln);
    bcopy((caddr_t)&zero,
          (caddr_t)&s2a->arpdata + s2a->arp_hln, s2a->arp_pln);
    bzero((caddr_t)&s2a->arpdata + s2a->arp_hln + s2a->arp_pln,
          s2a->arp_hln);
    bcopy((caddr_t)candidate,
          (caddr_t)&s2a->arpdata + 2 * s2a->arp_hln + s2a->arp_pln,
          s2a->arp_pln);

    ss2.ss2_len    = sizeof(ss2);
    ss2.ss2_family = AF_UNSPEC;
    ss2.ss2_type   = ssc->ss_arp.type;
    (*ssc->ss_if.if_output)(&ssc->ss_if, m, (struct sockaddr *)&ss2,
                            (struct rtentry *)0);
}

/* Send an ARP announcement: sender IP = target IP = addr (RFC 3927 §2.6) */
static void
arp_announce(struct sana_softc *ssc, struct in_addr *addr)
{
    struct mbuf      *m;
    struct s2_arppkt *s2a;
    struct sockaddr_sana2 ss2;

    if((m = m_gethdr(M_DONTWAIT, MT_DATA)) == NULL)
        return;
    m->m_len         = sizeof(*s2a);
    m->m_pkthdr.len  = sizeof(*s2a);
    MH_ALIGN(m, sizeof(*s2a));
    s2a = mtod(m, struct s2_arppkt *);
    aligned_bzero_const((caddr_t)s2a, sizeof(*s2a));
    m->m_flags |= M_BCAST;

    s2a->arp_hrd = htons(ssc->ss_arp_hrd);
    s2a->arp_pro = htons(ssc->ss_ip.type);
    s2a->arp_pln = sizeof(struct in_addr);
    s2a->arp_hln = ssc->ss_if.if_addrlen;
    s2a->arp_op  = htons(ARPOP_REQUEST);

    /* sender: our MAC + candidate IP */
    bcopy((caddr_t)ssc->ss_hwaddr,
          (caddr_t)&s2a->arpdata, s2a->arp_hln);
    bcopy((caddr_t)addr,
          (caddr_t)&s2a->arpdata + s2a->arp_hln, s2a->arp_pln);
    /* target: broadcast MAC + candidate IP */
    bzero((caddr_t)&s2a->arpdata + s2a->arp_hln + s2a->arp_pln,
          s2a->arp_hln);
    bcopy((caddr_t)addr,
          (caddr_t)&s2a->arpdata + 2 * s2a->arp_hln + s2a->arp_pln,
          s2a->arp_pln);

    ss2.ss2_len    = sizeof(ss2);
    ss2.ss2_family = AF_UNSPEC;
    ss2.ss2_type   = ssc->ss_arp.type;
    (*ssc->ss_if.if_output)(&ssc->ss_if, m, (struct sockaddr *)&ss2,
                            (struct rtentry *)0);
}

/*
 * IPv4 link-local addresses (RFC 3927): autoip picks a 169.254/16 address
 * and adds it; conflict detection (RFC 5227, below) probes, announces and
 * defends it as any other address - RFC 3927's probing and defence are the
 * same, with the same constants. When the address is duplicated (another
 * host answered a probe, or claimed it twice within DEFEND_INTERVAL),
 * autoip_conflict() takes it off and picks another; after MAX_CONFLICTS,
 * one a minute (RATE_LIMIT_INTERVAL). ss_autoip.wanted is the interface's
 * IP=AUTO: autoip runs again when the link comes back.
 */

/* Add the candidate: it is tentative until conflict detection probed it */
static void
autoip_claim(struct sana_softc *ssc)
{
    struct in_originreq ifro;
    struct in_aliasreq *ifra = &ifro.ifro_ifra;

    autoip_randaddr(ssc, &ssc->ss_autoip.candidate);
    ssc->ss_autoip.state = AUTOIP_CLAIMING;
    memset(&ifro, 0, sizeof(ifro));
    ifra->ifra_addr.sin_len         = sizeof(ifra->ifra_addr);
    ifra->ifra_addr.sin_family      = AF_INET;
    ifra->ifra_addr.sin_addr        = ssc->ss_autoip.candidate;
    ifra->ifra_mask.sin_len         = sizeof(ifra->ifra_mask);
    ifra->ifra_mask.sin_family      = AF_INET;
    ifra->ifra_mask.sin_addr.s_addr = htonl(0xFFFF0000UL);   /* /16 */
    ifro.ifro_origin = IN_ORIGIN_LINKLOCAL;
    switch(in_control(NULL, SIOCAIFADDR_ORIGIN, (caddr_t)&ifro, &ssc->ss_if)) {
    case 0:
        break;
    case EEXIST:
        /* the address is on; another interface's 169.254/16 route stays
         * (one route per network): link-local peers are reached by that
         * interface (RFC 3927 section 3.2) */
        __log(LOG_WARNING, "IPv4LL: %s%d: 169.254/16 is routed by another interface\n",
              ssc->ss_if.if_name, ssc->ss_if.if_unit);
        break;
    default:
        ssc->ss_autoip.state = AUTOIP_WAIT;     /* try again in a second */
        ssc->ss_autoip.ticks = 1;
        return;
    }
    __log(LOG_INFO, "IPv4LL: claiming %d.%d.%d.%d/16 on %s%d\n",
          (ntohl(ssc->ss_autoip.candidate.s_addr) >> 24) & 0xFF,
          (ntohl(ssc->ss_autoip.candidate.s_addr) >> 16) & 0xFF,
          (ntohl(ssc->ss_autoip.candidate.s_addr) >>  8) & 0xFF,
          ntohl(ssc->ss_autoip.candidate.s_addr)        & 0xFF,
          ssc->ss_if.if_name, ssc->ss_if.if_unit);
}

/* Take the candidate off the interface */
static void
autoip_drop_addr(struct sana_softc *ssc)
{
    struct ifreq ifr;
    struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;

    memset(&ifr, 0, sizeof(ifr));
    sin->sin_len    = sizeof(*sin);
    sin->sin_family = AF_INET;
    sin->sin_addr   = ssc->ss_autoip.candidate;
    in_control(NULL, SIOCDIFADDR, (caddr_t)&ifr, &ssc->ss_if);
}

/* The candidate is duplicated (from arp_dad_conflict()): another one */
static void
autoip_conflict(struct sana_softc *ssc)
{
    if(ssc->ss_autoip.conflicts < 255)
        ssc->ss_autoip.conflicts++;
    __log(LOG_WARNING, "IPv4LL: address conflict on %s%d (count %u)\n",
          ssc->ss_if.if_name, ssc->ss_if.if_unit, (unsigned)ssc->ss_autoip.conflicts);
    autoip_drop_addr(ssc);
    if(ssc->ss_autoip.conflicts >= AUTOIP_MAX_CONFLICTS) {
        ssc->ss_autoip.state = AUTOIP_WAIT;     /* one a minute */
        ssc->ss_autoip.ticks = AUTOIP_RATE_LIMIT_INTERVAL;
    } else
        autoip_claim(ssc);
}

/* Begin IPv4LL on an interface (IP=AUTO, no DHCP, no address) */
void
autoip_start(struct ifnet *ifp)
{
    struct sana_softc *ssc = (struct sana_softc *)ifp;

    if(ssc->ss_autoip.state != AUTOIP_DISABLED)
        autoip_stop(ifp);
    ssc->ss_autoip.wanted = 1;
    ssc->ss_autoip.conflicts = 0;
    autoip_claim(ssc);
}

/* Stop IPv4LL on an interface, its address going with the others; the link
 * coming back starts it again while ss_autoip.wanted is set */
void
autoip_stop(struct ifnet *ifp)
{
    struct sana_softc *ssc = (struct sana_softc *)ifp;

    if(ssc->ss_autoip.state == AUTOIP_CLAIMING)
        autoip_drop_addr(ssc);
    ssc->ss_autoip.state = AUTOIP_DISABLED;
    ssc->ss_autoip.ticks = 0;
}

/* Once a second: the rate limit's wait */
void
autoip_timer(void)
{
    struct sana_softc *ssc;

    for(ssc = ssq; ssc; ssc = ssc->ss_next)
        if(ssc->ss_autoip.state == AUTOIP_WAIT && --ssc->ss_autoip.ticks == 0)
            autoip_claim(ssc);
}

/* =========================================================
 * Address conflict detection, RFC 5227 (as NetBSD's arp_dad_*)
 * ========================================================= */

/*
 * An address configured on an ARP interface is tentative (IN_IFF_TENTATIVE)
 * until ip_dad_count probes - ARP requests from 0.0.0.0 for it - have gone
 * unanswered: it is not used as a source, not accepted as a destination and
 * not answered for meanwhile. Then it is announced twice and ready; a host
 * that answers a probe, or probes for it at the same time, makes it
 * IN_IFF_DUPLICATED. An address in use is defended once in DEFEND_INTERVAL
 * (an announcement); a second conflict within it makes a DHCP or IPv4LL
 * address duplicated (RFC 5227 section 2.4 (b)) and leaves one configured
 * by hand in use (2.4 (c), ia_origin). A duplicated address is
 * probed again every ARP_DAD_REPROBE, and is ready when no one answers. Each change is announced as RTM_NEWADDR and the
 * SBTC_SIG_ADDRESS_CHANGE_MASK signal (rt_newaddrmsg()), as IPv6's DAD
 * does. The 169.254/16 address autoip claims (RFC 3927) goes the same way,
 * and when duplicated back to autoip for another. net.inet.ip.dad_count 0
 * turns the probes off for other addresses: ready at once, announced, and
 * a conflict only logged.
 */
int ip_dad_count = 3;           /* PROBE_NUM */

/* The SANA-II interface that resolves with ARP behind ifp, if any. */
static struct sana_softc *
arp_dad_ssc(struct ifnet *ifp)
{
    struct sana_softc *ssc;

    if(ifp == NULL || (ifp->if_flags & (IFF_LOOPBACK | IFF_POINTOPOINT | IFF_NOARP)))
        return NULL;
    ssc = sana_softc_of(ifp);
    return ssc && ssc->ss_arp_table ? ssc : NULL;
}

int
arp_dad_capable(struct ifnet *ifp)
{
    return arp_dad_ssc(ifp) != NULL;
}

/* Is ia the 169.254/16 address autoip claims (RFC 3927)? */
static int
arp_dad_autoip(struct sana_softc *ssc, struct in_ifaddr *ia)
{
    return ssc->ss_autoip.state == AUTOIP_CLAIMING &&
           ia->ia_addr.sin_addr.s_addr == ssc->ss_autoip.candidate.s_addr;
}

/*
 * Where an address came from (ia_origin, given when it was added: see
 * IN_ORIGIN_* in <netinet/in_var.h>) decides what a conflict over it does
 * once it is in use (RFC 5227 section 2.4): one configured by hand is never
 * given up - defended once per DEFEND_INTERVAL, the conflict logged (2.4
 * (c)); only its probes, before it is first used, can refuse it. A DHCP
 * address whose defence fails is duplicated and goes back to dhclient,
 * which declines it; an IPv4LL one goes back to autoip for another.
 */

/*
 * An in-place reload deletes every address and adds them again: one that
 * was in use and comes back unchanged within ARP_DAD_KEEP_TIME keeps its
 * state and is not probed again (the window covers dhclient applying its
 * lease again after the reload).
 */
#define ARP_DAD_KEEP_MAX   16
#define ARP_DAD_KEEP_TIME  (30 * ARP_HZ)

static struct {
    u_short        index;
    struct in_addr addr;
    u_long         until;
} arp_dad_keep[ARP_DAD_KEEP_MAX];
static int arp_dad_reloading;

void
arp_dad_reload(int begin)
{
    arp_dad_reloading = begin;
}

/* ia is being deleted: during a reload, remember it if it was in use */
void
arp_dad_forget(struct in_ifaddr *ia)
{
    int i, slot = -1;

    if(!arp_dad_reloading || (ia->ia4_flags & IN_IFF_NOTREADY) ||
            ia->ia_addr.sin_addr.s_addr == INADDR_ANY || arp_dad_ssc(ia->ia_ifp) == NULL)
        return;
    for(i = 0; i < ARP_DAD_KEEP_MAX; i++)
        if(slot < 0 && (long)(arp_ticks - arp_dad_keep[i].until) >= 0)
            slot = i;
    if(slot < 0)
        return;
    arp_dad_keep[slot].index = ia->ia_ifp->if_index;
    arp_dad_keep[slot].addr = ia->ia_addr.sin_addr;
    arp_dad_keep[slot].until = arp_ticks + ARP_DAD_KEEP_TIME;
}

/* Was ia in use before a reload took it off? (The entry is used up.) */
static int
arp_dad_kept(const struct in_ifaddr *ia)
{
    int i;

    for(i = 0; i < ARP_DAD_KEEP_MAX; i++)
        if((long)(arp_ticks - arp_dad_keep[i].until) < 0 &&
                arp_dad_keep[i].index == ia->ia_ifp->if_index &&
                arp_dad_keep[i].addr.s_addr == ia->ia_addr.sin_addr.s_addr) {
            arp_dad_keep[i].until = arp_ticks;
            return 1;
        }
    return 0;
}

static void
arp_dad_log(int pri, const char *what, struct in_ifaddr *ia, caddr_t sha, int len)
{
    u_long a = ntohl(ia->ia_addr.sin_addr.s_addr);

    __log(pri, "%s%d: %s %lu.%lu.%lu.%lu%s%s\n", ia->ia_ifp->if_name, ia->ia_ifp->if_unit, what,
          (a >> 24) & 0xff, (a >> 16) & 0xff, (a >> 8) & 0xff, a & 0xff,
          sha ? " from " : "", sha ? sana_sprintf(sha, len) : "");
}

/* A new address on an ARP interface: probe it before it is used. */
void
arp_dad_start(struct in_ifaddr *ia)
{
    struct sana_softc *ssc = arp_dad_ssc(ia->ia_ifp);

    arp_dad_stop(ia);
    if(ssc == NULL || ia->ia_addr.sin_addr.s_addr == INADDR_ANY || arp_dad_kept(ia))
        return;
    /* RFC 3927 has an IPv4LL address probed whatever dad_count says */
    if(ip_dad_count == 0 && !arp_dad_autoip(ssc, ia)) {
        ia->ia_dad_announces = ARP_DAD_ANNOUNCE_NUM;
        ia->ia_dad_ticks = 1;
        return;
    }
    ia->ia4_flags |= IN_IFF_TENTATIVE;
    ia->ia_dad_count = ip_dad_count ? ip_dad_count : ARP_DAD_PROBE_NUM;
    ia->ia_dad_ticks = 1 + kern_random32() % ARP_DAD_PROBE_WAIT;
    arp_dad_log(LOG_INFO, "probing", ia, NULL, 0);
}

/* No more probing or announcing (the address is going, or changing). */
void
arp_dad_stop(struct in_ifaddr *ia)
{
    ia->ia4_flags &= ~IN_IFF_NOTREADY;
    ia->ia_dad_ticks = 0;
    ia->ia_dad_probes = 0;
    ia->ia_dad_announces = 0;
    ia->ia_dad_defended = 0;
}

/*
 * The link came back (NFE_LINKUP): the network may be another one. An
 * address that stayed through it - one configured by hand: the others went
 * with the link (in_iflinkdown()) - is probed again before it is used, as
 * RFC 5227 section 2.1 asks whenever the link-layer connection comes back
 * and NetBSD's in_if_link_up() does.
 */
void
arp_dad_linkup(const struct ifnet *ifp)
{
    struct in_ifaddr *ia;
    spl_t s = splimp();

    for(ia = in_ifaddr; ia; ia = ia->ia_next)
        if(ia->ia_ifp == ifp && ia->ia_addr.sin_addr.s_addr != INADDR_ANY) {
            arp_dad_start(ia);
            rt_newaddrmsg(RTM_NEWADDR, &ia->ia_ifa, 0, NULL);
        }
    splx(s);
}

/* Every fast tick (ip's pr_fasttimo): ARP's clock, conflict detection,
 * and once a second reachability */
void
arp_fasttimo(void)
{
    struct in_ifaddr *ia;

    arp_ticks++;
    if(arp_ticks % ARP_HZ == 0)
        arp_nud_timer();
    for(ia = in_ifaddr; ia; ia = ia->ia_next) {
        struct sana_softc *ssc;

        if(ia->ia_dad_ticks == 0 || --ia->ia_dad_ticks > 0)
            continue;
        if((ssc = arp_dad_ssc(ia->ia_ifp)) == NULL)
            continue;
        if(!(ssc->ss_if.if_flags & IFF_UP)) {
            ia->ia_dad_ticks = ARP_HZ;      /* the link is not up yet */
            continue;
        }
        if(ia->ia4_flags & IN_IFF_DUPLICATED) {
            /* probed again once a minute; back when no one answers */
            ia->ia4_flags = (ia->ia4_flags & ~IN_IFF_DUPLICATED) | IN_IFF_TENTATIVE;
            ia->ia_dad_probes = 0;
            ia->ia_dad_count = ip_dad_count ? ip_dad_count : ARP_DAD_PROBE_NUM;
            ia->ia_dad_ticks = 1 + kern_random32() % ARP_DAD_PROBE_WAIT;
            arp_dad_log(LOG_INFO, "probing again", ia, NULL, 0);
            rt_newaddrmsg(RTM_NEWADDR, &ia->ia_ifa, 0, NULL);
            continue;
        }
        if(ia->ia4_flags & IN_IFF_TENTATIVE) {
            if(ia->ia_dad_probes < ia->ia_dad_count) {
                arp_probe(ssc, &ia->ia_addr.sin_addr);
                ia->ia_dad_probes++;
                ia->ia_dad_ticks = ia->ia_dad_probes < ia->ia_dad_count ?
                                   ARP_DAD_PROBE_MIN + kern_random32() %
                                   (ARP_DAD_PROBE_MAX - ARP_DAD_PROBE_MIN) :
                                   ARP_DAD_ANNOUNCE_WAIT;
                continue;
            }
            ia->ia4_flags &= ~IN_IFF_TENTATIVE;     /* no one has it */
            arp_dad_log(LOG_INFO, "probed, no conflict:", ia, NULL, 0);
            rt_newaddrmsg(RTM_NEWADDR, &ia->ia_ifa, 0, NULL);
            ia->ia_dad_announces = ARP_DAD_ANNOUNCE_NUM;
        }
        if(ia->ia_dad_announces) {
            arp_announce(ssc, &ia->ia_addr.sin_addr);
            if(--ia->ia_dad_announces)
                ia->ia_dad_ticks = ARP_DAD_ANNOUNCE_INTERVAL;
        }
    }
}

/* Once a second (from arp_fasttimo()): DELAY and PROBE entries move on */
static void
arp_nud_timer(void)
{
    struct sana_softc *ssc;

    for(ssc = ssq; ssc; ssc = ssc->ss_next) {
        struct arptable *atb = ssc->ss_arp_table;
        struct arptab *at, *next;
        int i;

        if(atb == NULL)
            continue;
        ARPTAB_LOCK(atb);
        for(i = 0; i < ARPTAB_HSIZE; i++)
            for(at = (struct arptab *)atb->atb_entries[i].mlh_Head; (next = at->at_succ) != NULL; at = next) {
                if((at->at_nud != ARP_NUD_DELAY && at->at_nud != ARP_NUD_PROBE) ||
                        (long)(arp_ticks - at->at_nudtime) < 0)
                    continue;
                if(at->at_nud == ARP_NUD_DELAY) {
                    at->at_nud = ARP_NUD_PROBE;
                    at->at_probes = 0;
                }
                if(at->at_probes >= ARP_NUD_MAX_PROBES) {
                    arptfree(atb, at);          /* unreachable: resolve anew */
                    continue;
                }
                at->at_probes++;
                at->at_nudtime = arp_ticks + ARP_NUD_RETRANS_TIME;
                arpwhohas(ssc, &at->at_iaddr, at->at_hwaddr);
            }
        ARPTAB_UNLOCK(atb);
    }
}

/*
 * Another host (sha) has ia's address: it answered or sent a probe for it,
 * or uses it as its sender. 1 if the ARP packet is to be dropped.
 */
static int
arp_dad_conflict(struct sana_softc *ssc, struct in_ifaddr *ia, caddr_t sha)
{
    int len = ssc->ss_if.if_addrlen;

    if(!(ia->ia4_flags & IN_IFF_NOTREADY)) {
        if(ip_dad_count == 0 && !arp_dad_autoip(ssc, ia)) {
            arp_dad_log(LOG_ERR, "duplicate IP address", ia, sha, len);
            return 0;                   /* the old behaviour: answer */
        }
        if(ia->ia_dad_defended == 0 ||
                arp_ticks - ia->ia_dad_defended >= ARP_DAD_DEFEND_INTERVAL) {
            ia->ia_dad_defended = arp_ticks ? arp_ticks : 1;
            arp_announce(ssc, &ia->ia_addr.sin_addr);
            arp_dad_log(LOG_ERR, "defended", ia, sha, len);
            return 1;
        }
        if(ia->ia_origin == IN_ORIGIN_MANUAL)
            return 1;   /* kept, defended again when DEFEND_INTERVAL is past */
        arp_dad_log(LOG_ERR, "defence failed, duplicated:", ia, sha, len);
    } else if(ia->ia4_flags & IN_IFF_DUPLICATED)
        return 1;
    else
        arp_dad_log(LOG_ERR, "duplicated while probing:", ia, sha, len);
    ia->ia_dad_ticks = ARP_DAD_REPROBE;     /* probed again in a minute */
    ia->ia_dad_announces = 0;
    ia->ia4_flags = (ia->ia4_flags & ~IN_IFF_TENTATIVE) | IN_IFF_DUPLICATED;
    rt_newaddrmsg(RTM_NEWADDR, &ia->ia_ifa, 0, NULL);
    if(arp_dad_autoip(ssc, ia))
        autoip_conflict(ssc);           /* takes ia off: another address */
    return 1;
}

/*
 * Broadcast an ARP packet, asking who has addr on interface ssc; sent to
 * the hardware address to instead if not NULL (a neighbour's reachability
 * probe, RFC 1122 section 2.3.2.1).
 */
static void
arpwhohas(register struct sana_softc *ssc, struct in_addr *addr, const u_char *to)
{
    register struct mbuf *m;
    register struct s2_arppkt *s2a;
    struct sockaddr_sana2 ss2;

    if((m = m_gethdr(M_DONTWAIT, MT_DATA)) == NULL)
        return;
    m->m_len = sizeof(*s2a);
    m->m_pkthdr.len = sizeof(*s2a);
    MH_ALIGN(m, sizeof(*s2a));
    s2a = mtod(m, struct s2_arppkt *);
    aligned_bzero_const((caddr_t)s2a, sizeof(*s2a));
    if(to == NULL)
        m->m_flags |= M_BCAST;

    /* fill in header depending of the interface */
    s2a->arp_hrd = htons(ssc->ss_arp_hrd);
    s2a->arp_pro = htons(ssc->ss_ip.type);
    s2a->arp_pln = sizeof(struct in_addr); /* protocol address length */
    s2a->arp_hln = ssc->ss_if.if_addrlen;  /* hardware address length */
    s2a->arp_op = htons(ARPOP_REQUEST);

    /* Copy source hardware address */
    bcopy((caddr_t)ssc->ss_hwaddr,
          (caddr_t)&s2a->arpdata,
          s2a->arp_hln);
    /* Copy source protocol address: one of the interface's that is ready,
     * the interface's own first; 0.0.0.0 if none is (RFC 5227 2.1.1:
     * neighbours must not cache a tentative or duplicated one) */
    {
        struct in_addr src;
        const struct in_ifaddr *ia, *ready = NULL;

        for(ia = in_ifaddr; ia; ia = ia->ia_next)
            if(ia->ia_ifp == &ssc->ss_if && !(ia->ia4_flags & IN_IFF_NOTREADY) &&
                    ia->ia_addr.sin_addr.s_addr != INADDR_ANY &&
                    (ready == NULL || ia->ia_addr.sin_addr.s_addr == ssc->ss_ipaddr.s_addr))
                ready = ia;
        src.s_addr = ready ? ready->ia_addr.sin_addr.s_addr : INADDR_ANY;
        bcopy((caddr_t)&src,
              (caddr_t)&s2a->arpdata + s2a->arp_hln,
              s2a->arp_pln);
    }
    /* Zero target hardware address */
    bzero((caddr_t)&s2a->arpdata + s2a->arp_hln + s2a->arp_pln,
          s2a->arp_hln);
    /* Copy target protocol address */
    bcopy((caddr_t)addr,
          (caddr_t)&s2a->arpdata + 2 * s2a->arp_hln + s2a->arp_pln,
          s2a->arp_pln);

    /* Send an ARP packet */
    ss2.ss2_len = sizeof(ss2);
    ss2.ss2_family = AF_UNSPEC;
    ss2.ss2_type = ssc->ss_arp.type;
    if(to != NULL)
        bcopy((caddr_t)to, ss2.ss2_host, ssc->ss_if.if_addrlen);
    (*ssc->ss_if.if_output)(&ssc->ss_if, m, (struct sockaddr *)&ss2,
                            (struct rtentry *)0);
}

/*
 * Resolve an IP address into an SANA-II address.  If success,
 * desten is filled in.  If there is no entry in arptab,
 * set one up and broadcast a request for the IP address.
 * Hold onto this mbuf and resend it once the address
 * is finally resolved.  A return value of 1 indicates
 * that desten has been filled in and the packet should be sent
 * normally; a 0 return indicates that the packet has been
 * taken over here, either now or for later transmission.
 *
 * We do some (conservative) locking here at splimp, since
 * arptab is also altered from sana poll routine
 */
int
arpresolve(register struct sana_softc *ssc,
           struct mbuf *m,
           register struct in_addr *destip,
           register u_char *desten,
           int *error)
{
    register struct arptab *at;
    register struct arptable *atb;

    if(m->m_flags & M_BCAST) {	/* broadcast */
        return 1;
    }

#ifdef ENABLE_MULTICAST
    /*
     * A group has no ARP entry. On Ethernet it has an address of its own;
     * anywhere else ip_output() has already refused the packet, as the
     * interface has no IFF_MULTICAST.
     */
    if(IN_MULTICAST(ntohl(destip->s_addr))) {
        if(!sana_ether_mcast(&ssc->ss_if)) {
            m_freem(m);
            *error = ENETUNREACH;
            return 0;
        }
        in_mcast_macaddr(destip, desten);
        return 1;
    }
#endif

    /* if for us, use software loopback driver if up; an address of ours
     * that is not ready (RFC 5227: tentative, or another host's) is not
     * us - what is sent to it goes out on the link, to its owner */
    for(register struct in_ifaddr *ia = in_ifaddr; ia; ia = ia->ia_next)
        if((ia->ia_ifp == &ssc->ss_if) &&
                (destip->s_addr == ia->ia_addr.sin_addr.s_addr) &&
                !(ia->ia4_flags & IN_IFF_NOTREADY)) {
            /*
             * This test used to be
             *	if (loif.if_flags & IFF_UP)
             * It allowed local traffic to be forced
             * through the hardware by configuring the loopback down.
             * However, it causes problems during network configuration
             * for boards that can't receive packets they send.
             * It is now necessary to clear "useloopback"
             * to force traffic out to the hardware.
             */
            if(useloopback) {
                struct sockaddr_in sin;

                sin.sin_family = AF_INET;
                sin.sin_addr = *destip;
                (void) looutput(&loif, m, (struct sockaddr *)&sin, 0);
                /*
                 * The packet has already been sent and freed.
                 */
                return (0);
            } else {
                bcopy((caddr_t)ssc->ss_hwaddr, (caddr_t)desten, ssc->ss_if.if_addrlen);
                return (1);
            }
        }

    if(ssc->ss_if.if_flags & IFF_NOARP) {
        /* No arp */
        __log(LOG_ERR,
              "arpresolve: can't resolve address for if %s/%ld\n",
              ssc->ss_if.if_name, (long)ssc->ss_if.if_unit);
        *error = ENETUNREACH;
        m_freem(m);
        return (0);
    }

    /* Try to locate ARP table */
    if(!(atb = ssc->ss_arp_table)) {
        alloc_arptable(ssc, 0);
        if(!(atb = ssc->ss_arp_table)) {
            __log(LOG_ERR, "arpresolve: memory exhausted");
            *error = ENOBUFS;
            m_freem(m);
            return 0;
        }
    }

    ARPTAB_LOCK(atb);
    at = arptab_look(atb, destip->s_addr);
    if(at == 0) {		/* not found */
        at = arptnew(destip->s_addr, atb, FALSE);
        if(at) {
            arp_hold(at, m);
            arp_ask(ssc, at, destip);
        } else {
            __log(LOG_ERR, "arpresolve: no free entry");
            *error = ENETUNREACH;
            m_freem(m);
        }
        ARPTAB_UNLOCK(atb);
        return 0;
    }

    at->at_timer = 0;		/* restart the timer */
    if(at->at_flags & ATF_COM) {	/* entry IS complete */
        /* reachability (RFC 4861 7.3.3): used after it went stale, it
         * waits DELAY for a confirmation before it is probed */
        if(at->at_nud == ARP_NUD_REACHABLE && (long)(arp_ticks - at->at_nudtime) >= 0)
            at->at_nud = ARP_NUD_STALE;
        if(at->at_nud == ARP_NUD_STALE) {
            at->at_nud = ARP_NUD_DELAY;
            at->at_nudtime = arp_ticks + ARP_NUD_DELAY_TIME;
        }
        bcopy((caddr_t)at->at_hwaddr, (caddr_t)desten, ssc->ss_if.if_addrlen);
        ARPTAB_UNLOCK(atb);
        return 1;
    }
    /*
     * There is an arptab entry, but no address response yet: queue the
     * packet, and ask again if the last request is a second old.
     */
    arp_hold(at, m);
    arp_ask(ssc, at, destip);
    ARPTAB_UNLOCK(atb);
    return 0;
}

/*
 * IPv4's address resolution for the SANA-II interfaces, as the link type
 * that carries IPv4 asks for it (lt_resolve, net/if_sana.h): 0 with desten
 * filled in, or EAGAIN when the packet was taken over - held until the
 * address is known, sent over the loopback, or dropped. Called from
 * sana_output() at splimp; registered by arp_init().
 *
 * The rest of what IPv4 does for a packet on its way to a device is here
 * too: a copy of our own broadcast for ourselves where the device does not
 * hear what it sends, and the request for the low-delay type of service to
 * go ahead of the others.
 */
static int
arp_lt_resolve(struct ifnet *ifp, struct rtentry *rt, struct mbuf *m,
               struct sockaddr *dst, u_char *desten)
{
    struct sana_softc *ssc = (struct sana_softc *)ifp;
    struct in_addr idst = ((struct sockaddr_in *)dst)->sin_addr;
    int error = 0;

    /* If the address is not resolved, arpresolve
     * stores the packet to its private queue for
     * later transmit and broadcasts the resolve
     * request packet to the (ether)net.
     * (Now ARP works only with IP and ethernet.)
     */
    if((ifp->if_flags & IFF_NOARP) != IFF_NOARP &&
            !arpresolve(ssc, m, &idst, desten, &error))
        return (EAGAIN);

    /* Send to loopback if we do not hear our broadcasts */
    if((ifp->if_flags & IFF_SIMPLEX) && (m->m_flags & M_BCAST)) {
        /* If a broadcast, send a copy to ourself too */
        struct mbuf *mcopy = m_copy(m, 0, (int)M_COPYALL);

        if(mcopy != NULL)
            (void) looutput(ifp, mcopy, dst, rt);
    }
    /* Set the message priority */
    if(IPTOS_LOWDELAY & mtod(m, struct ip *)->ip_tos)
        m->m_flags |= M_LINKPRI;
    return (0);
}

/*
 * ARP and IPv4 link-local addressing become part of the running stack:
 * called from ip_init(). The SANA-II layer and the timers do not name
 * them - IPv4 registers its resolver for the link type that carries it,
 * the input of the ARP link type, and its two periodic calls.
 */
static void *arp_age_timer, *arp_ll_timer;      /* the two periodic calls */

void
arp_init(void)
{
    (void)sana_linktype_resolver(AF_INET, arp_lt_resolve);
    (void)sana_linktype_handler(SANA_PT_ARP, arpinput, sizeof(struct s2_arppkt));
    if((arp_age_timer = netperiodic_add(arptimer, ARPT_AGE * hz)) == NULL ||
            (arp_ll_timer = netperiodic_add(autoip_timer, 1 * hz)) == NULL)
        __log(LOG_ERR, "arp_init: no timer for ARP ageing or IPv4LL\n");
}

/*
 * ARP and IPv4 link-local addressing leave the running stack: called when
 * IPv4 stops (inet_stop(), netinet/in_proto.c), after the read requests of
 * the ARP type were taken back from the drivers (sana_linktype_quiesce())
 * and the interfaces' IPv4 addresses removed. The reverse of arp_init()
 * and of what ARP made since: the SANA-II layer no longer has the resolver
 * and the input, the two periodic calls are deleted, and every interface
 * loses its ARP table, its copy of the IPv4 address and its IPv4LL state.
 * This file's variables are then as the file has them.
 */
void
arp_deinit(void)
{
    struct sana_softc *ssc;
    spl_t s;

    (void)sana_linktype_resolver(AF_INET, NULL);
    (void)sana_linktype_handler(SANA_PT_ARP, NULL, 0);
    netperiodic_remove(arp_age_timer);
    netperiodic_remove(arp_ll_timer);
    arp_age_timer = arp_ll_timer = NULL;

    s = splimp();
    for(ssc = ssq; ssc; ssc = ssc->ss_next) {
        free_arptable(ssc);
        ssc->ss_ipaddr.s_addr = 0;
        bzero((caddr_t)&ssc->ss_autoip, sizeof(ssc->ss_autoip));    /* AUTOIP_DISABLED */
    }
    splx(s);
    arpentries = ARPENTRIES;
    useloopback = 0;
}

/*
 * Called from the sana poll routine
 * when ARP type packet is received.
 * Common length and type checks are done here,
 * then the protocol-specific routine is called.
 * In addition, a sanity check is performed on the sender
 * protocol address, to catch impersonators.
 */
void
arpinput(struct sana_softc *ssc,
         struct mbuf *m,
         caddr_t srcaddr)
{
    register struct arphdr *ar;
    int proto;

    if(ssc->ss_if.if_flags & IFF_NOARP)
        goto out;
    if(m->m_len < sizeof(struct arphdr))
        goto out;
    ar = mtod(m, struct arphdr *);
    if(ntohs(ar->ar_hrd) != ssc->ss_arp_hrd)
        goto out;
    if(m->m_len < sizeof(struct arphdr) + 2 * ar->ar_hln + 2 * ar->ar_pln)
        goto out;
    if(ar->ar_hln != ssc->ss_if.if_addrlen)
        goto out;

#ifdef paranoid_arp_mode
    /* Sanity check */
    if(bcmp(srcaddr, (UBYTE *)ar + sizeof(*ar), ar->ar_hln)) {
        __log(LOG_ERR, "An ARP packet sent as %s",
              sana_sprintf(srcaddr, ar->ar_hln));
        __log(LOG_ERR, " from address: %s!!\n",
              sana_sprintf((UBYTE *)ar + sizeof(*ar), ar->ar_hln));
        goto out;
    }
#endif
    /* the packet hooks (Miami hooks): one word tested while nothing is hooked (net/pfil.h) */
    if(PFIL_HOOKED(&arp_pfil, PFIL_IN)) {
        struct mbuf *hm = m;

        if(pfil_run(&arp_pfil, &hm, &ssc->ss_if, PFIL_IN) != 0)
            return;             /* dropped by a hook, and freed */
        m = hm;
        ar = mtod(m, struct arphdr *);
    }
    proto = ntohs(ar->ar_pro);

    if(proto == ssc->ss_ip.type) {
        in_arpinput(ssc, m);
        return;
    }

out:
    m_freem(m);
}

/*
 * ARP for Internet protocols on SANA-II interfaces.
 * Algorithm is that given in RFC 826.
 */
static void
in_arpinput(register struct sana_softc *ssc,
            struct mbuf *m)
{
    register struct s2_arppkt *s2a;
    struct sockaddr_in sin;
    struct in_addr isaddr, itaddr, myaddr;
    struct in_ifaddr *myia = NULL;      /* the address of ours concerned */
    int op;
#if 0 /* unused */
    int completed = 0;
#endif
    caddr_t sha, spa, tha, tpa;
    size_t  len = ssc->ss_if.if_addrlen;

    s2a = mtod(m, struct s2_arppkt *);
    op = ntohs(s2a->arp_op);

    if(s2a->arp_pln != sizeof(struct in_addr))
        goto out;

    sha = (caddr_t) & (s2a->arpdata); /* other members must be calculated */
    bcopy(spa = sha + len, (caddr_t)&isaddr, sizeof(isaddr));
    tha = spa + sizeof(struct in_addr);
    bcopy(tpa = tha + len, (caddr_t)&itaddr, sizeof(itaddr));


    {
        register struct in_ifaddr *ia;
        struct in_ifaddr *maybe_ia = 0;

        /* Check for our own ARP packets */
        for(ia = in_ifaddr; ia; ia = ia->ia_next)
            if(ia->ia_ifp == &ssc->ss_if) {
                maybe_ia = ia;
                if((itaddr.s_addr == ia->ia_addr.sin_addr.s_addr) ||
                        (isaddr.s_addr == ia->ia_addr.sin_addr.s_addr))
                    break;
            }
        if(maybe_ia == 0)
            goto out;
        myia = ia ? ia : maybe_ia;
        myaddr = myia->ia_addr.sin_addr;
        if(!bcmp(sha, (caddr_t)ssc->ss_hwaddr, len))
            goto out;			/* it's from me, ignore it. */
    }
#ifndef AMITCP
    if(!bcmp(sha, (caddr_t)etherbroadcastaddr, ac->ac_if.if_addrlen)) {
        __log(LOG_ERR,
              "arp: ether address is broadcast for IP address %lx!\n",
              (u_long)ntohl(isaddr.s_addr));
        goto out;
    }
#endif

    /*
     * Address conflicts (RFC 5227 section 2.1.1, 2.4): another host using
     * our address as its sender is always one; a probe for it (sender
     * 0.0.0.0, broadcast) is one while ours is still tentative - a host
     * probing for an address we use gets our answer below. An address
     * already duplicated is its owner's: what the owner sends is taken as
     * from any neighbour (its entry is made), and not answered for.
     */
    if(!(myia->ia4_flags & IN_IFF_DUPLICATED) &&
            (isaddr.s_addr == myaddr.s_addr ||
             (isaddr.s_addr == INADDR_ANY && itaddr.s_addr == myaddr.s_addr &&
              (m->m_flags & M_BCAST) && (myia->ia4_flags & IN_IFF_TENTATIVE)))) {
        if(arp_dad_conflict(ssc, myia, sha))
            goto out;
        itaddr = myaddr;                /* dad_count 0: answer, as before */
        if(op == ARPOP_REQUEST)
            goto reply;
        goto out;
    }

    {
        struct arptable *atb;
        register struct arptab *at = NULL; /* same as "merge" flag */

        /* Try to locate ARP table; it is made when first needed */
        if(!(atb = ssc->ss_arp_table)) {
            spl_t s = splimp();

            alloc_arptable(ssc, 0);
            splx(s);
            if(!(atb = ssc->ss_arp_table))
                goto reply;
        }

        ARPTAB_LOCK(atb);
        at = arptab_look(atb, isaddr.s_addr);

        if(at) {
            /* a reply to us confirms the neighbour; anything else from it
             * with another hardware address makes the entry stale, to be
             * checked before long (RFC 4861 7.2.5, 7.3.3) */
            if(!(at->at_flags & ATF_PERM)) {
                if(op == ARPOP_REPLY && itaddr.s_addr == myaddr.s_addr) {
                    at->at_nud = ARP_NUD_REACHABLE;
                    at->at_nudtime = arp_ticks + ARP_NUD_REACHABLE_TIME;
                } else if(!(at->at_flags & ATF_COM) ||
                          bcmp(sha, (caddr_t)at->at_hwaddr, len) != 0)
                    at->at_nud = ARP_NUD_STALE;
            }
            bcopy(sha, (caddr_t)at->at_hwaddr, len);
#if 0 /* unused */
            if((at->at_flags & ATF_COM) == 0)
                completed = 1;
#endif
            at->at_flags |= ATF_COM;
            if(at->at_hold) {
                /* the address is known: send what was queued, in order */
                struct mbuf *held = at->at_hold, *next;

                at->at_hold = NULL;
                at->at_holdlen = 0;
                sin.sin_family = AF_INET;
                sin.sin_addr = isaddr;
                for(; held != NULL; held = next) {
                    next = held->m_nextpkt;
                    held->m_nextpkt = NULL;
                    (*ssc->ss_if.if_output)(&ssc->ss_if, held,
                                            (struct sockaddr *)&sin, (struct rtentry *)0);
                }
            }
        }
        if(at == 0 && itaddr.s_addr == myaddr.s_addr) {
            /* ensure we have a table entry */
            if(at = arptnew(isaddr.s_addr, atb, FALSE)) {
                at->at_nud = ARP_NUD_STALE;     /* unsolicited: not confirmed */
                bcopy(sha, (caddr_t)at->at_hwaddr, len);
#if 0 /* unused */
                completed = 1;
#endif
                at->at_flags |= ATF_COM;
            }
        }
        ARPTAB_UNLOCK(atb);
    }

reply:
    /*
     * Reply if this is an IP request
     */
    if(op != ARPOP_REQUEST)
        goto out;

    if(itaddr.s_addr == myaddr.s_addr) {
        /* I am the target, if the address is ready (RFC 5227 2.1.1) */
        if(myia->ia4_flags & IN_IFF_NOTREADY)
            goto out;
        bcopy(sha, tha, len);
        bcopy((caddr_t)ssc->ss_hwaddr, sha, len);
    } else {
        /* Answer if we have a public entry */
        register struct arptab *at;

        /* Try to locate ARP table */
        if(!ssc->ss_arp_table)
            goto out;

        ARPTAB_LOCK(ssc->ss_arp_table);
        at = arptab_look(ssc->ss_arp_table, itaddr.s_addr);
        if(at && (at->at_flags & ATF_PUBL)) {
            bcopy(sha, tha, len);
            bcopy(at->at_hwaddr, sha, len);
        } else {
            at = NULL;
        }
        ARPTAB_UNLOCK(ssc->ss_arp_table);
        if(!at)
            goto out;
    }
    {
        struct sockaddr_sana2 ss2;
        bcopy(spa, tpa, sizeof(struct in_addr));
        bcopy((caddr_t)&itaddr, spa, sizeof(struct in_addr));
        s2a->arp_op = htons(ARPOP_REPLY);

        ss2.ss2_len = sizeof(ss2);
        ss2.ss2_family = AF_UNSPEC;
        ss2.ss2_type = ssc->ss_arp.type;
        bcopy(tha, ss2.ss2_host, len);

        m->m_flags &= ~(M_BCAST | M_MCAST);

        if(PFIL_HOOKED(&arp_pfil, PFIL_OUT)) {
            struct mbuf *hm = m;

            if(pfil_run(&arp_pfil, &hm, &ssc->ss_if, PFIL_OUT) != 0)
                return;         /* dropped by a hook, and freed */
            m = hm;
        }
        (*ssc->ss_if.if_output)(&ssc->ss_if, m, (struct sockaddr *)&ss2,
                                (struct rtentry *)0);
        return;
    }
out:
    m_freem(m);
    return;
}

static int arpioctl_locked(int cmd, caddr_t data);

/*
 * The daemon takes spl before ARPTAB_LOCK, and arptfree() frees at splimp,
 * so the ioctls must hold spl first too or the two orders deadlock.
 */
int
arpioctl(int cmd, caddr_t data)
{
    spl_t s = splimp();
    int error = arpioctl_locked(cmd, data);

    splx(s);
    return (error);
}

static int
arpioctl_locked(cmd, data)
int cmd;
caddr_t data;
{
    register struct arpreq *ar = (struct arpreq *)data;
    register struct arptab *at;
    register struct sockaddr_in *sin;
    struct arptable *atb;
    struct ifaddr *ifa;
    struct sana_softc *ssc;
    spl_t s;

    sin = (struct sockaddr_in *)&ar->arp_pa;
    sin->sin_len = sizeof(ar->arp_pa);

    if(ar->arp_pa.sa_family != AF_INET ||
            ar->arp_ha.sa_family != AF_UNSPEC)
        return (EAFNOSUPPORT);

    /*
     * SIOCGARPT with an unspecified (INADDR_ANY) address is a request
     * to dump *every* interface's ARP table - this is how "ip neigh"
     * reads the IPv4 cache.  The per-interface lookup further down maps
     * a real address to one interface via ifa_ifwithnet(), which cannot
     * resolve 0.0.0.0 (it would fail with ENETUNREACH and the cache
     * would never be shown), so walk the whole SANA interface list here.
     */
    if(cmd == SIOCGARPT && sin->sin_addr.s_addr == INADDR_ANY) {
        register struct arptabreq *atr = (struct arptabreq *)data;
        struct sana_softc *ssc2;
        struct arptable *atb2;
        int i, n = 0;
        long siz;

        ar = atr->atr_table;
        siz = ar ? atr->atr_size : 0;

        for(ssc2 = ssq; ssc2 != NULL; ssc2 = ssc2->ss_next) {
            if((atb2 = ssc2->ss_arp_table) == NULL)
                continue;

            ARPTAB_LOCK(atb2);
            for(i = 0; i < ARPTAB_HSIZE; i++) {
                for(at = (struct arptab *)atb2->atb_entries[i].mlh_Head;
                        at->at_succ; at = at->at_succ) {
                    n++;
                    if(siz > 0) {
                        struct sockaddr_in *asin =
                            (struct sockaddr_in *)&ar->arp_pa;
                        asin->sin_len = sizeof(*asin);
                        asin->sin_family = AF_INET;
                        asin->sin_addr = at->at_iaddr;
                        bcopy((caddr_t)at->at_hwaddr,
                              (caddr_t)ar->arp_ha.sa_data,
                              ar->arp_ha.sa_len =
                                  ssc2->ss_if.if_addrlen + 2);
                        ar->arp_flags = at->at_flags;
                        siz--;
                        ar++;
                    }
                }
            }
            ARPTAB_UNLOCK(atb2);
        }
        atr->atr_size -= siz;
        atr->atr_inuse = n;
        return 0;
    }

    s = splimp();
    if((ifa = ifa_ifwithnet(&ar->arp_pa)) == NULL) {
        splx(s);
        return (ENETUNREACH);
    }

    ssc = (struct sana_softc *)ifa->ifa_ifp;
    splx(s);

    /*if (ssc->ss_if.if_type != IFT_SANA || !(atb = ssc->ss_arp_table)) {*/
    if(ssc->ss_if.if_output != sana_output)
        return (EAFNOSUPPORT);
    /* made when first needed; we are at splimp (arpioctl()) */
    if(ssc->ss_arp_table == NULL && (ssc->ss_if.if_flags & IFF_NOARP) == 0)
        alloc_arptable(ssc, 0);
    if(!(atb = ssc->ss_arp_table)) {
        return (EAFNOSUPPORT);
    }

    ARPTAB_LOCK(atb);

    if(cmd != SIOCGARPT) {
        at = arptab_look(atb, sin->sin_addr.s_addr);
        if(at == NULL && cmd != SIOCSARP) {
            ARPTAB_UNLOCK(atb);
            return (ENXIO);
        }
    }

    switch(cmd) {

    case SIOCSARP:		/* set entry */
        if(ar->arp_ha.sa_len > sizeof(at->at_hwaddr) + 2 ||
                ar->arp_ha.sa_len != ssc->ss_if.if_addrlen + 2) {
            ARPTAB_UNLOCK(atb);
            return (EINVAL);
        }
        /*
         * Free if new entry should be allocated in a different way
         */
        if(at != NULL && (at->at_flags ^ ar->arp_flags) & ATF_PERM) {
            arptfree(atb, at);
            at = NULL;
        }
        if(at == NULL) {
            at = arptnew(sin->sin_addr.s_addr, atb, ar->arp_flags & ATF_PERM);
            if(at == NULL) {
                ARPTAB_UNLOCK(atb);
                return (EADDRNOTAVAIL);
            }
        }

        bcopy((caddr_t)ar->arp_ha.sa_data, (caddr_t)at->at_hwaddr,
              ar->arp_ha.sa_len - 2);
        at->at_flags = ATF_COM | ATF_INUSE |
                       (ar->arp_flags & (ATF_PERM | ATF_PUBL));
        at->at_timer = 0;
        break;

    case SIOCDARP:		/* delete entry */
        arptfree(atb, at);
        break;

    case SIOCGARP:		/* get entry */
        bcopy((caddr_t)at->at_hwaddr, (caddr_t)ar->arp_ha.sa_data,
              ar->arp_ha.sa_len = ssc->ss_if.if_addrlen + 2);
        ar->arp_flags = at->at_flags;
        break;

    case SIOCGARPT: {	/* get table */
        int i, n;
        long siz;
        register struct arptabreq *atr = (struct arptabreq *)data;
        ar = atr->atr_table;
        siz = ar ? atr->atr_size : 0;

        for(n = i = 0; i < ARPTAB_HSIZE; i++) {
            for(at = (struct arptab *)atb->atb_entries[i].mlh_Head;
                    at->at_succ;
                    at = at->at_succ) {
                n++;
                if(siz > 0) {
                    struct sockaddr_in *sin = (struct sockaddr_in *)&ar->arp_pa;
                    sin->sin_len = sizeof(*sin);
                    sin->sin_family = AF_INET;
                    sin->sin_addr = at->at_iaddr;
                    bcopy((caddr_t)at->at_hwaddr, (caddr_t)ar->arp_ha.sa_data,
                          ar->arp_ha.sa_len = ssc->ss_if.if_addrlen + 2);
                    ar->arp_flags = at->at_flags;
                    siz--;
                    ar++;
                }
            }
        }
        atr->atr_size -= siz;
        atr->atr_inuse = n;
    }
    }

    ARPTAB_UNLOCK(atb);
    return 0;
}

static const char *digits = "0123456789ABCDEF";
/*
 * Print Hardware Address
 */
static char *sana_sprintf(register u_char *ap, int len)
{
    register int i;
    static char addrbuf[17 * 3];
    register unsigned char *cp = addrbuf;

    for(i = 0; i < len;) {
        *cp++ = digits[*ap >> 4];
        *cp++ = digits[*ap++ & 0xf];
        i++;
        if(i < len)
            *cp++ = ':';
    }
    *cp = 0;
    return (addrbuf);
}
