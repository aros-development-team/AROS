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

int fgetc (int c, FILE * stream)
{
    if (((IPTR)stream) < 4)
    {
	switch ((IPTR)stream)
	{
	case 1: /* Stdin */
	    return FGetC (Input());

	case 2: /* Stdout */
	    errno = EINVAL;
	    return EOF;

	case 3: {
	    struct Process *me=(struct Process *)FindTask(NULL);
	    BPTR stream=me->pr_CES?me->pr_CES:me->pr_COS;

	    return FGetC (stream); }
	}
    }

    return FGetC ((BPTR)stream->fh);
} /* fgetc */

