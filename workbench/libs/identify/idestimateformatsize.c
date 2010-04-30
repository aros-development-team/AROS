/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "identify_intern.h"

/*****************************************************************************

    NAME */
#include <proto/identify.h>

        AROS_LH2(ULONG, IdEstimateFormatSize,

/*  SYNOPSIS */
        AROS_LHA(STRPTR          , String, A0),
        AROS_LHA(struct TagItem *, Tags  , A1),

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

    return 0;

    AROS_LIBFUNC_EXIT
} /* IdEstimateFormatSize */
