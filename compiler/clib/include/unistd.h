#ifndef _UNISTD_H
#define _UNISTD_H

/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI-C header file unistd.h
    Lang: english
*/
#include <sys/types.h>

/* Prototypes */
int access (const char *path, int mode);
int close (int fd);
int dup(int oldfd);
int dup2(int oldfd, int newfd);
int lseek(int fildes, off_t offset, int whence);
ssize_t read (int fd, void * buf, size_t count);
#define rmdir unlink
int truncate(const char *path, off_t length);
int unlink(const char *pathname);
void usleep(unsigned long usec);
ssize_t write (int fd, const void * buf, size_t count);

/* Standard file descriptors */
#define STDIN_FILENO  0 /* Standard input */
#define STDOUT_FILENO 1 /* Standard output */
#define STDERR_FILENO 2 /* Standard error output */

#ifndef _STDIO_H    /*stdio.h has the same definitions */

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

#endif

/* Values for the second argument to access.
   These may be OR'd together.  */
#define	R_OK	4		/* Test for read permission.  */
#define	W_OK	2		/* Test for write permission.  */
#define	X_OK	1		/* Test for execute permission.  */
#define	F_OK	0		/* Test for existence.  */

#endif /* _UNISTD_H */
