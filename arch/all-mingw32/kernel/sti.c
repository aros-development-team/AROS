/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>

#include "kernel_base.h"

AROS_LH0I(void, KrnSti,
	  struct KernelBase *, KernelBase, 10, Kernel)
{
    AROS_LIBFUNC_INIT

    /*
     * If we are in supervisor mode, don't do anything. Interrupts will
     * be enabled upon leaving supervisor mode by core_LeaveInterrupt().
     * Otherwise we can end up in nested interrupts
     */
    if (*KernelIFace.SuperState == 0)
	*KernelIFace.IntState = INT_ENABLE;

    AROS_LIBFUNC_EXIT
}
