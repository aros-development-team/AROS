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

/* In original AmigaOS the trap handler is entered in supervisor mode with the
 * following on the supervisor stack:
 *  0(sp).l = trap#
 *  4(sp) Processor dependent exception frame
 * In our current implementation we use two pointers on stack. The idea is taken
 * from AmigaOS v4.
 *
 * TODO: m68k trap handler should be binary-compatible with original AmigaOS
 * implementation. There are two possible ways to accomplish this:
 * 1. Implement a stub in asm around it. tc_TrapCode should point to this stub.
 * 2. Implement AROS-specific flag in task's tc_Flags which will tell that
 *    a newstyle trap handler is used.
 * Each of this approaches has its own pros and cons.
 */

void Exec_TrapHandler(ULONG trapNum, struct ExceptionContext *ctx)
{
    struct Task *task = SysBase->ThisTask;
    struct IntETask *iet;

    /* Our situation is deadend */
    trapNum |= AT_DeadEnd;

    if (task)
    {
	/* Get internal task structure */
        iet = GetIntETask(task);
	/*
	 * Protection against double-crash. If the alert code is already specified, we have
	 * a second crash during processing the first one. Then we just pick up initial alert code
	 * and call Alert() with it in supervisor mode.
	 */
	if (iet->iet_AlertCode)
	    trapNum = iet->iet_AlertCode;
	/*
	 * Workaround for i386-native. There trap handler already runs in user mode (BAD!),
	 * and it gets NULL as context pointer (this port saves CPU context in own format,
	 * which is TWICE BAD!!!)
	 * All this needs to be fixed.
	 */
	else if (ctx)
	{
	    /*
	     * Otherwise we can try to send the crash to user level.
	     *
	     * First mark crash condition, and also specify alert code for user-level handler.
	     * If we double-crash while jumping to user mode, we will know this (iet_AlertCode will
	     * already be set)
	     */
	    iet->iet_AlertCode = trapNum;

	    /* Location is our PC, where we crashed */
	    iet->iet_AlertLocation = (APTR)ctx->PC;
	    /* Remember also stack frame for backtrace */
	    iet->iet_AlertStack = (APTR)ctx->FP;

	    /*
	     * This is a CPU alert. We've got a full CPU context, so we remember it
	     * in our ETask structure for displaying to the user later.
	     * Note that we store only GPR part of the context. We don't copy
	     * attached FPU data (if any). This can be considered TODO.
	     */
	    iet->iet_AlertType = AT_CPU;
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

    Alert(trapNum);
} /* TrapHandler */
