/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>
#include <aros/kernel.h>

#include <inttypes.h>

#include "kernel_base.h"
#include "kernel_mingw32.h"

static unsigned int access_map[] =
{
    PAGE_NOACCESS,
    PAGE_READONLY,
    PAGE_READWRITE, /* Windows has no write-only access */
    PAGE_READWRITE
};

AROS_LH3(void, KrnSetProtection,
	AROS_LHA(void *, address, A0),
	AROS_LHA(uint32_t, length, D0),
        AROS_LHA(KRN_MapAttr, flags, D1),
	struct KernelBase *, KernelBase, 21, Kernel)
{
    AROS_LIBFUNC_INIT
    
    unsigned int win_flags = access_map[(flags & 0x0300) >> 8];

    if (flags & MAP_Executable)
	win_flags <<= 4;

    if (flags & MAP_CacheInhibit)
	win_flags = PAGE_NOCACHE;
    if (flags & MAP_WriteThrough)	/* FIXME: is it correct mapping? */
	win_flags |= PAGE_WRITECOMBINE;
    if (flags & MAP_Guarded)
	win_flags |= PAGE_GUARD;

    KernelIFace.core_protect(address, length, win_flags);

    AROS_LIBFUNC_EXIT
}
