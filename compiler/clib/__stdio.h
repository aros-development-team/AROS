#ifndef ___STDIO_H
#define ___STDIO_H

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: internal header file for stdio
    Lang: english
*/
#include <stdio.h>
#include <exec/lists.h>

typedef struct __FILENODE
{
    struct MinNode Node;
    int  fd;
    FILE File;
} FILENODE;

#endif /* ___STDIO_H */
