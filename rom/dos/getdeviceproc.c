/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GetDeviceProc() - Find the filesystem for a path.
    Lang: english
*/
#include "dos_intern.h"
#include <proto/exec.h>
#include <proto/utility.h>

#define DEBUG 0
#include <aros/debug.h>

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
    char vol[32];
    LONG len;
    char buf[256];
    BPTR cur = NULL, lock = NULL;
    BOOL stdio = FALSE;

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
            return NULL;
        }

        /* its fine, we'll start from here */
        dl = dp->dvp_DevNode;

        /* lock the dos list here, to match the result of the next block */
    	LockDosList(LDF_ALL | LDF_READ);
    }

    /* otherwise we need find a place to start in the doslist based on the
     * name they passed in */
    else {

        /* handle standard I/O streams as "virtual" devices */
        if (Stricmp(name, "IN:") == 0 || Stricmp(name, "STDIN:") == 0) {
            cur = pr->pr_CIS != NULL ? pr->pr_CIS : (BPTR) -1;
            stdio = TRUE;
        }
        else if (Stricmp(name, "OUT:") == 0 || Stricmp(name, "STDOUT:") == 0) {
            cur = pr->pr_COS != NULL ? pr->pr_COS : (BPTR) -1;
            stdio = TRUE;
        }
        else if (Stricmp(name, "ERR:") == 0 || Stricmp(name, "STDERR:") == 0) {
            cur = pr->pr_CES != NULL ? pr->pr_CES :
                  pr->pr_COS != NULL ? pr->pr_COS : (BPTR) -1;
            stdio = TRUE;
        }

        if (stdio) {
            /* handle doesn't exist */
            if (cur == (BPTR) -1) {
                SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
                return NULL;
            }

            /* got it, make a fake devproc */
            if ((dp = AllocMem(sizeof(struct DevProc), MEMF_ANY | MEMF_CLEAR)) == NULL) {
                SetIoErr(ERROR_NO_FREE_STORE);
                return NULL;
            }

            /* we need a lock for the devproc */
            if ((lock = DupLockFromFH(cur)) == NULL) {
                FreeMem(dp, sizeof(struct DevProc));
                return NULL;
            }

            /* build the devproc for return */
            dp->dvp_Port = (struct MsgPort *) ((struct FileHandle *) BADDR(lock))->fh_Device;
            dp->dvp_Lock = lock;
            dp->dvp_Flags = DVPF_UNLOCK; /* remember to unlock in FreeDeviceNode() */

            /* finding the device node
             * XXX this is naive - if the device appears on the doslist twice
             * we have no way to tell which one is ours.  we can't even use
             * NameFromLock() to get the volume name and then find the doslist
             * entry from that as console handlers probably don't even
             * implement names. bring on packets I say */

            dl = LockDosList(LDF_ALL | LDF_READ);
            while (dl != NULL && (dl->dol_Ext.dol_AROS.dol_Device != dp->dvp_Port))
                dl = dl->dol_Next;

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

        /* something real, work out what its relative to */
    	if (Strnicmp(name, "PROGDIR:", 8) == 0) {
    	    cur = pr->pr_HomeDir;

            /* move past the "volume" name, so that we end up in the "no
             * volume name" case below */;
            name = &name[8];
        }

        else
            cur = pr->pr_CurrentDir;

        /* if we got NULL, then its relative to the system root lock */
        if (cur == NULL)
            cur = DOSBase->dl_SYSLock;

        /* extract the volume name */
        len = SplitName(name, ':', vol, 0, sizeof(vol)-1);

        /* move the name past it, its now relative to the volume */
        name += len;

        /* if there wasn't one (or we found a lone ':'), then we need to
         * extract it from the name of the current dir */

        /* XXX this block sucks. NameFromLock () calls DupLock(), which calls
         * Lock(), which would end up back here if it wasn't for the
         * special-case in Lock(). once we have real FileLock locks, then this
         * code will go and we can just look at cur->fl_Volume to get the
         * doslist entry. see the morphos version for details */
        if (len <= 1) {

            /* if we didn't find a ':' at all, then we'll need to return the
             * lock that its relative to, so make a note */
            if (len == -1)
                lock = cur;

            if (NameFromLock(cur, buf, 255) != DOSTRUE)
                return NULL;

            len = SplitName(buf, ':', vol, 0, sizeof(vol)-1);

            /* if there isn't one, something is horribly wrong */
            if (len <= 1) {
                kprintf("%s:%d: NameFromLock() returned a path without a volume. Probably a bug, report it!\n"
                        "    GetDeviceProc() called for '%s'\n"
                        "    NameFromLock() called on 0x%08x, returned '%s'\n",
                        __FILE__, __LINE__, name, cur, buf);
                SetIoErr(ERROR_INVALID_COMPONENT_NAME);
                return NULL;
            }
        }

        /* allocate structure for return */
        if ((dp = AllocMem(sizeof(struct DevProc), MEMF_ANY | MEMF_CLEAR)) == NULL) {
            SetIoErr(ERROR_NO_FREE_STORE);
            return NULL;
        }

        do {
            /* now find the doslist entry for the named volume */
            dl = LockDosList(LDF_ALL | LDF_READ);
            dl = FindDosEntry(dl, vol, LDF_ALL);

            /* not found, bail out */
            if (dl == NULL) {
                UnLockDosList(LDF_ALL | LDF_READ);

                if (ErrorReport(ERROR_DEVICE_NOT_MOUNTED, REPORT_INSERT, (ULONG) vol, NULL) == DOSTRUE) {
                    FreeMem(dp, sizeof(struct DevProc));
                    SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
                    return NULL;
                }
            }
        } while(dl == NULL);

        /* relative to the current dir, then we have enough to get out of here */
        if (lock != NULL) {
            dp->dvp_Port = (struct MsgPort *) ((struct FileHandle *) BADDR(cur))->fh_Device;
            dp->dvp_Lock = lock;
            dp->dvp_Flags = 0;
            dp->dvp_DevNode = dl;

            UnLockDosList(LDF_ALL | LDF_READ);

            return dp;
        }
    }

    /* at this point, we have an allocated devproc in dp, the doslist is
     * locked for read, and we have the the entry for the named "volume"
     * (device, assign, etc) in dl and a filename relative to that in name */

    /* late assign. we resolve the target and then promote the doslist entry
     * to full assign */
    if (dl->dol_Type == DLT_LATE) {
        /* obtain a lock on the target */
        lock = Lock(dl->dol_misc.dol_assign.dol_AssignName, SHARED_LOCK);

        /* unlock the list, either we have a lock and need to assign it, or we
         * don't and need to bail out */
        UnLockDosList(LDF_ALL | LDF_READ);

        /* XXX there's a race here. with the doslist unlocked, its possible
         * that some other process will add or remove this assign, blowing
         * everything up. need more tuits before attempting a fix */

        /* didn't find the target */
        if (lock == NULL) {
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
        dp->dvp_Port = (struct MsgPort *) dl->dol_Ext.dol_AROS.dol_Device;
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
        dp->dvp_Port = (struct MsgPort *) ((struct FileHandle *) BADDR(lock))->fh_Device;
        dp->dvp_Lock = lock;
        dp->dvp_Flags = DVPF_UNLOCK;   /* remember to unlock in FreeDeviceNode() */
        dp->dvp_DevNode = dl;

        UnLockDosList(LDF_ALL | LDF_READ);

        return dp;
    }

    /* devices and volumes are easy */
    if (dl->dol_Type == DLT_DEVICE || dl->dol_Type == DLT_VOLUME) {
	if (dl->dol_Type == DLT_DEVICE) {
	    if (!dl->dol_Ext.dol_AROS.dol_Device) {
		D(bug("Accessing offline device %s\n", dl->dol_Ext.dol_AROS.dol_DevName));
		RunHandler((struct DeviceNode *)dl, DOSBase);
	    }
	} else {
	    while (!dl->dol_Ext.dol_AROS.dol_Device) {
	    	D(bug("Accessing offline volume %s\n", dl->dol_Ext.dol_AROS.dol_DevName));
		ErrorReport(ERROR_DEVICE_NOT_MOUNTED, REPORT_VOLUME, (ULONG)dl, NULL);
	    }
	}
        dp->dvp_Port = (struct MsgPort *) dl->dol_Ext.dol_AROS.dol_Device;
        dp->dvp_Lock = NULL;
        dp->dvp_Flags = 0;
        dp->dvp_DevNode = dl;

        UnLockDosList(LDF_ALL | LDF_READ);

        return dp;
    }

    /* sanity check */
    if (dl->dol_Type != DLT_DIRECTORY) {
        UnLockDosList(LDF_ALL | LDF_READ);
        FreeMem(dp, sizeof(struct DevProc));
        kprintf("%s:%d: DosList entry 0x%08x has unknown type %d. Probably a bug, report it!\n"
                "    GetDeviceProc() called for '%s'\n",
                __FILE__, __LINE__, dl, dl->dol_Type, name);
        SetIoErr(ERROR_BAD_NUMBER);
        return NULL;
    }

    /* real assigns. first, see if its just pointing to a single dir */
    if (dp->dvp_Flags != DVPF_ASSIGN) {
        /* just a plain assign, easy */
        dp->dvp_Port = (struct MsgPort *) dl->dol_Ext.dol_AROS.dol_Device;
        dp->dvp_Lock = dl->dol_Lock;
        dp->dvp_DevNode = dl;
        
        /* note multidirectory assigns so the caller knows to loop */
        dp->dvp_Flags = dl->dol_misc.dol_assign.dol_List != NULL ? DVPF_ASSIGN : 0;

        UnLockDosList(LDF_ALL | LDF_READ);

        return dp;
    }

    /* finally the tricky but - multidirectory assigns */

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
    dp->dvp_Port = (struct MsgPort *) ((struct FileHandle *) BADDR(dp->dvp_Lock))->fh_Device;
    dp->dvp_Flags = DVPF_ASSIGN;
    dp->dvp_DevNode = dl;

    UnLockDosList(LDF_READ|LDF_ALL);

    /* phew */
    SetIoErr(0);
    return dp;

    AROS_LIBFUNC_EXIT
} /* GetDeviceProc */

/* Attempt to start a handler for the DeviceNode */
BOOL RunHandler(struct DeviceNode *deviceNode, struct DosLibrary *DOSBase)
{
    struct MsgPort *mp;
    struct IOFileSys *iofs;
    BOOL ok = FALSE;

    mp = CreateMsgPort();

    if (mp != NULL)
    {
        iofs = (struct IOFileSys *)CreateIORequest(mp, sizeof(struct IOFileSys));

        if (iofs != NULL)
        {
	    STRPTR handler;
	    struct FileSysStartupMsg *fssm;

	    if (deviceNode->dn_Handler == NULL)
	    {
		handler = "afs.handler";
	    }
	    else
	    {
		handler = AROS_BSTR_ADDR(deviceNode->dn_Handler);
	    }

	    /* FIXME: this assumes that dol_Startup points to struct FileSysStartupMsg.
	       This is not true for plain handlers, dol_Startup is a BSTR in this case.
	       In order to make use of this we should implement direct support for
	       packet-style handlers in dos.library */
	    fssm = (struct FileSysStartupMsg *)BADDR(deviceNode->dn_Startup);
	    iofs->io_Union.io_OpenDevice.io_DeviceName = AROS_BSTR_ADDR(fssm->fssm_Device);
	    iofs->io_Union.io_OpenDevice.io_Unit       = fssm->fssm_Unit;
	    iofs->io_Union.io_OpenDevice.io_Environ    = (IPTR *)BADDR(fssm->fssm_Environ);
	    iofs->io_Union.io_OpenDevice.io_DosName    = deviceNode->dn_Ext.dn_AROS.dn_DevName;
	    iofs->io_Union.io_OpenDevice.io_DeviceNode = deviceNode;

	    D(bug("Starting up %s\n", handler));
	    if (!OpenDevice(handler, 0, &iofs->IOFS, fssm->fssm_Flags) ||
        	!OpenDevice("packet.handler", 0, &iofs->IOFS, fssm->fssm_Flags))
	    {
		/* Ok, this means that the handler was able to open. */
		D(bug("Handler started\n"));
		deviceNode->dn_Ext.dn_AROS.dn_Device = iofs->IOFS.io_Device;
		deviceNode->dn_Ext.dn_AROS.dn_Unit = iofs->IOFS.io_Unit;
		ok = TRUE;
	    }

	    DeleteIORequest(&iofs->IOFS);
	}

	DeleteMsgPort(mp);
    }
    return ok;
}
