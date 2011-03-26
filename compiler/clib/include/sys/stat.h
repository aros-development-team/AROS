#ifndef _SYS_STAT_H
#define _SYS_STAT_H

/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file sys/stat.h
    Lang: english
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
    time_t          st_atime;	    /* time of last access */
    long            st_spare1;
    time_t          st_mtime;	    /* time of last data modification */
    long            st_spare2;
    time_t          st_ctime;	    /* time of last file status change */
    long            st_spare3;
    blksize_t       st_blksize;	    /* optimal blocksize for I/O */
    blkcnt_t        st_blocks;	    /* blocks allocated for file */
    unsigned long   st_flags;	    /* user defined flags for file */
    unsigned long   st_gen;         /* file generation number */
};

#define	S_ISUID	0004000			/* set user id on execution */
#define	S_ISGID	0002000			/* set group id on execution */

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

#define	S_IFMT	 0170000		/* type of file */
#define	S_IFIFO	 0010000		/* named pipe (fifo) */
#define	S_IFCHR	 0020000		/* character special */
#define	S_IFDIR	 0040000		/* directory */
#define	S_IFBLK	 0060000		/* block special */
#define	S_IFREG	 0100000		/* regular */
#define	S_IFLNK	 0120000		/* symbolic link */
#define	S_IFSOCK 0140000		/* socket */
#define	S_ISVTX	 0001000		/* save swapped text even after use */

#define	ACCESSPERMS	(S_IRWXU|S_IRWXG|S_IRWXO)	/* 0777 */
							/* 7777 */
#define	ALLPERMS	(S_ISUID|S_ISGID|S_ISTXT|S_IRWXU|S_IRWXG|S_IRWXO)
							/* 0666 */
#define	DEFFILEMODE	(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)

#define S_BLKSIZE	512		/* block size used in the stat struct */


#define	S_ISDIR(m)	((m & 0170000) == 0040000)	/* directory */
#define	S_ISCHR(m)	((m & 0170000) == 0020000)	/* char special */
#define	S_ISBLK(m)	((m & 0170000) == 0060000)	/* block special */
#define	S_ISREG(m)	((m & 0170000) == 0100000)	/* regular file */
#define	S_ISLNK(m)	((m & 0170000) == 0120000)	/* symbolic link */
#define S_ISSOCK(m)	((m & 0170000) == 0140000)	/* socket */
#define	S_ISFIFO(m)	((m & 0170000) == 0010000)	/* fifo */

__BEGIN_DECLS

int stat(const char * restrict path, struct stat * restrict sb);
int lstat(const char * restrict path, struct stat * restrict sb);
int fstat(int fd, struct stat *sb);

mode_t umask(mode_t numask);
int chmod(const char *path, mode_t mode);
int fchmod(int fildes, mode_t mode);

int mkdir(const char *path, mode_t mode);
int mkfifo(const char *path, mode_t mode);
int mknod(const char *path, mode_t mode, dev_t dev);

__END_DECLS

#endif /* _SYS_STAT_H */
