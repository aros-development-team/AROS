#ifndef _FCNTL_H
#define _FCNTL_H

/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI-C header file fcntl.h
    Lang: english
*/

/* Prototypes */
int open  (const char * filename, int flags, ...);
int creat (const char * filename, int mode);

/* Flags for open */
#define O_ACCMODE	0x0003
#define O_RDONLY	0x0000
#define O_WRONLY	0x0001
#define O_RDWR		0x0002
#define O_CREAT 	0x0040
#define O_EXCL		0x0080
#define O_NOCTTY	0x0100
#define O_TRUNC 	0x0200
#define O_APPEND	0x0400
#define O_NONBLOCK	0x0800
#define O_NDELAY	O_NONBLOCK /* Alias */
#define O_SYNC		0x1000

#ifndef _STDIO_H    /*fcntl.h has the same definitions */

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

#endif

#endif /* _FCNTL_H */
