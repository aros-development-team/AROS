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

typedef struct __FILENODE
{
    struct MinNode Node;
    int  fd;
    FILE File;
} FILENODE;

extern struct MinList __stdio_files;
extern int __stdio_fd;

#define FILENODE2FILE(fn)       (&((fn)->File))
#define FILE2FILENODE(f)        ((FILENODE *)(((char *)(f))-offsetof(FILENODE,File)))

/* Prototypes */
FILENODE * GetFilenode4fd (int fd);

#endif /* ___STDIO_H */
