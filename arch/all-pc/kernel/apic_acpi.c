/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/asmcall.h>
#include <proto/acpica.h>
#include <proto/exec.h>

#include <utility/hooks.h>

#include <inttypes.h>
#include <string.h>

#include "kernel_base.h"
#include "kernel_debug.h"

#include "apic.h"
#include "apic_ia32.h"

#define D(x)

/************************************************************************************************
                                    ACPI RELATED FUNCTIONS
 ************************************************************************************************/

/* Process the 'Local APIC Address Overide' MADT Table */
AROS_UFH3(static IPTR, ACPI_hook_Table_LAPIC_Addr_Ovr_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(ACPI_MADT_LOCAL_APIC_OVERRIDE *, lapic_addr_ovr, A2),
	  AROS_UFHA(struct APICData *, data, A2))
{
    AROS_USERFUNC_INIT

    data->lapicBase = lapic_addr_ovr->Address;
    D(bug("[APIC-ACPI] (HOOK) ACPI_hook_Table_LAPIC_Addr_Ovr_Parse: Local APIC address Override to 0x%p\n", data->lapicBase));

    return TRUE;

    AROS_USERFUNC_EXIT
}

/*
 * Process the 'Local APIC' MADT Table.
 * This function collects APIC IDs into already allocated CPUData array.
 */
AROS_UFH3(static IPTR, ACPI_hook_Table_LAPIC_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(ACPI_MADT_LOCAL_APIC *, processor, A2),
	  AROS_UFHA(struct APICData *, data, A1))
{
    AROS_USERFUNC_INIT

    if (processor->LapicFlags & ACPI_MADT_ENABLED)
    {
	if (data->cores[0].lapicID == processor->Id)
	{
	    /* This is the BSP, slot 0 is always reserved for it. */
	    bug("[APIC-ACPI] Registering APIC [ID=0x%02X] for BSP\n", processor->Id);

	    data->cores[0].sysID = processor->ProcessorId;
	}
	else
	{
	    /* Add one more AP */
	    bug("[APIC-ACPI] Registering APIC [ID=0x%02X:0x%02X]\n", processor->Id, processor->ProcessorId);

	    data->cores[data->count].lapicID = processor->Id;
	    data->cores[data->count].sysID   = processor->ProcessorId;

	    data->count++;
	}


	return TRUE;
    }

    return FALSE;

    AROS_USERFUNC_EXIT
}

/* Process the 'Local APIC Non-Maskable Interrupt' MADT Table */
AROS_UFH3(IPTR, ACPI_hook_Table_LAPIC_NMI_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(ACPI_MADT_LOCAL_APIC_NMI *, lapic_nmi, A2),
	  AROS_UFHA(struct APICData *, data, A1))
{
    AROS_USERFUNC_INIT

    IPTR cpu_num = (IPTR)table_hook->h_Data;

    if ((lapic_nmi->ProcessorId == data->cores[cpu_num].sysID) || (lapic_nmi->ProcessorId == 0xff))
    {
        UWORD reg;
        ULONG val = LVT_MT_NMI;	/* This is the default (edge-triggered, active low) */

    	D(bug("[APIC-ACPI.%u] NMI LINT%u\n", cpu_num, lapic_nmi->Lint));

    	switch (lapic_nmi->Lint)
    	{
    	case 0:
    	    reg = APIC_LINT0_VEC;
    	    break;

    	case 1:
    	    reg = APIC_LINT1_VEC;
    	    break;

	default:
	    /* Invalid LINT# value */
    	    return FALSE;
        }

        if ((lapic_nmi->IntiFlags & ACPI_MADT_POLARITY_MASK) == ACPI_MADT_POLARITY_ACTIVE_LOW)
        {
	    D(bug("[APIC-ACPI.%u] NMI active low\n", cpu_num));
            val |= LVT_ACTIVE_LOW;
        } 

	if ((lapic_nmi->IntiFlags & ACPI_MADT_TRIGGER_MASK) == ACPI_MADT_TRIGGER_LEVEL)
	{
	    D(bug("[APIC-ACPI.%u] NMI level-triggered\n", cpu_num));
	    val |= LVT_TGM_LEVEL;
	}

	APIC_REG(data->lapicBase, reg) = val;
	return TRUE;
    }

    return FALSE;

    AROS_USERFUNC_EXIT
}

/* Process the 'IO-APIC' MADT Table */
AROS_UFH3(IPTR, ACPI_hook_Table_IOAPIC_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(ACPI_MADT_IO_APIC *, ioapic, A2),
	  AROS_UFHA(struct APICData *, data, A1))
{
    AROS_USERFUNC_INIT

    D(bug("[APIC-ACPI] (HOOK) ACPI_hook_Table_IOAPIC_Parse: IOAPIC %d @ %p [irq base = %d]\n", ioapic->Id, ioapic->Address, ioapic->GlobalIrqBase));

    data->ioapicBase = ioapic->Address;
    return TRUE;

    AROS_USERFUNC_EXIT
}

/* Process the 'Interrupt Source Overide' MADT Table */
AROS_UFH2(IPTR, ACPI_hook_Table_Int_Src_Ovr_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(ACPI_MADT_INTERRUPT_OVERRIDE *, intsrc, A2))
{
    AROS_USERFUNC_INIT

    D(bug("[APIC-ACPI] (HOOK) ACPI_hook_Table_Int_Src_Ovr_Parse: Bus %d, Source IRQ %d, Global IRQ %d, Flags 0x%x\n", intsrc->Bus, intsrc->SourceIrq,
                intsrc->GlobalIrq, intsrc->IntiFlags));

    return TRUE;

    AROS_USERFUNC_EXIT
}

/* Process the 'Non-Maskable Interrupt Source' MADT Table */
AROS_UFH2(IPTR, ACPI_hook_Table_NMI_Src_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(ACPI_MADT_NMI_SOURCE *, nmi_src, A2))
{
    AROS_USERFUNC_INIT

    D(bug("[APIC-ACPI] (HOOK) ACPI_hook_Table_NMI_Src_Parse()\n"));

    /* FIXME: Uh... shouldn't we do something with this? */

    return TRUE;

    AROS_USERFUNC_EXIT
}

static const struct Hook ACPI_TableParse_LAPIC_Addr_Ovr_hook =
{
    .h_Entry = (APTR)ACPI_hook_Table_LAPIC_Addr_Ovr_Parse
};

static const struct Hook ACPI_TableParse_LAPIC_hook =
{
    .h_Entry = (APTR)ACPI_hook_Table_LAPIC_Parse
};

static const struct Hook ACPI_TableParse_IOAPIC_hook =
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

/************************************************************************************************/
/************************************************************************************************
                APIC Functions used by kernel.resource from outside this file ..
 ************************************************************************************************/
/************************************************************************************************/

static int MADT_ScanEntries(ACPI_TABLE_MADT *madt, enum AcpiMadtType type, const struct Hook *hook, APTR userdata)
{
    UINT8 *madt_entry = (UINT8 *)&madt[1];
    UINT8 *madt_end  = (UINT8 *)madt + madt->Header.Length;
    int count;

    for (count = 0; madt_entry < madt_end; madt_entry += ((ACPI_SUBTABLE_HEADER *)madt_entry)->Length) {
        ACPI_SUBTABLE_HEADER *sh = (ACPI_SUBTABLE_HEADER *)madt_entry;
        if (sh->Type == (UINT8)type) {
            BOOL res;
            if (hook == NULL)
                res = TRUE;
            else
                res = CALLHOOKPKT((struct Hook *)hook, (APTR)sh, userdata);
            if (res)
                count++;
        }
    }

    return count;
}


/*
 * Initialize APIC on a CPU core with specified number.
 * This routine is ran by all cores.
 */
void acpi_APIC_InitCPU(struct APICData *data, IPTR cpuNum)
{
    /* Initialize APIC to the default configuration */
    core_APIC_Init(data, cpuNum);

    if (data->acpi_madt)
    {
        struct Hook hook;

        /* Set up NMI for ourselves */
        hook.h_Entry = (APTR)ACPI_hook_Table_LAPIC_NMI_Parse;
        hook.h_Data  = (APTR)cpuNum;
        MADT_ScanEntries(data->acpi_madt, ACPI_MADT_TYPE_LOCAL_APIC_NMI, &hook, data);
    }
}

/* Initialize APIC from ACPI data */
struct APICData *acpi_APIC_Init(struct Library *ACPICABase)
{
    ULONG result;
    ACPI_STATUS err;
    const ACPI_TABLE_MADT *madt;
    struct APICData *data;

    /* 
     * MADT : If it exists, parse the Multiple APIC Description Table "MADT", 
     * This table provides platform SMP configuration information [the successor to MPS tables]
     */
    err = AcpiGetTable("MADT", 1, (ACPI_TABLE_HEADER **)&madt);
    if (err != AE_OK) {
        D(bug("[APCI-ACPI] No MADT table found, err = %d\n", err));
        return NULL;
    }

    if (madt)
    {
    	/*
    	 * We have MADT from ACPI.
    	 * The first thing to do now is to count APICs and allocate struct APICData.
    	 */
        result = MADT_ScanEntries(madt, ACPI_MADT_TYPE_LOCAL_APIC, NULL, NULL);
        D(bug("[APIC-ACPI] Found %u enabled APICs\n", result));

	data = AllocMem(sizeof(struct APICData) + result * sizeof(struct CPUData), MEMF_CLEAR);
	if (!data)
	    return NULL;

        data->lapicBase = madt->Address;
        data->acpicaBase  = ACPICABase;	/* Cache ACPI data for secondary cores */
        data->acpi_madt = madt;
        data->count	= 1;		/* Only one CPU is running right now */
        data->flags     = ((madt->Flags & ACPI_MADT_PCAT_COMPAT) == ACPI_MADT_MULTIPLE_APIC) ? APF_8259 : 0;

        bug("[APIC-ACPI] Local APIC address 0x%08x; Flags 0x%04X\n", data->lapicBase, data->flags);

        /*
     	 * The local APIC base address is obtained from the MADT (32-bit value) and
     	 * (optionally) overriden by a LAPIC_ADDR_OVR entry (64-bit value).
     	 */
	MADT_ScanEntries(madt, ACPI_MADT_TYPE_LOCAL_APIC_OVERRIDE, &ACPI_TableParse_LAPIC_Addr_Ovr_hook, data);

	/* Remember ID of the bootstrap APIC, this is CPU #0 */
	data->cores[0].lapicID = core_APIC_GetID(data->lapicBase);
	D(bug("[APIC-ACPI] BSP ID: 0x%02X\n", data->cores[0].lapicID));

	/* Now fill in IDs (both HW and ACPI) of the rest APICs */
	MADT_ScanEntries(madt, ACPI_MADT_TYPE_LOCAL_APIC, &ACPI_TableParse_LAPIC_hook, data);
	bug("[APIC-ACPI] System Total APICs: %d\n", data->count);

	/* Initialize LAPIC for ourselves (CPU #0) */
	acpi_APIC_InitCPU(data, 0);

	/* TODO: The following is actually not implemented yet. IOAPIC should be configured here. */

	result = MADT_ScanEntries(madt, ACPI_MADT_TYPE_IO_APIC, &ACPI_TableParse_IOAPIC_hook, data);
	D(bug("[APIC-ACPI] ACPI_ScanEntries(ACPI_MADT_IOAPIC) returned %p\n", result));

	MADT_ScanEntries(madt, ACPI_MADT_TYPE_INTERRUPT_OVERRIDE, &ACPI_TableParse_Int_Src_Ovr_hook, data);

	result = MADT_ScanEntries(madt, ACPI_MADT_TYPE_NMI_SOURCE, &ACPI_TableParse_NMI_Src_hook, data);
	D(bug("[APIC-ACPI] ACPI_ScanEntries(ACPI_MADT_NMI_SRC) returned %p\n", result));

	/* Build a default routing table for legacy (ISA) interrupts. */
	/* TODO: implement legacy irq config.. */
	D(bug("[APIC-ACPI] Configuring Legacy IRQs .. Skipped (UNIMPLEMENTED) ..\n"));

	/* TODO: implement check for clustered apic's..
            core_APICClusteredCheck();
     	*/
        return data;
    }

    return NULL;
}
