/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <setjmp.h>

#include "dos_intern.h"

/*
 * On m68k exitting a program can be performed by setting SP to pr_ReturnAddr - 12,
 * putting return code into d0 and performing rts.
 * For the complete code, see http://cataclysm.cx/random/amiga/reference/Libraries_Manual_guide/node060F.html
 * Similar construction should work on all CPUs. I guess this is what is used by Exit() routine.
 * Also it is known that ULONG at address taken from pr_ReturnAddr is stack size for CLI processes.
 * For other CPUs we don't keep exact stack layout and consider it private. Only Exit() knows what
 * to do with it. However we keep stack size value where it was, some code relies on it.
 */
struct StackState
{
    IPTR stacksize;
    APTR stackaddr;
    ULONG retval;
    jmp_buf state;
};

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(void, Exit,

/*  SYNOPSIS */
	AROS_LHA(LONG, returnCode, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 24, Dos)

/*  FUNCTION
	Instantly terminate the program.

    INPUTS
	returnCode - Process' return code.

    RESULT
    	None.

    NOTES
	Calling this function bypasses normal termination sequence of your program.
	Automatically opened libraries will not be closed, destructors will not be
	called, etc. Do this only if you really know what are you doing. It's not
	adviced to use this function at all.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Process *me = (struct Process *)FindTask(NULL);
    struct StackState *ss = me->pr_ReturnAddr;

    /* Return code can be zero, so we can't pass it via longjmp() */
    ss->retval = returnCode;

    /* Close dos.library because the program has opened it in order to obtain DOSBase */
    CloseLibrary(&DOSBase->dl_lib);

    /* Disable() because we may have to swap stack limits */
    Disable();
    longjmp(ss->state, 1);

    AROS_LIBFUNC_EXIT
} /* Exit */

/*
 * CallEntry() is also defined here because it is closely associated with
 * Exit() implementation.
 * For different CPUs we may have optimized versions of these two functions in asm.
 */
ULONG CallEntry(STRPTR argptr, ULONG argsize, LONG_FUNC entry, struct Process *me)
{
    struct StackState ss;

    ss.stacksize = me->pr_Task.tc_SPUpper - me->pr_Task.tc_SPLower;
    ss.stackaddr = me->pr_Task.tc_SPLower;
    me->pr_ReturnAddr = &ss;

    if (setjmp(ss.state))
    {
    	/*
    	 * We came here from Exit().
    	 * Restore stack limits because the program mightt have swapped stack.
    	 * We are clever enough to recover from this.
    	 */
    	me->pr_Task.tc_SPLower = ss.stackaddr;
    	me->pr_Task.tc_SPUpper = ss.stackaddr + ss.stacksize;

    	Enable(); /* We Disable()d in Exit() */

    	return ss.retval;
    }
    else
    	return AROS_UFC3(ULONG, entry,
			 AROS_UFCA(STRPTR, argptr, A0),
			 AROS_UFCA(ULONG, argsize, D0),
			 AROS_UFCA(struct ExecBase *, SysBase, A6));
}
