#include <aros/kernel.h>

#include <exec/alerts.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_syscall.h>

#if defined(DEBUG) && (DEBUG == 1)
#define D(x) x
#else
#define D(x)
#endif

/* Quick little Exec/Supervisor handler to call into Dispatch */
extern void KrnSwitch_Dispatch(void);
asm  (
	"    .text\n"
	"    .align 4\n"
	"    .globl KrnSwitch_Dispatch\n"
	"KrnSwitch_Dispatch:\n"
	"    move.l %a6,%sp@-\n"
	"    move.l (0x4),%a6\n"
	"    jsr Kernel_KrnDispatch\n"
	"    rte\n"
	);

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH0(void, KrnSwitch,

/*  SYNOPSIS */

/*  LOCATION */
         struct KernelBase *, KernelBase, 5, Kernel)

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

    struct Task *task = SysBase->ThisTask;
    register volatile is_return asm("%d0");

    asm volatile ("or.w #0x2300,%sr\n");	// Disable CPU interrupts

    /* The order of operations here must match the
     * stack popping action of Dispatch()
     *
     *       (sp-(6+15*4)) %d0-%d7/%a0-%a6
     *            (sp-6) %sr
     *            (sp-4) Kernel_KrnSwitch_Restore
     * task->tc_SPReg => 
     *                   user data
     */
    asm volatile (
	"  move.l   %%a0,  %%sp@-\n"			// Save A0 to stack
	"  move.l   %%usp, %%a0\n"			// Use the user stack
        "  move.l   #Kernel_KrnSwitch_Restore,%%a0@-\n"	// Where to return to
        "  move.w   %%sr,  %%a0@-\n"			// Save %sr
        "  move.l   #1, %%d0\n"                         // Indicate when we come back
        "  movem.l  %%d0-%%d7/%%a0-%%a6, %%a0@-\n"	// Save everything
        "  move.l   %%sp@, %%a0@(4*8)\n"		// Fix up A0 in the saved regs
        "  move.l   %%sp@+, %%a0\n"                     // Restore A0
        "  move.l   %%usp,  %0\n"			// Save user stack
        "  move.l   #0, %%d0\n"				// Indicate noreturn
    	"Kernel_KrnSwitch_Restore:\n"

        : "=a" (task->tc_SPReg), "=r" (is_return)
        :
        : );

    if (is_return)
    	    return;

    /* Reset Enable()/Disable() locking */
    task->tc_IDNestCnt = SysBase->IDNestCnt;
    SysBase->IDNestCnt = -1;
    asm volatile ("move.w #0xc000,0xdff09a\n");	// Enable device interrupts

    if (task->tc_Flags & TB_SWITCH)
    	    AROS_UFC0(void, task->tc_Switch);

    Supervisor(KrnSwitch_Dispatch);	/* We don't return from this! */

    D(bug("Kernel_KrnSwitch: --- IMPOSSIBLE EXECUTION --\n"));

    AROS_LIBFUNC_EXIT
}
