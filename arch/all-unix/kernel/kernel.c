/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Initialize the interface to the "hardware".
    Lang: english
*/

#include <aros/config.h>
#include <exec/types.h>
#include <exec/interrupts.h>
#include <exec/execbase.h>
#include <exec/alerts.h>
#include <aros/asmcall.h>
#include <aros/atomic.h>
#include <aros/symbolsets.h>
#include <hardware/intbits.h>

#define timeval sys_timeval
#include "etask.h"

#include <stdarg.h>

#include "hostinterface.h"
#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intr.h"
#include "kernel_intern.h"
#include "kernel_interrupts.h"
#include "kernel_scheduler.h"
#include "kernel_timer.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#undef timeval

#include <proto/exec.h>

#define D(x)
#define DSC(x)

#ifdef SIGCORE_NEED_SA_SIGINFO
#define SETHANDLER(sa, h) 			\
    sa.sa_sigaction = (void *)h ## _gate;	\
    sa.sa_flags |= SA_SIGINFO
#else
#define SETHANDLER(sa, h) 			\
    sa.sa_handler = (SIGHANDLER_T)h ## _gate;
#endif

static void core_Trap(int sig, regs_t *regs)
{
    void (*trapHandler)(unsigned long, struct AROSCPUContext *) = NULL;
    struct SignalTranslation *s;
    struct AROSCPUContext ctx;

    /* Just for completeness */
    krnRunIRQHandlers(sig);

    bug("[KRN] Trap signal %d, SysBase %p, KernelBase %p\n", sig, SysBase, KernelBase);

    /* Find out trap handler for caught task */
    if (SysBase)
    {
	struct Task *t = SysBase->ThisTask;

        if (t)
	{
	    bug("[KRN] %s %p (%s)\n", t->tc_Node.ln_Type == NT_TASK ? "Task":"Process", t, t->tc_Node.ln_Name ? t->tc_Node.ln_Name : "--unknown--");
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
     * Initialize all context area to zero, this is important since it may include pointers to FPU state buffers.
     * TODO: FPU state also can be interesting for debuggers, we need to prepare space for it too. Needs to be
     * enclosed in some macros.
     */
    memset(&ctx, 0, sizeof(ctx));

    /* Trap handler expects struct ExceptionContext, so we have to convert regs_t to it */
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
static void core_SysCall(int sig, regs_t *regs)
{
    struct Task *task = SysBase->ThisTask;

    AROS_ATOMIC_INC(KernelBase->kb_PlatformData->supervisor);

    DSC(bug("[KRN] core_SysCall entered\n"));

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
	cpu_Switch(regs);

    /* If the task is removed, it's simply SC_DISPATCH */
    case TS_REMOVED:
	cpu_Dispatch(regs);
	break;

    /* Special state is used for returning from exception */
    case TS_EXCEPT:
	cpu_DispatchContext(task, regs);
	break;
    }

    AROS_ATOMIC_DEC(KernelBase->kb_PlatformData->supervisor);
}

static void core_IRQ(int sig, regs_t *sc)
{
    AROS_ATOMIC_INC(KernelBase->kb_PlatformData->supervisor);

    /* Just additional protection - what if there's more than 32 signals? */
    if (sig < IRQ_COUNT)
	krnRunIRQHandlers(sig);

    if (sig == SIGALRM)
	core_TimerTick();

    if (KernelBase->kb_PlatformData->supervisor == 1)
	core_ExitInterrupt(sc);

    AROS_ATOMIC_DEC(KernelBase->kb_PlatformData->supervisor);
}

/*
 * This is from sigcore.h - it brings in the definition of the
 * systems initial signal handler, which simply calls
 * sighandler(int signum, regs_t sigcontext)
*/
GLOBAL_SIGNAL_INIT(core_Trap)
GLOBAL_SIGNAL_INIT(core_SysCall)
GLOBAL_SIGNAL_INIT(core_IRQ)

extern struct HostInterface *HostIFace;

/* Set up the kernel. */
static int InitCore(struct KernelBase *KernelBase)
{
    struct PlatformData *pd;
    struct itimerval interval;
    struct sigaction sa;
    struct SignalTranslation *s;
    int ret;

    D(bug("[KRN] InitCore()\n"));

#ifdef HOST_OS_android
    KernelBase->kb_PageSize = *KernelIFace.__page_size;
#else
    KernelBase->kb_PageSize = KernelIFace.getpagesize();
    AROS_HOST_BARRIER
#endif
    D(bug("[KRN] Memory page size is %u\n", KernelBase->kb_PageSize));

#if AROS_MODULES_DEBUG
    /*
     * Provide a pointer to our modules list to the bootstrap.
     * This is needed because gdb is actually debugging bootstrap
     * and it can read debug information only from there
     */
    if (HostIFace->ModListPtr)
	*HostIFace->ModListPtr = &KernelBase->kb_Modules;
#endif

    /*
     * We allocate PlatformData separately from KernelBase because
     * its definition relies on host includes (sigset_t), and
     * we don't want generic code to depend on host includes
     */
    pd = AllocMem(sizeof(struct PlatformData), MEMF_ANY);
    D(bug("[KRN] PlatformData %p\n", pd));
    if (!pd)
	return FALSE;

    pd->supervisor = 0;
    pd->errnoPtr = KernelIFace.__error();
    AROS_HOST_BARRIER
    KernelBase->kb_PlatformData = pd;

    /* We only want signal that we can handle at the moment */
    SIGFILLSET(&pd->sig_int_mask);
    SIGEMPTYSET(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
#ifdef HOST_OS_linux
    sa.sa_restorer = NULL;
#endif

    /* 
     * These ones we consider as processor traps.
     * They are not be blocked by KrnCli()
     */
    SETHANDLER(sa, core_Trap);
    for (s = sigs; s->sig != -1; s++)
    {
	KernelIFace.sigaction(s->sig, &sa, NULL);
	AROS_HOST_BARRIER
	SIGDELSET(&pd->sig_int_mask, s->sig);
    }

    /* SIGUSRs are software interrupts, we also never block them */
    SIGDELSET(&pd->sig_int_mask, SIGUSR1);
    SIGDELSET(&pd->sig_int_mask, SIGUSR2);
    /* We want to be able to interrupt AROS using Ctrl-C in its console,
       so exclude SIGINT too. */
    SIGDELSET(&pd->sig_int_mask, SIGINT);

    /*
     * Any interrupt including software one must disable
     * all interrupts. Otherwise one interrupt may happen
     * between interrupt handler entry and supervisor count
     * increment. This can cause bad things in cpu_Dispatch()
     */
    sa.sa_mask = pd->sig_int_mask;

    /* Install interrupt handlers */
    SETHANDLER(sa, core_IRQ);
    KernelIFace.sigaction(SIGALRM, &sa, NULL);
    AROS_HOST_BARRIER
    KernelIFace.sigaction(SIGIO  , &sa, NULL);
    AROS_HOST_BARRIER

    /* Software IRQs do not need to block themselves. Anyway we know when we send them. */
    sa.sa_flags |= SA_NODEFER;

    KernelIFace.sigaction(SIGUSR2, &sa, NULL);
    AROS_HOST_BARRIER

    SETHANDLER(sa, core_SysCall);
    KernelIFace.sigaction(SIGUSR1, &sa, NULL);
    AROS_HOST_BARRIER

    /* We need to start up with disabled interrupts */
    KernelIFace.sigprocmask(SIG_BLOCK, &pd->sig_int_mask, NULL);
    AROS_HOST_BARRIER

    /* Set up the "pseudo" vertical blank interrupt. */
    D(bug("[InitCore] Timer frequency is %d\n", SysBase->ex_EClockFrequency));
    interval.it_interval.tv_sec = interval.it_value.tv_sec = 0;
    interval.it_interval.tv_usec =
    interval.it_value.tv_usec = 1000000 / SysBase->ex_EClockFrequency;

    ret = KernelIFace.setitimer(ITIMER_REAL, &interval, NULL);
    AROS_HOST_BARRIER
    D(bug("[KRN] setitimer() returned %d, errno %d\n", ret, *pd->errnoPtr));

    return !ret;
}

ADD2INITLIB(InitCore, 10);

/*
 * All syscalls are mapped to single SIGUSR1.
 * We look at task's tc_State to determine the
 * needed action
 */
void krnSysCall(unsigned char n)
{
    DSC(bug("[KRN] SysCall %d\n", n));
    KernelIFace.raise(SIGUSR1);
    AROS_HOST_BARRIER
}
