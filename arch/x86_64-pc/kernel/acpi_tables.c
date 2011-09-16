/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/asmcall.h>
#include <resources/acpi.h>
#include <proto/arossupport.h>
#include <proto/acpi.h>

#include <inttypes.h>
#include <string.h>

#include "kernel_base.h"
#include "kernel_bootmem.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "acpi.h"
#include "smp.h"

#define D(x) x
#define CONFIG_LAPICS	/* Disabling this disables secondary CPUs startup */

/************************************************************************************************
                                    ACPI RELATED FUNCTIONS
 ************************************************************************************************/

static const struct Hook ACPI_TableParse_LAPIC_Addr_Ovr_hook =
{
    .h_Entry = (APTR)ACPI_hook_Table_LAPIC_Addr_Ovr_Parse
};

static const struct Hook ACPI_TableParse_LAPIC_hook =
{
    .h_Entry = (APTR)ACPI_hook_Table_LAPIC_Parse
};

struct Hook ACPI_TableParse_LAPIC_NMI_hook =
{
    .h_Entry = (APTR)ACPI_hook_Table_LAPIC_NMI_Parse
};

struct Hook ACPI_TableParse_IOAPIC_hook =
{
    .h_Entry = (APTR)ACPI_hook_Table_IOAPIC_Parse
};

static const struct Hook ACPI_TableParse_Int_Src_Ovr_hook =
{
    .h_Entry = (APTR)ACPI_hook_Table_Int_Src_Ovr_Parse
};

static const struct Hook ACPI_TableParse_NMI_Src_hook =
{
    .h_Entry = (APTR)ACPI_hook_Table_NMI_Src_Parse
};

ULONG core_ACPIInitialise(void)
{
    struct Hook ACPI_TableParse_LAPIC_count_hook =
    {
    	.h_Entry = (APTR)ACPI_hook_Table_LAPIC_Count,
    	.h_Data  = NULL
    };

    IPTR result;
    struct ACPI_TABLE_TYPE_MADT *madt;
    struct ACPI_TABLE_TYPE_HPET *hpet;
    struct ACPIBase *ACPIBase = OpenResource("acpi.resource");

    D(bug("[Kernel] core_ACPIInitialise()\n"));

    if (!ACPIBase)
    {
    	D(bug("[Kernel] acpi.resource not found, no ACPI\n"));
    	return 0;
    }

    /* 
     * MADT : If it exists, parse the Multiple APIC Description Table "MADT", 
     * This table provides platform SMP configuration information [the successor to MPS tables]
     */
    madt = ACPI_FindSDT(ACPI_MAKE_ID('A', 'P', 'I', 'C'));
    D(bug("[Kernel] MADT at 0x%p\n", madt));

    if (!madt)
    	return 0;

    ACPI_Table_MADT_Parse(madt);

    /* 
     * Local APIC : The LAPIC address is obtained from the MADT (32-bit value)
     * and (optionally) overriden by a LAPIC_ADDR_OVR entry (64-bit value).
     */
    ACPI_ScanEntries(&madt->header, ACPI_MADT_LAPIC_ADDR_OVR, &ACPI_TableParse_LAPIC_Addr_Ovr_hook, NULL);
    ACPI_ScanEntries(&madt->header, ACPI_MADT_INT_SRC_OVR, &ACPI_TableParse_Int_Src_Ovr_hook, NULL);

    /*
     * Now get ready to set up secondary CPUs. First we just want to count number of APICs.
     * This hook function uses h_Data as a counter.
     */
    ACPI_ScanEntries(&madt->header, ACPI_MADT_LAPIC, &ACPI_TableParse_LAPIC_count_hook, NULL);
    D(bug("[Kernel] core_ACPIInitialise: ACPI found %lu enabled APICs\n", ACPI_TableParse_LAPIC_count_hook.h_Data));

#ifdef CONFIG_LAPICS
    if ((IPTR)ACPI_TableParse_LAPIC_count_hook.h_Data > 1)
    { 
	if (smp_Setup((IPTR)ACPI_TableParse_LAPIC_count_hook.h_Data))
    	{
    	    D(bug("[Kernel] Succesfully prepared SMP enviromnent\n"));

	    /* This will actually run secondary CPUs */
	    ACPI_ScanEntries(&madt->header, ACPI_MADT_LAPIC, &ACPI_TableParse_LAPIC_hook, NULL);
	    D(bug("[Kernel] core_ACPIInitialise: System Total APICs: %d\n", KernelBase->kb_CPUCount));
	}
    }
#endif

    result = ACPI_ScanEntries(&madt->header, ACPI_MADT_LAPIC_NMI, &ACPI_TableParse_LAPIC_NMI_hook, NULL);
    D(bug("[Kernel] core_ACPIInitialise: core_ACPITableMADTParse(ACPI_MADT_LAPIC_NMI) returned %p\n", result));

    result = ACPI_ScanEntries(&madt->header, ACPI_MADT_IOAPIC, &ACPI_TableParse_IOAPIC_hook, NULL);
    D(bug("[Kernel] core_ACPIInitialise: core_ACPITableMADTParse(ACPI_MADT_IOAPIC) returned %p\n", result));
    if (result)
    {
    	KernelBase->kb_PlatformData->kb_APIC_IRQ_Model = ACPI_IRQ_PIC_IOAPIC;
    	KernelBase->kb_PlatformData->kb_ACPI_IOAPIC    = 1;
    }

    /* Build a default routing table for legacy (ISA) interrupts. */
    /* TODO: implement legacy irq config.. */
    D(bug("[Kernel] core_ACPIInitialise: Configuring Legacy IRQs .. Skipped (UNIMPLEMENTED) ..\n"));

    result = ACPI_ScanEntries(&madt->header, ACPI_MADT_NMI_SRC, &ACPI_TableParse_NMI_Src_hook, NULL);
    D(bug("[Kernel] core_ACPIInitialise: core_ACPITableMADTParse(ACPI_MADT_NMI_SRC) returned %p\n", result));

    /* TODO: implement check for clustered apic's..
    if (KernelBase->kb_PlatformData->kb_APIC_Count)
    {
        D(bug("[Kernel] core_ACPIInitialise: SMP APICs Configured from ACPI\n"));

        core_APICClusteredCheck();
    } */

    hpet = ACPI_FindSDT(ACPI_MAKE_ID('H', 'P', 'E', 'T'));
    if (hpet)
    {
    	D(bug("[Kernel] core_ACPIInitialise: HPET table 0x%p\n", hpet));

	ACPI_Table_HPET_Parse(hpet);
    }

    return 1;
}
