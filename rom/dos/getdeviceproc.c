/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GetDeviceProc() - Find the filesystem for a path.
    Lang: english
*/
#include "dos_intern.h"
#include <proto/exec.h>
#include <proto/utility.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(struct DevProc *, GetDeviceProc,

/*  SYNOPSIS */
	AROS_LHA(STRPTR          , name, D1),
	AROS_LHA(struct DevProc *, dp, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 107, Dos)

/*  FUNCTION
	GetDeviceProc() will search for the filesystem handler which
	you should send a command to for a specific path.

	By calling GetDeviceProc() multiple times, the caller will
	be able to handle multi-assign paths.

	The first call to GetDeviceProc() should have the |dp| parameter
	as NULL.

    INPUTS
	name		- Name of the object to find.
	dp		- Previous result of GetDeviceProc() or NULL.

    RESULT
	A pointer to a DevProc structure containing the information
	required to send a command to a filesystem.

    NOTES

    EXAMPLE

    BUGS
	Currently doesn't return dvp_DevNode for locks which are
	relative to "PROGDIR:", ":", or the current directory.

	I'm working on it though...

    SEE ALSO
	FreeDeviceProc()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Process *pr = (struct Process *)FindTask(NULL);
    struct DosList *dl = NULL;
    struct FileHandle *fh;
    STRPTR volname = NULL, s1 = NULL;
    BPTR cur = NULL, lock = (BPTR)0;

    /*
	Note: We can free the DevProc structure anywhere an error occurs
	because for all cases except DLT_DIRECTORY types, we will have
	not returned a DevProc before, therefore the caller will have
	nothing to call FreeDeviceProc() on.

	There are no errors of this type for DLT_DIRECTORY, so we can
	just handle this as needed. However if there was an error, we
	would simply check the dp->dvp_Flags & DVPF_ASSIGN to see if
	we had returned the structure to the caller.
    */

    /* Have we already got a DevProc? */
    if( dp != NULL )
    {
	/*
	    Yes, if this is not a multi-assign directory, then there
	    is no reason for us to bother again. Just return NULL in
	    that case.
	*/
	if( dp->dvp_DevNode )
	    if(    dp->dvp_DevNode->dol_Type != DLT_DIRECTORY
		|| (dp->dvp_Flags & DVPF_ASSIGN) == 0 )
	    {
		return NULL;
	    }
	    
	/* We'll start from there. */
	
        /* Matches UnlockDosList at end of func */
    	LockDosList(LDF_ALL|LDF_READ);
	
	dl = dp->dvp_DevNode;
    }
    else
    {
	/*
	    We have to find the starting dl structure
	    What is the volume part of the filesystem?
	*/
	cur = pr->pr_CurrentDir;

    	if(!Strnicmp(name, "PROGDIR:", 8))
    	{
    	    /* The path refers to PROGDIR: */
    	    cur = pr->pr_HomeDir;
    	}
    	else if( *name == ':' )
    	{
    	    /*
		Relative to the current volume (ie current directory)
		We simply need to remove this case from the below.
	    */
    	}
    	else
    	{
    	    /* Copy the volume name */
    	    s1 = name;
    	    volname = NULL;
    
    	    while(*s1)
    	    {
    		if(*s1++ == ':')
    		{
    		    volname = (STRPTR)AllocMem(s1-name, MEMF_ANY);
    		    if( volname == NULL )
    		    {
    			SetIoErr(ERROR_NO_FREE_STORE);
    			return NULL;
    		    }
    		    CopyMem(name, volname, s1 - name -1 );
    		    volname[s1-name-1] = '\0';
    		    break;
    		}
    	    }
    	}

/* kprintf("Volume name: %p\n", volname);
if (volname)
	kprintf("volname: %s\n", volname);
*/    
	/* Allocate a DevProc */
	dp = AllocMem(sizeof(struct DevProc), MEMF_ANY|MEMF_CLEAR);
	if( dp == NULL )
	{
	    FreeMem(volname, s1-name);
	    SetIoErr(ERROR_NO_FREE_STORE);
	    return NULL;
	}

    	/* We now go through the DOS device list */
    	dl = LockDosList(LDF_ALL|LDF_READ);
    	if( volname != NULL )
    	{
    	    /* Find logical Device */
    	    dl = FindDosEntry(dl, volname, LDF_ALL);
    	    if( dl == NULL )
    	    {
/* kprintf("Found logical device\n");
*/    		UnLockDosList(LDF_ALL|LDF_READ);
    		FreeMem(volname, s1-name);
		FreeMem(dp, sizeof(struct DevProc));
    		SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
    		return NULL;
    	    }
	}
	else
	{
	    /* We have the lock in cur */
/* kprintf("lock in cur\n");
*/	    fh = BADDR(cur);
	    dp->dvp_Port = (struct MsgPort *)fh->fh_Device;
	    dp->dvp_Lock = cur;
	    dp->dvp_Flags = 0;
	    dp->dvp_DevNode = NULL;
	    UnLockDosList(LDF_ALL|LDF_READ);
	    SetIoErr(0);
	    return dp;
	}
    } /* we didn't have a DevProc passed into us */

    /* We now look at the type of the DosList entry */
    if( dl->dol_Type == DLT_LATE )
    {
/* kprintf("Late assign\n");
*/	UnLockDosList(LDF_ALL|LDF_READ);
	
	lock = Lock(dl->dol_misc.dol_assign.dol_AssignName, SHARED_LOCK);
	if( lock )
	{
	    AssignLock(volname, lock);
	    dl = LockDosList(LDF_ALL|LDF_READ);
	    dl = FindDosEntry(dl, volname, LDF_ALL);
	    if( dl == NULL )
	    {
		UnLockDosList(LDF_ALL|LDF_READ);
		FreeMem(volname, s1-name);
		FreeMem(dp, sizeof(struct DevProc));
		SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
		return NULL;
	    }

	    dp->dvp_Port = (struct MsgPort *)dl->dol_Ext.dol_AROS.dol_Device;
	    dp->dvp_Lock = lock;
	    dp->dvp_Flags = 0;
	    dp->dvp_DevNode = dl;
	}
    	else
    	{
    	    FreeMem(volname, s1-name);
	    FreeMem(dp, sizeof(struct DevProc));
    	    return NULL;
    	}
    } /* late binding assign */
    else if(dl->dol_Type == DLT_NONBINDING)
    {
/* kprintf("nonbinding assign\n");    
*/	lock = Lock(dl->dol_misc.dol_assign.dol_AssignName, SHARED_LOCK);
	fh = (struct FileHandle *)BADDR(lock);
	if( fh != NULL )
	{
	    dp->dvp_Port = (struct MsgPort *)fh->fh_Device;
	    dp->dvp_Lock = lock;
	    dp->dvp_Flags = DVPF_UNLOCK;
	    dp->dvp_DevNode = dl;
	}
	else
	{
	    UnLockDosList(LDF_ALL|LDF_READ);
	    FreeMem(dp, sizeof(struct DevProc));
	    FreeMem(volname, s1-name);
	    return NULL;
	}
    }
    else
    {
/* kprintf("generic case\n");    
*/	/* Generic case, a volume, a device, or a multi-assign */
	dp->dvp_Port = (struct MsgPort *)dl->dol_Ext.dol_AROS.dol_Device;
	dp->dvp_DevNode = dl;
	if( dl->dol_Type == DLT_DIRECTORY )
	{
	    /* If called before, this will be set */
	    if( dp->dvp_Flags == DVPF_ASSIGN )
	    {
/*		kprintf("DBPF_ASSIGN flag found\n");
*/		/* First iteration of list ? */
		if (dp->dvp_Lock == dl->dol_Lock)
		{
		    /* If so, set to first item in assignlist.
		       (The set DVPF_ASSIGN flag tells that a assignlist
		       does exist
		    */
/*		    kprintf("First time multiple assign\n");
*/		    dp->dvp_Lock = dl->dol_misc.dol_assign.dol_List->al_Lock;
		}
		else
		{

		    struct AssignList *al = dl->dol_misc.dol_assign.dol_List;
		
/*		    UBYTE buf[100];
		    
		    kprintf("Do assign list\n");
*/		    /*  
		    	Go through until we have found the last one returned.
		    */
		    while( al && (al->al_Lock != dp->dvp_Lock) )
		    {
/*		    	NameFromLock(al->al_Lock, buf, 100);
		    	kprintf("gdp: trying assigndir %s\n", buf);
*/		    	al = al->al_Next;
		    }

		    if( al != NULL )
		    {
/*		        kprintf("al != NULL\n");
*/		    	if( al->al_Next != NULL )
		    	{
    			    dp->dvp_Lock = al->al_Next->al_Lock;
			
/*		    	    NameFromLock(dp->dvp_Lock, buf, 100);
		    	    kprintf("gdp: returning assigndir %s\n", buf); 
*/		    	}
			else
			{
			    /* We have reached the end of the list - just return NULL */
			    UnLockDosList(LDF_ALL|LDF_READ);
			    SetIoErr(ERROR_NO_MORE_ENTRIES);
		    
			    if (volname)
				FreeMem(volname, s1-name);

		    	    FreeMem(dp, sizeof(struct DevProc));
				
			    return NULL;

			}
			
		    }
		    else
		    {
		    	/* We have reached the end of the list - just return NULL */
	    	    	UnLockDosList(LDF_ALL|LDF_READ);
		    	SetIoErr(ERROR_NO_MORE_ENTRIES);
		    
		    	if (volname)
		    	    FreeMem(volname, s1-name);


		    	FreeMem(dp, sizeof(struct DevProc));
			
		    	return NULL;
		    }
		    
		} /* if (first iteration of assignlist) */
		
	    } /* if (DVPF_ASSIGN flag has been set */
	    else
    		dp->dvp_Lock = dl->dol_Lock;

	    /* Only set this again if we have some locks to look at */
	    if( dl->dol_misc.dol_assign.dol_List != NULL )
    		dp->dvp_Flags = DVPF_ASSIGN;

	} /* DLT_DIRECTORY */
	else
	{
	    dp->dvp_Lock = NULL;
	    dp->dvp_Flags = 0;
	}
    }

    UnLockDosList(LDF_READ|LDF_ALL);
    if( volname != NULL )
    	FreeMem(volname,s1-name);
    SetIoErr(0);
    return dp;

    AROS_LIBFUNC_EXIT
} /* GetDeviceProc */
