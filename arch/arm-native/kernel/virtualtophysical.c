/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <proto/kernel.h>

#include "kernel_intern.h"
extern struct TagItem *BootMsg;

AROS_LH1I(void *, KrnVirtualToPhysical,
	AROS_LHA(void *, virtual, A0),
	struct KernelBase *, KernelBase, 20, Kernel)
{
    AROS_LIBFUNC_INIT

    if (virtual < (void *)0xf8000000)
        return virtual;
    else
    {
        void *lowest = krnFindTagItem(KRN_KernelLowest, BootMsg);
        void *physlowest = krnFindTagItem(KRN_KernelPhysLowest, BootMsg);

        return virtual - lowest + physlowest;
    }

    AROS_LIBFUNC_EXIT
}
