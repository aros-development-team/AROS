/*
    Copyright (C) 1995-2015, The AROS Development Team. All rights reserved.

    Desc: Default trap handler
*/

#include <exec/alerts.h>
#include <exec/interrupts.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#include "etask.h"
#include "exec_intern.h"
#include "exec_util.h"

/* User-mode code where the task jumps to.
 * NOTE: This is only called on ETasks!
 */
static void Exec_CrashHandler(void)
{
    struct Task *ThisTask = GET_THIS_TASK;
    struct IntETask *iet = GetIntETask(ThisTask);

    /*
     * The task is not going back to where it crashed. If that was inside
     * Forbid() or Disable(), the nesting is dropped and interrupts are let
     * in again: otherwise its CPU could not switch tasks, or could not
     * hear the other CPUs, for as long as the requester is up.
     * Not if what crashed was supervisor code.
     */
    if (!KrnIsSuper())
    {
        TDNESTCOUNT_SET(-1);
        IDNESTCOUNT_SET(-1);
        KrnSti();
    }

    /* Makes Alert() attempting to bring up Intuition requester */
    iet->iet_AlertFlags &= ~AF_Alert;

    Alert(iet->iet_AlertCode);
}

/*
 * Where a task is sent when it has crashed and cannot be given the
 * requester: it crashed again while handling the first crash, or it is on
 * its way out of the system. The crash is reported on the debug output and
 * the task waits for ever. Its CPU carries on with the other tasks.
 */
static void Exec_CrashSuspend(void)
{
    struct Task *ThisTask = GET_THIS_TASK;
    struct IntETask *iet = GetIntETask(ThisTask);

    /*
     * The trap handler cannot tell whose code crashed. If we are here in
     * supervisor mode it was not the task's own, and that is not for a
     * task to sit out: post the alert the old way.
     */
    if (KrnIsSuper())
        Alert(iet->iet_AlertCode);

    Exec_SuspendCrashedTask(iet->iet_AlertCode, iet->iet_AlertLocation, SysBase);
}

static inline void Exec_TrapRedirect(struct ExceptionContext *ctx, void (*handler)(void))
{
    ctx->PC = (IPTR)handler;
#ifdef __x86_64__
    /* the value (%rsp + 8) is always a multiple of 16 when control is transferred to the
       function entry point */
    if ((ctx->rsp & 0x0F) == 0) ctx->rsp -= 8;
#endif
}

/* In original AmigaOS the trap handler is entered in supervisor mode with the
 * following on the supervisor stack:
 *  0(sp).l = trap#
 *  4(sp) Processor dependent exception frame
 * In our current implementation we use two pointers on stack. The idea is taken
 * from AmigaOS v4.
 *
 * For m68k, we need a thunk, since we will call this routine via
 * the AOS 1.x-3.x method described above.
 *
 * See arch/m68k-all/kernel/m68k_exception.c for implementation details.
 */
#ifdef __mc68000
asm (
        "       .text\n"
        "       .balign 2\n"
        "       .global Exec_TrapHandler\n"
        "Exec_TrapHandler:\n"
        "       movem.l %d0-%d7/%a0-%a6,%sp@-\n"   /* Save regs */
        "       move.l  %sp@(15*4),%d0\n"          /* Get trap code */
        "       move.l  %usp,%a0\n"          /* Get USP */
        "       move.l  %a0,%sp@(15*4)\n"    /* Fix up A7 to be USP */
        "       move.l  %sp,%sp@-\n"    /* Push on pointer to exception ctx */
        "       move.l  %d0,%sp@-\n"    /* Push on trap number */
        "       jsr Exec__TrapHandler\n" /* Call C routine */
        "       addq.l  #8,%sp\n"       /* Pop off C routine args */
        "       movem.l %sp@+,%d0-%d7/%a0-%a5\n"  /* Restore registers */
        "       move.l  %sp@(4), %a6\n"
        "       move.l  %a6,%usp\n"     /* Restore USP */
        "       move.l  %sp@+,%a6\n"    /* Restore A6 */
        "       addq.l  #4,%sp\n"       /* Skip past A7 */
        "       rte\n"                  /* Return from trap */
        );
void Exec__TrapHandler(ULONG trapNum, struct ExceptionContext *ctx)
#else
void Exec_TrapHandler(ULONG trapNum, struct ExceptionContext *ctx)
#endif
{
    struct Task *ThisTask = GET_THIS_TASK;

    /* Our situation is deadend */
    trapNum |= AT_DeadEnd;

    /*
     * We must have a valid ETask in order to be able
     * to display a requester in user mode.
     */
    if (ThisTask && (ThisTask->tc_Flags & TF_ETASK))
    {
        /* Get internal task structure */
        struct IntETask *iet = GetIntETask(ThisTask);

        if (iet->iet_AlertFlags & AF_Suspend)
        {
            /*
             * It crashed on the way to being suspended. Nothing more can
             * be tried in the task: the alert is posted from here, in
             * supervisor mode.
             */
            if (iet->iet_AlertFlags & AF_Alert)
                trapNum = iet->iet_AlertCode;
        }
        else if ((iet->iet_AlertFlags & AF_Alert) || (ThisTask->tc_State == TS_REMOVED))
        {
            /*
             * A second crash while the first one is being handled, or a
             * crash in a task that is being removed. The requester is out
             * of the question, but this is one task's trouble, not the
             * system's: it is sent to report the crash on the debug output
             * and to wait for ever, and its CPU stays in use. (A task that
             * another CPU is removing leaves the CPU as soon as it gets
             * there.)
             */
            if (!(iet->iet_AlertFlags & AF_Alert))
            {
                iet->iet_AlertType     = AT_CPU;
                iet->iet_AlertCode     = trapNum;
                iet->iet_AlertLocation = (APTR)ctx->PC;
                iet->iet_AlertStack    = (APTR)ctx->FP;
            }
            iet->iet_AlertFlags |= AF_Alert|AF_Location|AF_Suspend;

            Exec_TrapRedirect(ctx, Exec_CrashSuspend);
            return;
        }
        else
        {
            /*
             * Otherwise we can try to send the crash to user level.
             *
             * First mark crash condition, and also specify alert code for user-level handler.
             * If we double-crash while jumping to user mode, we will know this (ETF_Alert will
             * already be set)
             */
            iet->iet_AlertType     = AT_CPU;
            iet->iet_AlertFlags    = AF_Alert|AF_Location;
            iet->iet_AlertCode     = trapNum;
            iet->iet_AlertLocation = (APTR)ctx->PC;    /* Location is our PC, where we crashed */
            iet->iet_AlertStack    = (APTR)ctx->FP;    /* Remember also stack frame for backtrace */

            /*
             * This is a CPU alert. We've got a full CPU context, so we remember it
             * in our ETask structure for displaying to the user later.
             * Note that we store only GPR part of the context. We don't copy
             * attached FPU data (if any). This can be considered TODO.
             */
            CopyMem(ctx, ETaskAlertData(iet), sizeof(struct ExceptionContext));

            /*
             * Make the task to jump to crash handler. We don't care about return address etc because
             * the alert is deadend anyway.
             */
            Exec_TrapRedirect(ctx, Exec_CrashHandler);

            /* Let the task go */
            return;
        }
    }

    Exec_ExtAlert(trapNum, (APTR)ctx->PC, (APTR)ctx->FP, AT_CPU, ctx, SysBase);
} /* TrapHandler */
