/*
    $Id$
    $Log$
    Revision 1.3  1997/01/27 00:32:30  ldp
    Polish

    Revision 1.2  1997/01/09 12:54:13  digulla
    Added parentheses around if (a=b)

    Revision 1.1  1997/01/08 03:36:13  iaint
    A few more utility.lib functions

    Desc: FilterTagChanges() - filter unchanged tags from a list.
    Lang: english
*/
#include "utility_intern.h"

/*****************************************************************************

    NAME */
#include <proto/utility.h>

	AROS_LH3(void, FilterTagChanges,

/*  SYNOPSIS */
	AROS_LHA(struct TagItem *, changeList, A0),
	AROS_LHA(struct TagItem *, originalList, A1),
	AROS_LHA(BOOL            , apply, D0),

/*  LOCATION */
	struct Library *, UtilityBase, 9, Utility)

/*  FUNCTION
	This function will scan through changeList, and if an item in
	changeList exists in originalList, but both items data values
	are equal, then the item in changeList will be removed from the
	list.

	If the value of apply is TRUE, then if the datas are different
	then the values in originalList will be updated to match those
	in changeList.

    INPUTS
	changeList	-   List of new tags (may be NULL).
	originalList	-   List of existing tags (may be NULL).
	apply		-   Boolean flag as to whether the values in
			    originalList should be updated to match
			    those in changeList.

    RESULT
	The changeList will be modified to show altered items, and if
	requested, the originalList will be updated.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	ApplyTagChanges()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (originalList && changeList)
    {
	struct TagItem *change, *orig;

	while ((change = NextTagItem(&changeList)))
	{
	    if ((orig = FindTagItem(change->ti_Tag, originalList)))
	    {
		if (change->ti_Data == orig->ti_Data)
		{
		    change->ti_Tag = TAG_IGNORE;
		}
		else
		{
		    if (apply)
			orig->ti_Data = change->ti_Data;
		}
	    } /* if (FindTagItem()) */
	} /* while (changeList++) */
    } /* if (lists are both valid) */

    AROS_LIBFUNC_EXIT
} /* FilterTagChanges */
