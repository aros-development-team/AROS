/*
    Copyright (C) 2017-2023, The AROS Development Team. All rights reserved.
*/

#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>
#include <proto/exec.h>

#include <asm/cpu.h>
#include <asm/io.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <exec/execbase.h>
#include <exec/lists.h>

#include "kernel_base.h"
#include "kernel_intern.h"
#include "kernel_debug.h"
#include "kernel_globals.h"

#define D(x)

int core_APICErrorHandle(struct ExceptionContext *regs, struct KernelBase *KernelBase, void *HandlerData2)
{
    ULONG error_code = 0;
    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    struct APICData *apicData = pdata->kb_APIC;
    IPTR __LAPICBase = apicData->lapicBase;
    int cpunum = KrnGetCPUNumber();

    // IN order to read APIC_ESR register one has to first write it with anything (zero is fine)
    // This forces update of the contents and new error codes may be read
    APIC_REG(__LAPICBase, APIC_ESR) = error_code;
    error_code = APIC_REG(__LAPICBase, APIC_ESR);

    bug("[Kernel:APIC-IA32.%03u] %s: APIC Error interrupt! Error code=%08x\n", cpunum, __func__, error_code);

    return 0;
}

int core_APICSpuriousHandle(struct ExceptionContext *regs, struct KernelBase *KernelBase, void *HandlerData2)
{
    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    struct APICData *apicData = pdata->kb_APIC;
    IPTR __LAPICBase;
    int cpunum = KrnGetCPUNumber();
    APTR ssp = NULL;

    bug("[Kernel:APIC-IA32.%03u] %s: APIC Spurious interrupt!\n", cpunum, __func__);
    if ((pdata) && (pdata->kb_APIC))
    {
        __LAPICBase = apicData->lapicBase;
    }
    else
        __LAPICBase = core_APIC_GetBase();

    bug("[Kernel:APIC-IA32.%03u] %s: __LAPICBase = %p\n", cpunum, __func__, __LAPICBase);
#if (__WORDSIZE==64)
    bug("[Kernel:APIC-IA32.%03u] %s: rflags=%08x\n", cpunum, __func__, regs->rflags);
#else
    bug("[Kernel:APIC-IA32.%03u] %s: eflags=%08x\n", cpunum, __func__, regs->eflags);
#endif

    if ((KrnIsSuper()) || (ssp = SuperState()) != NULL)
    {
        bug("[Kernel:APIC-IA32.%03u] %s: IRR = %08x%08x%08x%08x%08x%08x%08x%08x\n",
            cpunum, __func__,
            APIC_REG(__LAPICBase, APIC_IRR+0x70), APIC_REG(__LAPICBase, APIC_IRR+0x60),
            APIC_REG(__LAPICBase, APIC_IRR+0x50), APIC_REG(__LAPICBase, APIC_IRR+0x40),
            APIC_REG(__LAPICBase, APIC_IRR+0x30), APIC_REG(__LAPICBase, APIC_IRR+0x20),
            APIC_REG(__LAPICBase, APIC_IRR+0x10), APIC_REG(__LAPICBase, APIC_IRR+0x00));
        bug("[Kernel:APIC-IA32.%03u] %s: ISR = %08x%08x%08x%08x%08x%08x%08x%08x\n",
            cpunum, __func__,
            APIC_REG(__LAPICBase, APIC_ISR+0x70), APIC_REG(__LAPICBase, APIC_ISR + 0x60),
            APIC_REG(__LAPICBase, APIC_ISR + 0x50), APIC_REG(__LAPICBase, APIC_ISR + 0x40),
            APIC_REG(__LAPICBase, APIC_ISR + 0x30), APIC_REG(__LAPICBase, APIC_ISR + 0x20),
            APIC_REG(__LAPICBase, APIC_ISR + 0x10), APIC_REG(__LAPICBase, APIC_ISR + 0x00));
        if (ssp)
            UserState(ssp);
    }

    return 1;
}
