/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function fputc()
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

	int fputc (

/*  SYNOPSIS */
	int    c,
	FILE * stream)

/*  FUNCTION
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

    INTERNALS

    HISTORY
	06.12.1996 digulla created

******************************************************************************/
{
    return FPutC ((BPTR)stream->fh, c);
} /* fputc */

