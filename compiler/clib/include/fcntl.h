#ifndef _FCNTL_H_
#define _FCNTL_H_

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file fcntl.h
    Lang: english
*/

#include <sys/_types.h>
#include <sys/cdefs.h>

#if __XSI_VISIBLE
#include <unistd.h>
#include <sys/stat.h>
#endif

#ifndef __AROS_PID_T_DECLARED
#define __AROS_PID_T_DECLARED
typedef __pid_t         pid_t;
#endif

#ifndef __AROS_MODE_T_DECLARED
#define __AROS_MODE_T_DECLARED
typedef __mode_t        mode_t;
#endif

#ifndef __AROS_OFF_T_DECLARED
#define __AROS_OFF_T_DECLARED
typedef __off_t         off_t;
#endif

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
#define O_CREAT		0x0040
#define O_EXCL		0x0080
#define O_SHLOCK        0         /* files are always opened in shared mode,
					if not otherwise specified */
#define O_EXLOCK        0x0100
#define O_TRUNC		0x0200

/* Operating modes */
#define O_APPEND	0x0400
#define O_NONBLOCK	0x0800
#define O_NDELAY	O_NONBLOCK  /* Alias */
#define O_SYNC		0x1000
#define O_FSYNC         O_SYNC	    /* Alias */
#define O_ASYNC         0x2000
#define O_DSYNC		0x4000	    /* Different from O_SYNC */
#define O_RSYNC		0x8000	    /* Read sync */

/* Values for the second argument to `fcntl'.  */
#define F_DUPFD		0	/* Duplicate file descriptor.  */
#define F_GETFD		1	/* Get file descriptor flags.  */
#define F_SETFD		2	/* Set file descriptor flags.  */
#define F_GETFL		3	/* Get file status flags.  */
#define F_SETFL		4	/* Set file status flags.  */
#define F_GETLK		5	/* Get record locking info.  */
#define F_SETLK		6	/* Set record locking info (non-blocking).  */
#define F_SETLKW	7	/* Set record locking info (blocking).  */

/* XXX missing */
#define F_GETLK64	5	/* Get record locking info.  */
#define F_SETLK64	6	/* Set record locking info (non-blocking).  */
#define F_SETLKW64	7	/* Set record locking info (blocking).  */

# define F_SETOWN	8	/* Get owner of socket (receiver of SIGIO).  */
# define F_GETOWN	9	/* Set owner of socket (receiver of SIGIO).  */

# define F_SETSIG	10	/* Set number of signal to be sent.  */
# define F_GETSIG	11	/* Get number of signal to be sent.  */

/* For F_[GET|SET]FD.  */
#define FD_CLOEXEC	1	/* actually anything with low bit set goes */

/* For posix fcntl() and `l_type' field of a `struct flock' for lockf().  */
#define F_RDLCK		0	/* Read lock.  */
#define F_WRLCK		1	/* Write lock.  */
#define F_UNLCK		2	/* Remove lock.  */

/* for old implementation of bsd flock () */
#define F_EXLCK		4	/* or 3 */
#define F_SHLCK		8	/* or 4 */

#ifdef __BSD_VISIBLE
/* operations for bsd flock(), also used by the kernel implementation */
# define LOCK_SH	1	/* shared lock */
# define LOCK_EX	2	/* exclusive lock */
# define LOCK_NB	4	/* or'd with one of the above to prevent
				   blocking */
# define LOCK_UN	8	/* remove lock */
#endif

#if __POSIX_VERSION >= 200112
#define POSIX_FADV_NORMAL       1
#define POSIX_FADV_SEQUENTIAL   2
#define POSIX_FADV_RANDOM       3
#define POSIX_FADV_WILLNEED     4
#define POSIX_FADV_DONTNEED     5
#define POSIX_FADV_NOREUSE      6
#endif

struct flock
{
    short	l_type;	    /* type of lock F_RDLCK, F_WRLCK, F_UNLCK */
    short	l_whence;   /* flag for starting offset */
    off_t	l_start;    /* starting offset */
    off_t	l_len;	    /* size, if 0 then until EOF */
    pid_t	l_pid;	    /* pid of process holding the lock. */
};

/* Prototypes */
__BEGIN_DECLS

int fcntl (int fd, int cmd, ...);
int open  (const char * filename, int flags, ...);
int creat (const char * filename, int mode);

#if __POSIX_VERSION >= 200112
/* NOTIMPL int posix_fadvise(int fd, off_t offset, size_t len, int advice); */
/* NOTIMPL int posix_fallocate(int fd, off_t offset, size_t len); */
#endif

__END_DECLS

#endif /* _FCNTL_H_ */
