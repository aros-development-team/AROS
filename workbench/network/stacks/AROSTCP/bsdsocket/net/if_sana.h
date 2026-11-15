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

#ifndef IF_SANA_H
#define IF_SANA_H

#ifndef DEVICES_SANA_H
#include <devices/sana2.h>
#endif

#ifndef IF_ARP_H
#include <net/if_arp.h>
#endif

/* A prefix added to the SANA-II device name if needed */
#define NAME_PREFIX "networks/"

/*
 * Our Special SANA-II request
 */
struct IOIPReq {
  struct IOSana2Req  ioip_s2;	
#define ioip_ReplyPort  ioip_s2.ios2_Req.io_Message.mn_ReplyPort
#define ioip_Command ioip_s2.ios2_Req.io_Command
#define ioip_Error   ioip_s2.ios2_Req.io_Error
  struct sana_softc *ioip_if;	      /* pointer to network interface */
                     /* request dispatch routine */
  void             (*ioip_dispatch)(struct sana_softc *, struct IOIPReq *); 
  struct mbuf       *ioip_reserved;   /* reserved for packet */
  struct mbuf       *ioip_packet;     /* packet */
  struct IOIPReq    *ioip_next;	      /* allocation queue */
};

/*
 * A socket address for a generic SANA-II host
 */
#define MAXADDRSANA 16

struct sockaddr_sana2 {
  u_char  ss2_len;
  u_char  ss2_family;
  u_long  ss2_type;
  u_char  ss2_host[MAXADDRSANA];
};

/*
 * Interface descriptor
 *	NOTE: most of the code outside will believe this to be simply
 *	a "struct ifnet". The other information is, on the other hand,
 *	our own business.
 */
/*
 * A link packet type that is listened to on SANA-II interfaces, and what
 * an interface keeps for it. A family registers its type(s) with
 * sana_linktype_register(); when an interface is configured, each type is
 * given its wire type number and the number of read requests to keep with
 * the driver for that kind of hardware; when the interface goes online the
 * requests are sent, and each packet read comes back to lt_read().
 *
 * The first three entries are IPv4, ARP and IPv6, at fixed places so that
 * the stack's own paths reach them without a look-up. A type registered
 * later is given the next place; it is set up on interfaces configured
 * after that (or at the next reload).
 */
struct sana_softc;
struct IOIPReq;
struct ifnet;
struct rtentry;
struct mbuf;
struct sockaddr;

struct sana_ptype {
    UWORD reqno;                /* read requests to keep with the driver */
    UWORD sent;                 /* of which are with it now */
    ULONG type;                 /* wire packet type */
};

struct sana_linktype {
    const char *lt_name;        /* for messages: "IP", "ARP", ... */
    int         lt_family;      /* AF_xxx it carries, AF_UNSPEC if none */
    /* a packet was read: take it with sana_read() and pass it on */
    void      (*lt_read)(struct sana_softc *, struct IOIPReq *);
    /*
     * An interface is being configured: fill in the wire type and the
     * request count for its hardware (ss_hwtype); a count of 0 leaves the
     * type unused there. NULL for the three built-in types, which the
     * configuration sets itself.
     */
    void      (*lt_config)(struct sana_softc *, struct sana_ptype *);
    ULONG       lt_mtu;         /* read buffer size; 0 = the interface's MTU */
    int         lt_index;       /* set by sana_linktype_register() */
    /*
     * Find the link address of a packet's next hop (the family's address
     * resolution). 0: dst_hw is filled in, send. EAGAIN: the packet was
     * kept and is sent when the address is known. Else an errno, and the
     * packet is the caller's to free. NULL when the family has none.
     */
    int       (*lt_resolve)(struct ifnet *, struct rtentry *, struct mbuf *,
                            struct sockaddr *, u_char *);
    /*
     * For a type that is not queued for a family (lt_family is AF_UNSPEC;
     * ARP is one): each packet read is handed over at once, with the link
     * address it came from. Set by the module that speaks the protocol
     * (sana_linktype_handler()); while it is NULL the type is not read.
     */
    void      (*lt_input)(struct sana_softc *, struct mbuf *, caddr_t);
};

/*
 * Set on a packet by a family's resolver (lt_resolve) to have it sent
 * ahead of the others: the driver's request is given a higher message
 * priority. (IPv4 sets it for the low-delay type of service.) An mbuf
 * packet-header flag that only sana_output() looks at.
 */
#define M_LINKPRI   0x0400

#define SANA_PT_IP      0
#define SANA_PT_ARP     1
#define SANA_PT_IP6     2
#define SANA_PT_MAX     8       /* link types at one time */

#define ss_ip   ss_pt[SANA_PT_IP]
#define ss_arp  ss_pt[SANA_PT_ARP]
#define ss_ip6  ss_pt[SANA_PT_IP6]

struct sana_softc {
  struct ifnet       ss_if;	      /* network-visible interface */
  struct in_addr     ss_ipaddr;	      /* copy of ip address */
  ULONG              ss_hwtype;	      /* wiretype */
  UBYTE              ss_hwaddr[MAXADDRSANA]; /* General hardware address */
  struct Device      *ss_dev;	      /* pointer to device */
  struct Unit        *ss_unit;	      /* pointer to unit */
  VOID               *ss_bufmgnt;	      /* magic cookie for buffer management */
  UWORD		         ss_reqno;	      /* # of requests to allocate */
  UWORD              ss_cflags;	      /* configuration flags */
  struct IOIPReq     *ss_reqs;	      /* allocated requests */
  struct MinList     ss_freereq;	      /* free requests */
  struct IOIPReq     *ss_connectreq;  /* request for connect event */
  struct sana_ptype  ss_pt[SANA_PT_MAX]; /* per link type: see struct sana_linktype */
#if	INET
  ULONG              ss_arp_hrd;	      /* ARP header type */
  struct arptable    *ss_arp_table;   /* ARP/IP table */
  struct {                          /* IPv4LL / RFC 3927 state */
    UBYTE          state;           /* AUTOIP_xxx state */
    UBYTE          wanted;          /* IP=AUTO: run again when the link is back */
    UBYTE          ticks;           /* 1-second countdown (AUTOIP_WAIT) */
    UBYTE          conflicts;       /* conflict counter */
    struct in_addr candidate;       /* the address claimed */
  } ss_autoip;
#endif	/* INET */
#if	ISO
  UWORD           ss_isoreqno;	      /* for iso */
  UWORD           ss_isosent;
  ULONG           ss_isotype;
#endif	/* ISO */
#if	CCITT
  UWORD           ss_ccittreqno;      /* for ccitt */
  UWORD           ss_ccittsent;
  ULONG           ss_ccitttype;
#endif	/* CCITT */
#if	NS
  UWORD           ss_nsreqno;	      /* for ns */
  UWORD           ss_nssent;	
  ULONG           ss_nstype;
#endif	/* NS */
  UWORD           ss_rawreqno;	      /* for raw packets */
  UWORD           ss_rawsent;
  UWORD           ss_eventsent;	      /* sent event requests */
  UWORD           ss_maxmtu;	      /* limit given by device */
  UBYTE          *ss_execname;
  ULONG           ss_execunit;
  UBYTE           ss_name[IFNAMSIZ];
  ULONG           ss_reloadseen;      /* reload generation this iface was last in config */
  struct sana_softc *ss_next;
};

/*
 * Configuration flags
 */
#define SSF_TRACK (1<<0)	      /* Should we track packets? */
#define SSB_TRACK 0

/*
 * Set while the interface is being configured. They are not in if_flags:
 * every bit there has a meaning of its own (0x8000 is IFF_MULTICAST).
 */
#define SSF_NOUP    (1<<1)	      /* do not put online with the address */
#define SSF_DELAYUP (1<<2)	      /* configure once the API is shown */

/* Default configuration flags */
#define SS_CFLAGS (SSF_TRACK)

/*
 * Global functions defined in if_sana.c
 */
int sana_output(struct ifnet *ifp, struct mbuf *m0,
			struct sockaddr *dst, struct rtentry *rt);
int sana_ioctl(register struct ifnet *ifp, int cmd, caddr_t data);
/* start the device and, with `online', put the interface online: for the
 * configuration, whatever families the stack runs (net/if_sana.c) */
int sana_bringup(struct ifnet *ifp, int online);
/*
 * For the configuration reader (kern/netconfig_iface.c), which makes the
 * interfaces of the configuration and removes those a reload no longer
 * names: the interface's query function; taking an interface offline
 * (sana_down()) and back its requests (sana_unrun()); its addresses,
 * routes and DNS servers dropped (sana_if_deconfigure(), the count of
 * addresses); the generation of the last configuration read.
 */
int  sana_query(struct ifnet *ifn, struct TagItem *tag);
BOOL sana_down(struct sana_softc *ssc);
void sana_unrun(struct sana_softc *ssc);
int  sana_if_deconfigure(struct ifnet *ifp);
extern ULONG sana_reload_gen;

/*
 * The Ethernet header a SANA-II driver adds and strips, as BPF captures
 * and bpf_write() takes it (destination, source, type).
 */
#define SANA_ETHER_ADDRLEN  6
#define SANA_ETHER_HDRLEN   14

/*
 * The SANA-II interface behind an ifnet, or NULL for any other kind: a
 * tunnel and the loopback are a struct ifnet with something else behind it.
 */
#define sana_softc_of(ifp) \
    (((struct ifnet *)(ifp))->if_output == sana_output ? (struct sana_softc *)(ifp) : NULL)

/*
 * Whether an IPv4 group has a link address on this interface, which is
 * true of Ethernet alone (RFC 1112 section 6.4). The one test for it:
 * IFF_MULTICAST is set where it holds, the device is told of a group's
 * address where it holds, and arpresolve() gives that address where it
 * holds.
 *
 * (Both are macros, not inline functions, and evaluate `ifp' more than
 * once: an inline function in a header is compiled before a module's
 * mapping of the core's names exists (net/netcore_map.h), and would name
 * sana_output directly, which a module library cannot link.)
 */
#define sana_ether_mcast(ifp) \
    (sana_softc_of(ifp) != NULL && \
     ((struct sana_softc *)(ifp))->ss_hwtype == S2WireType_Ethernet)
/* queue for sana network interfaces */
extern struct sana_softc *ssq;
extern const struct sana_linktype *sana_linktypes[SANA_PT_MAX];
int  sana_linktype_register(struct sana_linktype *lt);
int  sana_linktype_unregister(struct sana_linktype *lt);
LONG sana_linktype_config(struct sana_softc *ssc);
/* the family started, or is stopping: its resolver for the link type that carries it */
int  sana_linktype_resolver(int family,
         int (*resolve)(struct ifnet *, struct rtentry *, struct mbuf *, struct sockaddr *, u_char *));
/*
 * The module that speaks the protocol of a built-in link type which is not
 * queued for a family started (or, with NULL, is stopping): where its
 * packets go and how large one can be. Interfaces that are online already
 * get their read requests of that type when they next come online.
 */
int  sana_linktype_handler(int index,
         void (*input)(struct sana_softc *, struct mbuf *, caddr_t), ULONG mtu);
/*
 * The read requests of ONE link type, on every interface, while the
 * others go on: what a family that stops or starts while the stack runs
 * needs (sana_unrun() takes back every request of an interface, and the
 * device with them).
 *
 * sana_linktype_quiesce(): abort and collect the requests of type `index'
 * that are with the drivers or replied and not yet looked at, free what
 * they hold and put them back with the interface's free requests. After it
 * no read completion of the type is delivered, and none is sent again
 * until sana_linktype_resume(). Returns how many were taken back.
 *
 * sana_linktype_resume(): the reverse, for the interfaces that are online
 * (they have reads of another type out): the type's requests are sent as
 * they are when an interface comes online, under the same conditions (a
 * family's type: its input queue is registered; another: its input is
 * set). An interface with no read out at all gets them when it is next
 * brought up. Returns how many were sent.
 *
 * Both in the stack's task with the stack fenced, as a module is stopped
 * and started; never from a read completion.
 */
int  sana_linktype_quiesce(int index);
int  sana_linktype_resume(int index);

/*
 * Tell the device of `ifp' to accept (add != 0) or stop accepting (add == 0)
 * the link-level multicast address `lladdr' (6 bytes). A family maps its
 * group to that address (RFC 1112 6.4, RFC 2464 7) and leaves the device
 * request to this, the one copy of it. Only Ethernet interfaces have such
 * an address (sana_ether_mcast()); for any other interface it does nothing
 * and returns 0. ENOBUFS without a request, EIO when the device refuses.
 * Synchronous (DoIO).
 */
int  sana_mcast(struct ifnet *ifp, const u_int8_t *lladdr, int add);

#endif /* of IF_SANA_H */
