/*
    Copyright � 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Unix version  of Switch().
    Lang: english
*/

#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <aros/kernel.h>
#include <proto/arossupport.h>

AROS_LH0(void, Switch,
    struct ExecBase *, SysBase, 9, Exec)
{
    AROS_LIBFUNC_INIT

    struct Task *this = SysBase->ThisTask;

  KRNWireImpl(SoftEnable);
  KRNWireImpl(SoftDisable);
  KRNWireImpl(SoftCause);

	/*
	If the state is not TS_RUN then the task is already in a list
    */

    Disable();
    
    if( this->tc_State != TS_RUN )
    {

	/*      Its quite possible that they have interrupts Disabled(),
		we should fix that here, otherwise we can't switch. 

		We can't call the dispatcher because we need a signal,
	    	lets just create one.

		Have to set the dispatch-required flag.
		I use SIGUSR1 (maps to SoftInt) because it has less effect on
		the system clock, and is probably quicker.
	*/

	  /* We now re-enable software interrupts. */
	  CALLHOOKPKT(krnSoftEnableImpl,0,0);
	SysBase->AttnResched |= 0x8000;
	  CALLHOOKPKT(krnSoftCauseImpl,0,0);
	  /* Disable software interrupts */
	  CALLHOOKPKT(krnSoftDisableImpl,0,0);
	  
    }

    Enable();
    
    AROS_LIBFUNC_EXIT
} /* Switch() */
