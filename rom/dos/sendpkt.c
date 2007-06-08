/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <aros/debug.h>
#include "dos_intern.h"
#include <dos/dosextens.h>
#include <dos/notify.h>
#include <proto/exec.h>
#include <exec/initializers.h>
#include <string.h>
#include <proto/utility.h>

/* TODO: This can be done much better! */
#ifndef AROS_FAST_BPTR
#define  BStrtoCStr(bstr) ({ \
                STRPTR cstr; \
                cstr = AllocMem(AROS_BSTR_strlen(bstr)+1, MEMF_PUBLIC); \
		if(cstr == NULL) \
		    return; \
	        CopyMem(AROS_BSTR_ADDR(bstr), cstr, AROS_BSTR_strlen(bstr)); \
 	        cstr[AROS_BSTR_strlen(bstr)] = 0; \
		return cstr;
		});
#else
#define  BStrtoCStr(bstr) ((STRPTR)bstr)
#endif


LONG DoNameAsynch(struct IOFileSys *iofs, STRPTR name, 
		  struct DosLibrary *DOSBase);

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(void, SendPkt,

/*  SYNOPSIS */
	AROS_LHA(struct DosPacket *, dp, D1),
	AROS_LHA(struct MsgPort   *, port, D2),
	AROS_LHA(struct MsgPort   *, replyport, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 41, Dos)

/*  FUNCTION

    Send a packet to a handler without waiting for the result. The packet will
    be returned to 'replyport'.

    INPUTS

    packet     --  the (initialized) packet to send
    port       --  the MsgPort to send the packet to
    replyport  --  the MsgPort to which the packet will be replied

    RESULT

    This function is callable from a task.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    DoPkt(), WaitPkt(), AbortPkt()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /*
     * Trying to emulate the packet system by rewriting the
     * packets to IO Requests. Sometimes there are too many
     * parameters in the packet but thats fine. If there are
     * not enough parameters or the wrong type etc. then
     * it is more difficult to translate the packet.
     *
     */

    struct IOFileSys  *iofs = AllocMem(sizeof(struct IOFileSys), MEMF_CLEAR);
    struct FileHandle *fh;
    BPTR               oldCurDir;
    LONG               result; 
     
    if (iofs != NULL)
    {
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
	iofs->IOFS.io_Message.mn_ReplyPort = replyport;
	iofs->IOFS.io_Message.mn_Length = sizeof(struct IOFileSys);
	iofs->IOFS.io_Flags = 0;

        /*
         * Have to rewrite this packet...
         */

        switch (dp->dp_Type)
	{
	case ACTION_NIL:
	case ACTION_READ_RETURN:
	case ACTION_WRITE_RETURN:
	case ACTION_TIMER:
	    kprintf("This packet is a reserved one and should not be sent"
		    " by you!\n");
	    // Alert();
	    return;
	    
	    // case ACTION_GET_BLOCK:
	case ACTION_DISK_CHANGE:
	case ACTION_DISK_TYPE:
	case ACTION_EVENT:
	case ACTION_SET_MAP:
	    
	    kprintf("This packet is obsolete. (ACTION type %d)\n",
		    dp->dp_Type);
	    return;


	case ACTION_FINDINPUT:      // Open() MODE_OLDFILE [*]
	    fh = (struct FileHandle *)dp->dp_Arg2;
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    
	    iofs->IOFS.io_Command = FSA_OPEN_FILE;
	    iofs->io_Union.io_OPEN_FILE.io_FileMode = FMF_WRITE | FMF_READ;
	    
	    oldCurDir = CurrentDir((BPTR)dp->dp_Arg2);
	    result = DoNameAsynch(iofs, BStrtoCStr((BSTR)dp->dp_Arg3),
				  DOSBase);
	    CurrentDir(oldCurDir);

	    if (result != 0)
	    {
		kprintf("Error: Didn't find file\n");
		return;
	    }
	    
	    kprintf("Returned from DoNameAsynch()\n");

	    break;
	    
	case ACTION_FINDOUTPUT:     // Open() MODE_NEWFILE [*]
	    fh = (struct FileHandle *)dp->dp_Arg2;
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    
	    iofs->IOFS.io_Command = FSA_OPEN_FILE;
	    iofs->io_Union.io_OPEN_FILE.io_FileMode = FMF_LOCK |
		FMF_CREATE | FMF_CLEAR | FMF_WRITE | FMF_READ;
	    
	    oldCurDir = CurrentDir((BPTR)dp->dp_Arg2);
	    result = DoNameAsynch(iofs, BStrtoCStr((BSTR)dp->dp_Arg3),
				  DOSBase);
	    CurrentDir(oldCurDir);

	    if (result != 0)
	    {
		return;
	    }
	    	    
	    break;
	    

	case ACTION_FINDUPDATE:     // Open() MODE_READWRITE [*]
	    fh = (struct FileHandle *)dp->dp_Arg2;
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    
	    iofs->IOFS.io_Command = FSA_OPEN_FILE;
	    iofs->io_Union.io_OPEN_FILE.io_FileMode = FMF_CREATE |
		FMF_WRITE | FMF_READ;
	    
	    oldCurDir = CurrentDir((BPTR)dp->dp_Arg2);
	    result = DoNameAsynch(iofs, BStrtoCStr((BSTR)dp->dp_Arg3),
				  DOSBase);
	    CurrentDir(oldCurDir);

	    if (result != 0)
	    {
		return;
	    }
	    	    
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
		fh->fh_Pos = fh->fh_End = fh->fh_Buf;
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
		UBYTE *buffer = AllocVec(sizeof(struct ExAllData), 
					 MEMF_PUBLIC | MEMF_CLEAR);

		if (buffer == NULL)
		{
		    dp->dp_Res1 = DOSFALSE;
		    dp->dp_Res2 = ERROR_NO_FREE_STORE;

		    return;
		}
		
		fh = (struct FileHandle *)dp->dp_Arg1;
		
		iofs->IOFS.io_Device = fh->fh_Device;
		iofs->IOFS.io_Unit   = fh->fh_Unit;
		
		iofs->IOFS.io_Command = FSA_EXAMINE;
		iofs->io_Union.io_EXAMINE.io_ead = (struct ExAllData *)buffer;
		iofs->io_Union.io_EXAMINE.io_Size = sizeof(struct ExAllData);
		iofs->io_Union.io_EXAMINE.io_Mode = ED_OWNER;

		/* A supplied FileInfoBlock (is a BPTR) is in dp_Arg2 */
		
		break;
	    }
	    
	case ACTION_EXAMINE_NEXT:   // ExNext() [*]
	    fh = (struct FileHandle *)dp->dp_Arg1;
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    iofs->IOFS.io_Unit   = fh->fh_Unit;
	    
	    iofs->IOFS.io_Command = FSA_EXAMINE_NEXT;
	    iofs->io_Union.io_EXAMINE_NEXT.io_fib = (BPTR)dp->dp_Arg2;
	    
	    break;	    

	case ACTION_CREATE_DIR:     // CreateDir() [*]
	    fh = (struct FileHandle *)dp->dp_Arg1;
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    iofs->IOFS.io_Unit   = fh->fh_Unit;
	    
	    iofs->IOFS.io_Command = FSA_CREATE_DIR;
	    iofs->io_Union.io_CREATE_DIR.io_Filename = BStrtoCStr((BSTR)dp->dp_Arg2);
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
	    // iofs->io_Union.io_DELETE_OBJECT.io_Filename = BStrtoCStr((BSTR)dp->dp_Arg2);
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
	    result = DoNameAsynch(iofs, "/", DOSBase);
	    CurrentDir(oldCurDir);
	    
	    if (result != 0)
	    {
		return;
	    }

	    return;

	case ACTION_SET_PROTECT:    // SetProtection()    [*]
	    // STRPTR io_Filename;   /* The file to change. */
	    // ULONG  io_Protection; /* The new protection bits. */

	    fh = (struct FileHandle *)dp->dp_Arg2;
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    iofs->IOFS.io_Unit   = fh->fh_Unit;
	    	
	    iofs->IOFS.io_Command = FSA_SET_PROTECT;
	    iofs->io_Union.io_SET_PROTECT.io_Protection = dp->dp_Arg4;
	    iofs->io_Union.io_SET_PROTECT.io_Filename = BStrtoCStr(dp->dp_Arg3);
		
	    break;
	    
	case ACTION_SET_COMMENT:    // SetComment()       [*]
	    fh = (struct FileHandle *)dp->dp_Arg2;
	    
	    iofs->IOFS.io_Device = fh->fh_Device;
	    iofs->IOFS.io_Unit   = fh->fh_Unit;
	    
	    iofs->IOFS.io_Command = FSA_SET_COMMENT;
	    iofs->io_Union.io_SET_COMMENT.io_Filename = BStrtoCStr((BSTR)dp->dp_Arg3);
	    iofs->io_Union.io_SET_COMMENT.io_Comment = BStrtoCStr(dp->dp_Arg4);
	    
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
	    result = DoNameAsynch(iofs, BStrtoCStr((BSTR)dp->dp_Arg2),
				  DOSBase);
	    CurrentDir(oldCurDir);

	    if (result != 0)
	    {
		return;
	    }
	    	    
	    dp->dp_Arg6 = (IPTR)AllocDosObject(DOS_FILEHANDLE, NULL);

	    if (dp->dp_Arg6 == NULL)
	    {
		return;
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
	    result = DoNameAsynch(iofs, "", DOSBase);
	    CurrentDir(oldCurDir);

	    if (result != 0)
	    {
		return;
	    }
	    
	    dp->dp_Arg6 = (IPTR)AllocDosObject(DOS_FILEHANDLE, NULL);

	    if (dp->dp_Arg6 == NULL)
	    {
		return;
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
	    result = DoNameAsynch(iofs, BStrtoCStr((BSTR)dp->dp_Arg3),
				  DOSBase);
	    CurrentDir(oldCurDir);

	    if (result != 0)
	    {
		return;
	    }
	    	    
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
	    {
		STRPTR name = BStrtoCStr(dp->dp_Arg2);
	    
		if (dp->dp_Arg4 == LINK_SOFT)
		{
		    /* We want a soft-link. */
		    iofs->IOFS.io_Command = FSA_CREATE_SOFTLINK;
		    iofs->io_Union.io_CREATE_SOFTLINK.io_Reference = (STRPTR)dp->dp_Arg3;
		}
		else
		{
		    /* We want a hard-link. */
		    struct FileHandle *fh = (struct FileHandle *)BADDR((BPTR)dp->dp_Arg3);
                    struct DevProc *dvp;

		    /* We check, if name and dest are on the same device. */
                    if ((dvp = GetDeviceProc(name, NULL)) == NULL)
		    {
			/* TODO: Simulate packet return */
			return;
		    }
		    
		    if (dvp->dvp_Port != fh->fh_Device)
		    {
                        FreeDeviceProc(dvp);
			SetIoErr(ERROR_RENAME_ACROSS_DEVICES);
			
			/* TODO: Simulate packet return */
			return;
		    }

                    FreeDeviceProc(dvp);
		    
		    iofs->IOFS.io_Command = FSA_CREATE_HARDLINK;
		    iofs->io_Union.io_CREATE_HARDLINK.io_OldFile = fh->fh_Unit;
		}
		
		oldCurDir = CurrentDir((BPTR)dp->dp_Arg1);
		DoNameAsynch(iofs, name, DOSBase);
		CurrentDir(oldCurDir);
	    }
	    
	    break;
	    
	case ACTION_READ_LINK:      // ReadLink()
	    /* TODO */
	    break;
	    
	case ACTION_EXAMINE_ALL:    // ExAll()
	    /* TODO */
	    break;
	    
	case ACTION_ADD_NOTIFY:     // StartNotify()
	    {
		struct NotifyRequest *notify = (struct NotifyRequest *)BADDR(dp->dp_Arg1);
	        struct FileHandle *dir;

		iofs->IOFS.io_Command = FSA_ADD_NOTIFY;
		iofs->io_Union.io_NOTIFY.io_NotificationRequest = notify;

		notify->nr_MsgCount = 0;

		if (strchr(notify->nr_Name, ':') != NULL)
		{
		    DoNameAsynch(iofs, notify->nr_Name, DOSBase);
		}
		else
		{
		    dir = BADDR(CurrentDir(NULL));
		    CurrentDir(MKBADDR(dir));	/* Set back the current dir */
		    
		    if (dir == NULL)
		    {
			return;
		    }
		    
		    iofs->IOFS.io_Device = dir->fh_Device;
		    iofs->IOFS.io_Unit = dir->fh_Unit;
		    
		    /* Save device for EndNotify() purposes */
		    notify->nr_Handler = (struct MsgPort *) dir->fh_Device;
	
		    if (iofs->IOFS.io_Device == NULL)
		    {
			return;
		    }
		}
	    }

	    break;
	    
	case ACTION_REMOVE_NOTIFY:  // EndNotify()
	    {
		struct NotifyRequest *notify = (struct NotifyRequest *)BADDR(dp->dp_Arg1);

		iofs->IOFS.io_Command = FSA_REMOVE_NOTIFY;
		iofs->io_Union.io_NOTIFY.io_NotificationRequest = notify;
		
		if (strchr(notify->nr_Name, ':'))
		{
		    DoNameAsynch(iofs, notify->nr_Name, DOSBase);
		}
		else
		{
		    iofs->IOFS.io_Device = (struct Device *)notify->nr_Handler;
		    
		    if (iofs->IOFS.io_Device == NULL)
		    {
			return;
		    }
		}
	    }

	    break;
	    

	    /* The following five packets are only relative to the
	       message port of the file system to send to. Therefore, 
	       we fill in only partial information in the IOFileSys and
	       hope for the best... Specifically, the device cannot
	       use iofs->io_Device. TODO */
	case ACTION_RENAME_DISK:    // Relabel()
	    /* TODO */
	    break;

	case ACTION_FORMAT:         // Format()
	    /* TODO */
	    break;
	    
	case ACTION_MORE_CACHE:     // AddBuffers()
	    /* TODO */
	    break;
	    
	case ACTION_INHIBIT:        // Inhibit()
	    /* TODO */
	    break;

	case ACTION_IS_FILESYSTEM:  // IsFileSystem()
	    /* TODO */
	    break;

	case ACTION_DISK_INFO:      // Info()
	    /* TODO */
	    break;

	case ACTION_INFO:           // No associated function
	    /* TODO */
	    break;

	case ACTION_CURRENT_VOLUME: // No associated function
	    /* TODO */
	    break;

	case ACTION_PARENT_FH:      // ParentOfFH()
	    /* TODO -- Should be the same as ACTION_PARENT? For some reason
	               ParentDir() and ParentOfFH() is implemented differently
	               in AROS. Have to investigate this further. */
	    break;
	    
	case ACTION_FH_FROM_LOCK:   // OpenFromLock()
	    /* TODO: Have so simulate ReplyMsg() here */
	    return;

	case ACTION_SET_OWNER:      // SetOwner()
	    // Unfortunately, I have no info regardning this packet.
	    
	    // iofs->IOFS.io_Command = FSA_SET_OWNER;
	    // iofs->io_Union.io_SET_OWNER.io_UID = owner_info >> 16;
	    // iofs->io_Union.io_SET_OWNER.io_GID = owner_info & 0xffff;
	    
	    break;

	case ACTION_CHANGE_MODE:    // ChangeMode()
	    /* TODO */
	    break;

	    /* I haven't looked into the following packets in detail */
	case ACTION_DIE:            // No associated function
	    break;
	    
	case ACTION_WRITE_PROTECT:  // No associated function
	    break;
	    
	case ACTION_FLUSH:          // No associated function
	    break;
	    
	case ACTION_SERIALIZE_DISK: // No associated function
	    break;
	    

	    // ---  Console only packets  ------------------------------
	    /* I think these should not be implemented as packets except
	       for ACTION_WAIT_CHAR which is not really a console only
	       packet. Instead functions should be added to console.device */

	case ACTION_SCREEN_MODE:    // SetMode()
	    /* TODO*/
	    break;
	    
	case ACTION_CHANGE_SIGNAL:  // No associated function
	    {
	    	struct MsgPort *msgport;
		struct Task    *task;
		
		fh = (struct FileHandle *)dp->dp_Arg1;
    	    	msgport = (struct MsgPort *)dp->dp_Arg2;
		task = msgport ? msgport->mp_SigTask : NULL;
		
		iofs->IOFS.io_Device = fh->fh_Device;
		iofs->IOFS.io_Unit   = fh->fh_Unit;

		iofs->IOFS.io_Command = FSA_CHANGE_SIGNAL;
		iofs->io_Union.io_CHANGE_SIGNAL.io_Task = task;
	    }
	    break;
	    
	case ACTION_WAIT_CHAR:      // WaitForChar()	
	    /* TODO */
	    break;

	default:
	    kprintf("Unknown packet type %d found in SendPkt()\n",
		    dp->dp_Type);
	    return;
	}

	kprintf("Calling SendIO() with command %u\n", iofs->IOFS.io_Command);
	
	SendIO((struct IORequest *)iofs);
    }        
 
   AROS_LIBFUNC_EXIT
} /* SendPkt */


LONG DoNameAsynch(struct IOFileSys *iofs, STRPTR name, 
		  struct DosLibrary *DOSBase)
{
    STRPTR volname, pathname, s1 = NULL;
    BPTR cur, lock = (BPTR)NULL;
    struct DosList    *dl;
    struct Device     *device;
    struct Unit       *unit;
    struct FileHandle *fh;
    struct Process    *me = (struct Process *)FindTask(NULL);

    if (!Strnicmp(name, "PROGDIR:", 8) && me->pr_HomeDir)
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

	    if (lock != NULL)
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
    iofs->io_Union.io_NamedFile.io_Filename = pathname;

    if (volname != NULL)
    {
	FreeMem(volname, s1 - name);
    }

    return 0;
}
