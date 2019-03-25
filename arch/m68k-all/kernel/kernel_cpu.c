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

    D(bug("[Kernel] %s: Switching ThisTask=0x%p '%s'\n", __func__, task, task->tc_Node.ln_Name);)

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
        D(
            bug("[Kernel] %s: TASK QUEUE EMPTY!!\n", __func__);

            // Dump tasks ...
            if (SysBase->ThisTask)
            {
                bug("[Kernel] %s: ThisTask=0x%p '%s'\n", __func__, SysBase->ThisTask, SysBase->ThisTask->tc_Node.ln_Name);
                bug("[Kernel] %s:     tc_Flags = %x\n", __func__, SysBase->ThisTask->tc_Flags);
                bug("[Kernel] %s:     tc_State = %x\n", __func__, SysBase->ThisTask->tc_State);
                bug("[Kernel] %s:     tc_SigAlloc = %x\n", __func__, SysBase->ThisTask->tc_SigAlloc);
                bug("[Kernel] %s:     tc_SigWait = %x\n", __func__, SysBase->ThisTask->tc_SigWait);
                bug("[Kernel] %s:     tc_SigRecv = %x\n", __func__, SysBase->ThisTask->tc_SigRecv);
                bug("[Kernel] %s:     tc_IDNestCnt = %d\n", __func__, SysBase->ThisTask->tc_IDNestCnt);
                bug("[Kernel] %s:     tc_TDNestCnt = %d\n", __func__, SysBase->ThisTask->tc_TDNestCnt);
            }

            ForeachNode(&SysBase->TaskReady, task)
            {
                bug("[Kernel] %s: Ready Task @ 0x%p '%s'\n", __func__, task, task->tc_Node.ln_Name);
                bug("[Kernel] %s:     tc_Flags = %x\n", __func__, task->tc_Flags);
                bug("[Kernel] %s:     tc_State = %x\n", __func__, task->tc_State);
                bug("[Kernel] %s:     tc_SigAlloc = %x\n", __func__, task->tc_SigAlloc);
                bug("[Kernel] %s:     tc_SigWait = %x\n", __func__, task->tc_SigWait);
                bug("[Kernel] %s:     tc_SigRecv = %x\n", __func__, task->tc_SigRecv);
                bug("[Kernel] %s:     tc_IDNestCnt = %d\n", __func__, task->tc_IDNestCnt);
                bug("[Kernel] %s:     tc_TDNestCnt = %d\n", __func__, task->tc_TDNestCnt);
            }
            ForeachNode(&SysBase->TaskWait, task)
            {
                bug("[Kernel] %s: Waiting Task @ 0x%p '%s'\n", __func__, task, task->tc_Node.ln_Name);
                bug("[Kernel] %s:     tc_Flags = %x\n", __func__, task->tc_Flags);
                bug("[Kernel] %s:     tc_State = %x\n", __func__, task->tc_State);
                bug("[Kernel] %s:     tc_SigAlloc = %x\n", __func__, task->tc_SigAlloc);
                bug("[Kernel] %s:     tc_SigWait = %x\n", __func__, task->tc_SigWait);
                bug("[Kernel] %s:     tc_SigRecv = %x\n", __func__, task->tc_SigRecv);
                bug("[Kernel] %s:     tc_IDNestCnt = %d\n", __func__, task->tc_IDNestCnt);
                bug("[Kernel] %s:     tc_TDNestCnt = %d\n", __func__, task->tc_TDNestCnt);
            }
        )
        /* Break IDNestCnt */
        if (SysBase->IDNestCnt >= 0) {
            SysBase->IDNestCnt=-1;
            asm volatile ("move.w #0xc000,0xdff09a\n");
        }
        asm volatile ("stop #0x2000\n"); // Wait for an interrupt
    }

    D(bug("[Kernel] %s: Dispatching Task @ 0x%p '%s'\n", __func__, task, task->tc_Node.ln_Name);)

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
