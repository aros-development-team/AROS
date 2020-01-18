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

#if !defined(__off64_t_defined)
#define off64_t UQUAD
#endif

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

extern int __smode2oflags(const char *mode);
extern int __oflags2sflags(int oflags);

#if !defined(POSIXC_NOSTDIO_DECL)
extern FILE * __fopen (const char * pathname, const char * mode, int    large);
extern int __fseeko (FILE * stream, off_t  offset, int    whence);
extern off_t __ftello (FILE *stream);
extern int __fseeko64 (FILE * stream, off64_t  offset, int    whence);
extern off64_t __ftello64 (FILE *stream);
#endif
#endif /* ___STDIO_H */
