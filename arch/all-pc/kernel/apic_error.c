/*
    Copyright (C) 2017, The AROS Development Team. All rights reserved.
*/

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

    // IN order to read APIC_ESR register one has to first write it with anything (zero is fine)
    // This forces update of the contents and new error codes may be read
    APIC_REG(__LAPICBase, APIC_ESR) = error_code;
    error_code = APIC_REG(__LAPICBase, APIC_ESR);

    bug("[KERNEL] %s: APIC Error interrupt! Error code=%08x\n", __func__, error_code);

    return TRUE;
}

int core_APICSpuriousHandle(struct ExceptionContext *regs, struct KernelBase *KernelBase, void *HandlerData2)
{
    bug("[KERNEL] %s: APIC Spurious interrupt!\n", __func__);

    return TRUE;
}
