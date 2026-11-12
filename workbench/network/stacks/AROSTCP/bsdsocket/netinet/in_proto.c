/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 Neil Cafferkey
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
 * Copyright (C) 1982, 1986 Regents of the University of California.
 * All rights reserved.
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
 *	@(#)in_proto.c	7.5 (Berkeley) 6/28/90
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/errno.h>
#include <sys/synch.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>

#include <netinet/in_proto_protos.h>
#include <netinet/in_protos.h>
#include <netinet/in_pcb.h>
#include <kern/amiga_dhcp.h>
#include <sys/syslog.h>
#include <net/if.h>
#include <net/netmodule.h>
#include <net/if_sana.h>
#include <net/sana2arp.h>
#include <netinet/inetfamily.h>
#include <netinet/inet_transport.h>
#include <net/pfil.h>

#ifdef ENABLE_MULTICAST
#include <netinet/igmp.h>
#endif

#include <stddef.h>             /* offsetof */
#include <stdarg.h>

/* IPv4 DHCP client state - one global instance for the IPv4 protocol family */
struct aros_dhcp_state aros_dhcpv4;

/*
 * IPv4's two packet hook points (net/pfil.h): IP packets, tested in
 * ip_input(), ip_forward() and ip_output(); ARP packets, in ARP's input
 * and its replies (net/sana2arp.c). Registered while IPv4 runs.
 */
struct pfil_point ip_pfil = { MIAMIPFBPT_IP, AF_INET };
struct pfil_point arp_pfil = { MIAMIPFBPT_ARP, 0 };

/*
 * The IPv4 protocol family: IP, ICMP, IGMP - and whatever transports are
 * registered (netinet/inet_transport.h). This file names none of them.
 */
#include <netinet/ip_icmp_protos.h>
#include <netinet/ip_input_protos.h>
#include <netinet/ip_output_protos.h>
#include <net/netcore_map.h>        /* always the last include */

extern const struct inetfamily in_family;       /* netinet/in_family.c */
extern struct protodispatch ip_protos;          /* netinet/ip_input.c */

/*
 * The transport module's interface (netinet/inet_transport.h), fetched
 * once when IPv4 starts: how IPv4 binds to the transports and registers
 * its instance of the family interface without being linked with them.
 */
const struct inet_transport_if *in_transports = NULL;
extern struct domain inetdomain;

static void in_transport_input(void *arg, ...);
static void in_transport_ctlinput(int cmd, struct sockaddr *sa, void *arg);
static void in_ctlinput_all(int cmd, struct sockaddr *sa, void *arg);
static void in_raw_input(void *arg, ...);

/*
 * The protocol table has three parts:
 *
 *   inetsw[0]                    IP itself: output, timers, and the control
 *                                input of pfctlinput(), which it passes to
 *                                every transport bound to IPv4;
 *   inetsw[INSW_TR ...]          IN_TRSLOTS entries for the transports. Empty
 *                                (no socket type, no protocol number, no
 *                                function: nothing finds them) until a
 *                                transport is bound to IPv4
 *                                (in_transport_add()), which fills one in;
 *   inetsw[INSW_OWN ...]         IPv4's own protocols. The sockets of every
 *                                one of them are raw sockets: their socket
 *                                type, pr_output, pr_ctloutput and pr_usrreq
 *                                are empty here and filled in from the raw
 *                                sockets' transport when that is bound. The
 *                                first of them is the raw protocol, to which
 *                                the packets of numbers nobody took go
 *                                (in_defaultproto), the last the wildcard.
 *
 * The array cannot grow - sockets point into it - so the number of slots
 * is the number of transports IPv4 can be served by at one time: the two
 * the stack has and two loaded later.
 */
#define IN_TRSLOTS  4
#define INSW_TR     1
#define INSW_OWN    (INSW_TR + IN_TRSLOTS)

#define IN_EMPTYSLOT \
    { 0, &inetdomain, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }

struct protosw inetsw[] = {
    {
        0,		&inetdomain,	0,		0,
        NULL,		ip_output,	in_ctlinput_all, NULL,
        NULL,
        ip_init,	NULL,		ip_slowtimo,	ip_drain,
    },
    IN_EMPTYSLOT, IN_EMPTYSLOT, IN_EMPTYSLOT, IN_EMPTYSLOT,
    /* raw IP, and the input of the numbers no protocol took */
    {
        0,		&inetdomain,	IPPROTO_RAW,	PR_ATOMIC | PR_ADDR,
        in_raw_input,	NULL,		NULL,		NULL,
        NULL,
        NULL,		NULL,		NULL,		NULL,
    },
    {
        0,		&inetdomain,	IPPROTO_ICMP,	PR_ATOMIC | PR_ADDR,
        icmp_input,	NULL,		NULL,		NULL,
        NULL,
        NULL,		NULL,		NULL,		NULL,
    },
#ifdef ENABLE_MULTICAST
    {
        0,		&inetdomain,	IPPROTO_IGMP,	PR_ATOMIC | PR_ADDR,
        igmp_input,	NULL,		NULL,		NULL,
        NULL,
        igmp_init,	igmp_fasttimo,	igmp_slowtimo,	NULL,
    },
#endif
    /*
     * (IP protocol 41, IPv6 in IPv4, is not here: the 6in4 tunnel takes
     * the number from IPv4's dispatch table when a tunnel is made,
     * net/if_stf.c.)
     */
    /* raw wildcard */
    {
        0,		&inetdomain,	0,		PR_ATOMIC | PR_ADDR,
        in_raw_input,	NULL,		NULL,		NULL,
        NULL,
        NULL,		NULL,		NULL,		NULL,
    },
};

#define INSW_END    (&inetsw[sizeof(inetsw) / sizeof(inetsw[0])])

struct domain inetdomain = {
    AF_INET, "internet", NULL, NULL, NULL,
    inetsw, INSW_END, NULL
};

/* where ip_init() sends the protocol numbers nobody took */
struct protosw *const in_defaultproto = &inetsw[INSW_OWN];

/*
 * The transports bound to IPv4: in_trslot[i] is the one whose entry is
 * inetsw[INSW_TR + i], and in_trbyx[] has the same descriptors by their
 * protocol numbers' places in IPv4's dispatch table
 * (PROTODISPATCH_INDEX()), which is how the input adapter finds the
 * transport of a packet: one byte and one pointer loaded, no search.
 * in_started: ip_init() has run, the dispatch table exists.
 */
#define IN_TRXMAX   16
static const struct inet_transport *in_trslot[IN_TRSLOTS];
static const struct inet_transport *in_trbyx[IN_TRXMAX];
static const struct inet_transport *in_rawtr;
static int in_started;

/*
 * The raw sockets' hand-over of a packet (the bound raw transport's
 * tr_deliver), for IPv4's own protocols that have dealt with one
 * (icmp_input()) and for in_raw_input(): called as
 * (*in_rawhandoff)(m, hlen, proto, &in_family, 0), m as ip_input() hands a
 * packet to a protocol. While no raw sockets are part of the running
 * stack, nobody takes it.
 */
static int
in_noraw(struct mbuf *m, int off, int proto, const struct inetfamily *inf, int flags)
{
    m_freem(m);
    return (0);
}

int (*in_rawhandoff)(struct mbuf *, int, int, const struct inetfamily *, int) = in_noraw;

/*
 * pr_input of the IPv4 protocols the stack has no handler for:
 * in_raw_input(m, hlen). A packet no raw socket wanted either was for no
 * protocol at all.
 */
static void
in_raw_input(void *arg, ...)
{
    struct mbuf *m = arg;
    va_list va;
    int hlen;

    va_start(va, arg);
    hlen = va_arg(va, int);
    va_end(va);

    if((*in_rawhandoff)(m, hlen, mtod(m, struct ip *)->ip_p, &in_family, 0) == 0) {
        ipstat.ips_noproto++;
        ipstat.ips_delivered--;
    }
}

/*
 * pr_input of every transport's entry: in_transport_input(m, hlen), as
 * ip_input() calls a protocol. The transport is the one that has the
 * packet's protocol number (ip_input() has just dispatched on it, so the
 * number is a bound transport's); it is handed the packet with IPv4's
 * instance of the family interface.
 */
static void
in_transport_input(void *arg, ...)
{
    struct mbuf *m = arg;
    const struct inet_transport *tr;
    va_list va;
    int hlen;

    va_start(va, arg);
    hlen = va_arg(va, int);
    va_end(va);

    tr = in_trbyx[PROTODISPATCH_INDEX(&ip_protos, mtod(m, struct ip *)->ip_p)];
    /*
     * Strip IP options, if any, for a transport that wants none
     * (TRF_BAREHDR); should skip this, make available to user, and use on
     * returned packets, but icmp_error() - should nobody listen on the
     * port - quotes a header without them.
     */
    if(hlen > sizeof(struct ip) && (tr->tr_flags & TRF_BAREHDR)) {
        ip_stripoptions(m, (struct mbuf *)0);
        hlen = sizeof(struct ip);
    }
    (*tr->tr_input)(m, hlen, &in_family);
}

/*
 * Tell a transport what ICMP said (struct inctlparam,
 * netinet/inetfamily.h): about the destination `sa' alone when ip is
 * NULL, else about the packet of ours quoted at ip - its IP header, the
 * transport header behind it, of which icmp_input() has made sure there
 * are eight bytes.
 */
static void
in_ctlnotify(const struct inet_transport *tr, int cmd, struct sockaddr *sa, struct ip *ip)
{
    struct inctlparam cp;

    if(tr->tr_ctlnotify == NULL)
        return;
    bzero(&cp, sizeof(cp));
    cp.icp_family = &in_family;
    cp.icp_dst = sa;
    cp.icp_faddr = &((struct sockaddr_in *)sa)->sin_addr;
    if(ip != NULL) {
        caddr_t th = (caddr_t)ip + (ip->ip_hl << 2);

        cp.icp_faddr = &ip->ip_dst;
        cp.icp_laddr = &ip->ip_src;
        if(tr->tr_flags & TRF_PORTS) {
            cp.icp_lport = ((const u_int16_t *)th)[0];
            cp.icp_fport = ((const u_int16_t *)th)[1];
        }
        if(tr->tr_flags & TRF_SEQ) {
            cp.icp_seq = ntohl(((const u_int32_t *)th)[1]);
            cp.icp_haveseq = 1;
        }
        if(cmd == PRC_MSGSIZE)
            /* icmp_input() has put the path MTU on the route and in the
             * message (icmp_mtudisc()) */
            cp.icp_mtu = ntohs(((const struct icmp *)
                ((caddr_t)ip - offsetof(struct icmp, icmp_ip)))->icmp_nextmtu);
    }
    (*tr->tr_ctlnotify)(cmd, &cp);
}

/*
 * pr_ctlinput of every transport's entry: what icmp_input() calls for the
 * protocol of a packet an error quotes (arg: the quoted IP header).
 * pfctlinput() calls it too, for every entry and without a packet: that
 * is in_ctlinput_all()'s to pass on, once.
 */
static void
in_transport_ctlinput(int cmd, struct sockaddr *sa, void *arg)
{
    struct ip *ip = arg;
    const struct inet_transport *tr;

    if(ip == NULL || sa == NULL || sa->sa_family != AF_INET)
        return;
    tr = in_trbyx[PROTODISPATCH_INDEX(&ip_protos, ip->ip_p)];
    if(tr != NULL)
        in_ctlnotify(tr, cmd, sa, ip);
}

/*
 * pr_ctlinput of IP's own entry: something happened to an address
 * (pfctlinput(): an interface went down, a redirect) - every transport
 * bound to IPv4 is told, in the order of the table.
 */
static void
in_ctlinput_all(int cmd, struct sockaddr *sa, void *arg)
{
    int i;

    /* pfctlinput() calls every protocol with addresses of every family */
    if(sa == NULL || sa->sa_family != AF_INET)
        return;
    for(i = 0; i < IN_TRSLOTS; i++)
        if(in_trslot[i] != NULL)
            in_ctlnotify(in_trslot[i], cmd, sa, (struct ip *)NULL);
}

/*
 * Give the transport of slot i its protocol number in IPv4's dispatch
 * table, and remember it by the number's place there.
 */
static int
in_transport_dispatch(int i)
{
    const struct inet_transport *tr = in_trslot[i];
    struct protosw *pr = &inetsw[INSW_TR + i];
    int error, x;

    if((error = protodispatch_set(&ip_protos, tr->tr_protocol, pr)) != 0)
        return (error);
    x = PROTODISPATCH_INDEX(&ip_protos, tr->tr_protocol);
    if(x >= IN_TRXMAX) {
        (void)protodispatch_clear(&ip_protos, tr->tr_protocol, pr);
        return (ENOSPC);
    }
    in_trbyx[x] = tr;
    return (0);
}

/*
 * A transport is registered (netinet/inet_transport.h): make it part of
 * IPv4. Called for each one when IPv4 starts (before ip_init(), which
 * then finds the entries in the table), and later for a transport that
 * is loaded while IPv4 runs.
 */
static int
in_transport_add(const struct inet_transport *tr)
{
    struct protosw *pr;
    spl_t s;
    int i, error = 0;

    if(tr->tr_flags & TRF_RAW) {
        if(in_rawtr != NULL)
            return (EEXIST);
        s = splnet();
        for(pr = &inetsw[INSW_OWN]; pr < INSW_END; pr++) {
            pr->pr_flags = tr->tr_prflags;
            pr->pr_output = tr->tr_output;
            pr->pr_ctloutput = tr->tr_ctloutput;
            pr->pr_usrreq = tr->tr_usrreq;
            pr->pr_type = tr->tr_type;      /* last: socket() finds it now */
        }
        in_rawtr = tr;
        in_rawhandoff = tr->tr_deliver;
        splx(s);
        return (0);
    }

    /* the number may be one of IPv4's own protocols, or a transport's */
    for(pr = inetsw; pr < INSW_END; pr++)
        if(pr->pr_protocol == tr->tr_protocol)
            return (EEXIST);
    for(i = 0; i < IN_TRSLOTS && in_trslot[i] != NULL; i++)
        ;
    if(i == IN_TRSLOTS)
        return (ENOSPC);
    pr = &inetsw[INSW_TR + i];

    s = splnet();
    in_trslot[i] = tr;
    pr->pr_protocol = tr->tr_protocol;
    pr->pr_flags = tr->tr_prflags;
    pr->pr_input = in_transport_input;
    pr->pr_ctlinput = in_transport_ctlinput;
    /* a transport without socket options of its own: its sockets have IP's */
    pr->pr_ctloutput = tr->tr_ctloutput ? tr->tr_ctloutput : ip_ctloutput;
    pr->pr_usrreq = tr->tr_usrreq;
    if(in_started && (error = in_transport_dispatch(i)) != 0) {
        static const struct protosw empty = IN_EMPTYSLOT;

        *pr = empty;
        in_trslot[i] = NULL;
    } else
        pr->pr_type = tr->tr_type;          /* last: socket() finds it now */
    splx(s);
    return (error);
}

/*
 * A transport is taken back; no socket of it exists. Its packets go to
 * the raw protocol again, like those of any number nobody has.
 */
static void
in_transport_remove(const struct inet_transport *tr)
{
    static const struct protosw empty = IN_EMPTYSLOT;
    struct protosw *pr;
    spl_t s = splnet();
    int i, x;

    if(tr == in_rawtr) {
        for(pr = &inetsw[INSW_OWN]; pr < INSW_END; pr++) {
            pr->pr_type = 0;
            pr->pr_flags = PR_ATOMIC | PR_ADDR;     /* as the table has them */
            pr->pr_output = NULL;
            pr->pr_ctloutput = NULL;
            pr->pr_usrreq = NULL;
        }
        in_rawhandoff = in_noraw;
        in_rawtr = NULL;
    } else
        for(i = 0; i < IN_TRSLOTS; i++) {
            if(in_trslot[i] != tr)
                continue;
            pr = &inetsw[INSW_TR + i];
            if(in_started) {
                x = PROTODISPATCH_INDEX(&ip_protos, tr->tr_protocol);
                if(protodispatch_clear(&ip_protos, tr->tr_protocol, pr) == 0 &&
                        x < IN_TRXMAX)
                    in_trbyx[x] = NULL;
            }
            *pr = empty;
            in_trslot[i] = NULL;
        }
    splx(s);
}

static const struct inet_trbinding in_trbinding = {
    AF_INET, in_transport_add, in_transport_remove
};

/*
 * IPv4 as a module of the stack (net/netmodule.h): the one descriptor of
 * the group, whose start makes every registration of IPv4, in this order -
 *
 *  0. netiface_fetch(): the transport module's interface, by name - the
 *     one way IPv4 calls the transports' registry (ti_bind,
 *     ti_family_register); kept in in_transports for as long as IPv4 runs;
 *  1. ti_bind (inet_transport_bind()): the registered transports become
 *     entries of inetsw[] (in_transport_add()), and one registered later
 *     is added the same way;
 *  2. domain_attach(&inetdomain): links the domain (sockets of AF_INET can
 *     be made, the entries' timers run) and calls the protocols' init
 *     functions in table order -
 *     ip_init()   - the dispatch table of the IP protocol numbers, from
 *                   the table as step 1 left it, published for the
 *                   protocols of other modules (protodispatch_publish());
 *                   the input queue (netinput_register()); IPv4's part in
 *                   the life of an interface and its address requests
 *                   (netfamily_ifops_register(&in_ifops)); IPv4's instance
 *                   of the family interface (inetfamily_register());
 *                   ARP: the resolver and the ARP packet type of the
 *                   SANA-II interfaces, the ageing and IPv4LL timers
 *                   (arp_init()); IPv4's variables (netvar_register());
 *     igmp_init() - IGMP's own state, nothing registered;
 *  3. the bound transports are noted by their protocol numbers' places in
 *     the dispatch table (in_transport_dispatch()).
 *
 * The descriptor is IPv4's own and makes itself known to the core; the
 * core does not name it. When IPv4 is a library file, the library hands
 * the descriptor to the stack instead (NetModuleAttach()).
 */
static int
inet_start(struct netmodule *nm)
{
    int error, i;

    /* the transports run - the registry started them for us (inet_requires) */
    in_transports = (const struct inet_transport_if *)
        netiface_fetch(nm, INET_TRANSPORT_IFNAME, INET_TRANSPORT_IFVERSION,
                       sizeof(struct inet_transport_if));
    if(in_transports == NULL)
        return (ENOENT);
    if((error = (*in_transports->ti_bind)(&in_trbinding)) != 0)
        return (error);
    if((error = domain_attach(&inetdomain)) != 0) {
        /* (a domain of this family is attached already) */
        (void)(*in_transports->ti_unbind)(&in_trbinding);
        for(i = 0; i < IN_TRSLOTS; i++)
            if(in_trslot[i] != NULL)
                in_transport_remove(in_trslot[i]);
        if(in_rawtr != NULL)
            in_transport_remove(in_rawtr);
        return (error);
    }
    for(i = 0; i < IN_TRSLOTS; i++)
        if(in_trslot[i] != NULL)
            (void)in_transport_dispatch(i);     /* ip_init() took the numbers */
    in_started = 1;
    /* packets may be hooked from the first one on (a hook waits in the core till then) */
    ip_pfil.pp_owner = arp_pfil.pp_owner = nm;
    (void)pfil_point_register(&ip_pfil);
    (void)pfil_point_register(&arp_pfil);
    /*
     * Started while the stack runs: the interfaces that are online get
     * their IPv4 and ARP read requests now (a starting stack sends them
     * when an interface comes online), each interface that is up its ARP
     * table, and the loopback its address. When the stack itself starts,
     * no interface is online or up yet and these do nothing.
     */
    (void)sana_linktype_resume(SANA_PT_IP);
    (void)sana_linktype_resume(SANA_PT_ARP);
    in_attach_running();
    return (0);
}

/* the transports (netinet/inet_transport.c) are started first, once */
static const char *const inet_requires[] = { "transport", NULL };

/*
 * What IPv4 hangs on a PCB that may be another family's (ti_family_pcbs,
 * netinet/inet_transport.h): its multicast options, with the groups the
 * socket joined - an AF_INET6 socket can set them, a socket's options are
 * found by their level.
 */
static int
in_pcbholds(const struct inpcb *inp)
{
    return (inp->inp_moptions != NULL);
}

/*
 * WHAT KEEPS IPv4 FROM STOPPING (nm_can_stop; nothing is changed). The
 * registry has refused already while a socket of AF_INET exists
 * (inetdomain.dom_refs: a connection in TIME_WAIT is still a socket) or
 * another module uses IPv4 (nm_refs: the 6in4 tunnel holds it while a
 * tunnel interface exists), and when IPv4 is the last family that runs. Here, each with its reason in the
 * log, EBUSY:
 *
 *   - a PCB of the transports is still IPv4's though its socket is not -
 *     the IPv4 connection of an AF_INET6 socket that serves both families
 *     - or carries IPv4's multicast options. The PCB points at IPv4's
 *     instance of the family interface and would call through it;
 *   - a protocol of another module still has one of IPv4's protocol
 *     numbers (protodispatch_set() on the published table) and has not
 *     said so by holding the module: its protosw would stay in a table
 *     that is about to be freed.
 */
static int
inet_can_stop(struct netmodule *nm)
{
    long n;
    int i;

    if(in_transports != NULL &&
            (n = (*in_transports->ti_family_pcbs)(&in_family, in_pcbholds)) != 0) {
        __log(LOG_NOTICE, "inet cannot stop: %ld socket(s) of another family still use "
              "IPv4 (an IPv4 connection of an AF_INET6 socket, or IPv4 multicast "
              "options set on one).", n);
        return (EBUSY);
    }
    for(i = 1; in_started && i < ip_protos.pd_n; i++) {
        struct protosw *pr = ip_protos.pd_tab[i];

        if(pr != NULL && (pr < inetsw || pr >= INSW_END)) {
            __log(LOG_NOTICE, "inet cannot stop: IP protocol %ld is held by a protocol "
                  "of another module.", (long)pr->pr_protocol);
            return (EBUSY);
        }
    }
    return (0);
}

/*
 * THE STOP OF IPv4: everything inet_start() and the init functions it had
 * called did, and everything IPv4 made since, is undone. Runs in the
 * stack's task with the socket calls fenced (net/netmodule.h): no timer of
 * IPv4's is running, no packet is on its way through ipintr(), and no
 * application is inside the stack. In this order -
 *
 *  1. NO NEW INPUT. The read requests of the IPv4 and the ARP link type
 *     are taken back from every driver (sana_linktype_quiesce(): no read
 *     completion of either is delivered any more); the input queue is
 *     unregistered and what waited on it dropped (ip_input_stop()): the
 *     loopback, a tunnel and the SANA-II read path find no queue for the
 *     family;
 *  2. NO NEW SOCKET, NO TIMER: domain_detach(&inetdomain). socket(AF_INET)
 *     finds no domain; ip_slowtimo(), the IGMP timers and ip_drain() are
 *     not called any more;
 *  3. THE INTERFACES (in_detach_all()): on every one the link-local claim,
 *     the IPv4 routes through it, its IPv4 addresses (127.0.0.1 on the
 *     loopback among them) and the routes that used them; then the
 *     remaining group memberships, of which the devices are told. Then
 *     ARP (arp_deinit()): its resolver and its input taken back from the
 *     SANA-II layer, its two periodic calls deleted, every interface's
 *     ARP table freed with the packets that waited in it;
 *  4. WHAT IS GLOBAL (ip_flush()): the route kept for forwarding, every
 *     route left in IPv4's table and the path MTU slots holding one, the
 *     reassembly queues;
 *  5. THE REGISTRATIONS: IPv4's instance of the family interface
 *     (ti_family_unregister, which also drops the half-open TCP
 *     connections that came in by IPv4 - they hold the instance's
 *     address); the binding to the transports and each transport's entry
 *     of inetsw[] with its protocol number; the interface events and
 *     address requests, the variables, the published dispatch table and
 *     its memory (ip_deinit());
 *  6. THE VARIABLES of IPv4's files, as the files have them: a library
 *     that stays loaded starts again from the same state as one loaded
 *     anew.
 *
 * AFTERWARDS THE CORE HOLDS NO POINTER INTO IPv4: no domain or protosw
 * (2; no socket of the domain existed), no input queue or handler (1), no
 * read request whose completion would reach ARP's input (1, 3), no
 * resolver (3), no periodic call (3), no interface address - and with them
 * no ifaddr on an interface's list and no route whose rt_ifa is one (3,
 * 4), no interface-event, address-query, variable or dispatch
 * registration (5); the transports hold no family instance, binding, PCB
 * or SYN-cache entry of IPv4's (5 and inet_can_stop()).
 */
static int
inet_stop(struct netmodule *nm)
{
    int i, error;

    /* first what can refuse, before anything is changed */
    if(inetdomain.dom_refs != 0)
        return (EBUSY);

    (void)sana_linktype_quiesce(SANA_PT_IP);
    (void)sana_linktype_quiesce(SANA_PT_ARP);
    ip_input_stop();
    /* no packet passes them any more; what is hooked waits in the core */
    (void)pfil_point_unregister(&ip_pfil);
    (void)pfil_point_unregister(&arp_pfil);

    if((error = domain_detach(&inetdomain)) != 0)
        __log(LOG_ERR, "inet: its domain would not detach (error %ld).", (long)error);

    (void)in_detach_all();
    arp_deinit();

    ip_flush();

    (void)(*in_transports->ti_family_unregister)(&in_family);
    (void)(*in_transports->ti_unbind)(&in_trbinding);
    for(i = 0; i < IN_TRSLOTS; i++)
        if(in_trslot[i] != NULL)
            in_transport_remove(in_trslot[i]);
    if(in_rawtr != NULL)
        in_transport_remove(in_rawtr);
    ip_deinit();

    in_started = 0;
    in_transports = NULL;       /* not to be used after the stop (net/netmodule.h) */
    for(i = 0; i < IN_TRXMAX; i++)
        in_trbyx[i] = NULL;
    inetdomain.dom_next = NULL;
    inetdomain.dom_refs = 0;
    icmp_defaults();
#ifdef ENABLE_MULTICAST
    igmp_defaults();
#endif
    in_family_defaults();
    bzero((caddr_t)&aros_dhcpv4, sizeof(aros_dhcpv4));
    return (0);
}

/*
 * WHEN THE STACK ENDS and IPv4 was not stopped (nm_shutdown,
 * net/netmodule.h) it has nothing to release itself. Its memory -
 * interface addresses, multicast records, reassembly queues, the dispatch
 * table, the ARP tables and entries - is from the core's pool or is
 * mbufs, freed in one piece by the core; its two periodic calls
 * (arp_init()) are the core's timer requests, deleted by timer_deinit();
 * the SANA-II read requests its handlers complete are the core's,
 * collected by sana_deinit(). The ARP tables' semaphores
 * (InitSemaphore()) need no release. It opens no device or library and
 * has no port, signal or AllocMem() memory. So nm_shutdown is NULL.
 */
NETMODULE_DESCRIPTOR(inet_module) = {
    sizeof(struct netmodule), NETMODULE_VERSION, "inet",
    /* selectable: PROTOCOLS says whether IPv4 runs, a module that needs it does not */
    NMF_SELECTABLE, inet_start, inet_stop, inet_can_stop,
    0, NULL, &inetdomain, inet_requires,
    /*
     * TEST SWITCH (see netmodule_start_builtin()): IPv4 is left out of the
     * running stack when the global variable AROSTCP/WithoutIPv4 exists,
     * whatever the configuration item PROTOCOLS says. The item is how a
     * stack is configured; the switch is for the test harness.
     */
    "AROSTCP/WithoutIPv4",
    NULL,           /* nm_library: the loader's */
    NULL            /* nm_shutdown: nothing outside the core's pools */
};
NETMODULE_BUILTIN(inet_module, 20)
