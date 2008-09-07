/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of graphics.library/GetRPAttrsA()
    Lang: english
*/

#include <graphics/rastport.h>
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/graphics.h>

	void GetRPAttrs (

/*  SYNOPSIS */
	struct RastPort * rp,
	Tag tag1,
	...)

/*  FUNCTION
        This is the varargs version of graphics.library/GetRPAttrsA().
        For information see graphics.library/GetRPAttrsA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        graphics.library/GetRPAttrsA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_NR_SLOWSTACKTAGS_PRE(tag1)
    GetRPAttrsA (rp, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_NR_SLOWSTACKTAGS_POST
} /* GetRPAttrs */
