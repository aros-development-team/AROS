#include <aros/kernel.h>
#include <aros/libcall.h>

#include "kernel_base.h"
#include "kernel_intern.h"
#include "apic.h"

AROS_LH0(unsigned int, KrnGetCPUNumber,
	 struct KernelBase *, KernelBase, 37, Kernel)
{
    AROS_LIBFUNC_INIT

    IPTR _APICBase = core_APIC_GetBase();

    return core_APIC_GetNumber(KernelBase, _APICBase);

    AROS_LIBFUNC_EXIT
}
