/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1997/01/01 03:46:08  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.5  1996/12/10 13:51:42  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.4  1996/10/24 15:50:46  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.3  1996/08/01 17:41:08  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

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

    HISTORY

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
	if(sb!=-1)
	{
	    /* Initialize messageport structure. First set signal bit. */
	    ret->mp_SigBit=sb;

	    /* Clear the list of messages */
	    ret->mp_MsgList.lh_Head=(struct Node *)&ret->mp_MsgList.lh_Tail;
	    /* ret->mp_MsgList.lh_Tail=NULL; */
	    ret->mp_MsgList.lh_TailPred=(struct Node *)&ret->mp_MsgList.lh_Head;

	    /* Set port to type 'signalling' */
	    ret->mp_Flags=PA_SIGNAL;

	    /* Finally set task to send the signal to. */
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

