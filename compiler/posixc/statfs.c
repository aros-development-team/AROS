/*
    Copyright © 2008-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>
#include <errno.h>
#include <sys/mount.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include "__upath.h"

short getnixfilesystemtype(LONG id_DiskType);

/*****************************************************************************

    NAME */
#include <sys/mount.h>

    int statfs(

/*  SYNOPSIS */
    const char *path,
    struct statfs *buf)

/*  FUNCTION
        Gets information about mounted filesystem.

    INPUTS
        path - path to any file in the filesystem we want to know about
        buf - pointer to statfs structures where information about filesystem
            will be stored

    RESULT
        Information about filesystem is stored in statfs structure

    NOTES

    EXAMPLE

    BUGS
        f_flags, f_files, f_ffree and f_fsid.val are always set to 0
        f_mntfromname is set to an empty string

    SEE ALSO
    	
    INTERNALS

******************************************************************************/
{
    BPTR lock;
    LONG ioerr = 0;
    struct InfoData data;
    const char *apath;
    
    if (path == NULL)
    {
	errno = EINVAL;
        return -1;
    }

    apath = __path_u2a(path);
    if (!apath) 
    {
	errno = EINVAL;
	return -1;
    }
    
    /* Get filesystem data from lock */
    if(((lock = Lock(apath, SHARED_LOCK))))
    {
	if(Info(lock, &data))
	{
	    /* Fill statfs structure */
	    buf->f_type = getnixfilesystemtype(data.id_DiskType);
	    buf->f_flags = 0;
	    buf->f_fsize = data.id_BytesPerBlock;
	    buf->f_bsize = data.id_BytesPerBlock;
	    buf->f_blocks = data.id_NumBlocks;
	    buf->f_bfree = data.id_NumBlocks - data.id_NumBlocksUsed;
	    buf->f_bavail = data.id_NumBlocks - data.id_NumBlocksUsed;
	    buf->f_files = 0;
	    buf->f_ffree = 0;
	    buf->f_fsid.val[0] = 0;
	    buf->f_fsid.val[1] = 0;
        CopyMem(__path_a2u(AROS_BSTR_ADDR(((struct DeviceList *)BADDR(data.id_VolumeNode))->dl_Name)), buf->f_mntonname, MNAMELEN);
        buf->f_mntonname[MNAMELEN -1] = '\0';
	    buf->f_mntfromname[0] = '\0';
	}
	else
	{
	    ioerr = IoErr();
	}
        UnLock(lock);
    }
    else
    {
	ioerr = IoErr();
    }
	
    if(ioerr != 0) {
	errno = __stdc_ioerr2errno(ioerr);
	return -1;
    }

    return 0;
}
