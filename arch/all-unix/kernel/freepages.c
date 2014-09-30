/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>

#include <inttypes.h>

#include <kernel_base.h>
#include <kernel_intern.h>

AROS_LH2(void, KrnFreePages,
	 AROS_LHA(void *, phy_addr, A0),
	 AROS_LHA(uint32_t, length, D0),
	 struct KernelBase *, KernelBase, 28, Kernel)
{
    AROS_LIBFUNC_INIT

    KernelIFace.munmap(phy_addr, length);
    AROS_HOST_BARRIER

    AROS_LIBFUNC_EXIT
}
