/*
    Copyright (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
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

