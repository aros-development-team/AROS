/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: stdio internals
    Lang: english
*/
#include <stdio.h>
#include <exec/lists.h>

FILE * stdin  = (FILE *)1L;
FILE * stdout = (FILE *)2L;
FILE * stderr = (FILE *)3L;

struct MinList __stdio_files =
{
    (struct MinNode *)&__stdio_files.mlh_Tail,
    NULL,
    (struct MinNode *)&__stdio_files
};

int __stdio_fd = 4;
