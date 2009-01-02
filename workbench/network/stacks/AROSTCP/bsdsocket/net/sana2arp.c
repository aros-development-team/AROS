/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 - 2007 The AROS Dev Team
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
 * Address Resolution Protocol.
 * TODO:
 *	add "inuse/lock" bit (or ref. count) along with valid bit
 */

#include <conf.h>

#include <libraries/miami.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/synch.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_protos.h>
#include <net/pfil.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>

#include <net/if_sana.h>
#include <net/sana2arp.h>

#include <net/if_loop_protos.h>

/*
 * Internet to hardware address resolution table entry
 */
struct	arptab {
  struct arptab *at_succ;	/* doubly linked list */
  struct arptab *at_pred;
  struct in_addr at_iaddr;	/* internet address */
  u_char         at_hwaddr[MAXADDRSANA]; /* hardware address */
  u_char         at_timer;	/* minutes since last reference */
  u_char         at_flags;	/* flags */
  struct mbuf   *at_hold;	/* last packet until resolved/timeout */
};

/*
 * Global constant for ARP entry allocation
 */
unsigned long arpentries = ARPENTRIES;

/*
 * General per interface hash table
 */
struct arptable {
  struct SignalSemaphore atb_lock;
  struct arptab         *atb_free;
  struct MinList         atb_entries[ARPTAB_HSIZE];
};

#define ARPTAB_LOCK(atb) (ObtainSemaphore(&atb->atb_lock))
#define ARPTAB_UNLOCK(atb) (ReleaseSemaphore(&atb->atb_lock)) 
#define	ARPTAB_HASH(a) ((u_long)(a) % ARPTAB_HSIZE)

extern struct ifnet loif;

int	useloopback = 0;	/* use loopback interface for local traffic */

static void arpwhohas(register struct sana_softc *ssc, struct in_addr * addr);
static void in_arpinput(register struct sana_softc *ssc, struct mbuf *m);
static char *sana_sprintf(register u_char *ap, int len);

/*
 * Initialization routine. Allocate ARP entries.
 * MUST BE CALLED AT SPLIMP.
 */
void
alloc_arptable(struct sana_softc* ssc, int to_allocate)
{
  struct arptab *at;
  struct arptable *atab;
  int i;
  
  if (ssc->ss_arp.table) 
    return /* (void)ssc->ss_arp.table */;

#if 0
  if (to_allocate < arpentries)
#endif
      to_allocate = arpentries;
    
  atab = bsd_malloc(sizeof(*atab), M_ARPENT, M_WAITOK);
  at = bsd_malloc(sizeof(*at) * to_allocate, M_ARPENT, M_WAITOK);

  if (atab && at) {
    InitSemaphore(&atab->atb_lock);
    for (i = 0; i < ARPTAB_HSIZE; i++)
      NewList((struct List *)(atab->atb_entries + i));

    aligned_bzero(at, sizeof(*at) * to_allocate);

    at[0].at_succ = NULL;
    for (i = 1; i < to_allocate; i++) {
      at[i].at_succ = &at[i - 1];
    }
    atab->atb_free = &at[to_allocate - 1];
  } else {
    if (atab) bsd_free(atab, M_ARPENT);
    if (at) bsd_free(at, M_ARPENT);
    __log(LOG_ERR, "Could not allocate ARP table for %s\n", ssc->ss_name);
  }

  ssc->ss_arp.table = atab;
}

/* 
 * Notification function for arp entries 
 */
LONG 
arpentries_notify(void *dummy, LONG value)
{
  return (ULONG)value > ARPENTRIES_MIN;
}

/*
 * Free an arptab entry. ARP TABLE MUST BE LOCKED
 */
static void
arptfree(register struct arptable *atb, register struct arptab *at)
{
  if (at->at_hold)
    m_freem(at->at_hold);

  Remove((struct Node *)at);

  if (at->at_flags & ATF_PERM) {
    bsd_free(at, M_ARPENT);
  } else {
    at->at_hold = NULL;
    at->at_timer = at->at_flags = 0;
    at->at_iaddr.s_addr = 0;
    at->at_succ = atb->atb_free;
    atb->atb_free = at;
  }
}

/*
 * Enter a new address in arptab. ARP TABLE MUST BE LOCKED
 */
static struct arptab *
arptnew(u_long addr, struct arptable *atb, int permanent)
{
  struct arptab *at = NULL;

  if (permanent) {
    at = bsd_malloc(sizeof(*at), M_ARPENT, M_WAITOK);
    bzero((caddr_t)at, sizeof(*at));
  } else {
    at = atb->atb_free;
    if (at) {
      atb->atb_free = at->at_succ;
    } else {
      /*
       * The oldest entry is pushed out from the 
       * interface table if there is no free entry.
       * This should always succeed since all 
       * entries can not be permanent
       */
      struct arptab *oldest = NULL;
      int i;

      for (i = 0; i < ARPTAB_HSIZE; i++) {
	for (at = (struct arptab *)atb->atb_entries[i].mlh_Head; 
	     at->at_succ; 
	     at = at->at_succ) {
	  if (at->at_flags == 0 || (at->at_flags & ATF_PERM))
	    continue;
	  if (!oldest || oldest->at_timer < at->at_timer)
	    oldest = at;
	}
      }
      if (oldest) {
	Remove((struct Node *)oldest);
	at = oldest;
      } else {
	at = NULL;
      }
    }
  }
  if (at) {
    at->at_iaddr.s_addr = addr;
    at->at_flags = ATF_INUSE;
    AddHead((struct List *)(&atb->atb_entries[ARPTAB_HASH(addr)]), 
	    (struct Node *)at);
  }
  return (at);
}

/*
 * Locate an IP address in the ARP table
 * Assume looker have locked the table
 */
static struct arptab*
arptab_look(struct arptable *table, u_long addr)
{
  register struct arptab *at = (struct arptab *)
    table->atb_entries[ARPTAB_HASH(addr)].mlh_Head; 

  for(;at->at_succ; at = at->at_succ) 
    if (at->at_iaddr.s_addr == addr) 
      return at;

  return NULL;
}

/*
 * Timeout routine.  Age arp_tab entries once a minute.
 */
void
arptimer()
{
  struct sana_softc *ssc;
  register struct arptable *atab;
  register struct arptab *at, *oldest;
  register i;

  for (ssc = ssq; ssc; ssc = ssc->ss_next) {
    if (!(atab = ssc->ss_arp.table))
      continue;
    /* Lock the table */
    ARPTAB_LOCK(atab);
    oldest = NULL;
    for (i = 0; i < ARPTAB_HSIZE; i++) {
      for (at = (struct arptab *)atab->atb_entries[i].mlh_Head; 
	   at->at_succ; 
	   at = at->at_succ) {
	if (at->at_flags == 0 || (at->at_flags & ATF_PERM))
	  continue;
	if (++at->at_timer < ((at->at_flags & ATF_COM) ?
			      ARPT_KILLC : ARPT_KILLI))
	  continue;
	/* timer has expired, clear entry */
	arptfree(atab, at);
      }
    }
    ARPTAB_UNLOCK(atab);
  }
}

/*
 * Broadcast an ARP packet, asking who has addr on interface ssc.
 */
static void
arpwhohas(register struct sana_softc *ssc, struct in_addr *addr)
{
  register struct mbuf *m;
  register struct s2_arppkt *s2a;
  struct sockaddr_sana2 ss2;

  if ((m = m_gethdr(M_DONTWAIT, MT_DATA)) == NULL)
    return;
  m->m_len = sizeof(*s2a);
  m->m_pkthdr.len = sizeof(*s2a);
  MH_ALIGN(m, sizeof(*s2a));
  s2a = mtod(m, struct s2_arppkt *);
  aligned_bzero_const((caddr_t)s2a, sizeof (*s2a));
  m->m_flags |= M_BCAST;

  /* fill in header depending of the interface */
  s2a->arp_hrd = htons(ssc->ss_arp.hrd);
  s2a->arp_pro = htons(ssc->ss_ip.type);
  s2a->arp_pln = sizeof(struct in_addr); /* protocol address length */
  s2a->arp_hln = ssc->ss_if.if_addrlen;  /* hardware address length */
  s2a->arp_op = htons(ARPOP_REQUEST);

  /* Copy source hardware address */
  bcopy((caddr_t)ssc->ss_hwaddr, 
	(caddr_t)&s2a->arpdata, 
	s2a->arp_hln);
  /* Copy source protocol address */
  bcopy((caddr_t)&ssc->ss_ipaddr, 
	(caddr_t)&s2a->arpdata + s2a->arp_hln, 
	s2a->arp_pln);
  /* Zero target hardware address */
  bzero((caddr_t)&s2a->arpdata + s2a->arp_hln + s2a->arp_pln, 
	s2a->arp_hln);
  /* Copy target protocol address */
  bcopy((caddr_t)addr, 
	(caddr_t)&s2a->arpdata + 2 * s2a->arp_hln + s2a->arp_pln, 
	s2a->arp_pln);

  /* Send an ARP packet */
  ss2.ss2_len = sizeof(ss2);
  ss2.ss2_family = AF_UNSPEC;
  ss2.ss2_type = ssc->ss_arp.type; 
  (*ssc->ss_if.if_output)(&ssc->ss_if, m, (struct sockaddr *)&ss2, 
			  (struct rtentry *)0);
}

/*
 * Resolve an IP address into an SANA-II address.  If success, 
 * desten is filled in.  If there is no entry in arptab,
 * set one up and broadcast a request for the IP address.
 * Hold onto this mbuf and resend it once the address
 * is finally resolved.  A return value of 1 indicates
 * that desten has been filled in and the packet should be sent
 * normally; a 0 return indicates that the packet has been
 * taken over here, either now or for later transmission.
 *
 * We do some (conservative) locking here at splimp, since
 * arptab is also altered from sana poll routine 
 */
int
arpresolve(register struct sana_softc *ssc,
	   struct mbuf *m,
	   register struct in_addr *destip,
	   register u_char *desten,
	   int *error)
{
  register struct arptab *at;
  register struct arptable *atb;
  struct sockaddr_in sin;
  register struct in_ifaddr *ia;
  u_long lna;

  if (m->m_flags & M_BCAST) {	/* broadcast */
    return 1;
  }
  lna = in_lnaof(*destip);
  /* if for us, use software loopback driver if up */
  for (ia = in_ifaddr; ia; ia = ia->ia_next)
    if ((ia->ia_ifp == &ssc->ss_if) &&
	(destip->s_addr == ia->ia_addr.sin_addr.s_addr)) {
      /*
       * This test used to be
       *	if (loif.if_flags & IFF_UP)
       * It allowed local traffic to be forced
       * through the hardware by configuring the loopback down.
       * However, it causes problems during network configuration
       * for boards that can't receive packets they send.
       * It is now necessary to clear "useloopback"
       * to force traffic out to the hardware.
       */
      if (useloopback) {
	sin.sin_family = AF_INET;
	sin.sin_addr = *destip;
	(void) looutput(&loif, m, (struct sockaddr *)&sin, 0);
	/*
	 * The packet has already been sent and freed.
	 */
	return (0);
      } else {
	bcopy((caddr_t)ssc->ss_hwaddr, (caddr_t)desten, ssc->ss_if.if_addrlen);
	return (1);
      }
    }

  if (ssc->ss_if.if_flags & IFF_NOARP) {
    /* No arp */
    __log(LOG_ERR, 
	"arpresolve: can't resolve address for if %s/%ld\n", 
	ssc->ss_if.if_name, ssc->ss_if.if_unit);
    *error = ENETUNREACH;
    m_freem(m);
    return (0);
  } 

  /* Try to locate ARP table */ 
  if (!(atb = ssc->ss_arp.table)) {
    alloc_arptable(ssc, 0);
    if (!(atb = ssc->ss_arp.table)) {
      __log(LOG_ERR, "arpresolve: memory exhausted");
      *error = ENOBUFS; 
      m_free(m); 
      return 0;
    }
  }

  ARPTAB_LOCK(atb);
  at = arptab_look(atb, destip->s_addr);
  if (at == 0) {		/* not found */
    at = arptnew(destip->s_addr, atb, FALSE);
    if (at) {
      at->at_hold = m;
      arpwhohas(ssc, destip);
    } else {
      __log(LOG_ERR, "arpresolve: no free entry");
      *error = ENETUNREACH;
      m_free(m); 
    } 
    ARPTAB_UNLOCK(atb);
    return 0;
  }

  at->at_timer = 0;		/* restart the timer */
  if (at->at_flags & ATF_COM) {	/* entry IS complete */
    bcopy((caddr_t)at->at_hwaddr, (caddr_t)desten, ssc->ss_if.if_addrlen);
    ARPTAB_UNLOCK(atb);
    return 1;
  }
  /*
   * There is an arptab entry, but no address response yet.  
   * Replace the held mbuf with this latest one.
   */
  if (at->at_hold)
    m_freem(at->at_hold);
  at->at_hold = m;
  arpwhohas(ssc, destip);	/* ask again */
  ARPTAB_UNLOCK(atb);
  return 0;
}

/*
 * Called from the sana poll routine
 * when ARP type packet is received.  
 * Common length and type checks are done here,
 * then the protocol-specific routine is called.
 * In addition, a sanity check is performed on the sender
 * protocol address, to catch impersonators.
 */
void
arpinput(struct sana_softc *ssc,
	struct mbuf *m,
        caddr_t srcaddr)
{
  register struct arphdr *ar;
  int proto;

  if (ssc->ss_if.if_flags & IFF_NOARP)
    goto out;
  if (m->m_len < sizeof(struct arphdr))
    goto out;
  ar = mtod(m, struct arphdr *);
  if (ntohs(ar->ar_hrd) != ssc->ss_arp.hrd)
    goto out;
  if (m->m_len < sizeof(struct arphdr) + 2 * ar->ar_hln + 2 * ar->ar_pln)
    goto out;
  if (ar->ar_hln != ssc->ss_if.if_addrlen) 
    goto out;

#ifdef paranoid_arp_mode
  /* Sanity check */
  if (bcmp(srcaddr, (UBYTE*)ar + sizeof(*ar), ar->ar_hln)) {
    __log(LOG_ERR, "An ARP packet sent as %s", 
	sana_sprintf(srcaddr, ar->ar_hln));
    __log(LOG_ERR, " from address: %s!!\n", 
	sana_sprintf((UBYTE*)ar + sizeof(*ar), ar->ar_hln));
    goto out;
  }
#endif
  pfil_run_hooks(m, &ssc->ss_if, MIAMIPFBPT_ARP);
  proto = ntohs(ar->ar_pro);

  if (proto == ssc->ss_ip.type) {
    in_arpinput(ssc, m);
    return;
  } 

 out:
  m_freem(m);
}

/*
 * ARP for Internet protocols on SANA-II interfaces.
 * Algorithm is that given in RFC 826.
 */
static void
in_arpinput(register struct sana_softc *ssc,
	    struct mbuf *m)
{
  register struct s2_arppkt *s2a;
  struct sockaddr_in sin;
  struct in_addr isaddr, itaddr, myaddr;
  int op, completed = 0;
  caddr_t sha, spa, tha, tpa;
  size_t  len = ssc->ss_if.if_addrlen;

  s2a = mtod(m, struct s2_arppkt *);
  op = ntohs(s2a->arp_op);

  if (s2a->arp_pln != sizeof(struct in_addr))
    goto out;

  sha = (caddr_t)&(s2a->arpdata); /* other members must be calculated */
  bcopy(spa = sha + len, (caddr_t)&isaddr, sizeof (isaddr));
  tha = spa + sizeof(struct in_addr);
  bcopy(tpa = tha + len, (caddr_t)&itaddr, sizeof (itaddr));


  {
    register struct in_ifaddr *ia;
    struct in_ifaddr *maybe_ia = 0;

    /* Check for our own ARP packets */
    for (ia = in_ifaddr; ia; ia = ia->ia_next)
      if (ia->ia_ifp == &ssc->ss_if) {
	maybe_ia = ia;
	if ((itaddr.s_addr == ia->ia_addr.sin_addr.s_addr) ||
	    (isaddr.s_addr == ia->ia_addr.sin_addr.s_addr))
	  break;
      }
    if (maybe_ia == 0)
      goto out;
    myaddr = ia ? ia->ia_addr.sin_addr : maybe_ia->ia_addr.sin_addr;
    if (!bcmp(sha, (caddr_t)ssc->ss_hwaddr, len))
      goto out;			/* it's from me, ignore it. */
  }
#ifndef AMITCP
  if (!bcmp(sha, (caddr_t)etherbroadcastaddr, ac->ac_if.if_addrlen)) {
    __log(LOG_ERR,
	"arp: ether address is broadcast for IP address %lx!\n",
	ntohl(isaddr.s_addr));
    goto out;
  }
#endif

  /* Check for duplicate IP addresses */
  if (isaddr.s_addr == myaddr.s_addr) {
    __log(LOG_ERR,
	"duplicate IP address %lx!! sent from hardware address: %s\n",
	ntohl(isaddr.s_addr), 
	sana_sprintf(sha, len));
    itaddr = myaddr;
    if (op == ARPOP_REQUEST)
      goto reply;
    goto out;
  }
  
  {
    struct arptable *atb;
    register struct arptab *at = NULL; /* same as "merge" flag */
    
    /* Try to locate ARP table */ 
    if (!(atb = ssc->ss_arp.table)) {
      goto reply;
    }

    ARPTAB_LOCK(atb);
    at = arptab_look(atb, isaddr.s_addr);

    if (at) {
      bcopy(sha, (caddr_t)at->at_hwaddr, len);
      if ((at->at_flags & ATF_COM) == 0)
	completed = 1;
      at->at_flags |= ATF_COM;
      if (at->at_hold) {
	sin.sin_family = AF_INET;
	sin.sin_addr = isaddr;
	(*ssc->ss_if.if_output)(&ssc->ss_if, at->at_hold,
			       (struct sockaddr *)&sin, (struct rtentry *)0);
	at->at_hold = 0;
      }
    }
    if (at == 0 && itaddr.s_addr == myaddr.s_addr) {
      /* ensure we have a table entry */
      if (at = arptnew(isaddr.s_addr, atb, FALSE)) {
	bcopy(sha, (caddr_t)at->at_hwaddr, len);
	completed = 1;
	at->at_flags |= ATF_COM;
      }
    }
    ARPTAB_UNLOCK(atb);
  }

 reply:
  /*
   * Reply if this is an IP request
   */
  if (op != ARPOP_REQUEST) 
    goto out;

  if (itaddr.s_addr == myaddr.s_addr) {
    /* I am the target */
    bcopy(sha, tha, len);
    bcopy((caddr_t)ssc->ss_hwaddr, sha, len);
  } else {
    /* Answer if we have a public entry */
    register struct arptab *at;
    
    /* Try to locate ARP table */ 
    if (!ssc->ss_arp.table)
      goto out;

    ARPTAB_LOCK(ssc->ss_arp.table);
    at = arptab_look(ssc->ss_arp.table, itaddr.s_addr);
    if (at && (at->at_flags & ATF_PUBL)) {
      bcopy(sha, tha, len);
      bcopy(at->at_hwaddr, sha, len);
    } else {
      at = NULL;
    }
    ARPTAB_UNLOCK(ssc->ss_arp.table);
    if (!at) 
      goto out;
  }
  {
    struct sockaddr_sana2 ss2;
    bcopy(spa, tpa, sizeof(struct in_addr));
    bcopy((caddr_t)&itaddr, spa, sizeof(struct in_addr));
    s2a->arp_op = htons(ARPOP_REPLY); 

    ss2.ss2_len = sizeof(ss2);
    ss2.ss2_family = AF_UNSPEC;
    ss2.ss2_type = ssc->ss_arp.type;
    bcopy(tha, ss2.ss2_host, len);

    m->m_flags &= ~(M_BCAST|M_MCAST);

    pfil_run_hooks(m, &ssc->ss_if, MIAMIPFBPT_ARP);
    (*ssc->ss_if.if_output)(&ssc->ss_if, m, (struct sockaddr *)&ss2, 
			    (struct rtentry *)0);
    return;
  }
 out:
  m_freem(m);
  return;
}

int
arpioctl(cmd, data)
	int cmd;
	caddr_t data;
{
  register struct arpreq *ar = (struct arpreq *)data;
  register struct arptab *at;
  register struct sockaddr_in *sin;
  struct arptable *atb;
  struct ifaddr *ifa;
  struct sana_softc *ssc;
  spl_t s;

  sin = (struct sockaddr_in *)&ar->arp_pa;
  sin->sin_len = sizeof(ar->arp_pa);

  if (ar->arp_pa.sa_family != AF_INET ||
      ar->arp_ha.sa_family != AF_UNSPEC)
    return (EAFNOSUPPORT);

  s = splimp();
  if ((ifa = ifa_ifwithnet(&ar->arp_pa)) == NULL) {
    splx(s);
    return (ENETUNREACH);
  }

  ssc = (struct sana_softc *)ifa->ifa_ifp;
  splx(s);

/*if (ssc->ss_if.if_type != IFT_SANA || !(atb = ssc->ss_arp.table)) {*/
  if (ssc->ss_if.if_output != sana_output || !(atb = ssc->ss_arp.table)) {
    return (EAFNOSUPPORT);
  }

  ARPTAB_LOCK(atb);

  if (cmd != SIOCGARPT) {
    at = arptab_look(atb, sin->sin_addr.s_addr);
    if (at == NULL && cmd != SIOCSARP) {
      ARPTAB_UNLOCK(atb);
      return (ENXIO);
    }
  }

  switch (cmd) {

  case SIOCSARP:		/* set entry */
    if (ar->arp_ha.sa_len > sizeof(at->at_hwaddr) + 2 ||
	ar->arp_ha.sa_len != ssc->ss_if.if_addrlen + 2) {
      ARPTAB_UNLOCK(atb);
      return (EINVAL);
    }
    /*
     * Free if new entry should be allocated in a different way 
     */
    if (at != NULL && (at->at_flags ^ ar->arp_flags) & ATF_PERM) {
      arptfree(atb, at);
      at = NULL;
    }
    if (at == NULL) {
      at = arptnew(sin->sin_addr.s_addr, atb, ar->arp_flags & ATF_PERM);
      if (at == NULL) {
	ARPTAB_UNLOCK(atb);
	return (EADDRNOTAVAIL);
      }
    }

    bcopy((caddr_t)ar->arp_ha.sa_data, (caddr_t)at->at_hwaddr,
	  ar->arp_ha.sa_len - 2);
    at->at_flags = ATF_COM | ATF_INUSE | 
      (ar->arp_flags & (ATF_PERM|ATF_PUBL));
    at->at_timer = 0;
    break;

  case SIOCDARP:		/* delete entry */
    arptfree(atb, at);
    break;

  case SIOCGARP:		/* get entry */
    bcopy((caddr_t)at->at_hwaddr, (caddr_t)ar->arp_ha.sa_data,
	  ar->arp_ha.sa_len = ssc->ss_if.if_addrlen + 2);
    ar->arp_flags = at->at_flags;
    break;

  case SIOCGARPT:		/* get table */
    {
      int i, n; long siz;
      register struct arptabreq *atr = (struct arptabreq *)data;
      ar = atr->atr_table;
      siz = ar ? atr->atr_size : 0;

      for (n = i = 0; i < ARPTAB_HSIZE; i++) {
	for (at = (struct arptab *)atb->atb_entries[i].mlh_Head; 
	     at->at_succ; 
	     at = at->at_succ) {
	  n++;
	  if (siz > 0) {
	    struct sockaddr_in *sin = (struct sockaddr_in *)&ar->arp_pa;
	    sin->sin_len = sizeof(*sin);
	    sin->sin_family = AF_INET;
	    sin->sin_addr = at->at_iaddr;
	    bcopy((caddr_t)at->at_hwaddr, (caddr_t)ar->arp_ha.sa_data,
		  ar->arp_ha.sa_len = ssc->ss_if.if_addrlen + 2);
	    ar->arp_flags = at->at_flags;
	    siz--;
	    ar++;
	  }
	}
      }
      atr->atr_size -= siz;
      atr->atr_inuse = n;
    }
  }

  ARPTAB_UNLOCK(atb);
  return 0;
}

static const char *digits = "0123456789ABCDEF";
/*
 * Print Hardware Address
 */
static char *sana_sprintf(register u_char *ap, int len)
{
  register i;
  static char addrbuf[17*3];
  register unsigned char *cp = addrbuf;

  for (i = 0; i < len; ) {
    *cp++ = digits[*ap >> 4];
    *cp++ = digits[*ap++ & 0xf];
    i++;
    if (i < len) 
      *cp++ = ':';
  }
  *cp = 0;
  return (addrbuf);
}
