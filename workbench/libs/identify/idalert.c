/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <proto/utility.h>

#include <string.h>

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
        struct IdentifyBaseIntern *, IdentifyBase, 7, Identify)

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

    STRPTR deadstr = NULL;
    STRPTR subsysstr = NULL;
    STRPTR generalstr = NULL;
    STRPTR specstr = NULL;
    BOOL localize = TRUE;
    ULONG strlength = 50;

    struct TagItem *tag;
    const struct TagItem *tags;

    for (tags = taglist; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
            case IDTAG_DeadStr:
                deadstr = (STRPTR)tag->ti_Data;
                break;

            case IDTAG_SubsysStr:
                subsysstr = (STRPTR)tag->ti_Data;
                break;

            case IDTAG_GeneralStr:
                generalstr = (STRPTR)tag->ti_Data;
                break;

            case IDTAG_SpecStr:
                specstr = (STRPTR)tag->ti_Data;
                break;

            case IDTAG_StrLength:
                strlength = tag->ti_Data;
                break;

            case IDTAG_Localize:
                localize = tag->ti_Data ? TRUE : FALSE;
                break;
        }
    }

    if (strlength <= 0)
    {
        return IDERR_NOLENGTH;
    }

    // return something to avoid crashes when function is called
    if (deadstr)
    {
        strlcpy(deadstr, "Unknown", strlength);
    }
    if (subsysstr)
    {
        strlcpy(subsysstr, "Unknown", strlength);
    }
    if (generalstr)
    {
        strlcpy(generalstr, "Unknown", strlength);
    }
    if (specstr)
    {
        strlcpy(specstr, "Unknown", strlength);
    }

    return 0;

    AROS_LIBFUNC_EXIT
} /* IdAlert */
