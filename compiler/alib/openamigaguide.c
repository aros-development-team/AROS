/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of amigaguide.library/OpenAmigaGuideA()
    Lang: english
*/

#define AROS_TAGRETURNTYPE AMIGAGUIDECONTEXT
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/amigaguide.h>

	AMIGAGUIDECONTEXT OpenAmigaGuide (

/*  SYNOPSIS */
	struct NewAmigaGuide * nag,
	Tag tag1,
	...)

/*  FUNCTION
        This is the varargs version of amigaguide.library/OpenAmigaGuideA().
        For information see amigaguide.library/OpenAmigaGuideA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        amigaguide.library/OpenAmigaGuideA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = OpenAmigaGuideA (nag, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* OpenAmigaGuide */
