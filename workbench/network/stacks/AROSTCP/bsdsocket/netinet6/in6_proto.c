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
 * in6_proto.c - IPv6 protocol domain and switch table, IPv6 as a module of
 * the stack.
 *
 * Defines the inet6 domain and the inet6sw[] protocol switch: IPv6 itself,
 * ICMPv6, raw IPv6 - and an entry for each transport that is registered
 * (netinet/inet_transport.h). This file names none of them.
 */

#include <conf.h>

#if INET6

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/errno.h>
#include <sys/synch.h>
#include <stdarg.h>

#include <net/if.h>
#include <net/netmodule.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_pcb.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>
#include <netinet6/mld6.h>
#include <netinet/inetfamily.h>
#include <netinet/inet_transport.h>
#include <kern/amiga_dhcp.h>
#include <kern/amiga_netdb.h>       /* ndb_ra_flush() */
#include <sys/syslog.h>
#include <net/if_sana.h>
#include <net/if_stf.h>
#include <net/pfil.h>
#include <net/netcore_map.h>        /* always the last include */

/*
 * IPv6's packet hook point (net/pfil.h), tested in ip6_input(),
 * ip6_forward() and ip6_output(). Registered while IPv6 runs.
 */
struct pfil_point in6_pfil = { MIAMIPFBPT_IP6, AF_INET6 };

/*
 * The transport module's interface (netinet/inet_transport.h), fetched
 * once when IPv6 starts: how IPv6 binds to the transports and registers
 * its instance of the family interface without being linked with them.
 */
const struct inet_transport_if *in6_transports = NULL;

#if DHCP6
/* IPv6 DHCP client state - one global instance for the IPv6 protocol family */
struct aros_dhcp_state aros_dhcpv6;
#endif

/*
 * Per-socket flag requesting that the received hop limit be delivered as
 * an IPV6_HOPLIMIT ancillary message (RFC 3542).  Defined defensively so
 * this remains consistent with IN6P_PKTINFO in <netinet6/in6_var.h>
 * without requiring that header to be modified.
 */
#ifndef IN6P_HOPLIMIT
#define IN6P_HOPLIMIT	0x02		/* receive hop limit (IPV6_HOPLIMIT) */
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

/* ctloutput: old BSD 5-arg signature */
int ip6_ctloutput(int op, struct socket *so, int level, int optname,
                  struct mbuf **m);

extern const struct inetfamily in6_family;      /* netinet6/in6_family.c */
extern struct protodispatch ip6_protos;         /* netinet6/ip6_input.c */
extern struct domain inet6domain;


static void in6_transport_input(void *arg, ...);
static void in6_ctlinput_all(int cmd, struct sockaddr *sa, void *arg);
static void in6_raw_input(void *arg, ...);

/*
 * The protocol table has three parts (as IPv4's, netinet/in_proto.c):
 *
 *   inet6sw[0]                   IPv6 itself: output, and the control input
 *                                of pfctlinput(), which it passes to every
 *                                transport bound to IPv6;
 *   inet6sw[IN6SW_TR ...]        IN6_TRSLOTS entries for the transports,
 *                                empty until a transport is bound to IPv6
 *                                (in6_transport_add());
 *   inet6sw[IN6SW_OWN ...]       IPv6's own protocols: ICMPv6, the raw
 *                                protocol (IN6SW_RAW: where the next-header
 *                                values nobody took go, in6_defaultproto)
 *                                and the wildcard. Their sockets are raw
 *                                sockets: socket type, pr_output,
 *                                pr_ctloutput and pr_usrreq are filled in
 *                                from the raw sockets' transport when that
 *                                is bound.
 *
 * The array cannot grow - sockets point into it - so the number of slots
 * is the number of transports IPv6 can be served by at one time.
 */
#define IN6_TRSLOTS 4
#define IN6SW_TR    1
#define IN6SW_OWN   (IN6SW_TR + IN6_TRSLOTS)
#define IN6SW_RAW   (IN6SW_OWN + 1)

#define IN6_EMPTYSLOT \
    { 0, &inet6domain, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }

struct protosw inet6sw[] = {
    /* base IPv6 entry: handles outbound path and timers */
    {
        0,            &inet6domain,   0,              0,
        NULL,         ip6_output,     in6_ctlinput_all, NULL,
        NULL,
        ip6_init,     NULL,           NULL,           NULL,
    },
    IN6_EMPTYSLOT, IN6_EMPTYSLOT, IN6_EMPTYSLOT, IN6_EMPTYSLOT,
    /* ICMPv6; its raw sockets are raw sockets like any other */
    {
        0,            &inet6domain,   IPPROTO_ICMPV6, PR_ATOMIC | PR_ADDR,
        icmp6_input,  NULL,           NULL,           NULL,
        NULL,
        icmp6_init,   icmp6_fasttimo, icmp6_slowtimo, icmp6_drain,
    },
    /* raw IPv6, and the input of protocols with no handler of their own */
    {
        0,            &inet6domain,   IPPROTO_RAW,    PR_ATOMIC | PR_ADDR,
        in6_raw_input, NULL,          NULL,           NULL,
        NULL,
        NULL,         NULL,           NULL,           NULL,
    },
    /* raw IPv6 wildcard */
    {
        0,            &inet6domain,   0,              PR_ATOMIC | PR_ADDR,
        in6_raw_input, NULL,          NULL,           NULL,
        NULL,
        NULL,         NULL,           NULL,           NULL,
    },
};

#define IN6SW_END   (&inet6sw[sizeof(inet6sw) / sizeof(inet6sw[0])])

struct domain inet6domain = {
    AF_INET6, "internet6", NULL, NULL, NULL,
    inet6sw, IN6SW_END, NULL
};

/* where ip6_init() sends the next-header values nobody took */
struct protosw *const in6_defaultproto = &inet6sw[IN6SW_RAW];

/*
 * The transports bound to IPv6: in6_trslot[i] is the one whose entry is
 * inet6sw[IN6SW_TR + i]; in6_trbyx[] has the same descriptors by their
 * protocol numbers' places in IPv6's dispatch table
 * (PROTODISPATCH_INDEX()), which is how the input adapter finds the
 * transport of a packet without a search. in6_started: ip6_init() has
 * run, the dispatch table exists.
 */
#define IN6_TRXMAX  16
static const struct inet_transport *in6_trslot[IN6_TRSLOTS];
static const struct inet_transport *in6_trbyx[IN6_TRXMAX];
static const struct inet_transport *in6_rawtr;
static int in6_started;

/* the raw sockets' hand-over of a packet (the bound raw transport's
 * tr_deliver); while the running stack has no raw sockets, nobody takes it */
static int
in6_noraw(struct mbuf *m, int off, int proto, const struct inetfamily *inf, int flags)
{
    m_freem(m);
    return (0);
}

static int (*in6_rawhandoff)(struct mbuf *, int, int, const struct inetfamily *, int) =
    in6_noraw;

/*
 * A packet for the raw sockets: of a next-header value the stack has no
 * handler for, or an ICMPv6 message once the kernel is done with it
 * (icmp6_input(), which has verified its checksum). m starts with the
 * IPv6 header; off is where the nxt header begins. Takes m.
 */
void
in6_raw_deliver(struct mbuf *m, int off, int nxt)
{
    if(m->m_len < (int)sizeof(struct ip6_hdr) &&
            (m = m_pullup(m, sizeof(struct ip6_hdr))) == NULL)
        return;
    if(nxt == IPPROTO_ICMPV6 && m->m_pkthdr.len < off + 1) {
        m_freem(m);                     /* no type to filter by */
        return;
    }
    (void)(*in6_rawhandoff)(m, off, nxt, &in6_family,
                            nxt == IPPROTO_ICMPV6 ? RIP_CKSUMOK : 0);
}

/* pr_input of the raw IPv6 protocols: in6_raw_input(m, off, nxt) */
static void
in6_raw_input(void *args, ...)
{
    va_list va;
    int off, nxt;

    va_start(va, args);
    off = va_arg(va, int);
    nxt = va_arg(va, int);
    va_end(va);
    in6_raw_deliver((struct mbuf *)args, off, nxt);
}

/*
 * pr_input of every transport's entry: in6_transport_input(m, off, nxt),
 * as ip6_input() calls a protocol - m starts at the IPv6 header, the
 * transport header is at off, after any extension headers. The transport
 * is the one that has the next-header value (ip6_input() has just
 * dispatched on it); it is handed the packet with IPv6's instance of the
 * family interface. (TRF_BAREHDR asks nothing of IPv6: its header has no
 * options, and off is past the extension headers.)
 */
static void
in6_transport_input(void *arg, ...)
{
    va_list va;
    int off, nxt;

    va_start(va, arg);
    off = va_arg(va, int);
    nxt = va_arg(va, int);
    va_end(va);
    (*in6_trbyx[PROTODISPATCH_INDEX(&ip6_protos, nxt)]->tr_input)
        ((struct mbuf *)arg, off, &in6_family);
}

/*
 * Tell a transport what ICMPv6 said (struct inctlparam,
 * netinet/inetfamily.h): about the destination `sa' alone when ip6cp is
 * NULL, else about the packet of ours an error quoted
 * (icmp6_notify_error()).
 */
static void
in6_ctlnotify(const struct inet_transport *tr, int cmd, struct sockaddr *sa,
    struct ip6ctlparam *ip6cp)
{
    struct inctlparam cp;

    if(tr->tr_ctlnotify == NULL)
        return;
    bzero(&cp, sizeof(cp));
    cp.icp_family = &in6_family;
    cp.icp_dst = sa;
    cp.icp_faddr = &((struct sockaddr_in6 *)sa)->sin6_addr;
    if(ip6cp != NULL) {
        cp.icp_laddr = &ip6cp->ip6c_src;
        cp.icp_fport = ip6cp->ip6c_dport;
        cp.icp_lport = ip6cp->ip6c_sport;
        cp.icp_seq = ip6cp->ip6c_seq;
        cp.icp_haveseq = ip6cp->ip6c_haveseq;
        cp.icp_mtu = ip6cp->ip6c_mtu;
    }
    (*tr->tr_ctlnotify)(cmd, &cp);
}

/*
 * For icmp6_notify_error(): what the transport that has next-header value
 * nxt says about its header (TRF_PORTS, TRF_SEQ: what of it an error's
 * quote is read for); 0 when the value is no bound transport's.
 */
int
in6_transport_flags(int nxt)
{
    int x = PROTODISPATCH_INDEX(&ip6_protos, nxt);

    return (x < IN6_TRXMAX && in6_trbyx[x] != NULL ? in6_trbyx[x]->tr_flags : 0);
}

/*
 * ... and the error itself, for the transport of the quoted packet.
 * Returns 0 when nxt is no bound transport's: the caller then looks for
 * another protocol that has the value.
 */
int
in6_transport_ctlinput(int nxt, int cmd, struct sockaddr *sa, struct ip6ctlparam *ip6cp)
{
    int x = PROTODISPATCH_INDEX(&ip6_protos, nxt);

    if(x >= IN6_TRXMAX || in6_trbyx[x] == NULL)
        return (0);
    in6_ctlnotify(in6_trbyx[x], cmd, sa, ip6cp);
    return (1);
}

/*
 * pr_ctlinput of IPv6's own entry: something happened to an address
 * (pfctlinput(): an interface went down) - every transport bound to IPv6
 * is told, in the order of the table.
 */
static void
in6_ctlinput_all(int cmd, struct sockaddr *sa, void *arg)
{
    int i;

    /* pfctlinput() calls every protocol with addresses of every family */
    if(sa == NULL || sa->sa_family != AF_INET6)
        return;
    for(i = 0; i < IN6_TRSLOTS; i++)
        if(in6_trslot[i] != NULL)
            in6_ctlnotify(in6_trslot[i], cmd, sa, (struct ip6ctlparam *)NULL);
}

/*
 * Give the transport of slot i its next-header value in IPv6's dispatch
 * table, and remember it by the value's place there.
 */
static int
in6_transport_dispatch(int i)
{
    const struct inet_transport *tr = in6_trslot[i];
    struct protosw *pr = &inet6sw[IN6SW_TR + i];
    int error, x;

    if((error = protodispatch_set(&ip6_protos, tr->tr_protocol, pr)) != 0)
        return (error);
    x = PROTODISPATCH_INDEX(&ip6_protos, tr->tr_protocol);
    if(x >= IN6_TRXMAX) {
        (void)protodispatch_clear(&ip6_protos, tr->tr_protocol, pr);
        return (ENOSPC);
    }
    in6_trbyx[x] = tr;
    return (0);
}

/*
 * A transport is registered (netinet/inet_transport.h): make it part of
 * IPv6. Called for each one when IPv6 starts (before ip6_init(), which
 * then finds the entries in the table), and later for a transport that
 * is loaded while IPv6 runs.
 */
static int
in6_transport_add(const struct inet_transport *tr)
{
    struct protosw *pr;
    spl_t s;
    int i, error = 0;

    if(tr->tr_flags & TRF_RAW) {
        if(in6_rawtr != NULL)
            return (EEXIST);
        s = splnet();
        for(pr = &inet6sw[IN6SW_OWN]; pr < IN6SW_END; pr++) {
            pr->pr_flags = tr->tr_prflags;
            pr->pr_output = tr->tr_output;
            pr->pr_ctloutput = tr->tr_ctloutput;
            pr->pr_usrreq = tr->tr_usrreq;
            pr->pr_type = tr->tr_type;      /* last: socket() finds it now */
        }
        in6_rawtr = tr;
        in6_rawhandoff = tr->tr_deliver;
        splx(s);
        return (0);
    }

    /* the value may be one of IPv6's own protocols, or a transport's */
    for(pr = inet6sw; pr < IN6SW_END; pr++)
        if(pr->pr_protocol == tr->tr_protocol)
            return (EEXIST);
    for(i = 0; i < IN6_TRSLOTS && in6_trslot[i] != NULL; i++)
        ;
    if(i == IN6_TRSLOTS)
        return (ENOSPC);
    pr = &inet6sw[IN6SW_TR + i];

    s = splnet();
    in6_trslot[i] = tr;
    pr->pr_protocol = tr->tr_protocol;
    pr->pr_flags = tr->tr_prflags;
    pr->pr_input = in6_transport_input;
    /* (no pr_ctlinput: icmp6_notify_error() reaches the transport through
     * in6_transport_ctlinput(), pfctlinput() through IPv6's own entry) */
    /* a transport without socket options of its own: its sockets have IPv6's */
    pr->pr_ctloutput = tr->tr_ctloutput ? tr->tr_ctloutput : ip6_ctloutput;
    pr->pr_usrreq = tr->tr_usrreq;
    if(in6_started && (error = in6_transport_dispatch(i)) != 0) {
        static const struct protosw empty = IN6_EMPTYSLOT;

        *pr = empty;
        in6_trslot[i] = NULL;
    } else
        pr->pr_type = tr->tr_type;          /* last: socket() finds it now */
    splx(s);
    return (error);
}

/*
 * A transport is taken back; no socket of it exists. Its packets go to
 * the raw protocol again, like those of any value nobody has.
 */
static void
in6_transport_remove(const struct inet_transport *tr)
{
    static const struct protosw empty = IN6_EMPTYSLOT;
    struct protosw *pr;
    spl_t s = splnet();
    int i, x;

    if(tr == in6_rawtr) {
        for(pr = &inet6sw[IN6SW_OWN]; pr < IN6SW_END; pr++) {
            pr->pr_type = 0;
            pr->pr_flags = PR_ATOMIC | PR_ADDR;     /* as the table has them */
            pr->pr_output = NULL;
            pr->pr_ctloutput = NULL;
            pr->pr_usrreq = NULL;
        }
        in6_rawhandoff = in6_noraw;
        in6_rawtr = NULL;
    } else
        for(i = 0; i < IN6_TRSLOTS; i++) {
            if(in6_trslot[i] != tr)
                continue;
            pr = &inet6sw[IN6SW_TR + i];
            if(in6_started) {
                x = PROTODISPATCH_INDEX(&ip6_protos, tr->tr_protocol);
                if(protodispatch_clear(&ip6_protos, tr->tr_protocol, pr) == 0 &&
                        x < IN6_TRXMAX)
                    in6_trbyx[x] = NULL;
            }
            *pr = empty;
            in6_trslot[i] = NULL;
        }
    splx(s);
}

static const struct inet_trbinding in6_trbinding = {
    AF_INET6, in6_transport_add, in6_transport_remove
};

/*
 * IPv6 as a module of the stack (net/netmodule.h): the one descriptor of
 * the group, whose start makes every registration of IPv6, in this order -
 *
 *  0. netiface_fetch(): the transport module's interface, by name - the
 *     one way IPv6 calls the transports' registry (ti_bind,
 *     ti_family_register); kept in in6_transports for as long as IPv6
 *     runs;
 *  1. ti_bind (inet_transport_bind()): the registered transports become
 *     entries of inet6sw[] (in6_transport_add()), and one registered later
 *     is added the same way;
 *  2. domain_attach(&inet6domain): links the domain (sockets of AF_INET6
 *     can be made, the entries' timers run) and calls the protocols' init
 *     functions in table order -
 *     ip6_init()   - the dispatch table of the next-header values, from
 *                    the table as step 1 left it, published for the
 *                    protocols of other modules (protodispatch_publish());
 *                    neighbour discovery's own state (nd6_init()); the
 *                    input queue (netinput_register()); IPv6's part in the
 *                    life of an interface and its address requests
 *                    (netfamily_ifops_register(&in6_ifops)); IPv6's
 *                    instance of the family interface
 *                    (inetfamily_register()); neighbour discovery as the
 *                    resolver of the SANA-II interfaces
 *                    (sana_linktype_resolver()); IPv6's statistics
 *                    (netvar_register());
 *     icmp6_init() - MLD's own state, nothing registered;
 *  3. the bound transports are noted by their next-header values' places
 *     in the dispatch table (in6_transport_dispatch());
 *  4. stf_attach(): the 6in4 tunnel, which is part of this module, makes
 *     itself known as a kind of tunnel interface (nettunnel_register()).
 *
 * The descriptor is IPv6's own and makes itself known to the core; the
 * core does not name it. When IPv6 is a library file, the library hands
 * the descriptor to the stack instead (NetModuleAttach()).
 */
static int
inet6_start(struct netmodule *nm)
{
    int error, i;

    /* the transports run - the registry started them for us (inet6_requires) */
    in6_transports = (const struct inet_transport_if *)
        netiface_fetch(nm, INET_TRANSPORT_IFNAME, INET_TRANSPORT_IFVERSION,
                       sizeof(struct inet_transport_if));
    if(in6_transports == NULL)
        return (ENOENT);
    if((error = (*in6_transports->ti_bind)(&in6_trbinding)) != 0)
        return (error);
    if((error = domain_attach(&inet6domain)) != 0) {
        /* (a domain of this family is attached already) */
        (void)(*in6_transports->ti_unbind)(&in6_trbinding);
        for(i = 0; i < IN6_TRSLOTS; i++)
            if(in6_trslot[i] != NULL)
                in6_transport_remove(in6_trslot[i]);
        if(in6_rawtr != NULL)
            in6_transport_remove(in6_rawtr);
        return (error);
    }
    for(i = 0; i < IN6_TRSLOTS; i++)
        if(in6_trslot[i] != NULL)
            (void)in6_transport_dispatch(i);    /* ip6_init() took the values */
    in6_started = 1;
    /* packets may be hooked from the first one on (a hook waits in the core till then) */
    in6_pfil.pp_owner = nm;
    (void)pfil_point_register(&in6_pfil);
    stf_attach();
    /*
     * Started while the stack runs: the interfaces that are online get
     * their IPv6 read requests now (a starting stack sends them when an
     * interface comes online), each interface that is up its link-local
     * address and neighbour discovery state, and the loopback its
     * address. When the stack itself starts, no interface is online or
     * up yet and these do nothing.
     */
    (void)sana_linktype_resume(SANA_PT_IP6);
    in6_attach_running();
    return (0);
}

/* the transports (netinet/inet_transport.c) are started first, once */
static const char *const inet6_requires[] = { "transport", NULL };

/*
 * What IPv6 hangs on a PCB that may be another family's (ti_family_pcbs,
 * netinet/inet_transport.h): its multicast options, with the groups the
 * socket joined - a socket's options are found by their level, so a TCP
 * socket of AF_INET can have set them.
 */
static int
in6_pcbholds(const struct inpcb *inp)
{
    return (inp->in6p_moptions != NULL);
}

/*
 * WHAT KEEPS IPv6 FROM STOPPING (nm_can_stop; nothing is changed). The
 * registry has refused already while a socket of AF_INET6 exists
 * (inet6domain.dom_refs: a connection in TIME_WAIT is still a socket) or
 * another module uses IPv6 (nm_refs), and when IPv6 is the last family
 * that runs. Here, each with its reason in the log, EBUSY:
 *
 *   - a PCB of the transports is IPv6's though its socket is not, or
 *     carries IPv6's multicast options: it would be left pointing at
 *     IPv6's instance of the family interface, or at group records IPv6
 *     is about to free;
 *   - a protocol of another module still has one of IPv6's next-header
 *     values (protodispatch_set() on the published table).
 *
 * (One more refusal can only be seen when the stop is tried, and is made
 * there before anything is changed: inet6_stop().)
 */
static int
inet6_can_stop(struct netmodule *nm)
{
    long n;
    int i;

    if(in6_transports != NULL &&
            (n = (*in6_transports->ti_family_pcbs)(&in6_family, in6_pcbholds)) != 0) {
        __log(LOG_NOTICE, "inet6 cannot stop: %ld socket(s) of another family still "
              "use IPv6 (IPv6 multicast options set on one).", n);
        return (EBUSY);
    }
    for(i = 1; in6_started && i < ip6_protos.pd_n; i++) {
        struct protosw *pr = ip6_protos.pd_tab[i];

        if(pr != NULL && (pr < inet6sw || pr >= IN6SW_END)) {
            __log(LOG_NOTICE, "inet6 cannot stop: next-header value %ld is held by a "
                  "protocol of another module.", (long)pr->pr_protocol);
            return (EBUSY);
        }
    }
    return (0);
}

/*
 * THE STOP OF IPv6: everything inet6_start() and the init functions it
 * had called did, and everything IPv6 made since, is undone. Runs in the
 * stack's task with the socket calls fenced (net/netmodule.h): no timer
 * of IPv6's is running, no packet is on its way through ip6intr(), and no
 * application is inside the stack. In this order -
 *
 *  0. WHAT CAN STILL REFUSE, before anything is changed: the DNS servers
 *     and search domains learned from Router Advertisements are in the
 *     core's dynamic name-server database, which only IPv6's timer ends
 *     (ndb_ra_timer()); they go now (ndb_ra_flush()). An application may
 *     hold that database locked for as long as it likes, and the stack's
 *     task must not wait for it: then the stop is refused, EBUSY, with
 *     the reason in the log, and can be tried again;
 *  1. THE TUNNELS (stf_detach()): the 6in4 tunnel is part of this module.
 *     While both families still work, the kind of tunnel is unregistered,
 *     every tunnel interface is taken out of the stack and freed, IPv4
 *     gets IP protocol 41 back and is no longer held by the tunnel;
 *  2. NO NEW INPUT: the read requests of the IPv6 link type are taken
 *     back from every driver (sana_linktype_quiesce()); the input queue
 *     is unregistered and what waited on it dropped (ip6_input_stop());
 *  3. NO NEW SOCKET, NO TIMER: domain_detach(&inet6domain). The timers of
 *     neighbour discovery, duplicate address detection, the routers and
 *     prefixes, MLD, reassembly and the temporary addresses all run from
 *     the domain's slow and fast timer, and stop with it;
 *  4. THE INTERFACES (in6_detach_all()): on every one the routers and
 *     prefixes heard on it, the IPv6 routes through it and with them the
 *     neighbour cache entries and the packets held in them, its IPv6
 *     addresses (::1 on the loopback among them) with their duplicate
 *     address detection and solicited-node groups, its neighbour
 *     discovery state; then the remaining group memberships, of which
 *     the devices are told;
 *  5. WHAT IS GLOBAL: every route left in IPv6's table and the path MTU
 *     slots holding one, the reassembly queues (ip6_flush()); what is
 *     left of the router, route and prefix lists (nd6_rtr_flush()), of
 *     the duplicate address detection queue (nd6_dad_flush()) and of the
 *     neighbour cache (nd6_deinit()) - nothing, unless something went
 *     wrong before;
 *  6. THE REGISTRATIONS: IPv6's instance of the family interface
 *     (ti_family_unregister, which also drops the half-open TCP
 *     connections that came in by IPv6); the binding to the transports
 *     and each transport's entry of inet6sw[] with its next-header value;
 *     neighbour discovery as the SANA-II interfaces' resolver, the
 *     interface events and address requests, the statistics, the
 *     published dispatch table and its memory (ip6_deinit());
 *  7. THE VARIABLES of IPv6's files, as the files have them.
 *
 * AFTERWARDS THE CORE HOLDS NO POINTER INTO IPv6: no domain or protosw (3;
 * no socket of the domain existed), no input queue or handler (2), no
 * resolver (6), no tunnel kind and no tunnel interface, whose output and
 * ioctl functions are this module's (1), no interface address - whose
 * ifa_rtrequest is nd6_rtrequest - on any interface's list, and no route
 * whose rt_ifa is one or whose rt_llinfo is a neighbour cache entry (4,
 * 5), no interface-event, address-query, variable or dispatch
 * registration (6); IPv4's dispatch table no longer has the tunnel's
 * protosw (1); the transports hold no family instance, binding, PCB or
 * SYN-cache entry of IPv6's (6 and inet6_can_stop()). IPv6 has no timer
 * request of its own.
 */
static int
inet6_stop(struct netmodule *nm)
{
    int i, error;

    /* first what can refuse, before anything is changed */
    if(inet6domain.dom_refs != 0)
        return (EBUSY);
    if((error = ndb_ra_flush()) != 0) {
        __log(LOG_NOTICE, "inet6 cannot stop now: an application holds the dynamic "
              "name-server database, in which the servers learned from Router "
              "Advertisements are. Try again.");
        return (error);
    }

    stf_detach();

    (void)sana_linktype_quiesce(SANA_PT_IP6);
    ip6_input_stop();
    /* no packet passes it any more; what is hooked waits in the core */
    (void)pfil_point_unregister(&in6_pfil);

    if((error = domain_detach(&inet6domain)) != 0)
        __log(LOG_ERR, "inet6: its domain would not detach (error %ld).", (long)error);

    (void)in6_detach_all();

    nd6_rtr_flush();
    ip6_flush();
    nd6_dad_flush();
    nd6_deinit();

    (void)(*in6_transports->ti_family_unregister)(&in6_family);
    (void)(*in6_transports->ti_unbind)(&in6_trbinding);
    for(i = 0; i < IN6_TRSLOTS; i++)
        if(in6_trslot[i] != NULL)
            in6_transport_remove(in6_trslot[i]);
    if(in6_rawtr != NULL)
        in6_transport_remove(in6_rawtr);
    ip6_deinit();

    in6_started = 0;
    in6_transports = NULL;      /* not to be used after the stop (net/netmodule.h) */
    for(i = 0; i < IN6_TRXMAX; i++)
        in6_trbyx[i] = NULL;
    inet6domain.dom_next = NULL;
    inet6domain.dom_refs = 0;
    icmp6_defaults();
    mld6_defaults();
    in6_family_defaults();
#if DHCP6
    bzero((caddr_t)&aros_dhcpv6, sizeof(aros_dhcpv6));
#endif
    return (0);
}

/*
 * WHEN THE STACK ENDS and IPv6 was not stopped (nm_shutdown,
 * net/netmodule.h) it has nothing to release itself. Its memory -
 * interface addresses, prefixes, routers, the neighbour cache, multicast
 * records, the reassembly queue, the dispatch table, the tunnel
 * interfaces' structures - is from the core's pool or is mbufs, freed in
 * one piece by the core; its timers run from the protocol switch's fast
 * and slow timers, which the core deletes (timer_deinit()); the SANA-II
 * read requests its packets arrive in are the core's, collected by
 * sana_deinit(). It opens no device or library and has no port, signal or
 * AllocMem() memory. So nm_shutdown is NULL.
 */
NETMODULE_DESCRIPTOR(inet6_module) = {
    sizeof(struct netmodule), NETMODULE_VERSION, "inet6",
    /* selectable: PROTOCOLS says whether IPv6 runs, a module that needs it does not */
    NMF_SELECTABLE, inet6_start, inet6_stop, inet6_can_stop,
    0, NULL, &inet6domain, inet6_requires,
    /*
     * TEST SWITCH (see netmodule_start_builtin()): IPv6 is left out of the
     * running stack when the global variable AROSTCP/WithoutIPv6 exists,
     * whatever the configuration item PROTOCOLS says. The item is how a
     * stack is configured; the switch is for the test harness.
     */
    "AROSTCP/WithoutIPv6",
    NULL,           /* nm_library: the loader's */
    NULL            /* nm_shutdown: nothing outside the core's pools */
};
NETMODULE_BUILTIN(inet6_module, 10)

/* ------------------------------------------------------------------
 * Helper: get or allocate ip6_moptions for a socket.
 * ------------------------------------------------------------------ */
static struct ip6_moptions *
ip6_getmoptions(struct ip6_moptions **imop)
{
    struct ip6_moptions *im6o = *imop;

    if(im6o == NULL) {
        MALLOC(im6o, struct ip6_moptions *, sizeof(*im6o),
               M_SOOPTS, M_WAITOK);
        if(im6o == NULL)
            return NULL;
        im6o->im6o_multicast_ifp  = NULL;
        im6o->im6o_multicast_hlim = IP6_DEFAULT_MULTICAST_HLIM;
        im6o->im6o_multicast_loop = IP6_DEFAULT_MULTICAST_LOOP;
        im6o->im6o_num_memberships = 0;
        *imop = im6o;
    }
    return im6o;
}

/*
 * ip6_freemoptions - release a socket's IPv6 multicast options when it goes
 * away: each group it joined is left (and, with the last reference, MLD Done
 * is sent and the device forgets the address), then the options are freed.
 * The IPv6 counterpart of ip_freemoptions().
 */
void
ip6_freemoptions(struct ip6_moptions *im6o)
{
    int i;

    if(im6o == NULL)
        return;
    for(i = 0; i < im6o->im6o_num_memberships; i++)
        in6_delmulti(im6o->im6o_membership[i]);
    FREE(im6o, M_SOOPTS);
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

    im6o = ip6_getmoptions(&inp->in6p_moptions);
    if(im6o == NULL)
        return ENOBUFS;

    if(im6o->im6o_num_memberships >= IPV6_MAX_MEMBERSHIPS)
        return ETOOMANYREFS;

    /* find interface by index; 0 means any */
    if(mreq->ipv6mr_interface == 0) {
        /* pick first non-loopback interface */
        for(ifp = ifnet_list; ifp; ifp = ifp->if_next) {
            if((ifp->if_flags & (IFF_UP | IFF_LOOPBACK)) == IFF_UP)
                break;
        }
        if(ifp == NULL)
            return EADDRNOTAVAIL;
    } else {
        for(ifp = ifnet_list; ifp; ifp = ifp->if_next)
            if(ifp->if_index == mreq->ipv6mr_interface)
                break;
        if(ifp == NULL)
            return ENXIO;
    }

    /* check not already a member */
    for(int i = 0; i < im6o->im6o_num_memberships; i++) {
        if(im6o->im6o_membership[i]->in6m_ifp == ifp &&
                IN6_ARE_ADDR_EQUAL(&im6o->im6o_membership[i]->in6m_addr,
                                   &mreq->ipv6mr_multiaddr))
            return EADDRINUSE;
    }

    in6m = in6_addmulti(&mreq->ipv6mr_multiaddr, ifp);
    if(in6m == NULL)
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

    if(im6o == NULL)
        return EADDRNOTAVAIL;

    /* find interface */
    if(mreq->ipv6mr_interface == 0) {
        ifp = NULL;
    } else {
        for(ifp = ifnet_list; ifp; ifp = ifp->if_next)
            if(ifp->if_index == mreq->ipv6mr_interface)
                break;
        if(ifp == NULL)
            return ENXIO;
    }

    for(int i = 0; i < im6o->im6o_num_memberships; i++) {
        if((ifp == NULL || im6o->im6o_membership[i]->in6m_ifp == ifp) &&
                IN6_ARE_ADDR_EQUAL(&im6o->im6o_membership[i]->in6m_addr,
                                   &mreq->ipv6mr_multiaddr)) {
            in6_delmulti(im6o->im6o_membership[i]);
            /* compact array */
            im6o->im6o_num_memberships--;
            for(; i < im6o->im6o_num_memberships; i++)
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

    if(level != IPPROTO_IPV6)
        return EINVAL;

    inp = sotoinpcb(so);
    if(inp == NULL)
        return EINVAL;

    switch(op) {

    case PRCO_SETOPT:
        switch(optname) {

        case IPV6_UNICAST_HOPS:
            if(*m == NULL || (*m)->m_len != sizeof(int)) {
                error = EINVAL;
                break;
            }
            inp->in6p_hops = *mtod(*m, int *);
            break;

        case IPV6_V6ONLY:
            if(*m == NULL || (*m)->m_len != sizeof(int)) {
                error = EINVAL;
                break;
            }
            /*
             * Only meaningful before the socket is bound - afterwards the
             * PCB lookups have already been deciding which traffic may
             * reach it (see in_pcbisipv6()).
             */
            if(inp->inp_lport || !IN6_IS_ADDR_UNSPECIFIED(&inp->inp_laddr6)) {
                error = EINVAL;
                break;
            }
            inp->in6p_v6only = (*mtod(*m, int *) != 0);
            break;

        case IPV6_TCLASS: {         /* RFC 3542 section 6.5: -1 is 0 */
            int v;

            if(*m == NULL || (*m)->m_len != sizeof(int)) {
                error = EINVAL;
                break;
            }
            v = *mtod(*m, int *);
            if(v < -1 || v > 255) {
                error = EINVAL;
                break;
            }
            inp->in6p_tclass = v < 0 ? 0 : v;
        }
        break;

        case IPV6_RECVTCLASS:
            if(*m == NULL || (*m)->m_len != sizeof(int)) {
                error = EINVAL;
                break;
            }
            if(*mtod(*m, int *))
                inp->in6p_flags |= IN6P_TCLASS;
            else
                inp->in6p_flags &= ~IN6P_TCLASS;
            break;

        case IPV6_RECVPKTINFO:
            if(*m == NULL || (*m)->m_len != sizeof(int)) {
                error = EINVAL;
                break;
            }
            if(*mtod(*m, int *))
                inp->in6p_flags |= IN6P_PKTINFO;
            else
                inp->in6p_flags &= ~IN6P_PKTINFO;
            break;

        case IPV6_RECVPATHMTU:      /* RFC 3542 section 11 */
        case IPV6_DONTFRAG: {
            int bit = optname == IPV6_DONTFRAG ? IN6P_DONTFRAG : IN6P_RECVPATHMTU;

            if(*m == NULL || (*m)->m_len != sizeof(int)) {
                error = EINVAL;
                break;
            }
            if(*mtod(*m, int *))
                inp->in6p_flags |= bit;
            else
                inp->in6p_flags &= ~bit;
        }
        break;

        case IPV6_USE_MIN_MTU: {
            int v;

            if(*m == NULL || (*m)->m_len != sizeof(int)) {
                error = EINVAL;
                break;
            }
            v = *mtod(*m, int *);
            if(v < -1 || v > 1) {
                error = EINVAL;
                break;
            }
            inp->in6p_flags &= ~(IN6P_MINMTU | IN6P_PMTU);
            if(v == 1)
                inp->in6p_flags |= IN6P_MINMTU;
            else if(v == 0)
                inp->in6p_flags |= IN6P_PMTU;
        }
        break;

        case IPV6_PATHMTU:          /* get only */
            error = ENOPROTOOPT;
            break;

        case IPV6_RECVHOPLIMIT:     /* RFC 3542 */
        case IPV6_HOPLIMIT:         /* RFC 2292, kept for older programs */
            if(*m == NULL || (*m)->m_len != sizeof(int)) {
                error = EINVAL;
                break;
            }
            if(*mtod(*m, int *))
                inp->in6p_flags |= IN6P_HOPLIMIT;
            else
                inp->in6p_flags &= ~IN6P_HOPLIMIT;
            break;

        case IPV6_MULTICAST_IF: {
            unsigned int ifindex;
            struct ifnet *ifp;

            im6o = ip6_getmoptions(&inp->in6p_moptions);
            if(im6o == NULL) {
                error = ENOBUFS;
                break;
            }

            if(*m == NULL || (*m)->m_len != sizeof(unsigned int)) {
                error = EINVAL;
                break;
            }
            ifindex = *mtod(*m, unsigned int *);
            if(ifindex == 0) {
                im6o->im6o_multicast_ifp = NULL;
                break;
            }
            for(ifp = ifnet_list; ifp; ifp = ifp->if_next)
                if(ifp->if_index == ifindex)
                    break;
            if(ifp == NULL) {
                error = ENXIO;
                break;
            }
            im6o->im6o_multicast_ifp = ifp;
        }
        break;

        case IPV6_MULTICAST_HOPS: {
            int hlim;
            im6o = ip6_getmoptions(&inp->in6p_moptions);
            if(im6o == NULL) {
                error = ENOBUFS;
                break;
            }
            if(*m == NULL || (*m)->m_len != sizeof(int)) {
                error = EINVAL;
                break;
            }
            hlim = *mtod(*m, int *);
            if(hlim < -1 || hlim > 255) {
                error = EINVAL;
                break;
            }
            im6o->im6o_multicast_hlim =
                (hlim < 0) ? IP6_DEFAULT_MULTICAST_HLIM : (u_int8_t)hlim;
        }
        break;

        case IPV6_MULTICAST_LOOP: {
            unsigned int loop;
            im6o = ip6_getmoptions(&inp->in6p_moptions);
            if(im6o == NULL) {
                error = ENOBUFS;
                break;
            }
            if(*m == NULL || (*m)->m_len != sizeof(unsigned int)) {
                error = EINVAL;
                break;
            }
            loop = *mtod(*m, unsigned int *);
            if(loop > 1) {
                error = EINVAL;
                break;
            }
            im6o->im6o_multicast_loop = (u_int8_t)loop;
        }
        break;

        case IPV6_JOIN_GROUP:
            if(*m == NULL || (*m)->m_len != sizeof(struct ipv6_mreq)) {
                error = EINVAL;
                break;
            }
            error = in6_joingroup_inp(inp, mtod(*m, struct ipv6_mreq *));
            break;

        case IPV6_LEAVE_GROUP:
            if(*m == NULL || (*m)->m_len != sizeof(struct ipv6_mreq)) {
                error = EINVAL;
                break;
            }
            error = in6_leavegroup_inp(inp, mtod(*m, struct ipv6_mreq *));
            break;

        default:
            error = ENOPROTOOPT;
            break;
        }
        if(*m) {
            m_freem(*m);
            *m = NULL;
        }
        break;

    case PRCO_GETOPT:
        switch(optname) {

        case IPV6_UNICAST_HOPS:
            *m = m_get(M_WAIT, MT_SOOPTS);
            if(*m == NULL) {
                error = ENOBUFS;
                break;
            }
            (*m)->m_len = sizeof(int);
            *mtod(*m, int *) = inp->in6p_hops;
            break;

        case IPV6_V6ONLY:
            *m = m_get(M_WAIT, MT_SOOPTS);
            if(*m == NULL) {
                error = ENOBUFS;
                break;
            }
            (*m)->m_len = sizeof(int);
            *mtod(*m, int *) = inp->in6p_v6only;
            break;

        case IPV6_TCLASS:
            *m = m_get(M_WAIT, MT_SOOPTS);
            if(*m == NULL) {
                error = ENOBUFS;
                break;
            }
            (*m)->m_len = sizeof(int);
            *mtod(*m, int *) = inp->in6p_tclass;
            break;

        case IPV6_RECVPKTINFO:
            *m = m_get(M_WAIT, MT_SOOPTS);
            if(*m == NULL) {
                error = ENOBUFS;
                break;
            }
            (*m)->m_len = sizeof(int);
            *mtod(*m, int *) = (inp->in6p_flags & IN6P_PKTINFO) ? 1 : 0;
            break;

        case IPV6_RECVTCLASS:
            *m = m_get(M_WAIT, MT_SOOPTS);
            if(*m == NULL) {
                error = ENOBUFS;
                break;
            }
            (*m)->m_len = sizeof(int);
            *mtod(*m, int *) = (inp->in6p_flags & IN6P_TCLASS) ? 1 : 0;
            break;

        case IPV6_RECVPATHMTU:
        case IPV6_DONTFRAG:
        case IPV6_USE_MIN_MTU:
            *m = m_get(M_WAIT, MT_SOOPTS);
            if(*m == NULL) {
                error = ENOBUFS;
                break;
            }
            (*m)->m_len = sizeof(int);
            *mtod(*m, int *) =
                optname == IPV6_USE_MIN_MTU ? IN6P_MINMTU_OPT(inp) :
                (inp->in6p_flags & (optname == IPV6_DONTFRAG ? IN6P_DONTFRAG :
                                    IN6P_RECVPATHMTU)) ? 1 : 0;
            break;

        case IPV6_PATHMTU: {        /* RFC 3542 section 11.4 */
            struct ip6_mtuinfo mi;

            if(IN6_IS_ADDR_UNSPECIFIED(&inp->inp_faddr6)) {
                error = ENOTCONN;
                break;
            }
            bzero(&mi, sizeof(mi));
            mi.ip6m_addr.sin6_family = AF_INET6;
            mi.ip6m_addr.sin6_len = sizeof(mi.ip6m_addr);
            mi.ip6m_addr.sin6_addr = inp->inp_faddr6;
            if(inp->inp_boundif != NULL &&
                    (IN6_IS_ADDR_LINKLOCAL(&inp->inp_faddr6) ||
                     IN6_IS_ADDR_MC_LINKLOCAL(&inp->inp_faddr6)))
                mi.ip6m_addr.sin6_scope_id = inp->inp_boundif->if_index;
            mi.ip6m_mtu = ip6_pathmtu(inp);
            if(mi.ip6m_mtu == 0) {
                error = EHOSTUNREACH;
                break;
            }
            *m = m_get(M_WAIT, MT_SOOPTS);
            if(*m == NULL) {
                error = ENOBUFS;
                break;
            }
            (*m)->m_len = sizeof(mi);
            bcopy(&mi, mtod(*m, caddr_t), sizeof(mi));
        }
        break;

        case IPV6_RECVHOPLIMIT:
        case IPV6_HOPLIMIT:
            *m = m_get(M_WAIT, MT_SOOPTS);
            if(*m == NULL) {
                error = ENOBUFS;
                break;
            }
            (*m)->m_len = sizeof(int);
            *mtod(*m, int *) = (inp->in6p_flags & IN6P_HOPLIMIT) ? 1 : 0;
            break;

        case IPV6_MULTICAST_IF:
            *m = m_get(M_WAIT, MT_SOOPTS);
            if(*m == NULL) {
                error = ENOBUFS;
                break;
            }
            (*m)->m_len = sizeof(unsigned int);
            im6o = inp->in6p_moptions;
            *mtod(*m, unsigned int *) =
                (im6o && im6o->im6o_multicast_ifp)
                ? im6o->im6o_multicast_ifp->if_index : 0;
            break;

        case IPV6_MULTICAST_HOPS:
            *m = m_get(M_WAIT, MT_SOOPTS);
            if(*m == NULL) {
                error = ENOBUFS;
                break;
            }
            (*m)->m_len = sizeof(int);
            im6o = inp->in6p_moptions;
            *mtod(*m, int *) = im6o
                               ? (int)im6o->im6o_multicast_hlim
                               : IP6_DEFAULT_MULTICAST_HLIM;
            break;

        case IPV6_MULTICAST_LOOP:
            *m = m_get(M_WAIT, MT_SOOPTS);
            if(*m == NULL) {
                error = ENOBUFS;
                break;
            }
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

