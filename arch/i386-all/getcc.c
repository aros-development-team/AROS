/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Return the contents of the CC register of the CPU (if supported)
    Lang: english
*/

/******************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(ULONG, GetCC,

/*  LOCATION */
	struct ExecBase *, SysBase, 88, Exec)

/*  FUNCTION
	If the CPU has a CC (condition code) register, then this function
	will return it's current contents.

    INPUTS
	None.

    RESULT
	The current value of the CPUs' CC register or ~0UL if this is
	not supported.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    return ~0UL;
} /* GetCC */

