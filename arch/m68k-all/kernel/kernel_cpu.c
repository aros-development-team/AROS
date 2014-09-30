/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * M68K Schedule functions
 */

#include <exec/execbase.h>
#include <exec/alerts.h>
#include <proto/exec.h>
#include <defines/kernel.h>

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
        "       .text\n"
        "       .align 4\n"
        "       .globl cpu_Exception\n"
        "cpu_Exception:\n"
        "       movem.l %d0-%d1/%a0-%a1/%a6,%sp@-\n"
        "       move.l  (4),%a6\n"
        "       jsr     %a6@(-1 * 6 * 11 /* Exception */)\n"
        "       movem.l %sp@+,%d0-%d1/%a0-%a1/%a6\n"
        "       rts\n"
);

void cpu_Switch(regs_t *regs)
{
    struct Task *task = SysBase->ThisTask;
    struct AROSCPUContext *ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;

    /* Actually save the context */
    CopyMem(regs, &ctx->cpu, sizeof(regs_t));

    /* If we have an FPU, save the FPU context */
    if (SysBase->AttnFlags & AFF_FPU)
            AROS_UFC2NR(void, FpuSaveContext,
                            AROS_UFCA(struct FpuContext *, &ctx->fpu, A0),
                            AROS_UFCA(UWORD, (SysBase->AttnFlags & AFF_68060) ? 2 : 0, D0));

    /* Update tc_SPReg */
    task->tc_SPReg = (APTR)regs->a[7];

    core_Switch();
}

void cpu_Dispatch(regs_t *regs)
{
    struct Task *task;
    struct AROSCPUContext *ctx;

    for (;;) {
        asm volatile ("ori  #0x0700, %sr\n");    // Disable CPU interrupts

        task = core_Dispatch();
        if (task != NULL)
            break;
        D(bug("-- IDLE HALT --\n"));

        /* Break IDNestCnt */
        if (SysBase->IDNestCnt >= 0) {
            SysBase->IDNestCnt=-1;
            asm volatile ("move.w #0xc000,0xdff09a\n");
        }
        asm volatile ("stop #0x2000\n"); // Wait for an interrupt
    }

    ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;
    CopyMem(&ctx->cpu, regs, sizeof(regs_t));
    regs->a[7] = (IPTR)task->tc_SPReg;

    /* If we have an FPU, restore the FPU context */
    if (SysBase->AttnFlags & AFF_FPU)
            AROS_UFC2NR(void, FpuRestoreContext,
                            AROS_UFCA(struct FpuContext *, &ctx->fpu, A0),
                            AROS_UFCA(UWORD, (SysBase->AttnFlags & AFF_68060) ? 2 : 0, D0));

    /* Re-enable interrupts if needed */
    if (SysBase->IDNestCnt < 0) {
        asm volatile ("move.w #0xc000,0xdff09a\n");
    } else {
        asm volatile ("move.w #0x4000,0xdff09a\n");
    }

    if (task->tc_Flags & TF_EXCEPT) {
        /* Exec_Exception() will Enable() */
        Disable();

        /* Manipulate the current CPU context so Exec_Exception gets
         * executed after we leave Supervisor mode.
         */
        task->tc_SPReg -= sizeof(ULONG);        /* RTS to original PC */
        if (task->tc_SPReg <= task->tc_SPLower)
                Alert(AT_DeadEnd|AN_StackProbe);
        *(ULONG *)(task->tc_SPReg) = regs->pc;

        regs->a[7] = (IPTR)task->tc_SPReg;
        regs->pc   = (IPTR)cpu_Exception;
    }
}
