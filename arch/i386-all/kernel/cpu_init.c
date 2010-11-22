#include <aros/symbolsets.h>
#include <exec/types.h>
#include <aros/i386/cpucontext.h>

#include "kernel_base.h"
#include "kernel_debug.h"

#define D(x)

static int cpu_Init(struct KernelBase *KernelBase)
{
    ULONG v1, v2, v3, v4;

    KernelBase->kb_ContextSize = sizeof(struct AROSCPUContext);

    /* Evaluate CPU capabilities */
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
	    KernelBase->kb_ContextSize += 112;		/* 112 bytes for legacy 8087 frame */
#else
	    KernelBase->kb_ContextFlags = ECF_FPX;
#endif
	    KernelBase->kb_ContextSize += 512 + 15;	/* Add 15 bytes for alignment */
	    break;
	}
    }
    else
    {
        /* FPU only */
	KernelBase->kb_ContextFlags = ECF_FPU;
	KernelBase->kb_ContextSize += 112;
    }

    D(bug("[Kernel] CPU context flags: 0x%08X\n", KernelBase->kb_ContextFlags));

    return TRUE;
}
 
ADD2INITLIB(cpu_Init, 5);
