#ifndef _SYS_STAT_H
#define _SYS_STAT_H

/*
    Copyright 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI-C header file sys/stat.h
    Lang: english
*/

#include <sys/types.h>
#include <time.h>

struct stat
{
	dev_t	st_dev;			/* inode's device */
	ino_t	st_ino;			/* inode's number */
	mode_t	st_mode;		/* inode protection mode */
	nlink_t	st_nlink;		/* number of hard links */
	uid_t	st_uid;			/* user ID of the file's owner */
	gid_t	st_gid;			/* group ID of the file's group */
	dev_t	st_rdev;		/* device type */
	off_t	st_size;		/* file size, in bytes */
	time_t	st_atime;		/* time of last access */
	long	st_spare1;
	time_t	st_mtime;		/* time of last data modification */
	long	st_spare2;
	time_t	st_ctime;		/* time of last file status change */
	long	st_spare3;
	long	st_blksize;		/* optimal blocksize for I/O */
	long	st_blocks;		/* blocks allocated for file */
	u_long	st_flags;		/* user defined flags for file */
	u_long	st_gen;			/* file generation number */
};

#define	S_ISUID	0004000			/* set user id on execution */
#define	S_ISGID	0002000			/* set group id on execution */

#define	S_IRWXU	0000700			/* RWX mask for owner */
#define	S_IRUSR	0000400			/* R for owner */
#define	S_IWUSR	0000200			/* W for owner */
#define	S_IXUSR	0000100			/* X for owner */

#define	S_ISDIR(m)	((m & 0170000) == 0040000)	/* directory */
#define	S_ISCHR(m)	((m & 0170000) == 0020000)	/* char special */
#define	S_ISBLK(m)	((m & 0170000) == 0060000)	/* block special */
#define	S_ISREG(m)	((m & 0170000) == 0100000)	/* regular file */
#define	S_ISLNK(m)	((m & 0170000) == 0120000)	/* symbolic link */
#define	S_ISFIFO(m)	((m & 0170000) == 0010000)	/* fifo */

int mkdir(const char *path, mode_t mode);

#endif /* _SYS_STAT_H */
