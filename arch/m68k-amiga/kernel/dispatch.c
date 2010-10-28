#include <aros/kernel.h>

#include <exec/alerts.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_syscall.h>
#include <kernel_debug.h>

#if defined(DEBUG) && (DEBUG == 1)
#define D(x) x
#else
#define D(x)
#endif

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH0(void, KrnDispatch,

/*  SYNOPSIS */

/*  LOCATION */
         struct KernelBase *, KernelBase, 4, Kernel)

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
    	asm volatile ("move #0x2700, %sr\n");	// Disable CPU interrupts
    	next = (struct Task *)RemHead(&SysBase->TaskReady);
    	if (next != NULL)
    		break;
    	SysBase->IdleCount++;
    	SysBase->AttnFlags |= ARF_AttnSwitch;
    	D(bug("-- IDLE HALT --\n"));
    	asm volatile ("stop #0x2000\n"); // Wait for an interrupt
    }

    D(bug(" Dispatch (%s) Task=%p, SP=%p (0x%04x, %p), TDnc=%d, IDnc=%d\n",
    	 next->tc_Node.ln_Name, next, next->tc_SPReg, *(UWORD *)(next->tc_SPReg - 6),
    	 *(ULONG *)(next->tc_SPReg - 4),
    	 next->tc_TDNestCnt,next->tc_IDNestCnt));

    SysBase->DispCount++;

    SysBase->IDNestCnt = next->tc_IDNestCnt;
    SysBase->ThisTask = next;
    SysBase->Elapsed = SysBase->Quantum;
    SysBase->SysFlags &= ~SFF_QuantumOver;
    next->tc_State = TS_RUN;

    if (SysBase->IDNestCnt < 0)
    	    KrnSti();
    else
    	    KrnCli();

    if (next->tc_Flags & TF_LAUNCH) {
    	    D(bug("%s:%d task->Launch called for %p\n", __func__, __LINE__, next->tc_Launch));
    	    AROS_UFC0(void, next->tc_Launch);
    }

    if (next->tc_Flags & TF_EXCEPT) {
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
     * be in Switch(), which can only be called from
     * Supervisor mode.
     */
    asm volatile (
    	"    move.l  %0,%%usp\n"
    	"    btst    #0,%1\n"			// Are we a 68010/68020?
    	"    beq.s   0f\n"			// Nope!
    	"    move.w  #0x0020,%%sp@-\n"		// Yep!
    	"0:\n"
    	"    move.l  %0@(-(4)),%%sp@-\n"	// Return PC
    	"    move.w  %0@(-(4+2)),%%sp@-\n"	// %sr
    	"    movem.l %0@(-(4+2+15*4)),%%d0-%%d7/%%a0-%%a6\n" // restore everything
    	"    rte\n"
    	:
    	: "a" (next->tc_SPReg),
    	  "r" ((SysBase->AttnFlags & (AFB_68010 | AFB_68020)) ? 1 : 0)
    	: );

    /* NOTE: We should never get here! */
    D(bug("[KrnDispatch] Oh noes! Unpossible code executed!\n"));

    AROS_LIBFUNC_EXIT
}
