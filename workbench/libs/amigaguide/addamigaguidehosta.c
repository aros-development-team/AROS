/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AmigaGuide function AddAmigaGuideHostA()
    Lang: english
*/
#include "amigaguide_intern.h"

/*****************************************************************************

    NAME */
#include <proto/amigaguide.h>

        AROS_LH3(AMIGAGUIDEHOST, AddAmigaGuideHostA,

/*  SYNOPSIS */
        AROS_LHA(struct Hook *, hook, A0),
        AROS_LHA(STRPTR, name, D0),
        AROS_LHA(struct TagItem *, attrs, A1),
	
/*  LOCATION */
        struct Library *, AmigaGuideBase, 23, AmigaGuide)

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

    /* FIXME: Write amigaguide/AddAmigaGuideHostA() */
    aros_print_not_implemented ("amigaguide/AddAmigaGuideHostA");

    return NULL;

    AROS_LIBFUNC_EXIT
} /* AddAmigaGuideHostA */
