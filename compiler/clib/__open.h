#ifndef ___OPEN_H
#define ___OPEN_H

/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: file descriptors handling internals - header file
    Lang: english
*/

#include <dos/dos.h>

/* file control block - one per file handle */
typedef struct _fcb
{
    BPTR fh;
    int  flags;
    unsigned int opencount;
    char  isdir;
    unsigned char privflags;
} fcb;

/* privflags */
#define _FCB_DONTCLOSE_FH 1 

/* file descriptor structure - one per descriptor */
typedef struct _fdesc
{
    fcb  *fcb;
    int  fdflags;
} fdesc;

int __register_init_fdarray(fdesc **fdarray, int numslots);
fdesc *__getfdesc(register int fd);
void __setfdesc(register int fd, fdesc *fdesc);
int __getfdslot(int wanted_fd);
int __getfirstfd(register int startfd);
int __open(int wanted_fd, const char *pathname, int flags, int mode);
void __updatestdio(void);
LONG __oflags2amode(int flags);
#endif
