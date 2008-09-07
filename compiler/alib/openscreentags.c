/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Open a screen
    Lang: english
*/

#define AROS_TAGRETURNTYPE  struct Screen *
#include <intuition/intuitionbase.h>
#include "alib_intern.h"

extern struct IntuitionBase * IntuitionBase;

/*****************************************************************************

    NAME */
#include <intuition/screens.h>
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/intuition.h>

	struct Screen * OpenScreenTags (

/*  SYNOPSIS */
	struct NewScreen *  newScreen,
	Tag                 tag1,
	...)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	intuition.library/OpenScreenTagList()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = OpenScreenTagList (newScreen, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* OpenScreenTags */
