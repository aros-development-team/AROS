/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <exec/types.h>

ULONG RangeSeed;

/*****************************************************************************

    NAME */
#include <proto/alib.h>

	ULONG RangeRand (

/*  SYNOPSIS */
	ULONG maxValue)

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
    ULONG a = RangeSeed;
    UWORD i = maxValue - 1;

    do
    {
	ULONG b = a;

	a <<= 1;

	if ((LONG)b <= 0)
	    a ^= 0x1d872b41;

    } while ((i >>= 1));

    RangeSeed = a;

    if ((UWORD)maxValue)
	return (UWORD)((UWORD)a * (UWORD)maxValue >> 16);

    return (UWORD)a;
} /* RangeRand */

