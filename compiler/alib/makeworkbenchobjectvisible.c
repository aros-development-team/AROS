/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of workbench.library/MakeWorkbenchObjectVisibleA()
    Lang: english
*/

#define AROS_TAGRETURNTYPE BOOL
#include <dos/bptr.h>
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/workbench.h>

        BOOL MakeWorkbenchObjectVisible(

/*  SYNOPSIS */
        STRPTR name,
        Tag    tag1,
        ...)

/*  FUNCTION
        This is the varargs version of workbench.library/MakeWorkbenchObjectVisibleA().
        For information see workbench.library/MakeWorkbenchObjectVisibleA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        workbench.library/MakeWorkbenchObjectVisibleA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = MakeWorkbenchObjectVisibleA( name, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* MakeWorkbenchObjectVisibleA */
