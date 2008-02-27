/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function getc().
*/

#include <errno.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "__errno.h"
#include "__open.h"

#include <stdio.h>
#undef getc

/*****************************************************************************

    NAME */
#include <stdio.h>

	int getc (

/*  SYNOPSIS */
	FILE * stream)

/*  FUNCTION
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

******************************************************************************/
{
    return fgetc(stream);
} /* getc */

