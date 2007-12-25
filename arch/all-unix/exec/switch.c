/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Unix version  of Switch().
    Lang: english
*/

#include <exec/execbase.h>
#include <proto/exec.h>

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

    Disable();
    
    if( this->tc_State != TS_RUN )
    {
#if 0
	if( SysBase->IDNestCnt >= 0 )
	{
		sigprocmask(SIG_UNBLOCK, &sig_int_mask, NULL);
	}
#endif
	sigset_t temp_sig_int_mask;
	
	sigemptyset(&temp_sig_int_mask);	
	sigaddset( &temp_sig_int_mask, SIGUSR1);
 		
	/*      Its quite possible that they have interrupts Disabled(),
		we should fix that here, otherwise we can't switch. 

		We can't call the dispatcher because we need a signal,
	    	lets just create one.

		Have to set the dispatch-required flag.
		I use SIGUSR1 (maps to SoftInt) because it has less effect on
		the system clock, and is probably quicker.
	*/

	sigprocmask(SIG_UNBLOCK, &temp_sig_int_mask, NULL);
	SysBase->AttnResched |= 0x8000;
	kill(getpid(), SIGUSR1);
	sigprocmask(SIG_BLOCK, &temp_sig_int_mask, NULL);

    }

    Enable();
    
    AROS_LIBFUNC_EXIT
} /* Switch() */
