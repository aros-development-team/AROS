/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <exec/types.h>
#include <utility/utility.h>
#include <utility/tagitem.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
#include <proto/utility.h>

	AROS_LH2(struct TagItem *, FindTagItem,

/*  SYNOPSIS */
	AROS_LHA(Tag,                    tagValue, D0),
	AROS_LHA(const struct TagItem *, tagList,  A0),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 5, Utility)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tag;
    const struct TagItem *tagptr = tagList;

    while( (tag = NextTagItem(&tagptr)) )
    {
	if(tag->ti_Tag == tagValue) return tag;
    }

    return NULL;

    AROS_LIBFUNC_EXIT
} /* FindTagItem */
