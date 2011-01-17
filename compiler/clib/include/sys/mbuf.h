/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon 
 * the rights to redistribute these changes.
 */

/*
 * Copyright (c) 1982, 1986, 1988 Regents of the University of California.
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
 *	@(#)mbuf.h	7.14 (Berkeley) 12/5/90
 */

#ifndef SYS_MBUF_H
#define SYS_MBUF_H

#ifndef SYS_MALLOC_H
#include <sys/malloc.h>
#endif

#if 0				/* not needed (yet), DO NOT DELETE! */
extern void mcl_free_routine();
#endif
extern struct mbuf *mfree;

/*
 * Mbufs are of a single size, MSIZE, which
 * includes overhead.  An mbuf may add a single "mbuf cluster" of size
 * mbconf.mclbytes (see struct mbconf), which has no additional overhead
 * and is used instead of the internal data area; this is done when
 * at least MINCLSIZE of data must be stored.
 */

#define	MSIZE		256			/* size of an mbuf */
#define	MLEN		(MSIZE - sizeof(struct m_hdr))	/* normal data len */
#define	MHLEN		(MLEN - sizeof(struct pkthdr))	/* data len w/pkthdr */

#define	MINCLSIZE	(MHLEN + MLEN) /* smallest amount to put in cluster */
#define	M_MAXCOMPRESS	(MHLEN / 2)    /* max amount to copy for compression */

/*
 * Mbuf cluster structure. The first word is used as a reference count when the
 * cluster is in use and as a next pointer if on the free list.
 */
struct mcluster {
	union {
		struct mcluster *mcl_next;
		short            mcl_refcnt;	/* reference count */
	} mcl;
	char	mcl_buf[0];	/* variable size (mbconf.mclbytes) */
};

/*
 * Macros for type conversion
 * mtod(m,t) -	convert mbuf pointer to data pointer of correct type
 * dtom(x) -	convert data pointer within mbuf to mbuf pointer (XXX)
 *              WARNING: cannot be used with clusters
 */
#define mtod(m,t)	((t)((m)->m_data))
#define	dtom(x)		((struct mbuf *)((int)(x) & ~(MSIZE-1)))

/* header at beginning of each mbuf: */
struct m_hdr {
	struct	mbuf *mh_next;		/* next buffer in chain */
	struct	mbuf *mh_nextpkt;	/* next chain in queue/record */
	int	mh_len;			/* amount of data in this mbuf */
	caddr_t	mh_data;		/* location of data */
	short	mh_type;		/* type of data in this mbuf */
	short	mh_flags;		/* flags; see below */
};

/* record/packet header in first mbuf of chain; valid if M_PKTHDR set */
struct	pkthdr {
	int	len;		/* total packet length */
	struct	ifnet *rcvif;	/* rcv interface */
	/* variables for ip and tcp reassembly */
	caddr_t header;                 /* pointer to packet header */	
};

/* description of external storage mapped into mbuf, valid if M_EXT set */
struct m_ext {
	struct mcluster *ext_buf;	/* external buffer */
#if 0					/* Not used in AmiTCP/IP */
	void	(*ext_free)();		/* free routine if not the usual */
#endif
	u_int	ext_size;		/* size of buffer, for ext_free */
};

struct mbuf {
	struct	m_hdr m_hdr;
	union {
		struct {
			struct	pkthdr MH_pkthdr;	/* M_PKTHDR set */
			union {
				struct	m_ext MH_ext;	/* M_EXT set */
				char	MH_databuf[MHLEN];
			} MH_dat;
		} MH;
		char	M_databuf[MLEN];		/* !M_PKTHDR, !M_EXT */
	} M_dat;
};
#define	m_next		m_hdr.mh_next
#define	m_len		m_hdr.mh_len
#define	m_data		m_hdr.mh_data
#define	m_type		m_hdr.mh_type
#define	m_flags		m_hdr.mh_flags
#define	m_nextpkt	m_hdr.mh_nextpkt
#define	m_act		m_nextpkt
#define	m_pkthdr	M_dat.MH.MH_pkthdr
#define	m_ext		M_dat.MH.MH_dat.MH_ext
#define	m_pktdat	M_dat.MH.MH_dat.MH_databuf
#define	m_dat		M_dat.M_databuf

/* mbuf flags */
/*
 * The M_EOR flag is not used by the TCP/IP protocols, so it is left
 * undefined, unless you define USE_M_EOR.
 */
#define	M_EXT		0x0001	/* has associated external storage */
#define	M_PKTHDR	0x0002	/* start of record */
#ifdef USE_M_EOR
#define	M_EOR		0x0004	/* end of record */
#endif
/* mbuf pkthdr flags, also in m_flags */
#define	M_BCAST		0x0100	/* send/received as link-level broadcast */
#define	M_MCAST		0x0200	/* send/received as link-level multicast */

/* flags copied when copying m_pkthdr */
#ifdef USE_M_EOR
#define	M_COPYFLAGS	(M_PKTHDR|M_EOR|M_BCAST|M_MCAST)
#else
#define	M_COPYFLAGS	(M_PKTHDR|M_BCAST|M_MCAST)
#endif


/* mbuf types */
#define	MT_FREE		0	/* should be on free list */
#define	MT_DATA		1	/* dynamic (data) allocation */
#define	MT_HEADER	2	/* packet header */
#define	MT_SOCKET	3	/* socket structure */
#define	MT_PCB		4	/* protocol control block */
#define	MT_RTABLE	5	/* routing tables */
#define	MT_HTABLE	6	/* IMP host tables */
#define	MT_ATABLE	7	/* address resolution tables */
#define	MT_SONAME	8	/* socket name */
#define	MT_SOOPTS	10	/* socket options */
#define	MT_FTABLE	11	/* fragment reassembly header */
#define	MT_RIGHTS	12	/* access rights */
#define	MT_IFADDR	13	/* interface address */
#define MT_CONTROL	14	/* extra-data protocol message */
#define MT_OOBDATA	15	/* expedited data  */
#define MTCOUNT         16	/* TOTAL COUNT OF THE TYPES */

/* flags to m_get/MGET */
#define	M_DONTWAIT	M_NOWAIT
#define	M_WAIT		M_WAITOK

/*
 * mbuf allocation/deallocation macros:
 *
 *	MGET(struct mbuf *m, int canwait, int type)
 * allocates an mbuf and initializes it to contain internal data.
 *
 *	MGETHDR(struct mbuf *m, int canwait, int type)
 * allocates an mbuf and initializes it to contain a packet header
 * and internal data.
 */
#define	MGET(m, canwait, type) { \
	spl_t ms = splimp(); \
        (m) = mfree; \
	if (m) { \
		mfree = (m)->m_next; \
		(m)->m_type = (type); \
		mbstat.m_mtypes[type]++; \
		(m)->m_next = NULL; \
		(m)->m_nextpkt = NULL; \
		(m)->m_data = (m)->m_dat; \
		(m)->m_flags = 0; \
	} else \
		(m) = m_retry((canwait), (type)); \
	splx(ms); \
}

#define	MGETHDR(m, canwait, type) { \
	MGET(m, canwait, type) \
	if (m) { \
	        (m)->m_data = (m)->m_pktdat; \
	        (m)->m_flags = M_PKTHDR; \
	} \
}

/*
 * Mbuf cluster macros.
 * MCLALLOC(struct mcluster *p, int canwait) allocates an mbuf cluster.
 * MCLGET adds such clusters to a normal mbuf;
 * the flag M_EXT is set upon success.
 * MCLFREE releases a reference to a cluster allocated by MCLALLOC,
 * freeing the cluster if the reference count has reached 0.
 */

#define	MCLALLOC(p, canwait) \
	{ spl_t ms = splimp(); \
	  if (mclfree == 0) \
		(void)m_clalloc(mbconf.clusterchunk, (canwait)); \
	  if ((p) = mclfree) { \
		mbstat.m_clfree--; \
		mclfree = (p)->mcl.mcl_next; \
		(p)->mcl.mcl_refcnt = 1; \
	  } \
	  splx(ms); \
	}

#define	MCLGET(m, canwait) \
	{ MCLALLOC((m)->m_ext.ext_buf, (canwait)); \
	  if ((m)->m_ext.ext_buf != NULL) { \
		(m)->m_data = (m)->m_ext.ext_buf->mcl_buf; \
		(m)->m_flags |= M_EXT; \
		(m)->m_ext.ext_size = mbconf.mclbytes; \
/*	        (m)->m_ext.ext_free = mcl_free_routine; */ \
	  } \
	}

#define	MCLFREE(p) \
	{ spl_t ms = splimp(); \
	  if (--((p)->mcl.mcl_refcnt) == 0) { \
		(p)->mcl.mcl_next = mclfree; \
		mclfree = (p); \
		mbstat.m_clfree++; \
	  } \
	  splx(ms); \
	}

/*
 * MFREE(struct mbuf *m, struct mbuf *n)
 * Free a single mbuf and associated external storage.
 * Place the successor, if any, in n.
 */
#define	MFREE(m, n) \
	{ spl_t ms = splimp(); \
	  mbstat.m_mtypes[(m)->m_type]--; \
	  if ((m)->m_flags & M_EXT) { \
/*		if ((m)->m_ext.ext_free) */ \
/*			(*((m)->m_ext.ext_free))((m)->m_ext.ext_buf, */ \
/*			    (m)->m_ext.ext_size); */ \
/*		else */ \
			MCLFREE((m)->m_ext.ext_buf); \
	  } \
	  (n) = (m)->m_next; \
	  (m)->m_next = mfree; mfree = (m); \
	  splx(ms); \
	}

/*
 * Copy mbuf pkthdr from from to to.
 * from must have M_PKTHDR set, and to must be empty.
 */
#define	M_COPY_PKTHDR(to, from) { \
	(to)->m_pkthdr = (from)->m_pkthdr; \
	(to)->m_flags = (from)->m_flags & M_COPYFLAGS; \
	(to)->m_data = (to)->m_pktdat; \
}

/*
 * Set the m_data pointer of a newly-allocated mbuf (m_get/MGET) to place
 * an object of the specified size at the end of the mbuf, longword aligned.
 */
#define	M_ALIGN(m, len) \
	{ (m)->m_data += (MLEN - (len)) &~ (sizeof(long) - 1); }
/*
 * As above, for mbufs allocated with m_gethdr/MGETHDR
 * or initialized by M_COPY_PKTHDR.
 */
#define	MH_ALIGN(m, len) \
	{ (m)->m_data += (MHLEN - (len)) &~ (sizeof(long) - 1); }

/*
 * Compute the amount of space available
 * before the current start of data in an mbuf.
 */
#define	M_LEADINGSPACE(m) \
        ((m)->m_flags & M_EXT ? /* (m)->m_data - (m)->m_ext.ext_buf */ 0 : \
	    (m)->m_flags & M_PKTHDR ? (m)->m_data - (m)->m_pktdat : \
	    (m)->m_data - (m)->m_dat)

/*
 * Compute the amount of space available
 * after the end of data in an mbuf.
 */
#define	M_TRAILINGSPACE(m) \
	((m)->m_flags & M_EXT ? (m)->m_ext.ext_buf->mcl_buf + (m)->m_ext.ext_size - \
	    ((m)->m_data + (m)->m_len) : \
	    &(m)->m_dat[MLEN] - ((m)->m_data + (m)->m_len))

/*
 * Arrange to prepend space of size plen to mbuf m.
 * If a new mbuf must be allocated, canwait specifies whether to wait.
 * If canwait is M_DONTWAIT and allocation fails, the original mbuf chain
 * is freed and m is set to NULL.
 */
#define	M_PREPEND(m, plen, canwait) { \
	if (M_LEADINGSPACE(m) >= (plen)) { \
		(m)->m_data -= (plen); \
		(m)->m_len += (plen); \
	} else \
		(m) = m_prepend((m), (plen), (canwait)); \
	if ((m) && (m)->m_flags & M_PKTHDR) \
		(m)->m_pkthdr.len += (plen); \
}

/* change mbuf to new type */
#define MCHTYPE(m, t) { \
	mbstat.m_mtypes[(m)->m_type]--; \
	mbstat.m_mtypes[t]++; \
	(m)->m_type = t;\
}

/* length to m_copy to copy all */
#define	M_COPYALL	1000000000

/* compatiblity with 4.3 */
#define  m_copy(m, o, l)	m_copym((m), (o), (l), M_DONTWAIT)

/*
 * Configurable variables. These are put in a structure to ensure that they
 * will be in sequence, since these are accessed as an array of u_longs
 * from the <kern/amiga_config.c>.
 * NOTE: <kern/amiga_config.c> depends on the order of these values.
 */
struct mbconf {
  u_long initial_mbuf_chunks;   /* # of mbuf chunks to allocate initially */
  u_long mbufchunk;	 	/* # of mbufs to allocate at a time */
  u_long clusterchunk;		/* # of clusters to allocate at a time */
  u_long maxmem;		/* maximum memory to use (in kilobytes) */
  u_long mclbytes;		/* size of the mbuf cluster */
};

/*
 * Mbuf statistics.
 */
struct mbstat {
	u_long	m_mbufs;	/* mbufs obtained from page pool */
	u_long	m_clusters;	/* clusters obtained from page pool */
	u_long	m_clfree;	/* free clusters */
	u_long	m_drops;	/* times failed to find space */
	u_long	m_wait;		/* times waited for space */
	u_long	m_drain;	/* times drained protocols for space */
	u_long  m_memused;	/* total amount of memory used for mbufs */
	u_short	m_mtypes[MTCOUNT];	/* type specific mbuf allocations */
};

#ifdef	KERNEL
/*
 * changed definitions to external declarations, storege is now defined
 * in kern/uipc_mbuf.c
 */
extern struct mcluster *mclfree;
extern struct mbconf   mbconf;
extern struct mbstat   mbstat;
extern int             max_linkhdr;	/* largest link-level header */
extern int             max_protohdr;	/* largest protocol header */
extern int             max_hdr;		/* largest link+protocol header */
extern int             max_datalen;	/* MHLEN - max_hdr */

int mb_check_conf(void *dp, LONG newvalue);

BOOL mbinit(void);
void mbdeinit(void);
BOOL m_alloc(int howmany, int canwait);
BOOL m_clalloc(int ncl, int canwait);
struct mbuf * m_retry(int canwait, int type);
void m_reclaim(void);
struct mbuf * m_get(int canwait, int type);
struct mbuf * m_gethdr(int canwait, int type);
struct mbuf * m_getclr(int canwait, int type);
struct mbuf * m_free(struct mbuf * m);
void m_freem(struct mbuf * m);
struct mbuf * m_prepend(struct mbuf * m, int len, int canwait);
struct mbuf * m_copym(struct mbuf * m, int off0, int len, int wait);
void m_copydata(struct mbuf * m, int off, int len, caddr_t cp);
void m_cat(struct mbuf * m, struct mbuf * n);
void m_adj(struct mbuf * mp, int req_len);
struct mbuf * m_pullup(struct mbuf * n, int len);

#endif /* KERNEL */

#endif /* !SYS_MBUF_H */
