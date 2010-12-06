/*
    Copyright � 1995-2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <dos/filesystem.h>
#include <proto/dos.h>

#include <errno.h>

#include "__arosc_privdata.h"
#include "__errno.h"
#include "__filesystem_support.h"
#include "__stat.h"
#include "__upath.h"

/* like Dos.Lock but no automatick soft link resolution */
static BPTR __lock(
    const char* name,
    LONG        accessMode);

/*****************************************************************************

    NAME */

#include <sys/stat.h>

        int lstat(

/*  SYNOPSIS */
        const char  *path,
        struct stat *sb)

/*  FUNCTION
        Returns information about a file like stat does except that lstat
        does not follow symbolic links. Information is stored in stat
        structure. Consult stat() documentation for detailed description
        of that structure.

    INPUTS
        path - Pathname of the file
        sb - Pointer to stat structure that will be filled by the lstat() call.

    RESULT
        0 on success and -1 on error. If an error occurred, the global
        variable errno is set.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        stat(), fstat()

    INTERNALS
	Consult stat() documentation for details.

******************************************************************************/
{
    int res = 0;
    BPTR lock;

    /* check for empty path before potential conversion from "." to "" */
    if (__doupath && path && *path == '\0')
    {
        errno = ENOENT;
        return -1;
    }

    path = __path_u2a(path);
    if (path == NULL)
        return -1;

    lock = __lock(path, SHARED_LOCK);
    if (!lock)
    {
        if (   IoErr() == ERROR_IS_SOFT_LINK
            || IoErr() == ERROR_OBJECT_IN_USE)
        {
            /* either the file is already locked exclusively
               or it is a soft link, in both cases only way
               to get info about it is to find it in the
               parent directory with the ExNext() function
            */

            SetIoErr(0);
            return __stat_from_path(path, sb);
        }

        errno = IoErr2errno(IoErr());
        return -1;
    }
    else
        res = __stat(lock, sb);

    UnLock(lock);

    return res;
}

#ifdef AROS_DOS_PACKETS
static BPTR __lock(
    const char* name,
    LONG        accessMode)
{
    return Lock(name, accessMode);
}
#else
static BPTR __lock(
    const char* name,
    LONG        accessMode)
{
    struct DevProc *dvp;
    LONG error;

    if (name == NULL)
        return BNULL;

    if (*name == '\0')
        return Lock(name, accessMode);

    /* Get pointer to process structure */
    struct Process *me = (struct Process *)FindTask(NULL);

    /* Create filehandle */
    struct FileHandle *
    ret = (struct FileHandle *)AllocDosObject(DOS_FILEHANDLE, NULL);

    if (ret != NULL)
    {
        /* Get pointer to I/O request. Use stackspace for now. */
        struct IOFileSys iofs;

    	/* Prepare I/O request. */
    	InitIOFS(&iofs, FSA_OPEN, DOSBase);

    	switch (accessMode)
     	{
        	case EXCLUSIVE_LOCK:
        	    iofs.io_Union.io_OPEN.io_FileMode = FMF_LOCK | FMF_READ;
        	    break;

        	case SHARED_LOCK:
        	    iofs.io_Union.io_OPEN.io_FileMode = FMF_READ;
        	    break;

        	default:
		    D(bug("[Lock] incompatible mode %d\n", accessMode));
	            FreeDosObject(DOS_FILEHANDLE, ret);
		    SetIoErr(ERROR_ACTION_NOT_KNOWN);
		    return BNULL;
     	}
 
        iofs.io_Union.io_OPEN.io_Filename = StripVolume(name);

        dvp = NULL;
    
        do {
            if ((dvp = GetDeviceProc(name, dvp)) == NULL) {
                error = IoErr();
                break;
            }

            error = DoIOFS(&iofs, dvp, NULL, DOSBase);
        } while (error == ERROR_OBJECT_NOT_FOUND);

        if (error == ERROR_NO_MORE_ENTRIES)
            error = me->pr_Result2 = ERROR_OBJECT_NOT_FOUND;

        FreeDeviceProc(dvp);
    
    	if (!error)
    	{
    	    ret->fh_Device = iofs.IOFS.io_Device;
    	    ret->fh_Unit   = iofs.IOFS.io_Unit;
    
    	    return MKBADDR(ret);
    	}
        else
        {
            FreeDosObject(DOS_FILEHANDLE, ret);
        }
    }
    else
    {
        SetIoErr(ERROR_NO_FREE_STORE);
    }

    return BNULL;
}
#endif /* AROS_DOS_PACKETS */
