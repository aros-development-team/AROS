/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
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
#ifdef AROS_SLOWSTACKTAGS
    ULONG	     retval;
    va_list	     args;
    struct TagItem * tags;

    va_start (args, tag1);

    if ((tags = GetTagsFromStack (tag1, args)))
    {
	retval = OpenWindowTagList (newWindow, tags);

	FreeTagsFromStack (tags);
    }
    else
	retval = 0L; /* fail :-/ */

    va_end (args);

    return retval;
#else
    return OpenWindowTagList (newWindow, (struct TagItem *)&tag1);
#endif
} /* OpenWindowTags */
