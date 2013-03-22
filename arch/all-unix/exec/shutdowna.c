/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ShutdownA() - Shut down the operating system.
    Lang: english
*/

#include <aros/debug.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_util.h"

/*****************************************************************************

    NAME */

	AROS_LH1(ULONG, ShutdownA,

/*  SYNOPSIS */
	AROS_LHA(ULONG, action, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 173, Exec)

/*  FUNCTION
	This function will shut down the operating system.

    INPUTS
	action - what to do:
	 * SD_ACTION_POWEROFF   - power off the machine.
	 * SD_ACTION_COLDREBOOT - cold reboot the machine (not only AROS).

    RESULT
	This function does not return in case of success. Otherwise is returns
	zero.

    NOTES
	It can be quite harmful to call this function. It may be possible that
	you will lose data from other tasks not having saved, or disk buffers
	not being flushed. Plus you could annoy the (other) users.

    EXAMPLE

    BUGS

    SEE ALSO
	ColdReboot()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    int exitcode;

    switch(action)
    {
    case SD_ACTION_POWEROFF:
    	exitcode = 0;
	break;

    case SD_ACTION_COLDREBOOT:
    	exitcode = 0x81; /* Magic value for our bootstrap */
    	break;

    default:
    	return 0; /* Unknown action code */
    }

    Exec_DoResetCallbacks((struct IntExecBase *)SysBase, action);

    PD(SysBase).SysIFace->exit(exitcode);
    AROS_HOST_BARRIER

    /* Just shut up the compiler, we won't return */
    return 0;

    AROS_LIBFUNC_EXIT
}
