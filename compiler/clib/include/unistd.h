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
int close (int fd);
int dup(int oldfd);
int dup2(int oldfd, int newfd);
ssize_t read (int fd, void * buf, size_t count);
ssize_t write (int fd, const void * buf, size_t count);
int unlink(const char *pathname);
void usleep(unsigned long usec);
int lseek(int fildes, off_t offset, int whence);

/* Standard file descriptors */
#define STDIN_FILENO  0 /* Standard input */
#define STDOUT_FILENO 1 /* Standard output */
#define STDERR_FILENO 2 /* Standard error output */

#ifndef _STDIO_H    /*stdio.h has the same definitions */

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

#endif

#endif /* _UNISTD_H */
