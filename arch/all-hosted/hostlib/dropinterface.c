/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/hostlib.h>

	AROS_LH1I(void, HostLib_DropInterface,

/*  SYNOPSIS */
   	AROS_LHA(APTR *, interface, A0),

/*  LOCATION */
   	APTR , HostLibBase, 6, HostLib)

/*  FUNCTION
	Free an array of symbol values obtained by HostLib_GetInterface()

    INPUTS
	interface - A pointer to values array

    RESULT
	None

    NOTES
	This function appeared in v2 of hostlib.resource.

    EXAMPLE

    BUGS

    SEE ALSO
	HostLib_GetInterface()

    INTERNALS

*****************************************************************************/	 
{
    AROS_LIBFUNC_INIT

    FreeVec(interface);

    AROS_LIBFUNC_EXIT
}
