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

        AROS_LH2(LONG, IdAlert,

/*  SYNOPSIS */
        AROS_LHA(ULONG           , id     , D0),
        AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */
        struct Library *, IdentifyBase, 7, Identify)

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
            case IDTAG_DeadStr:
                break;

            case IDTAG_SubsysStr:
                break;

            case IDTAG_GeneralStr:
                break;

            case IDTAG_SpecStr:
                break;

            case IDTAG_StrLength:
                break;

            case IDTAG_Localize:
                break;
        }
    }

    return 0;

    AROS_LIBFUNC_EXIT
} /* IdAlert */
