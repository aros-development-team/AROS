/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/execbase.h>

/******************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(void, Permit,

/*  LOCATION */
	struct ExecBase *, SysBase, 23, Exec)

/*  FUNCTION
	This function activates the dispatcher again after a call to Permit().

    INPUTS
	None.

    RESULT
	None.

    NOTES
	This function preserves all registers.

    EXAMPLE

    BUGS

    SEE ALSO
	Forbid(), Disable(), Enable()

    INTERNALS

    HISTORY

******************************************************************************/
{
    if ((--SysBase->TDNestCnt) < 0
	&& (SysBase->AttnResched & 0x80)
	&& SysBase->IDNestCnt < 0
    )
    {
	SysBase->AttnResched &= ~0x80;

	Switch ();
    }
} /* Permit */
