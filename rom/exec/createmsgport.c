/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create a new message port.
    Lang: english
*/

#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include "exec_util.h"

/*****************************************************************************

    NAME */

	AROS_LH0(struct MsgPort *, CreateMsgPort,

/*  SYNOPSIS */

/*  LOCATION */
	struct ExecBase *, SysBase, 111, Exec)

/*  FUNCTION
	Create a new message port. A signal will be allocated and the message
	port set to signal you task

    INPUTS

    RESULT
	Pointer to messageport structure

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MsgPort *ret;

    /* Allocate memory for struct MsgPort */
    ret=(struct MsgPort *)AllocMem(sizeof(struct MsgPort),MEMF_PUBLIC|MEMF_CLEAR);
    if(ret!=NULL)
    {
	BYTE sb;

	/* Allocate a signal bit */
	sb=AllocSignal(-1);
	if (sb != -1)
	{
	    /* Initialize messageport structure */
	    InitMsgPort(ret);
	    /* Set signal bit. */
	    ret->mp_SigBit=sb;
	    /* Set task to send the signal to. */
	    ret->mp_SigTask=SysBase->ThisTask;

	    /* Now the port is ready for use. */
	    return ret;
	}
	/* Couldn't get the signal bit. Free the memory. */
	FreeMem(ret,sizeof(struct MsgPort));
    }
    /* function failed */
    return NULL;
    AROS_LIBFUNC_EXIT
} /* CreateMsgPort */

