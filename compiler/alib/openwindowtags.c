/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/11/28 10:40:30  aros
    A couple of new functions in amiga.lib

    Easier code to handle stacktags and stackmethods.

    Revision 1.4  1996/11/25 10:53:18  aros
    Allow stacktags on special CPUs

    Revision 1.3  1996/09/21 14:09:10  digulla
    No need for __AROS macros

    Revision 1.2  1996/09/17 18:05:45  digulla
    Same names for same parameters

    Revision 1.1  1996/09/17 16:19:00  digulla
    New function: OpenWindowTags()


    Desc:
    Lang: english
*/
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
    OpenWindowTagList (newWindow, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* OpenWindowTags */
