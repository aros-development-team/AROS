/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"

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
    
    ReplyMsg(msg);
    
    /* TODO: Convert the whole thing back to a DosPacket */
    
    return (struct DosPacket *)msg;
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
