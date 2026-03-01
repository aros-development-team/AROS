/*
 * Copyright © 1983, 1989 The Regents of the University of California.
 * All rights reserved.
 * Copyright © 2005 Pavel Fedin
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
 */

/*
 * Modernized for AROS with PF_ROUTE socket and IPv6 support.
 * Based on FreeBSD route(8).
 */

/****** netutil.doc/route ***************************************************
*
*   NAME
*        route - manually manipulate the routing tables
*   
*   SYNOPSIS
*        route [-n] [-q] [-v] [-C] command [modifiers] destination gateway
*   
*   DESCRIPTION
*        Route is a program used to manually manipulate the network routing
*        tables. 
*   
*        Options supported by route:
*   
*        -n      Prevent attempts to print host and network names
*                symbolically when reporting actions.
*   
*        -v      (verbose) Print additional details.
*   
*        -q      Suppress all output.
*   
*        -C      Use old-style ioctls instead of PF_ROUTE socket.
*   
*        Commands accepted by route:
*   
*        add         Add a route.
*        delete      Delete a specific route.
*        change      Change aspects of a route (metrics/gateway).
*        get         Lookup and display the route for a destination.
*        show        Display the routing table.
*        flush       Remove all gateway routes.
*        monitor     Continuously report routing changes.
*   
*        The destination is the destination host or network, gateway is the
*        next-hop gateway to which packets should be addressed. Routes to a
*        particular host are distinguished from those to a network by
*        interpreting the Internet address associated with destination. The
*        optional modifiers -net and -host force the destination to be
*        interpreted as a network or a host, respectively.  Otherwise, if the
*        destination has a ``local address part'' of INADDR_ANY, or if the
*        destination is the symbolic name of a network, then the route is
*        assumed to be to a network; otherwise, it is presumed to be a route
*        to a host.
*   
*        For example, 128.32 is interpreted as -host 128.0.0.32; 128.32.130
*        is interpreted as -host 128.32.0.130; -net 128.32 is interpreted as
*        128.32.0.0; and -net 128.32.130 is interpreted as 128.32.130.0.
*   
*        To add a default route, give the destination as 'default'.
*    
*        If the route is via an interface rather than via a gateway, the
*        -interface modifier should be specified; the gateway given is the
*        address of this host on the common network, indicating the interface
*        to be used for transmission.
*   
*        The optional -netmask qualifier is used to specify the netmask of
*        the interface. One specifies an additional ensuing address parameter
*        (to be interpreted as a network mask).  The implicit network mask
*        generated can be overridden by making sure this option follows the
*        destination parameter.
*   
*        All symbolic names specified for a destination or gateway are looked
*        up first as a host name using gethostbyname(). If this lookup fails,
*        getnetbyname() is then used to interpret the name as that of a
*        network.
*   
*   DIAGNOSTICS
*        add [host | network ] %s: gateway %s flags %x
*                The specified route is being added to the tables. The values
*                printed are from the routing table entry supplied in the
*                IoctlSocket() call. If the gateway address used was not the
*                primary address of the gateway (the first one returned by
*                gethostbyname()), the gateway address is printed numerically
*                as well as symbolically.
*   
*        delete [ host | network ] %s: gateway %s flags %x
*                As above, but when deleting an entry.
*   
*        Network is unreachable
*                An attempt to add a route failed because the gateway listed
*                was not on a directly-connected network.  The next-hop
*                gateway must be given.
*   
*        not in table
*                A delete operation was attempted for an entry which wasn't
*                present in the tables.
*   
*        routing table overflow
*                An add operation was attempted, but the system was low on
*                resources and was unable to allocate memory to create the
*                new entry.
*   
*   SEE ALSO
*        ifconfig, protocols/routing
*   
*   HISTORY
*        The route command appeared in 4.2BSD.
*   
*****************************************************************************
*
*/
#define D(x)

#include <sys/errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h> /* NC */
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <net/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <dos/dos.h>

#define __CONFIG_ROADSHOW__
#include <proto/socket.h>
#include <proto/miami.h>
#include <proto/exec.h>
#include <libraries/bsdsocket.h>

#define herror(x) perror(x)
#define ioctl IoctlSocket

#ifndef IN6_IS_ADDR_UNSPECIFIED
#define IN6_IS_ADDR_UNSPECIFIED(a) \
    ((a)->un.u32_addr[0] == 0 && (a)->un.u32_addr[1] == 0 && \
     (a)->un.u32_addr[2] == 0 && (a)->un.u32_addr[3] == 0)
#endif
/*#include <clib/netlib_protos.h>*/

#ifndef AF_NS
#define AF_NS AF_NETBIOS
#endif

struct keytab {
	char	*kt_cp;
	int	kt_i;
} keywords[] = {
#include "keywords.h"
{0, 0}
};

struct	ortentry route;
union	sockunion {
	struct	sockaddr sa;
	struct	sockaddr_in s_in;
	struct	sockaddr_in6 s_in6;
#if 0
	struct	sockaddr_ns sns;
	struct	sockaddr_iso siso;
	struct	sockaddr_dl sdl;
	struct	sockaddr_x25 sx25;
#endif
} so_dst, so_gate, so_mask, so_genmask, so_ifa, so_ifp;

union sockunion *so_addrs[] =
	{ &so_dst, &so_gate, &so_mask, &so_genmask, &so_ifp, &so_ifa, 0}; 

typedef union sockunion *sup;
int	rtm_addrs;
int	s;
int	pid;
int	forcehost, forcenet, doflush, nflag, af, qflag, tflag, Cflag, 
  keyword(char *cp);
int	iflag, verbose, aflen = sizeof (struct sockaddr_in);
int	locking, lockrest, debugonly;
struct	sockaddr_in s_in = { sizeof(s_in), AF_INET };
struct	rt_metrics rt_metrics;
u_long  rtm_inits;

char	*routename(struct sockaddr *sa), *netname(struct sockaddr *sa);
void	flushroutes(int argc, char *argv[]), 
  newroute(int argc, char **argv), monitor(void);
void	show_routes(void);
void	print_getmsg(struct rt_msghdr *rtm, int msglen);
void	print_rtmsg(struct rt_msghdr *rtm, int msglen);
void	pmsg_common(struct rt_msghdr *rtm);
void	sodump(sup su, char *which);
void	bprintf(FILE *fp, int b, u_char *s);
void	mask_addr(void);
int	getaddr(int which, char *s, struct hostent **hpp);
int	rtmsg(int cmd, int flags);
int GetOpt(int argc, char **argv, char *opts);
VOID CleanUpExit(LONG error);

const TEXT version[] = "route 4.0 (01.03.2026) - IPv6/PF_ROUTE support";

void
usage(char *cp)
{
	if (cp)
		(void) fprintf(stderr, "route: botched keyword: %s\n", cp);
	(void) fprintf(stderr,
	    "usage: route [ -Cnqv ] cmd [[ -<qualifiers> ] args ]\n"
	    "cmds: add, delete, change, get, show, flush, monitor\n");
	CleanUpExit(1);
	/* NOTREACHED */
}

void
quit(char *s)
{
	int sverrno = errno;

	(void) fprintf(stderr, "route: ");
	if (s)
		(void) fprintf(stderr, "%s: ", s);
	(void) fprintf(stderr, "%s\n", strerror(sverrno));
	CleanUpExit(1);
	/* NOTREACHED */
}

#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

int main(int argc, char **argv)
{
	extern int optind;
	int ch;

	SetErrnoPtr(&errno, sizeof(errno));

	if (argc < 2)
		usage(NULL);

	while ((ch = GetOpt(argc, argv, "Cnqtv")) != EOF)
		switch(ch) {
		case 'C':
			Cflag = 1;	/* Use old ioctls. */
			break;
		case 'n':
			nflag = 1;
			break;
		case 'q':
			qflag = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 't':
			tflag = 1;
			break;
		case '?':
		default:
			usage(NULL);
		}
	argc -= optind;
	argv += optind;

	pid = (int)(long)FindTask(NULL);

	if (Cflag)
		s = socket(AF_INET, SOCK_RAW, 0);
	else
		s = socket(PF_ROUTE, SOCK_RAW, 0);

	/* 'show' doesn't need a socket — handle it before the socket check */
	if (*argv && keyword(*argv) == K_SHOW) {
		show_routes();
		CleanUpExit(0);
	}

	if (s < 0)
		quit("socket");
	if (*argv)
		switch (keyword(*argv)) {
		case K_GET:
			/* FALLTHROUGH */

		case K_CHANGE:
			if (Cflag)
				usage("change or get with -C");
			/* FALLTHROUGH */

		case K_ADD:
		case K_DELETE:
			newroute(argc, argv);
			CleanUpExit(0);
			/* NOTREACHED */

		case K_MONITOR:
			monitor();
			/* NOTREACHED */

		case K_FLUSH:
			flushroutes(argc, argv);
			CleanUpExit(0);
			/* NOTREACHED */
		}
	usage(*argv);
	/* NOTREACHED */

	return 0;
}

/*
 * Purge all entries in the routing tables not
 * associated with network interfaces.
 */
void
flushroutes(int argc, char *argv[])
{
	int seqno;
	struct rt_msghdr *rtm;
	char *next;

	if (Cflag) {
		(void) fprintf(stderr, "route: flush requires PF_ROUTE (omit -C)\n");
		CleanUpExit(1);
	}
	shutdown(s, 0); /* Don't want to read back our messages */
	if (argc > 1) {
		argv++;
		if (argc == 2 && **argv == '-')
		    switch (keyword(*argv + 1)) {
			case K_INET:
				af = AF_INET;
				break;
			case K_INET6:
				af = AF_INET6;
				break;
			default:
				goto bad;
		} else
bad:			usage(*argv);
	}

	/* Use GetRouteInfo() to enumerate all routes */
	{
		LONG af_filter = af ? af : AF_INET;
		struct rt_msghdr *buf;

		buf = GetRouteInfo(af_filter, 0);
		if (buf == NULL) {
			quit("GetRouteInfo");
			return;
		}
		seqno = 0;
		for (next = (char *)buf; ; next += rtm->rtm_msglen) {
			rtm = (struct rt_msghdr *)next;
			if (rtm->rtm_msglen == 0)
				break;  /* zero-length sentinel */
			if ((rtm->rtm_flags & RTF_GATEWAY) == 0)
				continue;
			if (af) {
				struct sockaddr *sa = (struct sockaddr *)(rtm + 1);
				if (sa->sa_family != af)
					continue;
			}
			rtm->rtm_type = RTM_DELETE;
			rtm->rtm_seq = seqno;
			{
				int rlen = send(s, (char *)rtm, rtm->rtm_msglen, 0);
				if (rlen < (int)rtm->rtm_msglen) {
					(void) fprintf(stderr,
					    "route: send to routing socket: %s\n",
					    strerror(errno));
					(void) printf("got only %d for rlen\n", rlen);
					break;
				}
			}
			seqno++;
			if (qflag)
				continue;
			if (verbose)
				print_rtmsg(rtm, rtm->rtm_msglen);
			else {
				struct sockaddr *sa = (struct sockaddr *)(rtm + 1);
				(void) printf("%-20.20s ", rtm->rtm_flags & RTF_HOST ?
				    routename(sa) : netname(sa));
				sa = (struct sockaddr *)(sa->sa_len + (char *)sa);
				(void) printf("%-20.20s ", routename(sa));
				(void) printf("done\n");
			}
		}
		FreeRouteInfo(buf);
	}
}
	
char *
routename(struct sockaddr *sa)
{
	register char *cp;
	static char line[INET6_ADDRSTRLEN + 10];
	struct hostent *hp;
	static char domain[MAXHOSTNAMELEN + 1];
	static int first = 1;

	if (first) {
		first = 0;
		if (gethostname(domain, MAXHOSTNAMELEN) == 0 &&
		    (cp = index(domain, '.')))
			(void) strcpy(domain, cp + 1);
		else
			domain[0] = 0;
	}
	switch (sa->sa_family) {

	case AF_INET:
	    {	struct in_addr in;
		in = ((struct sockaddr_in *)sa)->sin_addr;

		cp = 0;
		if (in.s_addr == INADDR_ANY)
			cp = "default";
		if (cp == 0 && !nflag) {
			hp = gethostbyaddr((char *)&in, sizeof (struct in_addr),
				AF_INET);
			if (hp) {
				if ((cp = index(hp->h_name, '.')) &&
				    !strcmp(cp + 1, domain))
					*cp = 0;
				cp = hp->h_name;
			}
		}
		if (cp)
			strcpy(line, cp);
		else {
			inet_ntop(AF_INET, &in, line, sizeof(line));
		}
		break;
	    }

	case AF_INET6:
	    {	struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
		cp = 0;
		if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr))
			cp = "default";
		if (cp == 0 && !nflag) {
			hp = gethostbyaddr((char *)&sin6->sin6_addr,
				sizeof(struct in6_addr), AF_INET6);
			if (hp) {
				if ((cp = index(hp->h_name, '.')) &&
				    !strcmp(cp + 1, domain))
					*cp = 0;
				cp = hp->h_name;
			}
		}
		if (cp)
			strcpy(line, cp);
		else
			inet_ntop(AF_INET6, &sin6->sin6_addr, line, sizeof(line));
		break;
	    }

#if 0
	case AF_NS:
		return (ns_print((struct sockaddr_ns *)sa));

	case AF_LINK:
		return (link_ntoa((struct sockaddr_dl *)sa));

	case AF_ISO:
		(void) sprintf(line, "iso %s",
		    iso_ntoa(&((struct sockaddr_iso *)sa)->siso_addr));
		break;
#endif
	default:
	    {	u_short *sp = (u_short *)sa->sa_data;
		u_short *slim = sp + ((sa->sa_len + 1) >> 1);
		char *lp = line + sprintf(line, "(%d)", sa->sa_family);

		while (sp < slim)
			lp += sprintf(lp, " %x", *sp++);
		break;
	    }
	}
	return (line);
}

/*
 * Return the name of the network whose address is given.
 * The address is assumed to be that of a net or subnet, not a host.
 */
char *
netname(struct sockaddr *sa)
{
	char *cp = 0;
	static char line[INET6_ADDRSTRLEN + 10];
	struct netent *np = 0;
	u_long net, mask;
	register u_long i;
	int subnetshift;

	switch (sa->sa_family) {

	case AF_INET:
	    {	struct in_addr in;
		in = ((struct sockaddr_in *)sa)->sin_addr;

		i = in.s_addr = ntohl(in.s_addr);
		if (in.s_addr == 0)
			cp = "default";
		else if (!nflag) {
			if (IN_CLASSA(i)) {
				mask = IN_CLASSA_NET;
				subnetshift = 8;
			} else if (IN_CLASSB(i)) {
				mask = IN_CLASSB_NET;
				subnetshift = 8;
			} else {
				mask = IN_CLASSC_NET;
				subnetshift = 4;
			}
			/*
			 * If there are more bits than the standard mask
			 * would suggest, subnets must be in use.
			 * Guess at the subnet mask, assuming reasonable
			 * width subnet fields.
			 */
			while (in.s_addr &~ mask)
				mask = (long)mask >> subnetshift;
			net = in.s_addr & mask;
			while ((mask & 1) == 0)
				mask >>= 1, net >>= 1;
			np = getnetbyaddr(net, AF_INET);
			if (np)
				cp = np->n_name;
		}
		if (cp)
			strcpy(line, cp);
		else {
			struct in_addr tmp;
			tmp.s_addr = htonl(i);
			inet_ntop(AF_INET, &tmp, line, sizeof(line));
		}
		break;
	    }

	case AF_INET6:
	    {	struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
		cp = 0;
		if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr))
			cp = "default";
		if (cp)
			strcpy(line, cp);
		else
			inet_ntop(AF_INET6, &sin6->sin6_addr, line, sizeof(line));
		break;
	    }

#if 0
	case AF_NS:
		return (ns_print((struct sockaddr_ns *)sa));
		break;

	case AF_LINK:
		return (link_ntoa((struct sockaddr_dl *)sa));

	case AF_ISO:
		(void) sprintf(line, "iso %s",
		    iso_ntoa(&((struct sockaddr_iso *)sa)->siso_addr));
		break;
#endif
	default:
	    {	u_short *sp = (u_short *)sa->sa_data;
		u_short *slim = sp + ((sa->sa_len + 1)>>1);
		char *lp = line + sprintf(line, "af %d:", sa->sa_family);

		while (sp < slim)
			lp += sprintf(lp, " %x", *sp++);
		break;
	    }
	}
	return (line);
}

void
set_metric(char *value, int key)
{
	int flag = 0; 
	u_long noval, *valp = &noval;

	switch (key) {
#define caseof(x, y, z)	case x: valp = &rt_metrics.z; flag = y; break
	caseof(K_MTU, RTV_MTU, rmx_mtu);
	caseof(K_HOPCOUNT, RTV_HOPCOUNT, rmx_hopcount);
	caseof(K_EXPIRE, RTV_EXPIRE, rmx_expire);
	caseof(K_RECVPIPE, RTV_RPIPE, rmx_recvpipe);
	caseof(K_SENDPIPE, RTV_SPIPE, rmx_sendpipe);
	caseof(K_SSTHRESH, RTV_SSTHRESH, rmx_ssthresh);
	caseof(K_RTT, RTV_RTT, rmx_rtt);
	caseof(K_RTTVAR, RTV_RTTVAR, rmx_rttvar);
	}
	rtm_inits |= flag;
	if (lockrest || locking)
		rt_metrics.rmx_locks |= flag;
	if (locking)
		locking = 0;
	*valp = atoi(value);
}

void
newroute(int argc, char **argv)
{
	char *cmd, *dest = "", *gateway = "", *err;
	int ishost = 0, ret, attempts, oerrno, flags = 0;
	int key;
	struct hostent *hp = 0;
	cmd = argv[0];
	if (*cmd != 'g')
		shutdown(s, 0); /* Don't want to read back our messages */
	while (--argc > 0) {
		if (**(++argv)== '-') {
			switch (key = keyword(1 + *argv)) {
#if 0
			case K_LINK:
				af = AF_LINK;
				aflen = sizeof(struct sockaddr_dl);
				break;
			case K_OSI:
			case K_ISO:
				af = AF_ISO;
				aflen = sizeof(struct sockaddr_iso);
				break;
#endif
			case K_INET:
				af = AF_INET;
				aflen = sizeof(struct sockaddr_in);
				break;
			case K_INET6:
				af = AF_INET6;
				aflen = sizeof(struct sockaddr_in6);
				break;
#if 0
			case K_X25:
				af = AF_CCITT;
				aflen = sizeof(struct sockaddr_x25);
				break;
			case K_SA:
				af = 0;
				aflen = sizeof(union sockunion);
				break;
			case K_XNS:
				af = AF_NS;
				aflen = sizeof(struct sockaddr_ns);
				break;
#endif
			case K_IFACE:
			case K_INTERFACE:
				iflag++;
				break;
			case K_LOCK:
				locking = 1;
				break;
			case K_LOCKREST:
				lockrest = 1;
				break;
			case K_HOST:
				forcehost++;
				break;
			case K_REJECT:
				flags |= RTF_REJECT;
				break;
			case K_PROTO1:
				flags |= RTF_PROTO1;
				break;
			case K_PROTO2:
				flags |= RTF_PROTO2;
				break;
			case K_CLONING:
				flags |= RTF_CLONING;
				break;
			case K_XRESOLVE:
				flags |= RTF_XRESOLVE;
				break;
			case K_IFA:
				argc--;
				(void) getaddr(RTA_IFA, *++argv, 0);
				break;
			case K_IFP:
				argc--;
				(void) getaddr(RTA_IFP, *++argv, 0);
				break;
			case K_GENMASK:
				argc--;
				(void) getaddr(RTA_GENMASK, *++argv, 0);
				break;
			case K_GATEWAY:
				argc--;
				(void) getaddr(RTA_GATEWAY, *++argv, 0);
				break;
			case K_DST:
				argc--;
				ishost = getaddr(RTA_DST, *++argv, &hp);
				dest = *argv;
				break;
			case K_NETMASK:
				argc--;
				(void) getaddr(RTA_NETMASK, *++argv, 0);
				/* FALLTHROUGH */
			case K_NET:
				forcenet++;
				break;
			case K_MTU:
			case K_HOPCOUNT:
			case K_EXPIRE:
			case K_RECVPIPE:
			case K_SENDPIPE:
			case K_SSTHRESH:
			case K_RTT:
			case K_RTTVAR:
				argc--;
				set_metric(*++argv, key);
				break;
			default:
				usage(1+*argv);
			}
		} else {
			if ((rtm_addrs & RTA_DST) == 0) {
				dest = *argv;
				ishost = getaddr(RTA_DST, *argv, &hp);
			} else if ((rtm_addrs & RTA_GATEWAY) == 0) {
				gateway = *argv;
				(void) getaddr(RTA_GATEWAY, *argv, &hp);
			} else {
				int ret = atoi(*argv);
				if (ret == 0) {
				    printf("%s,%s", "old usage of trailing 0",
					   "assuming route to if\n");
				    iflag = 1;
				    continue;
				} else if (ret > 0 && ret < 10) {
				    printf("old usage of trailing digit, ");
				    printf("assuming route via gateway\n");
				    iflag = 0;
				    continue;
				}
				(void) getaddr(RTA_NETMASK, *argv, 0);
			}
		}
	}
	if (forcehost)
		ishost = 1;
	if (forcenet)
		ishost = 0;
	flags |= RTF_UP;
	if (ishost)
		flags |= RTF_HOST;
	if (iflag == 0)
		flags |= RTF_GATEWAY;
	for (attempts = 1; ; attempts++) {
		errno = 0;
		if (Cflag && (af == AF_INET)) {
			D(printf ("Flags: 0x%08lx\n", flags);)
			D(printf ("Dst = 0x%08lx\n", so_dst.s_in.sin_addr.s_addr);)
			D(printf ("Gate = 0x%08lx\n", so_gate.s_in.sin_addr.s_addr);)
			route.rt_flags = flags;
			route.rt_dst = so_dst.sa;
			route.rt_gateway = so_gate.sa;
			if ((ret = ioctl(s, *cmd == 'a' ? SIOCADDRT : SIOCDELRT,
			     (caddr_t)&route)) == 0)
				break;
		} else {
		    if ((ret = rtmsg(*cmd, flags)) == 0)
				break;
		}
		if (errno != ENETUNREACH && errno != ESRCH)
			break;
		if (af == AF_INET && hp && hp->h_addr_list[1]) {
			hp->h_addr_list++;
			bcopy(hp->h_addr_list[0], (caddr_t)&so_dst.s_in.sin_addr,
			    hp->h_length);
		} else
			break;
	}
	if (*cmd == 'g')
		CleanUpExit(0);
	oerrno = errno;
	(void) printf("%s %s %s: gateway %s", cmd, ishost? "host" : "net",
		dest, gateway);
	if (attempts > 1 && ret == 0 && af == AF_INET)
        {
            struct sockaddr_in *route_sin = (struct sockaddr_in *)&route.rt_gateway;
	    (void) printf(" (%s)",
		Inet_NtoA(route_sin->sin_addr.s_addr));
        }
	if (ret == 0)
		(void) printf("\n");
	else {
		switch (oerrno) {
		case ESRCH:
			err = "not in table";
			break;
		case EBUSY:
			err = "entry in use";
			break;
		case ENOBUFS:
			err = "routing table overflow";
			break;
		default:
			err = strerror(oerrno);
			break;
		}
		(void) printf(": %s\n", err);
	}
}

void
inet_makenetandmask(u_long net, register struct sockaddr_in *s_in)
{
	u_long addr, mask = 0;
	register char *cp;

	rtm_addrs |= RTA_NETMASK;
	if (net == 0)
		mask = addr = 0;
	else if (net < 128) {
		addr = net << IN_CLASSA_NSHIFT;
		mask = IN_CLASSA_NET;
	} else if (net < 65536) {
		addr = net << IN_CLASSB_NSHIFT;
		mask = IN_CLASSB_NET;
	} else if (net < 16777216L) {
		addr = net << IN_CLASSC_NSHIFT;
		mask = IN_CLASSC_NET;
	} else {
		addr = net;
		if ((addr & IN_CLASSA_HOST) == 0)
			mask =  IN_CLASSA_NET;
		else if ((addr & IN_CLASSB_HOST) == 0)
			mask =  IN_CLASSB_NET;
		else if ((addr & IN_CLASSC_HOST) == 0)
			mask =  IN_CLASSC_NET;
		else
			mask = -1;
	}
	s_in->sin_addr.s_addr = htonl(addr);
	s_in = &so_mask.s_in;
	s_in->sin_addr.s_addr = htonl(mask);
	s_in->sin_len = 0;
	s_in->sin_family = 0;
	cp = (char *)(&s_in->sin_addr + 1);
	while (*--cp == 0 && cp > (char *)s_in)
		;
	s_in->sin_len = 1 + cp - (char *)s_in;
}

/*
 * Interpret an argument as a network address of some kind,
 * returning 1 if a host address, 0 if a network address.
 */
int
getaddr(int which, char *s, struct hostent **hpp)
{
	register sup su;
	struct hostent *hp;
	struct netent *np;
	u_long val;

	if (af == 0) {
		af = AF_INET;
		aflen = sizeof(struct sockaddr_in);
	}
	rtm_addrs |= which;
	switch (which) {
	case RTA_DST:		su = so_addrs[0]; su->sa.sa_family = af; break;
	case RTA_GATEWAY:	su = so_addrs[1]; su->sa.sa_family = af; break;
	case RTA_NETMASK:	su = so_addrs[2]; break;
	case RTA_GENMASK:	su = so_addrs[3]; break;
	case RTA_IFP:		su = so_addrs[4]; su->sa.sa_family = af; break;
	case RTA_IFA:		su = so_addrs[5]; su->sa.sa_family = af; break;
	default:		usage("Internal Error"); /*NOTREACHED*/
	}
	su->sa.sa_len = aflen;
	if (strcmp(s, "default") == 0) {
		switch (which) {
		case RTA_DST:
			forcenet++;
			(void) getaddr(RTA_NETMASK, s, 0);
			break;
		case RTA_NETMASK:
		case RTA_GENMASK:
			su->sa.sa_len = 0;
		}
		return 0;
	}

	/* Handle AF_INET6 addresses */
	if (af == AF_INET6) {
		struct sockaddr_in6 *sin6 = &su->s_in6;
		if (inet_pton(AF_INET6, s, &sin6->sin6_addr) == 1) {
			sin6->sin6_family = AF_INET6;
			sin6->sin6_len = sizeof(struct sockaddr_in6);
			return (1);  /* host address */
		}
		if (hpp == NULL)
			hpp = &hp;
		*hpp = NULL;
		hp = gethostbyname(s);
		if (hp && hp->h_addrtype == AF_INET6) {
			*hpp = hp;
			sin6->sin6_family = AF_INET6;
			sin6->sin6_len = sizeof(struct sockaddr_in6);
			memcpy(&sin6->sin6_addr, hp->h_addr, hp->h_length);
			return (1);
		}
		(void) fprintf(stderr, "%s: bad IPv6 address\n", s);
		CleanUpExit(1);
	}

	if (hpp == NULL)
		hpp = &hp;
	*hpp = NULL;
	if (((val = inet_addr(s)) != (u_long)-1) &&
	    (which != RTA_DST || forcenet == 0)) {
		su->s_in.sin_addr.s_addr = val;
		if (Inet_LnaOf(su->s_in.sin_addr.s_addr) != INADDR_ANY)
			return (1);
		else {
			val = ntohl(val);
		out:	if (which == RTA_DST)
				inet_makenetandmask(val, &su->s_in);
			return (0);
		}
	}
	val = inet_network(s);
	if (val != (u_long)-1) {
		goto out;
	}
	np = getnetbyname(s);
	if (np) {
		val = np->n_net;
		goto out;
	}
	hp = gethostbyname(s);
	if (hp) {
		*hpp = hp;
		su->s_in.sin_family = hp->h_addrtype;
		memcpy(&su->s_in.sin_addr, hp->h_addr, hp->h_length);
		return (1);
	}
	(void) fprintf(stderr, "%s: bad value\n", s);
	CleanUpExit(1);
#if 0
do_xns:
	if (which == RTA_DST) {
		extern short ns_bh[3];
		struct sockaddr_ns *sms = &(so_mask.sns);
		bzero((char *)sms, sizeof(*sms));
		sms->sns_family = 0;
		sms->sns_len = 6;
		sms->sns_addr.x_net = *(union ns_net *)ns_bh;
		rtm_addrs |= RTA_NETMASK;
	}
	su->sns.sns_addr = ns_addr(s);
	return (!ns_nullhost(su->sns.sns_addr));
do_osi:
	su->siso.siso_addr = *iso_addr(s);
	if (which == RTA_NETMASK || which == RTA_GENMASK) {
		register char *cp = (char *)TSEL(&su->siso);
		su->siso.siso_nlen = 0;
		do {--cp ;} while ((cp > (char *)su) && (*cp == 0));
		su->siso.siso_len = 1 + cp - (char *)su;
	}
	return (1);
do_ccitt:
	ccitt_addr(s, &su->sx25);
	return (1);
do_link:
	link_addr(s, &su->sdl);
	return (1);
do_sa:
	su->sa.sa_len = sizeof(*su);
	sockaddr(s, &su->sa);
	return (1);
#endif

	return 0;
}

#if 0
short ns_nullh[] = {0,0,0};
short ns_bh[] = {-1,-1,-1};

char *
ns_print(sns)
	struct sockaddr_ns *sns;
{
	struct ns_addr work;
	union { union ns_net net_e; u_long long_e; } net;
	u_short port;
	static char mybuf[50], cport[10], chost[25];
	char *host = "";
	register char *p;
	register u_char *q;

	work = sns->sns_addr;
	port = ntohs(work.x_port);
	work.x_port = 0;
	net.net_e  = work.x_net;
	if (ns_nullhost(work) && net.long_e == 0) {
		if (!port)
			return ("*.*");
		(void) sprintf(mybuf, "*.%XH", port);
		return (mybuf);
	}

	if (bcmp((char *)ns_bh, (char *)work.x_host.c_host, 6) == 0) 
		host = "any";
	else if (bcmp((char *)ns_nullh, (char *)work.x_host.c_host, 6) == 0)
		host = "*";
	else {
		q = work.x_host.c_host;
		(void) sprintf(chost, "%02X%02X%02X%02X%02X%02XH",
			q[0], q[1], q[2], q[3], q[4], q[5]);
		for (p = chost; *p == '0' && p < chost + 12; p++)
			/* void */;
		host = p;
	}
	if (port)
		(void) sprintf(cport, ".%XH", htons(port));
	else
		*cport = 0;

	(void) sprintf(mybuf,"%XH.%s%s", ntohl(net.long_e), host, cport);
	return (mybuf);
}
#endif

void
monitor(void)
{
	int n;
	char msg[2048];

	if (Cflag) {
		(void) fprintf(stderr, "route: monitor requires PF_ROUTE (omit -C)\n");
		CleanUpExit(1);
	}
	verbose = 1;
	for(;;) {
		n = recv(s, msg, sizeof(msg), 0);
		if (n <= 0)
			break;
		(void) printf("got message of size %d\n", n);
		print_rtmsg((struct rt_msghdr *)msg, n);
	}
}

struct {
	struct	rt_msghdr m_rtm;
	char	m_space[512];
} m_rtmsg;

int
rtmsg(int cmd, int flags)
{
	static int seq;
	int rlen;
	register char *cp = m_rtmsg.m_space;
	register int l;

#define NEXTADDR(w, u) \
	if (rtm_addrs & (w)) {\
	    l = ROUNDUP(u.sa.sa_len); memcpy(cp, (char *)&(u), l); cp += l;\
	    if (verbose) sodump(&(u),"u");\
	}

	errno = 0;
	memset((char *)&m_rtmsg, 0, sizeof(m_rtmsg));
	if (cmd == 'a')
		cmd = RTM_ADD;
	else if (cmd == 'c')
		cmd = RTM_CHANGE;
	else if (cmd == 'g')
		cmd = RTM_GET;
	else
		cmd = RTM_DELETE;
#define rtm m_rtmsg.m_rtm
	rtm.rtm_type = cmd;
	rtm.rtm_flags = flags;
	rtm.rtm_version = RTM_VERSION;
	rtm.rtm_seq = ++seq;
	rtm.rtm_addrs = rtm_addrs;
	rtm.rtm_rmx = rt_metrics;
	rtm.rtm_inits = rtm_inits;

	if (rtm_addrs & RTA_NETMASK)
		mask_addr();
	NEXTADDR(RTA_DST, so_dst);
	NEXTADDR(RTA_GATEWAY, so_gate);
	NEXTADDR(RTA_NETMASK, so_mask);
	NEXTADDR(RTA_GENMASK, so_genmask);
	NEXTADDR(RTA_IFP, so_ifp);
	NEXTADDR(RTA_IFA, so_ifa);
	rtm.rtm_msglen = l = cp - (char *)&m_rtmsg;
	if (verbose)
		print_rtmsg(&rtm, l);
	if (debugonly)
		return 0;
	if ((rlen = send(s, (char *)&m_rtmsg, l, 0)) < 0) {
		perror("sending to routing socket");
		return (-1);
	}
	if (cmd == RTM_GET) {
		do {
			l = recv(s, (char *)&m_rtmsg, sizeof(m_rtmsg), 0);
		} while (l > 0 && (rtm.rtm_seq != seq || rtm.rtm_pid != pid));
		if (l < 0)
			(void) fprintf(stderr,
			    "route: recv from routing socket: %s\n",
			    strerror(errno));
		else
			print_getmsg(&rtm, l);
	}
#undef rtm
	return (0);
}

void
mask_addr(void)
{
	register char *cp1, *cp2;

	if ((rtm_addrs & RTA_DST) == 0)
		return;
	switch(so_dst.sa.sa_family) {
	case AF_INET: case AF_INET6: case 0:
		return;
	}
	cp1 = so_mask.sa.sa_len + 1 + (char *)&so_dst;
	cp2 = so_dst.sa.sa_len + 1 + (char *)&so_dst;
	while (cp2 > cp1)
		*--cp2 = 0;
	cp2 = so_mask.sa.sa_len + 1 + (char *)&so_mask;
	while (cp1 > so_dst.sa.sa_data)
		*--cp1 &= *--cp2;
}

char *msgtypes[] = {
	"",
	"RTM_ADD: Add Route",
	"RTM_DELETE: Delete Route",
	"RTM_CHANGE: Change Metrics or flags",
	"RTM_GET: Report Metrics",
	"RTM_LOSING: Kernel Suspects Partitioning",
	"RTM_REDIRECT: Told to use different route",
	"RTM_MISS: Lookup failed on this address",
	"RTM_LOCK: fix specified metrics",
	"RTM_OLDADD: caused by SIOCADDRT",
	"RTM_OLDDEL: caused by SIOCDELRT",
	0,
};

char metricnames[] =
"\010rttvar\7rtt\6ssthresh\5sendpipe\4recvpipe\3expire\2hopcount\1mtu";
char routeflags[] = 
"\1UP\2GATEWAY\3HOST\4REJECT\5DYNAMIC\6MODIFIED\7DONE\010MASK_PRESENT\011CLONING\012XRESOLVE\013LLINFO\017PROTO2\020PROTO1";


void
print_rtmsg(struct rt_msghdr *rtm, int msglen)
{
	if (verbose == 0)
		return;
	if (rtm->rtm_version != RTM_VERSION) {
		(void) printf("routing message version %d not understood\n",
		    rtm->rtm_version);
		return;
	}
	(void) printf("%s\npid: %d, len %d, seq %d, errno %d, flags:",
		msgtypes[rtm->rtm_type], rtm->rtm_pid, rtm->rtm_msglen,
		rtm->rtm_seq, rtm->rtm_errno); 
	bprintf(stdout, rtm->rtm_flags, (u_char *)routeflags);
	pmsg_common(rtm);
}

void
print_getmsg(struct rt_msghdr *rtm, int msglen)
{
	if (rtm->rtm_version != RTM_VERSION) {
		(void)printf("routing message version %d not understood\n",
		    rtm->rtm_version);
		return;
	}
	if (rtm->rtm_msglen > msglen) {
		(void)printf("get length mismatch, in packet %d, returned %d\n",
		    rtm->rtm_msglen, msglen);
	}
	(void) printf("RTM_GET: errno %d, flags:", rtm->rtm_errno); 
	bprintf(stdout, rtm->rtm_flags, (u_char *)routeflags);
	(void) printf("\nmetric values:\n  ");
#define metric(f, e)\
    printf("%s: %d%s", #f, rtm->rtm_rmx.rmx_##f, e)
	metric(recvpipe, ", ");
	metric(sendpipe, ", ");
	metric(ssthresh, ", ");
	metric(rtt, "\n  ");
	metric(rttvar, ", ");
	metric(hopcount, ", ");
	metric(mtu, ", ");
	metric(expire, "\n");
#undef metric
	pmsg_common(rtm);
}

void
pmsg_common(struct rt_msghdr *rtm)
{
	char *cp;
	register struct sockaddr *sa;
	int i;

	(void) printf("\nlocks: ");
	bprintf(stdout, rtm->rtm_rmx.rmx_locks, (u_char *)metricnames);
	(void) printf(" inits: ");
	bprintf(stdout, rtm->rtm_inits, (u_char *)metricnames);
	(void) printf("\nsockaddrs: ");
	bprintf(stdout, rtm->rtm_addrs,
	    (u_char *)"\1DST\2GATEWAY\3NETMASK\4GENMASK\5IFP\6IFA\7AUTHOR");
	(void) putchar('\n');
	cp = ((char *)(rtm + 1));
	if (rtm->rtm_addrs)
		for (i = 1; i; i <<= 1)
			if (i & rtm->rtm_addrs) {
				sa = (struct sockaddr *)cp;
				(void) printf(" %s", routename(sa));
				ADVANCE(cp, sa);
			}
	(void) putchar('\n');
	(void) fflush(stdout);
}

void
bprintf(FILE *fp, int b, u_char *s_arg)
{
	register u_char *sp = s_arg;
	register int i;
	int gotsome = 0;

	if (b == 0)
		return;
	while ((i = *sp++) != 0) {
		if (b & (1 << (i-1))) {
			if (gotsome == 0)
				i = '<';
			else
				i = ',';
			(void) putc(i, fp);
			gotsome = 1;
			for (; (i = *sp) > 32; sp++)
				(void) putc(i, fp);
		} else
			while (*sp > 32)
				sp++;
	}
	if (gotsome)
		(void) putc('>', fp);
}

int
keyword(char *cp)
{
	register struct keytab *kt = keywords;

	while (kt->kt_cp && strcmp(kt->kt_cp, cp))
		kt++;
	return kt->kt_i;
}
void
sodump(sup su, char *which)
{
	char buf[INET6_ADDRSTRLEN];

	switch (su->sa.sa_family) {
	case AF_INET:
		(void) printf("%s: inet %s; ",
		    which, inet_ntop(AF_INET, &su->s_in.sin_addr, buf, sizeof(buf)));
		break;
	case AF_INET6:
		(void) printf("%s: inet6 %s; ",
		    which, inet_ntop(AF_INET6, &su->s_in6.sin6_addr, buf, sizeof(buf)));
		break;
	default:
		(void) printf("%s: af %d; ", which, su->sa.sa_family);
		break;
	}
	(void) fflush(stdout);
}

/*
 * Display routing table using GetRouteInfo().
 */
void
show_routes(void)
{
	struct rt_msghdr *buf, *rtm;
	struct sockaddr *sa_dst, *sa_gw, *sa_mask;
	char *next;
	int i;

	(void) printf("%-20s %-20s %-12s %s\n",
	    "Destination", "Gateway", "Flags", "Refs/Use");
	(void) printf("%-20s %-20s %-12s %s\n",
	    "-------------------", "-------------------",
	    "------------", "--------");

	/* Get IPv4 routes */
	buf = GetRouteInfo(AF_INET, 0);
	if (buf != NULL) {
		for (next = (char *)buf; ; next += rtm->rtm_msglen) {
			rtm = (struct rt_msghdr *)next;
			if (rtm->rtm_msglen == 0)
				break;
			/* Extract sockaddrs */
			sa_dst = sa_gw = sa_mask = NULL;
			{
				char *cp = (char *)(rtm + 1);
				for (i = 1; i; i <<= 1) {
					if (i & rtm->rtm_addrs) {
						struct sockaddr *sa = (struct sockaddr *)cp;
						if (i == RTA_DST)
							sa_dst = sa;
						else if (i == RTA_GATEWAY)
							sa_gw = sa;
						else if (i == RTA_NETMASK)
							sa_mask = sa;
						ADVANCE(cp, sa);
					}
				}
			}
			if (sa_dst) {
				(void) printf("%-20.20s ",
				    (rtm->rtm_flags & RTF_HOST) ?
				    routename(sa_dst) : netname(sa_dst));
			} else {
				(void) printf("%-20s ", "???");
			}
			if (sa_gw) {
				(void) printf("%-20.20s ", routename(sa_gw));
			} else {
				(void) printf("%-20s ", "*");
			}
			/* Print flags */
			{
				char flagbuf[16], *fp = flagbuf;
				if (rtm->rtm_flags & RTF_UP)
					*fp++ = 'U';
				if (rtm->rtm_flags & RTF_GATEWAY)
					*fp++ = 'G';
				if (rtm->rtm_flags & RTF_HOST)
					*fp++ = 'H';
				if (rtm->rtm_flags & RTF_REJECT)
					*fp++ = '!';
				if (rtm->rtm_flags & RTF_DYNAMIC)
					*fp++ = 'D';
				if (rtm->rtm_flags & RTF_MODIFIED)
					*fp++ = 'M';
				if (rtm->rtm_flags & RTF_CLONING)
					*fp++ = 'C';
				if (rtm->rtm_flags & RTF_LLINFO)
					*fp++ = 'L';
				*fp = '\0';
				(void) printf("%-12s ", flagbuf);
			}
			(void) printf("%d/%lu\n",
			    rtm->rtm_rmx.rmx_pksent ? 1 : 0,
			    (unsigned long)rtm->rtm_rmx.rmx_pksent);
		}
		FreeRouteInfo(buf);
	}

	/* Get IPv6 routes */
	buf = GetRouteInfo(AF_INET6, 0);
	if (buf != NULL) {
		for (next = (char *)buf; ; next += rtm->rtm_msglen) {
			rtm = (struct rt_msghdr *)next;
			if (rtm->rtm_msglen == 0)
				break;
			sa_dst = sa_gw = sa_mask = NULL;
			{
				char *cp = (char *)(rtm + 1);
				for (i = 1; i; i <<= 1) {
					if (i & rtm->rtm_addrs) {
						struct sockaddr *sa = (struct sockaddr *)cp;
						if (i == RTA_DST)
							sa_dst = sa;
						else if (i == RTA_GATEWAY)
							sa_gw = sa;
						else if (i == RTA_NETMASK)
							sa_mask = sa;
						ADVANCE(cp, sa);
					}
				}
			}
			if (sa_dst) {
				(void) printf("%-20.20s ",
				    (rtm->rtm_flags & RTF_HOST) ?
				    routename(sa_dst) : netname(sa_dst));
			} else {
				(void) printf("%-20s ", "???");
			}
			if (sa_gw) {
				(void) printf("%-20.20s ", routename(sa_gw));
			} else {
				(void) printf("%-20s ", "*");
			}
			{
				char flagbuf[16], *fp = flagbuf;
				if (rtm->rtm_flags & RTF_UP)
					*fp++ = 'U';
				if (rtm->rtm_flags & RTF_GATEWAY)
					*fp++ = 'G';
				if (rtm->rtm_flags & RTF_HOST)
					*fp++ = 'H';
				if (rtm->rtm_flags & RTF_REJECT)
					*fp++ = '!';
				if (rtm->rtm_flags & RTF_DYNAMIC)
					*fp++ = 'D';
				if (rtm->rtm_flags & RTF_MODIFIED)
					*fp++ = 'M';
				if (rtm->rtm_flags & RTF_CLONING)
					*fp++ = 'C';
				if (rtm->rtm_flags & RTF_LLINFO)
					*fp++ = 'L';
				*fp = '\0';
				(void) printf("%-12s ", flagbuf);
			}
			(void) printf("%d/%lu\n",
			    rtm->rtm_rmx.rmx_pksent ? 1 : 0,
			    (unsigned long)rtm->rtm_rmx.rmx_pksent);
		}
		FreeRouteInfo(buf);
	}
}

#if 0
/* Dead protocol support (XNS, ISO, X.25) kept for reference */
#define VIRGIN	0
#define GOTONE	1
#define GOTTWO	2
/* Inputs */
#define	DIGIT	(4*0)
#define	END	(4*1)
#define DELIM	(4*2)

void
sockaddr(addr, sa)
register char *addr;
register struct sockaddr *sa;
{
	register char *cp = (char *)sa;
	int size = sa->sa_len;
	char *cplim = cp + size;
	register int byte = 0, state = VIRGIN, new;

	bzero(cp, size);
	do {
		if ((*addr >= '0') && (*addr <= '9')) {
			new = *addr - '0';
		} else if ((*addr >= 'a') && (*addr <= 'f')) {
			new = *addr - 'a' + 10;
		} else if ((*addr >= 'A') && (*addr <= 'F')) {
			new = *addr - 'A' + 10;
		} else if (*addr == 0) 
			state |= END;
		else
			state |= DELIM;
		addr++;
		switch (state /* | INPUT */) {
		case GOTTWO | DIGIT:
			*cp++ = byte; /*FALLTHROUGH*/
		case VIRGIN | DIGIT:
			state = GOTONE; byte = new; continue;
		case GOTONE | DIGIT:
			state = GOTTWO; byte = new + (byte << 4); continue;
		default: /* | DELIM */
			state = VIRGIN; *cp++ = byte; byte = 0; continue;
		case GOTONE | END:
		case GOTTWO | END:
			*cp++ = byte; /* FALLTHROUGH */
		case VIRGIN | END:
			break;
		}
		break;
	} while (cp < cplim); 
	sa->sa_len = cp - (char *)sa;
}
#endif



#if 1
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*LINTLIBRARY*/
#define EOF	(-1)
#define ERR(s, c)\
  if(opterr) { fprintf(stderr, "%s%s%lc\n", argv[0], s, c); }

int	opterr = 1;
int	optind = 1;
int	optopt;
char	*optarg;

int GetOpt(int argc, char **argv, char *opts)
{
	static int sp = 1;
	register int c;
	register char *cp;

	if(sp == 1)
		if(optind >= argc ||
		   argv[optind][0] != '-' || argv[optind][1] == '\0')
			return(EOF);
		else if(strcmp(argv[optind], "--") == 0) {
			optind++;
			return(EOF);
		}
	optopt = c = argv[optind][sp];
	if(c == ':' || (cp=index(opts, c)) == NULL) {
		ERR(": illegal option -- ", c);
		if(argv[optind][++sp] == '\0') {
			optind++;
			sp = 1;
		}
		return('?');
	}
	if(*++cp == ':') {
		if(argv[optind][sp+1] != '\0')
			optarg = &argv[optind++][sp+1];
		else if(++optind >= argc) {
			ERR(": option requires an argument -- ", c);
			sp = 1;
			return('?');
		} else
			optarg = argv[optind++];
		sp = 1;
	} else {
		if(argv[optind][++sp] == '\0') {
			sp = 1;
			optind++;
		}
		optarg = NULL;
	}
	return(c);
}
#endif

VOID CleanUpExit(LONG error)
{
	exit(error);
}
