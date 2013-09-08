/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function getc().
*/

/*****************************************************************************

    NAME
#include <stdio.h>

	int getc (

    SYNOPSIS
	FILE * stream)

    FUNCTION
	Read one character from the stream. If there is no character
	available or an error occurred, the function returns EOF.

    INPUTS
	stream - Read from this stream

    RESULT
	The character read or EOF on end of file or error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	fgetc(), fputc(), putc()

    INTERNALS
        getc() is just an alias for fgetc().

******************************************************************************/
