#ifndef _NET_IF_H_
#define _NET_IF_H_
/*
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 2005 - 2006
 *	Pavel Fedin
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
 *	@(#)if.h	8.3 (Berkeley) 2/9/95
 */

/*
 * Structures defining a network interface, providing a packet
 * transport mechanism (ala level 0 of the PUP protocols).
 *
 * Each interface accepts output datagrams of a specified maximum
 * length, and provides higher level routines with input datagrams
 * received from its medium.
 *
 * Output occurs when the routine if_output is called, with three parameters:
 *	(*ifp->if_output)(ifp, m, dst, rt)
 * Here m is the mbuf chain to be sent and dst is the destination address.
 * The output routine encapsulates the supplied datagram if necessary,
 * and then transmits it on its medium.
 *
 * On input, each interface unwraps the data received by it, and either
 * places it on the input queue of a internetwork datagram routine
 * and posts the associated software interrupt, or passes the datagram to a raw
 * packet input routine.
 *
 * Routines exist for locating interfaces by their addresses
 * or for locating a interface on a certain network, as well as more general
 * routing and gateway routines maintaining information used to locate
 * interfaces.  These routines live in the files if.c and route.c
 */
 
#include <utility/tagitem.h>
#include <sys/cdefs.h>

#define	IFNAMSIZ	16


#ifndef KERNEL
#ifdef __STDC__
/*
 * Forward structure declarations for function prototypes [sic].
 */
struct	mbuf;
struct	proc;
struct	rtentry;	
struct	socket;
struct	ether_header;
#endif
#endif

/*
 * Structure describing information about an interface
 * which may be of interest to management entities.
 */
/*
 * Structure defining a queue for a network interface.
 *
 * (Would like to call this struct ``if'', but C isn't PL/1.)
 */

struct if_data {
/* generic interface information */
	u_char	ifi_type;	/* ethernet, tokenring, etc */
	u_char	ifi_addrlen;	/* media address length */
	u_char	ifi_hdrlen;	/* media header length */
	u_long	ifi_mtu;	/* maximum transmission unit */
	u_long	ifi_metric;	/* routing metric (external only) */
	u_long	ifi_baudrate;	/* linespeed */
/* volatile statistics */
	u_long	ifi_ipackets;	/* packets received on interface */
	u_long	ifi_ierrors;	/* input errors on interface */
	u_long	ifi_opackets;	/* packets sent on interface */
	u_long	ifi_oerrors;	/* output errors on interface */
	u_long	ifi_collisions;	/* collisions on csma interfaces */
	u_long	ifi_ibytes;	/* total number of octets received */
	u_long	ifi_obytes;	/* total number of octets sent */
	u_long	ifi_imcasts;	/* packets received via multicast */
	u_long	ifi_omcasts;	/* packets sent via multicast */
	u_long	ifi_iqdrops;	/* dropped on input, this interface */
	u_long	ifi_noproto;	/* destined for unsupported protocol */
	struct	timeval ifi_lastchange;/* last updated */
/* AROSTCP/MOSNet specific additions */
   struct   timeval ifi_aros_ontime;
   u_quad_t ifi_aros_lasttotal;
   u_char   ifi_aros_usedhcp;
   pid_t    ifi_aros_dhcp_pid;
   char     ifi_aros_dhcp_args[IFNAMSIZ + 5];
};

struct ifnet {
	char	*if_name;		/* name, e.g. ``en'' or ``lo'' */
	struct	ifnet *if_next;		/* all struct ifnets are chained */
	struct	ifaddr *if_addrlist;	/* linked list of addresses per if */
		int	if_pcount;		/* number of promiscuous listeners */
	caddr_t	if_bpf;			/* packet filter structure */
	u_short	if_index;		/* numeric abbreviation for this if  */
	short	if_unit;		/* sub-unit for lower level driver */
	short	if_timer;		/* time 'til if_watchdog called */
	short	if_flags;		/* up/down, broadcast, etc. */
	struct	if_data if_data;
/* procedure handles */
	int	(*if_init)		/* init routine */
		__P((int));
	int	(*if_output)		/* output routine (enqueue) */
		__P((struct ifnet *, struct mbuf *, struct sockaddr *,
		     struct rtentry *));
	int	(*if_start)		/* initiate output routine */
		__P((struct ifnet *));
	int	(*if_done)		/* output complete routine */
		__P((struct ifnet *));	/* (XXX not used; fake prototype) */
	int	(*if_ioctl)		/* ioctl routine */
		__P((struct ifnet *, int, caddr_t));
	int	(*if_reset)	
		__P((int));		/* new autoconfig will permit removal */
	int	(*if_watchdog)		/* timer routine */
		__P((int));
	int	(*if_query)
		__P((struct ifnet *, struct TagItem *));
	struct	ifqueue {
		struct	mbuf *ifq_head;
		struct	mbuf *ifq_tail;
		int	ifq_len;
		int	ifq_maxlen;
		int	ifq_drops;
	} if_snd;			/* output queue */
};
#define	if_mtu		if_data.ifi_mtu
#define	if_type		if_data.ifi_type
#define	if_addrlen	if_data.ifi_addrlen
#define	if_hdrlen	if_data.ifi_hdrlen
#define	if_metric	if_data.ifi_metric
#define	if_baudrate	if_data.ifi_baudrate
#define	if_ipackets	if_data.ifi_ipackets
#define	if_ierrors	if_data.ifi_ierrors
#define	if_opackets	if_data.ifi_opackets
#define	if_oerrors	if_data.ifi_oerrors
#define	if_collisions	if_data.ifi_collisions
#define	if_ibytes	if_data.ifi_ibytes
#define	if_obytes	if_data.ifi_obytes
#define	if_imcasts	if_data.ifi_imcasts
#define	if_omcasts	if_data.ifi_omcasts
#define	if_iqdrops	if_data.ifi_iqdrops
#define	if_noproto	if_data.ifi_noproto
#define	if_lastchange	if_data.ifi_lastchange
/*-
 * Interface flags are of two types: network stack owned flags, and driver
 * owned flags.  Historically, these values were stored in the same ifnet
 * flags field, but with the advent of fine-grained locking, they have been
 * broken out such that the network stack is responsible for synchronizing
 * the stack-owned fields, and the device driver the device-owned fields.
 * Both halves can perform lockless reads of the other half's field, subject
 * to accepting the involved races.
 *
 * Both sets of flags come from the same number space, and should not be
 * permitted to conflict, as they are exposed to user space via a single
 * field.
 *
 * The following symbols identify read and write requirements for fields:
 *
 * (i) if_flags field set by device driver before attach, read-only there
 *     after.
 * (n) if_flags field written only by the network stack, read by either the
 *     stack or driver.
 * (d) if_drv_flags field written only by the device driver, read by either
 *     the stack or driver.
 */
#define	IFF_UP		0x1		/* (n) interface is up */
#define	IFF_BROADCAST	0x2		/* (i) broadcast address valid */
#define	IFF_DEBUG	0x4		/* (n) turn on debugging */
#define	IFF_LOOPBACK	0x8		/* (i) is a loopback net */
#define	IFF_POINTOPOINT	0x10		/* (i) is a point-to-point link */
#define	IFF_NEEDSEPOCH	0x20		/* (i) calls if_input w/o net epoch */
#define	IFF_DRV_RUNNING	0x40		/* (d) resources allocated */
#define	IFF_NOARP	0x80		/* (n) no address resolution protocol */
#define	IFF_PROMISC	0x100		/* (n) receive all packets */
#define	IFF_ALLMULTI	0x200		/* (n) receive all multicast packets */
#define	IFF_DRV_OACTIVE	0x400		/* (d) tx hardware queue is full */
#define	IFF_SIMPLEX	0x800		/* (i) can't hear own transmissions */
#define	IFF_LINK0	0x1000		/* per link layer defined bit */
#define	IFF_LINK1	0x2000		/* per link layer defined bit */
#define	IFF_LINK2	0x4000		/* per link layer defined bit */
#define	IFF_ALTPHYS	IFF_LINK2	/* use alternate physical connection */
#define	IFF_MULTICAST	0x8000		/* (i) supports multicast */
#define	IFF_CANTCONFIG	0x10000		/* (i) unconfigurable using ioctl(2) */
#define	IFF_PPROMISC	0x20000		/* (n) user-requested promisc mode */
#define	IFF_MONITOR	0x40000		/* (n) user-requested monitor mode */
#define	IFF_STATICARP	0x80000		/* (n) static ARP */
#define	IFF_STICKYARP	0x100000	/* (n) sticky ARP */
#define	IFF_DYING	0x200000	/* (n) interface is winding down */
#define	IFF_RENAMING	0x400000	/* (n) interface is being renamed */
#define	IFF_PALLMULTI	0x800000	/* (n) user-requested allmulti mode */
#define	IFF_NETLINK_1	0x1000000	/* (n) used by netlink */

/* flags set internally only: */
#define	IFF_CANTCHANGE \
	(IFF_BROADCAST|IFF_POINTOPOINT|IFF_DRV_RUNNING|IFF_DRV_OACTIVE|\
	    IFF_SIMPLEX|IFF_MULTICAST|IFF_ALLMULTI|IFF_PROMISC|\
	    IFF_DYING|IFF_CANTCONFIG|IFF_NEEDSEPOCH)

/*
 * Values for if_link_state.
 */
#define	LINK_STATE_UNKNOWN	0	/* link invalid/unknown */
#define	LINK_STATE_DOWN		1	/* link is down */
#define	LINK_STATE_UP		2	/* link is up */

/*
 * Some convenience macros used for setting ifi_baudrate.
 * XXX 1000 vs. 1024? --thorpej@netbsd.org
 */
#define	IF_Kbps(x)	((uintmax_t)(x) * 1000)	/* kilobits/sec. */
#define	IF_Mbps(x)	(IF_Kbps((x) * 1000))	/* megabits/sec. */
#define	IF_Gbps(x)	(IF_Mbps((x) * 1000))	/* gigabits/sec. */

/*
 * Output queues (ifp->if_snd) and internetwork datagram level (pup level 1)
 * input routines have queues of messages stored on ifqueue structures
 * (defined above).  Entries are added to and deleted from these structures
 * by these macros, which should be called with ipl raised to splimp().
 */
#define	IF_QFULL(ifq)		((ifq)->ifq_len >= (ifq)->ifq_maxlen)
#define	IF_DROP(ifq)		((ifq)->ifq_drops++)
#define	IF_ENQUEUE(ifq, m) { \
	(m)->m_nextpkt = 0; \
	if ((ifq)->ifq_tail == 0) \
		(ifq)->ifq_head = m; \
	else \
		(ifq)->ifq_tail->m_nextpkt = m; \
	(ifq)->ifq_tail = m; \
	(ifq)->ifq_len++; \
}
#define	IF_PREPEND(ifq, m) { \
	(m)->m_nextpkt = (ifq)->ifq_head; \
	if ((ifq)->ifq_tail == 0) \
		(ifq)->ifq_tail = (m); \
	(ifq)->ifq_head = (m); \
	(ifq)->ifq_len++; \
}
#define	IF_DEQUEUE(ifq, m) { \
	(m) = (ifq)->ifq_head; \
	if (m) { \
		if (((ifq)->ifq_head = (m)->m_nextpkt) == 0) \
			(ifq)->ifq_tail = 0; \
		(m)->m_nextpkt = 0; \
		(ifq)->ifq_len--; \
	} \
}


/*
 * Capabilities that interfaces can advertise.
 *
 * struct ifnet.if_capabilities
 *   contains the optional features & capabilities a particular interface
 *   supports (not only the driver but also the detected hw revision).
 *   Capabilities are defined by IFCAP_* below.
 * struct ifnet.if_capenable
 *   contains the enabled (either by default or through ifconfig) optional
 *   features & capabilities on this interface.
 *   Capabilities are defined by IFCAP_* below.
 * struct if_data.ifi_hwassist in mbuf CSUM_ flag form, controlled by above
 *   contains the enabled optional feature & capabilites that can be used
 *   individually per packet and are specified in the mbuf pkthdr.csum_flags
 *   field.  IFCAP_* and CSUM_* do not match one to one and CSUM_* may be
 *   more detailed or differentiated than IFCAP_*.
 *   Hwassist features are defined CSUM_* in sys/mbuf.h
 *
 * Capabilities that cannot be arbitrarily changed with ifconfig/ioctl
 * are listed in IFCAP_CANTCHANGE, similar to IFF_CANTCHANGE.
 * This is not strictly necessary because the common code never
 * changes capabilities, and it is left to the individual driver
 * to do the right thing. However, having the filter here
 * avoids replication of the same code in all individual drivers.
 */

/* IFCAP values as bit indexes */

#define	IFCAP_B_RXCSUM		0 /* can offload checksum on RX */
#define	IFCAP_B_TXCSUM		1 /* can offload checksum on TX */
#define	IFCAP_B_NETCONS		2 /* can be a network console */
#define	IFCAP_B_VLAN_MTU	3 /* VLAN-compatible MTU */
#define	IFCAP_B_VLAN_HWTAGGING	4 /* hardware VLAN tag support */
#define	IFCAP_B_JUMBO_MTU	5 /* 9000 byte MTU supported */
#define	IFCAP_B_POLLING		6 /* driver supports polling */
#define	IFCAP_B_VLAN_HWCSUM	7 /* can do IFCAP_HWCSUM on VLANs */
#define	IFCAP_B_TSO4		8 /* can do TCP Segmentation Offload */
#define	IFCAP_B_TSO6		9 /* can do TCP6 Segmentation Offload */
#define	IFCAP_B_LRO		10 /* can do Large Receive Offload */
#define	IFCAP_B_WOL_UCAST	11 /* wake on any unicast frame */
#define	IFCAP_B_WOL_MCAST	12 /* wake on any multicast frame */
#define	IFCAP_B_WOL_MAGIC	13 /* wake on any Magic Packet */
#define	IFCAP_B_TOE4		14 /* interface can offload TCP */
#define	IFCAP_B_TOE6		15 /* interface can offload TCP6 */
#define	IFCAP_B_VLAN_HWFILTER	16 /* interface hw can filter vlan tag */
#define	IFCAP_B_NV		17 /* can do SIOCGIFCAPNV/SIOCSIFCAPNV */
#define	IFCAP_B_VLAN_HWTSO	18 /* can do IFCAP_TSO on VLANs */
#define	IFCAP_B_LINKSTATE	19 /* the runtime link state is dynamic */
#define	IFCAP_B_NETMAP		20 /* netmap mode supported/enabled */
#define	IFCAP_B_RXCSUM_IPV6	21 /* can offload checksum on IPv6 RX */
#define	IFCAP_B_TXCSUM_IPV6	22 /* can offload checksum on IPv6 TX */
#define	IFCAP_B_HWSTATS		23 /* manages counters internally */
#define	IFCAP_B_TXRTLMT		24 /* hardware supports TX rate limiting */
#define	IFCAP_B_HWRXTSTMP	25 /* hardware rx timestamping */
#define	IFCAP_B_MEXTPG		26 /* understands M_EXTPG mbufs */
#define	IFCAP_B_TXTLS4		27 /* can do TLS encryption and segmentation for TCP */
#define	IFCAP_B_TXTLS6		28 /* can do TLS encryption and segmentation for TCP6 */
#define	IFCAP_B_VXLAN_HWCSUM	29 /* can do IFCAN_HWCSUM on VXLANs */
#define	IFCAP_B_VXLAN_HWTSO	30 /* can do IFCAP_TSO on VXLANs */
#define	IFCAP_B_TXTLS_RTLMT	31 /* can do TLS with rate limiting */
#define	IFCAP_B_RXTLS4		32 /* can to TLS receive for TCP */
#define	IFCAP_B_RXTLS6		33 /* can to TLS receive for TCP6 */
#define	IFCAP_B_IPSEC_OFFLOAD	34 /* inline IPSEC offload */
#define	__IFCAP_B_SIZE		35

#define	IFCAP_B_MAX	(__IFCAP_B_MAX - 1)
#define	IFCAP_B_SIZE	(__IFCAP_B_SIZE)

#define	IFCAP_BIT(x)		(1 << (x))

#define	IFCAP_RXCSUM		IFCAP_BIT(IFCAP_B_RXCSUM)
#define	IFCAP_TXCSUM		IFCAP_BIT(IFCAP_B_TXCSUM)
#define	IFCAP_NETCONS		IFCAP_BIT(IFCAP_B_NETCONS)
#define	IFCAP_VLAN_MTU		IFCAP_BIT(IFCAP_B_VLAN_MTU)
#define	IFCAP_VLAN_HWTAGGING	IFCAP_BIT(IFCAP_B_VLAN_HWTAGGING)
#define	IFCAP_JUMBO_MTU		IFCAP_BIT(IFCAP_B_JUMBO_MTU)
#define	IFCAP_POLLING		IFCAP_BIT(IFCAP_B_POLLING)
#define	IFCAP_VLAN_HWCSUM	IFCAP_BIT(IFCAP_B_VLAN_HWCSUM)
#define	IFCAP_TSO4		IFCAP_BIT(IFCAP_B_TSO4)
#define	IFCAP_TSO6		IFCAP_BIT(IFCAP_B_TSO6)
#define	IFCAP_LRO		IFCAP_BIT(IFCAP_B_LRO)
#define	IFCAP_WOL_UCAST		IFCAP_BIT(IFCAP_B_WOL_UCAST)
#define	IFCAP_WOL_MCAST		IFCAP_BIT(IFCAP_B_WOL_MCAST)
#define	IFCAP_WOL_MAGIC		IFCAP_BIT(IFCAP_B_WOL_MAGIC)
#define	IFCAP_TOE4		IFCAP_BIT(IFCAP_B_TOE4)
#define	IFCAP_TOE6		IFCAP_BIT(IFCAP_B_TOE6)
#define	IFCAP_VLAN_HWFILTER	IFCAP_BIT(IFCAP_B_VLAN_HWFILTER)
#define	IFCAP_NV		IFCAP_BIT(IFCAP_B_NV)
#define	IFCAP_VLAN_HWTSO	IFCAP_BIT(IFCAP_B_VLAN_HWTSO)
#define	IFCAP_LINKSTATE		IFCAP_BIT(IFCAP_B_LINKSTATE)
#define	IFCAP_NETMAP		IFCAP_BIT(IFCAP_B_NETMAP)
#define	IFCAP_RXCSUM_IPV6	IFCAP_BIT(IFCAP_B_RXCSUM_IPV6)
#define	IFCAP_TXCSUM_IPV6	IFCAP_BIT(IFCAP_B_TXCSUM_IPV6)
#define	IFCAP_HWSTATS		IFCAP_BIT(IFCAP_B_HWSTATS)
#define	IFCAP_TXRTLMT		IFCAP_BIT(IFCAP_B_TXRTLMT)
#define	IFCAP_HWRXTSTMP		IFCAP_BIT(IFCAP_B_HWRXTSTMP)
#define	IFCAP_MEXTPG		IFCAP_BIT(IFCAP_B_MEXTPG)
#define	IFCAP_TXTLS4		IFCAP_BIT(IFCAP_B_TXTLS4)
#define	IFCAP_TXTLS6		IFCAP_BIT(IFCAP_B_TXTLS6)
#define	IFCAP_VXLAN_HWCSUM	IFCAP_BIT(IFCAP_B_VXLAN_HWCSUM)
#define	IFCAP_VXLAN_HWTSO	IFCAP_BIT(IFCAP_B_VXLAN_HWTSO)
#define	IFCAP_TXTLS_RTLMT	IFCAP_BIT(IFCAP_B_TXTLS_RTLMT)

/* IFCAP2_* are integers, not bits. */
#define	IFCAP2_RXTLS4		(IFCAP_B_RXTLS4 - 32)
#define	IFCAP2_RXTLS6		(IFCAP_B_RXTLS6 - 32)
#define	IFCAP2_IPSEC_OFFLOAD	(IFCAP_B_IPSEC_OFFLOAD - 32)

#define	IFCAP2_BIT(x)		(1UL << (x))

#define IFCAP_HWCSUM_IPV6	(IFCAP_RXCSUM_IPV6 | IFCAP_TXCSUM_IPV6)

#define IFCAP_HWCSUM	(IFCAP_RXCSUM | IFCAP_TXCSUM)
#define	IFCAP_TSO	(IFCAP_TSO4 | IFCAP_TSO6)
#define	IFCAP_WOL	(IFCAP_WOL_UCAST | IFCAP_WOL_MCAST | IFCAP_WOL_MAGIC)
#define	IFCAP_TOE	(IFCAP_TOE4 | IFCAP_TOE6)
#define	IFCAP_TXTLS	(IFCAP_TXTLS4 | IFCAP_TXTLS6)

#define	IFCAP_CANTCHANGE	(IFCAP_NETMAP | IFCAP_NV)
#define	IFCAP_ALLCAPS		0xffffffff

#define	IFQ_MAXLEN	50
#define	IFNET_SLOWHZ	1		/* granularity is 1 second */

/*
 * The ifaddr structure contains information about one address
 * of an interface.  They are maintained by the different address families,
 * are allocated and attached when an address is set, and are linked
 * together so all addresses for an interface can be located.
 */
struct ifaddr {
	struct	sockaddr *ifa_addr;	/* address of interface */
	struct	sockaddr *ifa_dstaddr;	/* other end of p-to-p link */
#define	ifa_broadaddr	ifa_dstaddr	/* broadcast address interface */
	struct	sockaddr *ifa_netmask;	/* used to determine subnet */
	struct	ifnet *ifa_ifp;		/* back-pointer to interface */
	struct	ifaddr *ifa_next;	/* next address for interface */
	void	(*ifa_rtrequest)(int, 	/* check or clean routes (+ or -)'d */
				 struct rtentry *, struct sockaddr *);
	struct	rtentry *ifa_rt;	/* XXXX for ROUTETOIF ????? */
	u_short	ifa_flags;		/* mostly rt_flags for cloning */
	short	ifa_refcnt;		/* extra to malloc for link info */
	int	ifa_metric;		/* cost of going out this interface */
};
#define	IFA_ROUTE	RTF_UP		/* route installed */

/*
 * Message format for use in obtaining information about interfaces
 * from getkerninfo and the routing socket
 */
struct if_msghdr {
	u_short	ifm_msglen;	/* to skip over non-understood messages */
	u_char	ifm_version;	/* future binary compatability */
	u_char	ifm_type;	/* message type */
	int	ifm_addrs;	/* like rtm_addrs */
	int	ifm_flags;	/* value of if_flags */
	u_short	ifm_index;	/* index for associated ifp */
	struct	if_data ifm_data;/* statistics and other data about if */
};

/*
 * Message format for use in obtaining information about interface addresses
 * from getkerninfo and the routing socket
 */
struct ifa_msghdr {
	u_short	ifam_msglen;	/* to skip over non-understood messages */
	u_char	ifam_version;	/* future binary compatability */
	u_char	ifam_type;	/* message type */
	int	ifam_addrs;	/* like rtm_addrs */
	int	ifam_flags;	/* value of ifa_flags */
	u_short	ifam_index;	/* index for associated ifp */
	int	ifam_metric;	/* value of ifa_metric */
};

/*
 * Interface request structure used for socket
 * ioctl's.  All interface ioctl's must have parameter
 * definitions which begin with ifr_name.  The
 * remainder may be interface specific.
 */
struct	ifreq {
	char	ifr_name[IFNAMSIZ];		/* if name, e.g. "en0" */
	union {
		struct	sockaddr ifru_addr;
		struct	sockaddr ifru_dstaddr;
		struct	sockaddr ifru_broadaddr;
		short	ifru_flags;
		short	ifru_index;
		int	ifru_metric;
		int	ifru_mtu;
		caddr_t	ifru_data;
	} ifr_ifru;
#define	ifr_addr	ifr_ifru.ifru_addr	/* address */
#define	ifr_dstaddr	ifr_ifru.ifru_dstaddr	/* other end of p-to-p link */
#define	ifr_broadaddr	ifr_ifru.ifru_broadaddr	/* broadcast address */
#define	ifr_flags	ifr_ifru.ifru_flags	/* flags */
#define	ifr_metric	ifr_ifru.ifru_metric	/* metric */
#define	ifr_mtu		ifr_ifru.ifru_mtu	/* mtu */
#define	ifr_data	ifr_ifru.ifru_data	/* for use by interface */
#define ifr_index	ifr_ifru.ifru_index	/* interface index */
#define ifr_ifindex	ifr_ifru.ifru_index	/* interface index (alias) */
};

struct ifaliasreq {
	char	ifra_name[IFNAMSIZ];		/* if name, e.g. "en0" */
	struct	sockaddr ifra_addr;
	struct	sockaddr ifra_broadaddr;
	struct	sockaddr ifra_mask;
};

/*
 * Structure used in SIOCGIFCONF request.
 * Used to retrieve interface configuration
 * for machine (useful for programs which
 * must know all networks accessible).
 */
struct	ifconf {
	int	ifc_len;		/* size of associated buffer */
	union {
		caddr_t	ifcu_buf;
		struct	ifreq *ifcu_req;
	} ifc_ifcu;
#define	ifc_buf	ifc_ifcu.ifcu_buf	/* buffer address */
#define	ifc_req	ifc_ifcu.ifcu_req	/* array of structures returned */
};

struct if_nameindex {
	unsigned long	if_index;
	char 			*if_name;
};

#ifndef NET_IF_ARP_H
#include <net/if_arp.h>
#endif

#endif /* !_NET_IF_H_ */
