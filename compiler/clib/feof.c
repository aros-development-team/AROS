/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <stdio.h>

int feof (FILE * stream)
{
    return (stream->flags & _STDIO_FILEFLAG_EOF) != 0;
} /* feof */

