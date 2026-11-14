/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef NET_NETMODULE_H
#define NET_NETMODULE_H

/*
 * A module of the stack: an address family, a transport, the packet
 * services, or a protocol that is not part of the stack as shipped. The
 * families compiled into the stack register through the same structure as a
 * module loaded later.
 *
 * A module is started once and may be stopped again. Stopping is never
 * forced: it is refused while the module is in use (nm_refs, nm_can_stop).
 * Registering and unregistering are done with the stack fenced - during
 * start-up, or in the stack's task with syscall_semaphore held, as an
 * in-place reload is.
 *
 * WHAT nm_stop MAY RELY ON, and must leave behind. It runs in the stack's
 * own task (the control port's command, kern/amiga_main.c control_module();
 * or the stack's shutdown), outside sana_poll() and timer_poll(), with
 * syscall_semaphore held and no level raised: no application is inside a
 * socket call (one that sleeps in tsleep() has given the fence up, but it
 * sleeps in the socket layer of the core, on a socket - and a module with
 * a socket of its domain is not asked to stop), no timer callback and no
 * packet input of the module is running, and none can start before it
 * returns. Code of a module cannot sleep (the core's table has no sleep
 * call), so no task is suspended inside it either. When nm_stop returns 0
 * the core holds no pointer into the module's code or data: every
 * registration is taken back, nothing of the module's is queued, and every
 * static variable has the value the file gives it - a library that stays
 * loaded is started again from the same state as one loaded anew. When it
 * returns an errno it has changed nothing.
 *
 * The structure is allocated by the module and grows at the end only;
 * nm_size and nm_version say what the module was built with.
 */

#define NETMODULE_VERSION   3       /* 2: nm_library; 3: nm_shutdown */

struct netmodule {
    unsigned short   nm_size;       /* sizeof(struct netmodule) */
    unsigned short   nm_version;    /* NETMODULE_VERSION */
    const char      *nm_name;
    unsigned long    nm_flags;      /* NMF_* */
    int            (*nm_start)(struct netmodule *);     /* 0 or an errno */
    int            (*nm_stop)(struct netmodule *);      /* 0 or an errno; NULL: cannot stop */
    int            (*nm_can_stop)(struct netmodule *);  /* 0, or the errno a stop would fail with */
    long             nm_refs;       /* users other than sockets; the module counts them */
    struct netmodule *nm_next;      /* owned by the registry */
    struct domain   *nm_domain;     /* the domain it attaches, or NULL; its sockets count as users */
    /*
     * The modules this one needs, by name, ending with NULL; NULL for none.
     * They are started first (one that is known but not running is started
     * for it, one in a library file is loaded for it) and cannot be stopped
     * while this one runs. A module that is needed and not there (ENOENT),
     * or a circle of needs (ELOOP), fails the start with a message that
     * names it. So does a needed module that the configuration left out
     * (ENXIO): being needed never starts a module whose test switch is
     * set, nor one that is NMF_SELECTABLE and not named in PROTOCOLS.
     */
    const char *const *nm_requires;
    /*
     * A TEST SWITCH, not configuration: the name of a global variable
     * (ENV:) which, when it exists, keeps a module that is linked into the
     * stack binary from being started with the stack, whatever the
     * configuration selects. The module stays known by name. NULL: none.
     * What decides which protocol modules a stack runs is the
     * configuration item PROTOCOLS (netmodule_start_builtin(),
     * kern/net_module.c); the switch is for the test harness, which
     * selects a configuration without editing a file, and it wins over
     * the item - with a line in the log that says so.
     */
    const char      *nm_leaveout;
    /*
     * The library file the module's code is in, held open by the stack
     * from when it is loaded until it is unloaded (kern/net_modload.c).
     * Set by the loader; NULL for a module that is part of the stack
     * program.
     */
    struct Library  *nm_library;
    /*
     * The stack is ending and this module could not be stopped (nm_stop
     * is NULL, or it refused): release what the module holds that is NOT
     * the core's to free. The core frees its memory pools and mbufs in
     * one piece afterwards, deletes its own timers and closes the
     * interfaces before this is called - so memory from bsd_malloc(),
     * mbufs, periodic calls (netperiodic_add()), deadlines and SANA-II
     * requests need nothing here. What does: an exec message port, an I/O
     * request, a device or a library the module opened itself, a signal,
     * memory from AllocMem(). Called once, in the stack's task, the
     * module that needs another before the one it needs; no socket
     * exists, no timer runs and no packet arrives any more, and after it
     * nothing of the module is called again - a module in a library file
     * is then removed from memory. NULL: it holds nothing of the kind.
     */
    void           (*nm_shutdown)(struct netmodule *);
};

/* what a module built for version 2 of the structure has of it */
#define NETMODULE_SIZE_V2   ((unsigned long)&((struct netmodule *)0)->nm_shutdown)
#define NETMODULE_HAS(nm, member) \
    ((nm)->nm_size >= (unsigned long)&((struct netmodule *)0)->member + sizeof((nm)->member))

/* set by the module */
#define NMF_PERMANENT   0x0001      /* never stopped once started */
/*
 * Whether the module runs is for the configuration to say: it is one of
 * those the item PROTOCOLS names (a protocol family). It is then started
 * only when the item names it - never because another module needs it:
 * that module's start fails instead (nm_requires above). A module without
 * the flag (the transports, anything that exists to serve other modules)
 * is started, and its library loaded, when one that needs it starts.
 */
#define NMF_SELECTABLE  0x0002
/* set by the registry */
#define NMF_STARTED     0x0100
#define NMF_BUILTIN     0x0200      /* code is part of the stack binary */
#define NMF_STARTING    0x0400      /* being started: seen again, its needs go round in a circle */

/*
 * Dispatch of a family's packets by protocol number. A family owns one of
 * these; a protocol - the family's own, a transport, a tunnel, one loaded
 * later - takes a number with protodispatch_set(). Numbers nobody took go
 * to the family's default (its raw protocol). One byte per number and one
 * pointer per protocol in use; looking a number up is two loads.
 */
struct protosw;

struct protodispatch {
    struct protosw **pd_tab;        /* pd_tab[0] is the default */
    unsigned char    pd_n;          /* entries in use */
    unsigned char    pd_max;        /* entries allocated */
    unsigned char    pd_x[256];     /* protocol number -> index into pd_tab */
};

#define PROTODISPATCH(pd, proto) ((pd)->pd_tab[(pd)->pd_x[(unsigned char)(proto)]])
/*
 * The place of a number in the table: 0 for one nobody took, else a small
 * number (below pd_max) that stays the same for as long as the number is
 * taken. The owner of the table may keep something of its own for each
 * taken number in an array with this index.
 */
#define PROTODISPATCH_INDEX(pd, proto) ((pd)->pd_x[(unsigned char)(proto)])

/*
 * An input queue of the stack: packets of one address family waiting for
 * the stack's task, with the function that takes them off. `ni_isr' is the
 * bit schednetisr() sets (net/netisr.h).
 */
struct ifqueue;

struct netinput {
    int              ni_family;     /* AF_xxx */
    int              ni_isr;        /* NETISR_xxx */
    struct ifqueue  *ni_queue;
    void           (*ni_intr)(void);
};

#define NETINPUT_MAX    16          /* input queues at one time */

/*
 * A family's part in the life of an interface. The interface layer and the
 * drivers do not call into a family by name: they raise an event, and each
 * registered family does its part, in ascending nfo_order.
 *
 * For NFE_UNCONFIGURE and NFE_LINKDOWN the handler returns the number of
 * addresses it removed; for the others, 0.
 */
struct ifnet;
struct socket;
struct sockaddr;

#define NFE_UP              1   /* brought up with its device running: make what the family makes itself */
#define NFE_LINKUP          2   /* the link came (back): the network may be another one */
#define NFE_LINKDOWN        3   /* the link went: give up what belonged to that network */
#define NFE_UNCONFIGURE_PRE 4   /* about to lose its configuration: before its routes are flushed */
#define NFE_UNCONFIGURE     5   /* ... after: remove the family's addresses */
#define NFE_REMOVING        6   /* is being taken out of the configuration, still usable */
#define NFE_DETACHING       7   /* device stopped, about to leave the interface list */
#define NFE_DETACHED        8   /* off the list, its addresses freed: nothing may go on naming it */
#define NFE_AUTOADDR        9   /* the configuration asks the family to find an address by itself
                                   (raised for one family: netfamily_if_event_to()) */

struct netfamily_ifops {
    int     nfo_family;         /* AF_xxx; AF_UNSPEC for a transport serving several */
    int     nfo_order;          /* lower is called first: 10 IPv4, 20 IPv6, 30 transports */
    int   (*nfo_event)(int event, struct ifnet *ifp);
    /*
     * The family's address requests (its SIOCxIFADDR ioctls and the like),
     * from an application or from the stack's own configuration. `ifp' is
     * NULL for a request that names no interface. NULL when it has none.
     */
    int   (*nfo_control)(struct socket *so, int cmd, caddr_t data, struct ifnet *ifp);
    /*
     * A question about one of the family's addresses (NFQ_xxx), for the
     * parts of the core that are handed an address and do not know the
     * family's rules. The answer is 0 or 1; for NFQ_NETMASK `res' is filled
     * in and the answer is 1. NULL when the family answers none.
     */
    int   (*nfo_addrquery)(int what, const struct sockaddr *sa, struct sockaddr *res);
};

/*
 * A named interface: how one module offers functions to another without
 * either being linked against the other - two library files cannot be -
 * and without the core knowing what the functions are.
 *
 * The module that offers puts a `struct netiface' at the head of a table
 * of function pointers of its own design and publishes it under a name
 * when it starts (netiface_publish()). A module that uses it fetches it
 * ONCE, in its own nm_start (netiface_fetch()), keeps the pointer and
 * calls through it from then on: nothing is looked up for a call, let
 * alone for a packet.
 *
 * Lifetime, with no count besides the registry's own: a module may fetch
 * only an interface of a module it names in nm_requires (or its own). The
 * registry has then started the publisher first and holds it (nm_refs)
 * for as long as the fetcher is started, so the publisher cannot be
 * stopped, and the table cannot go, under a fetcher; and a fetcher that
 * stops has nothing to give back. A fetched pointer must not be used
 * after the fetcher's own nm_stop.
 *
 * Versions: a table grows at the end only; nif_version is raised when it
 * does, nif_size is its size. A fetcher says the version and the size it
 * was built for and is refused (NULL, with a line in the log that names
 * both modules) when the published table is older or smaller.
 *
 * Names: "<publishing module>.<what>", e.g. "transport.inet" - by
 * convention; they are compared as they are. The table belongs to the
 * publisher, stays at the same address while published, and is not copied.
 * Publish, withdraw and fetch with the stack fenced, as modules are
 * started and stopped. What the publisher did not withdraw itself is
 * withdrawn for it when it has stopped.
 */
struct netiface {
    unsigned short   nif_size;      /* sizeof the whole table this is the head of */
    unsigned short   nif_version;   /* of that table */
    const char      *nif_name;
    struct netmodule *nif_owner;    /* set by netiface_publish() */
};

#define NETIFACE_MAX        8   /* published interfaces at one time */

/*
 * Requests of the stack itself to a family's nfo_control, about its table of
 * link-layer neighbours when that is not in the routing table (IPv6's
 * neighbour cache, as FreeBSD's lltable): the administration module's
 * routing socket and GetRouteInfo() ask it so. They are not ioctls - an
 * ioctl has IOC_IN, IOC_OUT or IOC_VOID set - and come with no socket. A
 * family that keeps its neighbours in routes has no such table and does
 * not answer them.
 */
struct rt_msghdr;

#define NFC_LLINFO_RTMSG    1   /* data: struct nfc_llinfo_rtmsg */
#define NFC_LLINFO_ROWS     2   /* data: struct nfc_llinfo_rows */

/* A routing message with RTF_LLINFO about one of the family's addresses:
 * RTM_ADD sets a neighbour's link-layer address by hand (the gateway a
 * sockaddr_dl), RTM_DELETE deletes it, RTM_GET reads it - into *nr_rtm,
 * which may be replaced by a larger one (R_Malloc()). */
struct nfc_llinfo_rtmsg {
    struct rt_msghdr  **nr_rtm;
    struct sockaddr    *nr_dst;
    struct sockaddr    *nr_gate;
};

/* The neighbours as GetRouteInfo() rows: with nr_buf NULL their size is
 * added to nr_size; else they are written at nr_buf, which moves past them. */
struct nfc_llinfo_rows {
    long                nr_size;
    char               *nr_buf;
};

#define NFQ_LOCAL       1   /* is the address on a network this host is attached to */
#define NFQ_FORWARDABLE 2   /* may a packet to the address be forwarded */
#define NFQ_NETMASK     3   /* the mask of the address's network when a route names none
                               (`res' has room for a struct sockaddr) */
#define NFQ_GATEWAYLINK 4   /* does the address, as the gateway of a route, name its link
                               itself (IPv6: a link-local address with a zone)? The answer
                               is the index of that interface, 0 if it names none; `res'
                               (room for a struct sockaddr_storage) is then this host's
                               address on that link as the interface has it, of length 0
                               when there is none */

#define NETFAMILY_IFOPS_MAX 8

/*
 * A variable or a statistics block that belongs to a module and is read or
 * set from outside it: by the configuration and ARexx variables, by
 * sysctl(), by a program looking the stack's symbols up (FindKernelVar()).
 * The core does not name them: the module registers a table of them when
 * it starts and takes it back when it stops, and each reader looks its
 * name up. A name that is not registered - its module is not part of the
 * running stack - is, to the caller, a variable that does not exist.
 *
 * The table is the module's and is not copied.
 */
#define NVT_INT     1       /* an int, read and set */
#define NVT_STATS   2       /* a block of counters (or another block a program reads
                               in place: a list of PCBs), read only */
#define NVT_SYSCTL  3       /* nv_addr is a function,
                               int (*)(const struct netvar *, void *oldp, size_t *oldlenp,
                                       void *newp, size_t newlen),
                               that serves the variable's sysctl name itself: a value
                               that is checked when set, or kept in another type */

struct netvar {
    const char     *nv_name;    /* the C name: "ipforwarding", "ipstat" */
    void           *nv_addr;
    unsigned long   nv_size;    /* in bytes */
    unsigned char   nv_type;    /* NVT_xxx */
    /* its sysctl name net.<family>.<proto>.<id>; nv_family 0 when it has none */
    unsigned char   nv_family;
    unsigned short  nv_proto;
    unsigned short  nv_id;
};

#define NETVAR_TABLES_MAX   8   /* registered tables at one time */

/*
 * Deadlines of a module that cannot wait for the next tick of the periodic
 * timers (TCP's retransmission deadlines). The core keeps one timer request
 * for them; the module that has them tells it how to read the clock they
 * are on and how to run them. One module at a time.
 *
 * nd_now: the module's clock, in milliseconds. nd_due: run what is due and
 * return the next deadline on that clock; *any is set to 0 when there is
 * none. Both are called at splsoftclock, in the stack's task. The module
 * calls tcp_timer_want(at) - at splnet, from whatever task is in the stack -
 * when it sets a deadline `at' that may be earlier than the one the core
 * waits for. (kern/amiga_time.c)
 */
struct netdeadline {
    u_int32_t (*nd_now)(void);
    u_int32_t (*nd_due)(int *any);
};

/*
 * A kind of tunnel interface a module can make: the reader of the
 * interface configuration (a line with TUNNEL) asks for one here instead
 * of calling a module by name. nt_carrier is the family the tunnel's two
 * end points are addresses of (TSRC, TDST). nt_make makes and attaches the
 * interface described by `ifc' (net/sana2config.h: its name and unit),
 * between `src' (may be "any": chosen for each packet) and `dst', sending
 * with `ttl' (0: the tunnel's default); NULL when it cannot - the reason is
 * logged by the module.
 *
 * A reload of the configuration takes out the interfaces whose line is gone,
 * tunnels as well as devices. For every interface named in the configuration
 * read (a tunnel made by nt_make included) the core calls nt_seen of every
 * kind with the reload's generation; the kind ignores an interface that is
 * not one of its own. After the reload the core calls nt_prune with the same
 * generation: the kind removes each of its tunnels that was not seen (as an
 * interface that left the configuration: if_remove()) and gives back what it
 * held for tunnels when none is left. Both in the stack's task with the
 * stack fenced. Both may be NULL: that kind's tunnels then stay until its
 * module stops.
 */
struct ssconfig;

struct nettunnel {
    const char     *nt_name;        /* the kind: "6in4" */
    int             nt_carrier;     /* AF_xxx of the end points */
    struct ifnet *(*nt_make)(struct ssconfig *ifc, const struct sockaddr *src,
                             const struct sockaddr *dst, int ttl);
    void          (*nt_seen)(struct ifnet *ifp, ULONG gen);
    void          (*nt_prune)(ULONG gen);
};

#define NETTUNNEL_MAX       4   /* registered kinds at one time */
#define PROTODISPATCH_FAMILIES 4 /* published dispatch tables at one time */

#ifdef KERNEL
extern struct netmodule *netmodules;

int     protodispatch_init(struct protodispatch *pd, struct protosw *deflt);
int     protodispatch_set(struct protodispatch *pd, int proto, struct protosw *pr);
int     protodispatch_clear(struct protodispatch *pd, int proto, struct protosw *pr);
/*
 * The owner of the table stops: its pointer array is freed and the table
 * is as before protodispatch_init(). After protodispatch_publish(family,
 * NULL), with nothing dispatching through it any more.
 */
void    protodispatch_done(struct protodispatch *pd);
/*
 * A family makes its table known under its AF_xxx, so that a protocol of
 * another module - a tunnel carried by the family, a protocol loaded later -
 * can take one of its numbers with protodispatch_set() without naming the
 * family's code. pd NULL takes it back. protodispatch_of(): NULL when the
 * family is not part of the running stack.
 */
int     protodispatch_publish(int family, struct protodispatch *pd);
struct protodispatch *protodispatch_of(int family);

int     netinput_register(const struct netinput *ni);
int     netinput_unregister(int family);
/*
 * The core's copy of the registration, or NULL. The pointer is into a
 * table that is kept sorted: it is good until the next netinput_register()
 * or netinput_unregister() and no longer. Read what is needed (the queue,
 * the bit) and keep that - for no longer than the family is known to run -
 * or look it up again.
 */
const struct netinput *netinput_find(int family);
/* the table changed: the SANA-II read path forgets the entries it kept (net/if_sana.c) */
void    sana_linktype_uncache(void);

int     netfamily_ifops_register(const struct netfamily_ifops *nfo);
int     netfamily_ifops_unregister(const struct netfamily_ifops *nfo);
int     netfamily_if_event(int event, struct ifnet *ifp);
int     netfamily_control(int family, struct socket *so, int cmd, caddr_t data, struct ifnet *ifp);
/* an event for one family alone; 0 when it is not part of the running stack */
int     netfamily_if_event_to(int family, int event, struct ifnet *ifp);
/* the family's answer (0 or 1); -1 when it is not part of the running stack or does not answer */
int     netfamily_addrquery(int family, int what, const struct sockaddr *sa, struct sockaddr *res);
/*
 * Whether the address family is part of the running stack (its socket
 * domain is attached): 1 or 0. What the core asks before it offers
 * something of a family - an address from getaddrinfo(), a setting of an
 * interface line - that only the family could make use of.
 */
int     netfamily_running(int family);

int     netvar_register(const struct netvar *tab, int count);
int     netvar_unregister(const struct netvar *tab);
const struct netvar *netvar_find(const char *name);
/*
 * By sysctl name. *known is set when anything at all is registered under
 * net.<family>.<proto>.
 */
const struct netvar *netvar_find_mib(int family, int proto, int id, int *known);

/*
 * A periodic call for a module: `fn' is called every `ticks' ticks (hz to
 * the second, sys/kernel.h) in the stack's task, at splsoftclock, from when
 * the stack's timers run. The handle is what netperiodic_remove() takes;
 * NULL when it could not be made. Add and remove with the stack fenced, as
 * modules are started and stopped - and never remove from inside `fn'.
 * (kern/amiga_time.c)
 */
void   *netperiodic_add(void (*fn)(void), unsigned long ticks);
void    netperiodic_remove(void *handle);

/*
 * The core's coarse clock: counts the protocols' slow timeouts, PR_SLOWHZ
 * to the second (sys/protosw.h), from when the stack's timers run. For
 * what only needs "how many half seconds since": rate limits, ageing.
 * Read it; only the core's timer adds to it. (kern/amiga_time.c)
 */
extern unsigned long net_slowticks;

/*
 * A family has changed an address while the signal that tells the
 * applications of it (SBTC_ADDRCHANGE...) could not be sent: have it sent
 * from the stack's task now. (api/amiga_api.c)
 */
void    api_addrchange_retry(void);

/* EEXIST: another module's deadlines are being served. (kern/amiga_time.c) */
int     netdeadline_register(const struct netdeadline *nd);
int     netdeadline_unregister(const struct netdeadline *nd);

int     nettunnel_register(const struct nettunnel *nt);
int     nettunnel_unregister(const struct nettunnel *nt);
/* by kind; NULL for the name: the first kind registered. NULL: none */
const struct nettunnel *nettunnel_find(const char *name);
/* the reload's side of nt_seen/nt_prune above: for every registered kind */
void    nettunnel_seen(struct ifnet *ifp, ULONG gen);
void    nettunnel_prune(ULONG gen);


/*
 * Named interfaces (struct netiface above). netiface_publish(): EINVAL
 * (no name, no owner, a size smaller than the head), EEXIST (the name is
 * taken), ENOSPC. netiface_withdraw(): ENOENT when it is not published.
 * netiface_fetch(): the table, or NULL - not published, too old or too
 * small for `version' and `size', or `user' does not name the publisher
 * in its nm_requires; the reason is logged.
 */
int     netiface_publish(struct netmodule *owner, struct netiface *nif);
int     netiface_withdraw(struct netiface *nif);
const struct netiface *netiface_fetch(struct netmodule *user, const char *name,
                                      unsigned int version, unsigned long size);

int     netmodule_register(struct netmodule *nm);
/*
 * Stop a module: ENOENT (not started), EPERM (NMF_PERMANENT, no nm_stop),
 * EBUSY (it has users - netmodule_users() - or it is the last
 * NMF_SELECTABLE module that runs: a running stack keeps one protocol
 * family, as its start-up demands one), or what nm_can_stop or nm_stop
 * answered. Nothing is changed by a refusal.
 */
int     netmodule_unregister(struct netmodule *nm);
struct netmodule *netmodule_find(const char *name);
void    netmodule_start_builtin(void);
/*
 * A user, other than a socket, of the module whose socket domain is of
 * address family `family': delta 1 takes, -1 gives back. For a module that
 * uses a family through the core's registrations and so cannot name the
 * family's module in nm_requires (the 6in4 tunnel: IPv6 does not need
 * IPv4, a tunnel interface does). While the count is held the family is
 * not stopped (EBUSY, the control port reports the users). ENOENT: the
 * family is not part of the running stack.
 */
int     netmodule_hold_family(int family, int delta);

/*
 * Which protocol modules the stack runs: the configuration item PROTOCOLS
 * (db/general.config: PROTOCOLS=INET,INET6), a list of module names
 * separated by commas, in any letter case. NULL - the item is absent -
 * stands for NETMODULE_PROTOCOLS_DEFAULT. netmodule_start_builtin() starts
 * each: a module of that name linked into the stack binary, else the
 * library <name>NETMODULE_KIND_PROT (inet.prot), loaded with
 * netmodule_load(). What a module needs (nm_requires: the transports, a
 * family) is started for it, found the same way. netmodule_protocols_started():
 * how many of the selected ones run.
 */
#define NETMODULE_PROTOCOLS_DEFAULT "INET,INET6"
/*
 * THE NAMES OF THE MODULE LIBRARIES, for the running stack: this place and
 * no other in the C sources. (For the build: STACK_MODULE_PROT,
 * STACK_MODULE_ATCP and STACK_MODULE_DIR in stack.cfg, which must say the
 * same. The stack's own library for modules is NETCORE_LIBNAME,
 * net/netcore.h.)
 *
 * A module <name> is the file <name><kind>, the name in small letters; the
 * file's name is also the name exec lists the library under. The kind says
 * what the module is, and the caller knows it from where the name comes:
 *   NETMODULE_KIND_PROT  a protocol module - what PROTOCOLS names and what
 *                        a module needs (nm_requires): inet.prot,
 *                        inet6.prot, transport.prot
 *   NETMODULE_KIND_ATCP  every other module of the stack - loaded on first
 *                        use, by MODULES, or for the configuration:
 *                        resolver.atcp, admin.atcp, packet.atcp,
 *                        netconfig.atcp, demo.atcp
 */
#define NETMODULE_KIND_PROT         ".prot"
#define NETMODULE_KIND_ATCP         ".atcp"
/* the directory, next to the stack's C, S and db directories, in which a
 * module library named without a path is looked for first */
#define NETMODULE_LIBDIR            "Libs"
extern char *netmodule_protocols;
int     netmodule_protocols_started(void);
/*
 * The optional modules to have running from the start instead of from
 * their first use (net/netdemand.h): the configuration item MODULES
 * (db/general.config), a list of module names written as PROTOCOLS is;
 * NULL - the item is absent, the default - loads none at start.
 * netmodule_start_listed() starts or loads each, in the stack's task
 * before the API is shown; a name that cannot be had is an error in the
 * log, and the stack goes on without it.
 */
extern char *netmodule_modules;
void    netmodule_start_listed(void);
/* the library file a module of that name and kind (NETMODULE_KIND_*) is
 * looked for in; 0 when buf is too small */
int     netmodule_libname(const char *name, int namelen, const char *kind,
                          char *buf, int buflen);

/*
 * A module whose code is linked into the stack binary puts this next to its
 * descriptor. It is then known by name from when the stack starts, and
 * netmodule_start_builtin() starts those the configuration selects, in
 * ascending `order' (10 IPv6, 20 IPv4: the order the domains have always
 * been initialised in), and whatever they need. The core does not name
 * it: the descriptors are collected by the linker (an AROS symbol set). A
 * module in a library file of its own does not use this - it is
 * registered when it is loaded.
 */
#include <aros/symbolsets.h>
/*
 * A protocol group's descriptor is written once and serves both builds:
 *
 *      NETMODULE_DESCRIPTOR(inet_module) = { ... };
 *      NETMODULE_BUILTIN(inet_module, 20)
 *
 * Built into the stack program it is a static structure in the set; built
 * into a library file of its own (-DNETMODULE) it is `netmodule_this', the
 * one name the library's glue (net/netmodule_lib.c) hands to the stack,
 * and it is in no set: the same module cannot be both part of the program
 * and loaded (netmodule_load() answers EEXIST for a name that is known).
 */
#ifdef NETMODULE
#define NETMODULE_DESCRIPTOR(name)      struct netmodule netmodule_this
#define NETMODULE_BUILTIN(nm, order)
#else
#define NETMODULE_DESCRIPTOR(name)      static struct netmodule name
#define NETMODULE_BUILTIN(nm, order)    ADD2SET(nm, NETMODULES, order)
#endif

/*
 * Modules whose code is in the stack binary but which are started on first
 * use and can be stopped: known by name whether started or not.
 * netmodule_use() is called in the stack's task only, as every start is; a
 * call of an application that needs a module started has it done there
 * through netdemand_load() (net/netdemand.h).
 */
void    netmodule_know(struct netmodule *nm);
struct netmodule *netmodule_known(const char *name);
int     netmodule_use(struct netmodule *nm);    /* start it if it is not running */
long    netmodule_users(struct netmodule *nm);

/*
 * arostcp.library: what the core offers a module in a library file of its
 * own (net/netcore.h, kern/net_services.c). Published while the stack
 * starts, before any module is; taken back when the stack ends, after the
 * last module library was unloaded.
 */
int     netcore_init(void);         /* 0 when the library could not be made or published */
void    netcore_deinit(void);

/*
 * TEST SWITCH (see nm_leaveout): the name of the global variable that
 * keeps the module called `name' out of the running stack, written to
 * `buf'. Decided from the name alone, so that it is known before a library
 * file is opened: AROSTCP/WithoutIPv4 for "inet", AROSTCP/WithoutIPv6 for
 * "inet6" (the two the test harness has always used), AROSTCP/Without_<name>
 * for any other. netmodule_leftout(): not 0 when that variable exists.
 */
void    netmodule_leaveout_name(const char *name, int namelen, char *buf, int buflen);
int     netmodule_leftout(const char *name, int namelen);

/*
 * The stack is ending (kern/amiga_main.c deinit_all(), after the
 * interfaces are closed and the timers deleted): call nm_shutdown of every
 * module that is still started, the one that needs another first; then
 * forget every module and everything modules registered with the core, so
 * that no pointer into a module is left where the rest of the shutdown
 * could follow it. After it netmodule_expunge_all() removes the module
 * libraries from memory.
 */
void    netmodule_shutdown_all(void);
/*
 * Before that, while the stack is still whole (interfaces open, timers
 * running, no socket left): stop every started module that lets itself be
 * stopped, by its own nm_stop, the ones that need another first. What
 * refuses or fails is named in the log and left for the cut-off above.
 */
void    netmodule_stop_all(void);
/* the same for the registrations kept outside kern/net_module.c */
void    netinput_forget_all(void);          /* net/netisr.c */
void    domain_forget_all(void);            /* kern/uipc_domain.c */

/*
 * WHERE MODULE LIBRARIES ARE (the one place that says it). A module
 * library is installed in
 *
 *      SYS:System/Network/AROSTCP/Libs/<name>.prot   (or <name>.atcp)
 *
 * (stack.cfg: STACK_MODULE_DIR, STACK_MODULE_PROT, STACK_MODULE_ATCP;
 * NETMODULE_KIND_* above), that is in the
 * directory NETMODULE_LIBDIR next to the directory the stack program was
 * started from (.../AROSTCP/C). netmodule_load() looks there first for a name
 * given without a path; when the file is not there, where OpenLibrary()
 * looks for any library: the stack's current directory and its Libs, the
 * program directory and its Libs, then LIBS:. A name with a path is
 * opened from exactly there.
 */

/*
 * Modules in library files (kern/net_modload.c). Call in the stack's task;
 * both take the fence (syscall_semaphore) themselves for the part that
 * touches the stack.
 *
 * netmodule_load(): open the library, attach it to the core, register and
 * start its module (after the modules it needs). `libname' is the name of
 * the file; one without a path is looked for in the stack's own Libs
 * directory first, then where OpenLibrary() looks. 0, or an errno with a
 * logged message that names the library: ENOENT no such file (or it would
 * not open), EEXIST loaded already, or a module of that name is part of the
 * stack, EPROTONOSUPPORT the library and the core are not made for each
 * other, or what the module's start failed with. On an error nothing stays
 * loaded.
 *
 * netmodule_unload(): stop the module if it runs - refused, never forced,
 * as netmodule_unregister() is (EBUSY, EPERM) - then detach it from the
 * core, and close and expunge its library. EINVAL for a module that did
 * not come from a library file.
 */
int     netmodule_load(const char *libname, struct netmodule **nmp);
/*
 * The same for a library that need not be installed: when the file cannot
 * be opened the line in the log is a notice, not an error (ENOENT is
 * answered all the same).
 */
int     netmodule_load_optional(const char *libname, struct netmodule **nmp);
/*
 * Open and attach a module library and leave its module NOT started: for
 * the registry, which must read a needed module's descriptor before it
 * decides whether to start it (netmodule_take_requires()). The module is
 * then known by name (netmodule_loaded()); the caller starts it with
 * netmodule_register() or sends it away with netmodule_unload(). Errors as
 * netmodule_load(), with a notice instead of an error for a file that is
 * not there.
 */
int     netmodule_attach(const char *libname, struct netmodule **nmp);
int     netmodule_unload(struct netmodule *nm);
/* a loaded module by name, started or not; NULL when there is none */
struct netmodule *netmodule_loaded(const char *name);
/* ... and by the name of its library file (no path) */
struct netmodule *netmodule_loaded_file(const char *file);
/*
 * At shutdown, in two steps (kern/amiga_main.c deinit_all()).
 * netmodule_unload_all(): while the stack is still whole, stop and unload
 * every module that lets itself be stopped. netmodule_expunge_all(): once
 * nothing can call a module any more (netmodule_shutdown_all() has run),
 * detach every library that is left and remove it from memory, started or
 * not - its static data would be stale for the next start of the stack.
 */
void    netmodule_unload_all(void);
void    netmodule_expunge_all(void);
#endif

/* modules loaded on first use, ioctl numbers and socket domains served by a module */
#include <net/netdemand.h>

#endif /* !NET_NETMODULE_H */
