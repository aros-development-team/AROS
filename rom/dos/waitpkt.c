/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#define  DEBUG 1
#include <aros/debug.h>

#include "dos_intern.h"
#include <dos/filesystem.h>
#include <dos/notify.h>
#include <proto/exec.h>
#include <string.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH0(struct DosPacket *, WaitPkt,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct DosLibrary *, DOSBase, 42, Dos)

/*  FUNCTION

    Wait for a packet to arrive at your process' pr_MsgPort. It will call
    pr_PktWait if such a function is installed.

    INPUTS

    RESULT

    The packet we received.

    NOTES

    The packet will be released from the port.

    This function should NOT be used. It's there only for AmigaOS
    compatibility.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    struct Process   *me = (struct Process *)FindTask(NULL);

    return internal_WaitPkt(&me->pr_MsgPort, DOSBase);

    AROS_LIBFUNC_EXIT
} /* WaitPkt */


struct DosPacket *internal_WaitPkt(struct MsgPort *msgPort,
				   struct DosLibrary *DOSBase)
{
    struct Message   *msg;
    struct DosPacket *packet;
    struct Process   *me = (struct Process *)FindTask(NULL);
    struct IOFileSys *iofs;
 
    if (__is_process(me))
    {
	/* Call the packet wait function if the user has one installed.
	   Unfortunately, the user gets something completely different than
	   a packet, but we cannot do anything about that... */
#if 0
	if (me->pr_PktWait != NULL)
	{
	    me->pr_PktWait();
        }
#endif
    }	

    /* Make sure we have a packet -- we may be woken up even if there is
       not a packet for us as SIGF_DOS may be used and we may have another
       message port that waits for packets, too. */
    while ((msg = GetMsg(msgPort)) == NULL)
    {
	Wait(1 << msgPort->mp_SigBit);
    }

    iofs = (struct IOFileSys *)msg;
    packet = iofs->io_PacketEmulation;
    
    /* Convert AROS IOFileSys results back to DosPacket results */

    switch (iofs->IOFS.io_Command)
    {
    case FSA_SEEK:
	packet->dp_Res1 = (IPTR)iofs->io_Union.io_SEEK.io_Offset;
 	packet->dp_Res2 = iofs->io_DosError;
	break;

	/* This FSA corrsponds to ACTION_LOCATE_OBJECT, ACTION_COPY_DIR and
	   ACTION_COPY_DIR_FH */
    case FSA_OPEN:
	{
	    struct FileHandle *fh = (struct FileHandle *)packet->dp_Arg6;

	    packet->dp_Res1 = (IPTR)MKBADDR(fh);
	    packet->dp_Res2 = iofs->io_DosError;

	    if (iofs->io_DosError != 0)
	    {
		FreeDosObject(DOS_FILEHANDLE, fh);
	    }

	    fh->fh_Device = iofs->IOFS.io_Device;
	    fh->fh_Unit   = iofs->IOFS.io_Unit;
	    break;
	}
	
	/* This corresponds to ACTION_FINDINPUT, ACTION_FINDOUTPUT,
	   ACTION_FINDUPDATE which fortunately have the same return values */
    case FSA_OPEN_FILE:
	{
	    struct FileHandle *fh = (struct FileHandle *)BADDR(packet->dp_Arg1);

	    fh->fh_Device = iofs->IOFS.io_Device;
	    fh->fh_Unit   = iofs->IOFS.io_Unit;

	    packet->dp_Res1 = iofs->io_DosError == 0;
	    packet->dp_Res2 = iofs->io_DosError;
	    break;
	}
	
    case FSA_READ:
    case FSA_WRITE:
	packet->dp_Res1 = (IPTR)iofs->io_Union.io_READ_WRITE.io_Length;
	kprintf("Packet (%p) length = %u", packet, packet->dp_Res1);
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
	{
	    struct NotifyRequest *notify = iofs->io_Union.io_NOTIFY.io_NotificationRequest;

	    if (notify->nr_Flags & NRF_SEND_MESSAGE)
	    {
		struct Node          *tempNode;
		struct NotifyMessage *nm;
		
		Disable();
		
		ForeachNodeSafe(&notify->nr_stuff.nr_Msg.nr_Port->mp_MsgList,
				nm, tempNode)
		{
		    if (notify->nr_MsgCount == 0)
		    {
			break;
		    }
		    
		    if (nm->nm_NReq == notify)
		    {
			notify->nr_MsgCount--;
			Remove((struct Node *)nm);
			ReplyMsg((struct Message *)nm);		
		    }
		}
		
		Enable();
	    }
	}

	break;

    case FSA_CREATE_DIR:
	{
	    struct FileHandle *fh = AllocDosObject(DOS_FILEHANDLE, NULL);
	    
	    /* If the allocation operation failed, we are in trouble as we
	       have to UnLock() the created directory -- this should be moved
	       to SendPkt()! */

	    if (fh == NULL)
	    {
		/* Crash... well, we keep the lock for now */
		
		packet->dp_Res1 = DOSFALSE;
		packet->dp_Res2 = ERROR_NO_FREE_STORE;
		break;
	    }

	    fh->fh_Unit   = iofs->IOFS.io_Unit;
	    fh->fh_Device = iofs->IOFS.io_Device;
	    packet->dp_Res1 = (IPTR)MKBADDR(fh);
	    break;
	}

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
	kprintf("Filesystem action %i not handled yet in WaitPkt()\n",
		iofs->IOFS.io_Command);
	break;
    }

    return packet;
}
