/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Initialize the interface to the "hardware".
    Lang: english
*/

#include <exec/types.h>
#include <exec/interrupts.h>
#include <exec/execbase.h>
#include <exec/alerts.h>
#include <aros/asmcall.h>
#include <aros/atomic.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <hardware/intbits.h>

#define timeval sys_timeval
#include "etask.h"

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intr.h"
#include "kernel_intern.h"
#include "kernel_interrupts.h"
#include "kernel_scheduler.h"
#include "kernel_timer.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#undef timeval

#include <proto/exec.h>

#ifdef SIGCORE_NEED_SA_SIGINFO
#define SETHANDLER(sa, h) 			\
    sa.sa_sigaction = (void *)h ## _gate;	\
    sa.sa_flags |= SA_SIGINFO
#else
#define SETHANDLER(sa, h) 			\
    sa.sa_handler = (SIGHANDLER_T)h ## _gate;
#endif

sigset_t sig_int_mask;	/* Mask of signals that Disable() block */
unsigned int supervisor;

static void core_Trap(int sig, regs_t *regs)
{
    void (*trapHandler)(unsigned long, regs_t *) = NULL;
    struct SignalTranslation *s;
    struct AROSCPUContext ctx;

    /* Just for completeness */
    krnRunIRQHandlers(sig);

    bug("[KRN] Trap signal %d, SysBase 0x%p, KernelBase 0x%p\n", sig, SysBase, KernelBase);

    /* Find out trap handler for caught task */
    if (SysBase)
    {
	struct Task *t = SysBase->ThisTask;

        if (t)
	{
	    bug("[KRN] %s 0x%p (%s)\n", t->tc_Node.ln_Type == NT_TASK ? "Task":"Process", t, t->tc_Node.ln_Name ? t->tc_Node.ln_Name : "--unknown--");
	    trapHandler = t->tc_TrapCode;
	}
	else
	    bug("[KRN] No task\n");

	if (!trapHandler)
	    trapHandler = SysBase->TaskTrapCode;
    }

    PRINT_SC(regs);

    /* Translate UNIX trap number to CPU and exec trap numbers */
    for (s = sigs; s->sig != -1; s++)
    {
	if (sig == s->sig)
	    break;
    }

    /*
     * Trap handler expects struct AROSCPUContext, but regs_t
     * differs from it. Well, we could pass raw regs_t, but in
     * future we are going to make struct AROSCPUContext public,
     * so this won't do.
     */
    SAVEREGS(&ctx, regs);

    if (s->CPUTrap != -1)
    {
	if (krnRunExceptionHandlers(s->CPUTrap, &ctx))
	    /* Do not call exec trap handler */
	    trapHandler = NULL;
    }

    if (trapHandler && (s->AmigaTrap != -1))
	/* Call our trap handler. Note that we may return, this means that the handler has
	   fixed the problem somehow and we may safely continue */
	trapHandler(s->AmigaTrap, &ctx);

    /* Trap handler(s) have possibly modified the context, so
       we convert it back before returning */
    RESTOREREGS(&ctx, regs);
}

/*
 * We do not have enough user-defined signals to map
 * them to four required syscalls (CAUSE, SCHEDULE,
 * SWITCH, DISPATCH). We get around this by assigning
 * CAUSE to SIGUSR2 and others to SIGUSR1.
 *
 * What action is to be taken upon SIGUSR1 can be
 * figured out by looking at caller task's state.
 * exec.library calls KrnDispatch() only after task
 * has been actually disposed, with state set to TS_REMOVED.
 * Similarly, KrnSwitch() is called only in Wait(), after
 * setting state to TS_WAIT. In other cases it's KrnSchedule().
 */
static void core_SysCall(int sig, regs_t *sc)
{
    struct Task *task = SysBase->ThisTask;
    struct AROSCPUContext *ctx;

    AROS_ATOMIC_INC(supervisor);

    krnRunIRQHandlers(sig);

    switch(task->tc_State)
    {
    /* A running task needs to be put into TaskReady list first. It's SC_SCHEDULE. */
    case TS_RUN:
	if (!core_Schedule())
	    break;

    /* If the task is already in some list with appropriate state, it's SC_SWITCH */
    case TS_READY:
    case TS_WAIT:
	cpu_Switch(sc);

    /* If the task is removed, it's simply SC_DISPATCH */
    case TS_REMOVED:
	cpu_Dispatch(sc);
	break;

    /* Special state is used for returning from exception */
    case TS_EXCEPT:
	ctx = GetIntETask(task)->iet_Context;

        SysBase->ThisTask->tc_SPReg = ctx->sc;
        memcpy(ctx, SysBase->ThisTask->tc_SPReg, sizeof(struct AROSCPUContext));
        SysBase->ThisTask->tc_SPReg += sizeof(struct AROSCPUContext);

        /* Restore the signaled context. */
        RESTOREREGS(ctx, sc);
	errno = ctx->errno_backup;
	SP(sc) = (IPTR)SysBase->ThisTask->tc_SPReg;

        SysBase->ThisTask->tc_State = TS_RUN;
	Enable();

	break;
    }

    AROS_ATOMIC_DEC(supervisor);
}

static void core_IRQ(int sig, regs_t *sc)
{
    AROS_ATOMIC_INC(supervisor);

    /* Just additional protection - what if there's more than 32 signals? */
    if (sig < IRQ_COUNT)
	krnRunIRQHandlers(sig);

    if (sig == SIGALRM)
	core_TimerTick();

    core_ExitInterrupt(sc);

    AROS_ATOMIC_DEC(supervisor);
}

/*
 * This is from sigcore.h - it brings in the definition of the
 * systems initial signal handler, which simply calls
 * sighandler(int signum, regs_t sigcontext)
*/
GLOBAL_SIGNAL_INIT(core_Trap)
GLOBAL_SIGNAL_INIT(core_SysCall)
GLOBAL_SIGNAL_INIT(core_IRQ)

/* Set up the kernel. */
static int InitCore(struct KernelBase *KernelBase)
{
    struct itimerval interval;
    struct sigaction sa;
    struct SignalTranslation *s;

    /* We only want signal that we can handle at the moment */
    sigfillset(&sig_int_mask);
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
#ifdef __linux__
    sa.sa_restorer = NULL;
#endif

    supervisor = 0;

    /* 
     * These ones we consider as processor traps.
     * They are not be blocked by KrnCli()
     */
    SETHANDLER(sa, core_Trap);
    for (s = sigs; s->sig != -1; s++)
    {
	/* Trap handling disabled for now because:
	   1. It doesn't work, at least with SIGSEGV
	   2. It can interfere with gdb debugging.
	   Coming soon. */
	sigaction(s->sig, &sa, NULL);
	sigdelset(&sig_int_mask, s->sig);
    }

    /* SIGUSRs are software interrupts, we also never block them */
    sigdelset(&sig_int_mask, SIGUSR1);
    sigdelset(&sig_int_mask, SIGUSR2);

    /*
     * Any interrupt including software one must disable
     * all interrupts. Otherwise one interrupt may happen
     * between interrupt handler entry and supervisor mode
     * mark. This can cause bad things in cpu_Dispatch()
     */
    sa.sa_mask = sig_int_mask;

    /* Install interrupt handlers */
    SETHANDLER(sa, core_SysCall);
    sigaction(SIGUSR1, &sa, NULL);

    SETHANDLER(sa, core_IRQ);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGIO  , &sa, NULL);

    /* We need to start up with disabled interrupts */
    sigprocmask(SIG_BLOCK, &sig_int_mask, NULL);

    /* Set up the "pseudo" vertical blank interrupt. */
    D(bug("[InitCore] Timer frequency is %d\n", SysBase->ex_EClockFrequency));
    interval.it_interval.tv_sec = interval.it_value.tv_sec = 0;
    interval.it_interval.tv_usec =
    interval.it_value.tv_usec = 1000000 / SysBase->ex_EClockFrequency;

    return !setitimer(ITIMER_REAL, &interval, NULL);
}

ADD2INITLIB(InitCore, 10);

/*
 * All syscalls are mapped to single SIGUSR1.
 * We look at task's tc_State to determine the
 * needed action
 */
void krnSysCall(unsigned char n)
{
    kill(getpid(), SIGUSR1);
}
