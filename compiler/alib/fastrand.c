/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
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
	06.12.96 digulla Created after original from libnix

******************************************************************************/
{
    ULONG a = seed << 1;

    if ((LONG)seed <= 0)
	a ^= 0x1d872b41;

    return a;
} /* FastRand */

