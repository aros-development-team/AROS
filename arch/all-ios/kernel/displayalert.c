/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Display an alert, iOS-hosted version
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

AROS_LH2(void, KrnDisplayAlert,
	 AROS_LHA(uint32_t, code, D0),
	 AROS_LHA(const char *, text, A0),
	 struct KernelBase *, KernelBase, 35, Kernel)
{
    AROS_LIBFUNC_INIT

    if (!KernelBase->kb_PlatformData->DisplayAlert)
    {
	/*
	 * Early alert. call hook is not initialized yet.
	 * Fail back to debug output.
	 */
    	krnDisplayAlert(text, KernelBase);
    	return;
    }

    /* Display the alert via our UIKit helper */
    KernelBase->kb_PlatformData->DisplayAlert(text);
    AROS_HOST_BARRIER

    AROS_LIBFUNC_EXIT
}
