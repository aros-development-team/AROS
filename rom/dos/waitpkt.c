/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

#include "dos_intern.h"
#include <dos/filesystem.h>
#include <proto/exec.h>

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

    HISTORY

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
 
    if (me->pr_Task.tc_Node.ln_Type == NT_PROCESS)
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

	/* This corresponds to ACTION_FINDINPUT, ACTION_FINDOUTPUT,
	   ACTION_FINDUPDATE which fortunately have the same return values */
    case FSA_OPEN:
	packet->dp_Res1 = iofs->io_DosError == 0;
 	packet->dp_Res2 = iofs->io_DosError;
	break;

    case FSA_READ:
    case FSA_WRITE:
	packet->dp_Res1 = (IPTR)iofs->io_Union.io_READ_WRITE.io_Length;
	packet->dp_Res2 = iofs->io_DosError;
	break;

    case FSA_CLOSE:
	packet->dp_Res1 = iofs->io_DosError == 0;
	break;


    case FSA_EXAMINE:
    case FSA_EXAMINE_NEXT:
    case FSA_SET_FILE_SIZE:
    case FSA_DELETE_OBJECT:
    case FSA_RENAME:
    case FSA_SET_PROTECT:
    case FSA_SET_COMMENT:
    case FSA_SET_DATE:
    case FSA_FORMAT:
    case FSA_IS_FILESYSTEM:
	packet->dp_Res1 = iofs->io_DosError == 0;
 	packet->dp_Res2 = iofs->io_DosError;
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
	    packet->dp_Res1 = MKBADDR(fh);
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
    case FSA_ADD_NOTIFY:
    case FSA_REMOVE_NOTIFY:
    case FSA_LOCK_RECORD:
    case FSA_UNLOCK_RECORD:
    case FSA_READ_SOFTLINK:
    case FSA_FILE_MODE:
	kprintf("Filesystem action %i not handled yet in WaitPkt()\n",
		iofs->IOFS.io_Command);
	break;
    }



    
    ReplyMsg(msg);

    return packet;
}

#if 0
    /*
    ** The Result code 1 is most of the time a DOS boolean
    ** but unfortunately this is not always true...
    */

#warning do_Res1 is always DOSTRUE!
    dp->dp_Res1 = DOSTRUE;
    dp->dp_Res2 = iofs->io_DosError;
    SetIoErr(iofs->io_DosError);
    
    FreeMem(iofs, sizeof(struct IOFileSys));    
    
    return dp;

#endif
