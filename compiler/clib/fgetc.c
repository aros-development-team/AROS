/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: ANSI C function fgetc()
    Lang: english
*/
#include <errno.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

	int fgetc (

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
	getc(), fputc(), putc()

    INTERNALS

    HISTORY
	10.12.1996 digulla created

******************************************************************************/
{
    int c;

    switch ((IPTR)stream)
    {
    case 1: /* Stdin */
	return FGetC (Input());

    case 2: /* Stdout */
    case 3: /* Stderr */
	errno = EINVAL;
	return EOF;

    default:
	c = FGetC ((BPTR)stream->fh);
	break;
    }

    if (c == EOF)
	stream->flags |= _STDIO_FILEFLAG_EOF;

    return c;
} /* fgetc */

