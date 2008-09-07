/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of graphics.library/BestModeIDA()
    Lang: english
*/
#define AROS_TAGRETURNTYPE ULONG
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/graphics.h>

	ULONG BestModeID (

/*  SYNOPSIS */
	Tag tag1,
	...)

/*  FUNCTION
        This is the varargs version of graphics.library/BestModeIDA().
        For information see graphics.library/BestModeIDA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        graphics.library/BestModeIDA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = BestModeIDA (AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* BestModeID */
