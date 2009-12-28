/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id: traphandler.c 32170 2009-12-24 15:33:46Z sonic $

    Desc: Default trap handler
    Lang: english
*/

#include <exec/alerts.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include <proto/arossupport.h>

#include "etask.h"
#include "exec_util.h"
#include "../kernel/cpucontext.h"

/* In original AmigaOS the trap handler is entered in supervisor mode with the
 * following on the supervisor stack:
 *  0(sp).l = trap#
 *  4(sp) Processor dependent exception frame
 * In our current implementation we use two pointers on stack. Context structure is
 * currently private because it differs even on the same CPU, however i beleive it
 * should be changed to some generalized form in order to allow applications (debuggers)
 * to use it.
 * This function is arch-specific *ONLY* because struct AROSCPUContext is currently arch-
 * specific. Please don't introduce more differences!
 */

void Exec_TrapHandler(ULONG trapNum, struct AROSCPUContext *ctx)
{
    struct Task *task = SysBase->ThisTask;
    struct IntETask *iet;
    
    /* Our situation is deadend */
    trapNum |= AT_DeadEnd;
    if (task) {
	/* Get internal task structure */
        iet = GetIntETask(task);
	/* Protection against double-crash. If the alert code is already specified, we have
	   a second crash during processing the first one. Then we just pick up initial alert code
	   and just call Alert(). */
	if (iet->iet_LastAlert[0])
	    trapNum = iet->iet_LastAlert[0];
	else {
	    /* Otherwise we can try to send the crash to user level. Set alert code for the task */
	    iet->iet_LastAlert[0] = trapNum;
            /* Make the task to jump to crash handler. We don't care about return address etc because
	       the alert is deadend anyway. */
            ctx->Eip = (IPTR)Exec_CrashHandler;
	    /* Let the task go */
	    return;
	}
    }
    Alert(trapNum);
} /* TrapHandler */
