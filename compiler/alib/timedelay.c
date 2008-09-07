/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: TimeDelay() - wait a specified time.
    Lang: english
*/


#include <exec/types.h>
#include <aros/asmcall.h>
#include <devices/timer.h>
#include <exec/tasks.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
	AROS_UFH3(ULONG, TimeDelay,

/*  SYNOPSIS */
	AROS_UFHA(LONG, Unit, D0),
	AROS_UFHA(ULONG, Seconds, D1),
	AROS_UFHA(ULONG, MicroSeconds, D2))

/*  FUNCTION
	TimeDelay() waits for the specified period of time before returning
	to the caller.

    INPUTS
	Unit         -  The timer.device unit to use for this command.
	Seconds      -  The number of seconds to wait.
	MicroSeconds -  The number of microseconds to wait.

    RESULT
	Zero if everything went ok, non-zero if there was a problem.

    NOTES
	If this function fails, the most likely reasons are:
	- invalid timer.device unit numbers

	This function uses the SIGF_SINGLE signal, strange things can
	happen if you are waiting on this signal when you call this
	function. Basically: Don't use it and call this function.

    EXAMPLE

    BUGS

    SEE ALSO
	timer.device/TR_ADDREQUEST

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_USERFUNC_INIT

    struct timerequest tr;
    struct MsgPort mp;
    UBYTE error = 0;

    /* Create a message port */
    mp.mp_Node.ln_Type = NT_MSGPORT;
    mp.mp_Node.ln_Pri = 0;
    mp.mp_Node.ln_Name = NULL;
    mp.mp_Flags = PA_SIGNAL;
    mp.mp_SigTask = FindTask(NULL);
    mp.mp_SigBit = SIGB_SINGLE;
    NEWLIST(&mp.mp_MsgList);

    tr.tr_node.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    tr.tr_node.io_Message.mn_Node.ln_Pri = 0;
    tr.tr_node.io_Message.mn_Node.ln_Name = NULL;
    tr.tr_node.io_Message.mn_ReplyPort = &mp;
    tr.tr_node.io_Message.mn_Length = sizeof(struct timerequest);

    SetSignal(0, SIGF_SINGLE);

    if(OpenDevice("timer.device", Unit, (struct IORequest *)&tr, 0) == 0)
    {
	tr.tr_node.io_Command = TR_ADDREQUEST;
	tr.tr_node.io_Flags = 0;
	tr.tr_time.tv_secs = Seconds;
	tr.tr_time.tv_micro = MicroSeconds;
		
	DoIO((struct IORequest *)&tr);

	CloseDevice((struct IORequest *)&tr);
	error = 1;
    }

    return error;

    AROS_USERFUNC_EXIT

} /* TimeDelay() */
