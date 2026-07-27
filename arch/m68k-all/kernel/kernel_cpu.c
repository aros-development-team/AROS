/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    The assembly scheduler entry saves and restores the integer context
    directly on the task's user stack; this file retains the common scheduler
    policy and optional CPU-context handling.
*/

#include <exec/execbase.h>
#include <exec/alerts.h>
#include <proto/exec.h>
#include <defines/kernel.h>

#include <kernel_base.h>
#include <kernel_scheduler.h>

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

void m68k_SwitchTail(APTR frame)
{
    struct Task *task = SysBase->ThisTask;
    struct AROSCPUContext *ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;

    if (SysBase->AttnFlags & AFF_FPU)
        AROS_UFC2NR(void, FpuSaveContext,
            AROS_UFCA(struct FpuContext *, &ctx->fpu, A0),
            AROS_UFCA(UWORD, (SysBase->AttnFlags & AFF_68060) ? 2 : 0, D0));

    if (SysBase->AttnFlags & AFF_68080)
        AROS_UFC1NR(void, AMMXSaveContext,
            AROS_UFCA(struct AMMXContext *, &ctx->ammx, A0));

    task->tc_SPReg = frame;
    core_Switch();
}

APTR m68k_DispatchFrame(void)
{
    struct Task *task;
    struct AROSCPUContext *ctx;
    UBYTE *frame;

    for (;;) {
        asm volatile ("ori #0x0700, %sr\n");

        task = core_Dispatch();
        if (task != NULL)
            break;

        if (SysBase->IDNestCnt >= 0) {
            SysBase->IDNestCnt = -1;
            asm volatile ("move.w #0xc000,0xdff09a\n");
        }
        asm volatile ("stop #0x2000\n");
    }

    ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;
    if (SysBase->AttnFlags & AFF_FPU)
        AROS_UFC2NR(void, FpuRestoreContext,
            AROS_UFCA(struct FpuContext *, &ctx->fpu, A0),
            AROS_UFCA(UWORD, (SysBase->AttnFlags & AFF_68060) ? 2 : 0, D0));

    if (SysBase->AttnFlags & AFF_68080)
        AROS_UFC1NR(void, AMMXRestoreContext,
            AROS_UFCA(struct AMMXContext *, &ctx->ammx, A0));

    if (SysBase->IDNestCnt < 0)
        asm volatile ("move.w #0xc000,0xdff09a\n");
    else
        asm volatile ("move.w #0x4000,0xdff09a\n");

    frame = task->tc_SPReg;
    if (task->tc_Flags & TF_EXCEPT) {
        ULONG originalPC = *(ULONG *)frame;
        UBYTE *exceptionFrame = frame - sizeof(ULONG);
        unsigned int i;

        /* Exec_Exception() balances this Disable() when it returns. */
        Disable();

        if (exceptionFrame <= (UBYTE *)task->tc_SPLower)
            Alert(AT_DeadEnd | AN_StackProbe);

        /*
         * Make room below the 66-byte task-stack frame.  The destination is
         * lower than the source, so a forward byte copy is overlap-safe.
         * Restoring this shifted frame sets USP to the word immediately after
         * it; place the interrupted PC there for cpu_Exception's final RTS.
         */
        for (i = 0; i < 66; i++)
            exceptionFrame[i] = frame[i];
        *(ULONG *)(exceptionFrame + 66) = originalPC;
        *(ULONG *)exceptionFrame = (ULONG)cpu_Exception;
        task->tc_SPReg = exceptionFrame;
        frame = exceptionFrame;
    }

    return frame;
}
