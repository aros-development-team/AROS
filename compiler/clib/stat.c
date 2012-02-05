/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <proto/dos.h>

#include <errno.h>

#include "__stat.h"
#include "__upath.h"
#include "__arosc_privdata.h"

/*****************************************************************************

    NAME */

#include <sys/stat.h>

	int stat(

/*  SYNOPSIS */
	const char *path,
	struct stat *sb)

/*  FUNCTION
	Returns information about a file. Information is stored in stat
	structure having the following fields:
	
	dev_t           st_dev;     - ID of device containing the file
	ino_t           st_ino;     - inode number
	mode_t          st_mode;    - protection mode
	nlink_t         st_nlink;   - number of hard links
	uid_t           st_uid;     - user ID of the file's owner
	gid_t           st_gid;     - group ID of the file's group
	dev_t           st_rdev;    - device ID (if the file is character
	                              or block special file)
	off_t           st_size;    - file size, in bytes
	time_t          st_atime;   - time of last acces
	time_t          st_mtime;   - time of last data modification
	time_t          st_ctime;   - time of last file status change
	blksize_t       st_blksize; - optimal blocksize for I/O
	blkcnt_t        st_blocks;  - number of blocks allocated for file

    INPUTS
	path - Pathname of the file
	sb - Pointer to stat structure that will be filled by the stat() call.

    RESULT
	0 on success and -1 on error. If an error occurred, the global
	variable errno is set.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	lstat(), fstat()

    INTERNALS
	Value of st_ino field is computed as hash from the canonical path of
	the file.

	Values of st_atime, st_mtime and st_ctime fields are always set to
	the last modification date of the file.

	There are no special files in AROS, so the st_rdev field is never
	filled.

	If the given file cannot be examined because of FSA_EXAMINE not
	implemented in the handler, stat structure is filled with some
	default values. It's necessary to allow calling stat() on NIL:.

******************************************************************************/
{
    struct aroscbase *aroscbase = __GM_GetBase();
    int res = 0;
    BPTR lock;

    /* check for empty path before potential conversion from "." to "" */
    if (aroscbase->acb_doupath && path && *path == '\0')
    {
        errno = ENOENT;
        return -1;
    }

    path = __path_u2a(path);
    if (path == NULL)
        return -1;
	
    lock = Lock(path, SHARED_LOCK);
    if (!lock)
    {
	if  (IoErr() == ERROR_OBJECT_IN_USE)
	{
	    /* the file is already locked exclusively, so the only way to get
	       info about it is to find it in the parent directory with the ExNext() function
            */

            SetIoErr(0);
            return __stat_from_path(path, sb);
        }

	errno = __arosc_ioerr2errno(IoErr());
	return -1;
    }
    else
        res = __stat(lock, sb, FALSE);

    UnLock(lock);

    return res;
}

