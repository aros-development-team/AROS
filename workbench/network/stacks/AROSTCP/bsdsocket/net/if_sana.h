/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 - 2012 The AROS Dev Team
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
#if	INET
  struct {
    UWORD reqno;	      /* for listening ip packets */
    UWORD sent;
    ULONG type;
  } ss_ip;
  struct {			/* for ARP */
    UWORD reqno;	      
    UWORD sent;
    ULONG type;			/* ARP packet type */
    ULONG hrd;			/* ARP header type */
    struct arptable *table;	/* ARP/IP table */
  } ss_arp;
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
  struct sana_softc *ss_next;
};

/*
 * Configuration flags
 */
#define SSF_TRACK (1<<0)	      /* Should we track packets? */
#define SSB_TRACK 0

/* Default configuration flags */
#define SS_CFLAGS (SSF_TRACK)

/* Private Interface Config flags */
#define IFF_NOUP     0x8000
#define IFF_DELAYUP  0x4000

/*
 * Global functions defined in if_sana.c
 */
int sana_output(struct ifnet *ifp, struct mbuf *m0,
			struct sockaddr *dst, struct rtentry *rt);
int sana_ioctl(register struct ifnet *ifp, int cmd, caddr_t data);
/* queue for sana network interfaces */
extern struct sana_softc *ssq;
#endif /* of IF_SANA_H */
