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
 * Copyright (c) 1983, 1988 Regents of the University of California.
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
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/synch.h>
#include <sys/malloc.h>
#include <sys/syslog.h>

#include <kern/amiga_includes.h>
#include <kern/amiga_rexx.h>
#include <kern/amiga_config.h>

#include <dos/rdargs.h>

#include <proto/dos.h>

#include <net/route.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#include <api/apicalls.h>

long count = 0;

int
ultoa(unsigned long ul,char *buffer)
{
  static char buf[10];
  char *p;
  int len;

  p = buf;
  *p='\0';

  do {
    *++p = (ul % 10)+'0';
  } while (ul /= 10);

  len = p - buf;

  while (*buffer++ = *p--)
    ;
  return(len);
}

int
ltoa(long l, char *buffer)
{
  int len=0;

  if(l<0){
    *buffer++='-';
    l=-l;
    len++;
  }
 return (len + ultoa((unsigned long)l, buffer));
}

/*
 * Allocate big enough buffer for reply
 */
LONG
CS_Alloc(struct CSource *reply, size_t size)
{
  /* do we have enough space? */
  if (reply->CS_Length < size) {
    char *buffer;
    if ((buffer = bsd_malloc(size, M_TEMP, 0)) == NULL){
      return 0;
    }
    /*
     * Old buffer will be freed by caller
     */
    reply->CS_Buffer = buffer;
    reply->CS_Length = size;
  }

  return 1;
}

/*
 * getsockets(): a reply statics for all sockets
 *
 */
struct printsocket {
  struct in_addr  inp_faddr;	/* u_long, far addr */
  u_short         inp_fport;
  struct in_addr  inp_laddr;	/* u_long, local addr */
  u_short         inp_lport;
  short           so_type;	/* SOCK_STREAM, _DGRAM... */
  u_short         so_rcv_sb_cc;	/* Recv queue size */
  u_short         so_snd_sb_cc;	/* Send queue */
  short           inp_ppcb_t_state; /* State of TCP connection */
};

LONG
getsockets(struct CSource *args, UBYTE **errstrp, struct CSource *res)
{
  int i, count = 0;
  struct inpcb *pcb;

  struct printsocket *pps, *mem;
  spl_t s = splnet();		/* Critical section starts here */

  /* Count number of connections */
//for(pcb = udb.inp_next; pcb != &udb ; pcb = pcb->inp_next)
  for(pcb = udb.lh_first; pcb != NULL; pcb = pcb->inp_list.le_next)
    ++count;
//for(pcb = tcb.inp_next; pcb != &tcb ; pcb = pcb->inp_next)
  for(pcb = tcb.lh_first; pcb != NULL; pcb = pcb->inp_list.le_next)
    ++count;

  if (count == 0) {		/* return now if nothing to print */
    splx(s);
    return RETURN_OK;
  }

  /* Allocate memory */
  mem = (struct printsocket *)
    bsd_malloc(sizeof(struct printsocket) * (count), M_TEMP, 0);

  if (mem == NULL) {
    splx(s);
    *errstrp = ERR_MEMORY;
    return RETURN_FAIL;
  }

  /* Proto recv-q send-q laddr lport faddr fport state */
  /*  1   1   4  1   4  1  8  1  4  1   8 1   4 1 1   1 =  42 chars */
#define STATLEN 42
  if (!CS_Alloc(res, STATLEN * count + 1)) {
    /* Allocation failed, free printsocket memory */
    splx(s);
    bsd_free(mem, M_TEMP);
    *errstrp = ERR_MEMORY;
    return RETURN_FAIL;
  }

  /* Copy information, TCP first.. */
//for(pcb = tcb.inp_next, pps = mem; pcb != &tcb; pcb = pcb->inp_next, ++pps){
  for(pcb = tcb.lh_first, pps = mem; pcb != NULL; pcb = pcb->inp_list.le_next){
    pps->inp_faddr = pcb->inp_faddr;
    pps->inp_fport = pcb->inp_fport;
    pps->inp_laddr = pcb->inp_laddr;
    pps->inp_lport = pcb->inp_lport;
    pps->so_type = pcb->inp_socket->so_type;
    pps->so_rcv_sb_cc = pcb->inp_socket->so_rcv.sb_cc;
    pps->so_snd_sb_cc = pcb->inp_socket->so_snd.sb_cc;
    pps->inp_ppcb_t_state = ((struct tcpcb *)(pcb->inp_ppcb))->t_state;
  }
  /* ...then UDP */
//for(pcb = udb.inp_next; pcb != &udb; pcb = pcb->inp_next, ++pps){
  for(pcb = udb.lh_first; pcb != NULL; pcb = pcb->inp_list.le_next){
    pps->inp_faddr = pcb->inp_faddr;
    pps->inp_fport = pcb->inp_fport;
    pps->inp_laddr = pcb->inp_laddr;
    pps->inp_lport = pcb->inp_lport;
    pps->so_type = pcb->inp_socket->so_type;
    pps->so_rcv_sb_cc = pcb->inp_socket->so_rcv.sb_cc;
    pps->so_snd_sb_cc = pcb->inp_socket->so_snd.sb_cc;
    pps->inp_ppcb_t_state = 0;	/* NO state for UDP */
  }

  splx(s);			/* Critical section completed now */

  /*
   * Print all socket entries
   */
  for(i = 0; i < count; i++)
    csprintf(res, "%lc %04lx %04lx %08lx %04lx %08lx %04lx %1lx%s",
	     mem[i].so_type == SOCK_STREAM ? 't': 'u',
	     mem[i].so_rcv_sb_cc, mem[i].so_snd_sb_cc,
	     mem[i].inp_laddr.s_addr, mem[i].inp_lport,
	     mem[i].inp_faddr.s_addr, mem[i].inp_fport,
	     mem[i].inp_ppcb_t_state,
	     (i < count - 1) ? " " : "");

#if DIAGNOSTIC			/* check for overrun */
  if (res->CS_CurChr >= res->CS_Length)
    log(LOG_ERR, "getsockets(): buffer overwritten by %ld bytes\n",
	res->CS_CurChr - res->CS_Length + 1);
#endif

  /*
   * free mem
   */
  bsd_free(mem, M_TEMP);

  return RETURN_OK;
}

/*
 * Get ICMP history profiles
 */
#include <netinet/icmp_var.h>
#include <netinet/ip_icmp.h>

extern struct icmpstat icmpstat;

LONG 
read_icmphist(struct CSource *args, UBYTE **errstrp, struct CSource *res)
{
  int i;
  UBYTE *p = res->CS_Buffer;

  for(i = 0;i <= ICMP_MAXTYPE; i++){
    p += ultoa(icmpstat.icps_outhist[i], p);
    *p++=' ';
  }

  for(i = 0;i <= ICMP_MAXTYPE; i++){
    p += ultoa(icmpstat.icps_inhist[i], p);
    *p++ = ' ';
  }
  *--p = '\0';

  res->CS_CurChr = p - res->CS_Buffer;
  return RETURN_OK;
}

/*
 * Get routing tables
 */
#include <net/route.h>
#include <net/if.h>

#define DB(x) ;

/* Address families supported */
extern STRPTR KW_Protocols;

/* What is stored in the route entry */
#define NORMAL 0
#define MASK   1
#define DUPED  2

/* Our recursion depth in route tree. 32 is # of bits in IP address */
/* Other protocols may require deeper stack */
#define MAX_ROUTE_TREE_DEPTH 32

struct printroute {
  struct sockaddr pr_dest;
  struct sockaddr pr_via;
  u_long          pr_use;
  struct ifnet   *pr_ifp;
  short           pr_refcnt;
  u_short         pr_flags;
  u_short         pr_what;
};

static void cstat_rtentry(struct rtentry *rt, struct printroute *pr);
static void cstat_maskentry(struct radix_node *rn, struct printroute *pr);
static int  cstat_rtree(struct radix_node *rn, struct printroute *pr, int);
static void cstat_rtflags(struct CSource *, const char *, register int);

LONG
getroutes(struct CSource *args, UBYTE **errstrp, struct CSource *res)
{
  UBYTE Buffer[KEYWORDLEN];
  int af, newroutes, routes = 0;
  struct radix_node_head *rnh;
  struct printroute *mem, *pr;
  spl_t s;

  /*
   * Parse address family
   *
   * Match the  against the template. The af is the index of
   * the matching keyword, if one is found.
   */
  if (ReadItem(Buffer, sizeof(Buffer), args) <= 0) {
    *errstrp = ERR_SYNTAX;
    return RETURN_ERROR;
  }
  if ((af = FindArg(KW_Protocols, Buffer)) < 0) {
    res->CS_CurChr = 0;
    csprintf(res, ERR_ILLEGAL_VAR, "getroutes", Buffer);
    *errstrp = res->CS_Buffer;
    return RETURN_WARN;
  }

  DB(log(LOG_DEBUG, "getroutes: getting routes for protocol %s (%ld)",
	 Buffer, af));

  s = splnet();			/*                         Critical section */

  /* Count entries in tables */
  for (rnh = radix_node_head; rnh ; rnh = rnh->rnh_next) {
    if (af == AF_UNSPEC || af == rnh->rnh_af) {
      newroutes = cstat_rtree(rnh->rnh_treetop, NULL, rnh->rnh_af != AF_UNSPEC);
      if (newroutes < 0) {
	splx(s);
	*errstrp = (UBYTE *)"Route tree overflow.\n";
	return RETURN_FAIL;
      }
      routes += newroutes;
    }
  }

  DB(log(LOG_DEBUG, "getroutes: found %ld routes", routes));

  /* Allocate memory for entries */
  pr = mem = (struct printroute *)
    bsd_malloc(sizeof(struct printroute) * (routes), M_TEMP, 0);

  if (mem == NULL) {
    splx(s);
    *errstrp = ERR_MEMORY;
    return RETURN_FAIL;
  }

  /* Fill in printroute table */
  for (rnh = radix_node_head; rnh ; rnh = rnh->rnh_next) {
    if (af == AF_UNSPEC || af == rnh->rnh_af) {
      pr += cstat_rtree(rnh->rnh_treetop, pr, rnh->rnh_af != AF_UNSPEC);
#if DIAGNOSTIC
      if (pr > mem + routes) {
	const UBYTE *msg = "cstat_rtree(): found more entries than counted!\n";
	splx(s);
	bsd_free(mem, M_TEMP);
	log(LOG_ERR, msg);
	*errstrp = (UBYTE *)msg;
	return RETURN_FAIL;
      }
#endif
    }
  }
  splx(s);			/*                  End of critical section */

  /* Reply line has format
   * protocol route gateway flags refcount used device/unit + separation
   * 2  +     8  +  8  +    4  +  4  +     8  + 32  +  10  + 10  = 90 chars
   */
  if (!CS_Alloc(res, 90 * routes)) {
    /* Allocation failed, free printroute memory */
    bsd_free(mem, M_TEMP);
    *errstrp = ERR_MEMORY;
    return RETURN_FAIL;
  }

  /* cstat_rtree_print */
  for (pr = mem; routes-- > 0 ; pr++) {
    csprintf(res, "%02lx %08lx %08lx ",
	     pr->pr_dest.sa_family,
	     ((struct sockaddr_in *)&pr->pr_dest)->sin_addr.s_addr,
	     ((struct sockaddr_in *)&pr->pr_via)->sin_addr.s_addr);
    cstat_rtflags(res, "%-8.8s ", pr->pr_flags);
    csprintf(res, "%04lx %08lx ", pr->pr_refcnt, pr->pr_use);
    if (pr->pr_ifp) {
      csprintf(res, "%s%ld ", pr->pr_ifp->if_name, pr->pr_ifp->if_unit);
    } else {
      csprintf(res, "none ");
    }
  }

  /* free printsocket structures */
  bsd_free(mem, M_TEMP);

  return RETURN_OK;
}

/*
 * cstat_rtree(): get a route tree to a table
 */
static int
cstat_rtree(struct radix_node *rn, struct printroute *pr, int do_rtent)
{
  short routes = 0;
  short stack_pointer = 0;
  struct radix_node *stack[MAX_ROUTE_TREE_DEPTH];

 again:
  if (rn->rn_b < 0) {
    if ((rn->rn_flags & RNF_ROOT) == 0) {
      if (pr) {
	if (do_rtent)
	  cstat_rtentry((struct rtentry*)rn, pr);
	else
	  cstat_maskentry(rn, pr);
	pr++;
      }
      routes++;
    }
    if (rn = rn->rn_dupedkey)
      goto again;
  } else {
    if (rn->rn_r) {
      if (stack_pointer >= MAX_ROUTE_TREE_DEPTH - 1)
	return -1;
      stack[stack_pointer++] = rn->rn_r;
    }
    /* tail recursion removal */
    if (rn = rn->rn_l) {
      goto again;
    }
  }

  /* recursion removal */
  if (stack_pointer-- > 0) {
    rn = stack[stack_pointer];
    goto again;
  }

  DB(log(LOG_DEBUG, "rtree: found %ld routes", routes));
  return routes;
}

/*
 * Copy rtentry into printroute
 *      assumes that rtentry and pr are valid
 */
static void
cstat_rtentry(struct rtentry *rt, struct printroute *pr)
{
  /* These should allow variable sized sockaddrs */
  bcopy(rt_key(rt), &pr->pr_dest, sizeof(pr->pr_dest));
  bcopy(rt->rt_gateway, &pr->pr_via, sizeof(pr->pr_via));

  /* */
  pr->pr_use    = rt->rt_use;
  pr->pr_refcnt = rt->rt_refcnt;
  pr->pr_ifp    = rt->rt_ifp;
  pr->pr_flags  = rt->rt_flags;
  if (rt->rt_nodes->rn_dupedkey)
    pr->pr_what = DUPED;
  else
    pr->pr_what = NORMAL;
}

/*
 * Copy netmask into printroute
 *      assumes that rn and pr are valid
 */
static void
cstat_maskentry(struct radix_node *rn, struct printroute *pr)
{
  /* These should allow variable sized sockaddrs */
  bcopy((struct sockaddr *)rn->rn_key, &pr->pr_dest, sizeof(pr->pr_dest));
  bzero(&pr->pr_via, sizeof(pr->pr_via));

  pr->pr_use    = 0;
  pr->pr_refcnt = 0;
  pr->pr_ifp    = NULL;
  pr->pr_flags  = 0;
  pr->pr_what   = MASK;
}

/*
 * Definitions for showing gateway flags.
 */
struct bits {
	short	b_mask;
	char	b_val;
} bits[] = {
	{ RTF_UP,	'U' },
	{ RTF_GATEWAY,	'G' },
	{ RTF_HOST,	'H' },
	{ RTF_DYNAMIC,	'D' },
	{ RTF_MODIFIED,	'M' },
	{ RTF_CLONING,	'C' },
	{ RTF_XRESOLVE,	'X' },
	{ RTF_LLINFO,	'L' },
	{ RTF_REJECT,	'R' },
	{ 0 }
};

static void
cstat_rtflags(struct CSource *res, const char *format, register int f)
{
  char name[33], *flags;
  register struct bits *p = bits;
  for (flags = name; p->b_mask; p++)
    if (p->b_mask & f)
      *flags++ = p->b_val;
  *flags = '\0';
  if (name[0] != '\0') {
    csprintf(res, format, name);
  } else {
    /* Empty string may confuse parsers */
    csprintf(res, format, "\"\"");
  }
}

extern char *host_name;
extern size_t host_namelen;
extern int sethostname(const char * name, size_t namelen);
extern struct Library *SocketBase; /* base opened by NETTRACE */

LONG
rexx_gethostname(struct CSource *args, UBYTE **errstrp, struct CSource *res)
{
  __gethostname(CURRENT(res), SPACE(res), (struct SocketBase *)SocketBase);
  res->CS_CurChr += strlen(CURRENT(res));

  return RETURN_OK;
}

LONG
rexx_sethostname(struct CSource *args, UBYTE **errstrp, struct CSource *res)
{
  char Buffer[MAXHOSTNAMELEN+1];

  if (ReadItem(Buffer, sizeof(Buffer), args) <= 0) {
    *errstrp = ERR_SYNTAX;
    return RETURN_ERROR;
  }

  sethostname(Buffer, sizeof(Buffer));
  return RETURN_OK;
}
