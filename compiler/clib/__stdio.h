#ifndef ___STDIO_H
#define ___STDIO_H

/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: internal header file for stdio
    Lang: english
*/
#include <stdio.h>
#include <stddef.h>
#include <exec/lists.h>

typedef struct
{
    struct MinNode Node;
    FILE File;
} FILENODE;

extern struct MinList __stdio_files;

#define FILENODE2FILE(fn)       (&((fn)->File))
#define FILE2FILENODE(f)        ((FILENODE *)(((char *)(f))-offsetof(FILENODE,File)))

int __smode2oflags(char *mode);
int __oflags2sflags(int oflags);

#endif /* ___STDIO_H */
