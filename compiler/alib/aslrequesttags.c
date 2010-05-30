/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of AslRequestA()
    Lang: english
*/
#define AROS_TAGRETURNTYPE BOOL
#include <utility/tagitem.h>


/*****************************************************************************

    NAME */
#include <libraries/asl.h>
#include <proto/asl.h>
extern struct Library *AslBase;
#undef AslRequestTags /* Get rid of the macro from inline/ */

	BOOL AslRequestTags (

/*  SYNOPSIS */
	APTR requester,
	Tag  tag1,
	...)

/*  FUNCTION
	This is the varargs version of the asl.library AslRequest().
	For information see asl.library/AslRequest().

    INPUTS
	requester - Pointer to requester returned by AllocAslRequest().
	tag1      - TagList of extra arguments.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	asl.library/AslRequest()

    INTERNALS

    HISTORY
	23-03-2000  bernie  Wrote.

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = AslRequest(requester, AROS_SLOWSTACKTAGS_ARG(tag1));

    AROS_SLOWSTACKTAGS_POST
} /* AslRequestTags */
