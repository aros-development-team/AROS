/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <stdio.h>
#include <exec/types.h>
#include <dos/dosextens.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

int fflush (FILE * stream)
{
    switch ((IPTR)stream)
    {
    case 0:
    case 1:
	if (Flush (Input ()))
	    return EOF;

	break;

    case 2:
	if (Flush (Output ()))
	    return EOF;

	break;

    case 3: {
	struct Process *me=(struct Process *)FindTask(NULL);

	if (Flush (me->pr_CES ? me->pr_CES : me->pr_COS))
	    return EOF;

	break; }

    default:
	if (Flush (stream->fh))
	    return EOF;
	break;
    }

    return 0;
} /* fflush */

