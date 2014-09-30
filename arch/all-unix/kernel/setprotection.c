/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_intern.h>

#include <sys/mman.h>
#include <inttypes.h>

AROS_LH3I(void, KrnSetProtection,
	 AROS_LHA(void *, address, A0),
	 AROS_LHA(uint32_t, length, D0),
         AROS_LHA(KRN_MapAttr, flags, D1),
	 struct KernelBase *, KernelBase, 21, Kernel)
{
    AROS_LIBFUNC_INIT

    int flags_unix = 0;

    if (flags & MAP_Readable)
	flags_unix |= PROT_READ;
    if (flags & MAP_Writable)
	flags_unix |= PROT_WRITE;
    if (flags & MAP_Executable)
	flags_unix |= PROT_EXEC;

    KernelBase->kb_PlatformData->iface->mprotect(address, length, flags_unix);
    AROS_HOST_BARRIER

    AROS_LIBFUNC_EXIT
}
