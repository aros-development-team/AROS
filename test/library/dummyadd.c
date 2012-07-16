/*
    Copyright © <year>, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <proto/dummy_rel.h>

/*****************************************************************************

    NAME */
        AROS_LH2(ULONG, DummyAdd,

/*  SYNOPSIS */
        AROS_LHA(ULONG, a, D0),
        AROS_LHA(ULONG, b, D1),

/*  LOCATION */
        struct Library *, UserelBase, 5, Userel)

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

    return add(a,b);

    AROS_LIBFUNC_EXIT
}

