/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Display an alert in Android GUI if possible
    Lang: english
*/

#include <aros/atomic.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include <inttypes.h>
#include <signal.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "kernel_unix.h"
#include "kernel_android.h"

#define D(x)

AROS_LH2(void, KrnDisplayAlert,
	 AROS_LHA(uint32_t, code, D0),
	 AROS_LHA(const char *, text, A0),
	 struct KernelBase *, KernelBase, 35, Kernel)
{
    AROS_LIBFUNC_INIT

    struct KernelInterface *KernelIFace = KernelBase->kb_PlatformData->iface;
    int res;
    sigset_t sigs;

    /*
     * These two are inlined in Android.
     * Enable also SIGTERM - what if AROS will want to kill us...
     */
    sigemptyset(&sigs);
    sigaddset(&sigs, SIGUSR2);
    sigaddset(&sigs, SIGTERM);


    /* Halt the system in order not to interfere with our I/O */
    Disable();

    /* Send alert message to the display server */
    if (!SendAlert(code, text))
    {
    	/* Standard pipe break reaction, see display driver code */
        ShutdownA(SD_ACTION_POWEROFF);
    }

    D(bug("[KrnDisplayAlert] Request sent, halting...\n"));

    if (!KernelIFace)
    {
    	/* This is early alert, KernelBase is incomplete, and the condition is not recoverable. */
    	ShutdownA(SD_ACTION_POWEROFF);
    }

    /*
     * Wait for SIGUSR2. Java side will deliver it to us after the alert was closed.
     * Normally it's used for KrnCause(), and in order not to execute scheduler and
     * SoftInts, we artificially raise our virtual privilege level.
     */
    AROS_ATOMIC_INC(UKB(KernelBase)->SupervisorCount);
    KernelIFace->sigwait(&sigs, &res);
    AROS_ATOMIC_DEC(UKB(KernelBase)->SupervisorCount);

    D(bug("[KrnDisplayAlert] Resume execution\n"));

    /* Recovered... */
    Enable();

    AROS_LIBFUNC_EXIT
}
