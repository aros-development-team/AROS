/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Modify the SR of the CPU (if supported)
    Lang: english
*/

/******************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH2(ULONG, SetSR,

/*  SYNOPSIS */
	AROS_LHA(ULONG, newSR, D0),
	AROS_LHA(ULONG, mask, D1),

/*  LOCATION */
	struct ExecBase *, SysBase, 24, Exec)

/*  FUNCTION
	Change the contents of the CPUs SR (status register).

    INPUTS
	newSR - New values for bits specified in the mask.
	mask - Bits to be changed. All other bits are not effected.

    RESULT
	The entire status register before it was changed or ~0UL if this
	function is not supported.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    return ~0UL;
} /* SetSR */

