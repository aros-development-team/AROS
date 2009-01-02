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
 * Copyright (c) 1982, 1986, 1988, 1991 Regents of the University of California.
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
 *	@(#)uipc_mbuf.c	7.19 (Berkeley) 4/20/91
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/synch.h>

#include <kern/amiga_includes.h>

#include <dos/rdargs.h>

/*
 * Configuration information.
 */
struct mbconf mbconf = {
  2,		                /* # of mbuf chunks to allocate initially */
//8,
  64,				/* # of mbufs to allocate at a time */
//256,
  4,				/* # of clusters to allocate at a time */
//256,				/* maximum memory to use (in kilobytes) */
  1024,
  2048				/* size of the mbuf cluster */
};

/*
 * List of free mbufs. Access to this list is protected by splimp()
 */
struct mbuf *mfree = NULL;

struct	mbstat mbstat = { 0 };

struct	mcluster *mclfree = NULL;

int	max_linkhdr = 0;		/* largest link-level header */
int	max_protohdr = 0;		/* largest protocol header */
int	max_hdr = 0;			/* largest link+protocol header */
int	max_datalen = 0;		/* MHLEN - max_hdr */

/*
 * Header structure that is placed at the start of every allocated memory 
 * region to be freed on deinit. All memory alloctions are thus 
 * sizeof(memHeader) larger and the data pointer is set past this header
 * before used. These headers are linked together and the mbufmem pointer 
 * holds the pointer to the start of the list.
 */
struct memHeader {
  struct memHeader *next;
  ULONG             size;
};

static struct memHeader *mbufmem = NULL;

static BOOL initialized = FALSE;

LONG mb_read_stats(struct CSource *args, UBYTE **errstrp, struct CSource *res)
{
  int i, total = 0;
  UBYTE *p = res->CS_Buffer;
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) mb_read_stats()\n"));
#endif

  for(i = 0; i < MTCOUNT; i++) {
    p += sprintf(p, "%ld ", mbstat.m_mtypes[i]);
    total += mbstat.m_mtypes[i];
  }
  p += sprintf(p, "%ld", total);

#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) mb_read_stats: %s\n", res->CS_Buffer));
#endif

  res->CS_CurChr = p - res->CS_Buffer;
  return RETURN_OK;
}

int 
mb_check_conf(void *dp, LONG newvalue)
{
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) mb_check_conf(0x%08x, %d)\n", dp, newvalue));
#endif

  if ((u_long *)dp == &mbconf.initial_mbuf_chunks) {
    if (newvalue > 0)
      return TRUE;
  }
  else 
  if (dp == &mbconf.mbufchunk) {
    if (newvalue >= 32)
      return TRUE;
  }
  else 
  if (dp == &mbconf.clusterchunk) {
    if (newvalue > 0)
      return TRUE;
  }
  else 
  if (dp == &mbconf.maxmem) {
    if (newvalue > 32)		/* kilobytes */
      return TRUE;
  }
  else 
  if (dp == &mbconf.mclbytes) {
    if (newvalue >= MINCLSIZE)
      return TRUE;
  }
  
  return FALSE;
} 

/*
 * mbinit() must be called before any other mbuf related function (exept the
 * mb_check_conf() which is called at configuration time). This
 * allocates memory from the system in one big chunk. This memory will not be
 * freed until AMITCP/IP is shut down.
 */

BOOL
mbinit(void)
{
  spl_t s;
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) mbinit()\n"));
#endif

  /*
   * Return success if already initialized
   */
  if (initialized)
    return TRUE;

  s = splimp();
  /*
   * Initialize the list headers to NULL
   */
  mfree = NULL;
  mclfree = NULL;

  /*
   * Preallocate some mbufs and mbuf clusters.
   */
  initialized = 
    (m_alloc(mbconf.initial_mbuf_chunks * mbconf.mbufchunk, M_WAIT)
     && m_clalloc(mbconf.clusterchunk, M_WAIT));

  splx(s);
  
  if (!initialized) {
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) mbinit: Failed to allocate memory!\n"));
#endif
    __log(LOG_ERR, "mbinit: Failed to allocate memory.");
    mbdeinit();
  }
  return (initialized);
}

/*
 * Free all memory allocated by mbuf subsystem. This must be the last mbuf
 * related function called. (Implying that NO mbuf allocations should be done
 * concurrently with this!)
 *
 * This is new function to AMITCP/IP.
 */
void
mbdeinit(void)
{
  struct memHeader *next;
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) mbdeinit()\n"));
#endif

  /*
   * free all memory chunks
   */
  while (mbufmem) {
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) mbdeinit: Freeing %d bytes @ 0x%08x\n", mbufmem, mbufmem->size));
#endif
    next = mbufmem->next;
    mbstat.m_memused -= mbufmem->size;
    FreeMem(mbufmem, mbufmem->size);
    mbufmem = next;
  }
  initialized = FALSE;
}

/*
 * Allocate memory for mbufs.
 * and place on the mbuf free list.
 * The canwait argument is currently ignored.
 *
 * MUST be called at splimp!
 */
BOOL
m_alloc(int howmany, int canwait)
{
 /*
  * Note that mbufs must be aligned on MSIZE boundary
  * for dtom to work correctly. This is archieved by allocating size for one 
  * additional mbuf per chunk so that given memory can be aligned properly.
  */ 
  struct mbuf *m;
  struct memHeader *mh;
  ULONG  size;
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_alloc()\n"));
#endif

  size = MSIZE * (howmany + 1) + sizeof(struct memHeader);

  /*
   * check if allowed to allocate more
   */
  if (mbstat.m_memused + size > mbconf.maxmem * 1024) {
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_alloc: max amount of memory already used (%ld bytes).\n",
	mbstat.m_memused));
#endif
    __log(LOG_ERR, "m_alloc: max amount of memory already used (%ld bytes).",
	mbstat.m_memused);
    return FALSE;
  }

  mh = AllocMem(size, MEMF_PUBLIC);	/* public since used from interrupts */
  if (mh == NULL) {
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_alloc: Cannot allocate memory for mbufs\n"));
#endif
    __log(LOG_ERR, "m_alloc: Cannot allocate memory for mbufs.");
    return FALSE;
  }

  /*
   * initialize the memHeader and link it to the chain of allocated memory 
   * blocks
   */
  mbstat.m_memused += size;		/* add to the total */
  mh->size = size;
  mh->next = mbufmem;
  mbufmem = mh;
  mh++;				/* pass by the memHeader */

  /*
   * update the statistics
   */
  mbstat.m_mbufs += howmany;

  /*
   * link mbufs into the free list
   */
  m = dtom(((caddr_t)mh) + MSIZE - 1); /* correctly aligned mbuf pointer */
  while(howmany--) {
    m->m_next = mfree;
    mfree = m++;
  }
  return TRUE;
}  

/*
 * Allocate some number of mbuf clusters
 * and place on cluster free list.
 * The canwait argument is currently ignored.
 * MUST be called at splimp.
 */
BOOL
m_clalloc(int ncl, int canwait)
{
  struct memHeader *mh;
  struct mcluster *p;
  ULONG  size;
  short  i;
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_clalloc()\n"));
#endif

  /*
   * struct mcluster has variable length buffer so its size is not calculated
   * in sizeof(struct mcluster). The size of the buffer is mbconf.mclbytes.
   * Each memory block allocated is prepended by the memHeader, so size
   * must be allocted for it, too.
   */
  size = ncl * (sizeof(struct mcluster) + mbconf.mclbytes)
    + sizeof(struct memHeader);

  /*
   * check if allowed to allocate more
   */
  if (mbstat.m_memused + size > mbconf.maxmem * 1024) {
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_clalloc: max amount of memory already used (%ld bytes).\n",
	mbstat.m_memused));
#endif
    __log(LOG_ERR, "m_clalloc: max amount of memory already used (%ld bytes).",
	mbstat.m_memused);
    return FALSE;
  }

  mh = AllocMem(size, MEMF_PUBLIC); /* public since used from interrupts */
  if (mh == NULL) {
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_clalloc: Cannot allocate memory for mbuf clusters\n"));
#endif
    __log(LOG_ERR, "m_clalloc: Cannot allocate memory for mbuf clusters");
    return FALSE;
  }
  /*
   * initialize the memHeader and link it to the chain of allocated memory 
   * blocks
   */
  mbstat.m_memused += size;
  mh->size = size;
  mh->next = mbufmem;
  mbufmem = mh;
  mh++;				/* pass by the memHeader */
  /*
   * link clusters to the free list
   */
  for (i = 0, p = (struct mcluster *)mh; 
       i < ncl; 
       i++, p = (struct mcluster*)((char *)(p + 1) + mbconf.mclbytes)) {
    p->mcl.mcl_next = mclfree;
    mclfree = p;
    mbstat.m_clfree++;
  }
  mbstat.m_clusters += ncl;
  
  return TRUE;
}

/*
 * When MGET failes, ask protocols to free space when short of memory,
 * then re-attempt to allocate an mbuf.
 *
 * Allocate more memory for mbufs if there still are no mbufs left 
 *
 * MUST be called at splimp.
 */
struct mbuf *
m_retry(int canwait, int type)
{
  register struct mbuf *m;
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_retry()\n"));
#endif

  m_reclaim();

  /*
   * Try to allocate more memory if still no free mbufs
   */
  if (!mfree)
    m_alloc(mbconf.mbufchunk, canwait);
  
#define m_retry(i, t)	/*mbstat.m_drops++,*/NULL
  MGET(m, canwait, type);
#undef m_retry
  return (m);
}

void
m_reclaim()
{
	register struct domain *dp;
	register struct protosw *pr;
	spl_t s = splimp();
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_reclaim()\n"));
#endif

	for (dp = domains; dp; dp = dp->dom_next)
		for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_drain)
				(*pr->pr_drain)();
	splx(s);
	mbstat.m_drain++;
}

/*
 * Space allocation routines.
 * These are also available as macros
 * for critical paths.
 */
struct mbuf *
m_get(canwait, type)
	int canwait, type;
{
	register struct mbuf *m;
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_get(canwait:%d, type:%d)\n", canwait, type));
#endif

	MGET(m, canwait, type);
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_get: returning 0x%08x\n", m));
#endif

	return (m);
}

struct mbuf *
m_gethdr(canwait, type)
	int canwait, type;
{
	register struct mbuf *m;
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_gethdr()\n"));
#endif

	MGETHDR(m, canwait, type);
	return (m);
}

struct mbuf *
m_getclr(canwait, type)
	int canwait, type;
{
	register struct mbuf *m;
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_getclr()\n"));
#endif

	MGET(m, canwait, type);
	if (m == 0)
		return (0);
	aligned_bzero_const(mtod(m, caddr_t), MLEN);
	return (m);
}

struct mbuf *
m_free(m)
	struct mbuf *m;
{
	register struct mbuf *n;
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_free()\n"));
#endif

	MFREE(m, n);
	return (n);
}

void
m_freem(m)
	register struct mbuf *m;
{
	register struct mbuf *n;
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_freem(0x%08x)\n", m));
#endif

	if (m == NULL)
	{
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_freem: Attempting to free NOTHING!!\n"));
#endif
			return;
	}
	do {
		MFREE(m, n);
	} while (m = n);
}

/*
 * Mbuffer utility routines.
 */

/*
 * Lesser-used path for M_PREPEND:
 * allocate new mbuf to prepend to chain,
 * copy junk along.
 */
struct mbuf *
m_prepend(m, len, canwait)
	register struct mbuf *m;
	int len, canwait;
{
	struct mbuf *mn;
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_prepend(0x%08x, len = %d)\n", m, len));
#endif

	MGET(mn, canwait, m->m_type);
	if (mn == NULL) {
		m_freem(m);
		return (NULL);
	}
	if (m->m_flags & M_PKTHDR) {
		M_COPY_PKTHDR(mn, m);
		m->m_flags &= ~M_PKTHDR;
	}
	mn->m_next = m;
	m = mn;
	if (len < MHLEN)
		MH_ALIGN(m, len);
	m->m_len = len;
	return (m);
}

/*
 * Make a copy of an mbuf chain starting "off0" bytes from the beginning,
 * continuing for "len" bytes.  If len is M_COPYALL, copy to end of mbuf.
 * The wait parameter is a choice of M_WAIT/M_DONTWAIT from caller.
 */
int MCFail;

struct mbuf *
m_copym(m, off0, len, wait)
	register struct mbuf *m;
	int off0, wait;
	register int len;
{
	register struct mbuf *n, **np;
	register int off = off0;
	struct mbuf *top = NULL;
	int copyhdr = 0;
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_copym(0x%08x, len = %d)\n", m, len));
#endif

	if (off < 0 || len < 0) {
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_copym: bad args\n"));
#endif
	  __log(LOG_ERR, "m_copym: Bad arguments");
	  goto nospace;
	}
	if (off == 0 && m->m_flags & M_PKTHDR)
		copyhdr = 1;
	/*
	 * find first mbuf to copy data from
	 */
	while (off > 0) {
		if (m == 0) {
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_copym: short mbuf chain!\n"));
#endif
		  __log(LOG_ERR, "m_copym: short mbuf chain");
		  goto nospace;
		}
		if (off < m->m_len)
			break;
		off -= m->m_len;
		m = m->m_next;
	}
	np = &top;
	while (len > 0) {
		if (m == 0) {
			if (len != M_COPYALL) {
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_copym: short mbuf chain!!\n"));
#endif
			  __log(LOG_ERR, "m_copym: short mbuf chain");
			  goto nospace;
			}
			break;
		}
		MGET(n, wait, m->m_type);
		*np = n;
		if (n == 0)
			goto nospace;
		if (copyhdr) {
			M_COPY_PKTHDR(n, m);
			if (len == M_COPYALL)
				n->m_pkthdr.len -= off0;
			else
				n->m_pkthdr.len = len;
			copyhdr = 0;
		}
		n->m_len = MIN(len, m->m_len - off);

		if (m->m_flags & M_EXT) {
			n->m_data = m->m_data + off;
			m->m_ext.ext_buf->mcl.mcl_refcnt++;
			n->m_ext = m->m_ext;
			n->m_flags |= M_EXT;
		} else
			bcopy(mtod(m, caddr_t)+off, mtod(n, caddr_t),
			    (unsigned)n->m_len);
		if (len != M_COPYALL)
			len -= n->m_len;
		off = 0;
		m = m->m_next;
		np = &n->m_next;
	}
	if (top == 0)
		MCFail++;
	return (top);
nospace:
	m_freem(top);
	MCFail++;
	return NULL;
}

/*
 * Copy data from an mbuf chain starting "off" bytes from the beginning,
 * continuing for "len" bytes, into the indicated buffer.
 */
void
m_copydata(m, off, len, cp)
	register struct mbuf *m;
	register int off;
	register int len;
	caddr_t cp;
{
	register unsigned count;
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_copydata(0x%08x, len = %d)\n", m, len));
#endif

	if (off < 0 || len < 0) {
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_copydata: bad arguments!\n"));
#endif
	  __log(LOG_ERR, "m_copydata: bad arguments");
	  return;
	}
	while (off > 0) {
		if (m == 0) {
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_copydata: short mbuf chain to copy from!\n"));
#endif
		  __log(LOG_ERR, "m_copydata: short mbuf chain to copy from");
		  return;
		}
		if (off < m->m_len)
			break;
		off -= m->m_len;
		m = m->m_next;
	}
	while (len > 0) {
		if (m == 0) {
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_copydata: short mbuf chain to copy from!!\n"));
#endif
		  __log(LOG_ERR, "m_copydata: short mbuf chain to copy from");
		  return;
		}
		count = MIN(m->m_len - off, len);
		bcopy(mtod(m, caddr_t) + off, cp, count);
		len -= count;
		cp += count;
		off = 0;
		m = m->m_next;
	}
}

/*
 * Concatenate mbuf chain n to m.
 * Both chains must be of the same type (e.g. MT_DATA).
 * Any m_pkthdr is not updated.
 */
void
m_cat(m, n)
	register struct mbuf *m, *n;
{
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_cat(0x%08x, 0x%08x)\n", m, n));
#endif

	while (m->m_next)
		m = m->m_next;
	while (n) {
		if (m->m_flags & M_EXT ||
		    m->m_data + m->m_len + n->m_len >= &m->m_dat[MLEN]) {
			/* just join the two chains */
			m->m_next = n;
			return;
		}
		/* splat the data from one into the other */
		bcopy(mtod(n, caddr_t), mtod(m, caddr_t) + m->m_len,
		    (u_int)n->m_len);
		m->m_len += n->m_len;
		n = m_free(n);
	}
}

void
m_adj(struct mbuf *mp, int req_len)
{
	register int len = req_len;
	register struct mbuf *m;
	register count;
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_adj()\n"));
#endif

	if ((m = mp) == NULL)
		return;
	if (len >= 0) {
		/*
		 * Trim from head.
		 */
		while (m != NULL && len > 0) {
			if (m->m_len <= len) {
				len -= m->m_len;
				m->m_len = 0;
				m = m->m_next;
			} else {
				m->m_len -= len;
				m->m_data += len;
				len = 0;
			}
		}
		m = mp;
		if (mp->m_flags & M_PKTHDR)
			m->m_pkthdr.len -= (req_len - len);
	} else {
		/*
		 * Trim from tail.  Scan the mbuf chain,
		 * calculating its length and finding the last mbuf.
		 * If the adjustment only affects this mbuf, then just
		 * adjust and return.  Otherwise, rescan and truncate
		 * after the remaining size.
		 */
		len = -len;
		count = 0;
		for (;;) {
			count += m->m_len;
			if (m->m_next == (struct mbuf *)0)
				break;
			m = m->m_next;
		}
		if (m->m_len >= len) {
			m->m_len -= len;
			if ((mp = m)->m_flags & M_PKTHDR)
				m->m_pkthdr.len -= len;
			return;
		}
		count -= len;
		if (count < 0)
			count = 0;
		/*
		 * Correct length for chain is "count".
		 * Find the mbuf with last data, adjust its length,
		 * and toss data from remaining mbufs on chain.
		 */
		m = mp;
		if (m->m_flags & M_PKTHDR)
			m->m_pkthdr.len = count;
		for (; m; m = m->m_next) {
			if (m->m_len >= count) {
				m->m_len = count;
				break;
			}
			count -= m->m_len;
		}
		while (m = m->m_next)
			m->m_len = 0;
	}
}

/*
 * Rearrange an mbuf chain so that len bytes from the beginning are
 * contiguous and in the data area of an mbuf (so that mtod and dtom
 * will work for a structure of size len). Note that resulting
 * structure is assumed to get properly aligned. This will happen only if
 * there is no odd-length data before the structure. Fortunately all
 * headers are before any data in the packet and are of even length.
 * Returns the resulting mbuf chain on success, frees it and returns
 * null on failure. If there is room, it will add up to max_protohdr-len
 * extra bytes to the contiguous region in an attempt to avoid being
 * called next time.
 */
int MPFail;

struct mbuf *
m_pullup(n, len)
	register struct mbuf *n;
	int len;
{
	register struct mbuf *m;
	register int count;
	int space;
#if defined(__AROS__)
D(bug("[AROSTCP](uipc_mbuf.c) m_pullup()\n"));
#endif

	/*
	 * If first mbuf has no cluster, and has room for len bytes
	 * without shifting current data, pullup into it,
	 * otherwise allocate a new mbuf to prepend to the chain.
	 */
	if ((n->m_flags & M_EXT) == 0 &&
	    n->m_data + len < &n->m_dat[MLEN] && n->m_next) {
		if (n->m_len >= len)
			return (n);
		m = n;			/* pullup to */
		n = n->m_next;		/* pullup from */
		len -= m->m_len; 	/* pullup length */
	} else {
		if (len > MHLEN)
			goto bad;
		MGET(m, M_DONTWAIT, n->m_type);
		if (m == 0)
			goto bad;
		m->m_len = 0;
		if (n->m_flags & M_PKTHDR) {
			M_COPY_PKTHDR(m, n);
			n->m_flags &= ~M_PKTHDR;
		}
	}
	space = &m->m_dat[MLEN] - (m->m_data + m->m_len);
	do {
		count = MIN(MIN(MAX(len, max_protohdr), space), n->m_len);
		bcopy(mtod(n, caddr_t), mtod(m, caddr_t) + m->m_len,
		  (unsigned)count);
		len -= count;
		m->m_len += count;
		n->m_len -= count;
		space -= count;
		if (n->m_len)
			n->m_data += count;
		else
			n = m_free(n);
	} while (len > 0 && n);
	if (len > 0) {
		(void) m_free(m);
		goto bad;
	}
	m->m_next = n;
	return (m);
bad:
	m_freem(n);
	MPFail++;
	return (0);
}

#if 0				/* not needed (yet), DO NOT DELETE! */
/*
 * Allocate a "funny" mbuf, that is, one whose data is owned by someone else.
 */
struct mbuf *
mclgetx(fun, arg, addr, len, wait)
        void (*fun)();
        int arg, len, wait;
        caddr_t addr;
{
        register struct mbuf *m;

        MGETHDR(m, wait, MT_DATA);
        if (m == 0)
                return (0);
        m->m_data = addr ;
        m->m_len = len;
        m->m_ext.ext_free = fun;
        m->m_ext.ext_size = len;
        m->m_ext.ext_buf = (caddr_t)arg;
        m->m_flags |= M_EXT;

        return (m);
}

void mcl_free_routine(buf, size)
    char *buf;
    int size;
{
}
#endif /* 0 */
