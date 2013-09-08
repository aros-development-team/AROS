/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    assert()
*/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/*****************************************************************************

    NAME */
#include <assert.h>

	void __posixc_assert (

/*  SYNOPSIS */
	const char * expr,
        const char * file,
        unsigned int line)

/*  FUNCTION
        This is a function that is used for implementation of the C99 assert()
        function.

    INPUTS
	expr - The expression to evaluate. The type of the expression does
		not matter, only if its zero/NULL or not.
        file - Name of the source file.
        line - Line number of assert() call.

    RESULT
        The function doesn't return.

    NOTES
        Different versions of this function are available. This function
        is used when a program is using posixc.library.

    EXAMPLE

    BUGS

    SEE ALSO
        stdc.library/assert(), stdc.library/__stdc_assert(),
        stdc.library/__stdcio_assert()

    INTERNALS

******************************************************************************/
{
    fprintf (stderr, "Assertion (%s) failed in %s:%u\n", expr, file, line);
    exit (10);
} /* assert */

