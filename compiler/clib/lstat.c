/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/filesystem.h>
#include <proto/dos.h>

#include <errno.h>

#include "__arosc_privdata.h"
#include "__errno.h"
#include "__stat.h"
#include "__upath.h"

#include <aros/debug.h>

/* like Dos.Lock but no automatic soft link resolution */
static BPTR __lock(
    const char* name,
    LONG        accessMode);

/*****************************************************************************

    NAME */

#include <sys/stat.h>

        int lstat(

/*  SYNOPSIS */
        const char *path,
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

static void InitIOFS(struct IOFileSys *iofs, ULONG type,
	      struct DosLibrary *DOSBase)
{
    struct Process *me = (struct Process *)FindTask(NULL);

    iofs->IOFS.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    iofs->IOFS.io_Message.mn_ReplyPort    = &me->pr_MsgPort;
    iofs->IOFS.io_Message.mn_Length       = sizeof(struct IOFileSys);
    iofs->IOFS.io_Command                 = type;
    iofs->IOFS.io_Flags                   = 0;
}

static CONST_STRPTR StripVolume(CONST_STRPTR name) {
    const char *path = strchr(name, ':');
    if (path != NULL)
        path++;
    else
        path = name;
    return path;
}

static LONG DoIOFS(struct IOFileSys *iofs, struct DevProc *dvp, CONST_STRPTR name,
    struct DosLibrary *DOSBase) {
    BOOL freedvp = FALSE;

    if (dvp == NULL) {
        if ((dvp = GetDeviceProc(name, NULL)) == NULL)
            return IoErr();

        freedvp = TRUE;
    }

    iofs->IOFS.io_Device = (struct Device *) dvp->dvp_Port;

    if (dvp->dvp_Lock != NULL)
        iofs->IOFS.io_Unit = ((struct FileHandle *) BADDR(dvp->dvp_Lock))->fh_Unit;
    else
        iofs->IOFS.io_Unit = dvp->dvp_DevNode->dol_Ext.dol_AROS.dol_Unit;

    if (name != NULL)
        iofs->io_Union.io_NamedFile.io_Filename = StripVolume(name);

    DoIO((struct IORequest *)iofs);

    if (freedvp)
        FreeDeviceProc(dvp);

    SetIoErr(iofs->io_DosError);

    return iofs->io_DosError;
}

static BPTR __lock(
    const char* name,
    LONG        accessMode)
{
    struct DevProc *dvp;
    LONG error;

    if (name == NULL)
        return NULL;

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
		    return NULL;
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

    return NULL;
}
