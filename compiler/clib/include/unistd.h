#ifndef _UNISTD_H
#define _UNISTD_H

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: ANSI-C header file unistd.h
    Lang: english
*/
#include <sys/types.h>

/* Prototypes */
int open (const char * filename, int flags, ...);
int creat (const char * filename, int mode);
ssize_t read (int fd, void * buf, size_t count);
ssize_t write (int fd, const void * buf, size_t count);
int close (int fd);

#endif /* _UNISTD_H */
