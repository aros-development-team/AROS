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

#include <conf.h>

#include <aros/asmcall.h>
#include <aros/libcall.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socketvar.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/protosw.h>
#include <sys/malloc.h>
#include <sys/synch.h>

#include <sys/time.h>
#include <sys/errno.h>

#include <sys/socket.h>
#include <net/route.h>

#include <kern/amiga_includes.h>

#include <api/amiga_api.h>
#include <api/amiga_libcallentry.h>
#include <api/allocdatabuffer.h>

#include <api/apicalls.h>

#include <net/if_protos.h>

#include <bsdsocket/socketbasetags.h>

#include <kern/uipc_domain_protos.h>
#include <kern/uipc_socket_protos.h>
#include <kern/uipc_socket2_protos.h>

/* Local protos */
static int selscan(struct SocketBase *p, 
		   fd_set *in, fd_set *ou, fd_set *ex, fd_mask *obits,
		   int nfd, int *retval, int *selitemcount_p);
void selenter(struct SocketBase * p, struct newselitem ** hdr);
static void unselect(register struct newselbuf * sb);
void selwakeup(struct newselitem **hdr);
static int soo_select(struct socket *so, int which, struct SocketBase *p);
static int countSockets(struct SocketBase * libPtr, struct socket * so);

/*
 * itimerfix copied from bsdss/server/kern/kern_time.c. since fields
 * in struct timeval in amiga are ULONGs, values less than zero need
 * not be checked. the second check, timeval less than resolution of
 * the clock is not needed in amiga...hmm, is removed also.
 */
static inline int itimerfix(struct timeval *tv)
{
	if (tv->tv_sec > 100000000 || tv->tv_usec >= 1000000)
		return (EINVAL);
/*	if (tv->tv_sec == 0 && tv->tv_usec != 0 && tv->tv_usec < tick)
		tv->tv_usec = tick;
*/	return (0);
}

/*
 * ffs() copied directly from bsdss/server/kern/subr_xxx.c
 */
static inline int ffs(register long mask)
{
  register int bit;

  if (!mask)
    return(0);
  for (bit = 1;; ++bit) {
    if (mask&0x01)
      return(bit);
    mask >>= 1;
  }
  /* NOT REACHED */
}


/*
 * Ioctl system call
 */

LONG __IoctlSocket(LONG fdes, ULONG cmd, caddr_t data, struct SocketBase *libPtr)
{
  register int error;
  register u_int size;
  struct socket *so;

  /*
   * Note: Syscall semaphore is essential here if two processes can access
   * the same socket. (can it be changed to spl_? ?)
   */
  CHECK_TASK();
  DSYSCALLS(log(LOG_DEBUG,"IoctlSocket(%ld, 0x%08lx, 0x%08lx) called", fdes, cmd, *(ULONG *)data);)
  ObtainSyscallSemaphore(libPtr);
  
  if (error = getSock(libPtr, fdes, &so))
    goto Return;

  /*
   * Interpret high order word to get size of the user data area
   */
  size = IOCPARM_LEN(cmd);
  if (size == 0 || size > IOCPARM_MAX) {
    error = ENOTTY;
    goto Return;
  }
  if (!(cmd & IOC_IN) && cmd & IOC_OUT)
    /*
     * Zero the buffer so the user always
     * gets back something deterministic.
     */
    bzero(data, size);
  
  switch (cmd) {
  case FIOCLEX:
  case FIONCLEX:		/* should this return error ???? */
    goto Return;

  case FIOSETOWN:
  case SIOCSPGRP:
    /*
     * data is a struct Task **, find corresponding SocketBase * and set owner
     */
    so->so_pgid = FindSocketBase(*(struct Task **)data);
    goto Return;
    
  case FIOGETOWN:
  case SIOCGPGRP:
    if (so->so_pgid)
      *(struct Task **)data = so->so_pgid->thisTask;
    else
      *(struct Task **)data = NULL;
    goto Return;

  case FIONBIO:
    if (*(int *)data)
      so->so_state |= SS_NBIO;
    else
      so->so_state &= ~SS_NBIO;
    goto Return;
    
  case FIOASYNC:
    if (*(int *)data) {
      so->so_state |= SS_ASYNC;
      so->so_rcv.sb_flags |= SB_ASYNC;
      so->so_snd.sb_flags |= SB_ASYNC;
    } else {
      so->so_state &= ~SS_ASYNC;
      so->so_rcv.sb_flags &= ~SB_ASYNC;
      so->so_snd.sb_flags &= ~SB_ASYNC;
    }
    goto Return;
    
  case FIONREAD:
    *(int *)data = so->so_rcv.sb_cc;
    goto Return;
    
  case SIOCATMARK:
    *(int *)data = (so->so_state&SS_RCVATMARK) != 0;
    goto Return;
  }

  /*
   * Interface/routing/protocol specific ioctls:
   * interface and routing ioctls should have a
   * different entry since a socket's unnecessary -- not really,
   * ifioctl needs the socket (?)
   */
  if ((IOCGROUP(cmd) & 0XDF) == 'I') {
    error = (ifioctl(so, cmd, data));
    goto Return;
  }
  if (IOCGROUP(cmd) == 'r') {
    error = (rtioctl(cmd, data));
    goto Return;
  }
  error = ((*so->so_proto->pr_usrreq)(so, PRU_CONTROL, 
	  (struct mbuf *)cmd, (struct mbuf *)data, (struct mbuf *)0));

 Return:
  ReleaseSyscallSemaphore(libPtr);
  DOPTERR(if (error) log(LOG_ERR,"IoctlSocket(): error %ld on command 0x%08lx", error, cmd);)
  API_STD_RETURN(error, 0);
}

AROS_LH3(LONG, IoctlSocket,
	AROS_LHA(LONG, fdes, D0),
        AROS_LHA(ULONG, cmd, D1),
        AROS_LHA(caddr_t, data, A0),
        struct SocketBase *, libPtr, 18, UL)
{
  AROS_LIBFUNC_INIT
  return __IoctlSocket(fdes, cmd, data, libPtr);
  AROS_LIBFUNC_EXIT
}



/*
 *  Semaphore to prevent select buffers from simultaneous modifications
 */
struct SignalSemaphore select_semaphore = { 0 };

struct newselbuf {
  int s_state;
#define	SB_CLEAR	0		/* wait condition cleared */
#define	SB_WILLWAIT	1		/* will wait if not cleared */
#define	SB_WAITING	2		/* waiting - wake up */
  int s_count;
  struct newselitem {
    struct	newselitem  *	si_next;	/* next selbuf in item chain */
    struct	newselitem **	si_prev;	/* back pointer */
    struct	newselbuf   *	si_selbuf;	/* 'thread' waiting */
  } s_item[0]; /* selitems are allocated at select when right
		  number of descriptors are known */
};

void select_init(void)
{
  InitSemaphore(&select_semaphore);
}

/*
 *  symbolic names FREAD and FWRITE aren'n needed anywhere else if only
 *  socket descriptors are in use.
 */
#define FREAD	0x1
#define FWRITE	0x2

/*
 * Select(), selscan(), selenter(), selwakeup() and unselect() are taken from
 * ../server/kern/sys_generic.h in bsdss distribution. soo_select() is
 * taken from ../server/kern/sys_socket.h, same distribution.
 */

/* 
 * Althrough the fd_set is used as the type of the masks, the size
 * of the fd_set is not fixed, and is indeed calculated from nfds.
 */
LONG __WaitSelect(ULONG nfds, fd_set *readfds, fd_set *writefds, fd_set *exeptfds,
   struct timeval *timeout, ULONG *sigmp, struct SocketBase *libPtr)
{
  fd_mask * obits;
  u_int obitsize;  /* in bytes */
  int error, retval;
  int selitemcount;
  ULONG sigmask = sigmp ? *sigmp : 0;

  struct newselbuf * newselbuf;
  
  CHECK_TASK();

  if (nfds > libPtr->dTableSize)
    nfds = libPtr->dTableSize;	/* forgiving; slightly wrong */
  
  if (timeout && itimerfix(timeout)) {
    error = EINVAL;
    goto Return;
  }
  
  /*
   *	We use selscan twice each time around the loop.
   *	The first time, we actually put ourself on device chains.
   *	The second time, we make a "fast" check for a true condition.
   *	selenter does nothing and we don't need to use unselect.
   *	We only need to zero obits once because if selscan
   *	turns on a bit it returns non-zero and we stop.
   *
   *	The "fast" scan is also done before the loop is ever entered.
   *
   *	We need to allocate some temporary buffers for output bit masks
   *	and, later for selitems. The amount of selitems needed is
   *	calculated during the first selscan -- when selenter does nothing
   */

  obitsize = howmany(nfds, NFDBITS) * sizeof (fd_mask);
  if (allocDataBuffer(&libPtr->selitems, 3 * obitsize) == FALSE) {
    error = ENOMEM;
    goto Return;
  }
  obits = (fd_mask *)libPtr->selitems.db_Addr;

  aligned_bzero((caddr_t)obits, 3 * obitsize);

  /*
   *	We make a first pass to check for conditions
   *	which are already true.  Clearing uu_sb
   *	will keep selenter from doing anything.
   */
  libPtr->p_sb = 0;
  error = selscan(libPtr, readfds, writefds, exeptfds,
		  obits, nfds, &retval, &selitemcount);

  /*
   *	Check if the 'real' loop should be entered.
   */
  if (!error && !retval) {
    if (timeout == NULL || timeout->tv_secs != 0L || timeout->tv_micro != 0L) {
	
      /*
       * Send timeout if there is any.
       */
      if (timeout)
	tsleep_send_timeout(libPtr, timeout);

      /*
       * Now, make sure there is room for all the selitems...
       */
      if (allocDataBuffer(&libPtr->selitems,
			  3 * obitsize + sizeof (struct newselbuf) +
			  selitemcount * sizeof(struct newselitem)) == FALSE) {
	error = ENOMEM;
	goto Return;
      }
      /*
       * We need to clear obits again _if_ the memory area for it has 
       * changed.
       */
      if ((fd_mask *)libPtr->selitems.db_Addr != obits) {
	obits = (fd_mask *)libPtr->selitems.db_Addr;
	aligned_bzero((caddr_t)obits, 3 * obitsize);
      }
      newselbuf = (struct newselbuf *)((caddr_t)obits + (3 * obitsize));
      
      while (TRUE) {

	/*
	 *	Now we get serious about blocking.
	 *	selenter will put us on device chains.
	 *	We don't need select_lock here because
	 *	no other thread can get at selbuf yet.
	 */
	
	newselbuf->s_state = SB_WILLWAIT;
	newselbuf->s_count = 0;
	libPtr->p_sb = newselbuf;
	
	error = selscan(libPtr, readfds, writefds, exeptfds,
			obits, nfds, &retval, &selitemcount);
	if (error || retval) {
	  ObtainSemaphore(&select_semaphore);
	  unselect(newselbuf);
	  ReleaseSemaphore(&select_semaphore);
	  break;
	}
	
	/*
	 *	If the state is already SB_CLEAR, then a selwakeup
	 *	slipped in after the selscan.
	 */
	
	ObtainSemaphore(&select_semaphore);
	if (newselbuf->s_state == SB_CLEAR) {
	  unselect(newselbuf);
	  ReleaseSemaphore(&select_semaphore);
	}
	else {
	  newselbuf->s_state = SB_WAITING;
	  tsleep_enter(libPtr, (caddr_t)newselbuf, "select");
	  ReleaseSemaphore(&select_semaphore);
	  /* ReleaseSemaphore(&syscall_semaphore); don't have this */
	  
	  error = tsleep_main(libPtr, sigmask);
	  
	  /* ObtainSemaphore(&syscall_semaphore); see above */
	  ObtainSemaphore(&select_semaphore);
	  unselect(newselbuf);
	  ReleaseSemaphore(&select_semaphore);
	  
	  if (error != 0) {
	    /*
	     * The break at the end of this block means that selscan will NOT 
	     * be done after this. This provides faster response to the 
	     * used defined signals.
	     *
	     * Note also that the semantics of the tsleep_abort_timeout() has 
	     * changed; it is now accepted to call it even after the timeout
	     * has expired.
	     */
	    if (error == ERESTART || error == EWOULDBLOCK)
	      error = 0;
	    
	    break;		/* do not scan now */
	  }
	  
	}
	/*
	 *	Scan for true conditions. Clearing uu_sb
	 *	will keep selenter from doing anything.
	 */
	libPtr->p_sb = 0;
	error = selscan(libPtr, readfds, writefds, exeptfds,
			obits, nfds, &retval, &selitemcount);
	if (error || retval)
	  break;
      } /* while (TRUE) */	

      if (timeout)		/* abort the timeout if any */
	tsleep_abort_timeout(libPtr, timeout);
    }
  }

 Return:
  libPtr->p_sb = 0;

#define	putbits(name, x) \
  if (name) { \
    aligned_bcopy(((caddr_t)obits) + (x * obitsize), \
		  (caddr_t)name, obitsize); \
   }

  if (error == 0) {
    if (sigmp)
      *sigmp &= SetSignal(0L, sigmask);
/*  ULONG sigs = SetSignal(0L, sigmask);
    if (sigmp)
      *sigmp &= sigs;
    SetSignal(0L,libPtr->sigIntrMask);*/
    
    putbits(readfds, 0);
    putbits(writefds, 1);
    putbits(exeptfds, 2);
  }
#undef putbits

  API_STD_RETURN(error, retval);
}

AROS_LH6(LONG, WaitSelect,
   AROS_LHA(ULONG, nfds, D0),
   AROS_LHA(fd_set *, readfds, A0),
   AROS_LHA(fd_set *, writefds, A1),
   AROS_LHA(fd_set *, exeptfds, A2),
   AROS_LHA(struct timeval *, timeout, A3),
   AROS_LHA(ULONG *, sigmp, D1),
   struct SocketBase *, libPtr, 19, UL)
{
  AROS_LIBFUNC_INIT
  DSYSCALLS(log(LOG_DEBUG,"WaitSelect(%lu, 0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx) called", nfds, readfds, writefds, exeptfds, timeout, sigmp);)
  return __WaitSelect(nfds, readfds, writefds, exeptfds, timeout, sigmp, libPtr);
  AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, GetSocketEvents,
   AROS_LHA(ULONG *,eventsp, A0),
   struct SocketBase *, libPtr, 46, UL)
{
   AROS_LIBFUNC_INIT
   struct soevent *se;
   struct socket *so;
   int i;

   ObtainSemaphore(&libPtr->EventLock);
   se = (struct soevent *)RemHead((struct List *)&libPtr->EventList);
   ReleaseSemaphore(&libPtr->EventLock);
   if (se) {
      *eventsp = se->events;
      DEVENTS(log(LOG_DEBUG,"GetSocketEvents(): events 0x%08lx for socket 0x%08lx libPtr = 0x%08lx", se->events, se->socket, libPtr);)
      so = se->socket;
      bsd_free(se, NULL);
      for (i = 0; i < libPtr->dTableSize; i++)
        if (libPtr->dTable[i] == so)
	   return i;
      DEVENTS(log(LOG_CRIT,"GetSocketEvents(): unreferenced socket 0x%08lx libPtr = 0x%08lx", so, libPtr);)
   }
      DEVENTS(else log (LOG_DEBUG,"GetSocketEvents(): no events pending libPtr = 0x%08lx", libPtr);)
   return -1;
   AROS_LIBFUNC_EXIT
}

/* 
 * Althrough the fd_set is used as the type of the masks, the size
 * of the fd_set is not fixed, and is indeed calculated from nfd.
 */
static int 
selscan(struct SocketBase *p,
	fd_set * in,
	fd_set * ou,
	fd_set * ex,
	fd_mask * obits,
	int nfd,
	int * retval,
	int * selitemcount_p)
{
	register int which, i, j, selitemcount = 0;
	register fd_mask bits;
	register fd_set * cbits;
	struct socket *so;
	int flag;
	int n = 0;
	int error = 0;
	u_int obitsize = howmany(nfd, NFDBITS); /* in longwords */

	for (which = 0; which < 3; which++) {
		switch (which) {

		case 0:
			flag = FREAD;
			cbits = in;
			break;
		case 1:
			flag = FWRITE;
			cbits = ou;
			break;
		case 2:
			flag = 0;
			cbits = ex;
			break;
		}
		if (cbits != NULL) {
		  for (i = 0; i < nfd; i += NFDBITS) {
			bits = cbits->fds_bits[i/NFDBITS];
			while ((j = ffs(bits)) && i + --j < nfd) {
				bits &= ~(1 << j);
				so = p->dTable[i + j];
				if (so == NULL) {
				  error = EBADF;
				  break;
				}
				selitemcount++;
				if (soo_select(so, flag, p)) {
				  FD_SET(i + j, (fd_set *)
					 (obits + (which * obitsize)));
				  n++;
				}
			}
		  }
		}
	}
	*retval = n;
	*selitemcount_p = selitemcount;
	return (error);
}

/*
 * Add a select event to the current select buffer.
 * Chain all select buffers that are waiting for
 * the same event.
 */
void selenter(struct SocketBase *	p,
	      struct newselitem **	hdr)
{
	register struct newselbuf *sb = p->p_sb;

	if (sb) {
	    register struct newselitem **sip = hdr;
	    register struct newselitem *si;

	    ObtainSemaphore(&select_semaphore);

	    /* initialize this select item */
	    si = &sb->s_item[sb->s_count++];
	    si->si_selbuf = sb;

	    /* and add it to the device chain */
	    if (si->si_next = *sip)
		si->si_next->si_prev = &si->si_next;
	    *(si->si_prev = sip) = si;

	    ReleaseSemaphore(&select_semaphore);
	}
}

static void
unselect(register struct newselbuf * sb)
{
	int i;

	/*
	 *	We unchain the select items.
	 *	This assumes select_lock is held.
	 */

	for (i = 0; i < sb->s_count; i++) {
	    register struct newselitem *si = &sb->s_item[i];

	    /*
	     *	We remove this item from its device chain.
	     *	If selwakeup already got to it, then this is a nop.
	     */

	    *si->si_prev = si->si_next;
	}
}

/*
 * Wakeup all threads waiting on the same select event.
 * Do not clear the other select events that each thread
 * is waiting on; thread will do that itself.
 */
void selwakeup(struct newselitem **hdr)
{
	register struct newselitem **sip;
	register struct newselitem *si;

	ObtainSemaphore(&select_semaphore);

	for (sip = hdr; (si = *sip) != 0; sip = &si->si_next) {
	    register struct newselbuf *sb;
	    int old_state;

	    /* for when unselect tries to dequeue this item */
	    si->si_prev = &si->si_next;

	    /* check for wakeup */
	    sb = si->si_selbuf;
	    old_state = sb->s_state;
	    sb->s_state = SB_CLEAR;
	    if (old_state == SB_WAITING)
		wakeup((caddr_t)sb);
	}

	/* clear the device chain */
	*(struct newselitem **)hdr = 0;
	ReleaseSemaphore(&select_semaphore);
}

static int
soo_select(struct socket *so,
	       int which,
	       struct SocketBase *p)
{
	register spl_t s = splnet();

	switch (which) {

	case FREAD:
		if (soreadable(so)) {
			splx(s);
			return (1);
		}
		sbselqueue(&so->so_rcv, p);
		break;

	case FWRITE:
		if (sowriteable(so)) {
			splx(s);
			return (1);
		}
		sbselqueue(&so->so_snd, p);
		break;

	case 0:
		if (so->so_oobmark ||
		    (so->so_state & SS_RCVATMARK)) {
			splx(s);
			return (1);
		}
		sbselqueue(&so->so_rcv, p);
		break;
	}
	splx(s);
	return (0);
}

/*
 * countSockets() counts how many references ONE task (SocketBase) have to a
 * socket
 */

static int
countSockets(struct SocketBase * libPtr, struct socket * so)
{
  int i, count = 0;

  for (i = 0; i < libPtr->dTableSize; i++)
    if (libPtr->dTable[i] == so)
      count++;

  return count;
}

LONG __CloseSocket(LONG fd, struct SocketBase *libPtr)
{
  register int error;
  struct socket *so;
  struct soevent *se;

  CHECK_TASK();
  ObtainSyscallSemaphore(libPtr);

  /*
   * Check from used sockets bitmask if this socket is in use
   */
  if (! FD_ISSET(fd, (fd_set *)(libPtr->dTable + libPtr->dTableSize))) {
    error = EBADF;
    goto Return;
  }

  /*
   * If the link library cannot free the fd, then we cannot do it either.
   */
  if (libPtr->fdCallback)
    if (error = AROS_UFC2(int, libPtr->fdCallback,
	AROS_UFCA(int, fd, D0),
	AROS_UFCA(int, FDCB_FREE, D1)))
      goto Return;

  FD_CLR(fd, (fd_set *)(libPtr->dTable + libPtr->dTableSize));

  if (error = getSock(libPtr, fd, &so)) {
/*    error = ENOTSOCK;	/* well, bit set, but socket == NULL */
    error = 0; /* ignore silently */
    goto Return;
  }
  if (so->so_pgid == libPtr && countSockets(libPtr, so) == 1) 
    so->so_pgid = NULL;		/* not ours any more */

  /*
   * Decrease the reference count of a socket (AmiTCP addition) and return if
   * not zero.
   */
  if (--so->so_refcnt <= 0) {
    error = soclose(so);
    /* Remove all events associated with this socket */
    ObtainSemaphore(&libPtr->EventLock);
    for (se = (struct soevent *)libPtr->EventList.mlh_Head; se->node.mln_Succ; se = (struct soevent *)se->node.mln_Succ)
    {
	if (se->socket == so) {
	  Remove((struct Node *)se);
	  bsd_free(se, NULL);
	}
    }
    ReleaseSemaphore(&libPtr->EventLock);
  }

  /*
   * now clear socket from descriptor table. Socket usage bitmask has been
   * cleared earlier
   */
  libPtr->dTable[fd] = NULL;

 Return: 
  ReleaseSyscallSemaphore(libPtr);
  API_STD_RETURN(error, 0);
}

AROS_LH1(LONG, CloseSocket,
   AROS_LHA(LONG, fd, D0),
   struct SocketBase *, libPtr,20, UL)
{
  AROS_LIBFUNC_INIT
  DSYSCALLS(log(LOG_DEBUG,"CloseSocket(%ld) called", fd);)
  return __CloseSocket(fd, libPtr);
  AROS_LIBFUNC_EXIT
}

struct SocketNode {
  struct MinNode	sn_Node;
  LONG			sn_Id;
  struct socket *	sn_Socket;
};

/*
 * checkId() checks that there are no released sockets w/ given id.
 * used from function makeId().
 */

static LONG checkId(int id)
{
  struct Node * sn;

  Forbid();
  for (sn = releasedSocketList.lh_Head; sn->ln_Succ; sn = sn->ln_Succ)
    if (((struct SocketNode *)sn)->sn_Id == id) {
      Permit();
      return EINVAL;
    }
  Permit();	
  return 0;
}

#define FIRSTUNIQUEID 65536

/*
 * makeId() gets next unique id for socket to be released if *id is -1
 * otherwise calls checkId for given *id.
 * used from functions ReleaseSocket() and ReleaseCopyOfSocket().
 */
static LONG makeId(LONG id)
{
  static LONG uniqueId = FIRSTUNIQUEID;

  if (id == -1) {
    do {
      uniqueId += sizeof (void *);
      if (uniqueId < 0)
	uniqueId = FIRSTUNIQUEID;
    } while (checkId(uniqueId) != 0);

    return uniqueId;
  }
  else {
    if (checkId(id) == 0)
      return id;
    else
      return -1; /* error */
  }
}

/*LONG SAVEDS ReleaseSocket(
   REG(d0, LONG fd),
   REG(d1, LONG id),
   REG(a6, struct SocketBase *libPtr))*/
AROS_LH2(LONG, ReleaseSocket,
   AROS_LHA(LONG, fd, D0),
   AROS_LHA(LONG, id, D1),
   struct SocketBase *,libPtr, 21, UL)
{
  AROS_LIBFUNC_INIT
  struct SocketNode *sn;
  struct socket *so;
  int error = 0;

  CHECK_TASK();
  DSYSCALLS(log(LOG_DEBUG,"ReleaseSocket(%ld, %ld) called", fd, id);)
  if ((ULONG)id >= FIRSTUNIQUEID && (id = makeId(id)) == -1) {
    error = EINVAL;
    goto Return;
  }

  if ((error = getSock(libPtr, fd, &so)) != 0)
    goto Return;
 
  if ((sn = AllocMem(sizeof (struct SocketNode), MEMF_PUBLIC)) == NULL) {
    error = ENOMEM;
    goto Return;
  }
  /*
   * Tell the link library that the fd is free
   */
  if (libPtr->fdCallback)
    if (error = AROS_UFC2(int, libPtr->fdCallback,
	AROS_UFCA(int, fd, D0),
	AROS_UFCA(int, FDCB_FREE, D1)))
 {
      FreeMem(sn, sizeof (struct SocketNode));
      goto Return;
    }
  
/*if (so->so_pgid == libPtr && countSockets(libPtr, so) == 1)
    so->so_pgid = NULL;*/	  /* not ours any more */
  sn->sn_Id = id;
  sn->sn_Socket = so;
  libPtr->dTable[fd] = NULL;
  FD_CLR(fd, (fd_set *)(libPtr->dTable + libPtr->dTableSize));
  
  Forbid();
  AddTail(&releasedSocketList, (struct Node *)sn);
  Permit();

 Return: API_STD_RETURN(error, id);
  AROS_LIBFUNC_EXIT
}

/*LONG SAVEDS ReleaseCopyOfSocket(
   REG(d0, LONG fd),
   REG(d1, LONG id),
   REG(a6, struct SocketBase *libPtr))*/
AROS_LH2(LONG, ReleaseCopyOfSocket,
   AROS_LHA(LONG, fd, D0),
   AROS_LHA(LONG, id, D1),
   struct SocketBase *, libPtr, 22, UL)
{
  AROS_LIBFUNC_INIT
  struct SocketNode *sn;
  struct socket *so;
  int error = 0;

  CHECK_TASK();
  DSYSCALLS(log(LOG_DEBUG,"ReleaseCopyOfSocket(%ld, %ld) called", fd, id);)
  if ((ULONG)id >= FIRSTUNIQUEID && (id = makeId(id)) == -1) {
    error = EINVAL;
    goto Return;
  }

  if ((error = getSock(libPtr, fd, &so)) != 0)
    goto Return;

  if ((sn = AllocMem(sizeof (struct SocketNode), MEMF_PUBLIC)) == NULL) {
    error = ENOMEM;
    goto Return;
  }
  /* so_pgid left intact */
  sn->sn_Id = id;
  sn->sn_Socket = so;
  so->so_refcnt++;
  Forbid();
  AddTail(&releasedSocketList, (struct Node *)sn);
  Permit();

 Return: API_STD_RETURN(error, id);
  AROS_LIBFUNC_EXIT
}

/*LONG SAVEDS ObtainSocket(
   REG(d0, LONG id),
   REG(d1, LONG domain),
   REG(d2, LONG type),
   REG(d3, LONG protocol),
   REG(a6, struct SocketBase *libPtr))*/
AROS_LH4(LONG, ObtainSocket,
   AROS_LHA(LONG, id, D0),
   AROS_LHA(LONG, domain, D1),
   AROS_LHA(LONG, type, D2),
   AROS_LHA(LONG, protocol, D3),
   struct SocketBase *, libPtr, 23, UL)
{
  AROS_LIBFUNC_INIT
  struct protosw *prp;
  struct Node * sn;
  int error = 0;
  LONG fd;

  CHECK_TASK();
  DSYSCALLS(log(LOG_DEBUG,"ObtainSocket(%ld, %ld, %ld, %ld) called", id, domain, type, protocol);)
  if (domain == 0) {
    if (id < FIRSTUNIQUEID) {
      error = EPFNOSUPPORT;
      goto Return;
    }
  }
  else {
    if (protocol)
      prp = pffindproto(domain, protocol, type);
    else
      prp = pffindtype(domain, type);
    if (prp == 0)
      error = EPROTONOSUPPORT;
    if (prp->pr_type != type)
      error = EPROTOTYPE;
    if (error)
      goto Return;
  }  

  if ((error = sdFind(libPtr, &fd)) != 0)
    goto Return;

  /*
   * Tell the link library about the new fd
   */
  if (libPtr->fdCallback)
    if (error = AROS_UFC2(int, libPtr->fdCallback,
	AROS_UFCA(int, fd, D0),
	AROS_UFCA(int, FDCB_ALLOC, D1)))
      goto Return;
  
  Forbid();
  for (sn = releasedSocketList.lh_Head; sn->ln_Succ; sn = sn->ln_Succ)
    if (((struct SocketNode *)sn)->sn_Id == id) {
      if (prp != ((struct SocketNode *)sn)->sn_Socket->so_proto)
	continue;
      Remove(sn);
      Permit();
      libPtr->dTable[fd] = ((struct SocketNode *)sn)->sn_Socket;
//    ((struct SocketNode *)sn)->sn_Socket->so_pgid = libPtr;
      FD_SET(fd, (fd_set *)(libPtr->dTable + libPtr->dTableSize));
      FreeMem(sn, sizeof (struct SocketNode));
      goto Return;
    }
  /* here if sdFind succeeded but search of socket failed */
  Permit();

  /*
   * Free the just allocated fd
   */
  if (libPtr->fdCallback)
    AROS_UFC2(int, libPtr->fdCallback,
	AROS_UFCA(int, fd, D0),
	AROS_UFCA(int, FDCB_FREE, D1));

  error = EWOULDBLOCK;
  
 Return: API_STD_RETURN(error, fd);
  AROS_LIBFUNC_EXIT
}

/*LONG SAVEDS Dup2Socket(
   REG(d0, LONG fd1),
   REG(d1, LONG fd2),
   REG(a6, struct SocketBase *libPtr))*/
AROS_LH2(LONG, Dup2Socket,
   AROS_LHA(LONG, fd1, D0),
   AROS_LHA(LONG, fd2, D1),
   struct SocketBase *, libPtr, 23, UL)
{
  AROS_LIBFUNC_INIT
  LONG newfd;
  int error = 0;
  struct socket *so;

  CHECK_TASK();
  DSYSCALLS(log(LOG_DEBUG,"Dup2Socket(%ld, %ld) called", fd1, fd2);)
  ObtainSyscallSemaphore(libPtr); /*many tasks may have access to the socket */

  if (fd1 == -1)
    so = NULL;
  else if ((error = getSock(libPtr, fd1, &so)))
    goto Return;

  if (fd2 == -1)
    if ((error = sdFind(libPtr, &newfd)))
      goto Return;
    else
      fd2 = newfd;
  else {
    if ((ULONG)fd2 >= libPtr->dTableSize) {
      error = EBADF;
      goto Return;
    }
    if (libPtr->dTable[fd2] != NULL)
      __CloseSocket(fd2, libPtr);
    /*
     * Check if the fd is free on the link library
     */
    if (libPtr->fdCallback)
      if (error = AROS_UFC2(int, libPtr->fdCallback,
	AROS_UFCA(int, fd2, D0),
	AROS_UFCA(int, FDCB_CHECK, D1)))
	goto Return;

  }
  /*
   * Tell the link library about the new fd
   */
  if (libPtr->fdCallback)
    if (error = AROS_UFC2(int, libPtr->fdCallback,
	AROS_UFCA(int, fd2, D0),
	AROS_UFCA(int, FDCB_ALLOC, D1)))
      goto Return;

  FD_SET(fd2, (fd_set *)(libPtr->dTable + libPtr->dTableSize));
  
  if (so != NULL) {
    so->so_refcnt++;
    libPtr->dTable[fd2] = so;
  }

 Return:
  ReleaseSyscallSemaphore(libPtr);
  API_STD_RETURN(error, fd2);
  AROS_LIBFUNC_EXIT
}
