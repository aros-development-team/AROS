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

        AROS_LH4(ULONG, IdFormatString,

/*  SYNOPSIS */
        AROS_LHA(STRPTR          , string, A0),
        AROS_LHA(STRPTR          , buffer, A1),
        AROS_LHA(ULONG           , len   , D0),
        AROS_LHA(struct TagItem *, tags  , A2),

/*  LOCATION */
        struct Library *, IdentifyBase, 11, Identify)

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

    extern void aros_print_not_implemented (char *);
    aros_print_not_implemented ("IdFormatString");

    // no tags

    return 0;

    AROS_LIBFUNC_EXIT
} /* IdFormatString */
