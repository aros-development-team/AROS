#ifndef _POSIXC_FCNTL_H_
#define _POSIXC_FCNTL_H_

/*
    Copyright © 1995-2025, The AROS Development Team.
    All rights reserved.
    $Id$

    POSIX.1-2008 header file: fcntl.h
*/

#include <aros/features.h>
#include <aros/system.h>
#include <sys/stat.h>

#include <aros/types/mode_t.h>
#include <aros/types/off_t.h>
#include <aros/types/pid_t.h>
#include <aros/types/seek.h>

/* --- fcntl() commands --- */
#define F_DUPFD         0
#define F_DUPFD_CLOEXEC 1
#define F_GETFD         2
#define F_SETFD         3
#define F_GETFL         4
#define F_SETFL         5
#define F_GETLK         6
#define F_SETLK         7
#define F_SETLKW        8
#define F_GETOWN        9
#define F_SETOWN        10

#if defined(_GNU_SOURCE) || (_XOPEN_SOURCE >= 500)
#define F_GETLK64       5
#define F_SETLK64       6
#define F_SETLKW64      7
#define F_GETSIG        11
#define F_SETSIG        12
#endif

#define FD_CLOEXEC      1

/* Lock types */
#define F_RDLCK         0
#define F_WRLCK         1
#define F_UNLCK         2

/* File creation flags */
#define O_CREAT         0x0040
#define O_EXCL          0x0080
#define O_NOCTTY        0
#define O_TRUNC         0x0200
/* NOTIMPL O_TTY_INIT */

#if defined(_BSD_SOURCE) || defined(_GNU_SOURCE)
#define O_SHLOCK        0
#define O_EXLOCK        0x0100
#endif

/* File status flags */
#define O_APPEND        0x0400

#if defined(_GNU_SOURCE) || (_XOPEN_SOURCE >= 500)
#define O_DSYNC         0x4000
#define O_RSYNC         0x8000
#define O_SYNC          0x1000
#endif

#if defined(_GNU_SOURCE)
#define O_ASYNC         0x2000
#define O_FSYNC         O_SYNC
#define O_NDELAY        O_NONBLOCK
#endif

#define O_NONBLOCK      0x0800
#define O_ACCMODE       0x0003

#define O_EXEC          0x0004
#define O_RDONLY        0x0001
#define O_WRONLY        0x0002
#define O_RDWR          (O_RDONLY | O_WRONLY)

/* NOTIMPL O_SEARCH */

#if defined(_GNU_SOURCE)
#define O_READ          O_RDONLY
#define O_WRITE         O_WRONLY
#endif

#define O_CLOEXEC       0x10000
#define O_DIRECTORY     0x20000
#define O_NOFOLLOW      0x40000

#define AT_FDCWD                -100
#define AT_EACCESS              0x01
#define AT_SYMLINK_NOFOLLOW     0x02
#define AT_SYMLINK_FOLLOW       0x04
#define AT_REMOVEDIR            0x08

/* Advice for posix_fadvise */
#define POSIX_FADV_DONTNEED     1
#define POSIX_FADV_NOREUSE      2
#define POSIX_FADV_NORMAL       3
#define POSIX_FADV_RANDOM       4
#define POSIX_FADV_SEQUENTIAL   5
#define POSIX_FADV_WILLNEED     6

#if defined(_BSD_SOURCE) || defined(_GNU_SOURCE)
/* BSD-style flock values */
#define F_EXLCK     4
#define F_SHLCK     8

#define LOCK_SH     1
#define LOCK_EX     2
#define LOCK_NB     4
#define LOCK_UN     8
#endif

struct flock
{
    short   l_type;
    short   l_whence;
    off_t   l_start;
    off_t   l_len;
    pid_t   l_pid;
};

__BEGIN_DECLS

int creat(const char *filename, int mode);

#if defined(_GNU_SOURCE) || (_XOPEN_SOURCE >= 500)
int creat64(const char *filename, int mode);
#endif

int fcntl(int fd, int cmd, ...);
int open(const char *filename, int flags, ...);
int openat(int dirfd, const char *path, int flags, ...);

/* NOTIMPL int posix_fadvise(int fd, off_t offset, size_t len, int advice); */
/* NOTIMPL int posix_fallocate(int fd, off_t offset, size_t len); */

__END_DECLS

#endif /* _POSIXC_FCNTL_H_ */
