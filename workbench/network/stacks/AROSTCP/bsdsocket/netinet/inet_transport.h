/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef NETINET_INET_TRANSPORT_H
#define NETINET_INET_TRANSPORT_H

/*
 * How a family comes by its transports, and a transport by its families.
 *
 * A transport - TCP, UDP, the raw sockets, one added later - is written
 * once, for every family (netinet/inetfamily.h). It makes itself known
 * with ONE descriptor, registered with the transports' common part
 * (netinet/inet_transport.c). A family names no transport: when it
 * starts it binds to this registry and is told each registered transport,
 * of which it makes an entry of its protocol table - the socket type, the
 * protocol number and the user request are the transport's, the input is
 * the family's one adapter, which hands the packet on with the family's
 * instance of the family interface. A transport registered after a family
 * was bound is told to that family then, and one that is taken back is
 * taken out of every family's table: a transport loaded later serves the
 * families that are running.
 *
 * Ownership: a descriptor belongs to the transport and stays valid, at
 * the same address, until inet_transport_unregister() has returned; a
 * binding belongs to the family, likewise until inet_transport_unbind().
 * Neither is copied. All four calls are made with the stack fenced, as
 * modules are started and stopped (net/netmodule.h).
 */
#include <net/netmodule.h>          /* struct netiface */

struct mbuf;
struct socket;
struct inetfamily;
struct inctlparam;

struct inet_transport {
    unsigned short tr_size;     /* sizeof(struct inet_transport) */
    unsigned short tr_flags;    /* TRF_xxx */
    short       tr_type;        /* SOCK_xxx */
    short       tr_protocol;    /* its protocol number; 0 with TRF_RAW */
    short       tr_prflags;     /* PR_xxx of its protocol table entries (sys/protosw.h) */

    /*
     * A packet for the transport has arrived by family inf: m starts at
     * the network header, the transport header is at off (see inf_rcvinfo,
     * netinet/inetfamily.h). Takes m. NULL with TRF_RAW.
     */
    void      (*tr_input)(struct mbuf *m, int off, const struct inetfamily *inf);
    /*
     * What a family's ICMP says about a packet of the transport that came
     * back quoted in an error, or (addresses only) about a destination.
     * NULL: the transport does not want to know.
     */
    void      (*tr_ctlnotify)(int cmd, const struct inctlparam *cp);
    /*
     * The socket options of a socket of the transport, of every level.
     * NULL: it has none of its own, and the family's (inf_ctloutput) are
     * its sockets' options.
     */
    int       (*tr_ctloutput)(int op, struct socket *so, int level, int optname,
                              struct mbuf **m);
    /* the user requests of a socket of the transport */
    int       (*tr_usrreq)(struct socket *so, int req, struct mbuf *m,
                           struct mbuf *nam, struct mbuf *control);

    /* ---- TRF_RAW only ---- */

    /*
     * Hand a packet of family inf to the raw sockets that want protocol
     * proto: m from the network header, off where the header of proto
     * begins; flags: RIP_xxx (netinet/in_pcb.h). Returns the number of
     * sockets that took it. Takes m.
     */
    int       (*tr_deliver)(struct mbuf *m, int off, int proto,
                            const struct inetfamily *inf, int flags);
    /* pr_output of the raw protocols' table entries */
    int       (*tr_output)(void *args, ...);
};

/* tr_flags */
#define TRF_RAW     0x0001  /* the raw sockets: no protocol number of its own - a family
                             * makes its raw protocol entries of it (the numbers nobody
                             * took), and the sockets of its own protocols (ICMP ...) */
#define TRF_BAREHDR 0x0002  /* tr_input wants the network header without options:
                             * a family that has them in its header removes them first */
#define TRF_PORTS   0x0004  /* its header begins with the source and the destination
                             * port, 16 bits each: a family's ICMP quotes them to
                             * tr_ctlnotify (icp_lport, icp_fport) */
#define TRF_SEQ     0x0008  /* ... followed by a 32-bit sequence number (icp_seq) */

/*
 * A family's side: how it is told the transports. tb_add is called for
 * each transport registered now and later; 0, or an errno when the family
 * cannot take it (no room in its table, the protocol number is another
 * protocol's): the transport then does not serve that family, which is
 * logged, and serves the others. tb_remove is called when a transport is
 * taken back - no socket of it exists then - and must leave nothing of
 * the family pointing at the descriptor.
 */
struct inet_trbinding {
    int         tb_family;      /* AF_xxx, for messages */
    int       (*tb_add)(const struct inet_transport *tr);
    void      (*tb_remove)(const struct inet_transport *tr);
};

/*
 * THE TRANSPORTS' INTERFACE FOR OTHER MODULES. The transports and the
 * families are separate modules - separate library files - and cannot
 * call one another by name. The transport module publishes this table as
 * the named interface INET_TRANSPORT_IFNAME (struct netiface,
 * net/netmodule.h) when it starts; a family fetches it once, in its own
 * nm_start, and makes the few calls below through it. They are made when a
 * module starts or stops, never for a packet: per packet a family calls a
 * transport through the descriptor it was told (tr_input ...), and a
 * transport the family through the family's instance (struct inetfamily).
 *
 * Version 2 (1: without ti_family_pcbs). The table grows at the end only.
 */
struct inetfamily;
struct inpcb;

struct inet_transport_if {
    struct netiface ti_if;      /* size, version, name */
    /* a family's instance of the family interface (netinet/inetfamily.h) */
    int       (*ti_family_register)(const struct inetfamily *inf);
    int       (*ti_family_unregister)(const struct inetfamily *inf);
    /* a family asks to be told the transports, and stops asking */
    int       (*ti_bind)(const struct inet_trbinding *tb);
    int       (*ti_unbind)(const struct inet_trbinding *tb);
    /* a transport in a module of its own makes itself known, and takes itself back */
    int       (*ti_register)(const struct inet_transport *tr);
    int       (*ti_unregister)(const struct inet_transport *tr);
    /*
     * (version 2) What a family asks before it stops: how many PCBs of
     * the transports - of every transport, of sockets of every family -
     * still use it. One that IS of the family (inp_family: its socket may
     * be another family's - the IPv4 connection of an AF_INET6 socket that
     * serves both), and one for which `holds' answers not 0: a PCB of
     * another family that carries something of this family's (the socket
     * options are found by their level, so an AF_INET6 socket can have
     * joined an IPv4 group). `holds' may be NULL. The family's socket
     * domain counts only its own sockets; while this is not 0 the family
     * must refuse to stop, or a PCB would be left pointing at it.
     */
    long      (*ti_family_pcbs)(const struct inetfamily *inf,
                                int (*holds)(const struct inpcb *inp));
};

#define INET_TRANSPORT_IFNAME       "transport.inet"
#define INET_TRANSPORT_IFVERSION    2

#define INET_TRANSPORT_MAX  6   /* registered transports at one time */
#define INET_TRBINDING_MAX  4   /* bound families at one time (INETFAMILY_MAX) */

#ifdef KERNEL
/*
 * The functions behind the table, for the files of the transport module
 * itself (netinet/inet_transport.c, netinet/in_pcb.c). A family does not
 * call them by these names: it is not linked with them.
 *
 * EINVAL: not a usable descriptor; EEXIST: registered already, or another
 * transport has the socket type and protocol number; ENOSPC.
 */
int     inet_transport_register(const struct inet_transport *tr);
/*
 * The caller - the transport - has made sure that no socket of it exists
 * (its PCB list is empty). ENOENT: not registered.
 */
int     inet_transport_unregister(const struct inet_transport *tr);

/* a family binds: tb_add is called for every registered transport, in the
 * order they were registered in */
int     inet_transport_bind(const struct inet_trbinding *tb);
/* ... and lets go: tb_remove is NOT called - the family takes its table
 * down itself */
int     inet_transport_unbind(const struct inet_trbinding *tb);

/*
 * For the transport module's stop and its answers to a family that stops
 * (netinet/inet_transport.c); each in the file that has the list.
 */
struct inpcbhead;
/* PCBs on the list that are of family inf, or for which holds() answers not 0 */
long    in_pcbcount(const struct inpcbhead *head, const struct inetfamily *inf,
                    int (*holds)(const struct inpcb *inp));    /* in_pcb.c */
long    rip_pcbcount(const struct inetfamily *inf,
                     int (*holds)(const struct inpcb *inp));   /* raw_ip.c */
void    inetfamily_forget(void);        /* in_pcb.c: no family is registered */
const struct inetfamily *inetfamily_any(void);  /* in_pcb.c: one that is, or NULL */
void    udp_deinit(void);               /* udp_usrreq.c: undoes udp_init() */
void    rip_deinit(void);               /* raw_ip.c */
#endif

#endif /* !NETINET_INET_TRANSPORT_H */
