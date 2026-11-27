/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
    
    http://download.intel.com/design/chipsets/datashts/29056601.pdf
*/

#include <aros/macros.h>
#include <aros/asmcall.h>
#include <proto/exec.h>
#include <proto/arossupport.h>
#include <proto/acpica.h>

#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>

#include <asm/io.h>

#include <inttypes.h>
#include <string.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "kernel_globals.h"

#include "acpi.h"
#include "apic.h"
#include "apic_ia32.h"
#include "ioapic.h"

#define D(x)
#define DINT(x)
#define DPARSE(x)

#define ACPI_MODPRIO_IOAPIC       50

/************************************************************************************************
                                    ACPI IO-APIC RELATED FUNCTIONS
 ************************************************************************************************/

const char *ACPI_TABLE_MADT_STR __attribute__((weak)) = "APIC";

/* descriptor for an ioapic routing table entry */
struct acpi_ioapic_route
{
    uint32_t    vect:8, dm:3, dstm:1, ds:1, pol:1, rirr:1, trig:1, mask:1, rsvd1:15;
    uint32_t    rsvd2:24, dst:8;
};

static ULONG acpi_IOAPIC_ReadReg(APTR apic_base, UBYTE offset)
{
    *(ULONG volatile *)(apic_base + IOREGSEL) = offset;
    return *(ULONG volatile *)(apic_base + IOREGWIN);
}

static void acpi_IOAPIC_WriteReg(APTR apic_base, UBYTE offset, ULONG val)
{
    *(ULONG volatile *)(apic_base + IOREGSEL) = offset;
    *(ULONG volatile *)(apic_base + IOREGWIN) = val;
}

/* IO-APIC Interrupt Functions ... ***************************/

struct IOAPICInt_Private
{
    
};

void ioapic_ParseTableEntry(UQUAD *tblData)
{
    struct acpi_ioapic_route *tblEntry = (struct acpi_ioapic_route *)tblData;

    bug("%08X%08X", (ULONG)(((*tblData >> 32) & 0xFFFFFFFF)), (ULONG)((*tblData & 0xFFFFFFFF)));

    if (tblEntry->mask)
    {
        bug(" Disabled.");
    }
    else
    {
        bug(" DM:%02x", tblEntry->dm);
        if (tblEntry->ds)
        {
            bug(" DS");
        }
        if (tblEntry->rirr)
        {
            bug(" IRR");
        }
        if (tblEntry->pol)
        {
            bug(" Active LOW,");
        }
        else
        {
            bug(" Active HIGH,");
        }
        
        if (tblEntry->trig)
        {
            bug(" LEVEL");
        }
        else
        {
            bug(" EDGE");
        }

        bug(" ->");

        if (tblEntry->dstm)
        {
            bug(" Logical %03u:%03u", ((tblEntry->dst >> 4) & 0xF), (tblEntry->dst & 0xF));
        }
        else
        {
            bug(" Physical %03u", (tblEntry->dst & 0xF));
        }
    }
}

icid_t IOAPICInt_Register(struct KernelBase *KernelBase)
{
    ACPI_STATUS         status;
    ACPI_OBJECT         arg = { ACPI_TYPE_INTEGER };
    ACPI_OBJECT_LIST    arg_list = { 1, &arg };

    DINT(bug("[Kernel:IOAPIC] %s()\n", __func__));

    /* if we have been disabled, fail to register */
    if (IOAPICInt_IntrController.ic_Flags & ICF_DISABLED)
        return KRN_ICID_INVALID;

    /*
     * Inform ACPI/BIOS that we want to use IOAPIC mode...
     *   APIC IRQ model 0 = PIC       (default)
     *   APIC IRQ model 1 = IOAPIC
     *   APIC IRQ model 2 = SIOAPIC
     */
    arg.Integer.Value = 1;
    status = AcpiEvaluateObject(NULL,
                                (char *)"\\_PIC",
                                &arg_list,
                                NULL);
	if (status == AE_NOT_FOUND)
	{
        return KRN_ICID_INVALID;
	}
    if (ACPI_FAILURE(status))
    {
        bug("[Kernel:IOAPIC] %s: Error evaluating _PIC: %s\n", __func__, AcpiFormatException(status));
        return KRN_ICID_INVALID;
    }

    bug("[Kernel:IOAPIC] ACPI IOAPIC Mode Enabled\n");
    DINT(bug("[Kernel:IOAPIC] %s: mode enable status=%08X\n", __func__, status));

    return IOAPICInt_IntrController.ic_Id;
}

/*
 *
 */
void IOAPIC_IntDeliveryOptions(ULONG base, UBYTE pin, UBYTE pol, UBYTE trig, UBYTE *rtPol, UBYTE *rtTrig)
{
    if (pol == 0)
    {
        if ((base < I8259A_IRQCOUNT) &&
            ((base + pin) < I8259A_IRQCOUNT))
            *rtPol = 0;
        else
            *rtPol = 1;
    }
    else
    {
        if (pol == 1)
            *rtPol = 0;
        else
            *rtPol = 1;
    }
    if (trig == 0)
    {
        if ((base < I8259A_IRQCOUNT) &&
            ((base + pin) < I8259A_IRQCOUNT))
            *rtTrig = 0;
        else
            *rtTrig = 1;
    }
    else
    {
        if (trig == 1)
            *rtTrig = 1;
        else
            *rtTrig = 0;
    }
}

static BOOL IOAPIC_IntToPin(struct IOAPICCfgData *ioapicData,
    struct IntrMapping *intrMap, irqid_t intNum, UBYTE *ioapic_pin)
{
    ULONG intGsi = intrMap ? intrMap->im_Int : (ULONG)intNum;

    if (intGsi < ioapicData->ioapic_GSI ||
        intGsi >= (ioapicData->ioapic_GSI + ioapicData->ioapic_IRQCount))
    {
        bug("[Kernel:IOAPIC] %s: IRQ %u outside IOAPIC [GSI %u-%u)\n",
            __func__, intGsi, ioapicData->ioapic_GSI,
            ioapicData->ioapic_GSI + ioapicData->ioapic_IRQCount);
        return FALSE;
    }

    *ioapic_pin = (UBYTE)(intGsi - ioapicData->ioapic_GSI);
    return TRUE;
}

BOOL IOAPICInt_Init(struct KernelBase *KernelBase, icinstid_t instanceCount)
{
    struct PlatformData *kernPlatD = (struct PlatformData *)KernelBase->kb_PlatformData;
    struct IOAPICCfgData *ioapicData;
    struct IOAPICData *ioapicPrivate = kernPlatD->kb_IOAPIC;
    struct APICData *apicPrivate = kernPlatD->kb_APIC;
    int instance;
    ULONG irq = 0, ioapic_irqbase, max_irq;
    struct IntrController *xtpicIC;
    ACPI_TABLE_FADT *fadt = NULL;
    BYTE sciIRQ = 0;

    DINT(bug("[Kernel:IOAPIC] %s(%u)\n", __func__, instanceCount));

    IOAPICInt_IntrController.ic_Private = ioapicPrivate;

    if (kernPlatD->kb_ACPI)
            fadt = (ACPI_TABLE_FADT *)kernPlatD->kb_ACPI->acpi_fadt;
    if (fadt)
    {
        sciIRQ = fadt->SciInterrupt;
        bug("[Kernel:ACPI-IOAPIC] %s: SCI IRQ = %u\n", __func__, sciIRQ);
    }

    for (
            instance = 0;
            ((instance < instanceCount) && (ioapicData = &ioapicPrivate->ioapics[instance]));
            instance++
        )
    {
        ULONG ioapicval;
    
        ioapic_irqbase = ioapicData->ioapic_GSI;
        if (ioapic_irqbase >= HW_IRQ_COUNT)
        {
            bug("[Kernel:IOAPIC] %s: IOAPIC GSI base %u exceeds software IRQ limit %u, skipping\n",
                __func__, ioapic_irqbase, HW_IRQ_COUNT);
            continue;
        }

        if (ioapicData->ioapic_Flags & IOAPICF_DUMP)
        {
            bug("[Kernel:IOAPIC] %s: Init IOAPIC #%u [ID=%03u] @ 0x%p\n", __func__, instance + 1, ioapicData->ioapic_ID, ioapicData->ioapic_Base);
        }

        /*
         * Skip controllers that report bogus version information
         */
        ioapicval = acpi_IOAPIC_ReadReg(
                        ioapicData->ioapic_Base,
                        IOAPICREG_VER);
        if (ioapicval == 0xFFFFFFFF)
                continue;

        DINT(
            bug("[Kernel:IOAPIC] %s: IOAPIC Version appears to be valid...\n", __func__);
        )
        if (ioapicData->ioapic_Ver >= 0x20)
        {
            ioapicData->ioapic_Flags |= IOAPICF_EOI;
            if (ioapicData->ioapic_Flags & IOAPICF_DUMP)
            {
                bug("[Kernel:IOAPIC] %s: EOI present\n", __func__);
            }
        }

        /*
         * Make sure we are using the correct LocalID
         */
        ioapicval = acpi_IOAPIC_ReadReg(
            ioapicData->ioapic_Base,
            IOAPICREG_ID);
        if (ioapicData->ioapic_ID != ((ioapicval >> 24) & 0xF))
        {
            if (ioapicData->ioapic_Flags & IOAPICF_DUMP)
            {
                bug("[Kernel:IOAPIC] %s: Adjusting LocalID (= %03u)\n", __func__, ((ioapicval >> 24) & 0xF));
            }
            ioapicval &= ~(0xF << 24);
            ioapicval |= (ioapicData->ioapic_ID << 24);

            acpi_IOAPIC_WriteReg(ioapicData->ioapic_Base,
                IOAPICREG_ID,
                ioapicval);
            if (ioapicData->ioapic_Flags & IOAPICF_DUMP)
            {
                bug("[Kernel:IOAPIC] %s: IOAPIC LocalID configured\n", __func__);
            }
        }

        /* Check if the 8259A has been registered, and disable it */
        if ((instance == 0) && ((xtpicIC = krnFindInterruptController(KernelBase, IIC_ID_I8259A)) != NULL))
        {
            bug("[Kernel:IOAPIC] Disabling i8259A PIC controller...\n");
            i8259a_Disable();
        }

        DINT(bug("[Kernel:IOAPIC] %s: Configuring IRQs & routing\n", __func__);)

        if ((ioapicData->ioapic_RouteTable = AllocMem(ioapicData->ioapic_IRQCount * sizeof(UQUAD), MEMF_ANY)) != NULL)
        {
            DINT(bug("[Kernel:IOAPIC] %s: Routing Data @ 0x%p\n", __func__, ioapicData->ioapic_RouteTable));
            max_irq = ioapic_irqbase + ioapicData->ioapic_IRQCount;
            if (max_irq > HW_IRQ_COUNT)
            {
                bug("[Kernel:IOAPIC] %s: IOAPIC IRQ range %u-%u exceeds software IRQ limit (capping at %u)\n",
                    __func__, ioapic_irqbase, max_irq, HW_IRQ_COUNT);
                max_irq = HW_IRQ_COUNT;
            }
            for (irq = ioapic_irqbase; irq < max_irq; irq++)
            {
                struct IntrMapping *intrMap = krnInterruptMapping(KernelBase, irq);
                struct IntrMapping *intrTgt = krnInterruptMapped(KernelBase, irq);
                struct acpi_ioapic_route *irqRoute;
                UBYTE ioapic_pin = (UBYTE)(irq - ioapic_irqbase);
                UBYTE rtPol, rtTrig;
                BOOL enabled = FALSE;
                APTR ssp = NULL;

                irqRoute = (struct acpi_ioapic_route *)&ioapicData->ioapic_RouteTable[ioapic_pin];
                if (ioapicData->ioapic_Flags & IOAPICF_DUMP)
                {
                    bug("[Kernel:IOAPIC] %s: Route Entry %u @ 0x%p\n", __func__, ioapic_pin, irqRoute);
                }

                ioapicval = acpi_IOAPIC_ReadReg(
                    ioapicData->ioapic_Base,
                    IOAPICREG_REDTBLBASE + (ioapic_pin << 1));
                ioapicData->ioapic_RouteTable[ioapic_pin] = (UQUAD)ioapicval;
                irqRoute->mask = 1;
                irqRoute->rsvd1 = 0; /* Mask out the bits we shouldnt touch */
                acpi_IOAPIC_WriteReg(ioapicData->ioapic_Base,
                    IOAPICREG_REDTBLBASE + (ioapic_pin << 1),
                    (ioapicData->ioapic_RouteTable[ioapic_pin] & 0xFFFFFFFF));
                irqRoute->rsvd2 = 0;
                ioapicval = acpi_IOAPIC_ReadReg(
                    ioapicData->ioapic_Base,
                    IOAPICREG_REDTBLBASE + (ioapic_pin << 1) + 1);
                ioapicData->ioapic_RouteTable[ioapic_pin] |= ((UQUAD)ioapicval << 32);

                irqRoute->ds = 0;
                irqRoute->rirr = 0;
                if ((intrMap) && !(ioapicData->ioapic_Flags & IOAPICF_NORD))
                {
                    if (ioapicData->ioapic_Flags & IOAPICF_DUMP)
                    {
                        bug("[Kernel:IOAPIC] %s: IRQ #%u mapped (pol:%d trig:%d)\n", __func__, irq, intrMap->im_Polarity, intrMap->im_Trig);
                    }
                    IOAPIC_IntDeliveryOptions(ioapicData->ioapic_GSI, ioapic_pin, intrMap->im_Polarity, intrMap->im_Trig, &rtPol, &rtTrig);
                }
                else
                {
                    IOAPIC_IntDeliveryOptions(ioapicData->ioapic_GSI, ioapic_pin, 0, 0, &rtPol, &rtTrig);
                }
                if (rtPol)
                    irqRoute->pol = IOAPIC_INTPOL_LOW;
                else
                    irqRoute->pol = IOAPIC_INTPOL_HIGH;
                if (rtTrig)
                    irqRoute->trig = 1;
                else
                    irqRoute->trig = 0;

                if (intrTgt)
                {
                    if (intrTgt->im_DeviceIRQ < HW_IRQ_VECTORS)
                    {
                        irqRoute->vect = (UBYTE)intrTgt->im_DeviceIRQ + HW_IRQ_BASE;
                        apicRegisterVector(irqRoute->vect, intrTgt->im_DeviceIRQ);
                    }
                    else
                        irqRoute->vect = 0;
                    if (ictl_is_irq_enabled(irq, KernelBase))
                        enabled = TRUE;
                }
                else if (irq < HW_IRQ_VECTORS)
                {
                    /* 1:1 vector mapping fits in IDT (vectors 48-245) */
                    irqRoute->vect = irq + HW_IRQ_BASE;
                    /*
                     * Register the vector in the dispatch map. Legacy IRQs
                     * 0-15 are premapped by apicInitVectorMap(), but pins
                     * 16 and up (PCIe INTx GSIs) are not - without this,
                     * core_IRQHandle sees an unmapped vector and drops the
                     * interrupt, deadlocking any driver waiting on it.
                     */
                    apicRegisterVector(irqRoute->vect, irq);
                }
                else
                {
                    /* High GSI: vector allocated on demand in EnableIRQ */
                    irqRoute->vect = 0;
                }
                /* setup delivery to the boot processor */
                if (ioapicData->ioapic_Flags & IOAPICF_DUMP)
                {
                    bug("[Kernel:IOAPIC] %s: Routing HW IRQ to Vector #$%02X\n", __func__, irqRoute->vect);
                }
                irqRoute->dm = IOAPIC_DELMOD_FIXED;
                irqRoute->dstm = IOAPIC_DESTMOD_PHYS;
                if (apicPrivate)
                {
                    ULONG targetCpu = 0;
                    if (intrMap)
                    {
                        targetCpu = intrMap->im_CPU;
                        if (targetCpu >= apicPrivate->apic_count)
                        {
                            bug("[Kernel:IOAPIC] %s: IRQ %u affinity CPU %u invalid (max %u), using CPU0\n",
                                __func__, irq, targetCpu, apicPrivate->apic_count - 1);
                            targetCpu = 0;
                        }
                        intrMap->im_CPU = targetCpu;
                    }
                    irqRoute->dst = apicPrivate->cores[targetCpu].cpu_LocalID;
                }
                else
                {
                    if (intrMap)
                    {
                        intrMap->im_CPU = 0;
                    }
                    irqRoute->dst = 0;
                }

                if ((KrnIsSuper()) || ((ssp = SuperState()) != NULL))
                {
                    BOOL  acquired = TRUE, setgate = TRUE;
                    if ((!krnInitInterrupt(KernelBase, irq, IOAPICInt_IntrController.ic_Id, instance)) &&
                        !(KERNELIRQ_ICID(irq) == APICInt_IntrController.ic_Id))
                    {
                        bug("[Kernel:IOAPIC] %s: Failed to acquire IRQ #$%02X\n", __func__, irq);
                        acquired = FALSE;
                    }
                    else if ((KERNELIRQ_ICID(irq) == APICInt_IntrController.ic_Id) ||
                                ((xtpicIC) && (KERNELIRQ_ICID(irq) == xtpicIC->ic_Id)))
                    {
                        if (ioapicData->ioapic_Flags & IOAPICF_DUMP)
                        {
                            bug("[Kernel:IOAPIC] %s: Taking ownership of IRQ #$%02X\n", __func__, irq);
                        }
                        KERNELIRQ_ICID(irq) = APICInt_IntrController.ic_Id;
                        KERNELIRQ_ICINST(irq) = instance;
                        setgate = FALSE;
                    }

                    if (acquired)
                    {
                        /*
                         * The gate goes into the boot CPU's IDT, the only one
                         * there is yet. The other CPUs copy its device gates
                         * when they are started (smp_Entry).
                         */
                        if (irqRoute->vect == 0)
                        {
                            /* High GSI: no vector yet, gate set in EnableIRQ */
                            if (ioapicData->ioapic_Flags & IOAPICF_DUMP)
                            {
                                bug("[Kernel:IOAPIC] %s: IRQ %u deferred (vector on demand)\n", __func__, irq);
                            }
                        }
                        else if (setgate && !core_SetIRQGate(apicPrivate->cores[0].cpu_IDT, irq, (uintptr_t)IntrDefaultGates[HW_IRQ_BASE + irq]))
                        {
                            bug("[Kernel:IOAPIC] %s: failed to set IRQ %d's gate\n", __func__, irq);
                        }
                        if ((!intrMap) && (ictl_is_irq_enabled(irq, KernelBase)))
                            enabled = TRUE;
                    }
                    if (ssp)
                        UserState(ssp);
                }
                acpi_IOAPIC_WriteReg(ioapicData->ioapic_Base,
                    IOAPICREG_REDTBLBASE + (ioapic_pin << 1 ) + 1,
                    ((ioapicData->ioapic_RouteTable[ioapic_pin] >> 32) & 0xFFFFFFFF));
                acpi_IOAPIC_WriteReg(ioapicData->ioapic_Base,
                    IOAPICREG_REDTBLBASE + (ioapic_pin << 1),
                    (ioapicData->ioapic_RouteTable[ioapic_pin] & 0xFFFFFFFF));
                if (enabled)
                {
                    irqRoute->mask = 0;
                    acpi_IOAPIC_WriteReg(ioapicData->ioapic_Base,
                        IOAPICREG_REDTBLBASE + (ioapic_pin << 1),
                        (ioapicData->ioapic_RouteTable[ioapic_pin] & 0xFFFFFFFF));
                }
                if (ioapicData->ioapic_Flags & IOAPICF_DUMP)
                {
                    bug("[Kernel:IOAPIC] %s:       ", __func__);
                    ioapic_ParseTableEntry((UQUAD *)&ioapicData->ioapic_RouteTable[ioapic_pin]);
                    bug("\n");
                }
            }
            ioapicData->ioapic_Flags |= IOAPICF_ENABLED;
        }
        ioapic_irqbase += ioapicData->ioapic_IRQCount;
    }

    return TRUE;
}

BOOL IOAPICInt_DisableIRQ(APTR icPrivate, icinstid_t icInstance, irqid_t intNum)
{
    struct IOAPICData *ioapicPrivate = (struct IOAPICData *)icPrivate;
    struct IOAPICCfgData *ioapicData = &ioapicPrivate->ioapics[icInstance];
    struct IntrMapping *intrMap;
    struct acpi_ioapic_route *irqRoute;
    UBYTE ioapic_pin;
    IPTR lockState;

    if (ioapicData->ioapic_Flags & IOAPICF_DUMP)
    {
        bug("[Kernel:IOAPIC] %s(%02X)\n", __func__, intNum);
    }

    /*
     * A register is reached by selecting it and then using the window, and
     * the routing table shadows what was written. Neither survives another
     * CPU doing the same in between, an acknowledge included.
     */
    lockState = ictl_Lock();

    intrMap = krnInterruptMapping(KernelBase, intNum);
    if (!IOAPIC_IntToPin(ioapicData, intrMap, intNum, &ioapic_pin))
    {
        ictl_Unlock(lockState);
        return FALSE;
    }

    if (ioapicData->ioapic_Flags & IOAPICF_DUMP)
    {
        bug("[Kernel:IOAPIC] %s: IOAPIC Pin %02X\n", __func__, ioapic_pin);
    }
    irqRoute = (struct acpi_ioapic_route *)&ioapicData->ioapic_RouteTable[ioapic_pin];

    irqRoute->mask = 1;
    irqRoute->rsvd1 = 0; /* Mask out the bits we shouldnt touch */
    acpi_IOAPIC_WriteReg(ioapicData->ioapic_Base,
        IOAPICREG_REDTBLBASE + (ioapic_pin << 1),
        (ioapicData->ioapic_RouteTable[ioapic_pin] & 0xFFFFFFFF));
    irqRoute->rsvd2 = 0;
    acpi_IOAPIC_WriteReg(ioapicData->ioapic_Base,
        IOAPICREG_REDTBLBASE + (ioapic_pin << 1 ) + 1,
        ((ioapicData->ioapic_RouteTable[ioapic_pin] >> 32) & 0xFFFFFFFF));

    ictl_Unlock(lockState);

    return TRUE;
}

BOOL IOAPICInt_EnableIRQ(APTR icPrivate, icinstid_t icInstance, irqid_t intNum)
{
    struct PlatformData *kernPlatD = (struct PlatformData *)KernelBase->kb_PlatformData;
    struct IOAPICData *ioapicPrivate = (struct IOAPICData *)icPrivate;
    struct IOAPICCfgData *ioapicData = &ioapicPrivate->ioapics[icInstance];
    struct IntrMapping *intrMap;
    struct APICData *apicPrivate = kernPlatD->kb_APIC;
    struct acpi_ioapic_route *irqRoute;
    UBYTE ioapic_pin;
    IPTR lockState;

    if (ioapicData->ioapic_Flags & IOAPICF_DUMP)
    {
        bug("[Kernel:IOAPIC] %s(%02X)\n", __func__, intNum);
    }

    /* See IOAPICInt_DisableIRQ */
    lockState = ictl_Lock();

    intrMap = krnInterruptMapping(KernelBase, intNum);
    if (!IOAPIC_IntToPin(ioapicData, intrMap, intNum, &ioapic_pin))
    {
        ictl_Unlock(lockState);
        return FALSE;
    }

    if (ioapicData->ioapic_Flags & IOAPICF_DUMP)
    {
        bug("[Kernel:IOAPIC] %s: IOAPIC Pin %02X\n", __func__, ioapic_pin);
    }
    irqRoute = (struct acpi_ioapic_route *)&ioapicData->ioapic_RouteTable[ioapic_pin];

    /*
     * if we have APIC's get the ID from there
     * otherwise use the pre-configured one. */
    if (apicPrivate)
    {
        /*
         * A pin nothing has mapped goes to the boot CPU. It used to go to
         * whichever CPU the caller happened to run on.
         */
        ULONG cpuNo = 0;
        if (intrMap)
        {
            cpuNo = intrMap->im_CPU;
        }
        if (cpuNo >= apicPrivate->apic_count)
        {
            bug("[Kernel:IOAPIC] %s: IRQ %u affinity CPU %u invalid (max %u), using CPU0\n",
                __func__, intNum, cpuNo, apicPrivate->apic_count - 1);
            cpuNo = 0;
        }
        if (intrMap)
        {
            intrMap->im_CPU = cpuNo;
        }
        irqRoute->dst = apicPrivate->cores[cpuNo].cpu_LocalID;
    }

    /*
     * Lazy vector allocation for high-GSI pins (vect==0).
     * Low-GSI pins already have a 1:1 vector from init.
     * Don't blindly overwrite vect — the init-time vector is authoritative
     * for pins that already have one.
     */
    if (irqRoute->vect == 0)
    {
        UBYTE vector = 0;
        APTR ssp = NULL;

        /*
         * The gates are written in supervisor mode. That is entered with
         * a syscall, and none may be made with the lock held (ictl.c):
         * let go, enter, take it again and look again, as another CPU may
         * have given the pin its vector meanwhile.
         */
        ictl_Unlock(lockState);
        if (apicPrivate && !KrnIsSuper())
            ssp = SuperState();
        lockState = ictl_Lock();

        if (irqRoute->vect == 0)
        {
            vector = apicAllocVector(intNum);
            if (vector != 0)
            {
                irqRoute->vect = vector;

                /* Install IDT gate for the newly allocated vector on ALL CPUs */
                if (apicPrivate && KrnIsSuper())
                {
                    int c;
                    for (c = 0; c < apicPrivate->apic_count; c++)
                    {
                        if (apicPrivate->cores[c].cpu_IDT)
                        {
                            core_SetIDTGate(
                                (x86vectgate_t *)apicPrivate->cores[c].cpu_IDT,
                                vector,
                                (uintptr_t)IntrDefaultGates[vector],
                                TRUE, FALSE);
                        }
                    }
                }
                bug("[Kernel:IOAPIC] %s: IRQ %u allocated vector %u\n", __func__, intNum, vector);
            }
        }
        else
            vector = irqRoute->vect;

        ictl_Unlock(lockState);
        if (ssp)
            UserState(ssp);

        if (vector == 0)
        {
            bug("[Kernel:IOAPIC] %s: IRQ %u - no IDT vectors available!\n", __func__, intNum);
            return FALSE;
        }

        lockState = ictl_Lock();
    }

    /* Enable the pin (unmask) */
    irqRoute->mask = 0;
    if (ioapicData->ioapic_Flags & IOAPICF_DUMP)
    {
        bug("[Kernel:IOAPIC] %s: ", __func__);
        ioapic_ParseTableEntry(&ioapicData->ioapic_RouteTable[ioapic_pin]);
        bug("\n");
    }

    irqRoute->rsvd2 = 0;
    acpi_IOAPIC_WriteReg(ioapicData->ioapic_Base,
        IOAPICREG_REDTBLBASE + (ioapic_pin << 1 ) + 1,
        ((ioapicData->ioapic_RouteTable[ioapic_pin] >> 32) & 0xFFFFFFFF));
    irqRoute->rsvd1 = 0; /* Mask out the bits we shouldnt touch */
    acpi_IOAPIC_WriteReg(ioapicData->ioapic_Base,
        IOAPICREG_REDTBLBASE + (ioapic_pin << 1),
        (ioapicData->ioapic_RouteTable[ioapic_pin] & 0xFFFFFFFF));

    ictl_Unlock(lockState);

    return TRUE;
}

BOOL IOAPICInt_AckIntr(APTR icPrivate, icinstid_t icInstance, irqid_t intNum)
{
    struct PlatformData *kernPlatD = (struct PlatformData *)KernelBase->kb_PlatformData;
    struct IOAPICData *ioapicPrivate = (struct IOAPICData *)icPrivate;
    struct IOAPICCfgData *ioapicData = &ioapicPrivate->ioapics[icInstance];
    struct IntrMapping *intrMap;
    struct acpi_ioapic_route *irqRoute;
    IPTR apic_base;
    UBYTE ioapic_pin;
    IPTR lockState;

    DINT(bug("[Kernel:IOAPIC] %s(%u)\n", __func__, intNum);)

    /*
     * This runs on whichever CPU took the interrupt, while another may be
     * acknowledging or reprogramming a pin of the same IOAPIC: the mapping
     * lookup, the routing entry and, on an old IOAPIC, the register window
     * all need the others kept out.
     */
    lockState = ictl_Lock();

    intrMap = krnInterruptMapping(KernelBase, intNum);

    /* write IOAPIC EIO if necessary .. */
    if (!IOAPIC_IntToPin(ioapicData, intrMap, intNum, &ioapic_pin))
    {
         ictl_Unlock(lockState);
         return FALSE;
    }

    DINT(bug("[Kernel:IOAPIC] %s(%u): IOAPIC Pin %02X\n", __func__, intNum, ioapic_pin);)
    irqRoute = (struct acpi_ioapic_route *)&ioapicData->ioapic_RouteTable[ioapic_pin];

    if (irqRoute->trig == 1)
    {
        DINT(bug("[Kernel:IOAPIC] %s(%u): LEVEL\n", __func__, intNum);)

        if (ioapicData->ioapic_Flags & IOAPICF_EOI)
        {
            volatile ULONG *ioapic_eoi = (volatile ULONG *)((IPTR)ioapicData->ioapic_Base + IOREGEOI);
            DINT(bug("[Kernel:IOAPIC] %s(%u): Writting EOI (%u) to IOAPIC REG @ 0x%p\n", __func__, intNum, irqRoute->vect, ioapic_eoi);)
            *ioapic_eoi = irqRoute->vect;
        }
        else
        {
            int rttrig = irqRoute->trig, rtmask = irqRoute->mask;

            DINT(bug("[Kernel:IOAPIC] %s(%u): Toggling trig/mask\n", __func__, intNum);)
            /*
             * If the IO-APIC is too old, Replicate the linux kernel behaviour
             * of setting the pin to edge trigger and back - masking the pin while changing
             */
            rttrig = irqRoute->trig;
            rtmask = irqRoute->mask;

            irqRoute->trig = 0;
            irqRoute->mask = 1;
            irqRoute->rsvd1 = 0; /* Mask out the bits we shouldnt touch */
            acpi_IOAPIC_WriteReg(ioapicData->ioapic_Base,
                IOAPICREG_REDTBLBASE + (ioapic_pin << 1),
                (ioapicData->ioapic_RouteTable[ioapic_pin] & 0xFFFFFFFF));
            irqRoute->trig = rttrig;
            irqRoute->mask = rtmask;
            acpi_IOAPIC_WriteReg(ioapicData->ioapic_Base,
                IOAPICREG_REDTBLBASE + (ioapic_pin << 1),
                (ioapicData->ioapic_RouteTable[ioapic_pin] & 0xFFFFFFFF));
        }
    }
    else
    {
        DINT(bug("[Kernel:IOAPIC] %s(%u): EDGE\n", __func__, intNum);)
    }

    ictl_Unlock(lockState);

    /* Write zero to EOI of APIC */
    apic_base = core_APIC_GetBase();
    DINT(bug("[Kernel:IOAPIC] %s(%u): apicBase = %p\n", __func__, intNum, apic_base);)
    APIC_REG(apic_base, APIC_EOI) = 0;

    return TRUE;
}

#if defined(__AROSEXEC_SMP__)
/*
 * Send a pin to another CPU. Runs on the CPU the pin is delivered to now,
 * in supervisor mode with interrupts off (KrnModifyIRQA() gets it there
 * with a call IPI), because only that CPU can tell whether it still has
 * the interrupt to serve: one it has accepted stays in its local APIC when
 * the redirection entry changes, and would be served there while the next
 * one is already being served on the new CPU. A handler is entered on one
 * CPU at a time today, and drivers rely on it.
 *
 * So the entry is changed only when this CPU is not inside the dispatch of
 * a device interrupt and has this vector neither in service nor requested.
 * Otherwise the answer is IOAPIC_MOVE_BUSY and the caller asks again, by
 * when the interrupt has been served. A level triggered pin is masked in
 * the IOAPIC meanwhile, so that it cannot be raised here again; nothing is
 * lost, as the line stays asserted and is seen when the mask is taken off.
 * An edge triggered pin is never masked, an edge on a masked pin being
 * dropped. The routing table keeps the mask the pin should have, and
 * IOAPIC_MoveIRQEnd() puts that back if the move is given up.
 *
 * Only the destination changes. The vector is the same on every CPU, so an
 * EOI for an interrupt taken before the move still finds the entry and
 * clears its remote IRR.
 */
LONG IOAPIC_MoveIRQ(struct KernelBase *KernelBase, irqid_t intNum, ULONG cpuNo)
{
    struct PlatformData *kernPlatD = (struct PlatformData *)KernelBase->kb_PlatformData;
    struct IOAPICData *ioapicPrivate = kernPlatD->kb_IOAPIC;
    struct APICData *apicPrivate = kernPlatD->kb_APIC;
    struct IOAPICCfgData *ioapicData;
    struct CPUData *thisCPU;
    struct IntrMapping *intrMap;
    struct acpi_ioapic_route *irqRoute;
    LONG result = IOAPIC_MOVE_FAILED;
    UBYTE ioapic_pin;
    IPTR lockState;

    if (!ioapicPrivate || !apicPrivate || (cpuNo >= apicPrivate->apic_count) ||
        (KERNELIRQ_ICINST(intNum) >= ioapicPrivate->ioapic_count))
        return IOAPIC_MOVE_FAILED;

    ioapicData = &ioapicPrivate->ioapics[KERNELIRQ_ICINST(intNum)];
    if (!(ioapicData->ioapic_Flags & IOAPICF_ENABLED))
        return IOAPIC_MOVE_FAILED;

    thisCPU = core_APIC_GetCPUData(apicPrivate);

    /* See IOAPICInt_DisableIRQ */
    lockState = ictl_Lock();

    intrMap = krnInterruptMapping(KernelBase, intNum);
    if (thisCPU && IOAPIC_IntToPin(ioapicData, intrMap, intNum, &ioapic_pin))
    {
        ULONG curNo = intrMap ? intrMap->im_CPU : 0;
        BOOL sync = FALSE;

        irqRoute = (struct acpi_ioapic_route *)&ioapicData->ioapic_RouteTable[ioapic_pin];

        if (curNo == cpuNo)
        {
            /* There already; an earlier pass may have left the pin masked */
            result = IOAPIC_MOVE_DONE;
            sync = TRUE;
        }
        else if (!intrMap)
        {
            /* Nowhere to record the CPU: the caller makes the mapping first */
        }
        else if (curNo != (ULONG)(thisCPU - apicPrivate->cores))
        {
            /* Moved by somebody else since the caller looked */
            result = IOAPIC_MOVE_ELSEWHERE;
        }
        else
        {
            UBYTE vector = irqRoute->vect;
            BOOL pending = FALSE;

            irqRoute->rsvd1 = 0; /* Mask out the bits we shouldnt touch */
            irqRoute->rsvd2 = 0;

            if ((irqRoute->trig == 1) && !irqRoute->mask)
            {
                acpi_IOAPIC_WriteReg(ioapicData->ioapic_Base,
                    IOAPICREG_REDTBLBASE + (ioapic_pin << 1),
                    (ULONG)(ioapicData->ioapic_RouteTable[ioapic_pin] & 0xFFFFFFFF) | (1 << 16));
            }

            if (thisCPU->cpu_IntrFlags & PLATFORMF_INIRQ)
                pending = TRUE;
            else if (vector)
            {
                IPTR apic_base = core_APIC_GetBase();
                ULONG regOffs = (vector >> 5) << 4, regBit = 1UL << (vector & 31);

                if ((APIC_REG(apic_base, APIC_IRR + regOffs) | APIC_REG(apic_base, APIC_ISR + regOffs)) & regBit)
                    pending = TRUE;
            }

            if (pending)
                result = IOAPIC_MOVE_BUSY;
            else
            {
                intrMap->im_CPU = cpuNo;
                irqRoute->dst = apicPrivate->cores[cpuNo].cpu_LocalID;
                result = IOAPIC_MOVE_DONE;
                sync = TRUE;
            }
        }

        if (sync)
        {
            /* The destination first: the other write may take the mask off */
            irqRoute->rsvd1 = 0;
            irqRoute->rsvd2 = 0;
            acpi_IOAPIC_WriteReg(ioapicData->ioapic_Base,
                IOAPICREG_REDTBLBASE + (ioapic_pin << 1) + 1,
                ((ioapicData->ioapic_RouteTable[ioapic_pin] >> 32) & 0xFFFFFFFF));
            acpi_IOAPIC_WriteReg(ioapicData->ioapic_Base,
                IOAPICREG_REDTBLBASE + (ioapic_pin << 1),
                (ioapicData->ioapic_RouteTable[ioapic_pin] & 0xFFFFFFFF));
        }
    }

    ictl_Unlock(lockState);

    return result;
}

/*
 * A move that was given up may have left a level triggered pin masked.
 * Give the pin the mask the routing table has for it. Runs on any CPU.
 */
void IOAPIC_MoveIRQEnd(struct KernelBase *KernelBase, irqid_t intNum)
{
    struct PlatformData *kernPlatD = (struct PlatformData *)KernelBase->kb_PlatformData;
    struct IOAPICData *ioapicPrivate = kernPlatD->kb_IOAPIC;
    struct IOAPICCfgData *ioapicData;
    struct acpi_ioapic_route *irqRoute;
    UBYTE ioapic_pin;
    IPTR lockState;

    if (!ioapicPrivate || (KERNELIRQ_ICINST(intNum) >= ioapicPrivate->ioapic_count))
        return;

    ioapicData = &ioapicPrivate->ioapics[KERNELIRQ_ICINST(intNum)];
    if (!(ioapicData->ioapic_Flags & IOAPICF_ENABLED))
        return;

    /* See IOAPICInt_DisableIRQ */
    lockState = ictl_Lock();

    if (IOAPIC_IntToPin(ioapicData, krnInterruptMapping(KernelBase, intNum), intNum, &ioapic_pin))
    {
        irqRoute = (struct acpi_ioapic_route *)&ioapicData->ioapic_RouteTable[ioapic_pin];

        irqRoute->rsvd1 = 0; /* Mask out the bits we shouldnt touch */
        acpi_IOAPIC_WriteReg(ioapicData->ioapic_Base,
            IOAPICREG_REDTBLBASE + (ioapic_pin << 1),
            (ioapicData->ioapic_RouteTable[ioapic_pin] & 0xFFFFFFFF));
    }

    ictl_Unlock(lockState);
}
#endif /* __AROSEXEC_SMP__ */

struct IntrController IOAPICInt_IntrController =
{
    {
        .ln_Name = "82093AA IO-APIC",
        .ln_Pri = 50
    },
    .ic_Count       = 0,
    .ic_Type        = IIC_ID_IOAPIC,
    .ic_Flags       = 0,
    .ic_Private     = NULL,
    .ic_Register    = IOAPICInt_Register,
    .ic_Init        = IOAPICInt_Init,
    .ic_IntrEnable  = IOAPICInt_EnableIRQ,
    .ic_IntrDisable = IOAPICInt_DisableIRQ,
    .ic_IntrAck     = IOAPICInt_AckIntr
};
 
 /********************************************************************/
 
 void acpi_IOAPIC_AllocPrivate(struct PlatformData *pdata)
{
    if (!pdata->kb_IOAPIC)
    {
        pdata->kb_IOAPIC = AllocMem(sizeof(struct IOAPICData) + pdata->kb_ACPI->acpi_ioapicCnt * sizeof(struct IOAPICCfgData), MEMF_CLEAR);
        D(bug("[Kernel:ACPI-IOAPIC] IO-APIC Private @ 0x%p, for %u IOAPIC's\n", pdata->kb_IOAPIC, pdata->kb_ACPI->acpi_ioapicCnt));
    }
}

static UBYTE ACPI_PolFromInti(UBYTE src, UBYTE scriirq, UWORD inti)
{
    switch (inti & ACPI_MADT_POLARITY_MASK) {
    case ACPI_MADT_POLARITY_CONFORMS:
            if ((scriirq) && (src == scriirq))
                return 2;
    case ACPI_MADT_POLARITY_ACTIVE_HIGH:
            return 1;
    case ACPI_MADT_POLARITY_ACTIVE_LOW:
            return 2;
    default:
            return 0;
    }
}

static UBYTE ACPI_TrigFromInti(UBYTE src, UBYTE scriirq, UWORD inti)
{
    switch (inti & ACPI_MADT_TRIGGER_MASK) {
    case ACPI_MADT_TRIGGER_CONFORMS:
            if ((scriirq) && (src == scriirq))
                return 1;
    case ACPI_MADT_TRIGGER_EDGE:
            return 2;
    case ACPI_MADT_TRIGGER_LEVEL:
            return 1;
    default:
            return 0;
    }
}

/* Process the 'Interrupt Source' MADT Table */
AROS_UFH3(IPTR, ACPI_hook_Table_Int_Src_Parse,
          AROS_UFHA(struct Hook *, table_hook, A0),
          AROS_UFHA(ACPI_MADT_INTERRUPT_SOURCE *, intsrc, A2),
          AROS_UFHA(struct ACPI_TABLESCAN_DATA *, tsdata, A1))
{
    AROS_USERFUNC_INIT

    DPARSE(
        bug("[Kernel:ACPI-IOAPIC] ## %s()\n", __func__);
        bug("[Kernel:ACPI-IOAPIC]    %s: %u:%u, GSI %u\n", __func__, intsrc->Id, intsrc->Eid,
                intsrc->GlobalIrq);
        bug("[Kernel:ACPI-IOAPIC]    %s: Flags 0x%x\n", __func__,intsrc->IntiFlags);
        if (intsrc->Type == 1)
        {
            bug("[Kernel:ACPI-IOAPIC]    %s: PMI, vector %d\n", __func__, intsrc->IoSapicVector);
        }
        else if(intsrc->Type == 2)
        {
            bug("[Kernel:ACPI-IOAPIC]    %s: INIT\n", __func__);
        }
        else if(intsrc->Type == 3)
        {
            bug("[Kernel:ACPI-IOAPIC]    %s: Corrected\n", __func__);
        }
    )
#if (0)
    if ((intrMap = krnInterruptMapped(KernelBase, intsrc->GlobalIrq)) == NULL)
    {
        intrMap = AllocMem(sizeof(struct IntrMapping), MEMF_CLEAR);

        bug("[Kernel:ACPI-IOAPIC]    %s: new mapping node @ 0x%p\n", __func__, intrMap);

        intrMap->im_DeviceIRQ = intsrc->SourceIrq;
        //intrMap->im_Node.ln_Type = IOAPICInt_IntrController->;
        intrMap->im_Int = intsrc->GlobalIrq;

        intrMap->im_Polarity = ACPI_PolFromInti(intsrc->SourceIrq, intsrc->IntiFlags);
        intrMap->im_Trig = ACPI_TrigFromInti(intsrc->SourceIrq, intsrc->IntiFlags);

        Enqueue(&KernelBase->kb_InterruptMappings, &intrMap->im_Node);
    }
    else
    {
        bug("[Kernel:ACPI-IOAPIC]    %s: updating mapping node @ 0x%p\n", __func__, intrMap);
        intrMap->im_Int = intsrc->GlobalIrq;
        intrMap->im_Polarity = (intsrc->IntiFlags & 1) ? 1 : 0;
        intrMap->im_Trig = ((intsrc->IntiFlags >> 2) & 1) ? 0 : 1;
    }
#else
    bug("[Kernel:ACPI-IOAPIC]    %s: AROS LACKS SUPPORT FOR ACPI_MADT_INTERRUPT_SOURCE\n", __func__);
    bug("[Kernel:ACPI-IOAPIC]    %s: Speak to a developer about adding this functionality!\n", __func__);
#endif
    return TRUE;

    AROS_USERFUNC_EXIT
}

/* Process the 'Interrupt Source Overide' MADT Table */
AROS_UFH3(IPTR, ACPI_hook_Table_Int_Src_Ovr_Parse,
          AROS_UFHA(struct Hook *, table_hook, A0),
          AROS_UFHA(ACPI_MADT_INTERRUPT_OVERRIDE *, intsrc, A2),
          AROS_UFHA(struct ACPI_TABLESCAN_DATA *, tsdata, A1))
{
    AROS_USERFUNC_INIT

    struct PlatformData *pdata = tsdata->acpits_UserData;
    ACPI_TABLE_FADT *fadt = NULL;
    struct IntrMapping *intrMap;
    BYTE sciIRQ = 0;
    BOOL newRt = FALSE;

    DPARSE(
        bug("[Kernel:ACPI-IOAPIC] ## %s()\n", __func__);
    )

    if ((intrMap = krnInterruptMapping(KernelBase, intsrc->SourceIrq)) == NULL)
    {
        intrMap = AllocMem(sizeof(struct IntrMapping), MEMF_CLEAR);
        newRt = TRUE;
        DPARSE(bug("[Kernel:ACPI-IOAPIC]    %s: new mapping node @ 0x%p\n", __func__, intrMap);)

        intrMap->im_DeviceIRQ = intsrc->SourceIrq;
        /* The boot CPU; this is parsed there, but say so */
        intrMap->im_CPU = 0;
    }
    else
    {
        DPARSE(bug("[Kernel:ACPI-IOAPIC]    %s: updating mapping node @ 0x%p\n", __func__, intrMap);)
    }

    intrMap->im_Int = intsrc->GlobalIrq;
    
    if (pdata->kb_ACPI)
            fadt = (ACPI_TABLE_FADT *)pdata->kb_ACPI->acpi_fadt;
    if (fadt)
    {
        sciIRQ = fadt->SciInterrupt;
        bug("[Kernel:ACPI-IOAPIC]    %s: SCI IRQ = %u\n", __func__, sciIRQ);
    }

    intrMap->im_Polarity = ACPI_PolFromInti(intsrc->SourceIrq, sciIRQ, intsrc->IntiFlags);
    intrMap->im_Trig = ACPI_TrigFromInti(intsrc->SourceIrq, sciIRQ, intsrc->IntiFlags);

    if (newRt)
    {
        if (intrMap->im_DeviceIRQ < 32)
            pdata->kb_ACPI->acpi_interruptOverrides |= (1 << intrMap->im_DeviceIRQ);
        Enqueue(&KernelBase->kb_InterruptMappings, &intrMap->im_Node);
    }

    DPARSE(
        bug("[Kernel:ACPI-IOAPIC]    %s: Bus %u, Source IRQ %u, GSI %u\n", __func__, intsrc->Bus, intsrc->SourceIrq, intsrc->GlobalIrq);
        bug("[Kernel:ACPI-IOAPIC]    %s: Flags 0x%x\n", __func__,intsrc->IntiFlags);
    )

    return TRUE;

    AROS_USERFUNC_EXIT
}


/* Process the 'Non-Maskable Interrupt Source' MADT Table */
AROS_UFH3(IPTR, ACPI_hook_Table_NMI_Src_Parse,
          AROS_UFHA(struct Hook *, table_hook, A0),
          AROS_UFHA(ACPI_MADT_NMI_SOURCE *, nmi_src, A2),
          AROS_UFHA(struct ACPI_TABLESCAN_DATA *, tsdata, A1))
{
    AROS_USERFUNC_INIT

    struct PlatformData *pdata = tsdata->acpits_UserData;
    ACPI_TABLE_FADT *fadt = NULL;
    struct IntrMapping *intrMap;
    BYTE sciIRQ = 0;
    BOOL newRt = FALSE;

    DPARSE(
        bug("[Kernel:ACPI-IOAPIC] ## %s()\n", __func__);
        bug("[Kernel:ACPI-IOAPIC]    %s: GSI %u\n", __func__, nmi_src->GlobalIrq);
        bug("[Kernel:ACPI-IOAPIC]    %s: Flags 0x%x\n", __func__,nmi_src->IntiFlags);
    )

    if ((intrMap = krnInterruptMapped(KernelBase, nmi_src->GlobalIrq)) == NULL)
    {
        intrMap = AllocMem(sizeof(struct IntrMapping), MEMF_CLEAR);
        newRt = TRUE;

        DPARSE(bug("[Kernel:ACPI-IOAPIC]    %s: new mapping node @ 0x%p\n", __func__, intrMap);)
        /* The boot CPU, as for an override */
        intrMap->im_CPU = 0;
    }
    else
    {
        DPARSE(bug("[Kernel:ACPI-IOAPIC]    %s: updating mapping node @ 0x%p\n", __func__, intrMap);)
    }

    intrMap->im_DeviceIRQ = nmi_src->GlobalIrq;
    intrMap->im_Int = nmi_src->GlobalIrq;

    if (pdata->kb_ACPI)
            fadt = (ACPI_TABLE_FADT *)pdata->kb_ACPI->acpi_fadt;
    if (fadt)
    {
        sciIRQ = fadt->SciInterrupt;
        bug("[Kernel:ACPI-IOAPIC]    %s: SCI IRQ = %u\n", __func__, sciIRQ);
    }

    intrMap->im_Polarity = ACPI_PolFromInti(nmi_src->GlobalIrq, sciIRQ, nmi_src->IntiFlags);
    intrMap->im_Trig = ACPI_TrigFromInti(nmi_src->GlobalIrq, sciIRQ, nmi_src->IntiFlags);

    if (newRt)
    {
        Enqueue(&KernelBase->kb_InterruptMappings, &intrMap->im_Node);
    }
    return TRUE;

    AROS_USERFUNC_EXIT
}

/* Process the 'IO-APIC' MADT Table */
AROS_UFH3(IPTR, ACPI_hook_Table_IOAPIC_Parse,
          AROS_UFHA(struct Hook *, table_hook, A0),
          AROS_UFHA(ACPI_MADT_IO_APIC *, ioapic, A2),
          AROS_UFHA(struct ACPI_TABLESCAN_DATA *, tsdata, A1))
{
    AROS_USERFUNC_INIT

    struct PlatformData *pdata = tsdata->acpits_UserData;

    DPARSE(bug("[Kernel:ACPI-IOAPIC] ## %s()\n", __func__));

    if (!pdata->kb_IOAPIC)
    {
        acpi_IOAPIC_AllocPrivate(pdata);
    }

    if (pdata->kb_IOAPIC)
    {
        struct TagItem *cmdTags = LibFindTagItem(KRN_CmdLine, BootMsg);
        icintrid_t ioapicICInstID;
        ULONG ioapicval;
        int i;

        bug("[Kernel:ACPI-IOAPIC] Registering IO-APIC #%u [ID=%03u] @ %p [GSI = %u]\n",
            pdata->kb_IOAPIC->ioapic_count + 1, ioapic->Id, ioapic->Address, ioapic->GlobalIrqBase);

        if ((ioapicICInstID = krnAddInterruptController(KernelBase, &IOAPICInt_IntrController)) != KRN_ICINTR_INVALID)
        {
            struct IOAPICCfgData *ioapicData = (struct IOAPICCfgData *)&pdata->kb_IOAPIC->ioapics[pdata->kb_IOAPIC->ioapic_count];

            if (cmdTags)
            {
                if (strstr((const char *)cmdTags->ti_Data, "ioapicdump"))
                {
                    ioapicData->ioapic_Flags |= IOAPICF_DUMP;
                }
                if (strstr((const char *)cmdTags->ti_Data, "ioapicdefrd"))
                {
                    ioapicData->ioapic_Flags |= IOAPICF_NORD;
                }
            }
#if defined(DEBUG) && (DEBUG > 0)
            ioapicData->ioapic_Flags |= IOAPICF_DUMP;
#endif
            if (ioapicData->ioapic_Flags & IOAPICF_DUMP)
            {
                bug("[Kernel:ACPI-IOAPIC] IO-APIC IC ID #%u:%u\n", ICINTR_ICID(ioapicICInstID), ICINTR_INST(ioapicICInstID));
            }

            ioapicData->ioapic_Base = (APTR)((IPTR)ioapic->Address);
            ioapicData->ioapic_GSI = ioapic->GlobalIrqBase;

            ioapicval = acpi_IOAPIC_ReadReg(
                ioapicData->ioapic_Base,
                IOAPICREG_ID);
            ioapicData->ioapic_ID = ioapic->Id; // we store the ACPI reported ID here, so we can check it during init.

            if (ioapicData->ioapic_Flags & IOAPICF_DUMP)
            {
                bug("[Kernel:ACPI-IOAPIC]    %s:       #%u,",
                __func__, ((ioapicval >> 24) & 0xF));
            }

            ioapicval = acpi_IOAPIC_ReadReg(
                ioapicData->ioapic_Base,
                IOAPICREG_VER);
            ioapicData->ioapic_IRQCount = ((ioapicval & IOAPICVER_MASKCNT) >> IOAPICVER_CNTSHIFT) + 1;
            ioapicData->ioapic_Ver = (ioapicval & IOAPICVER_MASKVER);
            if (ioapicData->ioapic_Flags & IOAPICF_DUMP)
            {
                bug(" ver %u, max irqs = %u,", ioapicData->ioapic_Ver, ioapicData->ioapic_IRQCount);
            }
            ioapicval = acpi_IOAPIC_ReadReg(
                ioapicData->ioapic_Base,
                IOAPICREG_ARB);
            if (ioapicData->ioapic_Flags & IOAPICF_DUMP)
            {
                bug("arb %d\n", ((ioapicval >> 24) & 0xF));
            }

            for (i = 0; i < (ioapicData->ioapic_IRQCount << 1); i += 2)
            {
                UQUAD tblraw = 0;

                ioapicval = acpi_IOAPIC_ReadReg(
                    ioapicData->ioapic_Base,
                    IOAPICREG_REDTBLBASE + i);
                tblraw = (UQUAD)ioapicval;

                ioapicval = acpi_IOAPIC_ReadReg(
                    ioapicData->ioapic_Base,
                    IOAPICREG_REDTBLBASE + i + 1);
                tblraw |= ((UQUAD)ioapicval << 32);

                if (ioapicData->ioapic_Flags & IOAPICF_DUMP)
                {
                    bug("[Kernel:ACPI-IOAPIC]    %s:       ", __func__);
                    ioapic_ParseTableEntry(&tblraw);
                    bug("\n");
                }
            }

            pdata->kb_IOAPIC->ioapic_count++;
        }
    }

    DPARSE(bug("[Kernel:ACPI-IOAPIC] %s: ioapics configuration done\n", __func__);)

    return TRUE;

    AROS_USERFUNC_EXIT
}

/*
 * Process the 'IO-APIC' MADT Table
 * This function counts the available IO-APICs.
 */
AROS_UFH3(static IPTR, ACPI_hook_Table_IOAPIC_Count,
          AROS_UFHA(struct Hook *, table_hook, A0),
          AROS_UFHA(ACPI_MADT_IO_APIC *, ioapic, A2),
          AROS_UFHA(struct ACPI_TABLESCAN_DATA *, tsdata, A1))
{
    AROS_USERFUNC_INIT

    struct PlatformData *pdata = tsdata->acpits_UserData;
    struct ACPI_TABLE_HOOK *scanHook;

    DPARSE(bug("[Kernel:ACPI-IOAPIC] ## %s()\n", __func__));

    if (pdata->kb_ACPI->acpi_ioapicCnt == 0)
    {
        struct TagItem *cmdTags = LibFindTagItem(KRN_CmdLine, BootMsg);
        BOOL parseoverride = TRUE;

        if (cmdTags)
        {
            if (strstr((const char *)cmdTags->ti_Data, "nointover"))
            {
                D(bug("[Kernel:ACPI-IOAPIC] %s: Interrupt Override Disabled\n", __func__));
                parseoverride = FALSE;
            }
        }

        DPARSE(bug("[Kernel:ACPI-IOAPIC]    %s: Registering IO-APIC Table Parser...\n", __func__));

        scanHook = (struct ACPI_TABLE_HOOK *)AllocMem(sizeof(struct ACPI_TABLE_HOOK), MEMF_CLEAR);
        if (scanHook)
        {
            scanHook->acpith_Node.ln_Name = (char *)ACPI_TABLE_MADT_STR;
            scanHook->acpith_Node.ln_Pri = ACPI_MODPRIO_IOAPIC - 10;                            /* Queue 10 priority levels after the module parser */
            scanHook->acpith_Hook.h_Entry = (APTR)ACPI_hook_Table_IOAPIC_Parse;
            scanHook->acpith_HeaderLen = sizeof(ACPI_TABLE_MADT);
            scanHook->acpith_EntryType = ACPI_MADT_TYPE_IO_APIC;
            scanHook->acpith_UserData = pdata;
            Enqueue(&pdata->kb_ACPI->acpi_tablehooks, &scanHook->acpith_Node);
        }

        scanHook = (struct ACPI_TABLE_HOOK *)AllocMem(sizeof(struct ACPI_TABLE_HOOK), MEMF_CLEAR);
        if (scanHook)
        {
            DPARSE(bug("[Kernel:ACPI-IOAPIC]    %s: Registering Interrupt Source Table Parser...\n", __func__));

            scanHook->acpith_Node.ln_Name = (char *)ACPI_TABLE_MADT_STR;
            scanHook->acpith_Node.ln_Pri = ACPI_MODPRIO_IOAPIC - 20;                            /* Queue 20 priority levels after the module parser */
            scanHook->acpith_Hook.h_Entry = (APTR)ACPI_hook_Table_Int_Src_Parse;
            scanHook->acpith_HeaderLen = sizeof(ACPI_TABLE_MADT);
            scanHook->acpith_EntryType = ACPI_MADT_TYPE_INTERRUPT_SOURCE;
            scanHook->acpith_UserData = pdata;
            Enqueue(&pdata->kb_ACPI->acpi_tablehooks, &scanHook->acpith_Node);
        }

        if (parseoverride)
        {
            scanHook = (struct ACPI_TABLE_HOOK *)AllocMem(sizeof(struct ACPI_TABLE_HOOK), MEMF_CLEAR);
            if (scanHook)
            {
                DPARSE(bug("[Kernel:ACPI-IOAPIC]    %s: Registering Interrupt Override Table Parser...\n", __func__));

                scanHook->acpith_Node.ln_Name = (char *)ACPI_TABLE_MADT_STR;
                scanHook->acpith_Node.ln_Pri = ACPI_MODPRIO_IOAPIC - 30;                            /* Queue 30 priority levels after the module parser */
                scanHook->acpith_Hook.h_Entry = (APTR)ACPI_hook_Table_Int_Src_Ovr_Parse;
                scanHook->acpith_HeaderLen = sizeof(ACPI_TABLE_MADT);
                scanHook->acpith_EntryType = ACPI_MADT_TYPE_INTERRUPT_OVERRIDE;
                scanHook->acpith_UserData = pdata;
                Enqueue(&pdata->kb_ACPI->acpi_tablehooks, &scanHook->acpith_Node);
            }
        }
        scanHook = (struct ACPI_TABLE_HOOK *)AllocMem(sizeof(struct ACPI_TABLE_HOOK), MEMF_CLEAR);
        if (scanHook)
        {
            DPARSE(bug("[Kernel:ACPI-IOAPIC]    %s: Registering NMI Source Table Parser...\n", __func__));

            scanHook->acpith_Node.ln_Name = (char *)ACPI_TABLE_MADT_STR;
            scanHook->acpith_Node.ln_Pri = ACPI_MODPRIO_IOAPIC - 40;                            /* Queue 40 priority levels after the module parser */
            scanHook->acpith_Hook.h_Entry = (APTR)ACPI_hook_Table_NMI_Src_Parse;
            scanHook->acpith_HeaderLen = sizeof(ACPI_TABLE_MADT);
            scanHook->acpith_EntryType = ACPI_MADT_TYPE_NMI_SOURCE;
            scanHook->acpith_UserData = pdata;
            Enqueue(&pdata->kb_ACPI->acpi_tablehooks, &scanHook->acpith_Node);
        }
    }
    pdata->kb_ACPI->acpi_ioapicCnt++;

    return TRUE;

    AROS_USERFUNC_EXIT
}


void ACPI_IOAPIC_SUPPORT(struct PlatformData *pdata)
{
    struct ACPI_TABLE_HOOK *scanHook;
    struct TagItem *cmdTags = LibFindTagItem(KRN_CmdLine, BootMsg);

    if (cmdTags)
    {
        if (strstr((const char *)cmdTags->ti_Data, "noioapic"))
        {
            D(bug("[Kernel:ACPI-IOAPIC] %s: IOAPIC Support Disabled\n", __func__));
            return;
        }
    }

    scanHook = (struct ACPI_TABLE_HOOK *)AllocMem(sizeof(struct ACPI_TABLE_HOOK), MEMF_CLEAR);
    if (scanHook)
    {
        D(bug("[Kernel:ACPI-IOAPIC] %s: Registering IOAPIC Table Parser...\n", __func__));
        D(bug("[Kernel:ACPI-IOAPIC] %s: Table Hook @ 0x%p\n", __func__, scanHook));
        scanHook->acpith_Node.ln_Name = (char *)ACPI_TABLE_MADT_STR;
        scanHook->acpith_Node.ln_Pri = ACPI_MODPRIO_IOAPIC;
        scanHook->acpith_Hook.h_Entry = (APTR)ACPI_hook_Table_IOAPIC_Count;
        scanHook->acpith_HeaderLen = sizeof(ACPI_TABLE_MADT);
        scanHook->acpith_EntryType = ACPI_MADT_TYPE_IO_APIC;
        scanHook->acpith_UserData = pdata;
        Enqueue(&pdata->kb_ACPI->acpi_tablehooks, &scanHook->acpith_Node);
    }
    D(bug("[Kernel:ACPI-IOAPIC] %s: Registering done\n", __func__));
}

DECLARESET(KERNEL__ACPISUPPORT)
ADD2SET(ACPI_IOAPIC_SUPPORT, KERNEL__ACPISUPPORT, 0)
