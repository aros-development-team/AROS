/*
 * M68K Schedule functions
 */

#include <exec/execbase.h>
#include <exec/alerts.h>
#include <proto/exec.h>
#include <defines/kernel.h>

#include <etask.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_scheduler.h>

#ifndef D
# ifdef DEBUG
#  define D(x) x
# else
#  define D(x)
# endif
#endif

extern void cpu_Exception(void);
asm (
	"	.text\n"
	"	.align 4\n"
	"	.globl cpu_Exception\n"
	"cpu_Exception:\n"
	"	movem.l	%d0-%d1/%a0-%a1/%a6,%sp@-\n"
	"	move.l	(4),%a6\n"
	"	jsr	%a6@(-1 * 6 * 11 /* Exception */)\n"
	"	movem.l	%sp@+,%d0-%d1/%a0-%a1/%a6\n"
	"	rts\n"
);

void cpu_Switch(regs_t *regs)
{
    struct Task *task = SysBase->ThisTask;
    struct AROSCPUContext *ctx = GetIntETask(task)->iet_Context;

    /* Actually save the context */
    CopyMem(regs, ctx, sizeof(regs_t));

    /* Update tc_SPReg */
    task->tc_SPReg = (APTR)regs->a[7];

    core_Switch();
}

void cpu_Dispatch(regs_t *regs)
{
    struct Task *task;
    struct AROSCPUContext *ctx;

    for (;;) {
        asm volatile ("move #0x2700, %sr\n");    // Disable CPU interrupts

        task = core_Dispatch();
        if (task != NULL)
            break;
        D(bug("-- IDLE HALT --\n"));

	/* Break IDNestCnt */
	if (SysBase->IDNestCnt >= 0) {
	    SysBase->IDNestCnt=-1;
	    KrnSti();
	}
        asm volatile ("stop #0x2000\n"); // Wait for an interrupt
	/* Ok, I don't like this, but we lose
	 * VBLANK interrupts on UAE if we don't add some
	 * delay after we come out of the STOP.
	 */
	{ volatile int i; for (i = 0; i < 1000; i++); }
    }

    ctx = GetIntETask(task)->iet_Context;
    CopyMem(ctx, regs, sizeof(regs_t));
    regs->a[7] = (IPTR)task->tc_SPReg;

    /* Re-enable interrupts if needed */
    if (SysBase->IDNestCnt < 0)
        KrnSti();
    else
        KrnCli();

    if (task->tc_Flags & TF_EXCEPT) {
    	/* Exec_Exception() will Enable() */
    	Disable();

	/* Manipulate the current CPU context so Exec_Exception gets
	 * executed after we leave Supervisor mode.
	 */
    	task->tc_SPReg -= sizeof(ULONG);	/* RTS to original PC */
    	if (task->tc_SPReg <= task->tc_SPLower)
    		Alert(AT_DeadEnd|AN_StackProbe);
    	*(ULONG *)(task->tc_SPReg) = regs->pc;

	regs->a[7] = (IPTR)task->tc_SPReg;
	regs->pc   = (IPTR)cpu_Exception;
    }
}
