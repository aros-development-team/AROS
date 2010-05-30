/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create a gadtools menu
    Lang: english
*/

#define AROS_TAGRETURNTYPE struct Menu *

#include "alib_intern.h"

extern struct Library * GadToolsBase;

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <utility/tagitem.h>
#include <libraries/gadtools.h>
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/gadtools.h>

	struct Menu * CreateMenus (

/*  SYNOPSIS */
	struct NewMenu * newmenu,
	Tag		 tag1,
	...		)

/*  FUNCTION
        Varargs version of gadtools.library/CreateMenusA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	gadtools.library/CreateMenusA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = CreateMenusA (newmenu, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* CreateMenus */
