/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Display an alert, Android-hosted version
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

#define D(x)

/*
 * This version displays an alert in Android GUI
 */

struct AlertRequest
{
    ULONG cmd;
    ULONG params;
    ULONG code;
    ULONG text;
};

AROS_LH2(void, KrnDisplayAlert,
	 AROS_LHA(uint32_t, code, D0),
	 AROS_LHA(const char *, text, A0),
	 struct KernelBase *, KernelBase, 35, Kernel)
{
    AROS_LIBFUNC_INIT

    struct AlertRequest req;
    int res;
    sigset_t sigs;

    /*
     * These two are inlined in Android.
     * Enable also SIGTERM - what if AROS will want to kill us...
     */
    sigemptyset(&sigs);
    sigaddset(&sigs, SIGUSR2);
    sigaddset(&sigs, SIGTERM);

    /* Prepare a message to server */    
    req.cmd    = 0x00001000;	/* cmd_Alert				   */
    req.params = 2;		/* Two parameters: code and string address */
    req.code   = code;
    req.text   = (IPTR)text;

    /* Halt the system in order not to interfere with our I/O */
    Disable();

    /* Send the packet */
    res = KernelIFace.write(KernelBase->kb_PlatformData->alertPipe, &req, sizeof(req));

    /* Standard pipe break reaction, see display driver code */
    if (res != sizeof(req))
    	ShutdownA(SD_ACTION_POWEROFF);

    D(bug("[KrnDisplayAlert] Request sent, halting...\n"));

    /*
     * Wait for SIGUSR2. Java side will deliver it to us after the alert was closed.
     * Normally it's used for KrnCause(), so in order not to execute scheduler and
     * SoftInts, we artificially raise our virtual privilege level.
     */
    AROS_ATOMIC_INC(KernelBase->kb_PlatformData->supervisor);
    KernelIFace.sigwait(&sigs, &res);
    AROS_ATOMIC_DEC(KernelBase->kb_PlatformData->supervisor);

    D(bug("[KrnDisplayAlert] Resume execution\n"));

    /* Recovered... */
    Enable();

    AROS_LIBFUNC_EXIT
}
