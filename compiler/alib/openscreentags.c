/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
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
#ifdef AROS_SLOWSTACKTAGS
    ULONG	     retval;
    va_list	     args;
    struct TagItem * tags;

    va_start (args, tag1);

    if ((tags = GetTagsFromStack (tag1, args)))
    {
	retval = OpenScreenTagList (newScreen, tags);

	FreeTagsFromStack (tags);
    }
    else
	retval = 0L; /* fail :-/ */

    va_end (args);

    return retval;
#else
    return OpenScreenTagList (newScreen, (struct TagItem *)&tag1);
#endif
} /* OpenScreenTags */
