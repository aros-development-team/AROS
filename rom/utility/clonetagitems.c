/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CloneTagItems()
    Lang: english
*/
#include "intern.h"

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <proto/utility.h>

	AROS_LH1(struct TagItem *, CloneTagItems,

/*  SYNOPSIS */
	AROS_LHA(struct TagItem *, tagList, A0),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 12, Utility)

/*  FUNCTION
	Duplicates a TagList. The input TagList can be NULL, in which
	case an empty TagList will be returned.

    INPUTS
	tagList     -	The TagList that you want to clone

    RESULT
	A TagList which contains a copy of the TagItems contained in the
	original list. The list is cloned so that calling FindTagItem()
	on a tag in the clone will return the same value as that in the
	original list (assuming the original has not been modified).

    NOTES

    EXAMPLE
	struct TagItem *tagList, *tagListClone;

	\* Set up the original taglist tagList *\

	tagListClone = CloneTagItems( tagList );

	\* Do what you want with your TagList here *\

	FreeTagItems( tagListClone );

    BUGS

    SEE ALSO
	AllocateTagItems(), FreeTagItems(), RefreshTagItemClones()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *newList;
    LONG numTags = 1;

    /*
	This is rather strange, if we have a NULL input, then we still
	have to allocate a list. This is to circumvent a bug in SID v2,
	which for some unknown reason is the only program I have seen
	that has this problem.

	Had to alter RefreshTagItemClones as well.
	However, that is also what the autodoc says...
    */
    if (tagList)
    {
	struct TagItem *tmp;
	/*
	    We start the counter at 1 since this count will not include the
	    TAG_DONE TagItem

	    newList is used here to save a variable. We don't need to do
	    anything to the value of newList afterwards, since AllocateTagItems()
	    will take care of setting it to NULL if the allocation fails.
	*/
	tmp = (struct TagItem *)tagList;
	while (NextTagItem (&tmp) != NULL)
	    numTags++;
    }

    /*
	Then we shall allocate the TagList.
	If we can actually allocate a clone tag list, then we shall copy
	the tag values from one tag to another, the function
	"RefreshTagItemClones()" is used here to help re-use code.

	Of course we don't have to worry about the if and only if
	statement, since the original is guaranteed to have not
	been changed since CloneTagItems() :)
    */

    if ((newList = AllocateTagItems(numTags)))
	RefreshTagItemClones(newList, tagList);

    /* newList == 0 when allocation failed - AllocateTagItems handles this*/
    return newList;

    AROS_LIBFUNC_EXIT
} /* CloneTagItems */
