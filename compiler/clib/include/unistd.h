#ifndef _UNISTD_H_
#define _UNISTD_H_

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file unistd.h
    Lang: english
*/
#include <sys/types.h>
#include <sys/_posix.h>

#include <sys/cdefs.h>

__BEGIN_DECLS

/* Prototypes */
int   access (const char *path, int mode);
int   close (int fd);
int   dup(int oldfd);
int   dup2(int oldfd, int newfd);
int   chdir( const char *path );
char *getcwd(char *buf, size_t size);
int   isatty(int fd);
int   lseek(int fildes, off_t offset, int whence);
int   chown(const char *path, uid_t owner, gid_t group);

/* returns the caller's process ID */
pid_t getpid(void);

/* Create a one-way communication channel (pipe).
   If successful, two file descriptors are stored in PIPEDES;
   bytes written on PIPEDES[1] can be read from PIPEDES[0].
   Returns 0 if successful, -1 if not.  */
int pipe(int pipedes[2]);

ssize_t read(int fd, void * buf, size_t count);
int rmdir(const char *pathname);
int truncate(const char *path, off_t length);
int ftruncate(int fd, off_t length);
int unlink(const char *pathname);
unsigned int sleep(unsigned int);
void usleep(unsigned long usec);
ssize_t write (int fd, const void * buf, size_t count);

void _exit(int code) __noreturn;

int execvp(const char *file, char *const argv[]);

uid_t getuid(void);
uid_t geteuid(void);

int readlink(const char *path, char *buf, int bufsiz);

__END_DECLS

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

#endif /* _UNISTD_H_ */
