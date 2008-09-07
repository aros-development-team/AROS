/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of AllocAslRequestA()
    Lang: english
*/
#define AROS_TAGRETURNTYPE APTR
#include <utility/tagitem.h>


/*****************************************************************************

    NAME */
#include <libraries/asl.h>
#include <proto/asl.h>
extern struct Library *AslBase;
#undef AllocAslRequestTags /* Get rid of the macro from inline/ */

	APTR AllocAslRequestTags (

/*  SYNOPSIS */
	ULONG   reqType,
	Tag     tag1,
	...)

/*  FUNCTION
	This is the varargs version of the asl.library AllocAslRequest().
	For information see asl.library/AllocAslRequest().

    INPUTS
	reqType     -   Type of requester to allocate.
	tag1        -   TagList of extra arguments.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	asl.library/AllocAslRequest()

    INTERNALS

    HISTORY
	23-03-2000  bernie  Wrote.

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = AllocAslRequest(reqType, AROS_SLOWSTACKTAGS_ARG(tag1));

    AROS_SLOWSTACKTAGS_POST

} /* AllocAslRequestTags */
