#ifndef ___STDIO_H
#define ___STDIO_H

/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc: internal header file for stdio
    Lang: English
*/
#include <stdio.h>
#include <stddef.h>
#include <exec/lists.h>

typedef struct
{
    struct MinNode Node;
    FILE File;
} FILENODE;

#ifndef _CLIB_KERNEL_
extern struct MinList __stdio_files;
#endif

#define FILENODE2FILE(fn)       (&((fn)->File))
#define FILE2FILENODE(f)        ((FILENODE *)(((char *)(f))-offsetof(FILENODE,File)))

int __smode2oflags(const char *mode);
int __oflags2sflags(int oflags);

#endif /* ___STDIO_H */
