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

#define __POSIXC_STDIO_EOF    0x0001L
#define __POSIXC_STDIO_ERROR  0x0002L
#define __POSIXC_STDIO_WRITE  0x0004L
#define __POSIXC_STDIO_READ   0x0008L
#define __POSIXC_STDIO_RDWR   __POSIXC_STDIO_WRITE | __POSIXC_STDIO_READ
#define __POSIXC_STDIO_APPEND 0x0010L

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
