/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AmigaGuide function LoadXRef()
    Lang: english
*/
#include "amigaguide_intern.h"

/*****************************************************************************

    NAME */
#include <proto/amigaguide.h>

        AROS_LH2(LONG, LoadXRef,

/*  SYNOPSIS */
        AROS_LHA(BPTR, lock, A0),
        AROS_LHA(STRPTR, name, A1),

/*  LOCATION */
        struct Library *, AmigaGuideBase, 21, AmigaGuide)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS
        This function is unimplemented.

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* FIXME: amigaguide/LoadXRef() */
    aros_print_not_implemented ("amigaguide/LoadXRef");

    return 0;

    AROS_LIBFUNC_EXIT
} /* LoadXRef */
