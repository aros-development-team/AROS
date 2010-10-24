#include <aros/kernel.h>

#include <exec/execbase.h>
#include <defines/kernel.h>

#include "exec_intern.h"

/*****************************************************************************

    NAME */
#include <proto/exec.h>

/*  NAME */

	AROS_LH0(void, Dispatch,

/*  LOCATION */
         struct ExecBase *, SysBase, 10, Exec)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *next;

    for (;;) { 
    	KrnCli();
    	next = (struct Task *)RemHead(&SysBase->TaskReady);
    	if (next != NULL)
    		break;
    	SysBase->IdleCount++;
    	SysBase->AttnFlags |= ARF_AttnSwitch;
    	D(bug("-- IDLE HALT --\n"));
    	asm volatile ("stop #0x2000\n"); // Wait for an interrupt
    }

    SysBase->DispCount++;

    SysBase->ThisTask = next;
    SysBase->Quantum = SysBase->Elapsed;
    next->tc_State = TS_RUN;

    SysBase->IDNestCnt = next->tc_IDNestCnt;
    if (SysBase->IDNestCnt < 0)
    	    KrnSti();
    else
    	    KrnCli();

    if (next->tc_Flags & TB_LAUNCH) {
    	    D(bug("%s:%d task->Launch called for %p\n", __func__, __LINE__, next->tc_Launch));
    	    AROS_UFC0(void, next->tc_Launch);
    }
    if (next->tc_Flags & TB_EXCEPT) {
    	    D(bug("%s:%d task exception - what to do?\n"));
    	    extern int breakpoint(void); breakpoint();

	    /* Call Exec/Exception to handle the exception */
    	    Exception();
#if 0
	    /* Some MAGIC goes here, I guess, to get back to
	     * the user process?
	     */
#endif
    }

    /* Copy from the user stack to the supervisor stack,
     * then 'rte' to the original frame, which should
     * be Switch().
     */
    asm volatile (
    	"move.l %0,%%usp\n"
    	"move.w %0@+,%%sp@-\n"		// %sr
    	"move.l %0@+,%%sp@-\n"		// Return PC
    	"movem.l (%0),%%d0-%%d7/%%a0-%%a6\n"
    	"rte\n"
    	:
    	: "a" (next->tc_SPReg)
    	: );

    /* NOTE: We should never get here! */
    AROS_LIBFUNC_EXIT
}
