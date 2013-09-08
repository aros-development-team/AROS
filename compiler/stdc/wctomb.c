/*
    Copyright © 2007-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function wctomb().
*/

/*****************************************************************************

    NAME
#include <stdlib.h>

	int wctomb(

    SYNOPSIS
	char *s,
	wchar_t wchar)

    FUNCTION
        Converts one wide char to a multi-byte char.

    INPUTS
        wchar: wide char to convert
        s: string pointer to put the converted char into.

    RESULT
        If s is not NULL it returns the number of chars written into s;
        zero is returned when wchar is a NULL character; -1 when the character
        is not valid.
        If s is NULL the function returns zero or non-zero when multi-byte
        string resp don't or do have state-dependent encodings.

    NOTES
	stdc.library currently only implements "C" locale.
        This means that 0 is returned when s is NULL and 0, 1 or -1 when s is
        not NULL.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        Implemented as static inline function to adapt to changing wchar_t
        definitions

******************************************************************************/

