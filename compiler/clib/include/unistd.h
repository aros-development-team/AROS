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

/* Standard file descriptors */
#define STDIN_FILENO  0 /* Standard input */
#define STDOUT_FILENO 1 /* Standard output */
#define STDERR_FILENO 2 /* Standard error output */

#endif /* _UNISTD_H */
