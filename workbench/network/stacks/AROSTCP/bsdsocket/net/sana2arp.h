/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 Neil Cafferkey
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
void alloc_arptable(struct sana_softc* ssc, int to_allocate);
void arptimer(void);
int arpresolve(register struct sana_softc *ssc, struct mbuf * m,
               register struct in_addr *destip, register u_char * desten,
               int * error);
void arpinput(struct sana_softc *ssc, struct mbuf *m, caddr_t srcaddr);
int arpioctl(int cmd, caddr_t data);
#endif

#endif /* !IF_ARPSANA_H */
