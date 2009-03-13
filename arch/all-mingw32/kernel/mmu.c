/*
 * mmu.c
 *
 *  Created on: Nov 09, 2008
 *      Author: Pavel Fedin <sonic_amiga@rambler.ru>
 *
 *      In hosted version we may have some (limited) access to MMU
 *	and at least we can allocate memory at a given virtual address with given protection flags.
 *	However we don't have a possibility to touch real physical addresses. This means that behaviour
 *	of these functions on hosted can be very different compared to native.
 *	Anyway currently this functionality is not implemented because no components currently need it.
 *	Things may change in x86_64-mingw32 version because 64 bit Windows probably doesn't allow to
 *	execute code in allocated memory by default.
 */

#include <aros/kernel.h>
#include <proto/kernel.h>

AROS_LH3I(void, KrnSetProtection,
		 AROS_LHA(void *, address, A0),
		 AROS_LHA(uint32_t, length, D0),
         AROS_LHA(KRN_MapAttr, flags, D1),
         struct KernelBase *, KernelBase, 9, Kernel)
{
    AROS_LIBFUNC_INIT

    AROS_LIBFUNC_EXIT
}

AROS_LH4I(int, KrnMapGlobal,
         AROS_LHA(void *, virtual, A0),
         AROS_LHA(void *, physical, A1),
         AROS_LHA(uint32_t, length, D0),
         AROS_LHA(KRN_MapAttr, flags, D1),
         struct KernelBase *, KernelBase, 9, Kernel)
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
}

AROS_LH2I(int, KrnUnmapGlobal,
		AROS_LHA(void *, virtual, A0),
		AROS_LHA(uint32_t, length, D0),
		struct KernelBase *, KernelBase, 10, Kernel)
{
	AROS_LIBFUNC_INIT

	return 0;

	AROS_LIBFUNC_EXIT
}

AROS_LH1I(void *, KrnVirtualToPhysical,
		AROS_LHA(void *, virtual, A0),
		struct KernelBase *, KernelBase, 0, Kernel)
{
	AROS_LIBFUNC_INIT

	return virtual;

	AROS_LIBFUNC_EXIT
}