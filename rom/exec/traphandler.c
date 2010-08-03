/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Default trap handler
    Lang: english
*/

#include <exec/alerts.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include <proto/arossupport.h>

#include "etask.h"

/* In original AmigaOS the trap handler is entered in supervisor mode with the
 * following on the supervisor stack:
 *  0(sp).l = trap#
 *  4(sp) Processor dependent exception frame
 * This generic implementation is quite incomplete. See arch/all-mingw32 for
 * more correct and complete one.
 */

void Exec_TrapHandler(ULONG trapNum)
{
    struct Task *task = SysBase->ThisTask;
    struct IntETask *iet;
    
    /* Our situation is deadend */
    trapNum |= AT_DeadEnd;

    if (task)
    {
	/* Protection against double-crash. If the alert code is already specified, we have
	   a second crash during processing the first one. Then we just pick up initial alert code
	   and just call Alert(). */
        iet = GetIntETask(task);
	if (iet->iet_AlertCode)
	    trapNum = iet->iet_AlertCode;
    }

    Alert(AT_DeadEnd | trapNum);
} /* TrapHandler */
