/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$    $Log
*/

#include <proto/arossupport.h>

#include "intern.h"

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <proto/utility.h>

	AROS_LH1I(struct TagItem *, NextTagItem,

/*  SYNOPSIS */
	AROS_LHA(struct TagItem **, tagListPtr, A0),

/*  LOCATION */
	struct Library *, UtilityBase, 8, Utility)

/*  FUNCTION
	Returns the address of the next tag-item in the list. This
	routine correctly handles TAG_END, TAG_DONE, TAG_MORE,
	TAG_IGNORE and TAG_SKIP.

	TAG_END and TAG_DONE both terminate a TagItems-array (in
	fact, TAG_DONE is the same as TAG_END).

	With TAG_MORE, you can redirect the processing to a new list
	of tags. Note that the processing will not return to the previous
	list when a TAG_END/TAG_DONE is encountered.

	TAG_IGNORE disables the processing of an entry in the list.
	This entry is just ignored (We use this technique for filtering).

	TAG_SKIP skips this tagitem, and the next number of tagitems as
	indicated in the tag's ti_Data field.

    INPUTS
	tagListPtr - Pointer to an element in a taglist.

    RESULT
	Next tag item or NULL if you reached the end of the list.

    NOTES
	- TAG_MORE works like "go on with new list" instead of "read new
	  list and go on with the current one".

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

/*  Gosh, can't enable these because we get LOTS of hits at startup time
 *
 *  ASSERT_VALID_PTR(tagListPtr);
 *  ASSERT_VALID_PTR(*tagListPtr);
 */

    /* Use code from libarossupport */
    return LibNextTagItem(tagListPtr);

    AROS_LIBFUNC_EXIT
} /* NextTagItem */
