/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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
#include "kernel_unix.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>
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

/*
 * This is the only platform-specific read-write variable we need.
 * In order to be able to make our kb_PlatformData read-only, we simply
 * moved it here. Let it be static...
 */
unsigned int SupervisorCount;

static void core_TrapHandler(int sig, regs_t *regs)
{
    struct SignalTranslation *s;
    short amigaTrap;
    struct AROSCPUContext ctx;

    AROS_ATOMIC_INC(SupervisorCount);

    /* Just for completeness */
    krnRunIRQHandlers(KernelBase, sig);

    bug("[KRN] Trap signal %d, SysBase %p, KernelBase %p\n", sig, SysBase, KernelBase);
    PRINT_SC(regs);

    /* Translate UNIX trap number to CPU and exec trap numbers */
    for (s = sigs; s->sig != -1; s++)
    {
	if (sig == s->sig)
	    break;
    }

    /*
     * Trap handler expects struct ExceptionContext, so we have to convert regs_t to it.
     * But first initialize all context area to zero, this is important since it may include
     * pointers to FPU state buffers.
     * TODO: FPU state also can be interesting for debuggers, we need to prepare space for it
     * too. Needs to be enclosed in some macros.
     */
    memset(&ctx, 0, sizeof(ctx));
    SAVEREGS(&ctx, regs);

    amigaTrap = s->AmigaTrap;
    if (s->CPUTrap != -1)
    {
	if (krnRunExceptionHandlers(KernelBase, s->CPUTrap, &ctx))
	    /* Do not call exec trap handler */
	    amigaTrap = -1;
    }

    /*
     * Call exec trap handler if needed.
     * Note that it may return, this means that the it has
     * fixed the problem somehow and we may safely continue.
     */
    if (amigaTrap != -1)
    	core_Trap(amigaTrap, &ctx);

    /* Trap handler(s) have possibly modified the context, so
       we convert it back before returning */
    RESTOREREGS(&ctx, regs);

    AROS_ATOMIC_DEC(SupervisorCount);
}

static void core_IRQ(int sig, regs_t *sc)
{
    AROS_ATOMIC_INC(SupervisorCount);

    /* Just additional protection - what if there's more than 32 signals? */
    if (sig < IRQ_COUNT)
	krnRunIRQHandlers(KernelBase, sig);

    if (SupervisorCount == 1)
	core_ExitInterrupt(sc);

    AROS_ATOMIC_DEC(SupervisorCount);
}

/*
 * This is from sigcore.h - it brings in the definition of the
 * systems initial signal handler, which simply calls
 * sighandler(int signum, regs_t sigcontext)
*/
GLOBAL_SIGNAL_INIT(core_TrapHandler)
GLOBAL_SIGNAL_INIT(core_SysCall)
GLOBAL_SIGNAL_INIT(core_IRQ)

extern struct HostInterface *HostIFace;

/* Set up the kernel. */
static int InitCore(struct KernelBase *KernelBase)
{
    struct PlatformData *pd;
    struct sigaction sa;
    struct SignalTranslation *s;
    sigset_t tmp_mask;

    D(bug("[KRN] InitCore()\n"));

    /*
     * We allocate PlatformData separately from KernelBase because
     * its definition relies on host includes (sigset_t), and
     * we don't want generic code to depend on host includes
     */
    pd = AllocMem(sizeof(struct PlatformData), MEMF_ANY);
    D(bug("[KRN] PlatformData %p\n", pd));
    if (!pd)
	return FALSE;

    pd->errnoPtr   = KernelIFace.__error();
    AROS_HOST_BARRIER

    KernelBase->kb_PlatformData = pd;

    SupervisorCount = 0;

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
    SETHANDLER(sa, core_TrapHandler);
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

    /*
     * Explicitly make sure that SIGUSR1 and SIGUSR2 are enabled.
     * This effectively kicks DalvikVM's ass on Android and takes SIGUSR1 away
     * from its "signal catcher". On other platforms this at least should not harm.
     * I also added SIGUSR2, just in case.
     */
    SIGEMPTYSET(&tmp_mask);
    SIGADDSET(&tmp_mask, SIGUSR1);
    SIGADDSET(&tmp_mask, SIGUSR2);
    KernelIFace.sigprocmask(SIG_UNBLOCK, &tmp_mask, NULL);
    AROS_HOST_BARRIER

    return TRUE;
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
