/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Disable() - Stop interrupts from occurring.
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/atomic.h>

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

    /* Only disable interrupts if they are not already disabled. The
       initial (enabled) value of IDNestCnt is -1
    */
    
    AROS_ATOMIC_INC(SysBase->IDNestCnt);
    
    /*
	We have to disable interrupts, however some silly person
	hasn't written the code required to do it yet. They should
	have created a file in config/$(KERNEL)/exec or
	config/$(ARCH)/exec called disable.c or disable.s which
	implements this function.
    */
    #ifndef __CXREF__
    #error You have not written the $(KERNEL) interrupt subsystem!
    #endif

    AROS_LIBFUNC_EXIT
} /* Disable() */
