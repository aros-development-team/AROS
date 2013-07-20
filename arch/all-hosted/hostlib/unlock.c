#include <aros/libcall.h>
#include <proto/exec.h>

#include "hostlib_intern.h"

/*****************************************************************************

    NAME */
#include <proto/hostlib.h>

	AROS_LH0(void, HostLib_Unlock,

/*  SYNOPSIS */

/*  LOCATION */
	struct HostLibBase *, HostLibBase, 8, HostLib)

/*  FUNCTION
	Release global host OS call semaphore.

    INPUTS
	None.

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	HostLib_Lock()

    INTERNALS
	The actual implementation of this function depends on
	the host OS. Do not assume anything particular about it.
	For example under Windows it's Permit(), not a semaphore.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    HOSTLIB_UNLOCK();

    AROS_LIBFUNC_EXIT
}
