/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <exec/types.h>
#include <utility/utility.h>
#include <utility/tagitem.h>
#include <proto/arossupport.h>

/*****************************************************************************

    NAME */
#include <proto/utility.h>

        AROS_LH2I(struct TagItem *, FindTagItem,

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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Use code from libarossupport */
    return LibFindTagItem(tagValue, tagList);

    AROS_LIBFUNC_EXIT
} /* FindTagItem */
