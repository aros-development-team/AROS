#ifndef _POSIXC_FCNTL_H_
#define _POSIXC_FCNTL_H_

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX.1-2008 header file fcntl.h
*/

#include <aros/system.h>

/* Both Linux and NetBSD seem to include this file and
   POSIX.1-2008 allows its inclusion
 */
#include <sys/stat.h>

#include <aros/types/mode_t.h>
#include <aros/types/off_t.h>
#include <aros/types/pid_t.h>

/* cmd argument for fcntl() */
#define F_DUPFD		0	/* Duplicate file descriptor.  */
#define F_DUPFD_CLOEXEC 1       /* Duplicate file descriptor with FD_CLOEXEC set */
#define F_GETFD		2	/* Get file descriptor flags.  */
#define F_SETFD		3	/* Set file descriptor flags.  */
#define F_GETFL		4	/* Get file status flags.  */
#define F_SETFL		5	/* Set file status flags.  */
#define F_GETLK		6	/* Get record locking info.  */
#define F_SETLK		7	/* Set record locking info (non-blocking).  */
#define F_SETLKW	8	/* Set record locking info (blocking).  */
#define F_GETOWN	9	/* Set owner of socket (receiver of SIGIO).  */
#define F_SETOWN	10	/* Get owner of socket (receiver of SIGIO).  */

/* Extra */
/* FIXME: Are they needed ? Should we add support ? */
#define F_GETLK64	5	/* Get record locking info.  */
#define F_SETLK64	6	/* Set record locking info (non-blocking).  */
#define F_SETLKW64	7	/* Set record locking info (blocking).  */
#define F_GETSIG	11	/* Get number of signal to be sent.  */
#define F_SETSIG	12	/* Set number of signal to be sent.  */

/* For F_(GET|SET)FD.  */
#define FD_CLOEXEC	1	/* actually anything with low bit set goes */

/* For posix fcntl() and `l_type' field of a `struct flock' for lockf().  */
#define F_RDLCK		0	/* Read lock.  */
#define F_WRLCK		1	/* Write lock.  */
#define F_UNLCK		2	/* Remove lock.  */

/* For l_whence */
#include <aros/types/seek.h>

/* file creation flags for open() */
#define O_CREAT		0x0040
#define O_EXCL		0x0080
#define O_NOCTTY	0         /* We ignore this one */
#define O_TRUNC		0x0200
/* NOTIMPL O_TTY_INIT */

/* Extension */
#define O_SHLOCK        0         /* files are always opened in shared mode,
					if not otherwise specified */
#define O_EXLOCK        0x0100   /* FIXME: Remove ? */


/* file status flags for open()/fcntl() */
#define O_APPEND	0x0400
#define O_DSYNC		0x4000	    /* Different from O_SYNC */
#define O_NONBLOCK	0x0800
#define O_RSYNC		0x8000	    /* Read sync */
#define O_SYNC		0x1000

/* Extension */
/* FIXME: Are they needed ? */
#define O_ASYNC         0x2000
#define O_FSYNC         O_SYNC	    /* Alias */
#define O_NDELAY	O_NONBLOCK  /* Alias */


/* mask for file access */
#define O_ACCMODE	0x0003

/* file access modes */
#define O_EXEC          0x0004 /* Not included in modes & O_ACCMODE */
#define O_RDONLY	0x0001
#define O_RDWR		(O_RDONLY | O_WRONLY)
/* NOTIMPL O_SEARCH */
#define O_WRONLY	0x0002

/* The GNU system specifies these */
#define O_READ          O_RDONLY
#define O_WRITE         O_WRONLY


/* Special value for *at() */
/* NOTIMPL #define AT_FDCWD */

/* flag for faccessat() */
/* NOTIMPL #define AT_EACCESS */

/* flag for fstatat(), fchmodat(), fchownat() and utimensat() */
/* NOTIMPL #define AT_SYMLINK_NOFOLLOW */

/* flag for linkat() */
/* NOTIMPL #define AT_SYMLINK_FOLLOW */


/* flags for open() */
/* NOTIMPL #define O_CLOEXEC */
/* NOTIMPL #define O_DIRECTORY */
/* NOTIMPL #define O_NOFOLLOW */

/* flag for unlinkat() */
/* NOTIMPL #define AT_REMOVEDIR */

/* advice arguemnt for posix_fadvise() */
#define POSIX_FADV_DONTNEED     1
#define POSIX_FADV_NOREUSE      2
#define POSIX_FADV_NORMAL       3
#define POSIX_FADV_RANDOM       4
#define POSIX_FADV_SEQUENTIAL   5
#define POSIX_FADV_WILLNEED     6


/* for old implementation of bsd flock () */
#define F_EXLCK		4	/* or 3 */
#define F_SHLCK		8	/* or 4 */

/* operations for bsd flock(), also used by the kernel implementation */
# define LOCK_SH	1	/* shared lock */
# define LOCK_EX	2	/* exclusive lock */
# define LOCK_NB	4	/* or'd with one of the above to prevent
				   blocking */
# define LOCK_UN	8	/* remove lock */


struct flock
{
    short	l_type;	    /* type of lock F_RDLCK, F_WRLCK, F_UNLCK */
    short	l_whence;   /* flag for starting offset */
    off_t	l_start;    /* starting offset */
    off_t	l_len;	    /* size, if 0 then until EOF */
    pid_t	l_pid;	    /* pid of process holding the lock. */
};

__BEGIN_DECLS

int creat (const char * filename, int mode);
int fcntl (int fd, int cmd, ...);
int open  (const char * filename, int flags, ...);
/* NOTIMPL int openat(int, const char *, int, ...); */
/* NOTIMPL int posix_fadvise(int fd, off_t offset, size_t len, int advice); */
/* NOTIMPL int posix_fallocate(int fd, off_t offset, size_t len); */

__END_DECLS

#endif /* _POSIXC_FCNTL_H_ */
