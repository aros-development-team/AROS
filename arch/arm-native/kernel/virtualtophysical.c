/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <proto/kernel.h>

AROS_LH1I(void *, KrnVirtualToPhysical,
	AROS_LHA(void *, virtual, A0),
	struct KernelBase *, KernelBase, 20, Kernel)
{
    AROS_LIBFUNC_INIT

    uintptr_t virt = (uintptr_t)virtual;
    uintptr_t phys;

#if (1)
    //TODO:
    phys = virt;
#endif
    
    return (void*)phys;

    AROS_LIBFUNC_EXIT
}
