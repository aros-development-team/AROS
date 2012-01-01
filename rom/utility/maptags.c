/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "intern.h"

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <proto/utility.h>

	AROS_LH3(void, MapTags,

/*  SYNOPSIS */
	AROS_LHA(struct TagItem *, tagList, A0),
	AROS_LHA(struct TagItem *, mapList, A1),
	AROS_LHA(ULONG           , mapType, D0),

/*  LOCATION */
	struct Library *, UtilityBase, 10, Utility)

/*  FUNCTION
	Replace the ti_Tags in tagList which match the ti_Tags in mapList
	by the ti_Data values of mapList.

    INPUTS
	tagList - This list is modified
	mapList - This defines which ti_Tag is replaced with what new value.

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct TagItem * tag, * map;

    while ((tag = NextTagItem (&tagList)))
    {
	if (mapList && (map = FindTagItem (tag->ti_Tag, mapList)))
	{
	    if (map->ti_Data == TAG_DONE)
		tag->ti_Tag = TAG_IGNORE;
	    else
		tag->ti_Tag = (ULONG)map->ti_Data;
	}
	else if (mapType == MAP_REMOVE_NOT_FOUND)
	    tag->ti_Tag = TAG_IGNORE;

    }

    AROS_LIBFUNC_EXIT
} /* MapTags */
