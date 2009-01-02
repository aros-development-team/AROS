/*
 * Copyright (c) 1983, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 2006
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
 */

/* "@(#)if.c	    8.2 (Berkeley) 2/21/94" */

#include <devices/timer.h>
#include <dos/dos.h>
#include <proto/exec.h>

#include <sys/types.h>
#include <sys/protosw.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#ifdef NETNS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif
#ifdef ISO
#include <netiso/iso.h>
#include <netiso/iso_var.h>
#endif
#include <arpa/inet.h>

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "netstat.h"

#define	YES	1
#define	NO	0

static void sidewaysintpr __P((u_int, u_long));
static void catchalarm __P((int));

/*
 * Print a description of the network interfaces.
 */
void
intpr(interval, ifnetaddr)
	int interval;
	u_long ifnetaddr;
{
	struct ifnet ifnet;
	union {
		struct ifaddr ifa;
		struct in_ifaddr in;
#ifdef NETNS
		struct ns_ifaddr ns;
#endif
#ifdef ISO
		struct iso_ifaddr iso;
#endif
	} ifaddr;
	u_long ifaddraddr;
	struct sockaddr *sa;
	char name[16];

	if (ifnetaddr == 0) {
		printf("ifnet: symbol not defined\n");
		return;
	}
	if (interval) {
		sidewaysintpr((unsigned)interval, ifnetaddr);
		return;
	}
	if (kread(ifnetaddr, (char *)&ifnetaddr, sizeof ifnetaddr))
		return;
	printf("%-5.5s %-5.5s %-11.11s %-15.15s %8.8s %5.5s",
		"Name", "Mtu", "Network", "Address", "Ipkts", "Ierrs");
	if (bflag)
		printf(" %10.10s","Ibytes");
	printf(" %8.8s %5.5s", "Opkts", "Oerrs");
	if (bflag)
		printf(" %10.10s","Obytes");
	printf(" %5s", "Coll");
	if (tflag)
		printf(" %s", "Time");
	if (dflag)
		printf(" %s", "Drop");
	putchar('\n');
	ifaddraddr = 0;
	while (ifnetaddr || ifaddraddr) {
		struct sockaddr_in *sin;
		register char *cp;
		int n, m;

		if (ifaddraddr == 0) {
			if (kread(ifnetaddr, (char *)&ifnet, sizeof ifnet) ||
			    kread((u_long)ifnet.if_name, name, 16))
				return;
			name[15] = '\0';
			ifnetaddr = (u_long)ifnet.if_next;
			if (interface != 0 && (strcmp(name, interface) != 0 ||
			    unit != ifnet.if_unit))
				continue;
			cp = index(name, '\0');
			cp += sprintf(cp, "%d", ifnet.if_unit);
			if ((ifnet.if_flags&IFF_UP) == 0)
				*cp++ = '*';
			*cp = '\0';
			ifaddraddr = (u_long)ifnet.if_addrlist;
		}
		printf("%-5.5s %-5d ", name, ifnet.if_mtu);
		if (ifaddraddr == 0) {
			printf("%-11.11s ", "none");
			printf("%-15.15s ", "none");
		} else {
			if (kread(ifaddraddr, (char *)&ifaddr, sizeof ifaddr)) {
				ifaddraddr = 0;
				continue;
			}
#define CP(x) ((char *)(x))
			cp = (CP(ifaddr.ifa.ifa_addr) - CP(ifaddraddr)) +
				CP(&ifaddr); sa = (struct sockaddr *)cp;
			switch (sa->sa_family) {
			case AF_UNSPEC:
				printf("%-11.11s ", "none");
				printf("%-15.15s ", "none");
				break;
			case AF_INET:
				sin = (struct sockaddr_in *)sa;
#ifdef notdef
				/* can't use inet_makeaddr because kernel
				 * keeps nets unshifted.
				 */
				in = inet_makeaddr(ifaddr.in.ia_subnet,
					INADDR_ANY);
				printf("%-11.11s ", netname(in.s_addr,
				    ifaddr.in.ia_subnetmask));
#else
				printf("%-11.11s ",
				    netname(htonl(ifaddr.in.ia_subnet),
				    ifaddr.in.ia_subnetmask));
#endif
				printf("%-15.15s ",
				    routename(sin->sin_addr.s_addr));
				break;
#ifdef NETNS
			case AF_NS:
				{
				struct sockaddr_ns *sns =
					(struct sockaddr_ns *)sa;
				u_long net;
				char netnum[8];

				*(union ns_net *) &net = sns->sns_addr.x_net;
		sprintf(netnum, "%lxH", ntohl(net));
				upHex(netnum);
				printf("ns:%-8s ", netnum);
				printf("%-15s ",
				    ns_phost((struct sockaddr *)sns));
				}
				break;
#endif
			case AF_LINK:
				{
				struct sockaddr_dl *sdl =
					(struct sockaddr_dl *)sa;
				    cp = (char *)LLADDR(sdl);
				    n = sdl->sdl_alen;
				}
				m = printf("<Link>");
				goto hexprint;
			default:
				m = printf("(%d)", sa->sa_family);
				for (cp = sa->sa_len + (char *)sa;
					--cp > sa->sa_data && (*cp == 0);) {}
				n = cp - sa->sa_data + 1;
				cp = sa->sa_data;
			hexprint:
				while (--n >= 0)
					m += printf("%02x%c", *cp++ & 0xff,
						    n > 0 ? '.' : ' ');
				m = 28 - m;
				while (m-- > 0)
					putchar(' ');
				break;
			}
			ifaddraddr = (u_long)ifaddr.ifa.ifa_next;
		}
		printf("%8d %5d ",
		    ifnet.if_ipackets, ifnet.if_ierrors);
		if (bflag)
			printf("%10d ", ifnet.if_ibytes);
		printf("%8d %5d ",
		    ifnet.if_opackets, ifnet.if_oerrors);
		if (bflag)
			printf("%10d ", ifnet.if_obytes);
		printf("%5d", ifnet.if_collisions);
		if (tflag)
			printf(" %3d", ifnet.if_timer);
		if (dflag)
			printf(" %3d", ifnet.if_snd.ifq_drops);
		putchar('\n');
	}
}

#define	MAXIF	10
struct	iftot {
	char	ift_name[16];		/* interface name */
	u_int	ift_ip;			/* input packets */
	u_int	ift_ie;			/* input errors */
	u_int	ift_op;			/* output packets */
	u_int	ift_oe;			/* output errors */
	u_int	ift_co;			/* collisions */
	u_int	ift_dr;			/* drops */
	u_int	ift_ib;			/* input bytes */
	u_int	ift_ob;			/* output bytes */
} iftot[MAXIF];

u_char	signalled;			/* set if alarm goes off "early" */

struct MsgPort *timerport;
struct timerequest *timerio;

void __chkabort()
{
}

void CleanupExit(rc)
{
	if (timerio)
		DeleteIORequest(timerio);
	if (timerport)
		DeleteMsgPort(timerport);
	exit(rc);
}

/*
 * Print a running summary of interface statistics.
 * Repeat display every interval seconds, showing statistics
 * collected over that interval.  Assumes that interval is non-zero.
 * First line printed at top of screen is always cumulative.
 */
static void
sidewaysintpr(interval, off)
	unsigned interval;
	u_long off;
{
	struct ifnet ifnet;
	u_long firstifnet;
	register struct iftot *ip, *total;
	register int line;
	struct iftot *lastif, *sum, *interesting;
	ULONG sigmask;

	if (kread(off, (char *)&firstifnet, sizeof (u_long)))
		return;
	lastif = iftot;
	sum = iftot + MAXIF - 1;
	total = sum - 1;
	interesting = iftot;
	for (off = firstifnet, ip = iftot; off;) {
		char *cp;

		if (kread(off, (char *)&ifnet, sizeof ifnet))
			break;
		ip->ift_name[0] = '(';
		if (kread((u_long)ifnet.if_name, ip->ift_name + 1, 15))
			break;
		if (interface && strcmp(ip->ift_name + 1, interface) == 0 &&
		    unit == ifnet.if_unit)
			interesting = ip;
		ip->ift_name[15] = '\0';
		cp = index(ip->ift_name, '\0');
		sprintf(cp, "%d)", ifnet.if_unit);
		ip++;
		if (ip >= iftot + MAXIF - 2)
			break;
		off = (u_long) ifnet.if_next;
	}
	lastif = ip;

	timerport = CreateMsgPort();
	if (!timerport) {
		fprintf(stderr, "Unable to create timer message port\n");
		exit(0);
	}
	sigmask = (1 << timerport->mp_SigBit) | SIGBREAKF_CTRL_C;
	timerio = CreateIORequest(timerport, sizeof(struct timerequest));
	if (!timerio) {
		fprintf(stderr, "Unable to create timer I/O request\n");
		CleanupExit(20);
	}
	if (OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)timerio, 0)) {
		fprintf(stderr, "Unable to open timer.device\n");
		CleanupExit(20);
	}
	timerio->tr_node.io_Command = TR_ADDREQUEST;
	timerio->tr_time.tv_secs = interval;
	timerio->tr_time.tv_micro = 0;
	SendIO ((struct IORequest *)timerio);
banner:
	printf("     input   %s%-6.6s  %soutput       ", bflag ? "          " : "",
	    interesting->ift_name, bflag ? "          " : "");
	if (lastif - iftot > 0) {
		if (dflag)
			printf("      ");
		printf("          input  %s(Total)  %soutput", bflag ? "          " : "",
		    bflag ? "          " : "");
	}
	for (ip = iftot; ip < iftot + MAXIF; ip++) {
		ip->ift_ip = 0;
		ip->ift_ie = 0;
		ip->ift_op = 0;
		ip->ift_oe = 0;
		ip->ift_co = 0;
		ip->ift_dr = 0;
		ip->ift_ib = 0;
		ip->ift_ob = 0;
	}
	putchar('\n');
	printf("%8.8s %5.5s ", "packets", "errs");
	if (bflag)
		printf("%10.10s ","bytes");
	printf("%8.8s %5.5s ", "packets", "errs");
	if (bflag)
		printf("%10.10s ","bytes");
	printf("%5.5s ", "colls");
	if (dflag)
		printf("%5.5s ", "drops");
	if (lastif - iftot > 0) {
		printf(" %8.8s %5.5s", "packets", "errs");
		if (bflag)
			printf(" %10.10s", "bytes");
		printf(" %8.8s %5.5s", "packets", "errs");
		if (bflag)
			printf(" %10.10s", "bytes");
		printf(" %5.5s", "colls");
		if (dflag)
			printf(" %5.5s", "drops");
	}
	putchar('\n');
	fflush(stdout);
	line = 0;
loop:
	sum->ift_ip = 0;
	sum->ift_ie = 0;
	sum->ift_op = 0;
	sum->ift_oe = 0;
	sum->ift_co = 0;
	sum->ift_dr = 0;
	sum->ift_ib = 0;
	sum->ift_ob = 0;
	for (off = firstifnet, ip = iftot; off && ip < lastif; ip++) {
		if (kread(off, (char *)&ifnet, sizeof ifnet)) {
			off = 0;
			continue;
		}
		if (ip == interesting) {
			printf("%8d %5d ",
				ifnet.if_ipackets - ip->ift_ip,
				ifnet.if_ierrors - ip->ift_ie);
			if (bflag)
				printf("%10d ", ifnet.if_ibytes - ip->ift_ib);
			printf("%8d %5d ",
				ifnet.if_opackets - ip->ift_op,
				ifnet.if_oerrors - ip->ift_oe);
			if (bflag)
				printf("%10d ", ifnet.if_obytes - ip->ift_ob);
			printf("%5d", ifnet.if_collisions - ip->ift_co);
			if (dflag)
				printf(" %5d",
				    ifnet.if_snd.ifq_drops - ip->ift_dr);
		}
		ip->ift_ip = ifnet.if_ipackets;
		ip->ift_ie = ifnet.if_ierrors;
		ip->ift_op = ifnet.if_opackets;
		ip->ift_oe = ifnet.if_oerrors;
		ip->ift_co = ifnet.if_collisions;
		ip->ift_dr = ifnet.if_snd.ifq_drops;
		ip->ift_ib = ifnet.if_ibytes;
		ip->ift_ob = ifnet.if_obytes;
		sum->ift_ip += ip->ift_ip;
		sum->ift_ie += ip->ift_ie;
		sum->ift_op += ip->ift_op;
		sum->ift_oe += ip->ift_oe;
		sum->ift_co += ip->ift_co;
		sum->ift_dr += ip->ift_dr;
		sum->ift_ib += ip->ift_ib;
		sum->ift_ob += ip->ift_ob;
		off = (u_long) ifnet.if_next;
	}
	if (lastif - iftot > 0) {
		printf("  %8d %5d",
			sum->ift_ip - total->ift_ip,
			sum->ift_ie - total->ift_ie);
		if (bflag)
			printf(" %10d", sum->ift_ib - total->ift_ib);
		printf(" %8d %5d",
			sum->ift_op - total->ift_op,
			sum->ift_oe - total->ift_oe);
		if (bflag)
			printf(" %10d", sum->ift_ob - total->ift_ob);
		printf(" %5d", sum->ift_co - total->ift_co);
		if (dflag)
			printf(" %5d", sum->ift_dr - total->ift_dr);
	}
	*total = *sum;
	putchar('\n');
	fflush(stdout);
	line++;
	if (Wait (sigmask) & SIGBREAKF_CTRL_C) {
		AbortIO ((struct IORequest *)timerio);
		WaitIO ((struct IORequest *)timerio);
		CloseDevice ((struct IORequest *)timerio);
		CleanupExit(0);
	}
	WaitIO ((struct IORequest *)timerio);
	timerio->tr_node.io_Command = TR_ADDREQUEST;
	timerio->tr_time.tv_secs = interval;
	timerio->tr_time.tv_micro = 0;
	SendIO ((struct IORequest *)timerio);
	if (line == 21)
		goto banner;
	goto loop;
	/*NOTREACHED*/
}

/*
 * Called if an interval expires before sidewaysintpr has completed a loop.
 * Sets a flag to not wait for the alarm.
 */
static void
catchalarm(signo)
	int signo;
{
	signalled = YES;
}
