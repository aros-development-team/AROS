/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Open a window
    Lang: english
*/
#define AROS_TAGRETURNTYPE  struct Window *

#include <intuition/intuitionbase.h>
#include "alib_intern.h"

extern struct IntuitionBase * IntuitionBase;

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <clib/intuition_protos.h>

	struct Window * OpenWindowTags (

/*  SYNOPSIS */
	struct NewWindow * newWindow,
	ULONG		   tag1,
	...		   )

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = OpenWindowTagList (newWindow, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* OpenWindowTags */
