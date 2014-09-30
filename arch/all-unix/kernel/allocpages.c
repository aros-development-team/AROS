/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_intern.h>

#include <sys/mman.h>
#include <inttypes.h>

AROS_LH3I(void *, KrnAllocPages,
	 AROS_LHA(uint32_t, length, D0),
	 AROS_LHA(uint32_t, flags, D1),
	 AROS_LHA(KRN_MapAttr, protection, D2),
	 struct KernelBase *, KernelBase, 27, Kernel)
{
    AROS_LIBFUNC_INIT

    int flags_unix = 0;
    void *map = 0;

    if (protection & MAP_Readable)
	flags_unix |= PROT_READ;
    if (protection & MAP_Writable)
	flags_unix |= PROT_WRITE;
    if (protection & MAP_Executable)
	flags_unix |= PROT_EXEC;

    /* Darwin does not define MAP_ANONYMOUS */
    map = KernelIFace.mmap(NULL, length, flags_unix, MAP_PRIVATE | MAP_ANON, -1, 0);
    AROS_HOST_BARRIER

    return map;

    AROS_LIBFUNC_EXIT
}
