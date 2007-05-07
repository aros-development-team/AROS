/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/exec.h>

#include "camd_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH1(APTR, LockCAMD,

/*  SYNOPSIS */
	AROS_LHA(ULONG, locktype, D0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 5, Camd)

/*  FUNCTION
		Locks the internal lists in camd.
		You must call UnlockCAMD later.

    INPUTS
		locktype - Only CD_Linkages is legal.

    RESULT
		APTR to send to UnlockCAMD

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	ObtainSemaphoreShared(CB(CamdBase)->CLSemaphore);
	return CB(CamdBase)->CLSemaphore;

   AROS_LIBFUNC_EXIT
}

