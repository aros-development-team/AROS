/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: stdio internals
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include "__stdio.h"

FILENODE * GetFilenode4fd (int fd)
{
    FILENODE * fn;

    ForeachNode (&__stdio_files,fn)
    {
	if (fn->fd == fd)
	    return fn;
    }

    return NULL;
} /* GetFilenode4fd */
