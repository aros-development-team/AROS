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

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
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
 *	@(#)in_proto.c	7.5 (Berkeley) 6/28/90
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <netinet/in_proto_protos.h>

/*
 * TCP/IP protocol family: IP, ICMP, UDP, TCP.
 */
#include <netinet/tcp.h>
#include <netinet/ip_icmp_protos.h>
#include <netinet/ip_input_protos.h>
#include <netinet/ip_output_protos.h>
#include <netinet/raw_ip_protos.h>

/*
 * IMP protocol family: raw interface.
 * Using the raw interface entry to get the timer routine
 * in is a kludge.
 */
#if NIMP > 0
int	rimp_output(), hostslowtimo();
#endif

#if NSIP
int	idpip_input(), nsip_ctlinput();
#endif

#if TPIP
int	tpip_input(), tpip_ctlinput(), tp_ctloutput(), tp_usrreq();
int	tp_init(), tp_slowtimo(), tp_drain();
#endif

#if EON
int	eoninput(), eonctlinput(), eonprotoinit();
#endif /* EON */

extern	struct domain inetdomain;

struct protosw inetsw[] = {
{ 0,		&inetdomain,	0,		0,
  NULL,		ip_output,	NULL,		NULL,
  NULL,
  ip_init,	NULL,		ip_slowtimo,	ip_drain,
},
{ SOCK_DGRAM,	&inetdomain,	IPPROTO_UDP,	PR_ATOMIC|PR_ADDR,
  udp_input,	NULL,		udp_ctlinput,	ip_ctloutput,
  udp_usrreq,
  udp_init,	NULL,		NULL,		NULL,
},
{ SOCK_STREAM,	&inetdomain,	IPPROTO_TCP,	PR_CONNREQUIRED|PR_IMPLOPCL|PR_WANTRCVD,
  tcp_input,	NULL,		tcp_ctlinput,	tcp_ctloutput,
  tcp_usrreq,
  tcp_init,	tcp_fasttimo,	tcp_slowtimo,	tcp_drain,
},
{ SOCK_RAW,	&inetdomain,	IPPROTO_RAW,	PR_ATOMIC|PR_ADDR,
  rip_input,	rip_output,	NULL,		rip_ctloutput,
  rip_usrreq,
  NULL,		NULL,		NULL,		NULL,
},
{ SOCK_RAW,	&inetdomain,	IPPROTO_ICMP,	PR_ATOMIC|PR_ADDR,
  icmp_input,	rip_output,	NULL,		rip_ctloutput,
  rip_usrreq,
  NULL,		NULL,		NULL,		NULL,
},
#if TPIP
{ SOCK_SEQPACKET,&inetdomain,	IPPROTO_TP,	PR_CONNREQUIRED|PR_WANTRCVD,
  tpip_input,	NULL,		tpip_ctlinput,		tp_ctloutput,
  tp_usrreq,
  tp_init,	NULL,		tp_slowtimo,	tp_drain,
},
#endif
/* EON (ISO CLNL over IP) */
#if EON
{ SOCK_RAW,	&inetdomain,	IPPROTO_EON,	0,
  eoninput,	NULL,		eonctlinput,		NULL,
  NULL,
  eonprotoinit,	NULL,		NULL,		NULL,
},
#endif
#if NSIP
{ SOCK_RAW,	&inetdomain,	IPPROTO_IDP,	PR_ATOMIC|PR_ADDR,
  idpip_input,	rip_output,	nsip_ctlinput,	NULL,
  rip_usrreq,
  NULL,		NULL,		NULL,		NULL,
},
#endif
	/* raw wildcard */
{ SOCK_RAW,	&inetdomain,	0,		PR_ATOMIC|PR_ADDR,
  rip_input,	rip_output,	NULL,		rip_ctloutput,
  rip_usrreq,
  NULL,		NULL,		NULL,		NULL,
},
};

struct domain inetdomain =
    { AF_INET, "internet", NULL, NULL, NULL, 
      inetsw, &inetsw[sizeof(inetsw)/sizeof(inetsw[0])], NULL };

#if NIMP > 0
extern	struct domain impdomain;

struct protosw impsw[] = {
{ SOCK_RAW,	&impdomain,	0,		PR_ATOMIC|PR_ADDR,
  0,		rimp_output,	0,		0,
  rip_usrreq,
  0,		0,		hostslowtimo,	0,
},
};

struct domain impdomain =
    { AF_IMPLINK, "imp", NULL, NULL, NULL,
      impsw, &impsw[sizeof (impsw)/sizeof(impsw[0])], NULL };
#endif

#if NHY > 0
/*
 * HYPERchannel protocol family: raw interface.
 */
int	rhy_output();
extern	struct domain hydomain;

struct protosw hysw[] = {
{ SOCK_RAW,	&hydomain,	0,		PR_ATOMIC|PR_ADDR,
  0,		rhy_output,	0,		0,
  rip_usrreq,
  0,		0,		0,		0,
},
};

struct domain hydomain =
    { AF_HYLINK, "hy", NULL, NULL, NULL,
      hysw, &hysw[sizeof (hysw)/sizeof(hysw[0])], NULL };
#endif
