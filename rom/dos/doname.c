/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Prepare an IO message for use with a specific filename.
    Lang: English
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <string.h>

#include "dos_intern.h"

#ifdef DEBUG
#undef DEBUG
#endif

# define  DEBUG  1
# include <aros/debug.h>


LONG DoName(struct IOFileSys *iofs, CONST_STRPTR name,
	    struct DosLibrary *DOSBase)
{
    CONST_STRPTR pathname, s1 = NULL;
    BPTR cur, lock = (BPTR)NULL;
    struct DosList *dl;
    struct Device *device;
    struct Unit *unit;
    struct FileHandle *fh;
    struct Process *me = (struct Process *)FindTask(NULL);

        STRPTR
    volname = NULL;

    if (!Strnicmp(name, "PROGDIR:", 8) && me->pr_HomeDir != NULL)
    {
        cur = me->pr_HomeDir;
        volname = NULL;
        pathname = name + 8;
    }
    else
    {
/* relative to root dir of current directory's filesystem */
        if (*name == ':')
        {
            cur = me->pr_CurrentDir;
            
                char
            buf[256];

/* get complete path name of current dir and make absolute path from <name> */            
            if (NameFromLock(cur, buf, 255))
            {
                    char*
                colon = strchr(buf, ':');
                
                if (NULL != colon)
                {
                    strcpy(colon + 1, name + 1);

                    name = buf;
                }
            }
        }

    	/* Copy volume name */
    	cur      = me->pr_CurrentDir;
    	s1       = name;
    	pathname = name;
    	volname  = NULL;
    
    	while (*s1 != 0)
    	{
    	    if (*s1++ == ':')
    	    {
        		volname = (STRPTR)AllocMem(s1 - name, MEMF_ANY);
        
        		if (volname == NULL)
        		{
        		    SetIoErr(ERROR_NO_FREE_STORE);
        		    return ERROR_NO_FREE_STORE;
        		}
        
        		CopyMem(name, volname, s1 - name - 1);
        		volname[s1 - name - 1] = '\0';
        
        		pathname = s1;
        		
        		break;
    	    }
    	}
    }

    if (!volname && !cur && !DOSBase->dl_SYSLock)
    {
    	return ERROR_OBJECT_NOT_FOUND;
    }

    dl = LockDosList(LDF_ALL | LDF_READ);

    if (volname != NULL)
    {
    	/* Find logical device */
    	dl = FindDosEntry(dl, volname, LDF_ALL);
    
    	if (dl == NULL)
    	{
    	    UnLockDosList(LDF_ALL|LDF_READ);
    	    FreeMem(volname, s1 - name);
    	    SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
    
    	    return ERROR_DEVICE_NOT_MOUNTED;
    	}
    	else if (dl->dol_Type == DLT_LATE)
    	{
    	    D(bug("Locking late assign %s\n",
                dl->dol_misc.dol_assign.dol_AssignName));
    
    	    lock = Lock(dl->dol_misc.dol_assign.dol_AssignName, SHARED_LOCK);

    	    UnLockDosList(LDF_ALL | LDF_READ);
    
    	    if (lock != NULL)
    	    {
        		AssignLock(volname, lock);

        		dl = LockDosList(LDF_ALL | LDF_READ);
        		dl = FindDosEntry(dl, volname, LDF_ALL);
        
        		if (dl == NULL)
        		{
        		    UnLockDosList(LDF_ALL | LDF_READ);
        		    FreeMem(volname, s1 - name);
        		    SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
        
        		    return ERROR_DEVICE_NOT_MOUNTED;
    	        }
        
        		device = dl->dol_Device;
        		unit = dl->dol_Unit;
    	    }
    	    else
    	    {
        		FreeMem(volname, s1 - name);
        
        		return IoErr();
    	    }
    	}
    	else if (dl->dol_Type == DLT_NONBINDING)
    	{
    	    lock = Lock(dl->dol_misc.dol_assign.dol_AssignName, SHARED_LOCK);

    	    fh = (struct FileHandle *)BADDR(lock);
    
    	    if (fh != NULL)
    	    {
        		device = fh->fh_Device;
        		unit = fh->fh_Unit;
    	    }
    	    else
    	    {
        		UnLockDosList(LDF_ALL | LDF_READ);
        		FreeMem(volname, s1-name);
        
        		return IoErr();
    	    }
    	}
    	else
    	{
    	    device = dl->dol_Device;
    	    unit   = dl->dol_Unit;
    	}
    
    	pathname = s1;
    }
    else if (cur)
    {
    	fh       = (struct FileHandle *)BADDR(cur);
    	device   = fh->fh_Device;
    	unit     = fh->fh_Unit;
    }
    else
    {
        #if 0
        	/* stegerg: ?? */
    	device = DOSBase->dl_NulHandler;
    	unit = DOSBase->dl_NulLock;
        #else
    	fh = (struct FileHandle *)BADDR(DOSBase->dl_SYSLock);
    	device = fh->fh_Device;
    	unit = fh->fh_Unit;
        #endif
    }

    iofs->IOFS.io_Device = device;
    iofs->IOFS.io_Unit = unit;
    iofs->io_Union.io_NamedFile.io_Filename = (STRPTR)pathname;

    /* Send the request. */
    DosDoIO(&iofs->IOFS);

    if (iofs->io_DosError == ERROR_OBJECT_NOT_FOUND
        && dl->dol_Type == DLT_DIRECTORY)
    {
    	struct AssignList *al;
    
    	for (al = dl->dol_misc.dol_assign.dol_List;
    	     al && iofs->io_DosError == ERROR_OBJECT_NOT_FOUND;
    	     al = al->al_Next)
    	{
    	    fh = BADDR(al->al_Lock);

    	    iofs->IOFS.io_Device = fh->fh_Device;
    	    iofs->IOFS.io_Unit = fh->fh_Unit;

    	    DosDoIO(&iofs->IOFS);
    	}
    }

    if (dl != NULL)
    {
    	if (dl->dol_Type == DLT_NONBINDING)
    	{
    	    UnLock(lock);
    	}
    
    	UnLockDosList(LDF_ALL | LDF_READ);
    }

    if (volname != NULL)
    {
        FreeMem(volname, s1-name);
    }

    SetIoErr(iofs->io_DosError);

    return iofs->io_DosError;
} /* DoName */
