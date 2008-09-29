/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AllocateTagItems()
    Lang: english
*/
#include <proto/exec.h>
#include "intern.h"

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <proto/utility.h>

	AROS_LH1(struct TagItem *, AllocateTagItems,

/*  SYNOPSIS */
	AROS_LHA(ULONG, numTags, D0),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 11, Utility)

/*  FUNCTION
	Allocate a number of TagItems in an array for whatever you like.
	The memory allocated will be cleared.

    INPUTS
	numTags     - The number of TagItems to allocate.

    RESULT
	A pointer to an array of struct TagItem containing numTags tags.

    NOTES
	The number you supply must include the terminating tag (ie TAG_DONE)
	There is no provision for extra TagItems at the end of the list.

	If the number of tags to allocate is zero, then none will be.

    EXAMPLE
	struct TagItem *tagList;

	tagList =  AllocateTagItems( 4 );

	tagList[0].ti_Tag  = NA_Name;
	tagList[0].ti_Data = (IPTR)"A list of tags";
	tagList[3].ti_Tag  = TAG_DONE;

	\* Do what you want with your TagList here ... *\

	FreeTagItems( tagList );

    BUGS

    SEE ALSO
	FreeTagItems()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h
	11-08-96    iaint   Moved code into the AROS source.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct TagItem *tags = NULL;

    if( numTags )
	tags = AllocVec( numTags * sizeof(struct TagItem) ,
			 MEMF_CLEAR | MEMF_PUBLIC );

    return tags;

    AROS_LIBFUNC_EXIT

} /* AllocateTagItems */
