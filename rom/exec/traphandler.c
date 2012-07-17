/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Default trap handler
    Lang: english
*/

#include <exec/alerts.h>
#include <exec/interrupts.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#include "etask.h"
#include "exec_intern.h"
#include "exec_util.h"

/* User-mode code where the task jumps to.
 * NOTE: This is only called on ETasks!
 */
static void Exec_CrashHandler(void)
{
    struct Task *task = FindTask(NULL);
    struct IntETask *iet = GetIntETask(task);

    /* Makes Alert() attempting to bring up Intuition requester */
    iet->iet_AlertFlags &= ~AF_Alert;

    Alert(iet->iet_AlertCode);
}

/* In original AmigaOS the trap handler is entered in supervisor mode with the
 * following on the supervisor stack:
 *  0(sp).l = trap#
 *  4(sp) Processor dependent exception frame
 * In our current implementation we use two pointers on stack. The idea is taken
 * from AmigaOS v4.
 *
 * For m68k, we need a thunk, since we will call this routine via
 * the AOS 1.x-3.x method described above.
 *
 * See arch/m68k-all/kernel/m68k_exception.c for implementation details.
 */
#ifdef __mc68000
asm (
        "       .text\n"
        "       .balign 2\n"
        "       .global Exec_TrapHandler\n"
        "Exec_TrapHandler:\n"
        "       move.l  #0,%sp@-\n"     /* Push on a NULL trapcode */
        "       movem.l %d0-%d7/%a0-%a7,%sp@-\n"   /* Save regs */
        "       move.l  %usp,%a0\n"          /* Get USP */
        "       move.l  %a0,%sp@(15*4)\n"    /* Fix up A7 to be USP */
        "       move.l  %sp@(17*4),%d0\n"    /* Get the trap number */
        "       move.l  %sp,%sp@-\n"    /* Push on pointer to exception ctx */
        "       move.l  %d0,%sp@-\n"    /* Push on trap number */
        "       jsr Exec__TrapHandler\n" /* Call C routine */
        "       addq.l  #8,%sp\n"       /* Pop off C routine args */
        "       movem.l %sp@+,%d0-%d7/%a0-%a5\n"  /* Restore registers */
        "       move.l  %sp@(4), %a6\n"
        "       move.l  %a6,%usp\n"     /* Restore USP */
        "       move.l  %sp@+,%a6\n"    /* Restore A6 */
        "       addq.l  #4,%sp\n"       /* Skip past A7 */
        "       addq.l  #8,%sp\n"       /* Skip past trapcode and traparg */
        "       rte\n"                  /* Return from trap */
        );
void Exec__TrapHandler(ULONG trapNum, struct ExceptionContext *ctx)
#else
void Exec_TrapHandler(ULONG trapNum, struct ExceptionContext *ctx)
#endif
{
    struct Task *task = SysBase->ThisTask;

    /* Our situation is deadend */
    trapNum |= AT_DeadEnd;

    /*
     * We must have a valid ETask in order to be able
     * to display a requester in user mode.
     */
    if (task && (task->tc_Flags & TF_ETASK) && (task->tc_State != TS_REMOVED))
    {
	/* Get internal task structure */
        struct IntETask *iet = GetIntETask(task);

	if (iet->iet_AlertFlags & AF_Alert)
	{
	    /*
	     * Protection against double-crash. If the task is already in alert state,  we have
	     * a second crash during processing the first one. Then we just pick up initial alert code
	     * and call Alert() with it in supervisor mode.
	     */
	    trapNum = iet->iet_AlertCode;
	}
	else
	{
	    /*
	     * Otherwise we can try to send the crash to user level.
	     *
	     * First mark crash condition, and also specify alert code for user-level handler.
	     * If we double-crash while jumping to user mode, we will know this (ETF_Alert will
	     * already be set)
	     */
	    iet->iet_AlertType     = AT_CPU;
	    iet->iet_AlertFlags    = AF_Alert|AF_Location;
	    iet->iet_AlertCode     = trapNum;
	    iet->iet_AlertLocation = (APTR)ctx->PC;    /* Location is our PC, where we crashed */
	    iet->iet_AlertStack    = (APTR)ctx->FP;    /* Remember also stack frame for backtrace */

	    /*
	     * This is a CPU alert. We've got a full CPU context, so we remember it
	     * in our ETask structure for displaying to the user later.
	     * Note that we store only GPR part of the context. We don't copy
	     * attached FPU data (if any). This can be considered TODO.
	     */
	    CopyMem(ctx, &iet->iet_AlertData, sizeof(struct ExceptionContext));

            /*
	     * Make the task to jump to crash handler. We don't care about return address etc because
	     * the alert is deadend anyway.
	     */
            ctx->PC = (IPTR)Exec_CrashHandler;
	    /* Let the task go */
	    return;
	}
    }

    Exec_ExtAlert(trapNum, (APTR)ctx->PC, (APTR)ctx->FP, AT_CPU, ctx, SysBase);
} /* TrapHandler */
