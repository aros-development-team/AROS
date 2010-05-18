/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "identify_intern.h"

static STRPTR finddollar(TEXT *t);

/*****************************************************************************

    NAME */
#include <proto/identify.h>

        AROS_LH2(ULONG, IdEstimateFormatSize,

/*  SYNOPSIS */
        AROS_LHA(STRPTR          , string, A0),
        AROS_LHA(struct TagItem *, tags  , A1),

/*  LOCATION */
        struct IdentifyBaseIntern *, IdentifyBase, 12, Identify)

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

    // no tags

    TEXT *from = string;
    ULONG result = 1;

    if (from == NULL)
    {
        return 0;
    }

    while (*from)
    {
        if (*from == '$')
        {
            from++;
            if (*from == '$')
            {
                from++;
                result++;
            }
            else
            {
                from = finddollar(from);
                result += STRBUFSIZE;
            }
        }
        else
        {
            result++;
            from++;
        }
    }

    return result;

    AROS_LIBFUNC_EXIT
} /* IdEstimateFormatSize */


static STRPTR finddollar(TEXT *t)
{
    while (*t && *t != '$')
    {
        t++;
    }

    if (*t) // '$'
    {
        return t + 1;
    }
    else
    {
        return t;
    }
}
