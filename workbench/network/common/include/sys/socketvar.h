/*
 * Copyright (C) 2005-2026, The AROS Development Team. All rights reserved.
 *
 * Derived from the 4.3BSD-Reno/Net-2 <sys/socketvar.h> (7.17, Berkeley, 5/5/91);
 * the Berkeley notice and terms follow. AROS changes: a socket is owned by a
 * bsdsocket library base rather than a process (so_pgid, so_refcnt, sb_sel
 * select chains, timeval linger/buffer timeouts), socket event reporting
 * (so_eventmask, struct soevent), SO_TTCP_SHUTUP, SB_MAX per architecture,
 * and the Unix file-operation prototypes are dropped.
 */

/*-
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
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
 *	@(#)socketvar.h	7.17 (Berkeley) 5/5/91
 */

#ifndef SYS_SOCKETVAR_H
#define SYS_SOCKETVAR_H

#ifndef SYS_TIME_H
#include <sys/time.h>
#endif

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

struct newselitem;

/*
 * Kernel structure per socket.
 * Contains send and receive buffer queues,
 * handle on protocol and pointer to protocol
 * private data and error information.
 */
struct socket {
	short	so_type;		/* generic type, see socket.h */
	short	so_options;		/* from socket call, see socket.h */
	struct timeval so_linger;	/* time to linger while closing */
	short	so_state;		/* internal state flags SS_*, below */
	short	so_refcnt;		/* holders keeping the socket alive across sleeps */
	caddr_t	so_pcb;			/* protocol control block */
	struct	protosw *so_proto;	/* protocol handle */
/*
 * Variables for connection queueing.
 * Socket where accepts occur is so_head in all subsidiary sockets.
 * If so_head is 0, socket is not related to an accept.
 * For head socket so_q0 queues partially completed connections,
 * while so_q is a queue of connections ready to be accepted.
 * If a connection is aborted and it has so_head set, then
 * it has to be pulled out of either so_q0 or so_q.
 * We allow connections to queue up based on current queue lengths
 * and limit on number of queued connections for this socket.
 */
	struct	socket *so_head;	/* back pointer to accept socket */
	struct	socket *so_q0;		/* queue of partial connections */
	struct	socket *so_q;		/* queue of incoming connections */
	short	so_q0len;		/* partials on so_q0 */
	short	so_qlen;		/* number of connections on so_q */
	short	so_qlimit;		/* max number queued connections */
	short	so_timeo;		/* connection timeout */
	u_short	so_error;		/* error affecting connection */
	struct	SocketBase *so_pgid;	/* owning library base; gets signals/events */
	u_long	so_oobmark;		/* chars to oob mark */
/*
 * Variables for socket buffering.
 */
	struct	sockbuf {
		u_long	sb_cc;		     /* actual chars in buffer */
		u_long	sb_hiwat;	     /* max actual char count */
		u_long	sb_mbcnt;	     /* chars of mbufs used */
		u_long	sb_mbmax;	     /* max chars of mbufs to use */
		long	sb_lowat;	     /* low water mark */
		struct	mbuf *sb_mb;	     /* the mbuf chain */
		struct	newselitem *sb_sel;  /* tasks blocked in WaitSelect() on this buffer */
		short	sb_flags;	     /* flags, see below */
		struct timeval sb_timeo;     /* timeout for read/write */
	} so_rcv, so_snd;
#if defined(__mc68000__)
#define	SB_MAX		(64*1024)	/* max chars in sockbuf (m68k) */
#else
#define	SB_MAX		(512*1024)	/* max chars in sockbuf */
#endif
#define	SB_LOCK		0x01		/* lock on data queue */
#define	SB_WANT		0x02		/* someone is waiting to lock */
#define	SB_WAIT		0x04		/* someone is waiting for data/space */
#define	SB_SEL		0x08		/* someone is selecting */
#define	SB_ASYNC	0x10		/* ASYNC I/O, need signals */
#define	SB_NOTIFY	(SB_WAIT|SB_SEL|SB_ASYNC)
#define	SB_COLL		0x20		/* collision selecting */
#define	SB_NOINTR	0x40		/* operations not interruptible */

	u_long	so_eventmask;		/* FD_* event classes to report (SO_EVENTMASK) */
	caddr_t	so_tpcb;		/* Wisc. protocol control block XXX */
};

/* A reported event, queued on the owning base's EventList until
 * GetSocketEvents() collects it.
 */
struct soevent {
	struct MinNode node;
	struct socket *socket;		/* socket the events belong to */
	u_long events;			/* FD_* classes that occurred */
};

/*
 * Socket state bits.
 */
#define	SS_NOFDREF		0x001	/* no file table ref any more */
#define	SS_ISCONNECTED		0x002	/* socket connected to a peer */
#define	SS_ISCONNECTING		0x004	/* in process of connecting to peer */
#define	SS_ISDISCONNECTING	0x008	/* in process of disconnecting */
#define	SS_CANTSENDMORE		0x010	/* can't send more data to peer */
#define	SS_CANTRCVMORE		0x020	/* can't receive more data from peer */
#define	SS_RCVATMARK		0x040	/* at mark on input */

#define	SS_PRIV			0x080	/* privileged for broadcast, raw... */
#define	SS_NBIO			0x100	/* non-blocking ops */
#define	SS_ASYNC		0x200	/* async i/o notify */
#define	SS_ISCONFIRMING		0x400	/* deciding to accept connection req */

/* so_options bit internal to the stack: TCP_NOPUSH was set, so a T/TCP
 * style sendto() on an unconnected socket reports ENOTCONN (see
 * amiga_sendrecv.c).
 */
#define SO_TTCP_SHUTUP 0x8000

/*
 * Macros for sockets and socket buffering.
 */

/*
 * How much space is there in a socket buffer (so->so_snd or so->so_rcv)?
 * This is problematical if the fields are unsigned, as the space might
 * still be negative (cc > hiwat or mbcnt > mbmax).  Should detect
 * overflow and return 0.  Should use "lmin" but it doesn't exist now.
 */
#define	sbspace(sb) \
    ((long) imin((int)((sb)->sb_hiwat - (sb)->sb_cc), \
	 (int)((sb)->sb_mbmax - (sb)->sb_mbcnt)))

/* do we have to send all at once on a socket? */
#define	sosendallatonce(so) \
    ((so)->so_proto->pr_flags & PR_ATOMIC)

/* can we read something from so? */
#define	soreadable(so) \
    ((so)->so_rcv.sb_cc >= (so)->so_rcv.sb_lowat || \
	((so)->so_state & SS_CANTRCVMORE) || \
	(so)->so_qlen || (so)->so_error)

/* can we write something to so? */
#define	sowriteable(so) \
    ((sbspace(&(so)->so_snd) >= (so)->so_snd.sb_lowat && \
      (((so)->so_state&SS_ISCONNECTED) || \
       ((so)->so_proto->pr_flags&PR_CONNREQUIRED)==0)) \
     || ((so)->so_state & SS_CANTSENDMORE) \
     || (so)->so_error)

/* adjust counters in sb reflecting allocation of m */
#define	sballoc(sb, m) { \
	(sb)->sb_cc += (m)->m_len; \
	(sb)->sb_mbcnt += MSIZE; \
	if ((m)->m_flags & M_EXT) \
		(sb)->sb_mbcnt += (m)->m_ext.ext_size; \
}

/* adjust counters in sb reflecting freeing of m */
#define	sbfree(sb, m) { \
	(sb)->sb_cc -= (m)->m_len; \
	(sb)->sb_mbcnt -= MSIZE; \
	if ((m)->m_flags & M_EXT) \
		(sb)->sb_mbcnt -= (m)->m_ext.ext_size; \
}

/*
 * Set lock on sockbuf sb; sleep if lock is already held.
 * Unless SB_NOINTR is set on sockbuf, sleep is interruptible.
 * Returns error without lock if sleep is interrupted.
 */
#define sblock(sb, p) ((sb)->sb_flags & SB_LOCK ? sb_lock(sb, p) : \
		((sb)->sb_flags |= SB_LOCK, 0))

/* release lock on sockbuf sb */
#define	sbunlock(sb) { \
	(sb)->sb_flags &= ~SB_LOCK; \
	if ((sb)->sb_flags & SB_WANT) { \
		(sb)->sb_flags &= ~SB_WANT; \
		wakeup((caddr_t)&(sb)->sb_flags); \
	} \
}
#ifdef KERNEL
extern u_long	sb_max;

/* strings for sleep message: */
extern	char netio[], netcon[], netcls[];

#endif
#endif /* !SYS_SOCKETVAR_H */
