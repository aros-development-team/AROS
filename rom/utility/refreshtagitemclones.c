/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: RefreshTagItemClones()
    Lang: english
*/
#include "intern.h"

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <proto/utility.h>

	AROS_LH2(void, RefreshTagItemClones,

/*  SYNOPSIS */
	AROS_LHA(struct TagItem *, clone, A0),
	AROS_LHA(const struct TagItem *, original, A1),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 14, Utility)

/*  FUNCTION
	If (and only if) the Tag list 'clone' was created by calling
	CloneTagItems on the Tag list 'original', and the list original
	has NOT been changed in any way, then this function will change
	the list 'clone' back to its original state.

    INPUTS
	original    - The source TagList (unaltered)
	clone	    - The destination TagList (MUST be allocated by
			CloneTagItems())

    RESULT
	The second TagList now has the same values as the first.

    NOTES
	If either of the inputs is NULL, then the function will not do
	anything.

    EXAMPLE
	struct TagItem *orig, clone;

	\* TagList orig has some values already *\
	clone = CloneTagList( orig );

	\* In between here we do something to the TagItems in clone,
	    but we need to have them restored.
	*\

	RefreshTagItemClones( clone, orig );

    BUGS
	None, however if either of the two pre-conditions is not fulfilled
	then this function will probably be unreliable, or trash memory.

	We warned you...

    SEE ALSO
	CloneTagItems()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h
	11-08-96    iaint   Based on the 3.0/2.04 version.
	05-09-96    iaint   Updated autodoc, and removed another variable.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *current;

    if( !clone )
	return;

    /* If we don't have an original, but do have a clone, set the
       clone to have a definite end of TagList.

       This is because CloneTagItems(NULL) is valid.
    */
    if(!original)
    {
	clone->ti_Tag = TAG_DONE;
	return;
    }

    /* Remember, the clone list is a straight memory block, however
	the original list may not be.
    */
    while ((current = NextTagItem ((struct TagItem **)&original)))
    {
	*clone = *current; /* Copies both tag and data */
	clone++;
    }

    AROS_LIBFUNC_EXIT
} /* RefreshTagItemClones */
