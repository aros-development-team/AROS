/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function putc().
*/

/*****************************************************************************

    NAME
#include <stdio.h>

	int putc (

    SYNOPSIS
	int    c,
	FILE * stream)

    FUNCTION
	Write one character to the specified stream.

    INPUTS
	c - The character to output
	stream - The character is written to this stream

    RESULT
	The character written or EOF on error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        fputc()

    INTERNALS
        putc() is an alias for fputc()

******************************************************************************/
