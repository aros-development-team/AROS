/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GetDeviceProc() - Find the filesystem for a path.
    Lang: english
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>

#include "dos_intern.h"

extern struct Process *r(struct DeviceNode *dn, struct DosLibrary *DOSBase);
static struct DevProc *deviceproc_internal(struct DosLibrary *DOSBase, CONST_STRPTR name, struct DevProc *dp);

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(struct DevProc *, GetDeviceProc,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name, D1),
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

    SEE ALSO
	FreeDeviceProc()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct DevProc *dp2;
    
    D(bug("[GetDeviceProc] '%s':0x%p\n", name, dp));
    dp2 = deviceproc_internal(DOSBase, name, dp);

#if DEBUG
    bug("[GetDeviceProc] = 0x%p", dp2);
    if (dp2)
        bug(", port=0x%p lock=0x%p dv=0x%p\n", dp2->dvp_Port, dp2->dvp_Lock, dp2->dvp_DevNode);
    RawPutChar('\n');
#endif

    return dp2;

    AROS_LIBFUNC_EXIT

} /* GetDeviceProc */

static struct DevProc *deviceproc_internal(struct DosLibrary *DOSBase, CONST_STRPTR name, struct DevProc *dp)
{
    struct Process *pr = (struct Process *)FindTask(NULL);
    struct DosList *dl = NULL;
    char vol[32];
    LONG len;
    BPTR cur = BNULL, lock = BNULL;
    BOOL stdio = FALSE;
    BOOL res;
    CONST_STRPTR origname = name;
    struct FileLock *fl;

    ASSERT_VALID_PROCESS(pr);

    /* if they passed us the result of a previous call, then they're wanted to
     * loop over the targets of a multidirectory assign */
    if (dp != NULL) {

        /* if what they passed us is not a multidirectory assign, then there's
         * nothing for us to do */
        if (dp->dvp_DevNode != NULL &&
            (dp->dvp_DevNode->dol_Type != DLT_DIRECTORY || !(dp->dvp_Flags & DVPF_ASSIGN))) {

            /* cleanup */
            if (dp->dvp_Flags & DVPF_UNLOCK)
                UnLock(dp->dvp_Lock);

            FreeMem(dp, sizeof(struct DevProc));
            SetIoErr(ERROR_NO_MORE_ENTRIES);
            return NULL;
        }

        /* it's fine, we'll start from here */
        dl = dp->dvp_DevNode;

        /* lock the dos list here, to match the result of the next block */
    	LockDosList(LDF_ALL | LDF_READ);
    }

    /* otherwise we need to find a place to start in the doslist based on the
     * name they passed in */
    else {

        /* handle standard I/O streams as "virtual" devices */
        if (Stricmp(name, "IN:") == 0 || Stricmp(name, "STDIN:") == 0) {
            cur = pr->pr_CIS != BNULL ? pr->pr_CIS : (BPTR) -1;
            stdio = TRUE;
        }
        else if (Stricmp(name, "OUT:") == 0 || Stricmp(name, "STDOUT:") == 0) {
            cur = pr->pr_COS != BNULL ? pr->pr_COS : (BPTR) -1;
            stdio = TRUE;
        }
        else if (Stricmp(name, "ERR:") == 0 || Stricmp(name, "STDERR:") == 0) {
            cur = pr->pr_CES != BNULL ? pr->pr_CES :
                  pr->pr_COS != BNULL ? pr->pr_COS : (BPTR) -1;
            stdio = TRUE;
        }

         /* allocate structure for return */
        if ((dp = AllocMem(sizeof(struct DevProc), MEMF_ANY | MEMF_CLEAR)) == NULL) {
            SetIoErr(ERROR_NO_FREE_STORE);
            return NULL;
        }

        if (stdio) {
            /* handle doesn't exist */
            if (cur == (BPTR) -1) {
                SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
                FreeMem(dp, sizeof(struct DevProc));
                return NULL;
            }

            /* we need a lock for the devproc */
            if ((lock = DupLockFromFH(cur)) == BNULL) {
                FreeMem(dp, sizeof(struct DevProc));
                return NULL;
            }

            /* build the devproc for return */
            dp->dvp_Port = ((struct FileLock *) BADDR(lock))->fl_Task;
            dp->dvp_Lock = lock;
            dp->dvp_Flags = DVPF_UNLOCK; /* remember to unlock in FreeDeviceNode() */

            dl = LockDosList(LDF_ALL | LDF_READ);
            while (dl != NULL && dl->dol_Task != dp->dvp_Port)
                dl = BADDR(dl->dol_Next);
            UnLockDosList(LDF_READ | LDF_ALL);

            /* not found */
            if (dl == NULL) {
                FreeMem(dp, sizeof(struct DevProc));
                SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
                return NULL;
            }

            /* take it */
            dp->dvp_DevNode = dl;

            return dp;
        }

        /* something real, work out what it's relative to */
    	if (Strnicmp(name, "PROGDIR:", 8) == 0) {
    	    lock = pr->pr_HomeDir;
    	    /* I am not sure if these are correct but AOS does return
    	     * non-NULL PROGDIR: handle even if pr_HomeDir is cleared */
    	    if (!lock)
    	    	lock = pr->pr_CurrentDir;
    	    if (lock) {
                fl = BADDR(lock);
                dp->dvp_Port = fl->fl_Task;
                dp->dvp_Lock = lock;
                dp->dvp_DevNode = BADDR(fl->fl_Volume);
            } else {
    	        dp->dvp_Port = DOSBase->dl_Root->rn_BootProc;
    	        dp->dvp_Lock = BNULL;
                dp->dvp_DevNode = NULL;
            } 
            dp->dvp_Flags = 0;
            return dp;
    	}

        /* extract the volume name */
        len = SplitName(name, ':', vol, 0, sizeof(vol) - 1);

        /* if there wasn't one (or we found a lone ':') -> current dir */
        if (len <= 1) {
            lock = pr->pr_CurrentDir;
            /* if we got NULL, then it's relative to the system root lock */
            if (lock != BNULL) {
                fl = BADDR(lock);
                dp->dvp_Port = fl->fl_Task;
                dp->dvp_Lock = lock;
                dp->dvp_DevNode = BADDR(fl->fl_Volume);
            } else {
    	        dp->dvp_Port = DOSBase->dl_Root->rn_BootProc;
    	        dp->dvp_Lock = BNULL;
                dp->dvp_DevNode = NULL;
            }

            /* Jump to the root directory if name[0] == ':' */
            if (name[0] == ':')
                dp->dvp_Lock = (dp->dvp_DevNode) ? dp->dvp_DevNode->dol_Lock : BNULL;

            dp->dvp_Flags = 0;
            return dp;
        }
 
        do {
            /* now find the doslist entry for the named volume */
            dl = LockDosList(LDF_ALL | LDF_READ);
            dl = FindDosEntry(dl, vol, LDF_ALL);

            /* not found, bail out */
            if (dl == NULL) {
                UnLockDosList(LDF_ALL | LDF_READ);

                if (ErrorReport(ERROR_DEVICE_NOT_MOUNTED, REPORT_INSERT, (IPTR)vol, NULL) == DOSTRUE) {
                    FreeMem(dp, sizeof(struct DevProc));
                    return NULL;
                }
            }
        } while(dl == NULL);
    }

    /* at this point, we have an allocated devproc in dp, the doslist is
     * locked for read, and we have the entry for the named "volume"
     * (device, assign, etc) in dl and a filename relative to that in name */

    /* late assign. we resolve the target and then promote the doslist entry
     * to full assign */
    if (dl->dol_Type == DLT_LATE) {
        /* obtain a lock on the target */
        lock = Lock(dl->dol_misc.dol_assign.dol_AssignName, SHARED_LOCK);

        /* unlock the list, either we have a lock and need to assign it, or we
         * don't and need to bail out */
        UnLockDosList(LDF_ALL | LDF_READ);

        /* XXX there's a race here. with the doslist unlocked, it's possible
         * that some other process will add or remove this assign, blowing
         * everything up. need more tuits before attempting a fix */

        /* didn't find the target */
        if (lock == BNULL) {
            FreeMem(dp, sizeof(struct DevProc));
            return NULL;
        }

        /* try to assign it */
        if (AssignLock(vol, lock) == DOSFALSE) {
            UnLock(lock);
            FreeMem(dp, sizeof(struct DevProc));
            return NULL;
        }

        /* we made the assign! now we have to go back over the list and find
         * the new entry */
        dl = LockDosList(LDF_ALL | LDF_READ);
        dl = FindDosEntry(dl, vol, LDF_ALL);

        /* not found. XXX this will only happen if we hit that race above */
        if (dl == NULL) {
            UnLockDosList(LDF_ALL | LDF_READ);
            FreeMem(dp, sizeof(struct DevProc));
            SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
            return NULL;
        }

        /* the added entry will be a DLT_DIRECTORY, so we can just copy the
         * details in and get out of here */
        dp->dvp_Port = dl->dol_Task;
        dp->dvp_Lock = dl->dol_Lock;
        dp->dvp_Flags = 0;
        dp->dvp_DevNode = dl;

        UnLockDosList(LDF_ALL | LDF_READ);

        return dp;
    }

    /* nonbinding assign. like a late assign, but with no doslist promotion */
    if (dl->dol_Type == DLT_NONBINDING) {
        lock = Lock(dl->dol_misc.dol_assign.dol_AssignName, SHARED_LOCK);

        /* just fill out the dp and return */
        dp->dvp_Port = ((struct FileLock *) BADDR(lock))->fl_Task;
        dp->dvp_Lock = lock;
        dp->dvp_Flags = DVPF_UNLOCK;   /* remember to unlock in FreeDeviceNode() */
        dp->dvp_DevNode = dl;

        UnLockDosList(LDF_ALL | LDF_READ);

        return dp;
    }

    /* devices and volumes are easy */
    if (dl->dol_Type == DLT_DEVICE || dl->dol_Type == DLT_VOLUME)
    {
    	struct MsgPort *newhandler = NULL;

	if (dl->dol_Type == DLT_DEVICE)
	{
	    /* Check if the handler is not started */
	    newhandler = dl->dol_Task;
	    if (!newhandler)
	    {
	    	D(bug("[GetDeviceProc] Accessing device '%b', path='%s'\n", dl->dol_Name, origname));

	    	/*
	    	 * Unlock before starting handler, handler may internally
	    	 * require dos list locks, for example to add volume node.
	     	 */
	    	UnLockDosList(LDF_ALL | LDF_READ);

	    	newhandler = RunHandler((struct DeviceNode *)dl, origname, DOSBase);
	    	if (!newhandler)
	    	{
		    FreeMem(dp, sizeof(struct DevProc));
	            SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
        	    return NULL;
        	}

	    	LockDosList(LDF_ALL | LDF_READ);
	    }
	}
	else
	{
	    res = TRUE;

	    while (res && !dl->dol_Task)
	    {
	    	D(bug("[GetDeviceProc] Accessing offline volume '%b'\n", dl->dol_Name));
		res = !ErrorReport(ERROR_DEVICE_NOT_MOUNTED, REPORT_VOLUME, (IPTR)dl, NULL);
	    }

	    if (!res)
	    {
	        UnLockDosList(LDF_ALL | LDF_READ);
            	FreeMem(dp, sizeof(struct DevProc));
            	SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
            	return NULL;
            }
	}

	/*
	 * A handler theoretically may choose to use custom MsgPort for communications.
	 * Pick up its preference if specified.
	 */
        dp->dvp_Port = dl->dol_Task ? dl->dol_Task : newhandler;
        dp->dvp_Lock = BNULL;
        dp->dvp_Flags = 0;
        dp->dvp_DevNode = dl;

        UnLockDosList(LDF_ALL | LDF_READ);

        return dp;
    }

    /* sanity check */
    if (dl->dol_Type != DLT_DIRECTORY) {
        UnLockDosList(LDF_ALL | LDF_READ);
        FreeMem(dp, sizeof(struct DevProc));
        kprintf("%s:%d: DosList entry 0x%p has unknown type %d. Probably a bug, report it!\n"
                "    GetDeviceProc() called for '%s'\n",
                __FILE__, __LINE__, dl, dl->dol_Type, name);
        SetIoErr(ERROR_BAD_NUMBER);
        return NULL;
    }

    /* real assigns. first, see if it's just pointing to a single dir */
    if (dp->dvp_Flags != DVPF_ASSIGN) {
        /* just a plain assign, easy */
        dp->dvp_Port = dl->dol_Task;
        dp->dvp_Lock = dl->dol_Lock;
        dp->dvp_DevNode = dl;
        
        /* note multidirectory assigns so the caller knows to loop */
        dp->dvp_Flags = dl->dol_misc.dol_assign.dol_List != NULL ? DVPF_ASSIGN : 0;

        UnLockDosList(LDF_ALL | LDF_READ);
        return dp;
    }

    /* finally the tricky bit - multidirectory assigns */

    /* if we're pointing at the "primary" lock, then we just take the first
     * one in the list */
    if (dp->dvp_Lock == dl->dol_Lock)
        dp->dvp_Lock = dl->dol_misc.dol_assign.dol_List->al_Lock;

    /* otherwise we're finding the next */
    else {
        struct AssignList *al = dl->dol_misc.dol_assign.dol_List;

        /* find our current lock (ie the one we returned last time) */
        for (; al != NULL && al->al_Lock != dp->dvp_Lock; al = al->al_Next);

        /* if we didn't find it, or we didn't but there's none after it, then
         * we've run out */
        if (al == NULL || (al = al->al_Next) == NULL) {
            UnLockDosList(LDF_ALL | LDF_READ);
            FreeMem(dp, sizeof(struct DevProc));

            SetIoErr(ERROR_NO_MORE_ENTRIES);
            return NULL;
        }

        /* fill out the lock from the new entry */
        dp->dvp_Lock = al->al_Lock;
    }

    /* final pieces */
    dp->dvp_Port = ((struct FileLock *) BADDR(dp->dvp_Lock))->fl_Task;
    dp->dvp_Flags = DVPF_ASSIGN;
    dp->dvp_DevNode = dl;

    UnLockDosList(LDF_READ|LDF_ALL);
    /* phew */
    SetIoErr(0);
    return dp;
}
