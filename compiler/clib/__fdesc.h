#ifndef ___FDESC_H
#define ___FDESC_H

/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
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

struct arosc_privdata;
int __register_init_fdarray(struct arosc_privdata *priv);
int __getfdslots(void);
fdesc *__getfdesc(register int fd);
void __setfdesc(register int fd, fdesc *fdesc);
int __getfdslot(int wanted_fd);
int __getfirstfd(register int startfd);
int __open(int wanted_fd, const char *pathname, int flags, int mode);
void __updatestdio(void);
LONG __oflags2amode(int flags);
fdesc *__alloc_fdesc(void);
void __free_fdesc(fdesc *fdesc);

#endif /* ___FDESC_H */
