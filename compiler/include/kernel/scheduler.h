#ifndef KERNEL_SCHEDULER_H
#define KERNEL_SCHEDULER_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id:$

    Desc: kernel.resource task scheduler definitions
    Lang: english
*/

#include <exec/tasks.h>

/*
 * Task scheduler state structure.
 * On multi-CPU system we have one such a structure per CPU core.
 * This structure is intentionally the same as part of ExecBase.
 */
struct TaskScheduler
{
    struct Task *ThisTask;      /* Pointer to currently running task (readable) */
    ULONG        IdleCount;	/* CPU idle time slice count			*/
    ULONG        DispCount;	/* Task dispatching count			*/
    UWORD        Quantum;       /* Reserved for exec.library			*/
    UWORD        Elapsed;       /* # of time slices the current task has run	*/
    UWORD        SysFlags;	/* State flags, see below			*/
    BYTE         IDNestCnt;	/* Interrupts disable nesting count		*/
    BYTE         TDNestCnt;	/* Task switch disable nesting count		*/
    UWORD        AttnFlags;     /* Reserved for m68k exec.library		*/
    UWORD        AttnResched;	/* More state flags, see below			*/
};

/* SysFlags */
#define SFF_SoftInt         (1L<<5)  /* There is a software interrupt pending */
#define SFF_QuantumOver     (1L<<13) /* Task's time slice is over	      */

/* AttnResched */
#define ARF_AttnSwitch      (1L<<7)  /* Delayed task switch pending */

#endif
