 /*
    $Id$
    $Log$
    Revision 1.4  1996/12/10 14:00:15  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.3  1996/10/24 15:51:36  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.2  1996/10/23 14:08:59  aros
    Formatted

    Added parens to all assignments which are used truth expressions

    Revision 1.1  1996/10/22 04:46:01  aros
    Some more utility.library functions.

    Desc: RefreshTagItemClones()
    Lang: english
*/
#include "utility_intern.h"

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <clib/utility_protos.h>

	AROS_LH2(void, RefreshTagItemClones,

/*  SYNOPSIS */
	AROS_LHA(struct TagItem *, clone, A0),
	AROS_LHA(struct TagItem *, original, A1),

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
	<utility/tagitem.h> CloneTagItems()

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

    if( !original || !clone )
	return;

    /* Remember, the clone list is a straight memory block, however
	the original list may not be.
    */
    while ((current = NextTagItem (&original)))
    {
	*clone = *current; /* Copies both tag and data */
	clone++;
    }

    AROS_LIBFUNC_EXIT
} /* RefreshTagItemClones */
