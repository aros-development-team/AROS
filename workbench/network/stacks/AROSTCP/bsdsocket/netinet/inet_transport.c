/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * The internet transports - TCP, UDP and the raw sockets, each one
 * implementation for every address family - as a module of the stack
 * (net/netmodule.h), and the registry through which the families come by
 * them (netinet/inet_transport.h).
 *
 * They belong to no family, and no family names them: each transport has
 * one descriptor, registered here when the module starts; a family that
 * starts binds to the registry and makes its protocol table entries of
 * the descriptors. The module is not started for its own sake: a family's
 * module names it as needed (nm_requires). So whether the stack runs
 * IPv4, IPv6 or both, the transports are initialised once, here, and
 * their timers run once.
 *
 * The initialisation and the timers use the mechanism the protocol switch
 * has always offered: this module attaches a domain of its own - of no
 * address family, with no protocol a socket could be made of - whose
 * entries carry the transports' init, timer and drain functions.
 * domain_attach() calls the init functions; pffasttimo() and pfslowtimo()
 * call the timers at exactly the periods they have always had (PR_FASTHZ:
 * 50 ms, which netperiodic_add(), counting whole 20 ms ticks, cannot
 * give); m_reclaim() calls the drain function. None of them is in a
 * family's table.
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/mbuf.h>
#include <sys/errno.h>
#include <sys/syslog.h>

#include <net/netmodule.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/raw_ip_protos.h>
#include <netinet/inetfamily.h>
#include <netinet/inet_transport.h>
#include "tcp_compat.h"             /* tcp_deinit() */
#include "tcp_syncache.h"           /* syncache_flush() */
#include <net/netcore_map.h>        /* always the last include */

/*
 * The registry: the transports in the order they were registered in, and
 * the families that asked to be told of them. Both small and fixed; they
 * are walked when a module starts or stops, never for a packet.
 */
static const struct inet_transport *inet_transports[INET_TRANSPORT_MAX];
static const struct inet_trbinding *inet_trbindings[INET_TRBINDING_MAX];

static void
inet_transport_tell(const struct inet_trbinding *tb, const struct inet_transport *tr)
{
    int error = (*tb->tb_add)(tr);

    if(error != 0)
        __log(LOG_ERR, "transport: protocol %ld (socket type %ld) does not serve "
              "address family %ld (error %ld).", (long)tr->tr_protocol,
              (long)tr->tr_type, (long)tb->tb_family, (long)error);
}

int
inet_transport_register(const struct inet_transport *tr)
{
    int i, n;

    if(tr == NULL || tr->tr_size < sizeof(struct inet_transport) ||
            tr->tr_type == 0 || tr->tr_usrreq == NULL)
        return (EINVAL);
    if(tr->tr_flags & TRF_RAW) {
        if(tr->tr_protocol != 0 || tr->tr_deliver == NULL || tr->tr_output == NULL)
            return (EINVAL);
    } else if(tr->tr_protocol <= 0 || tr->tr_protocol > 255 || tr->tr_input == NULL)
        return (EINVAL);
    for(n = 0; n < INET_TRANSPORT_MAX && inet_transports[n] != NULL; n++)
        if(inet_transports[n] == tr ||
                ((inet_transports[n]->tr_flags ^ tr->tr_flags) & TRF_RAW) == 0 &&
                ((tr->tr_flags & TRF_RAW) ||
                 inet_transports[n]->tr_protocol == tr->tr_protocol))
            return (EEXIST);    /* one transport for a protocol number; one for raw sockets */
    if(n == INET_TRANSPORT_MAX)
        return (ENOSPC);
    inet_transports[n] = tr;
    /* the families that run already */
    for(i = 0; i < INET_TRBINDING_MAX; i++)
        if(inet_trbindings[i] != NULL)
            inet_transport_tell(inet_trbindings[i], tr);
    return (0);
}

int
inet_transport_unregister(const struct inet_transport *tr)
{
    int i, n;

    for(n = 0; n < INET_TRANSPORT_MAX && inet_transports[n] != tr; n++)
        ;
    if(tr == NULL || n == INET_TRANSPORT_MAX)
        return (ENOENT);
    for(i = 0; i < INET_TRBINDING_MAX; i++)
        if(inet_trbindings[i] != NULL)
            (*inet_trbindings[i]->tb_remove)(tr);
    for(; n < INET_TRANSPORT_MAX - 1; n++)
        inet_transports[n] = inet_transports[n + 1];
    inet_transports[INET_TRANSPORT_MAX - 1] = NULL;
    return (0);
}

int
inet_transport_bind(const struct inet_trbinding *tb)
{
    int i, slot = -1;

    if(tb == NULL || tb->tb_add == NULL || tb->tb_remove == NULL)
        return (EINVAL);
    for(i = 0; i < INET_TRBINDING_MAX; i++) {
        if(inet_trbindings[i] == tb)
            return (EEXIST);
        if(inet_trbindings[i] == NULL && slot < 0)
            slot = i;
    }
    if(slot < 0)
        return (ENOSPC);
    inet_trbindings[slot] = tb;
    for(i = 0; i < INET_TRANSPORT_MAX && inet_transports[i] != NULL; i++)
        inet_transport_tell(tb, inet_transports[i]);
    return (0);
}

int
inet_transport_unbind(const struct inet_trbinding *tb)
{
    int i;

    for(i = 0; i < INET_TRBINDING_MAX; i++)
        if(tb != NULL && inet_trbindings[i] == tb) {
            inet_trbindings[i] = NULL;
            return (0);
        }
    return (ENOENT);
}

/*
 * The three transports of the stack as shipped. Registered in this order,
 * which is the order a family's table lists them in (UDP, TCP, raw: as
 * the tables have always been).
 */
static const struct inet_transport udp_transport = {
    .tr_size      = sizeof(struct inet_transport),
    /* UDP reads nothing of a network header's options, and the error it
     * has a family send for a port nobody listens on quotes a header
     * without them */
    .tr_flags     = TRF_BAREHDR | TRF_PORTS,
    .tr_type      = SOCK_DGRAM,
    .tr_protocol  = IPPROTO_UDP,
    .tr_prflags   = PR_ATOMIC | PR_ADDR,
    .tr_input     = udp_input,
    .tr_ctlnotify = udp_ctlnotify,
    /* no tr_ctloutput: a UDP socket's options are its family's */
    .tr_usrreq    = udp_usrreq,
};

static const struct inet_transport tcp_transport = {
    .tr_size      = sizeof(struct inet_transport),
    .tr_flags     = TRF_PORTS | TRF_SEQ,
    .tr_type      = SOCK_STREAM,
    .tr_protocol  = IPPROTO_TCP,
    .tr_prflags   = PR_CONNREQUIRED | PR_IMPLOPCL | PR_WANTRCVD,
    .tr_input     = tcp_input,
    .tr_ctlnotify = tcp_ctlnotify,
    .tr_ctloutput = tcp_ctloutput,
    .tr_usrreq    = tcp_usrreq,
};

static const struct inet_transport raw_transport = {
    .tr_size      = sizeof(struct inet_transport),
    .tr_flags     = TRF_RAW,
    .tr_type      = SOCK_RAW,
    .tr_prflags   = PR_ATOMIC | PR_ADDR,
    .tr_ctloutput = rip_ctloutput,
    .tr_usrreq    = rip_usrreq,
    .tr_deliver   = rip_deliver,
    .tr_output    = rip_output,
};

/*
 * TCP's retransmission deadlines, served by the core's deadline timer
 * (net/netmodule.h, kern/amiga_time.c): its clock and what runs the due
 * ones.
 */
static const struct netdeadline tcp_deadline = { tcp_ms, tcp_rexmt_due };

static struct domain transportdomain;
static int transport_stop(struct netmodule *nm);

/* for rip_pcbcount(): every PCB counts */
static int
transport_anypcb(const struct inpcb *inp)
{
    return (1);
}

/*
 * No entry has a socket type, a protocol number, an input or a user
 * request: nothing is found here by socket() or by a family's dispatch.
 */
static struct protosw transportsw[] = {
    {
        0,		&transportdomain,	0,		0,
        NULL,		NULL,		NULL,		NULL,
        NULL,
        udp_init,	NULL,		NULL,		NULL,
    },
    {
        0,		&transportdomain,	0,		0,
        NULL,		NULL,		NULL,		NULL,
        NULL,
        tcp_init,	tcp_fasttimo,	tcp_slowtimo,	tcp_drain,
    },
};

static struct domain transportdomain = {
    AF_UNSPEC, "transport", NULL, NULL, NULL,
    transportsw, &transportsw[sizeof(transportsw) / sizeof(transportsw[0])], NULL
};

/*
 * What the families (and a transport in a module of its own) reach the
 * transport module by: published by name when the module starts
 * (netinet/inet_transport.h). The addresses are this module's own
 * functions, so the table is constant data but for its head, which the
 * core completes.
 */

/*
 * ti_family_unregister: a family leaves the transports (it stops). Its
 * sockets are gone (its module's user count) and so are the PCBs that
 * were of it or carried something of it (ti_family_pcbs, which it asked
 * before). What the transports themselves still keep with a pointer to
 * the family is flushed here: the half-open connections that came in by
 * it (TCP's SYN cache). Then it is off the table, and no code of the
 * transports can come by its instance again.
 */
static int
transport_family_unregister(const struct inetfamily *inf)
{
    syncache_flush(inf);
    return (inetfamily_unregister(inf));
}

/* ti_family_pcbs: the PCBs of all three transports that still use the family */
static long
transport_family_pcbs(const struct inetfamily *inf, int (*holds)(const struct inpcb *inp))
{
    return (in_pcbcount(&tcb, inf, holds) + in_pcbcount(&udb, inf, holds) +
            rip_pcbcount(inf, holds));
}

static struct inet_transport_if transport_if = {
    { sizeof(struct inet_transport_if), INET_TRANSPORT_IFVERSION, INET_TRANSPORT_IFNAME, NULL },
    inetfamily_register,
    transport_family_unregister,
    inet_transport_bind,
    inet_transport_unbind,
    inet_transport_register,
    inet_transport_unregister,
    transport_family_pcbs,
};

/*
 * The start of the transports: everything the group registers anywhere is
 * registered from here, in this order -
 *
 *  1. domain_attach(&transportdomain): links the domain (from now on its
 *     timer and drain entries are called) and calls, in table order,
 *     udp_init()  - UDP's PCB list and hash table; its variables
 *                   (netvar_register(udp_netvars));
 *     tcp_init()  - TCP's variables (netvar_register(tcp_netvars), first,
 *                   so that what the configuration set reaches them); the
 *                   transports' part in the life of an interface
 *                   (netfamily_ifops_register(&transport_ifops), for the
 *                   PCBs of all three); the keys, the congestion control
 *                   algorithms, the PCB list and hash table, the SYN
 *                   cache, the TAO cache; max_protohdr.
 *     The raw sockets have nothing to initialise.
 *  2. netdeadline_register(): TCP's retransmission deadlines.
 *  3. inet_transport_register() for UDP, TCP and the raw sockets: each is
 *     told to the families that run already (none when the stack starts:
 *     a family starts this module before itself).
 *  4. netiface_publish(): the table the families bind and register
 *     through, under its name. Last, so that nobody finds a transport
 *     module that is not complete.
 *
 * Started once at a time: the registry starts a started module no second
 * time, and domain_attach() refuses a domain that is attached already. It
 * can be stopped (transport_stop()) and started again; every start finds
 * the module's variables as the file has them.
 */
static int
transport_start(struct netmodule *nm)
{
    int error;

    if((error = domain_attach(&transportdomain)) != 0)
        return (error);
    if((error = netdeadline_register(&tcp_deadline)) == 0 &&
            (error = inet_transport_register(&udp_transport)) == 0 &&
            (error = inet_transport_register(&tcp_transport)) == 0 &&
            (error = inet_transport_register(&raw_transport)) == 0 &&
            (error = netiface_publish(nm, &transport_if.ti_if)) == 0)
        return (0);
    /*
     * Cannot happen with an empty registry. Everything is taken back, as
     * a stop does it: the module can be started again.
     */
    __log(LOG_ERR, "transport: could not register (error %ld).", (long)error);
    (void)transport_stop(nm);
    return (error);
}

/*
 * WHAT KEEPS THE TRANSPORTS FROM STOPPING (nm_can_stop; nothing is
 * changed). The registry has already refused while a module that needs
 * this one is started (nm_refs: each family names it in nm_requires). Here:
 * a family is still bound or registered - one that did not stop cleanly,
 * or a module that binds without naming this one as needed; a transport
 * of another module is still registered with this registry; or a PCB is
 * left on one of the lists (every socket of a family is gone when the
 * family has stopped, so this would be a leak, and stopping would turn it
 * into a pointer into memory that is no longer the transports'). Each
 * with its reason in the log. EBUSY.
 */
static int
transport_can_stop(struct netmodule *nm)
{
    const struct inetfamily *inf;
    int i;

    for(i = 0; i < INET_TRBINDING_MAX; i++)
        if(inet_trbindings[i] != NULL) {
            __log(LOG_NOTICE, "transport cannot stop: address family %ld is still "
                  "bound to it.", (long)inet_trbindings[i]->tb_family);
            return (EBUSY);
        }
    if((inf = inetfamily_any()) != NULL) {
        __log(LOG_NOTICE, "transport cannot stop: address family %ld is still "
              "registered with it.", (long)inf->inf_family);
        return (EBUSY);
    }
    for(i = 0; i < INET_TRANSPORT_MAX && inet_transports[i] != NULL; i++)
        if(inet_transports[i] != &udp_transport && inet_transports[i] != &tcp_transport &&
                inet_transports[i] != &raw_transport) {
            __log(LOG_NOTICE, "transport cannot stop: protocol %ld of another module "
                  "is still registered with it.", (long)inet_transports[i]->tr_protocol);
            return (EBUSY);
        }
    if(tcb.lh_first != NULL || udb.lh_first != NULL ||
            rip_pcbcount(NULL, transport_anypcb) != 0) {
        __log(LOG_NOTICE, "transport cannot stop: sockets of TCP, UDP or the raw "
              "protocols still exist.");
        return (EBUSY);
    }
    return (0);
}

/*
 * THE STOP OF THE TRANSPORTS: everything transport_start() and the init
 * functions it had called did is undone, in this order -
 *
 *  1. netiface_withdraw(): nobody finds the transports' interface any
 *     more (no family holds it: transport_can_stop());
 *  2. inet_transport_unregister() for the raw sockets, TCP and UDP - no
 *     family is bound, so nobody is told;
 *  3. netdeadline_unregister(): the core's deadline request no longer
 *     calls TCP's clock and timer (a request that is out comes back and
 *     idles);
 *  4. domain_detach(&transportdomain): the fast and the slow timer and
 *     the drain entry are not called any more. From here on nothing in
 *     the core calls the transports;
 *  5. tcp_deinit(): the transports' interface events and TCP's variables
 *     taken back, the SYN cache's entries and table, TCP's PCB hash table
 *     and the trace buffer freed, TCP's variables as the file has them;
 *  6. udp_deinit(): UDP's variables taken back, its PCB hash table freed,
 *     its variables as the file has them; rip_deinit(): the raw sockets'
 *     list;
 *  7. the registry's two tables and the families' table emptied.
 *
 * Runs in the stack's task with the socket calls fenced (net/netmodule.h),
 * so no timer of TCP's is running and no packet is on its way in while
 * this is done. When it returns the core has no pointer into this module:
 * no domain, timer, deadline, interface-event or variable registration,
 * no published table - and the families, which had descriptors, bindings
 * and their instances here, stopped before (the registry does not ask
 * this module while one of them runs).
 */
static int
transport_stop(struct netmodule *nm)
{
    int error, i;

    /* first what can refuse, before anything is changed */
    if(transportdomain.dom_refs != 0)
        return (EBUSY);

    (void)netiface_withdraw(&transport_if.ti_if);
    (void)inet_transport_unregister(&raw_transport);
    (void)inet_transport_unregister(&tcp_transport);
    (void)inet_transport_unregister(&udp_transport);
    (void)netdeadline_unregister(&tcp_deadline);
    if((error = domain_detach(&transportdomain)) != 0)
        __log(LOG_ERR, "transport: its domain would not detach (error %ld).", (long)error);

    tcp_deinit();
    udp_deinit();
    rip_deinit();

    for(i = 0; i < INET_TRANSPORT_MAX; i++)
        inet_transports[i] = NULL;
    for(i = 0; i < INET_TRBINDING_MAX; i++)
        inet_trbindings[i] = NULL;
    inetfamily_forget();
    transportdomain.dom_next = NULL;
    transportdomain.dom_refs = 0;
    return (0);
}

/*
 * WHEN THE STACK ENDS and the module was not stopped (nm_shutdown,
 * net/netmodule.h) it has nothing to release itself: everything it
 * allocates comes from the core's pool (bsd_malloc(): the PCB hash tables,
 * PCBs, TCP control blocks, the SYN cache, RACK maps) or is mbufs, and the
 * core frees both in one piece; its timers are the protocol switch's fast
 * and slow timers and the core's deadline request, which the core deletes.
 * It opens no device or library and has no port, signal or AllocMem()
 * memory of its own. So nm_shutdown is NULL.
 */
NETMODULE_DESCRIPTOR(transport_module) = {
    sizeof(struct netmodule), NETMODULE_VERSION, "transport",
    0, transport_start, transport_stop, transport_can_stop,
    0, NULL, &transportdomain, NULL, NULL, NULL,
    NULL            /* nm_shutdown: nothing outside the core's pools */
};
/*
 * Known by name when the stack starts; started when a family needs it
 * (nm_requires), not because it is listed here.
 */
NETMODULE_BUILTIN(transport_module, 5)
