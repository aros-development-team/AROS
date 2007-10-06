/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Draw a bevel box
    Lang: english
*/

#include "alib_intern.h"

extern struct Library * GadToolsBase;

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/gadtools.h>

#include <utility/tagitem.h>

	void DrawBevelBox (

/*  SYNOPSIS */
        struct RastPort * rp,
        WORD              left,
        WORD              top,
        WORD              width,
        WORD              height,
	Tag               tag1,
	...		)

/*  FUNCTION
        Varargs version of gadtools.library/DrawBevelBoxA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_NR_SLOWSTACKTAGS_PRE(tag1)
    DrawBevelBoxA (rp, left, top, width, height, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_NR_SLOWSTACKTAGS_POST
} /* DrawBevelBox */


