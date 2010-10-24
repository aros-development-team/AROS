/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Switch() - Switch to the next available task.
    Lang: english
*/

#include <exec/execbase.h>

#include "exec_intern.h"

/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(void, Switch,

/*  LOCATION */
	struct ExecBase *, SysBase, 9, Exec)

/*  FUNCTION
	This function is obsolete and subject to removal.
	On AmigaOS(tm) this was a private function.

    INPUTS

    RESULT

    NOTES
	This function is still there because i386-native port
	still uses it.

    EXAMPLE

    BUGS

    SEE ALSO
	Dispatch(), Reschedule()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *task = SysBase->ThisTask;

    asm volatile ("or.w #0x2300,%sr\n");	// Disable CPU interrupts

    asm volatile (
	"  move.l   %%a0,%%sp@-\n"			// Save A0 to stack
	"  move.l   %%usp,%%a0\n"			// Use the user stack
        "  movem.l  %%d0-%%d7/%%a0-%%a6,%%a0@-\n"	// Save everything
        "  move.l   #Exec_Switch_Restore,%%a0@-\n"	// Where to return to
        "  move.w   %%sr,%%a0@-\n"			// Save %sr
        "  move.l   %%a0,%0\n"				// Save stack reg to task
        "  move.l   %%sp@,%%a0@(4*8)\n"                 // Fix up A0 in the saved regs
        "  move.l   %%sp@+,%%a0\n"                      // Restore A0
        : "=m" (task->tc_SPReg)
        :
        : );

    /* Reset Enable()/Disable() locking */
    task->tc_IDNestCnt = SysBase->IDNestCnt;
    SysBase->IDNestCnt = -1;
    asm volatile ("move.w #0xc000,0xdff09a\n");	// Enable device interrupts

    if (task->tc_Flags & TB_SWITCH)
    	    AROS_UFC0(void, task->tc_Switch);

    Dispatch();	/* We don't return from this! */

    D(bug("Exec_Switch: --- IMPOSSIBLE EXECUTION --\n"));

    /* Restore entry context - we 'come from' a RTE on
     * the user stack after a dispatch.
     *
     * Note that Dispatch() has kindly restored
     * our registers D0-D7/A0-A6 for us.
     */
    asm volatile (
    	"Exec_Switch_Restore:\n"
	);

    AROS_LIBFUNC_EXIT
} /* Switch() */
