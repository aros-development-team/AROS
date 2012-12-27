/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ColdReboot() - Reboot the computer.
    Lang: english
*/

#include <aros/debug.h>

#include "exec_util.h"

/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(void, ColdReboot,

/*  LOCATION */
	struct ExecBase *, SysBase, 121, Exec)

/*  FUNCTION
	This function will reboot the computer.

    INPUTS
	None.

    RESULT
	This function does not return.

    NOTES
	It can be quite harmful to call this function. It may be possible that
	you will lose data from other tasks not having saved, or disk buffers
	not being flushed. Plus you could annoy the (other) users.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	This function is not really necessary, and could be left unimplemented
	on many systems. It is best when using this function to allow the memory
	contents to remain as they are, since some programs may use this
	function when installing resident modules.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    Exec_DoResetCallbacks((struct IntExecBase *)SysBase, SD_ACTION_WARMREBOOT);

    AROS_LIBFUNC_EXIT
} /* ColdReboot() */
