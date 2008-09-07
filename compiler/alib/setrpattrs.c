/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of graphics.library/SetRPAttrsA()
    Lang: english
*/

#include <graphics/rastport.h>
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/graphics.h>

	void SetRPAttrs (

/*  SYNOPSIS */
	struct RastPort * rp,
	Tag tag1,
	...)

/*  FUNCTION
        This is the varargs version of graphics.library/SetRPAttrsA().
        For information see graphics.library/SetRPAttrsA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        graphics.library/SetRPAttrsA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_NR_SLOWSTACKTAGS_PRE(tag1)
    SetRPAttrsA (rp, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_NR_SLOWSTACKTAGS_POST
} /* SetRPAttrs */
