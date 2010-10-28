/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Switch() - Switch to the next available task.
    Lang: english
*/

#include <aros/kernel.h>
#include <aros/debug.h>

#include <exec/alerts.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_syscall.h>
#include <kernel_scheduler.h>

#if defined(DEBUG) && (DEBUG == 1)
#define D(x) x
#else
#define D(x)
#endif

extern void Kernel_KrnSwitch_Helper(void);
asm (
	"  .text\n"
	"  .align 4\n"
	"  .globl Kernel_KrnSwitch_Helper\n"
	"Kernel_KrnSwitch_Helper:\n"
	"  move.l   %a0,  %sp@-\n"			// Save A0 to stack
	"  move.l   %usp, %a0\n"			// Use the user stack
        "  move.l   %sp@(4+2),%a0@-\n"			// Where to return to
        "  move.w   %sp@(4), %a0@-\n"			// Save %sr
        "  movem.l  %d0-%d7/%a0-%a6, %a0@-\n"		// Save everything
        "  move.l   %sp@+, %a0@(4*8)\n"		// Fix up A0 in the saved regs
        /* 
         * From now on, we will never return, so we can
         * mess up the registers however we want.
         *
         */
        "  move.l   #_ss_stack_end, %sp\n"		// Zap the Supervisor stack
        "  move.l   (4), %a6\n"				// A6 = SysBase
        "  move.l   %a6@(276 /* ThisTask */), %a5\n"	// A5 = ThisTask
        "  move.l   %usp, %a0\n"			// A0 = User SP
        "  move.l   %a0, %a5@(54 /* tc_SPReg */)\n"	// Store User SP in task
        "  move.l   KernelBase, %a6\n"			// A6 = KernelBase
        "  move.l   %a6,%sp@-\n"			// For the stackcall build
        "  jmp      Kernel_KrnDispatch\n"		// Go to dispatch.
	);

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH0(void, KrnSwitch,

/*  SYNOPSIS */

/*  LOCATION */
         struct KernelBase *, KernelBase, 5, Kernel)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    core_Switch();

    Supervisor(Kernel_KrnSwitch_Helper);

    D(bug("[KrnSwitch] Back in task = %p\n", SysBase->ThisTask));

    /* We'll get back here when we switch back */

    AROS_LIBFUNC_EXIT
}
