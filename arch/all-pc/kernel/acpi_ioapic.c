/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
    
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

    bug("%08X%08X", ((*tblData >> 32) & 0xFFFFFFFF), (*tblData & 0xFFFFFFFF));

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
        return (icid_t)-1;

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

    if (ACPI_FAILURE(status))
    {
        bug("[Kernel:IOAPIC] %s: Error evaluating _PIC: %s\n", __func__, AcpiFormatException(status));
        return (icid_t)-1;
    }

    DINT(bug("[Kernel:IOAPIC] %s: IOAPIC Mode Enabled (status=%08X)\n", __func__, status));

    return (icid_t)IOAPICInt_IntrController.ic_Node.ln_Type;
}

/*
 *
 */
void IOAPIC_IntDeliveryOptions(UBYTE pin, UBYTE pol, UBYTE trig, UBYTE *rtPol, UBYTE *rtTrig)
{
    if (pol == 0)
    {
        if (pin < I8259A_IRQCOUNT)
            *rtPol = 0;
        else
            *rtPol = 1; // low
    }
    else
    {
        if (pol == 1)
            *rtPol = 0;
        else
            *rtPol = 1; // low
    }
    if (trig == 0)
    {
        if (pin < I8259A_IRQCOUNT)
            *rtTrig = 0;
        else
            *rtTrig = 1; // level
    }
    else
    {
        if (trig == 1)
            *rtTrig = 1; // level
        else
            *rtTrig = 0;
    }
}

BOOL IOAPICInt_Init(struct KernelBase *KernelBase, icid_t instanceCount)
{
    struct PlatformData *kernPlatD = (struct PlatformData *)KernelBase->kb_PlatformData;
    struct IOAPICCfgData *ioapicData;
    struct IOAPICData *ioapicPrivate = kernPlatD->kb_IOAPIC;
    struct APICData *apicPrivate = kernPlatD->kb_APIC;
    int instance, irq = 0, ioapic_irqbase;
    struct IntrController *xtpicIC;

    DINT(bug("[Kernel:IOAPIC] %s(%u)\n", __func__, instanceCount));

    IOAPICInt_IntrController.ic_Private = ioapicPrivate;

    for (
            instance = 0;
            ((instance < instanceCount) && (ioapicData = &ioapicPrivate->ioapics[instance]));
            instance++
        )
    {
        ULONG ioapicval;
    
        ioapic_irqbase = ioapicData->ioapic_GSI;

        DINT(
            bug("[Kernel:IOAPIC] %s: Init IOAPIC #%u [ID=%03u] @ 0x%p\n", __func__, instance + 1, ioapicData->ioapic_ID, ioapicData->ioapic_Base);
        )

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
            DINT(
                bug("[Kernel:IOAPIC] %s: EOI present\n", __func__);
            )
        }

	/*
         * Make sure we are using the correct LocalID
         */
        ioapicval = acpi_IOAPIC_ReadReg(
            ioapicData->ioapic_Base,
            IOAPICREG_ID);
        if (ioapicData->ioapic_ID != ((ioapicval >> 24) & 0xF))
        {
            ioapicval &= ~(0xF << 24);
            ioapicval |= (ioapicData->ioapic_ID << 24);

            acpi_IOAPIC_WriteReg(ioapicData->ioapic_Base,
                IOAPICREG_ID,
                ioapicval);
            DINT(bug("[Kernel:IOAPIC] %s: IOAPIC LocalID configured\n", __func__);)
        }

        /* Check if the 8259A has been registered, and disable it */
        if ((instance == 0) && ((xtpicIC = krnFindInterruptController(KernelBase, ICTYPE_I8259A)) != NULL))
        {
            DINT(bug("[Kernel:IOAPIC] %s: Disabling i8259A controller...\n", __func__);)
            i8259a_Disable();
        }

        DINT(bug("[Kernel:IOAPIC] %s: Configuring IRQs & routing\n", __func__);)

        if ((ioapicData->ioapic_RouteTable = AllocMem(ioapicData->ioapic_IRQCount * sizeof(UQUAD), MEMF_ANY)) != NULL)
        {
            DINT(bug("[Kernel:IOAPIC] %s: Routing Data @ 0x%p\n", __func__, ioapicData->ioapic_RouteTable));
            for (irq = ioapic_irqbase; irq < (ioapic_irqbase + ioapicData->ioapic_IRQCount); irq++)
            {
                struct IntrMapping *intrMap = krnInterruptMapping(KernelBase, irq);
                struct acpi_ioapic_route *irqRoute;
                UBYTE ioapic_pin;
                UBYTE rtPol, rtTrig;
                BOOL enabled = FALSE;
                APTR ssp = NULL;

                if (intrMap)
                {
                    ioapic_pin = intrMap->im_Int - ioapic_irqbase;
                }
                else
                    ioapic_pin = irq - ioapic_irqbase;

                irqRoute = (struct acpi_ioapic_route *)&ioapicData->ioapic_RouteTable[ioapic_pin];

                DINT(bug("[Kernel:IOAPIC] %s: Route Entry %u @ 0x%p\n", __func__, ioapic_pin, irqRoute));

                ioapicval = acpi_IOAPIC_ReadReg(
                    ioapicData->ioapic_Base,
                    IOAPICREG_REDTBLBASE + (ioapic_pin << 1));
                ioapicData->ioapic_RouteTable[ioapic_pin] = (UQUAD)ioapicval;
               
                ioapicval = acpi_IOAPIC_ReadReg(
                    ioapicData->ioapic_Base,
                    IOAPICREG_REDTBLBASE + (ioapic_pin << 1) + 1);
                ioapicData->ioapic_RouteTable[ioapic_pin] |= ((UQUAD)ioapicval << 32);

                irqRoute->ds = 0;
                irqRoute->rirr = 0;
                if (intrMap)
                {
                    IOAPIC_IntDeliveryOptions(ioapic_pin, intrMap->im_Polarity, intrMap->im_Trig, &rtPol, &rtTrig);
                }
                else
                {
                    IOAPIC_IntDeliveryOptions(ioapic_pin, 0, 0, &rtPol, &rtTrig);
                }
                if (rtPol)
                    irqRoute->pol = 1;
                else
                    irqRoute->pol = 0;
                if (rtTrig)
                    irqRoute->trig = 1;
                else
                    irqRoute->trig = 0;
                
                if (intrMap)
                {
                    irqRoute->vect = (UBYTE)intrMap->im_Node.ln_Pri + HW_IRQ_BASE;
                    if (ictl_is_irq_enabled(intrMap->im_Node.ln_Pri, KernelBase))
                        enabled = TRUE;
                }
                else
                {
                    irqRoute->vect = irq + HW_IRQ_BASE;
                }
                /* setup delivery to the boot processor */
                D(bug("[Kernel:IOAPIC] %s: Routing HW IRQ to Vector #$%02X\n", __func__, irqRoute->vect);)
                irqRoute->dm = 0; // fixed
                irqRoute->dstm = 0; // physical
                irqRoute->mask = 1;
                if (apicPrivate)
                     irqRoute->dst = apicPrivate->cores[0].cpu_LocalID;
                else
                    irqRoute->dst = 0;

                if ((KrnIsSuper()) || ((ssp = SuperState()) != NULL))
                {
                    if (!krnInitInterrupt(KernelBase, irq, IOAPICInt_IntrController.ic_Node.ln_Type, instance))
                    {
                        bug("[Kernel:IOAPIC] %s: Failed to acquire IRQ #$%02X\n", __func__, irq);
                    }
                    else
                    {
                        if (!core_SetIRQGate(apicPrivate->cores[0].cpu_IDT, irq, (uintptr_t)IntrDefaultGates[HW_IRQ_BASE + irq]))
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
                DINT(
                    bug("[Kernel:IOAPIC] %s:       ", __func__);
                    ioapic_ParseTableEntry((UQUAD *)&ioapicData->ioapic_RouteTable[ioapic_pin]);
                    bug("\n");
                );
            }
            ioapicData->ioapic_Flags |= IOAPICF_ENABLED;
        }
        ioapic_irqbase += ioapicData->ioapic_IRQCount;
    }

    return TRUE;
}

BOOL IOAPICInt_DisableIRQ(APTR icPrivate, icid_t icInstance, icid_t intNum)
{
    struct IOAPICData *ioapicPrivate = (struct IOAPICData *)icPrivate;
    struct IOAPICCfgData *ioapicData = &ioapicPrivate->ioapics[icInstance];
    struct IntrMapping *intrMap = krnInterruptMapping(KernelBase, intNum);
    struct acpi_ioapic_route *irqRoute;
    UBYTE ioapic_pin;

    DINT(bug("[Kernel:IOAPIC] %s(%02X)\n", __func__, intNum));

    if (intrMap)
    {
        ioapic_pin = (intrMap->im_Int - ioapicData->ioapic_GSI);
    }
    else
        ioapic_pin = intNum - ioapicData->ioapic_GSI;

    DINT(bug("[Kernel:IOAPIC] %s: IOAPIC Pin %02X\n", __func__, ioapic_pin));
    irqRoute = (struct acpi_ioapic_route *)&ioapicData->ioapic_RouteTable[ioapic_pin];

    irqRoute->mask = 1;

    acpi_IOAPIC_WriteReg(ioapicData->ioapic_Base,
        IOAPICREG_REDTBLBASE + (ioapic_pin << 1),
        (ioapicData->ioapic_RouteTable[ioapic_pin] & 0xFFFFFFFF));
    acpi_IOAPIC_WriteReg(ioapicData->ioapic_Base,
        IOAPICREG_REDTBLBASE + (ioapic_pin << 1 ) + 1,
        ((ioapicData->ioapic_RouteTable[ioapic_pin] >> 32) & 0xFFFFFFFF));

    return TRUE;
}

BOOL IOAPICInt_EnableIRQ(APTR icPrivate, icid_t icInstance, icid_t intNum)
{
    struct PlatformData *kernPlatD = (struct PlatformData *)KernelBase->kb_PlatformData;
    struct IOAPICData *ioapicPrivate = (struct IOAPICData *)icPrivate;
    struct IOAPICCfgData *ioapicData = &ioapicPrivate->ioapics[icInstance];
    struct IntrMapping *intrMap = krnInterruptMapping(KernelBase, intNum);
    struct APICData *apicPrivate = kernPlatD->kb_APIC;
    struct acpi_ioapic_route *irqRoute;
    UBYTE ioapic_pin;

    DINT(
        bug("[Kernel:IOAPIC] %s(%02X)\n", __func__, intNum);
    )

    if (intrMap)
    {
        ioapic_pin = (intrMap->im_Int - ioapicData->ioapic_GSI);
    }
    else
         ioapic_pin = intNum - ioapicData->ioapic_GSI;

    DINT(
        bug("[Kernel:IOAPIC] %s: IOAPIC Pin %02X\n", __func__, ioapic_pin);
    )
    irqRoute = (struct acpi_ioapic_route *)&ioapicData->ioapic_RouteTable[ioapic_pin];

    /*
     * if we have APIC's get the ID from there
     * otherwise use the pre-configured one. */
    if (apicPrivate)
    {
        apicid_t cpuNo = KrnGetCPUNumber();
        irqRoute->dst = apicPrivate->cores[cpuNo].cpu_LocalID;
    }

    irqRoute->vect = intNum + HW_IRQ_BASE;
    irqRoute->mask = 0; // enable!!
    if (intrMap)
    {
        UBYTE rtPol, rtTrig;
        IOAPIC_IntDeliveryOptions(ioapic_pin, intrMap->im_Polarity, intrMap->im_Trig, &rtPol, &rtTrig);
        if (rtPol)
            irqRoute->pol = 1; // low
        else
            irqRoute->pol = 0;
        if (rtTrig)
            irqRoute->trig = 1; // lvl
        else
            irqRoute->trig = 0;
    }

    DINT(
        bug("[Kernel:IOAPIC] %s: ", __func__);
        ioapic_ParseTableEntry(&ioapicData->ioapic_RouteTable[ioapic_pin]);
        bug("\n");
    )

    acpi_IOAPIC_WriteReg(ioapicData->ioapic_Base,
        IOAPICREG_REDTBLBASE + (ioapic_pin << 1 ) + 1,
        ((ioapicData->ioapic_RouteTable[ioapic_pin] >> 32) & 0xFFFFFFFF));
    acpi_IOAPIC_WriteReg(ioapicData->ioapic_Base,
        IOAPICREG_REDTBLBASE + (ioapic_pin << 1),
        (ioapicData->ioapic_RouteTable[ioapic_pin] & 0xFFFFFFFF));

    return TRUE;
}

BOOL IOAPICInt_AckIntr(APTR icPrivate, icid_t icInstance, icid_t intNum)
{
    struct PlatformData *kernPlatD = (struct PlatformData *)KernelBase->kb_PlatformData;
    struct IOAPICData *ioapicPrivate = (struct IOAPICData *)icPrivate;
    struct IOAPICCfgData *ioapicData = &ioapicPrivate->ioapics[icInstance];
    struct IntrMapping *intrMap = krnInterruptMapping(KernelBase, intNum);
    struct acpi_ioapic_route *irqRoute;
    IPTR apic_base;
    UBYTE ioapic_pin;

    DINT(bug("[Kernel:IOAPIC] %s(%u)\n", __func__, intNum);)

    /* Write zero to EOI of APIC */
    apic_base = core_APIC_GetBase();
    DINT(bug("[Kernel:IOAPIC] %s: apicBase = %p\n", __func__, apic_base));
    APIC_REG(apic_base, APIC_EOI) = 0;

    /* write IOAPIC EIO if necessary .. */
    if (intrMap)
    {
        ioapic_pin = (intrMap->im_Int - ioapicData->ioapic_GSI);
    }
    else
         ioapic_pin = intNum - ioapicData->ioapic_GSI;

    DINT(
        bug("[Kernel:IOAPIC] %s: IOAPIC Pin %02X\n", __func__, ioapic_pin);
    )
    irqRoute = (struct acpi_ioapic_route *)&ioapicData->ioapic_RouteTable[ioapic_pin];

    if (irqRoute->trig == 1)
    {
        if (ioapicData->ioapic_Flags & IOAPICF_EOI)
        {
            volatile ULONG *ioapic_eoi = (volatile ULONG *)((IPTR)ioapicData->ioapic_Base + IOREGEOI);
            DINT(
                bug("[Kernel:IOAPIC] %s: Writting IOAPIC EOI REG (@ 0x%p, %u)\n", __func__, ioapic_eoi, irqRoute->vect);
            )
            *ioapic_eoi = irqRoute->vect;
        }
        else
        {
            int rttrig = irqRoute->trig, rtmask = irqRoute->mask;

            DINT(
                bug("[Kernel:IOAPIC] %s: Toggling trig/mask\n", __func__);
            )
            /*
             * If the IO-APIC is too old, Replicate the linux kernel behaviour
             * of setting the pin to edge trigger and back - masking the pin while changing
             */
            rttrig = irqRoute->trig;
            rtmask = irqRoute->mask;

            irqRoute->trig = 0;
            irqRoute->mask = 1;
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
    return TRUE;
}

struct IntrController IOAPICInt_IntrController =
{
    {
        .ln_Name = "82093AA IO-APIC",
        .ln_Pri = 50
    },
    0,
    AROS_MAKE_ID('I','O','9','3'),
    0,
    NULL,
    IOAPICInt_Register,
    IOAPICInt_Init,
    IOAPICInt_EnableIRQ,
    IOAPICInt_DisableIRQ,
    IOAPICInt_AckIntr
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

static UBYTE ACPI_PolFromInti(UWORD inti)
{
    switch (inti & ACPI_MADT_POLARITY_MASK) {
    default:
    case ACPI_MADT_POLARITY_CONFORMS:
    case ACPI_MADT_POLARITY_ACTIVE_HIGH:
            return 1;
    case ACPI_MADT_POLARITY_ACTIVE_LOW:
            return 2;
    }
}

static UBYTE ACPI_TrigFromInti(UWORD inti)
{
    switch (inti & ACPI_MADT_TRIGGER_MASK) {
    default:
    case ACPI_MADT_TRIGGER_CONFORMS:
    case ACPI_MADT_TRIGGER_EDGE:
            return 2;
    case ACPI_MADT_TRIGGER_LEVEL:
            return 1;
    }
}

/* Process the 'Interrupt Source' MADT Table */
AROS_UFH2(IPTR, ACPI_hook_Table_Int_Src_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(ACPI_MADT_INTERRUPT_SOURCE *, intsrc, A2))
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

        intrMap->im_Node.ln_Pri = intsrc->SourceIrq;
        //intrMap->im_Node.ln_Type = IOAPICInt_IntrController->;
        intrMap->im_Int = intsrc->GlobalIrq;

        intrMap->im_Polarity = ACPI_PolFromInti(intsrc->IntiFlags);
        intrMap->im_Trig = ACPI_TrigFromInti(intsrc->IntiFlags);

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
AROS_UFH2(IPTR, ACPI_hook_Table_Int_Src_Ovr_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(ACPI_MADT_INTERRUPT_OVERRIDE *, intsrc, A2))
{
    AROS_USERFUNC_INIT

    struct IntrMapping *intrMap;
    BOOL newRt = FALSE;

    DPARSE(
        bug("[Kernel:ACPI-IOAPIC] ## %s()\n", __func__);
    )

    if ((intrMap = krnInterruptMapping(KernelBase, intsrc->SourceIrq)) == NULL)
    {
        intrMap = AllocMem(sizeof(struct IntrMapping), MEMF_CLEAR);
        newRt = TRUE;
        DPARSE(bug("[Kernel:ACPI-IOAPIC]    %s: new mapping node @ 0x%p\n", __func__, intrMap);)

        intrMap->im_Node.ln_Pri = intsrc->SourceIrq;
    }
    else
    {
        DPARSE(bug("[Kernel:ACPI-IOAPIC]    %s: updating mapping node @ 0x%p\n", __func__, intrMap);)
    }

    intrMap->im_Int = intsrc->GlobalIrq;
    intrMap->im_Polarity = ACPI_PolFromInti(intsrc->IntiFlags);
    intrMap->im_Trig = ACPI_TrigFromInti(intsrc->IntiFlags);

    if (newRt)
    {
        Enqueue(&KernelBase->kb_InterruptMappings, &intrMap->im_Node);
    }

    DPARSE(
        bug("[Kernel:ACPI-IOAPIC]    %s: Bus %u, Source IRQ %u, GSI %u\n", __func__, intsrc->Bus, intsrc->SourceIrq, intsrc->GlobalIrq);
        bug("[Kernel:ACPI-IOAPIC]    %s: Flags 0x%x\n", __func__,intsrc->IntiFlags);
    )

    if (intsrc->SourceIrq != intsrc->GlobalIrq)
    {
        newRt = FALSE;
        if ((intrMap = krnInterruptMapping(KernelBase, intsrc->GlobalIrq)) == NULL)
        {
            intrMap = AllocMem(sizeof(struct IntrMapping), MEMF_CLEAR);
            newRt = TRUE;
            DPARSE(bug("[Kernel:ACPI-IOAPIC]    %s: new mapping node @ 0x%p\n", __func__, intrMap);)

            intrMap->im_Node.ln_Pri = intsrc->GlobalIrq;
        }
        else
        {
            DPARSE(bug("[Kernel:ACPI-IOAPIC]    %s: updating mapping node @ 0x%p\n", __func__, intrMap);)
        }

        intrMap->im_Int = intsrc->SourceIrq;

        intrMap->im_Polarity = ACPI_PolFromInti(intsrc->IntiFlags);
        intrMap->im_Trig = ACPI_TrigFromInti(intsrc->IntiFlags);

        if (newRt)
        {
            Enqueue(&KernelBase->kb_InterruptMappings, &intrMap->im_Node);
        }
    }
    return TRUE;

    AROS_USERFUNC_EXIT
}



/* Process the 'Non-Maskable Interrupt Source' MADT Table */
AROS_UFH2(IPTR, ACPI_hook_Table_NMI_Src_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(ACPI_MADT_NMI_SOURCE *, nmi_src, A2))
{
    AROS_USERFUNC_INIT

    struct IntrMapping *intrMap;
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
    }
    else
    {
        DPARSE(bug("[Kernel:ACPI-IOAPIC]    %s: updating mapping node @ 0x%p\n", __func__, intrMap);)
    }

    intrMap->im_Node.ln_Pri = nmi_src->GlobalIrq;
    intrMap->im_Int = nmi_src->GlobalIrq;

    intrMap->im_Polarity = ACPI_PolFromInti(nmi_src->IntiFlags);
    intrMap->im_Trig = ACPI_TrigFromInti(nmi_src->IntiFlags);

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
        icintrid_t ioapicICInstID;
        ULONG ioapicval;
        int i;

        bug("[Kernel:ACPI-IOAPIC] Registering IO-APIC #%u [ID=%03u] @ %p [GSI = %u]\n",
            pdata->kb_IOAPIC->ioapic_count + 1, ioapic->Id, ioapic->Address, ioapic->GlobalIrqBase);

        if ((ioapicICInstID = krnAddInterruptController(KernelBase, &IOAPICInt_IntrController)) != (icintrid_t)-1)
        {
            struct IOAPICCfgData *ioapicData = (struct IOAPICCfgData *)&pdata->kb_IOAPIC->ioapics[pdata->kb_IOAPIC->ioapic_count];

            DPARSE(bug("[Kernel:ACPI-IOAPIC] IO-APIC IC ID #%u:%u\n", ICINTR_ICID(ioapicICInstID), ICINTR_INST(ioapicICInstID)));

            ioapicData->ioapic_Base = (APTR)((IPTR)ioapic->Address);
            ioapicData->ioapic_GSI = ioapic->GlobalIrqBase;

            ioapicval = acpi_IOAPIC_ReadReg(
                ioapicData->ioapic_Base,
                IOAPICREG_ID);
            ioapicData->ioapic_ID = ioapic->Id; // we store the ACPI reported ID here, so we can check it during init.

            DPARSE(bug("[Kernel:ACPI-IOAPIC]    %s:       #%u,",
                __func__, ((ioapicval >> 24) & 0xF)));

            ioapicval = acpi_IOAPIC_ReadReg(
                ioapicData->ioapic_Base,
                IOAPICREG_VER);
            ioapicData->ioapic_IRQCount = ((ioapicval & IOAPICVER_MASKCNT) >> IOAPICVER_CNTSHIFT) + 1;
            ioapicData->ioapic_Ver = (ioapicval & IOAPICVER_MASKVER);
            DPARSE(bug(" ver %u, max irqs = %u,",
                ioapicData->ioapic_Ver, ioapicData->ioapic_IRQCount));
            ioapicval = acpi_IOAPIC_ReadReg(
                ioapicData->ioapic_Base,
                IOAPICREG_ARB);
            DPARSE(bug("arb %d\n", ((ioapicval >> 24) & 0xF)));

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

                DPARSE(
                    bug("[Kernel:ACPI-IOAPIC]    %s:       ", __func__);
                    ioapic_ParseTableEntry(&tblraw);
                    bug("\n");
                )
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

    if (cmdTags && strstr((const char *)cmdTags->ti_Data, "noioapic"))
    {
        D(bug("[Kernel:ACPI-IOAPIC] %s: IOAPIC Support Disabled\n", __func__));
        return;
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
