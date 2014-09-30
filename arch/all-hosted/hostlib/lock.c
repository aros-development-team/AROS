/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>
#include <proto/exec.h>

#include "hostlib_intern.h"

/*****************************************************************************

    NAME */
#include <proto/hostlib.h>

	AROS_LH0(void, HostLib_Lock,

/*  SYNOPSIS */

/*  LOCATION */
	struct HostLibBase *, HostLibBase, 7, HostLib)

/*  FUNCTION
	Acquire global host OS call semaphore.

    INPUTS
	None.

    RESULT
	None.

    NOTES
	Host OS calls are typically not reentrant. You have to
	call this function before you may use any host OS API.
	Use HostLib_Unlock() function when you're done with it.

	This function has negative impact on AROS multitasking, so
	use it gently.

    EXAMPLE

    BUGS

    SEE ALSO
	HostLib_Unlock()

    INTERNALS
	The actual implementation of this function depends on
	the host OS. Do not assume anything particular about it.
	For example under Windows it's Forbid(), not a semaphore.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    HOSTLIB_LOCK();

    AROS_LIBFUNC_EXIT
}
