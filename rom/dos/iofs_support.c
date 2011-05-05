/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Packet I/O emulator for IOFS handlers
    Lang: English
*/

#ifndef AROS_DOS_PACKETS

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <dos/dosextens.h>
#include <dos/notify.h>
#include <proto/exec.h>
#include <exec/initializers.h>
#include <string.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include "dos_intern.h"

/*
 * IOFS_SendPkt() and DoNameAsynch() need to call dos.library functions,
 * including these using DOSBase.
 * For binary compatibility with m68k, packet I/O code must work without
 * valid DOSBase supplied. Because of this we store a static copy of DOSBase
 * for ourselves here.
 */
static struct DosLibrary *DOSBase;

static LONG setDosBase(struct DosLibrary *base)
{
    DOSBase = base;
    return TRUE;
}

ADD2INITLIB(setDosBase, 10);

static LONG DoNameAsynch(struct IOFileSys *iofs, STRPTR name)
{
    STRPTR volname, pathname, s1 = NULL;
    BPTR cur, lock = (BPTR)NULL;
    struct DosList    *dl;
    struct Device     *device;
    struct Unit       *unit;
    struct FileHandle *fh;
    struct Process    *me = (struct Process *)FindTask(NULL);

    if (!strnicmp(name, "PROGDIR:", 8) && me->pr_HomeDir)
    {
	cur = me->pr_HomeDir;
	volname = NULL;
	pathname = name + 8;
    }
    else if (*name == ':')
    {
	cur = me->pr_CurrentDir;
	volname = NULL;
	pathname = name + 1;
    }
    else
    {
	/* Copy volume name */
	cur = me->pr_CurrentDir;
	s1 = name;
	pathname = name;
	volname = NULL;

	while (*s1)
	{
	    if (*s1++ == ':')
	    {
		volname = (STRPTR)AllocMem(s1-name, MEMF_ANY);

		if (volname == NULL)
		{
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
	    UnLockDosList(LDF_ALL | LDF_READ);
	    FreeMem(volname, s1 - name);

	    return ERROR_DEVICE_NOT_MOUNTED;
	}
	else if (dl->dol_Type == DLT_LATE)
	{
	    lock = Lock(dl->dol_misc.dol_assign.dol_AssignName, SHARED_LOCK);
	    UnLockDosList(LDF_ALL | LDF_READ);

	    if (lock != BNULL)
	    {
		AssignLock(volname, lock);
		dl = LockDosList(LDF_ALL | LDF_READ);
		dl = FindDosEntry(dl, volname, LDF_ALL);

		if (dl == NULL)
		{
		    UnLockDosList(LDF_ALL | LDF_READ);
		    FreeMem(volname, s1 - name);

		    return ERROR_DEVICE_NOT_MOUNTED;
	        }
		
		device = dl->dol_Ext.dol_AROS.dol_Device;
		unit = dl->dol_Ext.dol_AROS.dol_Unit;
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
		FreeMem(volname, s1 - name);

		return IoErr();
	    }
	}
	else
	{
	    device = dl->dol_Ext.dol_AROS.dol_Device;
	    unit   = dl->dol_Ext.dol_AROS.dol_Unit;
	}
    }
    else if (cur)
    {
	fh = (struct FileHandle *)BADDR(cur);
	device = fh->fh_Device;
	unit = fh->fh_Unit;
    }
    else
    {
    	fh = (struct FileHandle *)BADDR(DOSBase->dl_SYSLock);
	device = fh->fh_Device;
	unit = fh->fh_Unit;
    }

    iofs->IOFS.io_Device = device;
    iofs->IOFS.io_Unit = unit;
    iofs->io_Union.io_NamedFile.io_Filename = pathname;

    if (volname != NULL)
    {
	FreeMem(volname, s1 - name);
    }

    return 0;
}

void IOFS_SendPkt(struct DosPacket *dp, struct MsgPort *replyport)
{
    /*
     * Trying to emulate the packet system by rewriting the
     * packets to IO Requests. Sometimes there are too many
     * parameters in the packet but that's fine. If there are
     * not enough parameters or the wrong type etc. then
     * it is more difficult to translate the packet.
     */
    struct IOFileSys *iofs;
    struct NotifyRequest *notify;
    LONG result = 0;

    /* First we check for some simple operations that doesn't require us to actually create IOFS request */
    switch (dp->dp_Type)
    {
    case ACTION_FH_FROM_LOCK:
    	/*
    	 * This one is very simple. Since locks and filehandles are the same,
    	 * we just reply with success and exit.
    	 */
	internal_ReplyPkt(dp, NULL, DOSTRUE, 0);
	return;

    case ACTION_REMOVE_NOTIFY:
    	/*
    	 * ACTION_REMOVE_NOTIFY implementation below relies on the fact
    	 * that nr_Handler points to IOFS device. However with packet filesystems
    	 * this may be not true. Here we catch such a case.
    	 */
    	notify = (struct NotifyRequest *)BADDR(dp->dp_Arg1);

    	if (notify->nr_Handler->mp_Node.ln_Type != NT_DEVICE)
    	{
    	    D(bug("[Pkt emu] NotifyRequest points to packet handler, using direct I/O\n"));

    	    /* Send a packet directly to the handler bypassing IOFS layer */
    	    dp->dp_Port               = replyport;
    	    dp->dp_Link->mn_ReplyPort = replyport;

    	    PutMsg(notify->nr_Handler, dp->dp_Link);
    	    return;
    	}
    }

    /* Have to rewrite this packet... */
    iofs = AllocMem(sizeof(struct IOFileSys), MEMF_CLEAR);
    D(bug("[Pkt emu] Allocated request 0x%p\n", iofs));

    if (iofs != NULL)
    {
        struct FileHandle *fh;
	BPTR oldCurDir;

	/*
	 * Also attach the packet to the io request.
	 * Will remain untouched by the driver.
	 */
	iofs->io_PacketEmulation = dp;

	/*
	 * In case the packet is to be aborted 
	 * I know which IORequest to use. The user will
	 * use the packet itself to abort the operation.
	 */
	dp->dp_Arg7 = (IPTR)iofs;

	iofs->IOFS.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
	iofs->IOFS.io_Message.mn_ReplyPort    = replyport;
	iofs->IOFS.io_Message.mn_Length       = sizeof(struct IOFileSys);
	iofs->IOFS.io_Flags                   = 0;

        switch (dp->dp_Type)
	{
	case ACTION_FINDINPUT:      // Open() MODE_OLDFILE [*]
	    fh = (struct FileHandle *)dp->dp_Arg2;

	    iofs->IOFS.io_Device  = fh->fh_Device;
	    iofs->IOFS.io_Command = FSA_OPEN_FILE;
	    iofs->io_Union.io_OPEN_FILE.io_FileMode = FMF_WRITE | FMF_READ;

	    oldCurDir = CurrentDir((BPTR)dp->dp_Arg2);
	    result = DoNameAsynch(iofs, BSTR2C((BSTR)dp->dp_Arg3));
	    CurrentDir(oldCurDir);

	    D(kprintf("[Pkt emu] Returned from DoNameAsynch(), result %u\n", result));
	    break;

	case ACTION_FINDOUTPUT:     // Open() MODE_NEWFILE [*]
	    fh = (struct FileHandle *)dp->dp_Arg2;
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    
	    iofs->IOFS.io_Command = FSA_OPEN_FILE;
	    iofs->io_Union.io_OPEN_FILE.io_FileMode = FMF_LOCK |
		FMF_CREATE | FMF_CLEAR | FMF_WRITE | FMF_READ;
	    
	    oldCurDir = CurrentDir((BPTR)dp->dp_Arg2);
	    result = DoNameAsynch(iofs, BSTR2C((BSTR)dp->dp_Arg3));
	    CurrentDir(oldCurDir);
    
	    break;	    

	case ACTION_FINDUPDATE:     // Open() MODE_READWRITE [*]
	    fh = (struct FileHandle *)dp->dp_Arg2;
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    
	    iofs->IOFS.io_Command = FSA_OPEN_FILE;
	    iofs->io_Union.io_OPEN_FILE.io_FileMode = FMF_CREATE |
		FMF_WRITE | FMF_READ;
	    
	    oldCurDir = CurrentDir((BPTR)dp->dp_Arg2);
	    result = DoNameAsynch(iofs, BSTR2C((BSTR)dp->dp_Arg3));
	    CurrentDir(oldCurDir);
 
	    break;

	case ACTION_READ:           // Read() [*]
	    fh = (struct FileHandle *)dp->dp_Arg1;
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    iofs->IOFS.io_Unit   = fh->fh_Unit;
	    
	    iofs->IOFS.io_Command = FSA_READ;
	    iofs->io_Union.io_READ_WRITE.io_Buffer = (APTR)dp->dp_Arg2;
	    iofs->io_Union.io_READ_WRITE.io_Length = (LONG)dp->dp_Arg3;
	    
	    break;

	case ACTION_WRITE:          // Write() [*]
	    fh = (struct FileHandle *)dp->dp_Arg1;
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    iofs->IOFS.io_Unit   = fh->fh_Unit;
	    
	    iofs->IOFS.io_Command = FSA_WRITE;
	    iofs->io_Union.io_READ_WRITE.io_Buffer = (APTR)dp->dp_Arg2;
	    iofs->io_Union.io_READ_WRITE.io_Length = (LONG)dp->dp_Arg3;

	    break;

	case ACTION_SEEK:           // Seek()      [*] CH
	    fh = (struct FileHandle *)dp->dp_Arg1;
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    iofs->IOFS.io_Unit   = fh->fh_Unit;

	    /* If the file is in write mode flush it. This is done
	       synchronously as otherwise the seek may be served before
	       the flush. (TODO: What are a reasonable semantics here?) */
	    if (fh->fh_Flags & FHF_WRITE)
	    {
		Flush(MKBADDR(fh));
	    }
	    else
	    {
		/* Read mode. Just reinit the buffers. We can't call
		   Flush() in this case as that would end up in recursion. */
		fh->fh_Pos = fh->fh_End = 0;
	    }
	    
	    iofs->IOFS.io_Command = FSA_SEEK;
	    iofs->io_Union.io_SEEK.io_Offset = (QUAD)dp->dp_Arg2;
	    iofs->io_Union.io_SEEK.io_SeekMode = dp->dp_Arg3;
	    
	    break;
	    
	case ACTION_SET_FILE_SIZE:  // SetFileSize()        [*]
	    fh = (struct FileHandle *)dp->dp_Arg1;
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    iofs->IOFS.io_Unit   = fh->fh_Unit;
	    
	    iofs->IOFS.io_Command = FSA_SET_FILE_SIZE;
	    iofs->io_Union.io_SET_FILE_SIZE.io_Offset   = (QUAD)dp->dp_Arg2;
	    iofs->io_Union.io_SET_FILE_SIZE.io_SeekMode = dp->dp_Arg3;
	    break;

	case ACTION_EXAMINE_FH:     // ExamineFH()
	case ACTION_EXAMINE_OBJECT: // Examine() [*]  --  the same thing
	                            //                    in AROS
	{
	    UBYTE *buffer = AllocVec(sizeof(struct ExAllData), MEMF_PUBLIC | MEMF_CLEAR);

	    if (buffer)
	    {		
		fh = (struct FileHandle *)dp->dp_Arg1;
		
		iofs->IOFS.io_Device = fh->fh_Device;
		iofs->IOFS.io_Unit   = fh->fh_Unit;
		
		iofs->IOFS.io_Command = FSA_EXAMINE;
		iofs->io_Union.io_EXAMINE.io_ead = (struct ExAllData *)buffer;
		iofs->io_Union.io_EXAMINE.io_Size = sizeof(struct ExAllData);
		iofs->io_Union.io_EXAMINE.io_Mode = ED_OWNER;

		/* A supplied FileInfoBlock (is a BPTR) is in dp_Arg2 */
	    }
	    else
	    	result = ERROR_NO_FREE_STORE;

	    break;
	}
	    
	case ACTION_EXAMINE_NEXT:   // ExNext() [*]
	    fh = (struct FileHandle *)dp->dp_Arg1;
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    iofs->IOFS.io_Unit   = fh->fh_Unit;
	    
	    iofs->IOFS.io_Command = FSA_EXAMINE_NEXT;
	    iofs->io_Union.io_EXAMINE_NEXT.io_fib = BADDR(dp->dp_Arg2);
	    
	    break;	    

	case ACTION_CREATE_DIR:     // CreateDir() [*]
	    fh = (struct FileHandle *)dp->dp_Arg1;
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    iofs->IOFS.io_Unit   = fh->fh_Unit;
	    
	    iofs->IOFS.io_Command = FSA_CREATE_DIR;
	    iofs->io_Union.io_CREATE_DIR.io_Filename = BSTR2C((BSTR)dp->dp_Arg2);
	    iofs->io_Union.io_CREATE_DIR.io_Protection = 0;
	    
	    break;

	    
	case ACTION_DELETE_OBJECT:  // DeleteFile()     [*]
	    // ARG1:   LOCK    Lock to which ARG2 is relative
	    // ARG2:   BSTR    Name of object to delete (relative to ARG1)
	    
	    fh = (struct FileHandle *)dp->dp_Arg1;
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    iofs->IOFS.io_Unit   = fh->fh_Unit;
	    
	    /* IFS_DELETE_OBJECT just consists of a STRPTR so we have to
	       use NameFromLock() here -- TODO */
	    // iofs->io_Union.io_DELETE_OBJECT.io_Filename = BSTR2C((BSTR)dp->dp_Arg2);
	    iofs->IOFS.io_Command = FSA_DELETE_OBJECT;
	    
	    break;

	case ACTION_RENAME_OBJECT:  // Rename()
	    /* TODO */
	    break;

	case ACTION_LOCK_RECORD:    // LockRecord()
	    fh = (struct FileHandle *)BADDR(dp->dp_Arg1);

	    iofs->IOFS.io_Device = fh->fh_Device;
	    iofs->IOFS.io_Unit = fh->fh_Unit;

	    iofs->io_Union.io_RECORD.io_Offset = (QUAD)dp->dp_Arg2;
	    iofs->io_Union.io_RECORD.io_Size = (LONG)dp->dp_Arg3;
	    iofs->io_Union.io_RECORD.io_RecordMode = (LONG)dp->dp_Arg4;
	    iofs->io_Union.io_RECORD.io_Timeout = (LONG)dp->dp_Arg5;
	    break;
	    
	case ACTION_FREE_RECORD:    // UnlockRecord()
	    fh = (struct FileHandle *)BADDR(dp->dp_Arg1);
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    iofs->IOFS.io_Unit = fh->fh_Unit;

	    iofs->io_Union.io_RECORD.io_Offset = (QUAD)dp->dp_Arg2;
	    iofs->io_Union.io_RECORD.io_Size = (LONG)dp->dp_Arg3;
	    iofs->io_Union.io_RECORD.io_RecordMode = (LONG)dp->dp_Arg4;
	    break; 

	case ACTION_PARENT:         // ParentDir()     [*]
	    fh = (struct FileHandle *)dp->dp_Arg1;
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    iofs->IOFS.io_Unit   = fh->fh_Unit;
	    
	    iofs->IOFS.io_Command = FSA_OPEN;
	    iofs->io_Union.io_OPEN.io_FileMode = FMF_READ; // Shared lock
	    
	    oldCurDir = CurrentDir((BPTR)dp->dp_Arg1);
	    result = DoNameAsynch(iofs, "/");
	    CurrentDir(oldCurDir);

	    break;

	case ACTION_SET_PROTECT:    // SetProtection()    [*]
	    // STRPTR io_Filename;   /* The file to change. */
	    // ULONG  io_Protection; /* The new protection bits. */

	    fh = (struct FileHandle *)dp->dp_Arg2;
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    iofs->IOFS.io_Unit   = fh->fh_Unit;
	    	
	    iofs->IOFS.io_Command = FSA_SET_PROTECT;
	    iofs->io_Union.io_SET_PROTECT.io_Protection = dp->dp_Arg4;
	    iofs->io_Union.io_SET_PROTECT.io_Filename = BSTR2C(dp->dp_Arg3);
		
	    break;
	    
	case ACTION_SET_COMMENT:    // SetComment()       [*]
	    fh = (struct FileHandle *)dp->dp_Arg2;
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    iofs->IOFS.io_Unit   = fh->fh_Unit;
	    
	    iofs->IOFS.io_Command = FSA_SET_COMMENT;
	    iofs->io_Union.io_SET_COMMENT.io_Filename = BSTR2C((BSTR)dp->dp_Arg3);
	    iofs->io_Union.io_SET_COMMENT.io_Comment = BSTR2C(dp->dp_Arg4);
	    
	    break;

	case ACTION_LOCATE_OBJECT:  // Lock()         [*]
	    fh = (struct FileHandle *)dp->dp_Arg1;
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    iofs->IOFS.io_Unit   = fh->fh_Unit;
	    
	    iofs->IOFS.io_Command = FSA_OPEN;
	    
	    switch (dp->dp_Arg3)
	    {
	    case EXCLUSIVE_LOCK:
		iofs->io_Union.io_OPEN.io_FileMode = FMF_LOCK | FMF_READ;
		break;
		
	    case SHARED_LOCK:
		iofs->io_Union.io_OPEN.io_FileMode = FMF_READ;
		break;
		
	    default:
		iofs->io_Union.io_OPEN.io_FileMode = dp->dp_Arg3;
		break;
	    }
	    
	    oldCurDir = CurrentDir((BPTR)dp->dp_Arg1);
	    result = DoNameAsynch(iofs, BSTR2C((BSTR)dp->dp_Arg2));
	    CurrentDir(oldCurDir);

	    if (!result)
	    {    	    
	    	dp->dp_Arg6 = (IPTR)AllocDosObject(DOS_FILEHANDLE, NULL);

	    	if (dp->dp_Arg6 == 0)
	    	    result = ERROR_NO_FREE_STORE;
	    }

	    break;

	case ACTION_COPY_DIR:       // DupLock()
	case ACTION_COPY_DIR_FH:    // DupLockFromFH() -- the same thing
	                            //                    in AROS
	    fh = (struct FileHandle *)BADDR(dp->dp_Arg1);
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    iofs->IOFS.io_Unit   = fh->fh_Unit;
	    
	    iofs->IOFS.io_Command = FSA_OPEN;
	    
	    /* Create a shared lock */
	    iofs->io_Union.io_OPEN.io_FileMode = FMF_READ;
		
	    oldCurDir = CurrentDir((BPTR)dp->dp_Arg1);
	    result = DoNameAsynch(iofs, "");
	    CurrentDir(oldCurDir);

	    if (!result)
	    {
		dp->dp_Arg6 = (IPTR)AllocDosObject(DOS_FILEHANDLE, NULL);

	    	if (dp->dp_Arg6 == 0)
	    	    result = ERROR_NO_FREE_STORE;
	    }

	    break;

	case ACTION_END:            // Close() [*]
	case ACTION_FREE_LOCK:      // UnLock() -- the same thing in AROS
	    fh = (struct FileHandle *)dp->dp_Arg1;
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    iofs->IOFS.io_Unit   = fh->fh_Unit;
	    
	    /* If the filehandle has a pending write on it Flush() the buffer.
	       This is made synchronously... */
	    if (fh->fh_Flags & FHF_WRITE)
	    {
		Flush(MKBADDR(fh));
	    }

	    iofs->IOFS.io_Command = FSA_CLOSE;

	    break;

	case ACTION_SET_DATE:       // SetFileDate() [*]
	    fh = (struct FileHandle *)dp->dp_Arg2;
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    iofs->IOFS.io_Unit   = fh->fh_Unit;
	    
	    iofs->IOFS.io_Command = FSA_SET_DATE;
	    memcpy(&iofs->io_Union.io_SET_DATE.io_Date, (APTR)dp->dp_Arg4,
		   sizeof(struct DateStamp));

	    oldCurDir = CurrentDir((BPTR)dp->dp_Arg2);
	    result = DoNameAsynch(iofs, BSTR2C((BSTR)dp->dp_Arg3));
	    CurrentDir(oldCurDir);
   	    
	    break;

	case ACTION_SAME_LOCK:      // SameLock() [*]
	    fh = (struct FileHandle *)dp->dp_Arg1;
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    iofs->IOFS.io_Unit   = fh->fh_Unit;
	    
	    iofs->IOFS.io_Command = FSA_SAME_LOCK;
	    
	    iofs->io_Union.io_SAME_LOCK.io_Lock[0] = BADDR((BPTR)dp->dp_Arg1);
	    iofs->io_Union.io_SAME_LOCK.io_Lock[1] = BADDR((BPTR)dp->dp_Arg2);
	    
	    break;
	    
	case ACTION_MAKE_LINK:      // MakeLink()
	    if (dp->dp_Arg4 == LINK_SOFT)
	    {
		/* We want a soft-link. */
		iofs->IOFS.io_Command = FSA_CREATE_SOFTLINK;
		iofs->io_Union.io_CREATE_SOFTLINK.io_Reference = (STRPTR)dp->dp_Arg3;
	    }
	    else
	    {
		/* We want a hard-link. */
		STRPTR name = BSTR2C(dp->dp_Arg2);
                struct DevProc *dvp = GetDeviceProc(name, NULL);

		/* We check, if name and dest are on the same device. */
                if (dvp)
		{
		    fh = (struct FileHandle *)BADDR((BPTR)dp->dp_Arg3);

		    if (dvp->dvp_Port != (struct MsgPort *)fh->fh_Device)
			result = ERROR_RENAME_ACROSS_DEVICES;

                    FreeDeviceProc(dvp);
		    
		    iofs->IOFS.io_Command = FSA_CREATE_HARDLINK;
		    iofs->io_Union.io_CREATE_HARDLINK.io_OldFile = fh->fh_Unit;
		}
		else
		    result = ERROR_OBJECT_NOT_FOUND;

		oldCurDir = CurrentDir((BPTR)dp->dp_Arg1);
		DoNameAsynch(iofs, name);
		CurrentDir(oldCurDir);

		FreeCSTR(name);
	    }

	    break;

/* TODO:
	case ACTION_READ_LINK:      // ReadLink()
	    break;
	    
	case ACTION_EXAMINE_ALL:    // ExAll()
	    break; */

	case ACTION_ADD_NOTIFY:     // StartNotify()
	    notify = (struct NotifyRequest *)BADDR(dp->dp_Arg1);

	    iofs->IOFS.io_Command = FSA_ADD_NOTIFY;
	    iofs->io_Union.io_NOTIFY.io_NotificationRequest = notify;

	    /* If full name is supplied, get its device and root unit */
	    if (strchr(notify->nr_Name, ':') != NULL)
		result = DoNameAsynch(iofs, notify->nr_Name);
	    else
	    {
	    	/* Otherwise use our current directory as a reference */
	    	struct Process *me = (struct Process *)FindTask(NULL);
		struct FileHandle *dir = __is_process(me) ? BADDR(me->pr_CurrentDir) : NULL;

		if (dir)
		{
		    
		    iofs->IOFS.io_Device = dir->fh_Device;
		    iofs->IOFS.io_Unit   = dir->fh_Unit;
		}

		if (iofs->IOFS.io_Device == NULL)
		    result = ERROR_OBJECT_NOT_FOUND;
	    }

	    notify->nr_MsgCount = 0;
	    notify->nr_Handler  = (struct MsgPort *)iofs->IOFS.io_Device;

	    break;

	case ACTION_REMOVE_NOTIFY:  // EndNotify()
	    notify = (struct NotifyRequest *)BADDR(dp->dp_Arg1);

	    iofs->IOFS.io_Command = FSA_REMOVE_NOTIFY;
	    iofs->IOFS.io_Device = (struct Device *)notify->nr_Handler;
	    iofs->IOFS.io_Unit   = (struct Unit *)notify->nr_Reserved[0];

	    D(bug("[Pkt emu] ACTION_REMOVE_NOTIFY: Device 0x%p, Unit 0x%p\n", notify->nr_Handler, notify->nr_Reserved[0]));

	    iofs->io_Union.io_NOTIFY.io_NotificationRequest = notify;

	    break;

	/*
	 * The following five packets are only relative to the
	 * message port of the file system to send to. Therefore, 
	 * we fill in only partial information in the IOFileSys and
	 * hope for the best... Specifically, the device cannot
	 * use iofs->io_Device. TODO
	case ACTION_RENAME_DISK:    // Relabel()
	    break;

	case ACTION_FORMAT:         // Format()
	    break;
	    
	case ACTION_MORE_CACHE:     // AddBuffers()
	    break;
	    
	case ACTION_INHIBIT:        // Inhibit()
	    break;

	case ACTION_IS_FILESYSTEM:  // IsFileSystem()
	    break;

	case ACTION_DISK_INFO:      // Info()
	    break;

	case ACTION_INFO:           // No associated function
	    break;

	case ACTION_CURRENT_VOLUME: // No associated function
	    break;

	case ACTION_PARENT_FH:      // ParentOfFH()
	       TODO -- Should be the same as ACTION_PARENT? For some reason
	               ParentDir() and ParentOfFH() is implemented differently
	               in AROS. Have to investigate this further.
	    break;

	case ACTION_SET_OWNER:      // SetOwner()
	    // Unfortunately, I have no info regardning this packet.

	    // iofs->IOFS.io_Command = FSA_SET_OWNER;
	    // iofs->io_Union.io_SET_OWNER.io_UID = owner_info >> 16;
	    // iofs->io_Union.io_SET_OWNER.io_GID = owner_info & 0xffff;
	    
	    break;

	case ACTION_CHANGE_MODE:    // ChangeMode()
	    break;

	    // I haven't looked into the following packets in detail
	case ACTION_DIE:            // No associated function
	    break;
	    
	case ACTION_WRITE_PROTECT:  // No associated function
	    break;
	    
	case ACTION_FLUSH:          // No associated function
	    break;
	    
	case ACTION_SERIALIZE_DISK: // No associated function
	    break;
	    

	    // ---  Console only packets  ------------------------------
	       I think these should not be implemented as packets except
	       for ACTION_WAIT_CHAR which is not really a console only
	       packet. Instead functions should be added to console.device

	case ACTION_SCREEN_MODE:    // SetMode()
	    break; */

	case ACTION_CHANGE_SIGNAL:  // No associated function
	    fh = (struct FileHandle *)dp->dp_Arg1;

	    iofs->IOFS.io_Device  = fh->fh_Device;
	    iofs->IOFS.io_Unit    = fh->fh_Unit;
	    iofs->IOFS.io_Command = FSA_CHANGE_SIGNAL;

	    iofs->io_Union.io_CHANGE_SIGNAL.io_Task = dp->dp_Arg2 ? ((struct MsgPort *)dp->dp_Arg2)->mp_SigTask : NULL;

	    break;

/* TODO
	case ACTION_WAIT_CHAR:      // WaitForChar()	
	    break; */

	default:
	    D(kprintf("[Pkt emu] Unknown packet type %d found in SendPkt()\n", dp->dp_Type));
	    
	    result = ERROR_ACTION_NOT_KNOWN;
	}

	if (!result)
	{
	    D(kprintf("[Pkt emu] Calling SendIO() with command %u\n", iofs->IOFS.io_Command));
	    SendIO((struct IORequest *)iofs);
	}
    }
    else
    	result = ERROR_NO_FREE_STORE;

    if (result != 0)
    {
    	/* Error happened. We didn't forward the request to the handler, so reply it immediately */
	D(kprintf("[Pkt emu] Error: %u\n", result));

	FreeMem(iofs, sizeof(struct IOFileSys));
	/* We don't have reply port, so set it to NULL */
	internal_ReplyPkt(dp, NULL, DOSFALSE, result);
    }
}

/*
 * FIXME: IOFS_SendPkt() calls BSTR2C() in many places. This means allocating
 * additional memory if real BCPL strings are used. This routine needs to free
 * allocated memory then.
 * Currently real BCPL strings are used only by m68k port, which does not use
 * this code at all, so it's okay. Perhaps IOFS is about to die, so this issue
 * can be currently ignored.
 */
struct DosPacket *IOFS_GetPkt(struct IOFileSys *iofs)
{
    struct DosPacket *packet = iofs->io_PacketEmulation;
    struct FileHandle *fh;
/*  struct NotifyRequest *notify;*/

    D(bug("[Pkt emu] Got request 0x%p\n", iofs));

    /* Convert AROS IOFileSys results back to DosPacket results */
    switch (iofs->IOFS.io_Command)
    {
    case FSA_SEEK:
	packet->dp_Res1 = (IPTR)iofs->io_Union.io_SEEK.io_Offset;
 	packet->dp_Res2 = iofs->io_DosError;
	break;

    /*
     * This FSA corrsponds to ACTION_LOCATE_OBJECT, ACTION_COPY_DIR and
     * ACTION_COPY_DIR_FH
     */
    case FSA_OPEN:
	fh = (struct FileHandle *)packet->dp_Arg6;

	packet->dp_Res1 = (IPTR)MKBADDR(fh);
	packet->dp_Res2 = iofs->io_DosError;

	if (iofs->io_DosError != 0)
	{
	    FreeDosObject(DOS_FILEHANDLE, fh);
	}

	fh->fh_Device = iofs->IOFS.io_Device;
	fh->fh_Unit   = iofs->IOFS.io_Unit;
	break;

    /*
     * This corresponds to ACTION_FINDINPUT, ACTION_FINDOUTPUT,
     * ACTION_FINDUPDATE which fortunately have the same return values
     */
    case FSA_OPEN_FILE:
	fh = (struct FileHandle *)BADDR(packet->dp_Arg1);

	fh->fh_Device = iofs->IOFS.io_Device;
	fh->fh_Unit   = iofs->IOFS.io_Unit;

	packet->dp_Res1 = iofs->io_DosError == 0;
	packet->dp_Res2 = iofs->io_DosError;
	break;

    case FSA_READ:
    case FSA_WRITE:
	packet->dp_Res1 = (IPTR)iofs->io_Union.io_READ_WRITE.io_Length;
	D(kprintf("[Pkt emu] Packet (%p) length = %u", packet, packet->dp_Res1));
	packet->dp_Res2 = iofs->io_DosError;
	break;

    case FSA_CLOSE:
	packet->dp_Res1 = iofs->io_DosError == 0;
	FreeDosObject(DOS_FILEHANDLE, (APTR)packet->dp_Arg1);
	break;

    case FSA_EXAMINE:
	{
	    /* Get supplied FileInfoBlock */
	    struct FileInfoBlock *fib = (struct FileInfoBlock *)BADDR(packet->dp_Arg2);
	    struct ExAllData     *ead = iofs->io_Union.io_EXAMINE.io_ead;

	    packet->dp_Res1 = iofs->io_DosError == 0;
	    packet->dp_Res2 = iofs->io_DosError;
	    
	    /* in fib_DiskKey the result from telldir is being stored which
	       gives us important info for a call to ExNext() */
	    fib->fib_DiskKey      = iofs->io_DirPos;
	    fib->fib_DirEntryType = ead->ed_Type;

	    strncpy(fib->fib_FileName, ead->ed_Name, MAXFILENAMELENGTH);

	    fib->fib_Protection = ead->ed_Prot;
	    fib->fib_EntryType = ead->ed_Type;
	    fib->fib_Size = ead->ed_Size;
	    fib->fib_Date.ds_Days = ead->ed_Days;
	    fib->fib_Date.ds_Minute = ead->ed_Mins;
	    fib->fib_Date.ds_Tick = ead->ed_Ticks;

	    if (ead->ed_Comment != NULL)
	    {
		strncpy(fib->fib_Comment, ead->ed_Comment, MAXCOMMENTLENGTH);
	    }

	    fib->fib_OwnerUID = ead->ed_OwnerUID;
	    fib->fib_OwnerGID = ead->ed_OwnerGID;

	    /* Release temporary buffer memory */
	    FreeVec(ead);
	    break;
	}
	
    case FSA_EXAMINE_NEXT:
    case FSA_SET_FILE_SIZE:
    case FSA_DELETE_OBJECT:
    case FSA_RENAME:
    case FSA_SET_PROTECT:
    case FSA_SET_COMMENT:
    case FSA_SET_DATE:
    case FSA_FORMAT:
    case FSA_IS_FILESYSTEM:
    case FSA_LOCK_RECORD:
    case FSA_UNLOCK_RECORD:
    case FSA_ADD_NOTIFY:
    case FSA_CREATE_HARDLINK:
    case FSA_CREATE_SOFTLINK:
	packet->dp_Res1 = iofs->io_DosError == 0;
 	packet->dp_Res2 = iofs->io_DosError;
	break;

    case FSA_REMOVE_NOTIFY:
/*
 * CHECKME: This code was originally present here but i disabled it.
 * Packet filesystems do not do this. There is a similar code in EndNotify(),
 * however i expect this to be low-level operation which should not do more
 * than packet filesystems do.
 * So this is currently disabled.
 *
	notify = iofs->io_Union.io_NOTIFY.io_NotificationRequest;

	if ((notify->nr_Flags & NRF_SEND_MESSAGE) && notify->nr_MsgCount)
	{
	    struct Node          *tempNode;
	    struct NotifyMessage *nm;

	    Disable();

	    ForeachNodeSafe(&notify->nr_stuff.nr_Msg.nr_Port->mp_MsgList, nm, tempNode)
	    {
		if (nm->nm_NReq == notify)
		{
		    Remove(&nm->mn_ExecMessage.mn_Node);
		    ReplyMsg(&nm->nm_ExecMessage);

		    if (--notify->nr_MsgCount == 0)
		    	break;
		}
	    }

	    Enable();
	}
 */
	packet->dp_Res1 = iofs->io_DosError == 0;
 	packet->dp_Res2 = iofs->io_DosError;

	break;

    case FSA_CREATE_DIR:
	fh = AllocDosObject(DOS_FILEHANDLE, NULL);
	    
	/*
	 * If the allocation operation failed, we are in trouble as we
	 * have to UnLock() the created directory -- this should be moved
	 * to SendPkt()!
	 */

	if (fh == NULL)
	{
	    /* Crash... well, we keep the lock for now */
		
	    packet->dp_Res1 = DOSFALSE;
	    packet->dp_Res2 = ERROR_NO_FREE_STORE;
	}
	else
	{
	    fh->fh_Unit   = iofs->IOFS.io_Unit;
	    fh->fh_Device = iofs->IOFS.io_Device;
	    packet->dp_Res1 = (IPTR)MKBADDR(fh);
	}

	break;

    case FSA_SAME_LOCK:
	packet->dp_Res1 = (IPTR)(iofs->io_Union.io_SAME_LOCK.io_Same == LOCK_SAME);
	packet->dp_Res2 = iofs->io_DosError;
	break;

    case FSA_EXAMINE_ALL:
	/* TODO */
	/* ExAll() seems to be flawed(?)... have to investigate this. */
	break;

    case FSA_DISK_INFO:
    case FSA_INHIBIT:
	packet->dp_Res1 = iofs->io_DosError == 0;
	break;

    case FSA_RELABEL:
	packet->dp_Res1 = iofs->io_Union.io_RELABEL.io_Result;
	break;

    case FSA_MORE_CACHE:
	packet->dp_Res1 = iofs->io_DosError == 0;
	packet->dp_Res2 = iofs->io_Union.io_MORE_CACHE.io_NumBuffers;
	break;

	/* TODO */
    case FSA_READ_SOFTLINK:
    case FSA_FILE_MODE:
    default:
	D(kprintf("[Pkt emu] Filesystem action %u not handled yet in WaitPkt()\n", iofs->IOFS.io_Command));
	break;
    }

    /* The IOFS request was allocated in IOFS_SendPkt(), here we free it */
    FreeMem(iofs, sizeof(struct IOFileSys));
    return packet;
}

#endif /* !AROS_DOS_PACKETS */
