/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
*/
#include <exec/execbase.h>
#include "callsave.h"

static void _Permit (void);

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
    callsave (_Permit);
} /* Permit */

void _Permit (void)
{
    if ((--SysBase->TDNestCnt) < 0
	&& (SysBase->AttnResched & 0x80)
	&& SysBase->IDNestCnt < 0
    )
    {
	SysBase->AttnResched &= ~0x80;

	Switch ();
    }
} /* _Permit */
