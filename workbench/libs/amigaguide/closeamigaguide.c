/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AmigaGuide function CloseAmigaGuide()
    Lang: english
*/
#include "amigaguide_intern.h"

/*****************************************************************************

    NAME */
#include <proto/amigaguide.h>

        AROS_LH1(void, CloseAmigaGuide,

/*  SYNOPSIS */
        AROS_LHA(AMIGAGUIDECONTEXT, handle, A0),

/*  LOCATION */
        struct Library *, AmigaGuideBase, 11, AmigaGuide)

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

    /* FIXME:  amigaguide/CloseAmigaGuide() */
    aros_print_not_implemented ("amigaguide/CloseAmigaGuide");

    AROS_LIBFUNC_EXIT
} /* CloseAmigaGuide */
