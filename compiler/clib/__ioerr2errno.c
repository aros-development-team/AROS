/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: errno internals
    Lang: english
*/
#include "__errno.h"

int IoErr2errno (int ioerr)
{
    return MAX_ERRNO+1;
} /* IoErr2errno */
