/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Obtain array of method ID bases
    Lang: english
*/

/*****************************************************************************

    NAME */

#include <proto/oop.h>
#include <oop/oop.h>

	AROS_LH2(ULONG, OOP_ObtainMethodBasesArray,

/*  SYNOPSIS */
	AROS_LHA(OOP_MethodID *, bases, A0),
        AROS_LHA(CONST_STRPTR *, ids, A1),

/*  LOCATION */
	struct Library *, OOPBase, 25, OOP)

/*  FUNCTION
        Obtain several method ID bases, storing them in linear array.

    INPUTS
        bases - a pointer to array to fill in
        ids   - a NULL-terminated array of interface IDs

    RESULT
        Zero on success or number of failed bases on failure. Failed array
        entries will be set to -1.

    NOTES
        Method IDs are owned by particular class, and are released when
        the class is destroyed. Thus, there is no ReleaseMethodBasesArray()
        function.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG failed = 0;

    while (*ids)
    {
        *bases = OOP_GetMethodID(*ids, 0);

	if (*bases == -1)
            failed++;

        bases++;
        ids++;
    }

    return failed;

    AROS_LIBFUNC_EXIT
}
