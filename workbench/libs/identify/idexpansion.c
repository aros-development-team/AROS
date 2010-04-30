/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <proto/utility.h>

#include "identify_intern.h"
#include "identify.h"

/*****************************************************************************

    NAME */
#include <proto/identify.h>

        AROS_LH1(LONG, IdExpansion,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */
        struct IdentifyBaseIntern *, IdentifyBase, 5, Identify)

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
            case IDTAG_ConfigDev:
                break;

            case IDTAG_ManufID:
                break;

            case IDTAG_ProdID:
                break;

            case IDTAG_ManufStr:
                break;

            case IDTAG_ProdStr:
                break;

            case IDTAG_ClassStr:
                break;

            case IDTAG_StrLength:
                break;

            case IDTAG_Expansion:
                break;

            case IDTAG_Secondary:
                break;

            case IDTAG_ClassID:
                break;

            case IDTAG_Localize:
                break;

        }
    }

    return 0;

    AROS_LIBFUNC_EXIT
} /* IdExpansion */
