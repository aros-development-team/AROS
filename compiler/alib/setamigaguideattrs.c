/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of amigaguide.library/SetAmigaGuideAttrsA()
    Lang: english
*/

#define AROS_TAGRETURNTYPE LONG
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/amigaguide.h>

	LONG SetAmigaGuideAttrs (

/*  SYNOPSIS */
	AMIGAGUIDECONTEXT handle,
	Tag tag1,
	...)

/*  FUNCTION
        This is the varargs version of amigaguide.library/SetAmigaGuideAttrsA().
        For information see amigaguide.library/SetAmigaGuideAttrsA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        amigaguide.library/SetAmigaGuideAttrsA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = SetAmigaGuideAttrsA (handle, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* SetAmigaGuideAttrs */
