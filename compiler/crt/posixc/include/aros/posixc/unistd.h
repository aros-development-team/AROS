#ifndef _POSIXC_UNISTD_H_
#define _POSIXC_UNISTD_H_

/*
    Copyright © 1995-2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX.1-2008 header file unistd.h
*/

#include <aros/features.h>
#include <aros/system.h>

/*
 * What version of POSIX things do we (try to) implement.
 */
#define _POSIX_VERSION              200809L
#define _POSIX2_VERSION             200809L

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

/* FIXME: Defines may be out of date with current implementation */
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
#define _POSIX_SAVED_IDS                    1       /* FIXME: MUST IMPLEMENT */
#define _POSIX_SEMAPHORES                   -1
#define _POSIX_SHARED_MEMORY_OBJECTS        -1
#define _POSIX_SHELL                        1       /* FIXME: MUST IMPLEMENT */
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
#define _POSIX_THREAD_ROBUST_PRIO_INHERIT   -1
#define _POSIX_THREAD_ROBUST_PRIO_PROTECT   -1
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

/* FIXME: This has to be adapted for non-i386 CPUs */
#define _V6_ILP32_OFF32                     0
#define _V6_ILP32_OFFBIG                    -1
#define _V6_LP64_OFF64                      -1
#define _V6_LPBIG_OFFBIG                    -1
#define _POSIX_V6_ILP32_OFF32               _V6_ILP32_OFF32
#define _POSIX_V6_ILP32_OFFBIG              _V6_ILP32_OFFBIG
#define _POSIX_V6_LP32_OFFBIG               _V6_LP32_OFFBIG
#define _POSIX_V6_LPBIG_OFFBIG              _V6_LPBIG_OFFBIG
#define _POSIX_V7_ILP32_OFF32               _V6_ILP32_OFF32
#define _POSIX_V7_ILP32_OFFBIG              _V6_ILP32_OFFBIG
#define _POSIX_V7_LP32_OFFBIG               _V6_LP32_OFFBIG
#define _POSIX_V7_LPBIG_OFFBIG              _V6_LPBIG_OFFBIG

#define _POSIX2_C_BIND                      200809L
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
#define _POSIX2_SW_DEV                      -1          /* FIXME */
#define _POSIX2_UPE                         200809L

#define _XOPEN_CRYPT                        -1
#define _XOPEN_ENH_I18N                     -1
#define _XOPEN_LEGACY                       -1
#define _XOPEN_REALTIME                     -1
#define _XOPEN_REALTIME_THREADS             -1
#define _XOPEN_SHM                          -1
#define _XOPEN_STREAMS                      -1
#define _XOPEN_UNIX                         -1
#define _XOPEN_UUCP                         -1

#define _POSIX_ASYNC_IO                     -1
#define _POSIX_PRIO_IO                      -1
#define _POSIX_SYNC_IO                      -1
#define _POSIX_TIMESTAMP_RESOLUTION         -1
#define _POSIX2_SYMLINKS                    -1

#include <aros/types/null.h>

/*
    Values for the second argument to access.
    These may be OR'd together.
*/
#define F_OK    0   /* Test for existence.  */
#define R_OK    4   /* Test for read permission.  */
#define W_OK    2   /* Test for write permission.  */
#define X_OK    1   /* Test for execute permission.  */

/*
    FIXME: Arguments for confstr()

    _CS_PATH
    _CS_POSIX_V7_ILP32_OFF32_CFLAGS
    _CS_POSIX_V7_ILP32_OFF32_LDFLAGS
    _CS_POSIX_V7_ILP32_OFF32_LIBS
    _CS_POSIX_V7_ILP32_OFFBIG_CFLAGS
    _CS_POSIX_V7_ILP32_OFFBIG_LDFLAGS
    _CS_POSIX_V7_ILP32_OFFBIG_LIBS
    _CS_POSIX_V7_LP64_OFF64_CFLAGS
    _CS_POSIX_V7_LP64_OFF64_LDFLAGS
    _CS_POSIX_V7_LP64_OFF64_LIBS
    _CS_POSIX_V7_LPBIG_OFFBIG_CFLAGS
    _CS_POSIX_V7_LPBIG_OFFBIG_LDFLAGS
    _CS_POSIX_V7_LPBIG_OFFBIG_LIBS
    _CS_POSIX_V7_THREADS_CFLAGS
    _CS_POSIX_V7_THREADS_LDFLAGS
    _CS_POSIX_V7_WIDTH_RESTRICTED_ENVS
    _CS_V7_ENV

    confstr Issue 6 compatibility:

    _CS_POSIX_V6_ILP32_OFF32_CFLAGS
    _CS_POSIX_V6_ILP32_OFF32_LDFLAGS
    _CS_POSIX_V6_ILP32_OFF32_LIBS
    _CS_POSIX_V6_ILP32_OFFBIG_CFLAGS
    _CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS
    _CS_POSIX_V6_ILP32_OFFBIG_LIBS
    _CS_POSIX_V6_LP64_OFF64_CFLAGS
    _CS_POSIX_V6_LP64_OFF64_LDFLAGS
    _CS_POSIX_V6_LP64_OFF64_LIBS
    _CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS
    _CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS
    _CS_POSIX_V6_LPBIG_OFFBIG_LIBS
    _CS_POSIX_V6_WIDTH_RESTRICTED_ENVS
    _CS_V6_ENV
*/

#include <aros/types/seek.h> /* SEEK_SET, SEEK_CUR and SEEK_END */

/* Arguments for lockf() */
#define F_LOCK      0
#define F_TEST      1
#define F_TLOCK     2
#define F_ULOCK     3

/* Arguments for pathconf() */
/* NOTIMPL #define _PC_2_SYMLINKS 1 */
/* NOTIMPL #define _PC_ALLOC_SIZE_MIN 2 */
/* NOTIMPL #define _PC_ASYNC_IO 3 */
/* NOTIMPL #define _PC_CHOWN_RESTRICTED 4 */
/* NOTIMPL #define _PC_FILESIZEBITS 5 */
/* NOTIMPL #define _PC_LINK_MAX 6 */
/* NOTIMPL #define _PC_MAX_CANON 7 */
/* NOTIMPL #define _PC_MAX_INPUT 8 */
/* NOTIMPL #define _PC_NAME_MAX 9 */
/* NOTIMPL #define _PC_NO_TRUNC 10 */
#define _PC_PATH_MAX 11
/* NOTIMPL #define _PC_PIPE_BUF 12 */
/* NOTIMPL #define _PC_PRIO_IO 13 */
/* NOTIMPL #define _PC_REC_INCR_XFER_SIZE 14 */
/* NOTIMPL #define _PC_REC_MAX_XFER_SIZE 15 */
/* NOTIMPL #define _PC_REC_MIN_XFER_SIZE 16 */
/* NOTIMPL #define _PC_REC_XFER_ALIGN 17 */
/* NOTIMPL #define _PC_SYMLINK_MAX 18 */
/* NOTIMPL #define _PC_SYNC_IO 19 */
/* NOTIMPL #define _PC_TIMESTAMP_RESOLUTION 20 */
#define _PC_VDISABLE 21

/* Arguments for sysconf() */
enum {
    _SC_2_C_BIND,
    _SC_2_C_DEV,
    _SC_2_CHAR_TERM,
    _SC_2_FORT_DEV,
    _SC_2_FORT_RUN,
    _SC_2_LOCALEDEF,
    _SC_2_PBS,
    _SC_2_PBS_ACCOUNTING,
    _SC_2_PBS_CHECKPOINT,
    _SC_2_PBS_LOCATE,
    _SC_2_PBS_MESSAGE,
    _SC_2_PBS_TRACK,
    _SC_2_SW_DEV,
    _SC_2_UPE,
    _SC_2_VERSION,
    _SC_ADVISORY_INFO,
    _SC_AIO_LISTIO_MAX,
    _SC_AIO_MAX,
    _SC_AIO_PRIO_DELTA_MAX,
    _SC_ARG_MAX,
    _SC_ASYNCHRONOUS_IO,
    _SC_ATEXIT_MAX,
    _SC_BARRIERS,
    _SC_BC_BASE_MAX,
    _SC_BC_DIM_MAX,
    _SC_BC_SCALE_MAX,
    _SC_BC_STRING_MAX,
    _SC_CHILD_MAX,
    _SC_CLK_TCK,
    _SC_CLOCK_SELECTION,
    _SC_COLL_WEIGHTS_MAX,
    _SC_CPUTIME,
    _SC_DELAYTIMER_MAX,
    _SC_EXPR_NEST_MAX,
    _SC_FSYNC,
    _SC_GETGR_R_SIZE_MAX,
    _SC_GETPW_R_SIZE_MAX,
    _SC_HOST_NAME_MAX,
    _SC_IOV_MAX,
    _SC_IPV6,
    _SC_JOB_CONTROL,
    _SC_LINE_MAX,
    _SC_LOGIN_NAME_MAX,
    _SC_MAPPED_FILES,
    _SC_MEMLOCK,
    _SC_MEMLOCK_RANGE,
    _SC_MEMORY_PROTECTION,
    _SC_MESSAGE_PASSING,
    _SC_MONOTONIC_CLOCK,
    _SC_MQ_OPEN_MAX,
    _SC_MQ_PRIO_MAX,
    _SC_NGROUPS_MAX,
    _SC_OPEN_MAX,
    _SC_PAGE_SIZE,
    _SC_PAGESIZE,
    _SC_PRIORITIZED_IO,
    _SC_PRIORITY_SCHEDULING,
    _SC_RAW_SOCKETS,
    _SC_RE_DUP_MAX,
    _SC_READER_WRITER_LOCKS,
    _SC_REALTIME_SIGNALS,
    _SC_REGEXP,
    _SC_RTSIG_MAX,
    _SC_SAVED_IDS,
    _SC_SEM_NSEMS_MAX,
    _SC_SEM_VALUE_MAX,
    _SC_SEMAPHORES,
    _SC_SHARED_MEMORY_OBJECTS,
    _SC_SHELL,
    _SC_SIGQUEUE_MAX,
    _SC_SPAWN,
    _SC_SPIN_LOCKS,
    _SC_SPORADIC_SERVER,
    _SC_SS_REPL_MAX,
    _SC_STREAM_MAX,
    _SC_SYMLOOP_MAX,
    _SC_SYNCHRONIZED_IO,
    _SC_THREAD_ATTR_STACKADDR,
    _SC_THREAD_ATTR_STACKSIZE,
    _SC_THREAD_CPUTIME,
    _SC_THREAD_DESTRUCTOR_ITERATIONS,
    _SC_THREAD_KEYS_MAX,
    _SC_THREAD_PRIO_INHERIT,
    _SC_THREAD_PRIO_PROTECT,
    _SC_THREAD_PRIORITY_SCHEDULING,
    _SC_THREAD_PROCESS_SHARED,
    _SC_THREAD_ROBUST_PRIO_INHERIT,
    _SC_THREAD_ROBUST_PRIO_PROTECT,
    _SC_THREAD_SAFE_FUNCTIONS,
    _SC_THREAD_SPORADIC_SERVER,
    _SC_THREAD_STACK_MIN,
    _SC_THREAD_THREADS_MAX,
    _SC_THREADS,
    _SC_TIMEOUTS,
    _SC_TIMER_MAX,
    _SC_TIMERS,
    _SC_TRACE,
    _SC_TRACE_EVENT_FILTER,
    _SC_TRACE_EVENT_NAME_MAX,
    _SC_TRACE_INHERIT,
    _SC_TRACE_LOG,
    _SC_TRACE_NAME_MAX,
    _SC_TRACE_SYS_MAX,
    _SC_TRACE_USER_EVENT_MAX,
    _SC_TTY_NAME_MAX,
    _SC_TYPED_MEMORY_OBJECTS,
    _SC_TZNAME_MAX,
    _SC_V7_ILP32_OFF32,
    _SC_V7_ILP32_OFFBIG,
    _SC_V7_LP64_OFF64,
    _SC_V7_LPBIG_OFFBIG,
    _SC_V6_ILP32_OFF32,
    _SC_V6_ILP32_OFFBIG,
    _SC_V6_LP64_OFF64,
    _SC_V6_LPBIG_OFFBIG,
    _SC_VERSION,
    _SC_XOPEN_CRYPT,
    _SC_XOPEN_ENH_I18N,
    _SC_XOPEN_REALTIME,
    _SC_XOPEN_REALTIME_THREADS,
    _SC_XOPEN_SHM,
    _SC_XOPEN_STREAMS,
    _SC_XOPEN_UNIX,
    _SC_XOPEN_UUCP,
    _SC_XOPEN_VERSION
};

#define STDIN_FILENO    0
#define STDOUT_FILENO   1
#define STDERR_FILENO   2

#include <aros/types/size_t.h>
#include <aros/types/ssize_t.h>
#include <aros/types/uid_t.h>
#include <aros/types/gid_t.h>
#include <aros/types/off_t.h>
#include <aros/types/pid_t.h>
#include <aros/types/intptr_t.h>

__BEGIN_DECLS

/* POSIX.1-1990 (default with _POSIX_SOURCE or none) */
int access(const char *path, int mode);
int chdir(const char *path);
int close(int fd);
int dup(int oldfd);
int dup2(int oldfd, int newfd);
void _exit(int) __noreturn;
int execl(const char *path, const char *arg, ...);
int execv(const char *path, char *const argv[]);
int execve(const char *path, char *const argv[], char *const envp[]);
int fchdir(int fd);
int fsync(int fd);
pid_t getpid(void);
pid_t getppid(void);
uid_t getuid(void);
gid_t getgid(void);
uid_t geteuid(void);
gid_t getegid(void);
int getgroups(int gidsetlen, gid_t *gidset);
int isatty(int fd);
off_t lseek(int filedes, off_t offset, int whence);
# if defined(__off64_t_defined)
__off64_t lseek64(int filedes, __off64_t offset, int whence);
# endif
int pipe(int filedes[2]);
ssize_t read(int d, void *buf, size_t nbytes);
int rmdir(const char *path);
int setuid(uid_t uid);
int setgid(gid_t gid);
unsigned sleep(unsigned);
int unlink(const char *path);
ssize_t write(int fd, const void *buf, size_t nbytes);

/* POSIX.1-2001 and later */
#if defined(__cplusplus) || !defined(__STRICT_ANSI__)
int execvp(const char *path, char *const argv[]);
int execlp(const char *path, const char *arg, ...);
char *getcwd(char *buf, size_t size);
long pathconf(const char *path, int name);
long sysconf(int name);
char *ttyname(int fd);
int getopt(int argc, char * const argv[], const char *optstring);
extern char *optarg;
extern int optind, opterr, optopt;
#endif

/* POSIX.1-2008 and XSI (or available as extensions) */
#if defined(__cplusplus) || (!defined(__STRICT_ANSI__) && \
    (!defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)))
int ftruncate(int fd, off_t length);
int truncate(const char *path, off_t length);
void sync(void);
pid_t setsid(void);
int setegid(gid_t egid);
int seteuid(uid_t euid);
char *getlogin(void);
int chown(const char *path, uid_t owner, gid_t group);
int fchown(int fd, uid_t owner, gid_t group);
ssize_t readlink(const char *restrict path, char *restrict buf, size_t bufsize);
int symlink(const char *name1, const char *name2);
void swab(const void *restrict src, void *restrict dst, size_t len);
pid_t tcgetpgrp(int fd);
#endif

/* GNU or BSD extensions */
#if defined(__cplusplus) || (!defined(__STRICT_ANSI__) && \
    (defined(_BSD_SOURCE) || defined(_GNU_SOURCE) || defined(_XOPEN_SOURCE)))
#include <aros/types/useconds_t.h>
char *getpass(const char *prompt);
pid_t getpgid(pid_t);
pid_t getpgrp(void);
pid_t vfork(void);
int link(const char *name1, const char *name2);
int usleep(useconds_t microseconds);
#endif

#if defined(_GNU_SOURCE) || (_POSIX_C_SOURCE >= 200809L)
int unlinkat(int dirfd, const char *pathname, int flags);
#endif

/* AROS-specific extensions */
void sharecontextwithchild(int share);

__END_DECLS

#endif /* _POSIXC_UNISTD_H_ */
