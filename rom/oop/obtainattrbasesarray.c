/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Obtain array of attribute base IDs
    Lang: english
*/

#include <proto/exec.h>
#include <exec/memory.h>
#include <aros/debug.h>

#include "intern.h"
#include "hash.h"

/*****************************************************************************

    NAME */

#include <proto/oop.h>
#include <oop/oop.h>

	AROS_LH2(ULONG, OOP_ObtainAttrBasesArray,

/*  SYNOPSIS */
	AROS_LHA(OOP_AttrBase *, bases, A0),
        AROS_LHA(CONST_STRPTR const *, ids, A1),

/*  LOCATION */
	struct Library *, OOPBase, 23, OOP)

/*  FUNCTION
        Obtain several attribute base IDs, storing them in linear array.

    INPUTS
        bases - a pointer to array to fill in
        ids   - a NULL-terminated array of interface IDs

    RESULT
        Zero on success or number of failed bases on failure. Failed
        entries will be set to 0.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        OOP_ReleaseAttrBasesArray

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG failed = 0;

    while (*ids)
    {
        *bases = OOP_ObtainAttrBase(*ids);

	if (*bases == 0)
            failed++;

        bases++;
        ids++;
    }

    return failed;

    AROS_LIBFUNC_EXIT
}
