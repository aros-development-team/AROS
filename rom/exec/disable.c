/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Disable() - Stop interrupts from occurring.
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/atomic.h>
#include <proto/kernel.h>

#include "exec_intern.h"

/*****************************************************************************/
#undef  Exec
#ifdef UseExecstubs
#    define Exec _Exec
#endif

/*  NAME */
#include <proto/exec.h>

	AROS_LH0(void, Disable,

/*  LOCATION */
	struct ExecBase *, SysBase, 20, Exec)

/*  FUNCTION
	This function will prevent interrupts from occuring. You can
	start the interrupts again with a call to Enable().

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

    RESULT
	Interrupts will be disabled AFTER this call returns.

    NOTES
	This function preserves all registers.

	To prevent deadlocks calling Wait() in disabled state breaks
	the disable - thus interrupts may happen again.

    EXAMPLE
	No you DEFINITELY don't want to use this function.

    BUGS
	The only architecture that you can rely on the registers being
	saved is on the Motorola mc68000 family.

    SEE ALSO
	Forbid(), Permit(), Enable(), Wait()

    INTERNALS
	This function must be replaced in the $(KERNEL) or $(ARCH)
	directories in order to do some work.

******************************************************************************/
{
#undef Exec

    AROS_LIBFUNC_INIT

    if (KernelBase)
	KrnCli();

#ifdef AROS_NO_ATOMIC_OPERATIONS
    /* Generic atomic operations in aros/atomic.h rely on Disable()/Enable() themselves,
       so we have to take care about it here. Otherwise we fall into endless loop.
       It is strongly recommended to implement real atomic operations in assembler.
       The same is done in Enable() */
    SysBase->IDNestCnt++;
#else
    AROS_ATOMIC_INC(SysBase->IDNestCnt);
#endif

    AROS_LIBFUNC_EXIT
} /* Disable() */
