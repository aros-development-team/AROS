/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <stdio.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

int putc (int c, FILE * stream)
{
    if (((IPTR)stream) < 4)
    {
	switch ((IPTR)stream)
	{
	case 1: /* Stdin */
	    return EOF;

	case 2: /* Stdout */
	    return FPutC (Output(), c);

	case 3: {
	    struct Process *me=(struct Process *)FindTask(NULL);
	    BPTR stream=me->pr_CES?me->pr_CES:me->pr_COS;

	    return FPutC (stream, c); }
	}
    }

    return FPutC ((BPTR)stream->fh, c);
} /* putc */

