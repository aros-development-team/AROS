/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Release array of attribute ID bases
    Lang: english
*/

/*****************************************************************************

    NAME */

#include <proto/oop.h>
#include <oop/oop.h>

	AROS_LH2(void, OOP_ReleaseAttrBasesArray,

/*  SYNOPSIS */
	AROS_LHA(OOP_AttrBase *, bases, A0),
        AROS_LHA(CONST_STRPTR const *, ids, A1),

/*  LOCATION */
	struct Library *, OOPBase, 24, OOP)

/*  FUNCTION
        Release several attribute ID bases, stored in linear array.

    INPUTS
        bases - a pointer to array of bases
        ids   - a NULL-terminated array of corresponding interface IDs

    RESULT
        None

    NOTES
        It is legal to have some entries in the array not filled in
        (equal to 0). They will be skipped.

    EXAMPLE

    BUGS

    SEE ALSO
        OOP_ObtainAttrBasesArray

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    while (*ids)
    {
        if (*bases)
        {
            OOP_ReleaseAttrBase(*ids);
            *bases = 0;
        }

        bases++;
        ids++;
    }

    AROS_LIBFUNC_EXIT
}
