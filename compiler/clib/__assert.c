/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    assert()
*/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/*****************************************************************************

    NAME
#include <assert.h>

	void assert (

    SYNOPSIS
	expr)

    FUNCTION
	Evaluates the expression expr and if it's FALSE or NULL, then
	printf a message and stops the program. The message will
	contain the expression, the name of the file with the assert
	in it and the line in the file.

    INPUTS
	expr - The expression to evaluate. The type of the expression does
		not matter, only if its zero/NULL or not.

    RESULT
	The function doesn't return.

    NOTES

    EXAMPLE
	// Make sure that x equals 1
	assert (x==1);

    BUGS

    SEE ALSO

    INTERNALS
******************************************************************************/
void __assert (const char * expr, const char * file, unsigned int line)
{
    fprintf (stderr, "Assertion (%s) failed in %s:%u\n", expr, file, line);
    exit (10);
} /* assert */

