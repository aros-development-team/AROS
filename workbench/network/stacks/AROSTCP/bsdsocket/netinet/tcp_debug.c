/*
 * Copyright (C) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)tcp_debug.c	8.1 (Berkeley) 6/10/93
 * $Id$
 */

#ifdef TCPDEBUG
/* load symbolic names */
#define PRUREQUESTS
#define TCPSTATES
#define	TCPTIMERS
#define	TANAMES
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/errno.h>
#include <sys/queue.h>

#include <net/route.h>
#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_debug.h>
#include <sys/malloc.h>
#include <kern/amiga_includes.h>	/* AvailMem() */
#include <kern/amiga_time.h>		/* time_msofday() */
#include <net/netcore_map.h>        /* always the last include */

#ifdef TCPDEBUG
int	tcpconsdebug = 0;
#endif

/*
 * The trace is a ring of tcp_ndebug records, each a whole control block
 * and header (about 0.7 KB). It used to be an array of 100 in the stack's
 * static data - 72 KB on a 68000 that every machine paid whether or not
 * any socket ever asked for a trace. Now it does not exist until the first
 * record is written, that is until a socket with SO_DEBUG set does
 * something.
 *
 * Its length follows the memory the machine has at that moment, the same
 * for every CPU: at most 1/TCP_DEBUG_SHARE of what is free, TCP_NDEBUG_MIN
 * to TCP_NDEBUG records (the old 100 from about 4.5 MB free on a 68000).
 * It comes from the stack's pool and goes back with it when the stack
 * ends. If it cannot be had, there is no trace - nothing else changes -
 * and it is not asked for again in this run.
 *
 * tcp_debug, tcp_ndebug and tcp_debx are what a reader of the trace needs:
 * the ring, its length and the next slot to be written.
 */
#define	TCP_NDEBUG		100	/* what it always was, when memory allows */
#define	TCP_NDEBUG_MIN		8
#define	TCP_DEBUG_SHARE		64

struct	tcp_debug *tcp_debug = NULL;
int	tcp_ndebug = 0;
int	tcp_debx = 0;
static int tcp_debug_unavail = 0;

static int
tcp_debug_alloc(void)
{
    u_long n;

    if(tcp_debug_unavail)
        return 0;
    n = (u_long)AvailMem(MEMF_ANY) / TCP_DEBUG_SHARE / sizeof(struct tcp_debug);
    if(n < TCP_NDEBUG_MIN)
        n = TCP_NDEBUG_MIN;
    if(n > TCP_NDEBUG)
        n = TCP_NDEBUG;
    tcp_debug = (struct tcp_debug *)
                bsd_malloc(n * sizeof(struct tcp_debug), M_TEMP, M_NOWAIT);
    if(tcp_debug == NULL) {
        tcp_debug_unavail = 1;
        return 0;
    }
    /* a reader must not take what the pool left there for records */
    bzero((caddr_t)tcp_debug, n * sizeof(struct tcp_debug));
    tcp_debx = 0;
    tcp_ndebug = n;
    return 1;
}

/*
 * TCP stops (tcp_deinit()): the trace buffer, if one was ever allocated,
 * goes back to the pool. A program that found it by name and reads it in
 * place (FindKernelVar("_tcp_debug")) has no way to be told - the same is
 * true of every block a module hands out that way, and no program of the
 * tree looks this one up.
 */
void
tcp_debug_free(void)
{
    if(tcp_debug != NULL)
        bsd_free((caddr_t)tcp_debug, M_TEMP);
    tcp_debug = NULL;
    tcp_ndebug = 0;
    tcp_debx = 0;
    tcp_debug_unavail = 0;
}

/*
 * Tcp debug routines
 */
void
tcp_trace(act, ostate, tp, ti, req)
short act, ostate;
struct tcpcb *tp;
struct tcpiphdr *ti;
int req;
{
    __unused tcp_seq seq, ack;
    __unused int len, flags;
    struct tcp_debug *td;

    if(tcp_debug == NULL && !tcp_debug_alloc())
        return;
    td = &tcp_debug[tcp_debx++];
    if(tcp_debx >= tcp_ndebug)
        tcp_debx = 0;
    td->td_time = htonl(time_msofday());   /* as iptime(): ms since midnight UT */
    td->td_act = act;
    td->td_ostate = ostate;
    td->td_tcb = (caddr_t)tp;
    if(tp)
        td->td_cb = *tp;
    else
        bzero((caddr_t)&td->td_cb, sizeof(*tp));
    if(ti)
        td->td_ti = *ti;
    else
        bzero((caddr_t)&td->td_ti, sizeof(*ti));
    td->td_req = req;
#ifdef TCPDEBUG
    if(tcpconsdebug == 0)
        return;
    if(tp)
        printf("%p %s:", tp, tcpstates[ostate]);
    else
        printf("???????? ");
    printf("%s ", tanames[act]);
    switch(act) {

    case TA_INPUT:
    case TA_OUTPUT:
    case TA_DROP:
        if(ti == 0)
            break;
        seq = ti->ti_seq;
        ack = ti->ti_ack;
        len = ti->ti_len;
        if(act == TA_OUTPUT) {
            seq = ntohl(seq);
            ack = ntohl(ack);
            len = ntohs((u_short)len);
        }
        if(act == TA_OUTPUT)
            len -= sizeof(struct tcphdr);
        if(len)
            printf("[%x..%x)", seq, seq + len);
        else
            printf("%x", seq);
        printf("@%x, urp=%x", ack, ti->ti_urp);
        flags = ti->ti_flags;
        if(flags) {
            char *cp = "<";
#define pf(f) {					\
	if (ti->ti_flags & TH_##f) {		\
		printf("%s%s", cp, #f);		\
		cp = ",";			\
	}					\
}
            pf(SYN);
            pf(ACK);
            pf(FIN);
            pf(RST);
            pf(PUSH);
            pf(URG);
            printf(">");
        }
        break;

    case TA_USER:
        printf("%s", prurequests[req & 0xff]);
        if((req & 0xff) == PRU_SLOWTIMO)
            printf("<%s>", tcptimers[req >> 8]);
        break;
    }
    if(tp)
        printf(" -> %s", tcpstates[tp->t_state]);
    /* print out internal state of tp !?! */
    printf("\n");
    if(tp == 0)
        return;
    printf("\trcv_(nxt,wnd,up) (%x,%x,%x) snd_(una,nxt,max) (%x,%x,%x)\n",
           tp->rcv_nxt, tp->rcv_wnd, tp->rcv_up, tp->snd_una, tp->snd_nxt,
           tp->snd_max);
    printf("\tsnd_(wl1,wl2,wnd) (%x,%x,%x)\n",
           tp->snd_wl1, tp->snd_wl2, tp->snd_wnd);
#endif /* TCPDEBUG */
}
