/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: enable.c 15705 2002-11-11 03:38:29Z bergers $

    Desc: Enable() - Allow interrupts to occur after Disable().
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <asm/registers.h>

#define DEBUG 1
#include <aros/debug.h>

/*****************************************************************************/
#undef Exec
#ifdef UseExecstubs
#    define Exec _Exec
#endif

/*  NAME */
#include <proto/exec.h>

	AROS_LH0(void, Enable,

/*  LOCATION */
	struct ExecBase *, SysBase, 21, Exec)

/*  FUNCTION
	This function will allow interrupts to occur after they have
	been disabled by Disable().

	Note that calls to Disable() nest, and for every call to
	Disable() you need a matching call to Enable().

	***** WARNING *****

	Using this function is considered very harmful, and it is
	not recommended to use this function for ** ANY ** reason.

	It is quite possible to either crash the system, or to prevent
	normal activities (disk/port i/o) from occuring.

	Note: As taskswitching is driven by the interrupts subsystem,
	      this function has the side effect of disabling
	      multitasking.

    INPUTS
	None.

    RESULT
	Interrupts will be enabled again when this call returns.

    NOTES
	This function preserves all registers.

	To prevent deadlocks calling Wait() in disabled state breaks
	the disable - thus interrupts may happen again.

    EXAMPLE
	No you DEFINITATELY don't want to use this function.

    BUGS
	The only architecture that you can rely on the registers being
	saved is on the Motorola mc68000 family.

    SEE ALSO
	Forbid(), Permit(), Disable(), Wait()

    INTERNALS
	This function must be replaced in the $(KERNEL) or $(ARCH)
	directories in order to do some work.

    HISTORY

******************************************************************************/
{
#undef Exec

        AROS_LIBFUNC_INIT

	if( --SysBase->IDNestCnt < 0) {
		/*
		 * Enable interrupt by allowing all of them.
		 */
		SetSR(0,0x700);
	}

	AROS_LIBFUNC_EXIT
} /* Enable() */
