/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AmigaGuide function RemoveAmigaGuideHostA()
    Lang: english
*/
#include "amigaguide_intern.h"

/*****************************************************************************

    NAME */
#include <proto/amigaguide.h>

        AROS_LH2(LONG, RemoveAmigaGuideHostA,

/*  SYNOPSIS */
        AROS_LHA(AMIGAGUIDEHOST, key, A0),
        AROS_LHA(struct TagItem *, attrs, A1),

/*  LOCATION */
        struct Library *, AmigaGuideBase, 24, AmigaGuide)

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

    /* FIXME: amigaguide/RemoveAmigaGuideHostA() */
    aros_print_not_implemented ("amigaguide/RemoveAmigaGuideHostA");

    return 0;

    AROS_LIBFUNC_EXIT
} /* RemoveAmigaGuideHostA */
