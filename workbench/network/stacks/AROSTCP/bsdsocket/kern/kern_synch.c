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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/synch.h>
#include <sys/queue2.h>
#include <sys/syslog.h>

#include <kern/amiga_includes.h>

#include <api/amiga_api.h>

#include <proto/exec.h>

/*
 * Note about spl-functions: this implementation does NOT check for software
 * interrupts when returning to level spl 0 (not needed on AmigaOS, see the 
 * bottom of this file).
 */

/*
 * Sleeping threads are hashed by 'chan' onto sleep queues.
 */
 
#define	SLEEP_QUEUE_SIZE	32	/* power of 2 */
#define	SLEEP_HASH(x)	(((IPTR)(x)>>5) & (SLEEP_QUEUE_SIZE - 1))

queue_head_t	sleep_queue[SLEEP_QUEUE_SIZE];

/*
 * semaphore protecting sleep queues
 */
struct SignalSemaphore sleep_semaphore;
static BOOL sleep_initialized = FALSE;

/*
 * Sleep system initialization.
 */
BOOL
sleep_init(void)
{
  register int i;

#if defined(__AROS__)
D(bug("[AROSTCP](kern_synch.c) sleep_init()\n"));
#endif

  if (!sleep_initialized) {
    /*
     * initialize the semaphore protecting sleep queues
     */
    InitSemaphore(&sleep_semaphore);
    
    /*
     * initialize the sleep queues
     */
    for (i = 0; i < SLEEP_QUEUE_SIZE; i++)
      queue_init(&sleep_queue[i]);
    
    sleep_initialized = TRUE;
  }
  return TRUE;
}

void
tsleep_send_timeout(struct SocketBase *p,
		    const struct timeval *time_out)
{
#if defined(__AROS__)
D(bug("[AROSTCP](kern_synch.c) tsleep_send_timeout()\n"));
#endif
  /*
   * Make sure that the timer message is back from the timer device
   */
  if (p->tsleep_timer->tr_node.io_Message.mn_Node.ln_Type != NT_UNKNOWN) {
    /*
     * abort previous timeout if it has not been completed yet
     */
    if (p->tsleep_timer->tr_node.io_Message.mn_Node.ln_Type != NT_REPLYMSG) {
      AbortIO((struct IORequest *)(p->tsleep_timer));
    }
    /*
     * Remove timerequest from reply port.
     */
    WaitIO((struct IORequest *)p->tsleep_timer);
    /*
     * Set the node type to NT_UNKNOWN to mark that it is referenced only by
     * the p->tsleep_timer.
     */
    p->tsleep_timer->tr_node.io_Message.mn_Node.ln_Type = NT_UNKNOWN;
#if 0
    /*
     * Make sure the signal gets cleared.
     *
     * if this is not done, the tsleep_main will do one unnecessary round
     * IF the signal was set. IS THIS WORTH OF IT?
     */
    SetSignal(0, 1 << p->timerPort->mp_SigBit);
#endif
  }
  /*
   * send timeout request if necessary
   */
  if (time_out) {
    /*
     * set the timeout
     */
    p->tsleep_timer->tr_time = *time_out;
    /*
     * Enable signalling again
     */
    p->timerPort->mp_Flags = PA_SIGNAL;
    /*
     * send the request to the timer device
     */
    BeginIO((struct IORequest *)(p->tsleep_timer));  
  }
}

void
tsleep_abort_timeout(struct SocketBase *p,
		     const struct timeval *time_out)
{
#if defined(__AROS__)
D(bug("[AROSTCP](kern_synch.c) tsleep_abort_timeout()\n"));
#endif
  if (time_out) {
    /* do not signal us any more */
    p->timerPort->mp_Flags = PA_IGNORE;
  }
}

void
tsleep_enter(struct SocketBase *p,
	     caddr_t chan,		/* 'channel' to wait on */
	     const char *wmesg)		/* reason to sleep */
{
  register queue_t	q;
#if defined(__AROS__)
D(bug("[AROSTCP](kern_synch.c) tsleep_enter()\n"));
#endif
  /*
   * Zero is a reserved value, used to indicate
   * that we have been woken up and are no longer on
   * the sleep queues.
   */
  
#if DIAGNOSTIC  
  if (chan == 0)
    panic("tsleep");
#endif
  
  /*
   * The sleep_semaphore protects the sleep queues and
   * p_ fields in SocketBases.
   *
   * When the process is in a sleep queue its p_wchan field is nonzero.
   */
  ObtainSemaphore(&sleep_semaphore);
  p->p_wchan = chan;
  p->p_wmesg = wmesg;
  q = &sleep_queue[SLEEP_HASH(chan)];
  queue_enter(q, p, struct SocketBase *, p_sleep_link);
  ReleaseSemaphore(&sleep_semaphore);
}

int
tsleep_main(struct SocketBase *p, ULONG wakemask)
{
  ULONG sigmask, bmask, timermask;
  struct timerequest *timerReply;
  register queue_t q;
  int result;
#if defined(__AROS__)
D(bug("[AROSTCP](kern_synch.c) tsleep_main()\n"));
#endif
  /* 
   * Set the signal mask for the wait
   */
  timermask = 1 << p->timerPort->mp_SigBit;
  sigmask = timermask | p->sigIntrMask | wakemask;

  for (;;) {
    /* 
     * wait for timeout, wakeup or interrupt
     */
    bmask = Wait(sigmask);

    /*
     * Check if we were interrupted
     */
    if (bmask & p->sigIntrMask & ~wakemask) {
      result = EINTR;
      break;
    }

    /*
     * Check for user signals
     */
    if (bmask & wakemask) {
      result = ERESTART;
      break;
    }

    /*
     * check if we were woken up. 
     *
     * If p->p_chan is zero then the wakener has removed us from
     * the sleep queue.
     */
    if (p->p_wchan == 0) {
      /*
       * Set back the signals which interrupted us so that user program can
       * detect them
       */
      bmask &= p->sigIntrMask|wakemask;
      if (bmask)
	SetSignal(bmask, bmask);
      
      return 0;			/* return success */
    }

    /*
     * check if we got the timer reply signal and message
     */
    if (bmask & timermask &&
	(timerReply = (struct timerequest *)GetMsg(p->timerPort)) && 
	timerReply == p->tsleep_timer) { /* sanity check */
      /*
       * timeout expired.
       *
       * Set the node type to NT_UNKNOWN to mark that it is referenced only by
       * the p->tsleep_timer.
       */
      timerReply->tr_node.io_Message.mn_Node.ln_Type = NT_UNKNOWN;

      result = EWOULDBLOCK;
      break;
    }
    
  } /* for */

  /* Return path when sleeper has to be removed from the sleep queue */

  /*
   * Set back the signals which interrupted us so that user program can
   * detect them
   */
  bmask &= p->sigIntrMask | wakemask;
  if (bmask)
    SetSignal(bmask, bmask);

  /*
   * remove from the sleep queue
   */
  ObtainSemaphore(&sleep_semaphore);
  /*
   * If p_chan is nonzero then we still are on the sleep queue and
   * need to be removed from there.
   */
  if (p->p_wchan != 0) {
    q = &sleep_queue[SLEEP_HASH(p->p_wchan)];
    p->p_wchan = (char *)0;
    queue_remove(q, p, struct SocketBase *, p_sleep_link);
  }
  ReleaseSemaphore(&sleep_semaphore);

  return result;
}

/*
 * General sleep call. 
 * NOTE: caller is assumed to hold the syscall_semaphore!         \* XXX *\
 * Suspends current process until a wakeup is made on chan.
 * Sleeps at most the time specified in a time_out (NULL means no timeout).
 * Lowers the current spl-level to 0 while in sleep.
 * Returns 0 if awakened, EWOULDBLOCK if the timeout expires and
 * EINTR if interrupted.
 */
int
tsleep(struct SocketBase *p,  /* Library base through which this call came */
       caddr_t chan,	      /* 'channel' to wait on */
       const char *wmesg,	      /* reason to sleep */
       const struct timeval *time_out) /* timeout as timeval structure */
{
  int result;
  spl_t old_spl;
#if defined(__AROS__)
D(bug("[AROSTCP](kern_synch.c) tsleep()\n"));
#endif

#if DIAGNOSTIC
  extern struct Task *AROSTCP_Task;
  if (FindTask(NULL) == AROSTCP_Task) {
    log(LOG_ERR, "TCP/IP stack did tsleep() itself!");
    return (-1);
  }
#endif 
  
#if DIAGNOSTIC
  if (p == NULL) {
    log(LOG_ERR, "tsleep() called with NULL SocketBase pointer!");
    return (-1);
  }
#endif 

#if DIAGNOSTIC
  if (FindTask(NULL) != syscall_semaphore.ss_Owner) {
    log(LOG_ERR, "tsleep() called with NO syscall_semaphore!");
    return (-1);
  }
#endif
    
  tsleep_send_timeout(p, time_out);
  
  tsleep_enter(p, chan, wmesg);

  /*
   * release spl-level while in sleep.
   * 
   * NOTE: syscall_semaphore must be freed as well!
   */

  old_spl = spl0();
  ReleaseSemaphore(&syscall_semaphore);	                     /* XXX */

  result = tsleep_main(p, 0);

  /*
   * return old spl-level
   */
  ObtainSemaphore(&syscall_semaphore);	                     /* XXX */
  splx(old_spl);

  /*
   * abort the timeout request if necessary
   */
  if (result != EWOULDBLOCK)
    tsleep_abort_timeout(p, time_out);

  return (result);
}

void
wakeup(caddr_t chan)
{
  register queue_t q;
  struct SocketBase *p, *next;
#if defined(__AROS__)
D(bug("[AROSTCP](kern_synch.c) wakeup()\n"));
#endif

#if DIAGNOSTIC
  if (chan == 0) {
    log(LOG_ERR, "wakeup on chan zero");
    return;
  }
#endif 

  ObtainSemaphore(&sleep_semaphore);
  q = &sleep_queue[SLEEP_HASH(chan)];
  
  p = (struct SocketBase *)queue_first(q);
  while (!queue_end(q, (queue_entry_t)p)) {
    next = (struct SocketBase *)queue_next(&p->p_sleep_link);
    if (p->p_wchan == chan) {
      /*
       * mark sleeper as woken up
       */
      p->p_wchan = NULL;
      /*
       * remove sleeper from the sleep queue
       */
      queue_remove(q, p, struct SocketBase *, p_sleep_link);
      /*
       * signal process to take attention
       */
      Signal(p->thisTask, 1 << p->timerPort->mp_SigBit);
    }
    p = next;
  }
  ReleaseSemaphore(&sleep_semaphore);
}

/*
 * Spl-level emulation:
 *
 * In this implementation the processor priority levels are modelled
 * either with one semaphore (ifdef DEBUG) or by Exec Task switch
 * disabling feature. Semaphore is used while debugging for security (to
 * be able to single step almost everywhere). The production version uses
 * ExecBase's TDNestCnt for speed (prevent unnecessary task switches).
 * 
 * Note that both ways lead to the fact that when someone sets, say, splimp()
 * he will WAIT for the holder of splnet() to finish.
 *
 * N.B. The above comments are now out-of-date. The semaphore implementation
 * is now always used.
 */

/*#ifndef DEBUG*/ /* NC */
#if 1
/*
 * spl_semaphore is used as mutex for all spl-levels
 *
 * Note that InitSemaphore() requires the signalSemaphore to be initialized
 * to zero (here done statically).
 */
struct SignalSemaphore spl_semaphore = { };

/*
 * spl_level holds the current pseudo priority level.
 * NOTE: this may be accessed only while holding the spl_semaphore.
 */
spl_t spl_level = SPL0;
static BOOL spl_initialized = FALSE;

BOOL
spl_init(void)
{
#if defined(__AROS__)
D(bug("[AROSTCP](kern_synch.c) spl_init()\n"));
#endif
  if (!spl_initialized) {
    /*
     * Initialize spl_semaphore for use. After this call any number of 
     * tasks may use spl-functions.
     */
    InitSemaphore(&spl_semaphore);
    spl_initialized = TRUE;
  }
  return TRUE;
}

spl_t
spl_n(spl_t new_level)
{
  register spl_t old_level;
/* Uncomment the following lines to get debug - however please note: this func runs often */
//#if defined(__AROS__)
//D(bug("[AROSTCP](kern_synch.c) spl_n()\n"));
//#endif

  ObtainSemaphore(&spl_semaphore);
  old_level = spl_level;
  spl_level = new_level;
  
  if (new_level > old_level) {	/* raise level */
    if (old_level == 0)		/* lock when raising over zero */
      /* 
       * so. return without releasing the lock
       */
      return old_level;
  }
  else if (new_level < old_level) {	/* lower level */
    if (new_level == 0)			/* unlock when lowering to zero */
      /*
       * now release the lock kept above
       */
      ReleaseSemaphore(&spl_semaphore);
  }
  ReleaseSemaphore(&spl_semaphore);

  return old_level;
}

#else

BOOL
spl_init(void)
{
#if defined(__AROS__)
D(bug("[AROSTCP](kern_synch.c) spl_init() VOID\n"));
#endif
  return TRUE;
}

/*
 * spl_n is defined as an inline in <sys/synch.h>
 */
#endif /* DEBUG */
