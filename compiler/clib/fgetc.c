/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <stdio.h>
#include <errno.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

int fgetc (FILE * stream)
{
    int c;

    switch ((IPTR)stream)
    {
    case 1: /* Stdin */
	c = FGetC (Input());
	break;

    case 2: /* Stdout */
	errno = EINVAL;
	return EOF;

    case 3: {
	struct Process *me=(struct Process *)FindTask(NULL);

	c = FGetC (me->pr_CES ? me->pr_CES : me->pr_COS);

	break; }

    default:
	c = FGetC ((BPTR)stream->fh);
	break;
    }

    if (c == EOF)
	stream->flags |= _STDIO_FILEFLAG_EOF;

    return c;
} /* fgetc */

