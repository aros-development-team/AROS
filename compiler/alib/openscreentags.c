/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/11/28 10:40:30  aros
    A couple of new functions in amiga.lib

    Easier code to handle stacktags and stackmethods.

    Revision 1.2  1996/11/25 10:53:18  aros
    Allow stacktags on special CPUs

    Revision 1.1  1996/09/21 14:10:58  digulla
    New function: OpenScreenTags()


    Desc:
    Lang: english
*/
#include <intuition/intuitionbase.h>
#include "alib_intern.h"

extern struct IntuitionBase * IntuitionBase;

/*****************************************************************************

    NAME */
#include <intuition/screens.h>
#include <clib/intuition_protos.h>

	struct Screen * OpenScreenTags (

/*  SYNOPSIS */
	struct NewScreen * newScreen,
	unsigned long	   tag1,
	...)

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
    OpenScreenTagList (newScreen, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* OpenScreenTags */
