/*
    Copyright © <year>, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/pertask_rel.h>

/*****************************************************************************

    NAME */
        AROS_LH0(APTR, PertaskGetParentBase,

/*  SYNOPSIS */

/*  LOCATION */
        struct Library *, UserelBase, 6, Userel)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return GetParentBase();

    AROS_LIBFUNC_EXIT
}

