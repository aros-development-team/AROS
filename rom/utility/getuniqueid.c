/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include "intern.h"

/*****************************************************************************

    NAME */
#include <proto/utility.h>

	AROS_LH0(ULONG, GetUniqueID,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 45, Utility)

/*  FUNCTION
	Returns a unique id that is different from any other id that is
	obtained from this function call.

    INPUTS

    RESULT
	an unsigned long id

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	Calls Disable()/Enable() to guarentee uniqueness.

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h
	17-08-96    iaint   Reimplemented. CVS lost my old one. Well I did.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    ULONG ret;

    Disable();

    ret = ++(GetIntUtilityBase(UtilityBase)->ub_LastID);

    Enable();

    return ret;

    AROS_LIBFUNC_EXIT
} /* GetUniqueID */
