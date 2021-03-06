/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.
*/

/*****************************************************************************

    NAME */
#include <proto/alib.h>

        ULONG FastRand (

/*  SYNOPSIS */
        ULONG seed)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    ULONG a = seed << 1;

    if ((LONG)seed <= 0)
        a ^= 0x1d872b41;

    return a;
} /* FastRand */

