/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of amigaguide.library/SetAmigaGuideContextA()
    Lang: english
*/

#define AROS_TAGRETURNTYPE BOOL
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/amigaguide.h>

	BOOL SetAmigaGuideContext (

/*  SYNOPSIS */
	AMIGAGUIDECONTEXT handle,
	ULONG context,
	Tag tag1,
	...)

/*  FUNCTION
        This is the varargs version of amigaguide.library/SetAmigaGuideContextA().
        For information see amigaguide.library/SetAmigaGuideContextA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        amigaguide.library/SetAmigaGuideContextA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = SetAmigaGuideContextA (handle, context, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* SetAmigaGuideContext */
