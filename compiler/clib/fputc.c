/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: ANSI C function fputc()
    Lang: english
*/
#include <errno.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

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
    BPTR fh;

    switch ((IPTR)stream)
    {
    case 0:
    case 1: /* Stdin */
	errno = EINVAL;
	return EOF;

    case 2: /* Stdout */
	fh = Output();

    case 3: {
	struct Process * me = (struct Process *)FindTask (NULL);

	fh = me->pr_CES ? me->pr_CES : me->pr_COS;

	break; }

    default:
	fh = stream->fh;
	break;
    }

    return FPutC (fh, c);
} /* fputc */

