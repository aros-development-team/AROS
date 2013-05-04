/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AmigaGuide function UnlockAmigaGuideBase()
    Lang: english
*/
#include "amigaguide_intern.h"

/*****************************************************************************

    NAME */
#include <proto/amigaguide.h>

        AROS_LH1(void, UnlockAmigaGuideBase,

/*  SYNOPSIS */
        AROS_LHA(LONG, key, D0),

/*  LOCATION */
        struct Library *, AmigaGuideBase, 7, AmigaGuide)

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

    /* FIXME: amigaguide/UnlockAmigaGuideBase() */
    aros_print_not_implemented ("amigaguide/UnlockAmigaGuideBase");

    AROS_LIBFUNC_EXIT
} /* UnlockAmigaGuideBase */
