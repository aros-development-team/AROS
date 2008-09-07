/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of amigaguide.library/OpenAmigaGuideAsyncA()
    Lang: english
*/

#define AROS_TAGRETURNTYPE AMIGAGUIDECONTEXT
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/amigaguide.h>

	AMIGAGUIDECONTEXT OpenAmigaGuideAsync (

/*  SYNOPSIS */
	struct NewAmigaGuide * nag,
	Tag tag1,
	...)

/*  FUNCTION
        This is the varargs version of amigaguide.library/OpenAmigaGuideAsyncA().
        For information see amigaguide.library/OpenAmigaGuideAsyncA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        amigaguide.library/OpenAmigaGuideAsyncA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = OpenAmigaGuideAsyncA (nag, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* OpenAmigaGuideAsync */
