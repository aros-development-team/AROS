/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AmigaGuide function LockAmigaGuideBase()
    Lang: english
*/
#include "amigaguide_intern.h"

/*****************************************************************************

    NAME */
#include <proto/amigaguide.h>

        AROS_LH1(LONG, LockAmigaGuideBase,

/*  SYNOPSIS */
        AROS_LHA(AMIGAGUIDECONTEXT, handle, A0),

/*  LOCATION */
        struct Library *, AmigaGuideBase, 6, AmigaGuide)

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

    /* FIXME: amigaguide/LockAmigaGuideBase() */
    aros_print_not_implemented ("amigaguide/LockAmigaGuideBase");

    return 0;

    AROS_LIBFUNC_EXIT
} /* LockAmigaGuideBase */
