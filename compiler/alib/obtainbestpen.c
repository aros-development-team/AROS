/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Obtain the best pen available for a given color
    Lang: english
*/

#define AROS_TAGRETURNTYPE LONG
#include <graphics/view.h>
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/graphics.h>

	LONG ObtainBestPen (

/*  SYNOPSIS */
	struct ColorMap * cm,
	ULONG R,
	ULONG G,
	ULONG B,
	Tag tag1,
	... )

/*  FUNCTION
        This is the varargs version of graphics.library/ObtainBestPenA().
        For information see graphics.library/ObtainBestPenA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        graphics.library/ObtainBestPenA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = ObtainBestPenA (cm, R,G,B, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* ObtainBestPen */
