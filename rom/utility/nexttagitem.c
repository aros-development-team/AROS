/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$    $Log
    Desc:
    Lang: english
*/
#include "utility_intern.h"

/*****************************************************************************

    NAME */
	#include <utility/tagitem.h>
	#include <clib/utility_protos.h>

	__AROS_LH1(struct TagItem *, NextTagItem,

/*  SYNOPSIS */
	__AROS_LHA(struct TagItem **, tagListPtr, A0),

/*  LOCATION */
	struct Library *, UtilityBase, 8, Utility)

/*  FUNCTION
	Returns the address of the next tag-item in the list. This
	routine correctly handles TAG_END, TAG_DONE, TAG_MORE and
	TAG_IGNORE.

	TAG_END and TAG_DONE both terminate a TagItems-array (in
	fact, TAG_DONE is the same as TAG_END).

	With TAG_MORE, you can redirect the processing to a new list
	of tags. Note that the processing will not return to the previous
	list when a TAG_END/TAG_DONE is encountered.

	TAG_IGNORE disables the processing of an entry in the list.
	This entry is just ignored (We use this technique for filtering).

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

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h

*****************************************************************************/
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct Library *,UtilityBase)

    while (TRUE)
    {
	switch ((*tagListPtr)->ti_Tag)
	{
	case TAG_MORE:
	    (*tagListPtr) = (struct TagItem *)(*tagListPtr)->ti_Data;
	    continue;

	case TAG_IGNORE:
	    break;

	case TAG_END:
	    return (NULL);

	case TAG_SKIP:
	    (*tagListPtr) += (*tagListPtr)->ti_Data + 1;
	    continue;

	default:
	    /* Use post-increment (return will return the current value and
		then tagListPtr will be incremented) */
	    return (*tagListPtr) ++;
	}

	(*tagListPtr) ++;
    }

    __AROS_FUNC_EXIT
} /* NextTagItem */
