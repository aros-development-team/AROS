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

#ifndef IF_ARPSANA_H
#define IF_ARPSANA_H

#ifndef IF_H
#include <net/if.h>
#endif

#ifndef IF_ARP_H
#include <net/if_arp.h>
#endif

/*
 * Address Resolution Protocol.
 *
 * See RFC 826 for protocol description.  Structure below is adapted
 * to resolving  addresses.  Field names used correspond to 
 * RFC 826.
 */
struct s2_arppkt {
  struct	arphdr s2a_hdr;	  /* fixed-size header */
  struct {
      u_char sha_dum[MAXADDRARP]; /* space for sender hardware address */
      u_char spa_dum[MAXADDRARP]; /* space for sender protocol address */
      u_char tha_dum[MAXADDRARP]; /* space for target hardware address */
      u_char tpa_dum[MAXADDRARP]; /* space for target protocol address */
  } arpdata;
};
#define	arp_hrd	s2a_hdr.ar_hrd
#define	arp_pro	s2a_hdr.ar_pro
#define	arp_hln	s2a_hdr.ar_hln
#define	arp_pln	s2a_hdr.ar_pln
#define	arp_op	s2a_hdr.ar_op

#define	ARPTAB_HSIZE	11	/* hash table size */
#define ARPENTRIES      11*15	/* normal amount of ARP entries to allocate */
#define ARPENTRIES_MIN  11	/* minimum # of ARP entries to allocate */

/*
 * timer values
 */
#define	ARPT_AGE	(60*1)	/* aging timer, 1 min. */
#define	ARPT_KILLC	20	/* kill completed entry in 20 mins. */
#define	ARPT_KILLI	3	/* kill incomplete entry in 3 minutes */

#ifdef	KERNEL
void arp_init(void);	/* from ip_init(): registers ARP and IPv4LL with the core */
void arp_deinit(void);	/* IPv4 stops: takes them back, frees the interfaces' ARP tables */
void alloc_arptable(struct sana_softc* ssc, int to_allocate);
void arptimer(void);
int arpresolve(register struct sana_softc *ssc, struct mbuf * m,
               register struct in_addr *destip, register u_char * desten,
               int * error);
void arpinput(struct sana_softc *ssc, struct mbuf *m, caddr_t srcaddr);
int arpioctl(int cmd, caddr_t data);

/* IPv4LL (RFC 3927): autoip's states; conflict detection probes, announces
 * and defends the address it claims (arp_dad_*) */
#define AUTOIP_DISABLED  0
#define AUTOIP_CLAIMING  1      /* the candidate is on the interface */
#define AUTOIP_WAIT      2      /* waiting to claim another (rate limit) */

#define AUTOIP_MAX_CONFLICTS     10     /* then one claim a minute */
#define AUTOIP_RATE_LIMIT_INTERVAL 60   /* seconds */

void autoip_start(struct ifnet *ifp);
void autoip_stop(struct ifnet *ifp);
void autoip_timer(void);

/*
 * Address conflict detection for configured addresses (RFC 5227, as
 * NetBSD's arp_dad_*), in fast ticks (PR_FASTHZ, 50 ms): a random wait
 * below PROBE_WAIT, ip_dad_count probes PROBE_MIN..PROBE_MAX apart,
 * ANNOUNCE_WAIT, then ANNOUNCE_NUM announcements ANNOUNCE_INTERVAL apart;
 * one defence per DEFEND_INTERVAL.
 */
#define ARP_HZ                20
#define ARP_DAD_PROBE_NUM         3      /* an IPv4LL address's probes, always */
#define ARP_DAD_PROBE_WAIT        (1 * ARP_HZ)
#define ARP_DAD_PROBE_MIN         (1 * ARP_HZ)
#define ARP_DAD_PROBE_MAX         (2 * ARP_HZ)
#define ARP_DAD_ANNOUNCE_WAIT     (2 * ARP_HZ)
#define ARP_DAD_ANNOUNCE_NUM      2
#define ARP_DAD_ANNOUNCE_INTERVAL (2 * ARP_HZ)
#define ARP_DAD_DEFEND_INTERVAL   (10 * ARP_HZ)
#define ARP_DAD_REPROBE           (60 * ARP_HZ)

struct in_ifaddr;
extern int ip_dad_count;
int arp_dad_capable(struct ifnet *ifp);
void arp_dad_start(struct in_ifaddr *ia);
void arp_dad_stop(struct in_ifaddr *ia);
void arp_dad_forget(struct in_ifaddr *ia);
void arp_dad_reload(int begin);
void arp_fasttimo(void);
struct rtentry;
#endif

#endif /* !IF_ARPSANA_H */
