/*
    $Id$
    $Log$
    Revision 1.5  1996/10/24 22:51:46  aros
    Use proper Amiga datatypes (eg: ULONG not unsigned long)

    Revision 1.4  1996/10/24 15:51:34  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.3  1996/09/13 17:36:37  digulla
    Use IPTR

    Revision 1.2  1996/09/12 14:52:47  digulla
    Better way to separate public and private parts of the library base

    Revision 1.1  1996/08/31 12:58:11  aros
    Merged in/modified for FreeBSD.

    Desc: AllocateTagItems()
    Lang: english
*/
#include "utility_intern.h"

/*****************************************************************************

    NAME */
	#include <utility/tagitem.h>
	#include <clib/utility_protos.h>

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
	FreeTagsItems()

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
	tags = AllocVec( numTags << 3, MEMF_CLEAR | MEMF_PUBLIC );

    return tags;

    AROS_LIBFUNC_EXIT

} /* AllocateTagItems */
