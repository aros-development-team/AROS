/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of workbench.library/OpenWorkbenchObjectA()
    Lang: english
*/

#define AROS_TAGRETURNTYPE BOOL
#include <dos/bptr.h>
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/workbench.h>

        BOOL OpenWorkbenchObject(

/*  SYNOPSIS */
        STRPTR name,
        Tag    tag1,
        ...)

/*  FUNCTION
        This is the varargs version of workbench.library/OpenWorkbenchObjectA().
        For information see workbench.library/OpenWorkbenchObjectA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        workbench/OpenWorkbenchObjectA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    return OpenWorkbenchObjectA( name, &tag1);
} /* OpenWorkbenchObject */
