/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of amigaguide.library/AddAmigaGuideHostA()
    Lang: english
*/
#define AROS_TAGRETURNTYPE AMIGAGUIDEHOST
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/amigaguide.h>

	AMIGAGUIDEHOST AddAmigaGuideHost (

/*  SYNOPSIS */
	struct Hook * hook,
	STRPTR name,
	Tag tag1,
	...)

/*  FUNCTION
        This is the varargs version of amigaguide.library/AddAmigaGuideHostA().
        For information see amigaguide.library/AddAmigaGuideHostA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        amigaguide.library/AddAmigaGuideHostA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = AddAmigaGuideHostA (hook, name, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* AddAmigaGuideHost */
