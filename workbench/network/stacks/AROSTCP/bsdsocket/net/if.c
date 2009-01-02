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
 * Copyright (c) 1980, 1986 Regents of the University of California.
 * All rights reserved.
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
 *	@(#)if.c	7.14 (Berkeley) 4/20/91
 */

#include "conf.h"

#include <dos/rdargs.h>
#include <net/route.h>

#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/kernel.h>
#include <sys/sockio.h>
#include <sys/synch.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <netinet/in.h> /* for findid */

void ifinit(void);
void if_attach(struct ifnet *);
struct ifaddr *ifa_ifwithaddr(register struct sockaddr *);
struct ifaddr *ifa_ifwithdstaddr(register struct sockaddr *);
struct ifaddr *ifa_ifwithnet(struct sockaddr *);
struct ifaddr *ifa_ifwithaf(register int );
struct ifaddr *ifaof_ifpforaddr(struct sockaddr *, register struct ifnet *);
void link_rtrequest(int, struct rtentry *, struct sockaddr *);
void if_down(register struct ifnet *);
void if_qflush(register struct ifqueue *);
void if_slowtimo(void);
struct ifnet *ifunit(register char *);
int ifioctl(struct socket *, int, caddr_t);
int ifconf(int, caddr_t);

/* Compatibility with AmiTCP/IP 2 */
#include <sys/a_ioctl.h>
#include <net/a_if.h>
struct ifnet *aifunit(register char *name);
int aifconf(int cmd, caddr_t data);

#include <kern/uipc_domain_protos.h>

static char *sprint_d(u_int n, char *buf, int buflen);
int	ifqmaxlen = IFQ_MAXLEN;

struct	ifnet *ifnet = NULL;

/*
 * Network interface utility routines.
 *
 * Routines with ifa_ifwith* names take sockaddr *'s as
 * parameters.
 */

void ifinit(void)
{
	register struct ifnet *ifp;

	for (ifp = ifnet; ifp; ifp = ifp->if_next)
		if (ifp->if_snd.ifq_maxlen == 0)
			ifp->if_snd.ifq_maxlen = ifqmaxlen;
	if_slowtimo();
}

#ifdef vax
/*
 * Call each interface on a Unibus reset.
 */
ifubareset(uban)
	int uban;
{
	register struct ifnet *ifp;

	for (ifp = ifnet; ifp; ifp = ifp->if_next)
		if (ifp->if_reset)
			(*ifp->if_reset)(ifp->if_unit, uban);
}
#endif

int if_index = 0;
struct ifaddr **ifnet_addrs;

/*
 * Attach an interface to the
 * list of "active" interfaces.
 */
void
if_attach(ifp)
     struct ifnet *ifp;
{
	unsigned socksize, ifasize;
	int namelen, unitlen;
	char workbuf[12], *unitname;
	register struct sockaddr_dl *sdl;
	register struct ifaddr *ifa;
	static int if_indexlim = 8;
	register struct ifnet **p = &ifnet;

	while (*p)
		p = &((*p)->if_next);
	*p = ifp;
	ifp->if_index = ++if_index;
	if (ifnet_addrs == 0 || if_index >= if_indexlim) {
		unsigned n = (if_indexlim <<= 1) * sizeof(ifa);
		struct ifaddr **q = (struct ifaddr **)
					bsd_malloc(n, M_IFADDR, M_WAITOK);
		if (ifnet_addrs) {
			aligned_bcopy((caddr_t)ifnet_addrs, (caddr_t)q, n/2);
			bsd_free((caddr_t)ifnet_addrs, M_IFADDR);
		}
		ifnet_addrs = q;
	}
	/*
	 * create a Link Level name for this device
	 */
	unitname = sprint_d((u_int)ifp->if_unit, workbuf, sizeof(workbuf));
	namelen = strlen(ifp->if_name);
        unitlen = strlen(unitname);
#define _offsetof(t, m) ((int)((caddr_t)&((t *)0)->m))
	socksize = _offsetof(struct sockaddr_dl, sdl_data[0]) +
			       unitlen + namelen + ifp->if_addrlen;
#define ROUNDUP(a) (1 + (((a) - 1) | (sizeof(long) - 1)))
	socksize = ROUNDUP(socksize);
	if (socksize < sizeof(*sdl))
		socksize = sizeof(*sdl);
	ifasize = sizeof(*ifa) + 2 * socksize;
	ifa = (struct ifaddr *)bsd_malloc(ifasize, M_IFADDR, M_WAITOK);
	if (ifa == 0)
		return;
	ifnet_addrs[if_index - 1] = ifa;
	aligned_bzero((caddr_t)ifa, ifasize);
	sdl = (struct sockaddr_dl *)(ifa + 1);
	ifa->ifa_addr = (struct sockaddr *)sdl;
	ifa->ifa_ifp = ifp;
	sdl->sdl_len = socksize;
	sdl->sdl_family = AF_LINK;
	bcopy(ifp->if_name, sdl->sdl_data, namelen);
	bcopy(unitname, namelen + (caddr_t)sdl->sdl_data, unitlen);
	sdl->sdl_nlen = (namelen += unitlen);
	sdl->sdl_index = ifp->if_index;
	/* Fill in the hardware address */
	sdl->sdl_type = ifp->if_type;
	sdl->sdl_alen = ifp->if_addrlen;
	if ((ifp->if_output == sana_output) && (sdl->sdl_alen))
		bcopy((caddr_t)((struct sana_softc *)ifp)->ss_hwaddr,LLADDR(sdl), sdl->sdl_alen);
	sdl = (struct sockaddr_dl *)(socksize + (caddr_t)sdl);
	ifa->ifa_netmask = (struct sockaddr *)sdl;
	sdl->sdl_len = socksize - ifp->if_addrlen;
	while (namelen != 0)
		sdl->sdl_data[--namelen] = (char)0xff;
	ifa->ifa_next = ifp->if_addrlist;
	ifa->ifa_rtrequest = link_rtrequest;
	ifp->if_addrlist = ifa;
}

/*
 * Locate an interface based on a complete address.
 */

/*
 * Find an interface address suitable for host id. If no suitable interface
 * address is found, the *id is not touched.
 *
 * This routine stops after the first non-loopback AF_INET address is found.
 *
 * This routine is specially made for the gethostid() system call.
 */
void
findid(unsigned long *id)
{
  struct ifnet *ifp;
  struct ifaddr *ifa;

  spl_t s = splimp();

  for (ifp = ifnet; ifp; ifp = ifp->if_next)
    if (ifp->if_flags & IFF_UP)
      for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
	if (ifa->ifa_addr->sa_family == AF_INET) {
	  *id = ((struct sockaddr_in *)ifa->ifa_addr)->sin_addr.s_addr;
	  if (!(ifp->if_flags & IFF_LOOPBACK))
	    goto ret;
	}
 ret:
  splx(s);
}

/*ARGSUSED*/
struct ifaddr *
ifa_ifwithaddr(addr)
	register struct sockaddr *addr;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;

#define	equal(a1, a2) \
  (bcmp((caddr_t)(a1), (caddr_t)(a2), ((struct sockaddr *)(a1))->sa_len) == 0)
	for (ifp = ifnet; ifp; ifp = ifp->if_next)
	    for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family != addr->sa_family)
			continue;
		if (equal(addr, ifa->ifa_addr))
			return (ifa);
		if ((ifp->if_flags & IFF_BROADCAST) && ifa->ifa_broadaddr &&
		    equal(ifa->ifa_broadaddr, addr))
			return (ifa);
	}
	return ((struct ifaddr *)0);
}
/*
 * Locate the point to point interface with a given destination address.
 */
/*ARGSUSED*/
struct ifaddr *
ifa_ifwithdstaddr(addr)
	register struct sockaddr *addr;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;

	for (ifp = ifnet; ifp; ifp = ifp->if_next) 
	    if (ifp->if_flags & IFF_POINTOPOINT)
		for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr->sa_family != addr->sa_family)
				continue;
			if (equal(addr, ifa->ifa_dstaddr))
				return (ifa);
	}
	return ((struct ifaddr *)0);
}

/*
 * Find an interface on a specific network.  If many, choice
 * is first found.
 */
struct ifaddr *
ifa_ifwithnet(addr)
	struct sockaddr *addr;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;
	u_int af = addr->sa_family;

	if (af >= AF_MAX)
		return (0);
	if (af == AF_LINK) {
	    register struct sockaddr_dl *sdl = (struct sockaddr_dl *)addr;
	    if (sdl->sdl_index && sdl->sdl_index <= if_index)
		return (ifnet_addrs[sdl->sdl_index - 1]);
	}
	for (ifp = ifnet; ifp; ifp = ifp->if_next)
	    for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
		register char *cp, *cp2, *cp3;
		register char *cplim;
		if (ifa->ifa_addr->sa_family != af || ifa->ifa_netmask == 0)
			continue;
		cp = addr->sa_data;
		cp2 = ifa->ifa_addr->sa_data;
		cp3 = ifa->ifa_netmask->sa_data;
		cplim = ifa->ifa_netmask->sa_len + (char *)ifa->ifa_netmask;
		for (; cp3 < cplim; cp3++)
			if ((*cp++ ^ *cp2++) & *cp3)
				break;
		if (cp3 == cplim)
			return (ifa);
	    }
	return ((struct ifaddr *)0);
}

/*
 * Find an interface using a specific address family
 */
struct ifaddr *
ifa_ifwithaf(af)
	register int af;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;

	for (ifp = ifnet; ifp; ifp = ifp->if_next)
	    for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
		if (ifa->ifa_addr->sa_family == af)
			return (ifa);
	return ((struct ifaddr *)0);
}

/*
 * Find an interface address specific to an interface best matching
 * a given address.
 */
struct ifaddr *
ifaof_ifpforaddr(addr, ifp)
	struct sockaddr *addr;
	register struct ifnet *ifp;
{
	register struct ifaddr *ifa;
	register char *cp, *cp2, *cp3;
	register char *cplim;
	struct ifaddr *ifa_maybe = 0;
	u_int af = addr->sa_family;

	if (af >= AF_MAX)
		return (0);
	for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family != af)
			continue;
		ifa_maybe = ifa;
		if (ifa->ifa_netmask == 0) {
			if (equal(addr, ifa->ifa_addr) ||
			    (ifa->ifa_dstaddr && equal(addr, ifa->ifa_dstaddr)))
				return (ifa);
			continue;
		}
		cp = addr->sa_data;
		cp2 = ifa->ifa_addr->sa_data;
		cp3 = ifa->ifa_netmask->sa_data;
		cplim = ifa->ifa_netmask->sa_len + (char *)ifa->ifa_netmask;
		for (; cp3 < cplim; cp3++)
			if ((*cp++ ^ *cp2++) & *cp3)
				break;
		if (cp3 == cplim)
			return (ifa);
	}
	return (ifa_maybe);
}

#include <net/route.h>

/*
 * Default action when installing a route with a Link Level gateway.
 * Lookup an appropriate real ifa to point to.
 * This should be moved to /sys/net/link.c eventually.
 */
void
link_rtrequest(int cmd, struct rtentry *rt, struct sockaddr *sa)
{
	register struct ifaddr *ifa;
	struct sockaddr *dst;
	struct ifnet *ifp;

	if (cmd != RTM_ADD || ((ifa = rt->rt_ifa) == 0) ||
	    ((ifp = ifa->ifa_ifp) == 0) || ((dst = rt_key(rt)) == 0))
		return;
	if (ifa = ifaof_ifpforaddr(dst, ifp)) {
		rt->rt_ifa = ifa;
		if (ifa->ifa_rtrequest && 
		    ifa->ifa_rtrequest != link_rtrequest)
			ifa->ifa_rtrequest(cmd, rt, sa);
	}
}

/*
 * Mark an interface down and notify protocols of
 * the transition.
 * NOTE: must be called at splnet or eqivalent.
 */
void
if_down(ifp)
	register struct ifnet *ifp;
{
	register struct ifaddr *ifa;

	ifp->if_flags &= ~IFF_UP;
	for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
		pfctlinput(PRC_IFDOWN, ifa->ifa_addr);
	if_qflush(&ifp->if_snd);
}

/*
 * Flush an interface queue.
 */
void
if_qflush(ifq)
	register struct ifqueue *ifq;
{
	register struct mbuf *m, *n;

	n = ifq->ifq_head;
	while (m = n) {
		n = m->m_act;
		m_freem(m);
	}
	ifq->ifq_head = 0;
	ifq->ifq_tail = 0;
	ifq->ifq_len = 0;
}

/*
 * Handle interface watchdog timer routines.  Called
 * from softclock, we decrement timers (if set) and
 * call the appropriate interface routine on expiration.
 */
void
if_slowtimo()
{
  /*
   * This routine is disabled since there are 
   * no timeouts in our network interfaces
   */
#ifndef AMITCP			
	register struct ifnet *ifp;
	spl_t s = splimp();

	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (ifp->if_timer == 0 || --ifp->if_timer)
			continue;
		if (ifp->if_watchdog)
			(*ifp->if_watchdog)(ifp->if_unit);
	}
	splx(s);
#ifndef AMITCP
	/*
	 * Timeouts are scheduled from amiga_time.c in AmiTCP/IP.
	 */
	timeout(if_slowtimo, (caddr_t)0, hz / IFNET_SLOWHZ);
#endif
#endif
}

/*
 * Map interface name to
 * interface structure pointer.
 */
struct ifnet *
ifunit(char *name)
{
	register struct ifnet *ifp;
	register char *cp;
	int unit;
	unsigned len;
	char *ep, c;

	for (cp = name; cp < name + IFNAMSIZ && *cp; cp++)
		if (*cp >= '0' && *cp <= '9')
			break;
	if (*cp == '\0' || cp == name + IFNAMSIZ)
		return ((struct ifnet *)0);
	/*
	 * Save first char of unit, and pointer to it,
	 * so we can put a null there to avoid matching
	 * initial substrings of interface names.
	 */
	len = cp - name + 1;
	c = *cp;
	ep = cp;
	for (unit = 0; *cp >= '0' && *cp <= '9'; )
		unit = unit * 10 + *cp++ - '0';
	*ep = 0;

	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (bcmp(ifp->if_name, name, len))
			continue;
		if (unit == ifp->if_unit)
			break;
	}
	*ep = c;
	return (ifp);
}

/*
 * Interface ioctls.
 */
int
ifioctl(so, cmd, data)
	struct socket *so;
	int cmd;
	caddr_t data;
{
	register struct ifnet *ifp;
	register struct ifreq *ifr;
#ifndef AMITCP
	int error;
#endif
	extern int arpioctl(int cmd, caddr_t data);

	switch (cmd) {

	case SIOCGIFCONF:
		return (ifconf(cmd, data));

#ifdef COMPAT_AMITCP2
	case ASIOCGIFCONF:
		return (aifconf(cmd, data));
#endif

#if INET && NETHER > 0
	case SIOCSARP:
	case SIOCDARP:
#ifndef AMITCP /* no protection on AmigaOS */
		if (error = suser(p->p_ucred, &p->p_acflag))
			return (error);
		/* FALL THROUGH */
#else
	case SIOCGARPT:
#endif /* AMITCP */
	case SIOCGARP:
		return (arpioctl(cmd, data));
#endif
	}

#ifndef COMPAT_AMITCP2
	/* Do we have old ioctl? */
	if (IOCGROUP(cmd) == 'I') {
		struct aifreq *aifr;
		aifr = (struct aifreq *)data;
		/* Nobody needs interface name after we have got ifp */
		ifp = aifunit(aifr->ifr_name);
		ifr = (struct ifreq *)(data + AIFNAMSIZ - IFNAMSIZ);
		cmd -= ASIOCSIFADDR - SIOCSIFADDR;
	} 
	else 
#endif
	{
		ifr = (struct ifreq *)data;
		ifp = ifunit(ifr->ifr_name);
	}

	if (ifp == 0)
		return (ENXIO);
	switch (cmd) {

        case SIOCGIFINDEX:
                ifr->ifr_index = ifp->if_index;
                break;

	case SIOCGIFFLAGS:
		ifr->ifr_flags = ifp->if_flags;
		break;

	case SIOCGIFMETRIC:
		ifr->ifr_metric = ifp->if_metric;
		break;

	case SIOCGIFMTU:
		ifr->ifr_mtu = ifp->if_mtu;
		break;

	case SIOCSIFFLAGS:
		/* if_down() is kludged for Sana-II driver ioctl */
		if (ifp->if_flags & IFF_UP && (ifr->ifr_flags & IFF_UP) == 0) {
			spl_t s = splimp();
			if_down(ifp);
			splx(s);
		}

		if (ifp->if_ioctl)
			(void) (*ifp->if_ioctl)(ifp, cmd, data);

		ifp->if_flags = (ifp->if_flags & IFF_CANTCHANGE) |
			(ifr->ifr_flags &~ IFF_CANTCHANGE);
		break;

	case SIOCSIFMETRIC:
#ifndef AMITCP /* no protection on AmigaOS */
		if (error = suser(p->p_ucred, &p->p_acflag))
			return (error);
#endif /* AMITCP */
		ifp->if_metric = ifr->ifr_metric;
		break;

	default:
		if (so->so_proto == 0)
			return (EOPNOTSUPP);
		return ((*so->so_proto->pr_usrreq)(so,
						   PRU_CONTROL,
						   (struct mbuf *)cmd,
						   (struct mbuf *)ifr,
						   (struct mbuf *)ifp));
	}
	return (0);
}

/*
 * A helper routine for internal interface control
 * Here we do the same as SIOCSIFFLAGS processing code from net/if.c
 */

void ifupdown(struct ifnet *ifp, int up) {
	struct ifreq ifr;

	if (up) {
		ifr.ifr_flags = ifp->if_flags & ~IFF_UP;
		if (ifp->if_flags & IFF_UP) {
			spl_t s = splimp();
			if_down(ifp);
			splx(s);
		}
	} else {
		ifr.ifr_flags = ifp->if_flags | IFF_UP;
	}
	if (ifp->if_ioctl)
		(void) (*ifp->if_ioctl)(ifp, SIOCSIFFLAGS, (caddr_t)&ifr);
	ifp->if_flags = ifr.ifr_flags;
}

/*
 * Return interface configuration
 * of system.  List may be used
 * in later ioctl's (above) to get
 * other information.
 */
int
ifconf(cmd, data)
	int cmd;
	caddr_t data;
{
	register struct ifconf *ifc = (struct ifconf *)data;
	register struct ifnet *ifp = ifnet;
	register struct ifaddr *ifa;
	register char *cp, *ep;
	struct ifreq ifr, *ifrp;
	int space = ifc->ifc_len, error = 0;

	ifrp = ifc->ifc_req;
#ifndef AMITCP
	ep = ifr.ifr_name + sizeof (ifr.ifr_name) - 2;
#endif
	for (; space > sizeof (ifr) && ifp; ifp = ifp->if_next) {
#ifdef AMITCP
		ep = sprint_d(ifp->if_unit, ifr.ifr_name, sizeof(ifr.ifr_name));
		/* Copy the interface name into ifr */
		bcopy(ifp->if_name, ifr.ifr_name, ep - ifr.ifr_name);
		/* Find the end of interface name */
		for (cp = ifr.ifr_name; cp < ep && *cp; cp++)
			;
		/* Append unit number to it */
		for (; *cp = *ep; cp++, ep++)
			;
#else
		bcopy(ifp->if_name, ifr.ifr_name, sizeof (ifr.ifr_name) - 2);
		for (cp = ifr.ifr_name; cp < ep && *cp; cp++)
			;
		*cp++ = '0' + ifp->if_unit; *cp = '\0';
#endif
		if ((ifa = ifp->if_addrlist) == 0) {
			aligned_bzero_const((caddr_t)&ifr.ifr_addr, sizeof(ifr.ifr_addr));
			bcopy(&ifr, ifrp, sizeof(ifr));
			space -= sizeof (ifr), ifrp++;
		} else 
		    for ( ; space > sizeof (ifr) && ifa; ifa = ifa->ifa_next) {
			register struct sockaddr *sa = ifa->ifa_addr;
#ifdef COMPAT_43
			if (cmd == OSIOCGIFCONF) {
				struct osockaddr *osa =
					 (struct osockaddr *)&ifr.ifr_addr;
				ifr.ifr_addr = *sa;
				osa->sa_family = sa->sa_family;
				error = copyout((caddr_t)&ifr, (caddr_t)ifrp,
						sizeof (ifr));
				ifrp++;
			} else
#endif
			if (sa->sa_len <= sizeof(*sa)) {
				ifr.ifr_addr = *sa;
				bcopy(&ifr, ifrp, sizeof(ifr));
				ifrp++;
			} else {
				space -= sa->sa_len - sizeof(*sa);
				if (space < sizeof (ifr))
					break;
#ifdef AMITCP
				aligned_bcopy_const((caddr_t)&ifr, 
						    (caddr_t)ifrp,
						    sizeof (ifr.ifr_name));
				aligned_bcopy((caddr_t)sa,
				      (caddr_t)&ifrp->ifr_addr, sa->sa_len);
#else
				error = copyout((caddr_t)&ifr, (caddr_t)ifrp,
						sizeof (ifr.ifr_name));
				if (error == 0)
				    error = copyout((caddr_t)sa,
				      (caddr_t)&ifrp->ifr_addr, sa->sa_len);
#endif
				ifrp = (struct ifreq *)
					(sa->sa_len + (caddr_t)&ifrp->ifr_addr);
			}
#ifndef AMITCP
			if (error)
				break;
#endif
			space -= sizeof (ifr);
		}
	}
	ifc->ifc_len -= space;
	return (error);
}

int aifconf(int cmd, caddr_t data)
{
	return ENOSYS;
}

static char *
sprint_d(n, buf, buflen)
	u_int n;
	char *buf;
	int buflen;
{
	register char *cp = buf + buflen - 1;

	*cp = 0;
	do {
		cp--;
		*cp = "0123456789"[n % 10];
		n /= 10;
	} while (n != 0);
	return (cp);
}
