/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Layout a gadtools menu
    Lang: english
*/

#define AROS_TAGRETURNTYPE BOOL

#include "alib_intern.h"

extern struct Library * GadToolsBase;

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <utility/tagitem.h>
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/gadtools.h>

	BOOL LayoutMenus (

/*  SYNOPSIS */
	struct Menu * menu,
	APTR          vi,
	Tag	      tag1,
	...	      )

/*  FUNCTION
        Varargs version of gadtools.library/LayoutMenusA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	gadtools.library/LayoutMenusA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = LayoutMenusA (menu, vi, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* LayoutMenus */
