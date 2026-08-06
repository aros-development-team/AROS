/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Post-exec platform data for the opensbi-riscv64 target.

    Builds the per-hart table from what the boot parse found in the
    device tree - modeled on the x86 APIC per-CPU data. Only the boot
    hart executes AROS today; the others are parked in the SBI, and
    this is the record to bring them up from.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/symbolsets.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include <kernel_base.h>

#include "kernel_intern.h"

static int Platform_Init(struct KernelBase *LIBBASE)
{
    struct krnFDTInfo *fdt = __bootfdtinfo;
    struct PlatformData *pdata;
    uint32_t count, i;

    count = fdt->ncpus ? fdt->ncpus : 1;

    pdata = AllocMem(sizeof(struct PlatformData) +
                     count * sizeof(struct HartData),
                     MEMF_PUBLIC | MEMF_CLEAR);
    if (!pdata)
        return FALSE;

    pdata->kb_HartCount = count;
    pdata->kb_BootHart = fdt->boot_cpu;
    pdata->kb_TimebaseFreq = fdt->tb_freq;

    for (i = 0; i < count; i++)
    {
        pdata->kb_Harts[i].hd_HartID =
            (i < KRN_MAX_HARTS) ? fdt->hartids[i] : i;
        pdata->kb_Harts[i].hd_PLICContext =
            (i < fdt->plic_ncontexts) ? fdt->plic_contexts[i] : -1;
    }

    pdata->kb_Harts[pdata->kb_BootHart].hd_Flags = HARTF_BOOT | HARTF_ONLINE;

    LIBBASE->kb_PlatformData = pdata;

    D(
        bug("[Kernel:riscv64] %s: %u hart(s), boot hart index %d\n",
            __func__, count, pdata->kb_BootHart);
        for (i = 0; i < count; i++)
        {
            bug("[Kernel:riscv64] %s:  hart %u: id %lu plic ctx %d flags 0x%x\n",
                __func__, i, (unsigned long)pdata->kb_Harts[i].hd_HartID,
                pdata->kb_Harts[i].hd_PLICContext,
                pdata->kb_Harts[i].hd_Flags);
        }
    )

    return TRUE;
}

ADD2INITLIB(Platform_Init, 5)
