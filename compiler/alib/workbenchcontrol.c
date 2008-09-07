/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of workbench.library/WorkbenchControlA()
    Lang: english
*/

#define AROS_TAGRETURNTYPE BOOL
#include <dos/bptr.h>
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/workbench.h>

        BOOL WorkbenchControl(

/*  SYNOPSIS */
        STRPTR name,
        Tag    tag1,
        ...)

/*  FUNCTION
        This is the varargs version of workbench.library/WorkbenchControlA().
        For information see workbench.library/WorkbenchControlA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        workbench.library/WorkbenchControlA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = WorkbenchControlA( name, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* WorkbenchControl */
