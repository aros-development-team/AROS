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

/* Access modes: */
#define O_ACCMODE	0x0003
#define O_RDONLY	0x0001
#define O_WRONLY	0x0002
#define O_RDWR		(O_RDONLY | O_WRONLY)

/* The GNU system specifies these */
#define O_READ          O_RDONLY
#define O_WRITE         O_WRONLY

/* This is not included in the result of modes & O_ACCMODE */
#define O_EXEC          0x0004

/* Open time flags */
#define O_NOCTTY	0         /* We ignore this one */
#define O_CREAT 	0x0040
#define O_EXCL		0x0080
#define O_SHLOCK        0         /* files are always opened in shared mode, if not otherwise specified */
#define O_EXLOCK        0x0100
#define O_TRUNC 	0x0200

/* Operating modes */
#define O_APPEND	0x0400
#define O_NONBLOCK	0x0800
#define O_NDELAY	O_NONBLOCK /* Alias */
#define O_SYNC		0x1000
#define O_FSYNC         O_SYNC     /* Alias */

#ifndef _STDIO_H    /*fcntl.h has the same definitions */

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

#endif

#endif /* _FCNTL_H */
