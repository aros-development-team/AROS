/*
    Copyright © 1998-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AllocMiscResource() function.
    Lang: english
*/
#include <resources/misc.h>
#include <proto/exec.h>
#include "misc_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH1(VOID , FreeMiscResource,
		 AROS_LHA(ULONG,  unitNum, d0),
/*  SYNOPSIS */

/*  LOCATION */
	APTR, MiscBase, 2, Misc)

/*  FUNCTION

    Frees one of the miscellaneous resources.

    INPUTS

    unitNum  --  The resource to free.

    RESULT

    NOTES

    You must have allocated the resource to free it!

    EXAMPLE

    BUGS

    SEE ALSO

    AllocMiscResource()

    INTERNALS

    HISTORY

    23.7.98   SDuvan implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ObtainSemaphore(&GPB(MiscBase)->mb_Lock);

    GPB(MiscBase)->mb_owners[unitNum] = NULL;

    ReleaseSemaphore(&GPB(MiscBase)->mb_Lock);

    AROS_LIBFUNC_EXIT
} /* FreeMiscResource */



