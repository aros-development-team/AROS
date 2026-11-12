/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef NETINET_INETFAMILY_H
#define NETINET_INETFAMILY_H

/*
 * The family interface: what the internet transports (TCP, UDP, raw
 * sockets) and the PCB layer ask of the address family a socket belongs
 * to. There is one such interface, the same for every family; a family
 * registers its instance when it starts, and a PCB carries a pointer to
 * the instance of its family from the moment it is made (inp_family).
 * Code that has a PCB never tests which family it is: it asks.
 *
 * The structure grows at the end only; inf_size says how much of it a
 * family filled in.
 *
 * Ownership: the instance belongs to the family and stays valid while the
 * family is registered; a family cannot be unregistered while a socket of
 * it exists (its domain counts them), so a PCB's pointer never dangles.
 * `nam' is an mbuf holding a socket address of the family, owned by the
 * caller. Nothing passed through this interface changes owner.
 */
struct inpcb;
struct mbuf;
struct socket;

/* Types and flags of the operations below. */
struct ifnet;
struct protosw;
struct rtentry;
struct sockaddr;
struct inetfamily;

/* inf_flags */
#define INF_HASDF       0x01    /* routers fragment unless told not to: the family has a
                                 * "don't fragment" bit, its output says EMSGSIZE when a
                                 * packet with it does not fit, and a transport may give
                                 * the bit up (INF_OUT_DF not set) to get through */
#define INF_CKSUM_REQUIRED 0x02 /* a transport checksum of zero is not "none" */
#define INF_ZONES       0x04    /* addresses have zones: a PCB's interface (inp_boundif)
                                 * is the link its peer is on */
#define INF_RTREF       0x08    /* inf_pcbroute() gives a reference the caller drops with
                                 * rtfree() (the family has no place to keep the route) */

/* flags of inf_output() */
#define INF_OUT_DF      0x01    /* must not be fragmented on the way (INF_HASDF) */
#define INF_OUT_BARE    0x02    /* a segment a transport makes by itself (reset, keepalive,
                                 * answer to a segment): not the socket's TOS; IPv4 also
                                 * sends it with the default TTL, without the socket's IP
                                 * options and SO_DONTROUTE, as it always has */
#define INF_OUT_ROOM    0x04    /* the inf_hdrlen bytes in front of the mbuf's data are
                                 * the caller's to give (a received packet's own header):
                                 * the header goes there, whatever M_LEADINGSPACE says */

/* flags of inf_dgoutput() */
#define INF_DG_ROOM     0x01    /* the inf_hdrlen bytes in front of the mbuf's data are
                                 * the caller's to give, in the same mbuf: the network
                                 * header goes there */
#define INF_DG_NOTZERO  0x02    /* a checksum that comes out as zero is sent as all ones
                                 * (UDP: zero means "none") */

/* inf_rawflags */
#define INFR_HDR        0x01    /* a raw socket is handed the packet with its network
                                 * header in front (else from the transport header on) */
#define INFR_SRCLATE    0x02    /* the family's output gives a packet without a source
                                 * address one: none is selected for it beforehand, and
                                 * connect() does not fix one */

#define INF_RAWOPTS     4       /* ints of a datagram's send options (inf_rawsendopts) */

/*
 * Did the socket of PCB inp, of family inf, ask its family for ancillary
 * data with what it receives (inf_rcvopt_off, inf_rcvopt_mask)?
 */
#define INF_WANTSCONTROL(inp, inf) \
    (*(const int *)((const char *)(inp) + (inf)->inf_rcvopt_off) & (inf)->inf_rcvopt_mask)

/*
 * Control input: what a family's ICMP tells a transport about a packet the
 * transport sent and that came back quoted in an error. Addresses are in
 * the family's form (inf_addrlen bytes); ports in network byte order.
 */
struct inctlparam {
    const struct inetfamily *icp_family;
    struct sockaddr *icp_dst;   /* where the packet was going, as a socket address */
    const void *icp_faddr;      /* the same, as an address */
    const void *icp_laddr;      /* where it came from (ours); NULL: not known */
    u_int16_t   icp_fport;      /* its ports; 0: not known */
    u_int16_t   icp_lport;
    int         icp_haveseq;    /* icp_seq was quoted */
    u_int32_t   icp_seq;        /* TCP: its sequence number, host byte order */
    u_int32_t   icp_mtu;        /* PRC_MSGSIZE: the path MTU the error names; 0: none */
};

/*
 * Every operation is required and is called without a test, unless its
 * comment says that it may be NULL: inetfamily_register() refuses an
 * instance that lacks a required one.
 */
struct inetfamily {
    unsigned long inf_size;     /* sizeof(struct inetfamily) */
    int         inf_family;     /* AF_xxx */
    int         inf_optlevel;   /* the level of the family's own socket options (IPPROTO_IP, ...) */

    /* a socket option of level inf_optlevel */
    int       (*inf_ctloutput)(int op, struct socket *, int level, int optname, struct mbuf **);

    /* is the address in `nam' a multicast address of the family? */
    int       (*inf_sa_multicast)(struct mbuf *nam);
    /* is the PCB's peer on a network this machine is attached to? */
    int       (*inf_peer_local)(struct inpcb *);
    /* the PCB is going: release what the family hung on it */
    void      (*inf_pcbfree)(struct inpcb *);

    /*
     * Instances are filled in with designated initialisers (.inf_x = ...),
     * so members can be added below without touching what is there.
     */

    /* ---- per packet: receive information, checksum, output ---- */

    int         inf_addrlen;    /* bytes in an address of the family */
    int         inf_hdrlen;     /* its network header, without options */
    const int  *inf_mssdflt;    /* the MSS to assume when nothing says otherwise (a
                                 * variable: it can be set while the stack runs) */
    u_int32_t   inf_bhmtu;      /* the MTU to fall back to when a path seems to drop
                                 * large packets without saying so */
    int         inf_flags;      /* INF_xxx */

    /*
     * A packet has arrived for a transport: m starts at the network header,
     * the transport header is at off, both in the first mbuf. Gives pointers
     * to the addresses (in the packet: copy them before the next call), the
     * TOS/traffic class, the TTL/hop limit and the length from the
     * transport header to the end. 0, or an errno if the packet is no good.
     */
    int       (*inf_rcvinfo)(struct mbuf *m, int off, const void **src, const void **dst,
                             int *tos, int *hlim, int *ulen);
    /*
     * The transport checksum of the len bytes at off, with the family's
     * pseudo-header for proto: 0 if a received packet is good. The network
     * header in front of off is the family's scratch space from here on;
     * src and dst must not point into it.
     */
    int       (*inf_cksum)(struct mbuf *m, int proto, int off, int len,
                           const void *src, const void *dst);
    /*
     * Send. m starts at the transport header and its packet header has the
     * length; the 16-bit checksum field at cksumoff is filled in here (over
     * the pseudo-header for proto). inp may be NULL (no socket): then, and
     * with INF_OUT_BARE, nothing of a socket's settings applies but its
     * route and zone. tos: bits to add to the TOS/traffic class (ECN).
     * hlim: the TTL/hop limit; 0: the socket's, or the family's default.
     * flags: INF_OUT_xxx. Takes m in every case; returns an errno.
     */
    int       (*inf_output)(struct mbuf *m, struct inpcb *inp, const void *laddr,
                            const void *faddr, int proto, int cksumoff, int tos, int hlim,
                            int flags);
    /* is this address (inf_addrlen bytes) a multicast address? */
    int       (*inf_addr_multicast)(const void *addr);
    /* may a connection from src to local port lport (network order) be accepted? */
    int       (*inf_acceptok)(const void *src, u_int lport);
    /*
     * The MTU to assume towards faddr when answering a connection request
     * (no PCB yet); zone: the link, if the address needs one, else NULL.
     */
    u_int32_t (*inf_mtu)(const void *faddr, const struct ifnet *zone);
    /* the route to the PCB's peer, or NULL (see INF_RTREF) */
    struct rtentry *(*inf_pcbroute)(struct inpcb *);

    /* ---- PCB layer: what one implementation of it asks of the family ---- */

    /*
     * The PCB layer (netinet/in_pcb.c: inpcb_xxx()) compares, tests for
     * "any" and hashes addresses as inf_addrlen bytes, which must be a
     * multiple of 4 and at most INP_ADDRMAX; all zero is "any".
     * No operation here is called per packet, except inf_pcbanyfamily for
     * a PCB of another family that sits on the port being looked up.
     */
    int         inf_salen;      /* bytes in a socket address of the family */
    const u_char *inf_ctlerrmap; /* the errno a transport reports for each
                                 * PRC_xxx (PRC_NCMDS entries; 0: none) */

    /* a PCB of the family has been made: the family's defaults */
    void      (*inf_pcbinit)(struct inpcb *);
    /*
     * Does this PCB of the family stand for the other families as well, as
     * long as it has no address (an AF_INET6 socket without IPV6_V6ONLY)?
     */
    int       (*inf_pcbanyfamily)(struct inpcb *);
    /*
     * Take a socket address apart: 0 and pointers into `nam' to the address
     * and the port (network order), or EINVAL (length), EAFNOSUPPORT
     * (family), ENXIO (it names a zone that does not exist). *zone: the
     * interface it names, NULL if none.
     */
    int       (*inf_sockaddr_parse)(struct mbuf *nam, const void **addr, u_short *port,
                                    struct ifnet **zone);
    /*
     * Make a socket address (inf_salen bytes at sa) of an address and a
     * port; zone: the interface to name if the address needs one, or NULL.
     */
    void      (*inf_mksockaddr)(struct sockaddr *sa, const void *addr, u_int port,
                                const struct ifnet *zone);
    /*
     * Write at dst (inf_addrlen bytes) the address `addr' of family `from'
     * as this family presents it (IPv6: an IPv4 address in mapped form).
     * 0, or EAFNOSUPPORT if it has no such form.
     */
    int       (*inf_addr_from)(void *dst, const struct inetfamily *from, const void *addr);
    /* the interface to name with the PCB's own addresses, or NULL */
    const struct ifnet *(*inf_pcbzone)(struct inpcb *);
    /*
     * Is addr an address of this machine that may be used as a local
     * address now? zone: the interface it must be on if it is an address
     * that needs a zone, or NULL for any.
     */
    int       (*inf_ownaddr)(const void *addr, struct ifnet *zone);
    /*
     * The local address for a packet to dst from a PCB that has none
     * (inp may be NULL): written at src. oifp: the interface the packet is
     * known to leave by, or NULL. 0 or an errno.
     */
    int       (*inf_selectsrc)(struct inpcb *inp, const void *dst, struct ifnet *oifp,
                               void *src);
    /*
     * bind(): may the PCB be bound to the address in `nam'? Also asked
     * with nam NULL (a port only). 0 and, for a `nam', pointers into it to
     * the address and the port, and in *zone the interface the PCB is to
     * be bound to with it (left alone if none); or an errno.
     */
    int       (*inf_pcbbindaddr)(struct inpcb *, struct mbuf *nam, const void **addr,
                                 u_short *port, struct ifnet **zone);
    /*
     * connect(): is the address in `nam' one the PCB can be connected to,
     * and from which local address? May rewrite the address in `nam' (IPv4:
     * "any" and limited broadcast). 0, the local address at laddr (the
     * PCB's own if it has one) and in *zone the interface the PCB is to be
     * bound to (left alone if none); or an errno.
     */
    int       (*inf_pcbladdr)(struct inpcb *, struct mbuf *nam, void *laddr,
                              struct ifnet **zone);

    /* ---- UDP and raw sockets ---- */

    /*
     * What the one UDP (netinet/udp_usrreq.c) asks of the family besides
     * the per-packet operations above. A datagram coming in costs three
     * calls into the family (inf_rcvinfo, inf_cksum, inf_rcvfrom) and the
     * PCB look-up - and inf_savecontrol for a socket that asked for
     * ancillary data; one going out costs inf_dgoutput, and
     * inf_sockaddr_parse before it when it is sent with an address.
     */
    /*
     * Which sockets asked for ancillary data from inf_savecontrol: those
     * with one of the bits inf_rcvopt_mask set in the int at offset
     * inf_rcvopt_off of their PCB (the family's "receive xxx" socket
     * options). Data, not an operation, so that a datagram for a socket
     * that asked for nothing costs no call.
     */
    int         inf_rcvopt_off;
    int         inf_rcvopt_mask;
    /*
     * A datagram has arrived (m as for inf_rcvinfo; src and dst are
     * copies of its addresses, sport its source port in network order):
     * write the socket address of its sender at sa (inf_salen bytes, the
     * zone that of the interface it came in by if the address needs one).
     * Returns not 0 if the destination is an address several sockets may
     * all be listening on - a group, or a broadcast address - so that
     * every one of them is given the datagram.
     */
    int       (*inf_rcvfrom)(struct mbuf *m, const void *src, u_int sport,
                             const void *dst, struct sockaddr *sa);
    /*
     * The ancillary data the PCB's socket options ask for, about a packet
     * that came in by rcvif for dst with this TOS/traffic class and
     * TTL/hop limit (as inf_rcvinfo gave them: the packet's header is not
     * read again). Each item is an mbuf chained on at *mp; returns where
     * the next one goes. For a datagram and for a raw socket's packet
     * alike. Required when inf_rcvopt_mask is not 0, and only called for
     * a socket that has one of its bits set.
     */
    struct mbuf **(*inf_savecontrol)(struct inpcb *, const struct ifnet *rcvif,
                                     const void *dst, int tos, int hlim,
                                     struct mbuf **mp);
    /*
     * Nobody listens on the port the packet is for (m as for inf_rcvinfo,
     * after inf_cksum): tell its sender, the family's way. Takes m.
     */
    void      (*inf_unreach)(struct mbuf *m, int off);
    /*
     * Send a datagram of a datagram socket. m starts at the transport
     * header and its packet header has the length. faddr NULL: to the
     * PCB's peer, from the PCB's address. Else to faddr - with zone as
     * inf_sockaddr_parse gave them - and the PCB is not changed: from its
     * local address if it has one, else from the address the family
     * selects for this datagram; the family may refuse the destination or
     * replace it by what it stands for (IPv4: "any", limited broadcast).
     * control: the ancillary data sent with the datagram, or NULL; what
     * the family knows of it applies to this datagram alone (TOS/traffic
     * class, source address and interface, don't fragment, minimum MTU),
     * the rest is ignored. Everything else is the socket's: its TOS, TTL
     * or hop limit, options, multicast options, don't-fragment setting,
     * SO_DONTROUTE and SO_BROADCAST, the interface it is bound to.
     * cksumoff: where in the transport header the checksum over the
     * family's pseudo-header for proto goes; negative: none is made.
     * flags: INF_DG_xxx. Takes m and control in every case; returns an
     * errno.
     */
    int       (*inf_dgoutput)(struct mbuf *m, struct inpcb *inp, const void *faddr,
                              struct ifnet *zone, struct mbuf *control, int proto,
                              int cksumoff, int flags);
    /*
     * The path to dst (a socket address of the family) takes packets of
     * at most mtu bytes: tell the PCB's socket if it asked to be told.
     * Called for each PCB that may have sent the packet the error quotes.
     * NULL: the family has no such notification.
     */
    void      (*inf_pmtunotify)(struct inpcb *, struct sockaddr *dst, u_int32_t mtu);
    /*
     * An ioctl on a socket of the family that the socket layer did not
     * handle (PRU_CONTROL): the family's interface and address requests,
     * the same whichever transport the socket is of. NULL: none are
     * supported (EOPNOTSUPP).
     */
    int       (*inf_control)(struct socket *, int cmd, caddr_t data, struct ifnet *);

    /* ---- raw sockets ---- */

    /*
     * What the one raw-socket implementation (netinet/raw_ip.c) asks of
     * the family. A raw socket is a PCB with a struct rawpcb behind it
     * (netinet/in_pcb.h: its protocol and the offset of a checksum the
     * stack makes and verifies), followed by inf_rawprivlen bytes that are
     * the family's own. Every operation here may be NULL; a family
     * without inf_rawoutput has no raw sockets (EPROTONOSUPPORT), and one
     * that sets RPF_FILTER on a socket must have inf_rawfilter.
     *
     * Sending is not the datagram sockets' inf_dgoutput, and the two are
     * kept apart on purpose. A raw socket's control message is checked
     * (an option the family does not know is an error; a datagram socket
     * ignores it) and may ask for more (IPv6: the hop limit); its source
     * address may stay unspecified, or be left to the family's output
     * (INFR_SRCLATE), where a datagram socket must have one selected; and
     * in header-included mode the data is the whole packet and nothing of
     * all this applies. One operation for both would be the two bodies
     * behind a flag in every family, and the raw sockets' rules (which
     * source, which interface) would have to be written once per family
     * instead of once in netinet/raw_ip.c.
     */
    int         inf_rawflags;   /* INFR_xxx */
    int         inf_rawprivlen; /* bytes of the family's own behind a struct rawpcb */

    /* a raw socket has been made (its struct rawpcb is set up): the
     * family's defaults, in the PCB and in its own part */
    void      (*inf_rawattach)(struct inpcb *);
    /*
     * Is the packet (m from the network header, its transport header at
     * off) of a kind this socket has asked not to be given? Asked only of
     * a socket with RPF_FILTER set, which the family sets. Not 0: skip it.
     */
    int       (*inf_rawfilter)(struct inpcb *, struct mbuf *m, int off, int proto);
    /*
     * A socket option only a raw socket of the family has: its errno, or
     * -1 if this is not one (it then goes to inf_ctloutput). Frees *m on
     * a set it handled, as inf_ctloutput does.
     */
    int       (*inf_rawctloutput)(int op, struct socket *, int level, int optname,
                                  struct mbuf **m);
    /*
     * What a datagram's control message asks for: a source address (at
     * src, inf_addrlen bytes, left zero if none), the interface to leave
     * by (*oifp, left alone if none), and whatever else the family reads,
     * in opts (INF_RAWOPTS ints, zero on entry) for its inf_rawoutput.
     * Does not free control. 0 or an errno. NULL: the family has no send
     * options, and a control message is refused.
     */
    int       (*inf_rawsendopts)(struct inpcb *, struct mbuf *control, void *src,
                                 struct ifnet **oifp, int *opts);
    /*
     * Send. Normally m is what follows the network header: the family
     * puts its header in front, for proto from laddr (all zero: none
     * could be chosen) to faddr, and - cksumoff not negative - stores the
     * checksum of the data with its pseudo-header at that offset of the
     * data. With INP_HDRINCL set on the PCB (which only a family's own
     * socket option does) m is the whole packet, header included, and
     * laddr, proto and cksumoff are not used. oifp: the interface the
     * datagram is to leave by, or NULL. opts: as inf_rawsendopts left
     * them, or NULL if there was no control message. Takes m in every
     * case; returns an errno.
     */
    int       (*inf_rawoutput)(struct mbuf *m, struct inpcb *, const void *laddr,
                               const void *faddr, int proto, int cksumoff,
                               struct ifnet *oifp, const int *opts);

    /* ---- protocols outside the family's own table ---- */

    /*
     * A protocol that is not in the family's protocol table - a tunnel,
     * one loaded later - takes the family's protocol number proto: the
     * packets of that number are then handed to pr->pr_input as they are
     * to a protocol of the table (IPv4: (m, header length)), and what the
     * family's ICMP says about one to pr->pr_ctlinput if it has one. pr
     * is the caller's and must stay until the number is given back with
     * inf_proto_unregister. 0 or an errno. NULL: the family takes none.
     */
    int       (*inf_proto_register)(int proto, struct protosw *pr);
    int       (*inf_proto_unregister)(int proto, struct protosw *pr);

    /*
     * Interface ifp is going away: forget it in what the family hung on
     * the PCB (the interface its multicast options name). Asked of every
     * family for every PCB, whichever family the PCB is of - a socket's
     * options are found by their level (inetfamily_bylevel()). May be
     * NULL.
     */
    void      (*inf_pcbifgone)(struct inpcb *, const struct ifnet *ifp);
};

#define INETFAMILY_MAX  4       /* families at one time */

#ifdef KERNEL
int     inetfamily_register(const struct inetfamily *inf);
int     inetfamily_unregister(const struct inetfamily *inf);
const struct inetfamily *inetfamily_find(int family);
const struct inetfamily *inetfamily_bylevel(int optlevel);
#endif

#endif /* !NETINET_INETFAMILY_H */
