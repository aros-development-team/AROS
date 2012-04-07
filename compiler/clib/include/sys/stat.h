#ifndef _SYS_STAT_H
#define _SYS_STAT_H

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX.1-2008 header file sys/stat.h
*/
#include <aros/system.h>

/* POSIX.1-2008 */
#include <aros/types/blk_t.h> /* blkcnt_t and blksize_t */
#include <aros/types/dev_t.h>
#include <aros/types/ino_t.h>
#include <aros/types/mode_t.h>
#include <aros/types/nlink_t.h>
#include <aros/types/uid_t.h>
#include <aros/types/gid_t.h>
#include <aros/types/off_t.h>
#include <aros/types/time_t.h>
#include <aros/types/timespec_s.h>

struct stat
{
    dev_t           st_dev;	    /* inode's device */
    ino_t           st_ino;	    /* inode's number */
    mode_t          st_mode;	    /* inode protection mode */
    nlink_t         st_nlink;	    /* number of hard links */
    uid_t           st_uid;	    /* user ID of the file's owner */
    gid_t           st_gid;	    /* group ID of the file's group */
    dev_t           st_rdev;	    /* device type */
    off_t           st_size;	    /* file size, in bytes */
    struct timespec st_atim;	    /* time of last access */
    struct timespec st_mtim;	    /* time of last data modification */
    struct timespec st_ctim;	    /* time of last file status change */
    blksize_t       st_blksize;	    /* optimal blocksize for I/O */
    blkcnt_t        st_blocks;	    /* blocks allocated for file */
    unsigned long   st_flags;	    /* user defined flags for file */
    unsigned long   st_gen;         /* file generation number */
};

#define st_atime st_atim.tv_sec
#define st_mtime st_mtim.tv_sec
#define st_ctime st_ctim.tv_sec

/* mode_t values */
#define	S_IFMT	 0170000		/* type of file */
#define	S_IFBLK	 0060000		/* block special */
#define	S_IFCHR	 0020000		/* character special */
#define	S_IFIFO	 0010000		/* named pipe (fifo) */
#define	S_IFREG	 0100000		/* regular */
#define	S_IFDIR	 0040000		/* directory */
#define	S_IFLNK	 0120000		/* symbolic link */
#define	S_IFSOCK 0140000		/* socket */

#define	S_IRWXU	0000700			/* RWX mask for owner */
#define	S_IRUSR	0000400			/* R for owner */
#define	S_IWUSR	0000200			/* W for owner */
#define	S_IXUSR	0000100			/* X for owner */

#define	S_IRWXG	0000070			/* RWX mask for group */
#define	S_IRGRP	0000040			/* R for group */
#define	S_IWGRP	0000020			/* W for group */
#define	S_IXGRP	0000010			/* X for group */

#define	S_IRWXO	0000007			/* RWX mask for other */
#define	S_IROTH	0000004			/* R for other */
#define	S_IWOTH	0000002			/* W for other */
#define	S_IXOTH	0000001			/* X for other */
#define	S_ISUID	0004000			/* set user id on execution */
#define	S_ISGID	0002000			/* set group id on execution */
#define	S_ISVTX	0001000			/* save swapped text even after use */
#define	S_ISBLK(m)	((m & 0170000) == 0060000)	/* block special */
#define	S_ISCHR(m)	((m & 0170000) == 0020000)	/* char special */
#define	S_ISDIR(m)	((m & 0170000) == 0040000)	/* directory */
#define	S_ISFIFO(m)	((m & 0170000) == 0010000)	/* fifo */
#define	S_ISREG(m)	((m & 0170000) == 0100000)	/* regular file */
#define	S_ISLNK(m)	((m & 0170000) == 0120000)	/* symbolic link */
#define S_ISSOCK(m)	((m & 0170000) == 0140000)	/* socket */

/* NOTIMPL
S_TYPEISMQ(buf)
S_TYPEISSEM(buf)
S_TYPEISSHM(buf)
S_TYPEISTMO(buf)
*/

/* Extras */
/* FIXME: Are they needed ? */
#define	ACCESSPERMS	(S_IRWXU|S_IRWXG|S_IRWXO)	/* 0777 */
							/* 7777 */
#define	ALLPERMS	(S_ISUID|S_ISGID|S_ISTXT|S_IRWXU|S_IRWXG|S_IRWXO)
							/* 0666 */
#define	DEFFILEMODE	(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)

#define S_BLKSIZE	512		/* block size used in the stat struct */



__BEGIN_DECLS

int chmod(const char *path, mode_t mode);
int fchmod(int fildes, mode_t mode);
/* NOTIMPL int fchmodat(int, const char *, mode_t, int); */
int fstat(int fd, struct stat *sb);
/* NOTIMPL int fstatat(int, const char *restrict, struct stat *restrict, int); */
/* NOTIMPL int futimens(int, const struct timespec [2]); */
int lstat(const char * restrict path, struct stat * restrict sb);
int mkdir(const char *path, mode_t mode);
/* NOTIMPL int mkdirat(int, const char *, mode_t); */
/* NOTIMPL int mkfifo(const char *path, mode_t mode); */
/* NOTIMPL int mkfifoat(int, const char *, mode_t); */
int mknod(const char *path, mode_t mode, dev_t dev);
/* NOTIMPL int mknodat(int, const char *, mode_t, dev_t); */
int stat(const char * restrict path, struct stat * restrict sb);
mode_t umask(mode_t numask);
/* NOTIMPL int utimensat(int, const char *, const struct timespec [2], int); */

__END_DECLS

#endif /* _SYS_STAT_H */
