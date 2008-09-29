/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Get a (private) visual info structure
    Lang: english
*/

#include <exec/types.h>
#define AROS_TAGRETURNTYPE APTR

#include "alib_intern.h"

extern struct Library * GadToolsBase;

/*****************************************************************************

    NAME */
#include <intuition/screens.h>
#include <utility/tagitem.h>
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/gadtools.h>

	APTR GetVisualInfo (

/*  SYNOPSIS */
	struct Screen * screen,
	Tag		tag1,
	...		)

/*  FUNCTION
        Varargs version of gadtools.library/GetVisualInfoA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	gadtools.library/GetVisualInfoA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = GetVisualInfoA (screen, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* GetVisualInfo */
