/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "intern.h"
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#include <proto/utility.h>

	AROS_LH2(BOOL, TagInArray,

/*  SYNOPSIS */
	AROS_LHA(Tag  , tagValue, D0),
	AROS_LHA(Tag *, tagArray, A0),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 15, Utility)

/*  FUNCTION
	Determines whether the value tagValue exists in an array of Tags
	pointed to by tagArray. This array must be contiguous, and must be
	terminated by TAG_DONE.

	This is an array of Tags (ie: Tag tagArray[]), not an array of
	TagItems (ie: struct TagItem tagArray[]).

    INPUTS
	tagValue    -	The value of the Tag to search for.
	tagArray    -	The ARRAY of Tag's to scan through.

    RESULT
	TRUE	if tagValue exists in tagArray
	FALSE	otherwise

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	<utility/tagitem.h>, FilterTagItems()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h
	01-09-96    iaint   Implemented from autodoc.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    while(*tagArray != TAG_DONE)
    {
	if(*tagArray == tagValue)
	    return TRUE;
	tagArray++;
    }
    return FALSE;

    AROS_LIBFUNC_EXIT
} /* TagInArray */
