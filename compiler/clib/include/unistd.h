#ifndef _UNISTD_H_
#define _UNISTD_H_

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX header file unistd.h
    Lang: english
*/
#include <sys/_types.h>
#include <sys/types.h>

/*
 * What version of POSIX things do we implement. We must define this before
 * including <sys/cdefs.h>
 */
#define _POSIX_VERSION              200112L
#define _POSIX2_VERSION             200112L
#define _XOPEN_VERSION              600

#include <sys/cdefs.h>

/*
   POSIX options and option groups:
    These inform the program what POSIX options this system implements. In
    general we don't implement any of them, when we do, the value can be
    changed.

    Valid values are:
        -1              unimplemented
        0               implemented, but must check at run time
        >0              implemented always
 */

#define _POSIX_ADVISORY_INFO                -1
#define _POSIX_ASYNCHRONOUS_IO              -1
#define _POSIX_BARRIERS                     -1
#define _POSIX_CHOWN_RESTRICTED             0
#define _POSIX_CLOCK_SELECTION              -1
#define _POSIX_CPUTIME                      -1
#define _POSIX_FSYNC                        -1
#define _POSIX_IPV6                         -1
#define _POSIX_JOB_CONTROL                  0
#define _POSIX_MAPPED_FILES                 -1
#define _POSIX_MEMLOCK                      -1
#define _POSIX_MEMLOCK_RANGE                -1
#define _POSIX_MEMORY_PROTECTION            -1
#define _POSIX_MESSAGE_PASSING              -1
#define _POSIX_MONOTONIC_CLOCK              -1
#define _POSIX_NO_TRUNC                     1
#define _POSIX_PRIORITIZED_IO               -1
#define _POSIX_PRIORITY_SCHEDULING          -1
#define _POSIX_RAW_SOCKETS                  -1
#define _POSIX_READER_WRITER_LOCKS          -1
#define _POSIX_REALTIME_SIGNALS             -1
#define _POSIX_REGEXP                       0
#define _POSIX_SAVED_IDS                    1       /* XXX: MUST IMPLEMENT */
#define _POSIX_SEMAPHORES                   -1
#define _POSIX_SHARED_MEMORY_OBJECTS        -1
#define _POSIX_SHELL                        1       /* XXX: MUST IMPLEMENT */
#define _POSIX_SPAWN                        -1
#define _POSIX_SPIN_LOCKS                   -1
#define _POSIX_SPORADIC_SERVER              -1
#define _POSIX_SYNCHRONIZED_IO              -1
#define _POSIX_THREAD_ATTR_STACKADDR        -1
#define _POSIX_THREAD_ATTR_STACKSIZE        -1
#define _POSIX_THREAD_CPUTIME               -1
#define _POSIX_THREAD_PRIO_INHERIT          -1
#define _POSIX_THREAD_PRIO_PROTECT          -1
#define _POSIX_THREAD_PRIORITY_SCHEDULING   -1
#define _POSIX_THREAD_PROCESS_SHARED        -1
#define _POSIX_THREAD_SAFE_FUNCTIONS        -1
#define _POSIX_THREAD_SPORADIC_SERVER       -1
#define _POSIX_THREADS                      -1
#define _POSIX_TIMEOUTS                     -1
#define _POSIX_TIMERS                       -1
#define _POSIX_TRACE                        -1
#define _POSIX_TRACE_EVENT_FILTER           -1
#define _POSIX_TRACE_INHERIT                -1
#define _POSIX_TRACE_LOG                    -1
#define _POSIX_TYPED_MEMORY_OBJECTS         -1
#define _POSIX_VDISABLE                     0xff
#define _POSIX2_C_BIND                      200112L
#define _POSIX2_C_DEV                       -1          /* need c99 utility */
#define _POSIX2_CHAR_TERM                   -1
#define _POSIX2_FORT_DEV                    -1
#define _POSIX2_FORT_RUN                    -1
#define _POSIX2_LOCALEDEF                   -1
#define _POSIX2_PBS                         -1
#define _POSIX2_PBS_ACCOUNTING              -1
#define _POSIX2_PBS_CHECKPOINT              -1
#define _POSIX2_PBS_LOCATE                  -1
#define _POSIX2_PBS_MESSAGE                 -1
#define _POSIX2_PBS_TRACK                   -1
#define _POSIX2_SW_DEV                      -1          /* XXX */
#define _POSIX2_UPE                         200112L

#define _V6_ILP32_OFF32                     0
#define _V6_ILP32_OFFBIG                    -1
#define _V6_LP64_OFF64                      -1
#define _V6_LPBIG_OFFBIG                    -1

#if __XSI_VISIBLE
#define _XOPEN_CRYPT                        -1
#define _XOPEN_ENH_I18N                     -1
#define _XOPEN_LEGACY                       -1
#define _XOPEN_REALTIME                     -1
#define _XOPEN_REALTIME_THREADS             -1
#define _XOPEN_SHM                          -1
#define _XOPEN_STREAMS                      -1
#define _XOPEN_UNIX                         -1
#endif

#define __need_NULL
#define __need_size_t
#include <stddef.h>

#ifndef __AROS_GID_T_DECLARED
#define __AROS_GID_T_DECLARED
typedef __gid_t     gid_t;
#endif

#ifndef __AROS_INTPTR_T_DECLARED
#define __AROS_INTPTR_T_DECLARED
typedef __intptr_t  intptr_t;
#endif

#ifndef __AROS_OFF_T_DECLARED
#define __AROS_OFF_T_DECLARED
typedef __off_t     off_t;
#endif

#ifndef __AROS_PID_T_DECLARED
#define __AROS_PID_T_DECLARED
typedef __pid_t     pid_t;
#endif

#ifndef __AROS_SSIZE_T_DECLARED
#define __AROS_SSIZE_T_DECLARED
typedef __ssize_t   ssize_t;
#endif

#ifndef __AROS_UID_T_DECLARED
#define __AROS_UID_T_DECLARED
typedef __uid_t     uid_t;
#endif

#ifndef __AROS_USECONDS_T_DECLARED
#define __AROS_USECONDS_T_DECLARED
typedef __useconds_t    useconds_t;
#endif

/*
    Values for the second argument to access.
   These may be OR'd together.
*/
#define	R_OK	4		/* Test for read permission.  */
#define	W_OK	2		/* Test for write permission.  */
#define	X_OK	1		/* Test for execute permission.  */
#define	F_OK	0		/* Test for existence.  */

/*
    XXX Arguments for confstr()
*/

#define SEEK_SET    0           /* standard input file descriptor */
#define SEEK_CUR    1           /* standard output file descriptor */
#define SEEK_END    2           /* standard error file descriptor */

#if __XSI_VISIBLE || __POSIX_VISIBLE >= 200112
#define F_LOCK      0
#define F_TEST      1
#define F_TLOCK     2
#define F_ULOCK     3
#endif

/*
    XXX Arguments for pathconf()
    XXX Arguments for sysconf()
*/

#define STDIN_FILENO                        0
#define STDOUT_FILENO                       1
#define STDERR_FILENO                       2

__BEGIN_DECLS

/* Prototypes */
int         access(const char *path, int mode);
/* NOTIMPL unsigned    alarm(unsigned); */
int         chdir(const char *path);
int         chown(const char *path, uid_t owner, gid_t group);
int         close(int fd);
int         dup(int oldfd);
int         dup2(int oldfd, int newfd);
int         execl(const char *path, const char *arg, ...);
/* NOTIMPL int         execle(const char *path, const char *arg, ...); */
/* NOTIMPL int         execlp(const char *path, const char *arg, ...); */
/* NOTIMPL int         execv(const char *path, char *const argv[]); */
/* NOTIMPL int         execve(const char *path, char *const argv[]); */
int         execvp(const char *path, char *const argv[]);
void        _exit(int) __noreturn;
/* NOTIMPL long        fpathconf(int fd, int name); */
char        *getcwd(char *buf, size_t size);
gid_t       getegid(void);
uid_t       geteuid(void);
gid_t       getgid(void);
int         getgroups(int gidsetlen, gid_t *gidset);
/* NOTIMPL char        *getlogin(void); */
pid_t       getpgrp(void);
pid_t       getpid(void);
pid_t       getppid(void);
uid_t       getuid(void);
int         isatty(int fd);
int         link(const char *name1, const char *name2);
off_t       lseek(int filedes, off_t offset, int whence);
/* NOTIMPL long        pathconf(const char *path, int name); */
/* NOTIMPL int         pause(void); */
int         pipe(int filedes[2]);
ssize_t     read(int d, void *buf, size_t nbytes);
int         rmdir(const char *path);
int         setgid(gid_t gid);
/* NOTIMPL int         setpgid(pid_t pid, pid_t pgrp); */
/* NOTIMPL pid_t       setsid(void); */
int         setuid(uid_t uid);
unsigned    sleep(unsigned);
/* NOTIMPL long        sysconf(int name); */
/* NOTIMPL pid_t       tcgetpgrp(int fd); */
/* NOTIMPL int         tcsetpgrp(int fd, pid_t pgrp_id); */
/* NOTIMPL char        *ttyname(int fd); */
/* NOTIMPL int         ttyname_r(int fd, char *buf, size_t len); */
int         unlink(const char *path);
ssize_t     write(int fd, const void *buf, size_t nbytes);

#if __POSIX_VISIBLE >= 199209 || __XSI_VISIBLE
/* NOTIMPL size_t      confstr(int name, char *buf, size_t len); */
/* NOTIMPL int         getopt(int argc, char * const *argv, const char *optstring); */

/* NOTIMPL extern char *optarg; */
/* NOTIMPL extern int  optind, opterr, optopt; */
#endif

#if __POSIX_VISIBLE >= 199506 || __XSI_VISIBLE
int         fsync(int fd);
int         ftruncate(int fd, off_t length);
#endif

#if __POSIX_VISIBLE >= 199506
/* NOTIMPL int         getlogin_r(char *name, int len); */
#endif

#if __POSIX_VISIBLE >= 200112 || __XSI_VISIBLE
int         fchown(int fd, uid_t owner, gid_t group);
ssize_t     readlink(const char * restrict path, char * restrict buf, int bufsize);
#endif

#if __POSIX_VISIBLE >= 200112
/* NOTIMPL int         gethostname(char *name, size_t namelen); */
/* NOTIMPL int         setegid(gid_t egid); */
/* NOTIMPL int         seteuid(uid_t euid); */
#endif

#if __POSIX_VISIBLE >= 200112 || __XSI_VISIBLE >= 402 || __BSD_VISIBLE
int         symlink(const char *name1, const char *name2);
#endif

#if __XSI_VISIBLE
/* NOTIMPL char        *crypt(const char *key, const char *salt); */
/* NOTIMPL char        *ctermid(char *buf); */
/* NOTIMPL void        encrypt(char block[64], int flag); */
/* NOTIMPL int         fchdir(int fd); */
/* NOTIMPL long        gethostid(void); */
/* NOTIMPL pid_t       getpgid(pid_t); */
/* NOTIMPL pid_t       getsid(pid_t pid); */
/* NOTIMPL int         lchown(const char *path, uid_t owner, gid_t group); */
/* NOTIMPL int         lockf(int filedes, int function, off_t size); */
/* NOTIMPL int         nice(int incr); */
/* NOTIMPL ssize_t     pread(int d, void *buf, size_t nbytes, off_t offset); */
/* NOTIMPL ssize_t     pwrite(int d, const void *buf, size_t nbytes, off_t offset); */
/* NOTIMPL int         setpgrp(pid_t pid, pid_t pgrp); */
/* NOTIMPL int         setregid(gid_t rgid, gid_t egid); */
/* NOTIMPL int         setreuid(uid_t ruid, uid_t euid); */
void        swab(const void * restrict src, void * restrict dst, size_t len);
/* NOTIMPL void        sync(void); */
int         truncate(const char *path, off_t length);
/* NOTIMPL useconds_t  ualarm(useconds_t microseconds, useconds_t interval); */
/* NOTIMPL int         usleep(useconds_t microseconds); */
/* NOTIMPL pid_t       vfork(void); */
#endif

#if __POSIX_VERSION >= 200112
/* NOTIMPL int         fdatasync(int fd); */
#endif

int mknod(const char *pathname, mode_t mode, dev_t dev);

__END_DECLS

#endif /* _UNISTD_H_ */
