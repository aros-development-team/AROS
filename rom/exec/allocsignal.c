/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Allocate a signal
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(BYTE, AllocSignal,

/*  SYNOPSIS */
	AROS_LHA(LONG, signalNum, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 55, Exec)

/*  FUNCTION
	Allocate a given signal out of the current task's pool of signals.
	Every task has a set of signals to communicate with other tasks.
	Half of them are reserved for the system and half of them is
	free for general use. Some of the reserved signals (e.g.
	SIGBREAKF_CTRL_C) have a defined behaviour and may be used by user
	code, however.

    INPUTS
	signalNum - Number of the signal to allocate or -1 if any signal
		    will do.

    RESULT
	Number of the signal or -1 if the signal couldn't be allocated.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	FreeSignal(), Signal(), Wait()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *ThisTask;
    ULONG *mask;
    ULONG mask1;

    /* Protect signal mask against possible task exceptions. */
    Forbid();

    ThisTask = FindTask(NULL);

    /* Get pointer to mask of allocated signal */
    mask=&ThisTask->tc_SigAlloc;

    /* Get signal */
    if(signalNum<0)
    {
	/* Any signal will do. */

	/*
	 * To get the last nonzero bit in a number I use a&~a+1:
	 * Given a number that ends with a row of zeros  xxxx1000
	 * I first toggle all bits in that number	 XXXX0111
	 * then add 1 to toggle all but the last 0 again XXXX1000
	 * and AND this with the original number	 00001000
	 *
	 * And since ~a+1=-a I can use a&-a instead.
	 *
	 * And to get the last zero bit I finally use ~a&-~a.
	 */
	mask1=~*mask&-~*mask;

	/* Got a bit? */
	if(mask1)
	{
	    /* Allocate and reset the bit */
	    *mask|=mask1;
	    ThisTask->tc_SigRecvd  &= ~mask1;
	    ThisTask->tc_SigExcept &= ~mask1;
	    ThisTask->tc_SigWait   &= ~mask1;

	    /* And get the bit number */
	    signalNum=(mask1&0xffff0000?16:0)+(mask1&0xff00ff00?8:0)+
		      (mask1&0xf0f0f0f0? 4:0)+(mask1&0xcccccccc?2:0)+
		      (mask1&0xaaaaaaaa? 1:0);
	}
    }else
    {
	/* Get a specific signal */
	mask1=1<<signalNum;

	/* Check if signal is free */
	if(*mask&mask1)
	    /* No. Return */
	    signalNum=-1;
	else
	{
	    /* It is free. Allocate and reset it. */
	    *mask|=mask1;
	    ThisTask->tc_SigRecvd  &= ~mask1;
	    ThisTask->tc_SigExcept &= ~mask1;
	    ThisTask->tc_SigWait   &= ~mask1;
	}
    }

    Permit();

    return signalNum;
    AROS_LIBFUNC_EXIT
}

