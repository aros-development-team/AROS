/*
    Copyright (C) 1995-2000 AROS - The Amiga Research OS
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
	locale/AslRequest()

    INTERNALS

    HISTORY
	23-03-2000  bernie  Wrote.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,AslBase)

    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = AslRequest(requester, AROS_SLOWSTACKTAGS_ARG(tag1));

    AROS_SLOWSTACKTAGS_POST

    AROS_LIBFUNC_EXIT
} /* AslRequestTags */
