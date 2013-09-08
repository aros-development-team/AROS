/*
    Copyright © 2004-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function mblen().
*/

#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <stdlib.h>

	int mblen(

/*  SYNOPSIS */
	const char *s,
	size_t n)

/*  FUNCTION
        This function returns the number of bytes of the next multi-byte
        character.

    INPUTS
        s: string pointer to look at next multi-byte character.
        n: The maximum number of bytes to look at.

    RESULT
        if s is not NULL will return the length in bytes of the next
        multi-byte character in s; 0 is return when it is a NULL
        byte character; -1 if it is not a valid multi-byte character.
        If s is NULL zero or non-zero is returned when multi-byte encodings
        resp. don't or do have state-dependent encodings.

    NOTES
	stdc.library currently only implements the "C" locale
        This means that either 0 or 1 is returned when s is not NULL.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    if (s == NULL)
        /* No state-dependent encondings */
        return 0;

    if (n == 0 || *s == '\0')
        return 0;

    if (isascii(*s))
        return 1;
    else
        return -1;
}

