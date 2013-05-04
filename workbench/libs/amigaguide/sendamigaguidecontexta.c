/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AmigaGuide function SendAmigaGuideContextA()
    Lang: english
*/
#include "amigaguide_intern.h"

/*****************************************************************************

    NAME */
#include <proto/amigaguide.h>

        AROS_LH2(BOOL, SendAmigaGuideContextA,

/*  SYNOPSIS */
        AROS_LHA(AMIGAGUIDECONTEXT, handle, A0),
        AROS_LHA(struct TagItem *, attrs, D0),

/*  LOCATION */
        struct Library *, AmigaGuideBase, 16, AmigaGuide)

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

    /* FIXME: amigaguide/SendAmigaGuideContextA() */
    aros_print_not_implemented ("amigaguide/SendAmigaGuideContextA");

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* SendAmigaGuideContextA */
