/*
    Copyright (C) 1995-2023, The AROS Development Team. All rights reserved.

    Desc: Intel IA-32 APIC driver.
*/

#include <aros/macros.h>
#include <asm/cpu.h>
#include <asm/io.h>
#include <exec/types.h>

#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>
#include <proto/exec.h>
#include <proto/acpica.h>

#include <acpica/acnames.h>
#include <acpica/accommon.h>

#if (1)
#include <hardware/pit.h>
#endif

#include <inttypes.h>

#include "kernel_base.h"
#include "kernel_intern.h"
#include "kernel_objects.h"
#include "kernel_debug.h"
#include "kernel_syscall.h"
#include "kernel_timer.h"
#include "kernel_ipi.h"

#include "kernel_interrupts.h"

#include "acpi.h"
#include "apic_ia32.h"

#define D(x)
#define DINT(x)
#define DWAKE(x)         /* Badly interferes with AP startup */
#define DID(x)           /* Badly interferes with everything */
#define DCALIB(x)
#define DPIT(x)
/* #define DEBUG_WAIT */

/*
 * On i386 platform we need to support various quirks of old APICs.
 * x86-64 is free of that crap.
 */
#ifdef __i386__
#define CONFIG_LEGACY
#endif

#define FORCE_PIT_CALIB

#define	APIC_CRMAXVAL	0XFFFFFFFFUL

extern int core_APICErrorHandle(struct ExceptionContext *, void *, void *);

extern int APICHeartbeatServer(struct ExceptionContext *regs, struct KernelBase *KernelBase, struct ExecBase *SysBase);

/* APIC Interrupt Controller Functions ... ***************************/

struct APICInt_Private
{
    
};

icid_t APICInt_Register(struct KernelBase *KernelBase)
{
    DINT(bug("[Kernel:APIC-IA32] %s()\n", __func__));

    return (icid_t)APICInt_IntrController.ic_Node.ln_Type;
}

BOOL APICInt_Init(struct KernelBase *KernelBase, icid_t instanceCount)
{
    struct PlatformData *kernPlatD = (struct PlatformData *)KernelBase->kb_PlatformData;
    struct ACPIData *acpiData  = kernPlatD->kb_ACPI;
    struct APICData *apicPrivate = kernPlatD->kb_APIC;
    APTR ssp;
    int irq, msibase = 0, count = 0, msiavailable = 0;

    DINT(bug("[Kernel:APIC-IA32] %s(%d)\n", __func__, instanceCount));

    /* It's not fatal to fail on these IRQs */
    if ((ssp = SuperState()) != NULL)
    {
        /* Set up the APIC IRQs for CPU #0 */
        for (irq = (APIC_IRQ_BASE - X86_CPU_EXCEPT_COUNT); irq < HW_IRQ_COUNT; irq++)
        {
            if ((KERNELIRQ_LIST(irq).lh_Type != KBL_INTERNAL) || (!krnInitInterrupt(KernelBase, irq, APICInt_IntrController.ic_Node.ln_Type, 0)))
            {
                D(
                    if (KERNELIRQ_LIST(irq).lh_Type == KBL_INTERNAL)
                    {
                        bug("[Kernel:APIC-IA32] %s: failed to obtain IRQ %d\n", __func__, irq);
                    }
                )
                if (count > 31)
                    msiavailable += count;
                count = 0;
            }
            else
            {
                /* Don't enable the vector yet */
                if (!core_SetIDTGate((x86vectgate_t *)apicPrivate->cores[0].cpu_IDT, HW_IRQ_BASE + irq, (uintptr_t)IntrDefaultGates[HW_IRQ_BASE + irq], FALSE, FALSE))
                {
                    bug("[Kernel:APIC-IA32] %s: failed to set IRQ %d's Vector gate\n", __func__, irq);
                    if (count > 31)
                        msiavailable += count;
                    count = 0;
                }
                else 
                {
                    if ((count++ > 31) && (msibase == 0))
                        msibase = irq + 1 - count;
                }
            }
        }
        if ((count > 31) && (msiavailable == 0))
            msiavailable = count;

        UserState(ssp);
    }

    /*
     * If we have at least 32 APIC interrupts available (the
     * most a single MSI device will request) then report that
     * we can use MSI
     */
    if (count > 31)
    {
        BOOL useMSI = TRUE;
        if ((acpiData) && (acpiData->acpi_fadt))
        {
            ACPI_TABLE_FADT *fadt = (ACPI_TABLE_FADT *)acpiData->acpi_fadt;
            if (fadt->BootFlags & ACPI_FADT_NO_MSI)
                useMSI = FALSE;
        }

        if ((useMSI) &&
            (!(kernPlatD->kb_PDFlags & PLATFORMF_HAVEMSI)))
        {
            kernPlatD->kb_PDFlags |= PLATFORMF_HAVEMSI;
            apicPrivate->msibase = msibase;
//            D(
                bug("[Kernel:APIC-IA32] MSI Interrupts Allocatable\n");
                bug("[Kernel:APIC-IA32]     start = %u\n", msibase);
                bug("[Kernel:APIC-IA32]     total = %u\n", msiavailable);
//            )
        }
    }
    return TRUE;
}

BOOL APICInt_DisableIRQ(APTR icPrivate, icid_t icInstance, icid_t intNum)
{
    struct PlatformData *kernPlatD = (struct PlatformData *)KernelBase->kb_PlatformData;
    struct APICData *apicPrivate = kernPlatD->kb_APIC;
    apicid_t cpuNum = KrnGetCPUNumber();
    x86vectgate_t *IGATES;
    APTR ssp = NULL;
    BOOL retVal = FALSE;

    DINT(bug("[Kernel:APIC-IA32.%03u] %s(#$%02X)\n", cpuNum, __func__, intNum));

    IGATES = (x86vectgate_t *)apicPrivate->cores[cpuNum].cpu_IDT;

    if ((KrnIsSuper()) || ((ssp = SuperState()) != NULL))
    {
        IGATES[HW_IRQ_BASE + intNum].p = 0;
        retVal = TRUE;

        if (ssp)
            UserState(ssp);
    }

    return retVal;
}

BOOL APICInt_EnableIRQ(APTR icPrivate, icid_t icInstance, icid_t intNum)
{
    struct PlatformData *kernPlatD = (struct PlatformData *)KernelBase->kb_PlatformData;
    struct APICData *apicPrivate = kernPlatD->kb_APIC;
    apicid_t cpuNum = KrnGetCPUNumber();
    x86vectgate_t *IGATES;
    APTR ssp = NULL;
    BOOL retVal = FALSE;

    DINT(bug("[Kernel:APIC-IA32.%03u] %s(#$%02X)\n", cpuNum, __func__, intNum));

    IGATES = (x86vectgate_t *)apicPrivate->cores[cpuNum].cpu_IDT;

    if ((KrnIsSuper()) || ((ssp = SuperState()) != NULL))
    {
        IGATES[HW_IRQ_BASE + intNum].p = 1;
        retVal = TRUE;

        if (ssp)
            UserState(ssp);
    }

    return retVal;
}

BOOL APICInt_AckIntr(APTR icPrivate, icid_t icInstance, icid_t intNum)
{
    IPTR apic_base;

    DINT(bug("[Kernel:APIC-IA32] %s(%03u #$%02X)\n", __func__, icInstance, intNum));

    /* Write zero to EOI of APIC */
    apic_base = core_APIC_GetBase();

    APIC_REG(apic_base, APIC_EOI) = 0;

    return TRUE;
}

struct IntrController APICInt_IntrController =
{
    {
        .ln_Name = "x86 Local APIC",
        .ln_Pri = -50
    },
    0,
    IIC_ID_APIC,
    0,
    NULL,
    APICInt_Register,
    APICInt_Init,
    APICInt_EnableIRQ,
    APICInt_DisableIRQ,
    APICInt_AckIntr
};

/* APIC IPI Related Functions ... ***************************/

static ULONG ia32_ipi_send(IPTR __APICBase, ULONG target, ULONG cmd)
{
    ULONG ipisend_timeout, status_ipisend;

    D(
        apicid_t cpuNum = KrnGetCPUNumber();
        bug("[Kernel:APIC-IA32.%03u] %s: Command 0x%08X to target %03u\n", cpuNum, __func__, cmd, target);
    )

    /*
     * Send the IPI.
     * First we write target APIC ID into high command register.
     * Writing to the low register triggers the IPI itself.
     */
    APIC_REG(__APICBase, APIC_ICRH) = target << 24;
    APIC_REG(__APICBase, APIC_ICRL) = cmd;

    D(bug("[Kernel:APIC-IA32.%03u] %s: Waiting for IPI to complete ", cpuNum, __func__));

    for (ipisend_timeout = 1000; ipisend_timeout > 0; ipisend_timeout--)
    {
        krnClockSourceUdelay(100);
#ifdef DEBUG_WAIT
        if ((ipisend_timeout % 100) == 0)
        {
            bug(".");
        }
#endif
        status_ipisend = APIC_REG(__APICBase, APIC_ICRL) & ICR_DS;
        /* Delivery status resets to 0 when delivery is done */
        if (status_ipisend == 0)
            break;
    }
    D(bug("\n"));
    D(bug("[Kernel:APIC-IA32.%03u] %s: ... left wait loop (status = 0x%08X)\n", cpuNum, __func__, status_ipisend));

    return status_ipisend;
}

/*
 * Calibrate LAPIC timer frequency using the PIT.
 * The idea behind the calibration is to run the timer once, and see how many ticks
 * pass in some defined period of time. Then calculate a proportion.
 * We use 8253 PIT as our reference.
 * This calibration algorithm is based on NetBSD one.
 *
 * wait for 11931 PIT ticks, which is equal to 10 milliseconds.
 * We don't use krnClockSourceUdelay() here, because for improved accuracy we need to sample LAPIC timer counter twice,
 * before and after our actual delay (PIT setup also takes up some time, so LAPIC will count away from its
 * initial value).  We run it 10 times to make up for cache setup discrepancies.
 */

#define PIT_SAMPLECOUNT     10
#define PIT_WAITTICKS       11931
static UQUAD ia32_tsc_calibrate_pit(apicid_t cpuNum)
{
    UQUAD tsc_initial, tsc_final, calibrated_tsc = 0;
    UQUAD difftsc[PIT_SAMPLECOUNT];
    UWORD pitresults[PIT_SAMPLECOUNT];
    ULONG pit_total = 0;
    UWORD pit_final;
    int samples, iter, i;

    samples = iter = PIT_SAMPLECOUNT;

    tsc_initial = RDTSC();
    DPIT(bug("[Kernel:APIC-IA32.%03u] %s: TSC pre-calib = %u\n", cpuNum, __func__, tsc_initial);)

    for (i = 0; i < PIT_SAMPLECOUNT; i++)
    {
        tsc_initial = RDTSC();

        pit_final   = pit_wait(PIT_WAITTICKS);

        tsc_final = RDTSC();

        /* Ignore results that are invalid, or wrap around. */
        if ((pit_final >= PIT_WAITTICKS) && (tsc_final > tsc_initial))
        {
            difftsc[i] = tsc_final - tsc_initial;
            pitresults[i] = pit_final;
            pit_total += pit_final;
            DPIT(bug("[Kernel:APIC-IA32.%03u] %s: %u, diff = %u\n", cpuNum, __func__, pitresults[i], difftsc[i]);)
        }
        else if (tsc_final < tsc_initial)
        {
            DPIT(bug("[Kernel:APIC-IA32.%03u] %s: skipping wrap around\n", cpuNum, __func__);)
            i -= 1;
        }
        else
        {
            DPIT(bug("[Kernel:APIC-IA32.%03u] %s: ## %02u - ignoring pit_final = %u\n", cpuNum, __func__, (PIT_SAMPLECOUNT - samples), pit_final);)
            pitresults[i] = 0;
            samples -= 1;
        }
    }

    if (samples > 0)
    {
        UWORD pit_avg = (pit_total / samples), pit_error = pit_avg - ((pit_avg * (100 - (pit_avg / PIT_WAITTICKS))) / 100);

        DPIT(
            bug("[Kernel:APIC-IA32.%03u] %s: PIT Avg %u, error %u\n", cpuNum, __func__, pit_avg, pit_error);
            bug("[Kernel:APIC-IA32.%03u] %s:     Min %u\n", cpuNum, __func__, pit_avg - pit_error);
            bug("[Kernel:APIC-IA32.%03u] %s:     Max %u\n", cpuNum, __func__, pit_avg + pit_error);
        )

        for (i = 0; i < PIT_SAMPLECOUNT; i++)
        {
            if ((pitresults[i] >= pit_avg - pit_error) && (pitresults[i] <= pit_avg + pit_error))
                calibrated_tsc += (difftsc[i] * PIT_WAITTICKS) / pitresults[i];
            else
                iter -= 1;
        }

        if (iter > 0)
        {
            DPIT(
                for (i = 0; i < PIT_SAMPLECOUNT; i++)
                {
                  bug("[Kernel:APIC-IA32.%03u] %s: pit_final #%02u = %u (%llu)\n", cpuNum, __func__, i, pitresults[i], difftsc[i]);
                }
                bug("[Kernel:APIC-IA32.%03u] %s: iter: %u, freq: %llu\n", cpuNum, __func__, iter, (10 * iter * calibrated_tsc) / iter);
              )
            calibrated_tsc = (10 * iter * calibrated_tsc / iter);
        }
        else
        {
            bug("[Kernel:APIC-IA32.%03u] %s: calibration failed\n", cpuNum, __func__);
        }
    }
    else
    {
        bug("[Kernel:APIC-IA32.%03u] %s: sampling failed\n", cpuNum, __func__);
    }

    return calibrated_tsc;
}

static UQUAD ia32_lapic_calibrate_pit(apicid_t cpuNum, IPTR __APICBase)
{
    UQUAD lapic_initial, lapic_final, calibrated_lapic = 0;
    UQUAD difflapic[PIT_SAMPLECOUNT];
    UWORD pitresults[PIT_SAMPLECOUNT];
    ULONG pit_total = 0;
    UWORD pit_final;
    int samples, iter, i;

    samples = iter = PIT_SAMPLECOUNT;

    APIC_REG(__APICBase, APIC_TIMER_ICR) = APIC_CRMAXVAL;

    for (i = 0; i < PIT_SAMPLECOUNT; i++)
    {
        lapic_initial = APIC_REG(__APICBase, APIC_TIMER_CCR);

        pit_final   = pit_wait(11931);

        lapic_final = APIC_REG(__APICBase, APIC_TIMER_CCR);

        /* Ignore results that are invalid, or wrap around. */
        if ((pit_final >= PIT_WAITTICKS) && (lapic_initial > lapic_final))
        {
            difflapic[i] = lapic_initial - lapic_final;
            pitresults[i] = pit_final;
            pit_total += pit_final;
            DPIT(bug("[Kernel:APIC-IA32.%03u] %s: %u, diff = %u\n", cpuNum, __func__, pitresults[i], difflapic[i]);)
        }
        else
        {
            DPIT(bug("[Kernel:APIC-IA32.%03u] %s: ## %02u - ignoring pit_final = %u\n", cpuNum, __func__, (PIT_SAMPLECOUNT - samples), pit_final);)
            pitresults[i] = 0;
            samples -= 1;
        }
    }

    if (samples > 0)
    {
        UWORD pit_avg = (pit_total / samples), pit_error = pit_avg - ((pit_avg * (100 - (pit_avg / PIT_WAITTICKS))) / 100);

        DPIT(
            bug("[Kernel:APIC-IA32.%03u] %s: PIT Avg %u, error %u\n", cpuNum, __func__, pit_avg, pit_error);
            bug("[Kernel:APIC-IA32.%03u] %s:     Min %u\n", cpuNum, __func__, pit_avg - pit_error);
            bug("[Kernel:APIC-IA32.%03u] %s:     Max %u\n", cpuNum, __func__, pit_avg + pit_error);
        )

        for (i = 0; i < PIT_SAMPLECOUNT; i++)
        {
            if ((pitresults[i] >= pit_avg - pit_error) && (pitresults[i] <= pit_avg + pit_error))
                calibrated_lapic += (difflapic[i] * PIT_WAITTICKS) / pitresults[i];
            else
                iter -= 1;
        }

        if (iter > 0)
        {
            DPIT(
                for (i = 0; i < PIT_SAMPLECOUNT; i++)
                {
                  bug("[Kernel:APIC-IA32.%03u] %s: pit_final #%02u = %u (%llu)\n", cpuNum, __func__, i, pitresults[i], difflapic[i]);
                }
                bug("[Kernel:APIC-IA32.%03u] %s: iter: %u, freq: %llu\n", cpuNum, __func__, iter, (10 * iter * calibrated_lapic) / iter);
              )
            calibrated_lapic = (10 * iter * calibrated_lapic / iter);
        }
    }

    return calibrated_lapic;
}

static UQUAD ia32_tsc_calibrate(apicid_t cpuNum)
{
#if !defined(FORCE_PIT_CALIB)
    ULONG eax, ebx, ecx, edx;
    asm volatile("cpuid":"=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx):"a"(0x00000000));
    if ((ebx == 0x756e6547) &&
         (ecx == 0x6c65746e) &&
         (edx == 0x49656e69 ))
    {
        if (eax >= 0x15)
        {
            ULONG numerator, denominator;
            eax = ebx = ecx = edx = 0;
            asm volatile("cpuid":"=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx):"a"(0x00000015));
            DCALIB(bug("[Kernel:APIC-IA32.%03u] %s: numerator = %u, denominator = %u\n", cpuNum, __func__, ebx, eax);)
            if (((denominator = eax) != 0) && ((numerator = ebx) != 0))
            {
                if ((ecx / 1000) != 0)
                    return ecx * numerator / denominator;
                ULONG model;
                eax = ebx = ecx = edx = 0;
                asm volatile("cpuid":"=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx):"a"(0x00000001));
                model = ((eax & 0xF00000) >>14) | ((eax & 0xF0) >> 4);
                DCALIB(bug("[Kernel:APIC-IA32.%03u] %s: model = %02x\n", cpuNum, __func__, model);)
                if (model == 0x4E || model == 0x5E)
                    return 24000000 * numerator / denominator; // 24.0 MHz
                else if (model == 0x5C)
                    return 19200000 * numerator / denominator; // 19.2 MHz
            }
        }
    }
#endif
    return ia32_tsc_calibrate_pit(cpuNum);
}

static UQUAD ia32_lapic_calibrate(apicid_t cpuNum, IPTR __APICBase)
{
#if !defined(FORCE_PIT_CALIB)
    ULONG eax, ebx, ecx, edx;
    asm volatile("cpuid":"=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx):"a"(0x00000000));
    if ((ebx == 0x756e6547) &&
         (ecx == 0x6c65746e) &&
         (edx == 0x49656e69 ))
    {
        if (eax >= 0x16)
        {
            eax = ebx = ecx = edx = 0;
            asm volatile("cpuid":"=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx):"a"(0x00000016));
            DCALIB(bug("[Kernel:APIC-IA32.%03u] %s: eax = %08x\n", cpuNum, __func__, eax);)
            if (eax > 0)
                return (eax * 1000000);
        }
    }
#endif
    return ia32_lapic_calibrate_pit(cpuNum, __APICBase);
}

static BOOL APIC_isMSHyperV(void)
{
        ULONG arg = 1;
        ULONG res[4];

        asm volatile (
            "cpuid"
                : "=c"(res[0])
                : "a"(arg)
                : "%ebx", "%edx"
        );

        if (res[0] & 0x80000000) {
            /* Hypervisor Flag .. */
            arg = 0x40000000;
            asm volatile (
                "cpuid"
                    : "=a"(res[0]), "=b"(res[1]), "=c"(res[2]), "=d"(res[3])
                    : "a"(arg)
                    :
            );
            /*
             * res[0]           contains the number of CPUID functions
             * res[1 - 3]       contain the hypervisor signature...
            */
            if ((res[0] >= HYPERV_CPUID_MIN && res[0] <= HYPERV_CPUID_MAX) &&
                (res[1] == 0x7263694d) &&       /* 'r','c','i','M' */
                (res[2] == 0x666f736f) &&       /* 'f','o','s','o' */
                (res[3] == 0x76482074))         /* 'v','H',' ','t' */
            {
                    return TRUE;
            }
        }
        return FALSE;
}


/**********************************************************
                        Driver functions
 **********************************************************/

int APICHeartbeatFinalizer(struct ExceptionContext *regs, struct KernelBase *LIBBASE, struct ExecBase *SysBase)
{
    struct PlatformData *pdata = LIBBASE->kb_PlatformData;

    D(bug("[Kernel:APIC] %s(0x%p)\n", __func__, LIBBASE));

    pdata->kb_PDFlags &= ~PLATFORMF_HAVEHEARTBEAT;
}

static AROS_INTH1(APICResetHandler, struct KernelBase *, KernelBase)
{
    AROS_INTFUNC_INIT

    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    APTR ssp = NULL;

    D(bug("[Kernel:APIC] %s(0x%p)\n", __func__, KernelBase));

    /*
     * End the APIC heartbeat.
     */
    Forbid();

    KrnRemExceptionHandler(pdata->kb_APICHeartBeat);
    pdata->kb_APICHeartBeat = KrnAddExceptionHandler(APIC_EXCEPT_HEARTBEAT, APICHeartbeatFinalizer, KernelBase, SysBase);

    if ((KrnIsSuper()) || ((ssp = SuperState()) != NULL))
    {
        /* Cause the heartbeat to fire its last time .. */
        IPTR __APICBase = core_APIC_GetBase();
        APIC_REG(__APICBase, APIC_TIMER_ICR) = 0x0;

        if (ssp)
            UserState(ssp);
    }

    KrnSti();
    KrnCli();

    if ((KrnIsSuper()) || ((ssp = SuperState()) != NULL))
    {
        /* Now disable heartbeat timer */
        IPTR __APICBase = core_APIC_GetBase();
        APIC_REG(__APICBase, APIC_TIMER_VEC) = 0xFF;

        if (ssp)
            UserState(ssp);
    }

    Permit();

    D(bug("[Kernel:APIC] %s: Timer shutdown complete\n", __func__);)
    
    return 0;

    AROS_INTFUNC_EXIT
}

void core_APIC_Calibrate(struct APICData *apic, apicid_t cpuNum)
{
    IPTR __APICBase = apic->lapicBase;
    APTR ssp = NULL;

    DCALIB(bug("[Kernel:APIC-IA32.%03u] %s()\n", cpuNum, __func__);)

    /*
     * Calibrate LAPIC timer frequency.
     */

    /* Set the timer to one-shot mode, no interrupt, 1:1 divisor */
    APIC_REG(__APICBase, APIC_TIMER_VEC) = LVT_MASK | APIC_CPU_EXCEPT_TO_VECTOR(APIC_EXCEPT_HEARTBEAT);
    APIC_REG(__APICBase, APIC_TIMER_DIV) = TIMER_DIV_1;

    DCALIB(bug("[Kernel:APIC-IA32.%03u] %s: Calibrating timers ...\n", cpuNum, __func__));

    if (APIC_isMSHyperV())
    {
        if ((KrnIsSuper()) || ((ssp = SuperState()) != NULL))
        {
            ULONG res[2];

            DCALIB(bug("[Kernel:APIC-IA32.%03u] %s: Reading Hypervisor TSC/APIC frequencies...\n", cpuNum, __func__));

            asm volatile("rdmsr":"=a"(res[0]),"=d"(res[1]):"c"(0x40000022)); // Read TSC frequency
            apic->cores[cpuNum].cpu_TSCFreq = 100 * res[0];
            DCALIB(bug("[Kernel:APIC-IA32.%03u] %s:     MSR TSC frequency = %u\n", cpuNum, __func__, res[0]);)

            asm volatile("rdmsr":"=a"(res[0]),"=d"(res[1]):"c"(0x40000023)); // Read APIC frequency
            apic->cores[cpuNum].cpu_TimerFreq = res[0];
            DCALIB(bug("[Kernel:APIC-IA32.%03u] %s:     MSR APIC frequency = %u...\n", cpuNum, __func__, res[0]);)

            if (ssp)
                UserState(ssp);
        }
    }

    if (!apic->cores[cpuNum].cpu_TSCFreq)
    {
        UQUAD calib_tsc;
        int count = 10;

        DCALIB(bug("[Kernel:APIC-IA32.%03u] %s:     Calibrating TSC...\n", cpuNum, __func__);)
        while(((calib_tsc = ia32_tsc_calibrate(cpuNum)) == 0) && (count-- > 0));
        apic->cores[cpuNum].cpu_TSCFreq = calib_tsc;
    }
    if (apic->cores[cpuNum].cpu_TSCFreq < 100)
    {
        DCALIB(bug("[Kernel:APIC-IA32.%03u] %s: Fixup invalid TSC Freq (%u)\n", cpuNum, __func__, apic->cores[cpuNum].cpu_TSCFreq);)
        apic->cores[cpuNum].cpu_TSCFreq = 100;
    }

    if (!apic->cores[cpuNum].cpu_TimerFreq)
    {
        ULONG calib_lapic;

        DCALIB(bug("[Kernel:APIC-IA32.%03u] %s:     Calibrating LAPIC...\n", cpuNum, __func__);)
        calib_lapic = ia32_lapic_calibrate(cpuNum, __APICBase);
        apic->cores[cpuNum].cpu_TimerFreq = calib_lapic;
    }
    if (apic->cores[cpuNum].cpu_TimerFreq < 100)
    {
        DCALIB(bug("[Kernel:APIC-IA32.%03u] %s: Fixup invalid Timer Freq (%u)\n", cpuNum, __func__, apic->cores[cpuNum].cpu_TSCFreq);)
        apic->cores[cpuNum].cpu_TimerFreq = 100;
    }

    DCALIB(
        bug("[Kernel:APIC-IA32.%03u] %s: TSC frequency should be %u kHz (%u MHz)\n", cpuNum, __func__, (ULONG)((apic->cores[cpuNum].cpu_TSCFreq + 500)/1000), (ULONG)((apic->cores[cpuNum].cpu_TSCFreq + 500000) / 1000000));
        bug("[Kernel:APIC-IA32.%03u] %s: LAPIC frequency should be %u Hz (%u MHz)\n", cpuNum, __func__, apic->cores[cpuNum].cpu_TimerFreq, (apic->cores[cpuNum].cpu_TimerFreq + 500000) / 1000000);
    )
    /*
     * Once APIC timer has been calibrated -:
     * # Set it to run at its full frequency.
     * # Enable the heartbeat vector and use a suitable rate,
     *   otherwise set to reload every second and disable it.
     */
    if (cpuNum == 0)
    {
        struct PlatformData *pdata = KernelBase->kb_PlatformData;

        pdata->kb_APICHeartBeat = KrnAddExceptionHandler(APIC_EXCEPT_HEARTBEAT, APICHeartbeatServer, KernelBase, SysBase);
        pdata->kb_APICResetHandler.is_Node.ln_Pri = -62;
        pdata->kb_APICResetHandler.is_Node.ln_Name =
            KernelBase->kb_Node.ln_Name;
        pdata->kb_APICResetHandler.is_Code = (VOID_FUNC)APICResetHandler;
        pdata->kb_APICResetHandler.is_Data = KernelBase;
        AddResetCallback(&pdata->kb_APICResetHandler);

        apic->flags |= APF_TIMER;
        pdata->kb_PDFlags |= PLATFORMF_HAVEHEARTBEAT;
    }

    APIC_REG(__APICBase, APIC_TIMER_DIV) = TIMER_DIV_1;

    if ((apic->flags & APF_TIMER) &&
        ((KrnIsSuper()) || ((ssp = SuperState()) != NULL)))
    {
#if defined(__AROSEXEC_SMP__)
        tls_t *apicTLS = apic->cores[cpuNum].cpu_TLS;
        struct X86SchedulerPrivate *schedData = apicTLS->ScheduleData;
        DCALIB(bug("[Kernel:APIC-IA32.%03u] %s: tls @ 0x%p, scheduling data @ 0x%p\n", cpuNum, __func__, apicTLS, schedData);)
#endif

        apic->cores[cpuNum].cpu_LAPICTick = 0;
        DCALIB(bug("[Kernel:APIC-IA32.%03u] %s: heartbeat Exception Vector #$%02X (%d) set\n", cpuNum, __func__, APIC_EXCEPT_HEARTBEAT, APIC_CPU_EXCEPT_TO_VECTOR(APIC_EXCEPT_HEARTBEAT));)

        if (ssp)
            UserState(ssp);

#if defined(__AROSEXEC_SMP__)
        // TODO: Adjust based on the amount of work the APIC can do at its given frequency.
        schedData->Granularity = 1;
        schedData->Quantum = 5;
        APIC_REG(__APICBase, APIC_TIMER_ICR) = (apic->cores[cpuNum].cpu_TimerFreq);
#else
        APIC_REG(__APICBase, APIC_TIMER_ICR) = (apic->cores[cpuNum].cpu_TimerFreq + 25) / 50;
#endif
        APIC_REG(__APICBase, APIC_TIMER_VEC) = APIC_CPU_EXCEPT_TO_VECTOR(APIC_EXCEPT_HEARTBEAT); // | LVT_TMM_PERIOD;
    }
    else
    {
        APIC_REG(__APICBase, APIC_TIMER_ICR) = apic->cores[cpuNum].cpu_TimerFreq;
        APIC_REG(__APICBase, APIC_TIMER_VEC) = LVT_MASK | LVT_TMM_PERIOD | APIC_CPU_EXCEPT_TO_VECTOR(APIC_EXCEPT_HEARTBEAT);
    }

    DCALIB(bug("[Kernel:APIC-IA32.%03u] %s: Calibration complete\n", cpuNum, __func__);)
}

/* Configure the APIC */
void core_APIC_Config(IPTR __APICBase, apicid_t cpuNum)
{
    ULONG apic_ver = APIC_REG(__APICBase, APIC_VERSION);
    ULONG maxlvt = APIC_LVT(apic_ver);

    D(bug("[Kernel:APIC-IA32.%03u] %s(%p)\n", cpuNum, __func__, __APICBase);)

#ifdef CONFIG_LEGACY
    /* 82489DX doesn't report no. of LVT entries. */
    if (!APIC_INTEGRATED(apic_ver))
        maxlvt = 2;
#endif

    /* Use flat interrupt model with logical destination ID = 1 */
    APIC_REG(__APICBase, APIC_DFR) = DFR_FLAT;
    APIC_REG(__APICBase, APIC_LDR) = 1 << LDR_ID_SHIFT;

    D(bug("[Kernel:APIC-IA32.%03u] %s: APIC IRQ delivery mode configured\n", cpuNum, __func__);)

    /*
     * Set spurious IRQ vector -:
     *     APIC = enabled
     *     Focus Check = disabled
     *     EOI broadcast suppresion = disabled
     */
    APIC_REG(__APICBase, APIC_SVR) = SVR_ASE|APIC_CPU_EXCEPT_TO_VECTOR(APIC_EXCEPT_SPURIOUS);

    D(
        bug("[Kernel:APIC-IA32.%03u] %s: Initial LINT0 = %08x\n", cpuNum, __func__, APIC_REG(__APICBase, APIC_LINT0_VEC));
        bug("[Kernel:APIC-IA32.%03u] %s: Initial LINT1 = %08x\n", cpuNum, __func__, APIC_REG(__APICBase, APIC_LINT1_VEC));
    )

    /*
     * Set LINT0 to external and LINT1 to NMI.
     * These are common defaults and they are going to be overridden by ACPI tables.
     *
     * On all other LAPICs mask LINT0 and use some fake vector (0xff in this case),
     * otherwise LAPIC may throw an error.
     */
    if (cpuNum == 0)
        APIC_REG(__APICBase, APIC_LINT0_VEC) = LVT_MT_EXT;
    else
        APIC_REG(__APICBase, APIC_LINT0_VEC) = LVT_MASK | LVT_VEC_MASK;

    APIC_REG(__APICBase, APIC_LINT1_VEC) = LVT_MT_NMI;

#ifdef CONFIG_LEGACY
    /* Due to the Pentium erratum 3AP. */
    if (maxlvt > 3)
    {
         APIC_REG(__APICBase, APIC_ESR) = 0;
         APIC_REG(__APICBase, APIC_ESR) = 0;
    }
#endif

    /* Disable performance counter overflow interrupts, if supported */
    if (((APIC_REG(__APICBase, APIC_VERSION)>>16) & 0xFF) >= 4)
        APIC_REG(__APICBase, APIC_PCOUNT_VEC) = LVT_MASK;

    D(bug("[Kernel:APIC-IA32.%03u] %s: APIC ESR before enabling vector: %08x\n", cpuNum, __func__, APIC_REG(__APICBase, APIC_ESR)));

    /* Set APIC error interrupt to fixed vector interrupt "APIC_IRQ_ERROR",  on APIC error */
    APIC_REG(__APICBase, APIC_ERROR_VEC) = APIC_CPU_EXCEPT_TO_VECTOR(APIC_EXCEPT_ERROR);

    /* spec says clear errors after enabling vector. */
    if (maxlvt > 3)
    {
         APIC_REG(__APICBase, APIC_ESR) = 0;
         APIC_REG(__APICBase, APIC_ESR) = 0;
    }

    D(bug("[Kernel:APIC-IA32.%03u] %s: APIC ESR after enabling vector: %08x\n", cpuNum, __func__, APIC_REG(__APICBase, APIC_ESR)));
}

void core_APIC_Enable(IPTR __APICBase, apicid_t cpuNum)
{
    D(bug("[Kernel:APIC-IA32.%03u] %s(%p)\n", cpuNum, __func__, __APICBase);)

    /* Clear error status register using back-to-back writes */
    APIC_REG(__APICBase, APIC_ESR) = 0;
    APIC_REG(__APICBase, APIC_ESR) = 0;

    /* Ack any pending interrupts */
    APIC_REG(__APICBase, APIC_EOI) = 0;

    /* Send an Init Level De-Assert to synchronize arbitration ID's. */
    APIC_REG(__APICBase, APIC_ICRH) = 0;
    APIC_REG(__APICBase, APIC_ICRL) =  ICR_DSH_ALL | ICR_INT_LEVELTRIG | ICR_DM_INIT;
    while (APIC_REG(__APICBase, APIC_ICRL) & ICR_DS)
        ;

    D(bug("[Kernel:APIC-IA32.%03u] %s: Initial TPR: %08x\n", cpuNum, __func__, APIC_REG(__APICBase, APIC_TPR));)

    /* Set Task Priority to 'accept all interrupts' */
    APIC_REG(__APICBase, APIC_TPR) = 0;
}

/* Initialize the APIC */
void core_APIC_Init(struct APICData *apic, apicid_t cpuNum)
{
    IPTR __APICBase = apic->lapicBase;
    icintrid_t coreICInstID;

    D(bug("[Kernel:APIC-IA32.%03u] %s(%p, %p)\n", cpuNum, __func__, apic, __APICBase);)

    if ((coreICInstID = krnAddInterruptController(KernelBase, &APICInt_IntrController)) != (icintrid_t)-1)
    {
        APTR ssp = NULL;
        D(
            bug("[Kernel:APIC-IA32.%03u] %s: APIC IC ID #%d:%d\n", cpuNum, __func__, ICINTR_ICID(coreICInstID), ICINTR_INST(coreICInstID));
          )

        /*
         * NB: - BSP calls us in user mode, but AP's call us from supervisor
         */
        if ((KrnIsSuper()) || ((ssp = SuperState()) != NULL))
        {
            int i;
            /*
                    Obtain/set the critical IRQs and Vectors
                    core_SetExGates is run early on the BSP
                    via platform_init.c
                */
            if (cpuNum > 0)
                core_SetExGates((x86vectgate_t *)apic->cores[cpuNum].cpu_IDT);

            for (i = X86_CPU_EXCEPT_COUNT; i < APIC_EXCEPT_TOP; i++)
            {
                if ((i == APIC_EXCEPT_SYSCALL) ||
                    ((cpuNum == 0) && ((i == APIC_EXCEPT_HEARTBEAT)||(i == APIC_EXCEPT_SPURIOUS))))
                    continue;

                if (!core_SetIDTGate((x86vectgate_t *)apic->cores[cpuNum].cpu_IDT,
                                                APIC_CPU_EXCEPT_TO_VECTOR(i),
                                                (uintptr_t)IntrDefaultGates[APIC_CPU_EXCEPT_TO_VECTOR(i)],
                                                TRUE, FALSE))
                {
                    krnPanic(NULL, "Failed to set APIC Exception Vector\n"
                                    "Vector #$%02X\n", i);
                }
            }
            D(bug("[Kernel:APIC-IA32.%03u] %s: APIC Exception Vectors configured\n", cpuNum, __func__));

            if (cpuNum == 0)
            {
                KrnAddExceptionHandler(APIC_EXCEPT_ERROR, core_APICErrorHandle, KernelBase, NULL);

                D(bug("[Kernel:APIC-IA32.%03u] %s: APIC Error Exception handler (exception #$%02X) installed\n", cpuNum, __func__, APIC_EXCEPT_ERROR));

                for (i = APIC_EXCEPT_IPI_NOP; i <= APIC_EXCEPT_IPI_CAUSE; i++)
                {
                    KrnAddExceptionHandler(i, core_IPIHandle, (void *)((intptr_t)i - APIC_EXCEPT_IPI_NOP), KernelBase);
                }
                D(bug("[Kernel:APIC-IA32.%03u] %s: APIC IPI Vectors configured\n", cpuNum, __func__));
            }
            
            if (ssp)
                UserState(ssp);
        }
        else
        {
            krnPanic(NULL, "Failed to configure APIC\n"
                               "APIC #%03e ID %03u\n", cpuNum, apic->cores[cpuNum].cpu_LocalID);
        }

        D(bug("[Kernel:APIC-IA32.%03u] %s: APIC vectors/exceptions initialized\n", cpuNum, __func__);)

        if (cpuNum> 0)
        {
            core_APIC_Config(__APICBase, cpuNum);
            core_APIC_Calibrate(apic, cpuNum);
            core_APIC_Enable(__APICBase, cpuNum);
        }
    }
}

/*
 * Finalize the APIC.
 * performed during warm reboot, to set the APIC back to an initial state
 */
void core_APIC_Finalize(IPTR _APICBase)
{
    /* disable heartbeat timer */
    APIC_REG(_APICBase, APIC_TIMER_VEC) = 0xFF;
    APIC_REG(_APICBase, APIC_TIMER_ICR) = 0x10000;

    /* Clear error status register using back-to-back writes */
    APIC_REG(_APICBase, APIC_ESR) = 0;
    APIC_REG(_APICBase, APIC_ESR) = 0;

    APIC_REG(_APICBase, APIC_LINT1_VEC) = LVT_MT_NMI;
    APIC_REG(_APICBase, APIC_LINT0_VEC) = 0;

    /* disable spurious */
    APIC_REG(_APICBase, APIC_SVR) = 0;

    /* Ack any pending interrupts */
    APIC_REG(_APICBase, APIC_EOI) = 0;
}

apicid_t core_APIC_GetID(IPTR _APICBase)
{
    apicid_t _apic_id;

    /* The actual ID is in 8 most significant bits */
    _apic_id = APIC_REG(_APICBase, APIC_ID) >> APIC_ID_SHIFT;
    DID(bug("[Kernel:APIC-IA32] %s: %03u\n", __func__, _apic_id));

    return _apic_id;
}

ULONG core_APIC_Wake(APTR wake_apicstartrip, apicid_t wake_apicid, IPTR __APICBase)
{
    ULONG status_ipisend, status_ipirecv;
    ULONG start_count;
#ifdef CONFIG_LEGACY
    ULONG apic_ver = APIC_REG(__APICBase, APIC_VERSION);
#endif
    apicid_t cpuNo = KrnGetCPUNumber();
    DWAKE(
        bug("[Kernel:APIC-IA32.%03u] %s(%03u @ %p)\n", cpuNo, __func__, wake_apicid, wake_apicstartrip);
        bug("[Kernel:APIC-IA32.%03u] %s: Base @ %p\n", cpuNo, __func__, __APICBase);
    )
#ifdef CONFIG_LEGACY
    /*
     * Check if we have old 82489DX discrete APIC (version & 0xF0 == 0).
     * This APIC needs different startup procedure. It doesn't support STARTUP IPI
     * because old CPUs didn't have INIT signal. They jump to BIOS ROM boot code
     * immediately after INIT IPI. In order to run the bootstrap, a BIOS warm reset
     * magic has to be used there.
     */
    if (!APIC_INTEGRATED(apic_ver))
    {
        /*
         * BIOS warm reset magic, part one.
         * Write real-mode bootstrap routine address to 40:67 (real-mode address) location.
         * This is standard feature of IBM PC AT BIOS. If a warm reset condition is detected,
         * the BIOS jumps to the given address.
         */
        DWAKE(bug("[Kernel:APIC-IA32.%03u] %s: Setting BIOS vector for trampoline @ %p ..\n", cpuNo, __func__, wake_apicstartrip));
        *((volatile unsigned short *)0x469) = (IPTR)wake_apicstartrip >> 4;
        *((volatile unsigned short *)0x467) = 0; /* Actually wake_apicstartrip & 0x0F, it's page-aligned. */

        /*
         * BIOS warm reset magic, part two.
         * This writes 0x0A into CMOS RAM, location 0x0F. This signals a warm reset condition to BIOS,
         * making part one work.
         */
        DWAKE(bug("[Kernel:APIC-IA32.%03u] %s: Setting warm reset code ..\n", cpuNo, __func__));
        outb(0xf, 0x70);
        outb(0xa, 0x71);
    }
#endif

    /* Flush TLB (we are supervisor here) */
    wrcr(cr3, rdcr(cr3));

    /* First we send the INIT command (reset the core). Vector must be zero for this. */
    status_ipisend = ia32_ipi_send(__APICBase, wake_apicid, ICR_INT_LEVELTRIG | ICR_INT_ASSERT | ICR_DM_INIT);
    if (status_ipisend)
    {
        bug("[Kernel:APIC-IA32.%03u] %s: Error asserting INIT\n", cpuNo, __func__);
        return status_ipisend;
    }

    /* Deassert INIT after a small delay */
    krnClockSourceUdelay(10 * 1000);

    /* Deassert INIT to all - Intel docs says we should use shorthand here! */
    status_ipisend = ia32_ipi_send(__APICBase, wake_apicid, ICR_DSH_ALL | ICR_INT_LEVELTRIG | ICR_DM_INIT);
    if (status_ipisend)
    {
        bug("[Kernel:APIC-IA32.%03u] %s: Error deasserting INIT\n", cpuNo, __func__);
        return status_ipisend;
    }

    /* memory barrier */
    asm volatile("mfence":::"memory");

#ifdef CONFIG_LEGACY
    /* If it's 82489DX, we are done. */
    if (!APIC_INTEGRATED(apic_ver))
    {
        DWAKE(bug("[Kernel:APIC-IA32.%03u] %s: 82489DX detected, wakeup done\n", cpuNo, __func__));
        return 0;
    }
#endif

    /*
     * Perform IPI STARTUP loop.
     * According to official Intel specification, this must be done twice.
     * It's not explained why. ;-)
     */
    for (start_count = 1; start_count <= 2; start_count++)
    {
        DWAKE(bug("[Kernel:APIC-IA32.%03u] %s: Attempting STARTUP .. %u\n", cpuNo, __func__, start_count));

        /* Clear any pending error condition */
        APIC_REG(__APICBase, APIC_ESR) = 0;

        /*
         * Send STARTUP IPI.
         * The processor starts up at CS = (vector << 16) and IP = 0.
         */
        status_ipisend = ia32_ipi_send(__APICBase, wake_apicid, ICR_DM_STARTUP | ((IPTR)wake_apicstartrip >> 12));

        /* Allow the target APIC to accept the IPI */
        krnClockSourceUdelay(200);

#ifdef CONFIG_LEGACY
        /* Pentium erratum 3AP quirk */
        if (APIC_LVT(apic_ver) > 3)
            APIC_REG(__APICBase, APIC_ESR) = 0;
#endif

        status_ipirecv = APIC_REG(__APICBase, APIC_ESR) & 0xEF;

        /*
         * EXPERIMENTAL:
         * On my machine (macmini 3,1, as OS X system profiler says), the core starts up from first
         * attempt. The second attempt ends up in error (according to the documentation, the STARTUP
         * can be accepted only once, while the core in RESET or INIT state, and first STARTUP, if
         * successful, brings the core out of this state).
         * Here we try to detect this condition. If the core accepted STARTUP, we suggest that it has
         * started up, and break the loop.
         * A topic at osdev.org forum (http://forum.osdev.org/viewtopic.php?f=1&t=23018)
         * also tells about some problems with double STARTUP. According to it, the second STARTUP can
         * manage to re-run the core from the given address, leaving it in 64-bit mode, causing it to crash.
         *
         * If startup problems pops up (the core doesn't respond and AROS halts at "Launching APIC no X" stage),
         * the following two variations of this algorithm can be tried:
         * a) Always send STARTUP twice, but signal error condition only if both attempts failed.
         * b) Send first STARTUP, abort on error. Allow second attempt to fail and ignore its result.
         *
         *                                                              Sonic <pavel_fedin@mail.ru>
         */
        if (!status_ipisend && !status_ipirecv)
            break;
    }

    DWAKE(bug("[Kernel:APIC-IA32.%03u] %s: STARTUP run status 0x%08X, error 0x%08X\n", cpuNo, __func__, status_ipisend, status_ipirecv));

    /*
     * We return nonzero on error.
     * Actually least significant byte of this value holds ESR value, and 12th bit
     * holds delivery status flag from ia32_ipi_send() routine. It will be '1' if we got
     * stuck at sending phase.
     */
    return (status_ipisend | status_ipirecv);
}
