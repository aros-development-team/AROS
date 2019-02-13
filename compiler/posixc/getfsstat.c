/*
    Copyright © 2004-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <string.h>
#include <errno.h>
#include <sys/mount.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include "__upath.h"

short getnixfilesystemtype(LONG id_DiskType)
{
    switch(id_DiskType)
    {
	case ID_DOS_DISK:
	case ID_FASTDIR_DOS_DISK:
	    return MOUNT_ADOS_OFS;
	case ID_INTER_DOS_DISK:
	    return MOUNT_ADOS_IOFS;
	case ID_FFS_DISK:
	case ID_FASTDIR_FFS_DISK:
	    return MOUNT_ADOS_FFS;
	case ID_INTER_FFS_DISK:
	    return MOUNT_ADOS_IFFS;
	default:
	    return MOUNT_NONE;
    }
}

/*****************************************************************************

    NAME */
#include <sys/mount.h>

    int getfsstat(

/*  SYNOPSIS */
    struct statfs *buf, 
    long bufsize, 
    int flags)

/*  FUNCTION
        Gets information about mounted filesystems.

    INPUTS
        buf - pointer to statfs structures where information about filesystems
            will be stored or NULL
        bufsize - size of buf in bytes
        flags - not used

    RESULT
        If buf is NULL number of mounted filesystems is returned. If buf is
        not null, information about mounted filesystems is stored in statfs
        structures up to bufsize bytes

    NOTES

    EXAMPLE

    BUGS
        f_flags, f_files, f_ffree and f_fsid.val are always set to 0
        f_mntfromname is set to an empty string

    SEE ALSO
    	
    INTERNALS

******************************************************************************/
{
    STRPTR name;
    BPTR lock;
    struct DosList *dlist;
    struct InfoData data;
    int fscount = 0; /* number of filesystems */
    LONG ioerr = 0;

    dlist = LockDosList(LDF_READ | LDF_VOLUMES);
    while ((dlist = NextDosEntry(dlist, LDF_VOLUMES)) != NULL)
    {
	if(IsFileSystem(AROS_BSTR_ADDR(dlist->dol_Name)) == FALSE)
		continue;
	fscount++;

	/* If buf is NULL just count filesystems */
	if(buf == NULL)
	    continue;

	/* See if another structure can be stored */
	bufsize -= sizeof(struct statfs);
	if(bufsize < 0) 
	{
	    fscount--;
	    break;
	}
	
	/* Create a volume name */
	if(!(name = (STRPTR) AllocVec(AROS_BSTR_strlen(dlist->dol_Name) + 2,
	    MEMF_CLEAR | MEMF_ANY)))
	{
	    ioerr = ERROR_NO_FREE_STORE;
	    break;
	}
	
	strcpy(name, AROS_BSTR_ADDR(dlist->dol_Name));
	strcat(name, ":");

	/* Get filesystem data from lock */
	if((lock = Lock(name, SHARED_LOCK)))
	{
	    if(Info(lock, &data))
	    {
		/* Fill statfs structure */
		buf[fscount - 1].f_type = getnixfilesystemtype(
		    data.id_DiskType);
	        buf[fscount - 1].f_flags = 0;
	        buf[fscount - 1].f_fsize = data.id_BytesPerBlock;
	        buf[fscount - 1].f_bsize = data.id_BytesPerBlock;
	        buf[fscount - 1].f_blocks = data.id_NumBlocks;
	        buf[fscount - 1].f_bfree = data.id_NumBlocks - 
		    data.id_NumBlocksUsed;
	        buf[fscount - 1].f_bavail = data.id_NumBlocks - 
		    data.id_NumBlocksUsed;
	        buf[fscount - 1].f_files = 0;
	        buf[fscount - 1].f_ffree = 0;
	        buf[fscount - 1].f_fsid.val[0] = 0;
	        buf[fscount - 1].f_fsid.val[1] = 0;
			CopyMem(__path_a2u(name), buf[fscount - 1].f_mntonname, 
	            MNAMELEN);
	        buf[fscount - 1].f_mntfromname[MNAMELEN - 1] = '\0';
	        buf[fscount - 1].f_mntfromname[0] = '\0';
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
	FreeVec(name);
	if(ioerr)
	    break;
    }
    UnLockDosList(LDF_READ | LDF_VOLUMES);

    if(ioerr) {
	errno = __stdc_ioerr2errno(ioerr);
	return -1;
    }
    return fscount;
}
