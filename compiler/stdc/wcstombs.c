/*
    Copyright © 2007-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function wcstombs().
*/

/*****************************************************************************

    NAME
#include <stdlib.h>

	size_t wcstombs(

    SYNOPSIS
	char * restrict s,
	const wchar_t * restrict pwcs,
	size_t n)

    FUNCTION
        C99 function wcstombs

    INPUTS

    RESULT

    NOTES
        stdc.library currently only implements "C" locale

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        Implemented as static inline function to adapt to changing wchar_t
        definitions

******************************************************************************/
