/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AmigaGuide function SendAmigaGuideCmdA()
    Lang: english
*/
#include "amigaguide_intern.h"

/*****************************************************************************

    NAME */
#include <proto/amigaguide.h>

        AROS_LH3(BOOL, SendAmigaGuideCmdA,

/*  SYNOPSIS */
        AROS_LHA(AMIGAGUIDECONTEXT, handle, A0),
        AROS_LHA(STRPTR, cmd, D0),
        AROS_LHA(struct TagItem *, attrs, D1),

/*  LOCATION */
        struct Library *, AmigaGuideBase, 17, AmigaGuide)

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

    /* FIXME: amigaguide/SendAmigaGuideCmdA() */
    aros_print_not_implemented ("amigaguide/SendAmigaGuideCmdA");

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* SendAmigaGuideCmdA */
