/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <stdio.h>

int fputc (int c, FILE * stream)
{
    return putc (c, stream);
}

