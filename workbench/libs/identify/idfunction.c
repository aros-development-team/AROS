/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <proto/utility.h>

#include "identify_intern.h"
#include "identify.h"
#include "modulefunctions.h"

/*****************************************************************************

    NAME */
#include <proto/identify.h>

        AROS_LH3(LONG, IdFunction,

/*  SYNOPSIS */
        AROS_LHA(STRPTR          , libname, A0),
        AROS_LHA(LONG            , offset , D0),
        AROS_LHA(struct TagItem *, taglist, A1),

/*  LOCATION */
        struct Library *, IdentifyBase, 8, Identify)

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
    const struct TagItem *tags;

    for (tags = taglist; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
            case IDTAG_FuncNameStr:
                break;

            case IDTAG_StrLength:
                break;
        }
    }

    return 0;

    AROS_LIBFUNC_EXIT
} /* IdFunction */
