/*
    Copyright � 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include "kernel_intern.h"

#include <proto/kernel.h>

AROS_LH3I(void, KrnSetProtection,
	AROS_LHA(void *, address, A0),
	AROS_LHA(uint32_t, length, D0),
        AROS_LHA(KRN_MapAttr, flags, D1),
	struct KernelBase *, KernelBase, 21, Kernel)
{
    AROS_LIBFUNC_INIT

    D(bug("[Kernel] KrnSetProtection(%08x, %08x, %04x)\n", address, address + length - 1, flags));

    AROS_LIBFUNC_EXIT
}
