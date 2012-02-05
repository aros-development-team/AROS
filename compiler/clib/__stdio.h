#ifndef ___STDIO_H
#define ___STDIO_H

/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: internal header file for stdio
    Lang: English
*/

#include <stdio.h>
#include <stddef.h>
#include <exec/lists.h>

struct __sFILE
{
    int fd;
    int flags;
};

#define _STDIO_EOF    0x0001L
#define _STDIO_ERROR  0x0002L
#define _STDIO_WRITE  0x0004L
#define _STDIO_READ   0x0008L
#define _STDIO_RDWR   _STDIO_WRITE | _STDIO_READ
#define _STDIO_APPEND 0x0010L

typedef struct
{
    struct MinNode Node;
    FILE File;
} FILENODE;

#define FILENODE2FILE(fn)       (&((fn)->File))
#define FILE2FILENODE(f)        ((FILENODE *)(((char *)(f))-offsetof(FILENODE,File)))

int __smode2oflags(const char *mode);
int __oflags2sflags(int oflags);

#endif /* ___STDIO_H */
