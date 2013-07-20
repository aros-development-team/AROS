#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

#include <proto/kernel.h>

/* See rom/kernel/virtualtophysical.c for documentation */

AROS_LH1I(void *, KrnVirtualToPhysical,
    AROS_LHA(void *, virtual, A0),
    struct KernelBase *, KernelBase, 20, Kernel)
{
	AROS_LIBFUNC_INIT

	uintptr_t virt = (uintptr_t)virtual;
	uintptr_t phys = virt;

	if (virt >= 0xff000000)
		phys = virt - 0xff000000;

	return (void*)phys;

	AROS_LIBFUNC_EXIT
}
