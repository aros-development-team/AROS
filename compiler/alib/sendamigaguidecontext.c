/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of amigaguide.library/SendAmigaGuideContextA()
    Lang: english
*/

#define AROS_TAGRETURNTYPE BOOL
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/amigaguide.h>

	BOOL SendAmigaGuideContext (

/*  SYNOPSIS */
	AMIGAGUIDECONTEXT handle,
	Tag tag1,
	...)

/*  FUNCTION
        This is the varargs version of amigaguide.library/SendAmigaGuideContextA().
        For information see amigaguide.library/SendAmigaGuideContextA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        amigaguide.library/SendAmigaGuideContextA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = SendAmigaGuideContextA (handle, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* SendAmigaGuideContext */
