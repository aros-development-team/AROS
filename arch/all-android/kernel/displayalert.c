/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Display an alert, Android-hosted version
    Lang: english
*/

#include <aros/libcall.h>
#include <proto/exec.h>

#include <inttypes.h>
#include <signal.h>

#include "kernel_base.h"
#include "kernel_intern.h"

AROS_LH2(void, KrnDisplayAlert,
	 AROS_LHA(uint32_t, code, D0),
	 AROS_LHA(const char *, text, A0),
	 struct KernelBase *, KernelBase, 35, Kernel)
{
    AROS_LIBFUNC_INIT

    /*
     * Explicitly disable task switching.
     * Yes, we are in Disable(). However Dalvik VM will enable SIGALRM.
     * This means Disable()d state will be broken. Additionally it messes
     * with stack or threads, which will cause AN_StackProbe guru during
     * displaying an alert if we don't do this.
     */
    Forbid();

    /* Display the alert via Java interface. */
    KernelBase->kb_PlatformData->DisplayAlert(text);

    /*
     * Fix up interrupts state before Permit().
     * Yes, there will be Enable() after return, but let's not
     * forget about nesting count.
     */
    KernelIFace.sigprocmask(SIG_BLOCK, &KernelBase->kb_PlatformData->sig_int_mask, NULL);
    Permit();

    AROS_LIBFUNC_EXIT
}
