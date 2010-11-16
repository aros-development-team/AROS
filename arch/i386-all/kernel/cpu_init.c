#include <aros/symbolsets.h>
#include <exec/types.h>
#include <aros/i386/cpucontext.h>

#include "kernel_base.h"
#include "kernel_debug.h"

#define D(x) x

static int cpu_Init(struct KernelBase *KernelBase)
{
    /* Evaluate CPU capabilities */
    ULONG v1, v2, v3, v4;

    asm volatile("cpuid":"=a"(v1),"=b"(v2),"=c"(v3),"=d"(v4):"a"(1));

    if (v4 & (1 << 24))
    {
	switch ((v4 >> 25) & 3)
	{
        case 3:
        case 2:
        case 1:
	    /* FPU + SSE */
#ifdef USE_LEGACY_8087
	    KernelBase->kb_ContextFlags = ECF_FPU|ECF_FPX;
#else
	    KernelBase->kb_ContextFlags = ECF_FPX;
#endif
	    break;
	}
    }
    else
        /* FPU only */
	KernelBase->kb_ContextFlags = ECF_FPU;

    D(bug("[Kernel] CPU context flags: 0x%08X\n", KernelBase->kb_ContextFlags));

    return TRUE;
}
 
ADD2INITLIB(cpu_Init, 5);
