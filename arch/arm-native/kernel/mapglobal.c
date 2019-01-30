/*
    Copyright � 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include "kernel_intern.h"

#include <proto/kernel.h>

AROS_LH4I(int, KrnMapGlobal,
	AROS_LHA(void *, virtual, A0),
	AROS_LHA(void *, physical, A1),
	AROS_LHA(uint32_t, length, D0),
	AROS_LHA(KRN_MapAttr, flags, D1),
	struct KernelBase *, KernelBase, 16, Kernel)
{
    AROS_LIBFUNC_INIT

    int retval = 0;

    D(bug("[Kernel] KrnMapGlobal(%08x->%08x %08x %04x)\n", virtual, physical, length, flags));

    return retval;

    AROS_LIBFUNC_EXIT
}
