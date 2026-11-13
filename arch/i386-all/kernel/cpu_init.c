/*
    Copyright (C) 1995-2023, The AROS Development Team. All rights reserved.
*/

#include <aros/config.h>
#include <aros/symbolsets.h>
#include <asm/cpu.h>
#include <exec/types.h>
#include <aros/i386/cpucontext.h>

#include "kernel_base.h"
#include "kernel_debug.h"

#define D(x)

#ifndef SIZEOF_8087_FRAME
#define SIZEOF_8087_FRAME   sizeof(struct FPUContext)
#endif

/* FXSAVE/FXRESTOR instruction capability */
#define CPUID_EDX_FXSRB     24
#define CPUID_EDX_FXSRF     (1 << CPUID_EDX_FXSRB)
#define CPUID_EDX_SSEB      25
#define CPUID_EDX_SSEF      (1 << CPUID_EDX_SSEB)
#define CPUID_EDX_SSE2B     26
#define CPUID_EDX_SSE2F     (1 << CPUID_EDX_SSE2B)

static int cpu_Init(struct KernelBase *KernelBase)
{
    ULONG v1, v2, v3, v4;

    D(bug("[Kernel] cpu_Init(0x%p) for i386\n", KernelBase));

    KernelBase->kb_ContextSize = sizeof(struct AROSCPUContext);

    /* Evaluate CPU capabilities */
    asm volatile("cpuid":"=a"(v1),"=b"(v2),"=c"(v3),"=d"(v4):"a"(1));

    if (v4 & CPUID_EDX_FXSRF)
    {
        switch ((v4 & (CPUID_EDX_SSEF|CPUID_EDX_SSE2F)) >> CPUID_EDX_SSEB)
        {
        case 3:
        case 2:
            /* SSE2 */
        case 1:
            /* SSE */

#if (AROS_FLAVOUR & AROS_FLAVOUR_STANDALONE)
            /* tell the CPU that we will support SSE */
            wrcr(cr4, rdcr(cr4) | (3 << 9));
            /* Clear the EM and MP flags of CR0 */
            wrcr(cr0, rdcr(cr0) & ~6);
#endif

#ifdef USE_LEGACY_8087
            KernelBase->kb_ContextFlags = ECF_FPU|ECF_FPFXS;
            KernelBase->kb_ContextSize += SIZEOF_8087_FRAME;              /* Legacy 8087 frame with private portion */
#else
            KernelBase->kb_ContextFlags = ECF_FPFXS;
#endif
            KernelBase->kb_ContextSize += sizeof(struct FPXContext) + 15; /* Add 15 bytes for alignment */
            break;
        }
    }
    else
    {
        /* FPU only */
        KernelBase->kb_ContextFlags = ECF_FPU;
        KernelBase->kb_ContextSize += SIZEOF_8087_FRAME;
    }

    D(bug("[Kernel] CPU context flags: 0x%08X\n", KernelBase->kb_ContextFlags));

    return TRUE;
}

ADD2INITLIB(cpu_Init, 10);
