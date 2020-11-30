/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/config.h>
#include <aros/symbolsets.h>
#include <exec/types.h>
#include <asm/cpu.h>

#include "kernel_base.h"
#include <kernel_debug.h>

#define D(x)

static int cpu_Init(struct KernelBase *KernelBase)
{
    ULONG XContextSize = 0;
    ULONG AVXOffs = 0;

    D(bug("[Kernel] %s: KernelBase @ 0x%p\n", __func__, KernelBase);)

#if AROS_FLAVOUR == AROS_FLAVOUR_EMULATION
    /* check if the host enabled xgetbv */
#if (0)
    if (rdcr(cr4) & _CR4_OSXSBV)
    {
        D(bug("[Kernel] %s: Host has enabled xgetbv/xsetbv!\n", __func__, KernelBase);)

        // TODO: get the correct host values ..
        AVXOffs = 1;
        XContextSize = 0;
    }
#endif
#else
    D(bug("[Kernel] %s: Checking for XSAVE/AVX availability...\n", __func__, KernelBase);)

    /* Check CPUID to see if the processor
     * has support for XSAVE/AVX
     */
    asm volatile (
        "    mov $0x01, %%eax\n\t"
        "    cpuid\n\t"
        "    mov        $0x04000000, %%eax\n\t"                                         /* First check for XSAVE (bit 26)               */
        "    and        %%ecx, %%eax\n\t"
        "    jz .xscheck_done\n\t"
        "    mov        $0x10000000, %%eax\n\t"                                         /* and then check for AVX (bit 28)              */
        "    and        %%ecx, %%eax\n\t"
        "    jz .no_avx\n\t"
        "    mov        $0x0d, %%eax\n\t"                                               /* ok get the AVX offset                        */
        "    mov        $0x02, %%ecx\n\t"
        "    cpuid\n\t"
        "    mov        %%ebx, %1\n\t\n"
        ".no_avx:\n\t"
        "    mov        $0x0d, %%eax\n\t"                                               /* and get the size of the XSAVE area           */
        "    xor        %%ecx, %%ecx\n\t"                                               /* for Extended Control Register 0              */
        "    cpuid\n\t"
        "    mov        %%ebx, %0\n\t\n"
        ".xscheck_done:\n\t"
        : "=m"(XContextSize), "=m"(AVXOffs)
        :
        : "%eax", "%ebx", "%ecx", "%edx"
    );
#endif

    if (AVXOffs)
    {
        ULONG featMask;

        /* Use AVX/XSAVE support */
#if AROS_FLAVOUR != AROS_FLAVOUR_EMULATION
        D(bug("[Kernel] %s: Adjusting CR0 flags ...\n", __func__);)
        /* Enable monitoring media instruction to generate #NM when CR0.TS is set.
         * Disable coprocessor emulation.
         */
        wrcr(cr0, (rdcr(cr0) & ~_CR0_EM) | _CR0_MP);
        
        D(bug("[Kernel] %s: Enabling #XF & XGETVB ...\n", __func__);)
        /* Enable #XF instead of #UD when a SIMD exception occurs
         * Enable xgetvb/xsetvb
         */ 
        wrcr(cr4, rdcr(cr4) | _CR4_OSXMMEXCPT | _CR4_OSXSBV);
#endif
        D(bug("[Kernel] %s: Enabling Saving AVX context...\n", __func__);)
        asm volatile (
            "    xor        %%rcx, %%rcx\n\t"
            "    xgetbv\n\t"                                                            /* Load XCR0 register                           */
            "    or         $0b111, %%eax\n\t"                                          /* Set FPU, XMM, YMM bits                       */
            "    mov        %%eax, %0\n\t\n"
            "    xsetbv\n\t"                                                            /* Save back to XCR0                            */
            : "=m"(featMask) : : "%eax",  "%rcx"
            );

        D(
            bug("[Kernel] %s: AVX feature mask %08x\n", __func__, featMask);
            bug("[Kernel] %s: AVX required size = %u + %u, 64byte align\n", __func__, sizeof(struct AROSCPUContext), XContextSize);
        )
        KernelBase->kb_ContextSize = AROS_ROUNDUP2(XContextSize + sizeof(struct AROSCPUContext), 64);
    }
    else
    {
        /* All x86-64 processors have SSE/FXSAVE */
        D(bug("[Kernel] %s: SSE required size = %u + %u, 16byte align\n", __func__, sizeof(struct AROSCPUContext), sizeof(struct FPFXSContext));)
        KernelBase->kb_ContextSize = AROS_ROUNDUP2(sizeof(struct FPFXSContext) + sizeof(struct AROSCPUContext), 16);
    }

    D(bug("[Kernel] %s: CPU Context size = %u bytes\n", __func__, KernelBase->kb_ContextSize);)

    return TRUE;
}

ADD2INITLIB(cpu_Init, 5);
