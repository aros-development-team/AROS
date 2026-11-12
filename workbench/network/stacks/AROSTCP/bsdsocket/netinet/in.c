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

/*
 * Copyright (C) 1982, 1986, 1991 Regents of the University of California.
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
 *	@(#)in.c	7.17 (Berkeley) 4/20/91
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/synch.h>

#include <netinet/in_systm.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include "in_var.h"

#include <netinet/in_protos.h>
#include <net/rtsock_protos.h>

#ifdef ENABLE_MULTICAST
#include <net/if_sana.h>
#include <netinet/ip_var.h>
#include <netinet/in_pcb.h>
#include <netinet/inetfamily.h>
#include <netinet/ip_output_protos.h>
#include <kern/amiga_dhcp.h>
#include <net/sana2arp.h>
#include <net/netmodule.h>
#include <devices/sana2.h>
#include <netinet/igmp.h>
#endif
#include <sys/syslog.h>
#include <protos/net/route_protos.h>
#include <net/netcore_map.h>        /* always the last include */

#if INET
/*
 * Formulate an Internet address from network + host.
 */
struct in_addr
in_makeaddr(net, host)
u_long net, host;
{
    register struct in_ifaddr *ia;
    register u_long mask;
    union {
        u_long addr;
        struct in_addr in_addr;
    } __tmp;

    if(IN_CLASSA(net))
        mask = IN_CLASSA_HOST;
    else if(IN_CLASSB(net))
        mask = IN_CLASSB_HOST;
    else
        mask = IN_CLASSC_HOST;
    for(ia = in_ifaddr; ia; ia = ia->ia_next)
        if((ia->ia_netmask & net) == ia->ia_net) {
            mask = ~ia->ia_subnetmask;
            break;
        }
    __tmp.addr = htonl(net | (host & mask));
    return __tmp.in_addr;
}

/*
 * Return the network number from an internet address.
 */
u_long
in_netof(in)
struct in_addr in;
{
    register u_long i = ntohl(in.s_addr);
    register u_long net;
    register struct in_ifaddr *ia;

    if(IN_CLASSA(i))
        net = i & IN_CLASSA_NET;
    else if(IN_CLASSB(i))
        net = i & IN_CLASSB_NET;
    else if(IN_CLASSC(i))
        net = i & IN_CLASSC_NET;
    else
        return (0);

    /*
     * Check whether network is a subnet;
     * if so, return subnet number.
     */
    for(ia = in_ifaddr; ia; ia = ia->ia_next)
        if(net == ia->ia_net)
            return (i & ia->ia_subnetmask);
    return (net);
}

/*
 * Compute and save network mask as sockaddr from an internet address.
 */
void
in_sockmaskof(in, sockmask)
struct in_addr in;
register struct sockaddr_in *sockmask;
{
    register u_long net;
    register u_long mask;
    {
        register u_long i = ntohl(in.s_addr);

        if(i == 0)
            net = 0, mask = 0;
        else if(IN_CLASSA(i))
            net = i & IN_CLASSA_NET, mask = IN_CLASSA_NET;
        else if(IN_CLASSB(i))
            net = i & IN_CLASSB_NET, mask = IN_CLASSB_NET;
        else if(IN_CLASSC(i))
            net = i & IN_CLASSC_NET, mask = IN_CLASSC_NET;
        else
            net = i, mask = -1;
    }
    {
        register struct in_ifaddr *ia;
        /*
         * Check whether network is a subnet;
         * if so, return subnet number.
         */
        for(ia = in_ifaddr; ia; ia = ia->ia_next)
            if(net == ia->ia_net)
                mask =  ia->ia_subnetmask;
    }
    {
        register char *cpbase = (char *) & (sockmask->sin_addr);
        register char *cp = (char *)(1 + & (sockmask->sin_addr));

        sockmask->sin_addr.s_addr = htonl(mask);
        sockmask->sin_len = 0;
        while(--cp >= cpbase)
            if(*cp) {
                sockmask->sin_len = 1 + cp - (caddr_t)sockmask;
                break;
            }
    }
}

/*
 * Return the host portion of an internet address.
 */
u_long
in_lnaof(in)
struct in_addr in;
{
    register u_long i = ntohl(in.s_addr);
    register u_long net, host;
    register struct in_ifaddr *ia;

    if(IN_CLASSA(i)) {
        net = i & IN_CLASSA_NET;
        host = i & IN_CLASSA_HOST;
    } else if(IN_CLASSB(i)) {
        net = i & IN_CLASSB_NET;
        host = i & IN_CLASSB_HOST;
    } else if(IN_CLASSC(i)) {
        net = i & IN_CLASSC_NET;
        host = i & IN_CLASSC_HOST;
    } else
        return (i);

    /*
     * Check whether network is a subnet;
     * if so, use the modified interpretation of `host'.
     */
    for(ia = in_ifaddr; ia; ia = ia->ia_next)
        if(net == ia->ia_net)
            return (host & ~ ia->ia_subnetmask);
    return (host);
}

#ifndef SUBNETSARELOCAL
#define	SUBNETSARELOCAL	1
#endif
int subnetsarelocal = SUBNETSARELOCAL;
/*
 * Return 1 if an internet address is for a ``local'' host
 * (one to which we have a connection).  If subnetsarelocal
 * is true, this includes other subnets of the local net.
 * Otherwise, it includes only the directly-connected (sub)nets.
 */
int
in_localaddr(in)
struct in_addr in;
{
    register u_long i = ntohl(in.s_addr);
    register struct in_ifaddr *ia;

    if(subnetsarelocal) {
        for(ia = in_ifaddr; ia; ia = ia->ia_next)
            if((i & ia->ia_netmask) == ia->ia_net)
                return (1);
    } else {
        for(ia = in_ifaddr; ia; ia = ia->ia_next)
            if((i & ia->ia_subnetmask) == ia->ia_subnet)
                return (1);
    }
    return (0);
}

/*
 * Determine whether an IP address is in a reserved set of addresses
 * that may not be forwarded, or whether datagrams to that destination
 * may be forwarded.
 */
int
in_canforward(in)
struct in_addr in;
{
    register u_long i = ntohl(in.s_addr);

    /* multicast and class E (224/3); link-local 169.254/16, which RFC
     * 3927 section 2.7 says must not be forwarded, as FreeBSD */
    if(IN_EXPERIMENTAL(i) || (i & 0xffff0000) == 0xa9fe0000)
        return (0);
    if(IN_CLASSA(i)) {
        register u_long net = i & IN_CLASSA_NET;
        /* net 0, and 127 - in its place, as 4.4BSD has it: the shift was
         * lost, and the loopback network went out as forwardable */
        if(net == 0 || net == (IN_LOOPBACKNET << IN_CLASSA_NSHIFT))
            return (0);
    }
    return (1);
}

int	in_interfaces;		/* number of external internet interfaces */

static int in_control_locked(struct socket *, int, caddr_t, struct ifnet *, u_char);

#ifdef ENABLE_MULTICAST
/*
 * Every multicast interface belongs to the all-hosts group, 224.0.0.1
 * (RFC 1112 section 7.2). IGMP queries are sent to it: an interface that
 * is not a member never hears one, never answers, and a snooping switch
 * or a router stops forwarding its groups when their membership times out.
 * No report is ever sent for this group itself.
 */
static void
in_join_allhosts(struct ifnet *ifp)
{
    struct in_addr allhosts;
    struct in_multi *inm;

    if((ifp->if_flags & IFF_MULTICAST) == 0)
        return;
    allhosts.s_addr = htonl(INADDR_ALLHOSTS_GROUP);
    IN_LOOKUP_MULTI(allhosts, ifp, inm);
    if(inm == NULL)
        (void)in_addmulti(&allhosts, ifp);
}
#endif

/*
 * Generic internet control operations (ioctl's).
 * Ifp is 0 if not an interface-specific ioctl.
 * The address lists are walked by ip_input, so they change at splimp.
 */
int
in_control(struct socket *so, int cmd, caddr_t data, struct ifnet *ifp)
{
    spl_t s;
    int error;
    u_char origin = IN_ORIGIN_MANUAL;

    /* SIOCAIFADDR saying where the address came from: its request first */
    if(cmd == SIOCAIFADDR_ORIGIN) {
        u_int o = ((struct in_originreq *)data)->ifro_origin;

        if(o > IN_ORIGIN_LINKLOCAL)
            return (EINVAL);
        origin = (u_char)o;
        cmd = SIOCAIFADDR;
    }
    s = splimp();
    error = in_control_locked(so, cmd, data, ifp, origin);
    splx(s);
    return (error);
}

static int
in_control_locked(so, cmd, data, ifp, origin)
struct socket *so;
int cmd;
caddr_t data;
register struct ifnet *ifp;
u_char origin;
{
    register struct ifreq *ifr = (struct ifreq *)data;
    register struct in_ifaddr *ia = 0;
    register struct ifaddr *ifa;
    struct in_ifaddr *oia;
    struct in_aliasreq *ifra = (struct in_aliasreq *)data;
    int error, hostIsNew, maskIsNew;
    u_long i;

    /*
     * Find address for this interface, if it exists.
     */
    if(ifp)
        for(ia = in_ifaddr; ia; ia = ia->ia_next)
            if(ia->ia_ifp == ifp)
                break;

    switch(cmd) {

    case SIOCAIFADDR:
    case SIOCDIFADDR:
        if(ifra->ifra_addr.sin_family == AF_INET)
            for(; ia; ia = ia->ia_next) {
                if(ia->ia_ifp == ifp  &&
                        ia->ia_addr.sin_addr.s_addr ==
                        ifra->ifra_addr.sin_addr.s_addr)
                    break;
            }
        if(cmd == SIOCDIFADDR && ia == 0)
            return (EADDRNOTAVAIL);
    /* FALLTHROUGH */
    case SIOCSIFADDR:
    case SIOCSIFNETMASK:
    case SIOCSIFDSTADDR:
        /*		if ((so->so_state & SS_PRIV) == 0)
        			return (EPERM);*/

        if(ifp == 0)
            panic("in_control");
        if(ia == (struct in_ifaddr *)0) {
            struct mbuf *m = m_getclr(M_WAIT, MT_IFADDR);
            if(m == (struct mbuf *)NULL)
                return (ENOBUFS);
            if(ia = in_ifaddr) {
                for(; ia->ia_next; ia = ia->ia_next)
                    ;
                ia->ia_next = mtod(m, struct in_ifaddr *);
            } else
                in_ifaddr = mtod(m, struct in_ifaddr *);
            ia = mtod(m, struct in_ifaddr *);
            if(ifa = ifp->if_addrlist) {
                for(; ifa->ifa_next; ifa = ifa->ifa_next)
                    ;
                ifa->ifa_next = (struct ifaddr *) ia;
            } else
                ifp->if_addrlist = (struct ifaddr *) ia;
            ia->ia_ifa.ifa_addr = (struct sockaddr *)&ia->ia_addr;
            ia->ia_ifa.ifa_dstaddr
                = (struct sockaddr *)&ia->ia_dstaddr;
            ia->ia_ifa.ifa_netmask
                = (struct sockaddr *)&ia->ia_sockmask;
            ia->ia_sockmask.sin_len = 8;
            if(ifp->if_flags & IFF_BROADCAST) {
                ia->ia_broadaddr.sin_len = sizeof(ia->ia_addr);
                ia->ia_broadaddr.sin_family = AF_INET;
            }
            ia->ia_ifp = ifp;
            if(ifp != &loif)
                in_interfaces++;
        }
        break;

    case SIOCGIFAFLAG_IN: {         /* an address's IN_IFF_* flags (NetBSD) */
        const struct sockaddr_in *sin = (const struct sockaddr_in *)&ifr->ifr_addr;

        for(; ia; ia = ia->ia_next)
            if(ia->ia_ifp == ifp && ia->ia_addr.sin_addr.s_addr == sin->sin_addr.s_addr)
                break;
        if(ia == NULL || sin->sin_family != AF_INET)
            return (EADDRNOTAVAIL);
        ifr->ifr_addrflags = ia->ia4_flags;
        return (0);
    }

    case SIOCSIFBRDADDR:
    /*		if ((so->so_state & SS_PRIV) == 0)
    			return (EPERM);*/
    /* FALLTHROUGH */

    case SIOCGIFADDR:
    case SIOCGIFNETMASK:
    case SIOCGIFDSTADDR:
    case SIOCGIFBRDADDR:
        if(ia == (struct in_ifaddr *)0)
            return (EADDRNOTAVAIL);
        break;

    default:
        return (EOPNOTSUPP);
        break;
    }
    switch(cmd) {

    case SIOCGIFADDR: {
        struct sockaddr_in *ifr_saddr = (struct sockaddr_in *)&ifr->ifr_addr;
        *ifr_saddr = ia->ia_addr;
    }
    break;

    case SIOCGIFBRDADDR: {
        struct sockaddr_in *ifrdst_saddr = (struct sockaddr_in *)&ifr->ifr_dstaddr;
        if((ifp->if_flags & IFF_BROADCAST) == 0)
            return (EINVAL);
        *ifrdst_saddr = ia->ia_broadaddr;
    }
    break;

    case SIOCGIFDSTADDR: {
        struct sockaddr_in *ifrdst_saddr = (struct sockaddr_in *)&ifr->ifr_dstaddr;
        if((ifp->if_flags & IFF_POINTOPOINT) == 0)
            return (EINVAL);
        *ifrdst_saddr = ia->ia_dstaddr;
    }
    break;

    case SIOCGIFNETMASK: {
        struct sockaddr_in *ifr_saddr = (struct sockaddr_in *)&ifr->ifr_addr;
        *ifr_saddr = ia->ia_sockmask;
    }
    break;

    case SIOCSIFDSTADDR: {
        struct sockaddr_in *ifrdst_saddr = (struct sockaddr_in *)&ifr->ifr_dstaddr;
        struct sockaddr_in oldaddr;
        if((ifp->if_flags & IFF_POINTOPOINT) == 0)
            return (EINVAL);
        oldaddr = ia->ia_dstaddr;
        ia->ia_dstaddr = *ifrdst_saddr;
        if(ifp->if_ioctl &&
                (error = (*ifp->if_ioctl)(ifp, SIOCSIFDSTADDR, (caddr_t)ia))) {
            ia->ia_dstaddr = oldaddr;
            return (error);
        }
        if(ia->ia_flags & IFA_ROUTE) {
            ia->ia_ifa.ifa_dstaddr = (struct sockaddr *)&oldaddr;
            rtinit(&(ia->ia_ifa), (int)RTM_DELETE, RTF_HOST);
            ia->ia_ifa.ifa_dstaddr =
                (struct sockaddr *)&ia->ia_dstaddr;
            rtinit(&(ia->ia_ifa), (int)RTM_ADD, RTF_HOST | RTF_UP);
        }
    }
    break;

    case SIOCSIFBRDADDR: {
        struct sockaddr_in *ifrbrd_saddr = (struct sockaddr_in *)&ifr->ifr_broadaddr;
        if((ifp->if_flags & IFF_BROADCAST) == 0)
            return (EINVAL);
        ia->ia_broadaddr = *ifrbrd_saddr;
    }
    break;

    case SIOCSIFADDR:
        ia->ia_origin = IN_ORIGIN_MANUAL;
        return (in_ifinit(ifp, ia,
                          (struct sockaddr_in *) &ifr->ifr_addr, 1));

    case SIOCSIFNETMASK:
        i = ifra->ifra_addr.sin_addr.s_addr;
        ia->ia_subnetmask = ntohl(ia->ia_sockmask.sin_addr.s_addr = i);
        break;

    case SIOCAIFADDR:
        maskIsNew = 0;
        hostIsNew = 1;
        error = 0;
        if(ia->ia_addr.sin_family == AF_INET) {
            if(ifra->ifra_addr.sin_len == 0) {
                ifra->ifra_addr = ia->ia_addr;
                hostIsNew = 0;
            } else if(ifra->ifra_addr.sin_addr.s_addr ==
                      ia->ia_addr.sin_addr.s_addr)
                hostIsNew = 0;
        }
        if(ifra->ifra_mask.sin_len) {
            in_ifscrub(ifp, ia);
            ia->ia_sockmask = ifra->ifra_mask;
            ia->ia_subnetmask =
                ntohl(ia->ia_sockmask.sin_addr.s_addr);
            maskIsNew = 1;
        }
        if((ifp->if_flags & IFF_POINTOPOINT) &&
                (ifra->ifra_dstaddr.sin_family == AF_INET)) {
            in_ifscrub(ifp, ia);
            ia->ia_dstaddr = ifra->ifra_dstaddr;
            maskIsNew  = 1; /* We lie; but the effect's the same */
        }
        ia->ia_origin = origin;         /* before its probes start */
        if(ifra->ifra_addr.sin_family == AF_INET &&
                (hostIsNew || maskIsNew))
            error = in_ifinit(ifp, ia, &ifra->ifra_addr, 0);
        if((ifp->if_flags & IFF_BROADCAST) &&
                (ifra->ifra_broadaddr.sin_family == AF_INET))
            ia->ia_broadaddr = ifra->ifra_broadaddr;
        if(error == 0)
            rt_newaddrmsg(RTM_NEWADDR, &ia->ia_ifa, 0, NULL);
        return (error);

    case SIOCDIFADDR:
        arp_dad_forget(ia);             /* a reload may give it back */
        rt_newaddrmsg(RTM_DELADDR, (struct ifaddr *)ia, 0, NULL);
        in_ifscrub(ifp, ia);
        rt_purgeifa((struct ifaddr *)ia);   /* gateway routes, ARP entries */
        if((ifa = ifp->if_addrlist) == (struct ifaddr *)ia)
            ifp->if_addrlist = ifa->ifa_next;
        else {
            while(ifa->ifa_next &&
                    (ifa->ifa_next != (struct ifaddr *)ia))
                ifa = ifa->ifa_next;
            if(ifa->ifa_next)
                ifa->ifa_next = ((struct ifaddr *)ia)->ifa_next;
            else
                log(LOG_ERR, "Couldn't unlink inifaddr from ifp\n");
        }
        oia = ia;
        if(oia == (ia = in_ifaddr))
            in_ifaddr = ia->ia_next;
        else {
            while(ia->ia_next && (ia->ia_next != oia))
                ia = ia->ia_next;
            if(ia->ia_next)
                ia->ia_next = oia->ia_next;
            else
                log(LOG_ERR, "Didn't unlink inifadr from list\n");
        }
        (void) m_free(dtom(oia));
        break;

    default:
        if(ifp == 0 || ifp->if_ioctl == 0)
            return (EOPNOTSUPP);
        return ((*ifp->if_ioctl)(ifp, cmd, data));
    }
    return (0);
}

/* in_control() keeps an in_ifaddr in an mbuf */
typedef char in_ifaddr_fits_an_mbuf[sizeof(struct in_ifaddr) <= MLEN ? 1 : -1];

/*
 * Delete any existing route for an interface.
 */
void
in_ifscrub(ifp, ia)
register struct ifnet *ifp;
register struct in_ifaddr *ia;
{

    if((ia->ia_flags & IFA_ROUTE) == 0)
        return;
    if(ifp->if_flags & (IFF_LOOPBACK | IFF_POINTOPOINT))
        rtinit(&(ia->ia_ifa), (int)RTM_DELETE, RTF_HOST);
    else
        rtinit(&(ia->ia_ifa), (int)RTM_DELETE, 0);
    ia->ia_flags &= ~IFA_ROUTE;
}

/*
 * Initialize an interface's internet address
 * and routing table entry.
 */
int
in_ifinit(ifp, ia, sin, scrub)
register struct ifnet *ifp;
register struct in_ifaddr *ia;
struct sockaddr_in *sin;
int scrub;
{
    register u_long i = ntohl(sin->sin_addr.s_addr);
    struct sockaddr_in oldaddr;
    int error, flags = RTF_UP, newaddr;
    spl_t s = splimp();

    oldaddr = ia->ia_addr;
    newaddr = oldaddr.sin_family != AF_INET || oldaddr.sin_addr.s_addr != sin->sin_addr.s_addr;
    ia->ia_addr = *sin;
    /*
     * Give the interface a chance to initialize
     * if this is its first address,
     * and to validate the address if necessary.
     */
    if(ifp->if_ioctl
            && (error = (*ifp->if_ioctl)(ifp, SIOCSIFADDR, (caddr_t)ia))) {
        splx(s);
        ia->ia_addr = oldaddr;
        return (error);
    }
    /*
     * What IPv4 keeps with a SANA-II interface: the copy of the address
     * that ARP requests are sent from, and the ARP table (still at splimp,
     * as alloc_arptable() wants).
     */
    {
        struct sana_softc *ssc = sana_softc_of(ifp);

        if(ssc != NULL) {
            ssc->ss_ipaddr = sin->sin_addr;
            if((ifp->if_flags & IFF_NOARP) == 0)
                alloc_arptable(ssc, 0);
        }
    }
    splx(s);
    if(scrub) {
        ia->ia_ifa.ifa_addr = (struct sockaddr *)&oldaddr;
        in_ifscrub(ifp, ia);
        ia->ia_ifa.ifa_addr = (struct sockaddr *)&ia->ia_addr;
    }
    if(IN_CLASSA(i))
        ia->ia_netmask = IN_CLASSA_NET;
    else if(IN_CLASSB(i))
        ia->ia_netmask = IN_CLASSB_NET;
    else
        ia->ia_netmask = IN_CLASSC_NET;
    /*
     * The subnet mask usually includes at least the standard network part,
     * but may may be smaller in the case of supernetting.
     * If it is set, we believe it.
     */
    if(ia->ia_subnetmask == 0) {
        ia->ia_subnetmask = ia->ia_netmask;
        ia->ia_sockmask.sin_addr.s_addr = htonl(ia->ia_subnetmask);
    } else
        ia->ia_netmask &= ia->ia_subnetmask;
    ia->ia_net = i & ia->ia_netmask;
    ia->ia_subnet = i & ia->ia_subnetmask;
    {
        register char *cp = (char *)(1 + & (ia->ia_sockmask.sin_addr));
        register char *cpbase = (char *) & (ia->ia_sockmask.sin_addr);
        while(--cp >= cpbase)
            if(*cp) {
                ia->ia_sockmask.sin_len =
                    1 + cp - (char *) & (ia->ia_sockmask);
                break;
            }
    }
    /*
     * The unspecified address is a placeholder - a DHCP client holding the
     * interface up while it waits for a lease. It names no network: a route
     * for it would send whole address ranges out of this interface, and a
     * second interface doing the same would be refused its address (EEXIST).
     */
    if(sin->sin_addr.s_addr == INADDR_ANY) {
        if(ifp->if_flags & IFF_BROADCAST)
            ia->ia_broadaddr.sin_addr.s_addr = INADDR_BROADCAST;
        return (0);
    }
    /*
     * Add route for the network.
     */
    if(ifp->if_flags & IFF_BROADCAST) {
        ia->ia_broadaddr.sin_addr =
            in_makeaddr(ia->ia_subnet, INADDR_BROADCAST);
        ia->ia_netbroadcast.s_addr =
            htonl(ia->ia_net | (INADDR_BROADCAST & ~ ia->ia_netmask));
    } else if(ifp->if_flags & IFF_LOOPBACK) {
        ia->ia_ifa.ifa_dstaddr = ia->ia_ifa.ifa_addr;
        flags |= RTF_HOST;
    } else if(ifp->if_flags & IFF_POINTOPOINT) {
        if(ia->ia_dstaddr.sin_family != AF_INET)
            return (0);
        flags |= RTF_HOST;
    }
    if((error = rtinit(&(ia->ia_ifa), (int)RTM_ADD, flags)) == 0)
        ia->ia_flags |= IFA_ROUTE;
#ifdef ENABLE_MULTICAST
    in_join_allhosts(ifp);
#endif
    /* a new address on an ARP interface is probed before it is used (RFC
     * 5227); a new mask on the same address is not a new address. EEXIST:
     * another interface has the network's route, the address is here all
     * the same (two links with 169.254/16, RFC 3927 section 3.2) */
    if((error == 0 || error == EEXIST) && newaddr)
        arp_dad_start(ia);
    return (error);
}

/*
 * An address that is not ready - tentative, or another host's (RFC 5227) -
 * is no source: another of its interface's that is ready, as NetBSD's
 * in_selectsrc() does. With none ready it stays, and ip_output() refuses
 * it.
 */
struct in_ifaddr *
in_readyia(struct in_ifaddr *ia)
{
    struct in_ifaddr *other;

    if(ia == NULL || !(ia->ia4_flags & IN_IFF_NOTREADY))
        return (ia);
    for(other = in_ifaddr; other; other = other->ia_next)
        if(other->ia_ifp == ia->ia_ifp && !(other->ia4_flags & IN_IFF_NOTREADY))
            return (other);
    return (ia);
}

/*
 * Return address info for specified internet network.
 */
struct in_ifaddr *
in_iaonnetof(net)
u_long net;
{
    register struct in_ifaddr *ia;

    for(ia = in_ifaddr; ia; ia = ia->ia_next)
        if(ia->ia_subnet == net)
            return (ia);
    return ((struct in_ifaddr *)0);
}

/*
 * Return 1 if the address might be a local broadcast address.
 */
int
in_broadcast(in)
struct in_addr in;
{
    register struct in_ifaddr *ia;

    /*
     * Look through the list of addresses for a match
     * with a broadcast address.
     */
    for(ia = in_ifaddr; ia; ia = ia->ia_next)
        /* a host address (/32), or one of a point-to-point pair (/31, RFC
         * 3021), has no network or broadcast address of its own: its
         * "subnet" is the address itself, or its peer */
        if((ia->ia_ifp->if_flags & IFF_BROADCAST) &&
                ia->ia_subnetmask < 0xfffffffeUL) {
            u_long t;
            if(ia->ia_broadaddr.sin_addr.s_addr == in.s_addr)
                return (1);
            /*
             * Check for old-style (host 0) broadcast.
             */
            if((t = ntohl(in.s_addr)) == ia->ia_subnet || t == ia->ia_net)
                return (1);
        }
    if(in.s_addr == INADDR_BROADCAST || in.s_addr == INADDR_ANY)
        return (1);
    return (0);
}

#ifdef ENABLE_MULTICAST
/*
 * in_mcast_macaddr - compute the Ethernet multicast MAC for an IPv4 group.
 * RFC 1112 §6.4: map a class-D address to 01:00:5e:XX:XX:XX using the low
 * 23 bits of the group address.
 */
void
in_mcast_macaddr(struct in_addr *maddr, u_int8_t *mac)
{
    u_long addr = ntohl(maddr->s_addr);

    mac[0] = 0x01;
    mac[1] = 0x00;
    mac[2] = 0x5e;
    mac[3] = (addr >> 16) & 0x7f;
    mac[4] = (addr >> 8) & 0xff;
    mac[5] = addr & 0xff;
}

/*
 * in_mcast_device - tell the interface's device to accept (add) or stop
 * accepting (!add) the link address of an IPv4 group. The device request
 * is the core's (sana_mcast(), shared with IPv6); only the mapping is ours.
 */
static int
in_mcast_device(struct ifnet *ifp, struct in_addr *maddr, int add)
{
    u_int8_t mac[6];

    in_mcast_macaddr(maddr, mac);
    return sana_mcast(ifp, mac, add);
}

/*
 * in_addmulti - add (or refcount) an IPv4 multicast membership on an
 * interface.  On the first reference the group's link address is registered
 * with the device (in_mcast_device()) and an unsolicited IGMP report is sent.
 * Mirrors in6_addmulti() in netinet6/in6.c.
 */
struct in_multi *in_multihead = NULL;

struct in_multi *
in_addmulti(struct in_addr *maddr, struct ifnet *ifp)
{
    struct in_multi *inm;

    /* search for an existing entry */
    for(inm = in_multihead; inm != NULL; inm = inm->inm_next) {
        if(inm->inm_ifp == ifp &&
           inm->inm_addr.s_addr == maddr->s_addr) {
            inm->inm_refcount++;
            return inm;
        }
    }

    /* register the group's link address with the device */
    if(in_mcast_device(ifp, maddr, 1) != 0) {
        __log(LOG_ERR, "in_addmulti: S2_ADDMULTICASTADDRESS failed for %s%d\n",
              ifp->if_name, ifp->if_unit);
        /* continue anyway -- some drivers accept all multicast */
    }

    /* allocate a new entry */
    MALLOC(inm, struct in_multi *, sizeof(*inm), M_IFADDR, M_WAITOK);
    if(inm == NULL)
        return NULL;
    bzero(inm, sizeof(*inm));
    inm->inm_addr     = *maddr;
    inm->inm_ifp      = ifp;
    inm->inm_refcount = 1;
    inm->inm_state    = IGMP_OTHERMEMBER;
    inm->inm_timer    = 0;
    inm->inm_next     = in_multihead;
    in_multihead      = inm;

    /* announce our membership (unsolicited IGMP report) */
    igmp_joingroup(inm);

    return inm;
}

/*
 * in_delmulti - drop a reference to an IPv4 multicast membership.  When the
 * last reference is released an IGMP Leave is sent (if we were the last
 * reporter), the Ethernet multicast is unregistered and the entry is freed.
 * Mirrors in6_delmulti() in netinet6/in6.c.
 */
void
in_delmulti(struct in_multi *inm)
{
    struct in_multi **p;

    if(--inm->inm_refcount > 0)
        return;

    /* a membership that outlived its interface has nobody to tell;
     * see in_ifdetach() */
    if(inm->inm_ifp != NULL) {
        /* announce that we are leaving before removing from the list */
        igmp_leavegroup(inm);

        /* unregister the group's link address from the device */
        in_mcast_device(inm->inm_ifp, &inm->inm_addr, 0);
    }

    for(p = &in_multihead; *p != NULL; p = &(*p)->inm_next) {
        if(*p == inm) {
            *p = inm->inm_next;
            break;
        }
    }
    FREE(inm, M_IFADDR);
}

/*
 * in_ifdetach - the interface is going away for good: nothing may keep a
 * membership that names it. Another ifnet can be allocated at the same
 * address later, and would then look as if it had joined already, so the
 * device would never be told the group's address.
 *
 * The interface's own membership of the all-hosts group, from
 * in_join_allhosts(), is released here. What sockets hold is released by
 * in_pcbifgone() (in_family.c); an entry still held is cut loose from the
 * interface and goes with its last holder. Nothing is sent and the device is not
 * asked to forget the addresses: it is offline and about to be closed.
 */
void
in_ifdetach(struct ifnet *ifp)
{
    struct in_multi *inm, **p;
    spl_t s = splimp();

    p = &in_multihead;
    while((inm = *p) != NULL) {
        if(inm->inm_ifp != ifp) {
            p = &inm->inm_next;
            continue;
        }
        inm->inm_timer = 0;
        inm->inm_state = IGMP_OTHERMEMBER;
        inm->inm_ifp = NULL;
        if(ntohl(inm->inm_addr.s_addr) == INADDR_ALLHOSTS_GROUP &&
           --inm->inm_refcount == 0) {
            *p = inm->inm_next;
            FREE(inm, M_IFADDR);
            continue;
        }
        p = &inm->inm_next;
    }
    splx(s);
}
#endif /* ENABLE_MULTICAST */
#endif


/*
 * IPv4's part in the life of an interface (net/netmodule.h). Registered
 * from ip_init().
 */
static int
in_ifevent(int event, struct ifnet *ifp)
{
    struct sana_softc *ssc = sana_softc_of(ifp);
    struct ifreq ifr;
    int deleted = 0;

    switch(event) {
    case NFE_UP:
        /* up with its device running: the ARP table, unless it has one */
        if(ssc != NULL && (ifp->if_flags & IFF_NOARP) == 0) {
            spl_t s = splimp();

            alloc_arptable(ssc, 0);
            splx(s);
        }
        break;

    case NFE_AUTOADDR:
        /* the configuration says AUTO: a link-local address (RFC 3927) */
        if(ssc != NULL) {
            ssc->ss_autoip.wanted = 1;
            autoip_start(ifp);
        }
        break;

    case NFE_LINKUP:
        /* New network -> new address. autoip was stopped when the link
         * went (NFE_LINKDOWN): the configuration's AUTO says it runs. */
        kill_dhclient(ifp);
        run_dhclient(ifp);
        if(ssc != NULL && ssc->ss_autoip.wanted)
            autoip_start(ifp);
        break;

    case NFE_LINKDOWN:
        kill_dhclient(ifp);
        if(ssc != NULL && ssc->ss_autoip.state != AUTOIP_DISABLED)
            autoip_stop(ifp);
        /* in_control() scrubs only each address's interface route: a gateway
         * route (the DHCP default) would keep rt_ifa on the freed address. */
        rt_purgeif(ifp, AF_INET);
        /* FALLTHROUGH: and remove the addresses */

    case NFE_UNCONFIGURE:
        /*
         * A request naming no address means "the first one on this
         * interface", so this deletes them all.
         */
        while(1) {
            bzero(&ifr, sizeof(ifr));
            if(in_control(NULL, SIOCDIFADDR, (caddr_t)&ifr, ifp) != 0)
                break;
            deleted++;
        }
        /* the end of what the configuration gave back (NFE_UNCONFIGURE_PRE) */
        arp_dad_reload(FALSE);
        break;

    case NFE_UNCONFIGURE_PRE:
        /*
         * The configuration is about to be taken off and, for a reload,
         * put back: an address in use that it gives back is remembered,
         * and is not probed again if it comes back (RFC 5227: nothing has
         * changed on the network). A link that went is another matter
         * (NFE_LINKDOWN).
         */
        arp_dad_reload(TRUE);
        if(ssc != NULL) {
            if(ssc->ss_autoip.state != AUTOIP_DISABLED)
                autoip_stop(ifp);
            ssc->ss_autoip.wanted = 0;      /* the configuration says again */
        }
        break;

    case NFE_REMOVING:
        kill_dhclient(ifp);
        break;

    case NFE_DETACHED:
#ifdef ENABLE_MULTICAST
        /* no group membership may go on naming this interface */
        in_ifdetach(ifp);
#endif
        break;
    }
    return (deleted);
}

/*
 * IPv4's address requests: its interface address ioctls, and those of its
 * address resolution (the ARP table).
 */
static int
in_ifcontrol(struct socket *so, int cmd, caddr_t data, struct ifnet *ifp)
{
    switch(cmd) {
    case SIOCSARP:
    case SIOCDARP:
    case SIOCGARP:
    case SIOCGARPT:
        return (arpioctl(cmd, data));
    }
    return (in_control(so, cmd, data, ifp));
}

/* What the core asks about an IPv4 address (NFQ_xxx, net/netmodule.h). */
static int
in_addrquery(int what, const struct sockaddr *sa, struct sockaddr *res)
{
    const struct sockaddr_in *sin = (const struct sockaddr_in *)sa;

    switch(what) {
    case NFQ_LOCAL:
        return (in_localaddr(sin->sin_addr) != 0);
    case NFQ_FORWARDABLE:
        return (in_canforward(sin->sin_addr) != 0);
    case NFQ_NETMASK:
        in_sockmaskof(sin->sin_addr, (struct sockaddr_in *)res);
        return (1);
    }
    return (0);
}

const struct netfamily_ifops in_ifops = {
    AF_INET, 10, in_ifevent, in_ifcontrol, in_addrquery
};

/*
 * IPV4 STARTS WHILE THE STACK RUNS (inet_start(), netinet/in_proto.c): the
 * interfaces are there already, and nobody will raise for them the events
 * a starting stack raises. For each interface that is up IPv4 does what it
 * does when one comes up (NFE_UP: the ARP table); and the loopback
 * interface gets IPv4's loopback address, 127.0.0.1, which the stack's
 * start-up gives it (loconfig(), net/if_loop.c) and which went with IPv4
 * when it stopped.
 *
 * Nothing else is made here. The addresses the configuration gives the
 * interfaces, their routes, a DHCP lease and a link-local claim come back
 * with the next reload of the configuration, which reads them again: the
 * stack keeps no copy of an interface's configuration that a family could
 * apply by itself.
 *
 * When the stack itself starts, no interface is up yet when the families
 * are started - the loopback is brought up by loconfig(), the others by
 * the configuration, both later - and this does nothing.
 */
void
in_attach_running(void)
{
    struct ifnet *ifp;

    for(ifp = ifnet_list; ifp != NULL; ifp = ifp->if_next) {
        if(!(ifp->if_flags & IFF_UP))
            continue;
        (void)in_ifevent(NFE_UP, ifp);
        if(ifp->if_flags & IFF_LOOPBACK) {
            struct ifreq ifr;
            struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;

            bzero((caddr_t)&ifr, sizeof(ifr));
            sin->sin_len = sizeof(struct sockaddr_in);
            sin->sin_family = AF_INET;
            sin->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            (void)in_control((struct socket *)NULL, SIOCSIFADDR, (caddr_t)&ifr, ifp);
        }
    }
}

/*
 * IPV4 STOPS (inet_stop()): what it has on the interfaces goes, on every
 * interface of the stack - the loopback and tunnels among them - as it
 * goes on one whose link went (NFE_LINKDOWN): the DHCP client is told, the
 * link-local claim is given up, every IPv4 route through the interface is
 * deleted (rt_purgeif(), which also frees what hangs on them) and then
 * every IPv4 address, each with the routes that still used it
 * (rt_purgeifa() in in_control()).
 *
 * Then the group memberships. Those of sockets were left when the sockets
 * were closed - none exists now; what remains is each interface's own
 * membership of the all-hosts group (in_join_allhosts()), for which no
 * report is ever sent, and any that a socket did not leave. The devices
 * stay open, so each is told to forget the group's link address; nothing
 * is sent - IPv4 has no address left to send from.
 *
 * Returns the number of addresses removed.
 */
int
in_detach_all(void)
{
    struct ifnet *ifp;
    int deleted = 0;
    spl_t s;

    for(ifp = ifnet_list; ifp != NULL; ifp = ifp->if_next)
        deleted += in_ifevent(NFE_LINKDOWN, ifp);

#ifdef ENABLE_MULTICAST
    s = splimp();
    while(in_multihead != NULL) {
        struct in_multi *inm = in_multihead;

        in_multihead = inm->inm_next;
        if(inm->inm_ifp != NULL)
            in_mcast_device(inm->inm_ifp, &inm->inm_addr, 0);
        FREE(inm, M_IFADDR);
    }
    splx(s);
#endif
    in_interfaces = 0;
    subnetsarelocal = SUBNETSARELOCAL;
    return (deleted);
}
