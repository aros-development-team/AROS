/*
    Copyright (C) 1995-1997 AROS - The Amiga Research OS
    $Id$

    Desc: Unix version  of Switch().
    Lang: english
*/

#include <exec/execbase.h>
#include <proto/exec.h>
#include <exec_pdefs.h>

#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

extern sigset_t sig_int_mask;

AROS_LH0(void, Switch,
    struct ExecBase *, SysBase, 9, Exec)
{
    AROS_LIBFUNC_INIT

    struct Task *this = SysBase->ThisTask;

    /*
	If the state is not TS_RUN then the task is already in a list
    */

    if( (this->tc_State != TS_RUN)
		&& !(this->tc_Flags & TF_EXCEPT) )
    {
	if( SysBase->IDNestCnt >= 0 )
	{
		sigprocmask(SIG_UNBLOCK, &sig_int_mask, NULL);
	}
		
	/*      Its quite possible that they have interrupts Disabled(),
		we should fix that here, otherwise we can't switch. 

		We can't call the dispatcher because we need a signal,
	    	lets just create one.

		Have to set the dispatch-required flag.
		I use SIGUSR1 (maps to SoftInt) because it has less effect on
		the system clock, and is probably quicker.
	*/

	SysBase->AttnResched |= 0x8000;
this->tc_Flags |= 2;
//kprintf("Calling kill!\n");
	kill(getpid(), SIGUSR1);

while (0 != (this->tc_Flags & 2))
{
  int dummy=0;
  dummy++;
}
//kprintf("Running again!\n");
    }

    AROS_LIBFUNC_EXIT
} /* Switch() */
