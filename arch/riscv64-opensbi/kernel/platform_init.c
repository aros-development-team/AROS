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

#include <asm/cpu.h>

#include <kernel_base.h>

#include "kernel_intern.h"
#include "kernel_sbi.h"

#if defined(__AROSEXEC_SMP__)
/* startup.S */
extern void __secondary_entry(void);

static struct PlatformData *rv_pdata;

/* The SBI hart id of a logical CPU */
static inline unsigned long rv_CPUHartID(uint32_t cpu)
{
    return rv_pdata->kb_Harts[rv_pdata->kb_CPUHart[cpu]].hd_HartID;
}

/*
 * RVI_StartCPU: start the next hart that has not been tried yet as
 * logical CPU cpu. 0 when HSM accepted it, 1 when that hart could not
 * be started (try the next), -1 when there are none left.
 */
static int rv_StartCPU(uint32_t cpu, tls_t *tls)
{
    uint32_t i;

    for (i = 0; i < rv_pdata->kb_HartCount; i++)
    {
        struct HartData *hd = &rv_pdata->kb_Harts[i];
        long status, err;

        if (hd->hd_Flags & (HARTF_BOOT | HARTF_ONLINE | HARTF_TRIED))
            continue;
        hd->hd_Flags |= HARTF_TRIED;

        /* Only a hart the firmware has parked can be started. One just
           stopped by a warm restart may still be on its way there. */
        status = sbi_hart_get_status(hd->hd_HartID);
        {
            int tries = 1000;

            while ((status == SBI_HSM_STATE_STOP_PENDING) && --tries)
                status = sbi_hart_get_status(hd->hd_HartID);
        }
        if (status != SBI_HSM_STATE_STOPPED)
        {
            bug("[Kernel:riscv64] hart %lu: HSM state %ld, not starting it\n",
                (unsigned long)hd->hd_HartID, status);
            return 1;
        }

        hd->hd_CPUNumber = cpu;
        rv_pdata->kb_CPUHart[cpu] = (int)i;
        tls->HartID = hd->hd_HartID;
        tls->Platform = hd;

        err = sbi_hart_start(hd->hd_HartID, (unsigned long)__secondary_entry,
                             (unsigned long)tls);
        if (err != SBI_SUCCESS)
        {
            bug("[Kernel:riscv64] hart %lu: hart_start failed (%ld)\n",
                (unsigned long)hd->hd_HartID, err);
            rv_pdata->kb_CPUHart[cpu] = -1;
            return 1;
        }
        return 0;
    }

    return -1;
}

/* RVI_StopCPU: back to the firmware for good (see krnWarmKick) */
static void rv_StopCPU(void)
{
    csr_write(sie, 0);
    sbi_hart_stop();
    for (;;)
        asm volatile("wfi");
}

/* RVI_SendIPI: one SBI call per hart - ids need not be contiguous */
static void rv_SendIPI(const ULONG *mask)
{
    uint32_t cpu;

    for (cpu = 0; cpu < RISCV_MAXCPUS; cpu++)
        if (mask[cpu / 32] & (1UL << (cpu % 32)))
            sbi_send_ipi(1, rv_CPUHartID(cpu));
}

/* RVI_RemoteSFence: SBI RFENCE remote_sfence_vma, synchronous */
static void rv_RemoteSFence(const ULONG *mask, IPTR va, IPTR size)
{
    uint32_t cpu;

    for (cpu = 0; cpu < RISCV_MAXCPUS; cpu++)
        if (mask[cpu / 32] & (1UL << (cpu % 32)))
            sbi_remote_sfence_vma(1, rv_CPUHartID(cpu), va, size);
}

/*
 * RVI_InitCore. Nothing to set up on a secondary hart beyond what its
 * entry did: device interrupts stay with the boot CPU. The PLIC has
 * every source enabled on every supervisor context (S-mode cannot be
 * sure which context is its own, see kernel_plic.c) and the boot CPU
 * claims from all of them, so a secondary leaves sie.SEIE clear and
 * never takes a device interrupt.
 */
static void rv_InitCore(struct KernelBase *KernelBase, struct ExecBase *SysBase)
{
    struct HartData *hd = (struct HartData *)TLS_GET(Platform);

    (void)KernelBase;
    (void)SysBase;

    if (hd)
        hd->hd_Flags |= HARTF_ONLINE;
}
#endif /* __AROSEXEC_SMP__ */

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

    /*
     * Cross-check the boot hart with the firmware. What we were handed
     * is not always right (an EFI loader without RISCV_EFI_BOOT_PROTOCOL
     * reports 0), and everything numbered by it - the IPIs to CPU 0
     * above all - would go to the wrong hart. Every hart but the one we
     * run on is still parked, so if HSM says the recorded one is not
     * running but exactly one other is, that one is us.
     */
    if (sbi_probe_extension(SBI_EXT_HSM) && (count > 1) &&
        (sbi_hart_get_status(pdata->kb_Harts[pdata->kb_BootHart].hd_HartID) !=
         SBI_HSM_STATE_STARTED))
    {
        int running = -1, nrunning = 0;

        for (i = 0; i < count; i++)
        {
            if (sbi_hart_get_status(pdata->kb_Harts[i].hd_HartID) ==
                SBI_HSM_STATE_STARTED)
            {
                running = (int)i;
                nrunning++;
            }
        }
        if (nrunning == 1)
        {
            bug("[Kernel:riscv64] %s: booted on hart %lu, not %lu as reported\n",
                __func__, (unsigned long)pdata->kb_Harts[running].hd_HartID,
                (unsigned long)pdata->kb_Harts[pdata->kb_BootHart].hd_HartID);
            pdata->kb_BootHart = running;
            __boot_hartid = pdata->kb_Harts[running].hd_HartID;
            __boot_cpu_index = running;
#if defined(__AROSEXEC_SMP__)
            TLS_SET(HartID, __boot_hartid);
#endif
        }
    }

    pdata->kb_Harts[pdata->kb_BootHart].hd_Flags = HARTF_BOOT | HARTF_ONLINE;

    /* The boot hart is logical CPU 0; the rest are numbered as they start */
    for (i = 0; i < RISCV_MAXCPUS; i++)
        pdata->kb_CPUHart[i] = -1;
    pdata->kb_CPUHart[0] = pdata->kb_BootHart;
    pdata->kb_Harts[pdata->kb_BootHart].hd_CPUNumber = 0;

    LIBBASE->kb_PlatformData = pdata;

    __riscv_arosintern.RVI_TimebaseFreq = fdt->tb_freq;

#if defined(__AROSEXEC_SMP__)
    rv_pdata = pdata;

    /* The boot hart's TLS (startup.S loaded tp with it) */
    TLS_SET(SysBase, SysBase);
    TLS_SET(KernelBase, (void *)LIBBASE);
    TLS_SET(Platform, &pdata->kb_Harts[pdata->kb_BootHart]);

    __riscv_arosintern.RVI_InitCore = rv_InitCore;
    __riscv_arosintern.RVI_InitTimerCore = krnTimerInitCore;

    /* Without HSM and IPI the other harts can not be used: stay on one */
    if (sbi_probe_extension(SBI_EXT_HSM) && sbi_probe_extension(SBI_EXT_IPI))
    {
        __riscv_arosintern.RVI_StartCPU = rv_StartCPU;
        __riscv_arosintern.RVI_StopCPU = rv_StopCPU;
        __riscv_arosintern.RVI_SendIPI = rv_SendIPI;
        if (sbi_probe_extension(SBI_EXT_RFENCE))
            __riscv_arosintern.RVI_RemoteSFence = rv_RemoteSFence;
    }
    else
        bug("[Kernel:riscv64] %s: no SBI HSM/IPI - running on the boot hart only\n",
            __func__);

    /* Inter-processor interrupts reach the boot hart too */
    csr_set(sie, SIE_SSIE);
#endif

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
