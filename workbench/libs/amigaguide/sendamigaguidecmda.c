/*
    (C) 2000 AROS - The Amiga Research OS
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

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, AmigaGuideBase)

#warning TODO: Write amigaguide/SendAmigaGuideCmdA()
    aros_print_not_implemented ("amigaguide/SendAmigaGuideCmdA");

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* SendAmigaGuideCmdA */
