/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <exec/types.h>
#include <utility/utility.h>
#include <utility/tagitem.h>
#include <aros/libcall.h>
#include <proto/arossupport.h>

/*****************************************************************************

    NAME */
#include <proto/utility.h>

	AROS_LH2I(struct TagItem *, FindTagItem,

/*  SYNOPSIS */
	AROS_LHA(Tag,                    tagValue, D0),
	AROS_LHA(struct TagItem *, tagList,  A0),

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

    /* Use code from libarossupport */
    return LibFindTagItem(tagValue, tagList);

    AROS_LIBFUNC_EXIT
} /* FindTagItem */
