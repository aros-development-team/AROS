/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/asmcall.h>
#include <resources/acpi.h>
#include <proto/acpi.h>
#include <proto/exec.h>

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

/*
 * Pre-process the 'Local APIC' MADT Table.
 * This function simply counts enabled APICs in order to determine number of entries in APIC maps.
 */
AROS_UFH2(static IPTR, ACPI_hook_Table_LAPIC_Count,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(struct ACPI_TABLE_TYPE_LAPIC *, processor, A2))
{
    AROS_USERFUNC_INIT

    D(bug("[APIC-ACPI] (HOOK) ACPI_hook_Table_LAPIC_Count: Local APIC %d:%d  [Flags=%08x]\n", processor->acpi_id, processor->id, processor->flags));
    return processor->flags.enabled;

    AROS_USERFUNC_EXIT
}

/* Process the 'Local APIC Address Overide' MADT Table */
AROS_UFH3(static IPTR, ACPI_hook_Table_LAPIC_Addr_Ovr_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(struct ACPI_TABLE_TYPE_LAPIC_ADDROVR *, lapic_addr_ovr, A2),
	  AROS_UFHA(struct APICData *, data, A2))
{
    AROS_USERFUNC_INIT

    data->lapicBase = lapic_addr_ovr->address;
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
	  AROS_UFHA(struct ACPI_TABLE_TYPE_LAPIC *, processor, A2),
	  AROS_UFHA(struct APICData *, data, A1))
{
    AROS_USERFUNC_INIT

    if (processor->flags.enabled)
    {
	if (data->cores[0].lapicID == processor->id)
	{
	    /* This is the BSP, slot 0 is always reserved for it. */
	    bug("[APIC-ACPI] Registering APIC [ID=0x%02X] for BSP\n", processor->id);

	    data->cores[0].sysID = processor->acpi_id;
	}
	else
	{
	    /* Add one more AP */
	    bug("[APIC-ACPI] Registering APIC [ID=0x%02X:0x%02X]\n", processor->id, processor->acpi_id);

	    data->cores[data->count].lapicID = processor->id;
	    data->cores[data->count].sysID   = processor->acpi_id;

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
	  AROS_UFHA(struct ACPI_TABLE_TYPE_LAPIC_NMI *, lapic_nmi, A2),
	  AROS_UFHA(struct APICData *, data, A1))
{
    AROS_USERFUNC_INIT

    IPTR cpu_num = (IPTR)table_hook->h_Data;

    if ((lapic_nmi->acpi_id == data->cores[cpu_num].sysID) || (lapic_nmi->acpi_id == ACPI_ID_BROADCAST))
    {
        UWORD reg;
        ULONG val = LVT_MT_NMI;	/* This is the default (edge-triggered, active low) */

    	D(bug("[APIC-ACPI.%u] NMI LINT%u\n", cpu_num, lapic_nmi->lint));

    	switch (lapic_nmi->lint)
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

        if (lapic_nmi->flags.polarity == INTF_POLARITY_LOW)
        {
	    D(bug("[APIC-ACPI.%u] NMI active low\n", cpu_num));
            val |= LVT_ACTIVE_LOW;
        } 

	if (lapic_nmi->flags.trigger == INTF_TRIGGER_LEVEL)
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
	  AROS_UFHA(struct ACPI_TABLE_TYPE_IOAPIC *, ioapic, A2),
	  AROS_UFHA(struct APICData *, data, A1))
{
    AROS_USERFUNC_INIT

    D(bug("[APIC-ACPI] (HOOK) ACPI_hook_Table_IOAPIC_Parse: IOAPIC %d @ %p [irq base = %d]\n", ioapic->id, ioapic->address, ioapic->global_irq_base));

    data->ioapicBase = ioapic->address;
    return TRUE;

    AROS_USERFUNC_EXIT
}

/* Process the 'Interrupt Source Overide' MADT Table */
AROS_UFH2(IPTR, ACPI_hook_Table_Int_Src_Ovr_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(struct ACPI_TABLE_TYPE_INT_SRCOVR *, intsrc, A2))
{
    AROS_USERFUNC_INIT

    D(bug("[APIC-ACPI] (HOOK) ACPI_hook_Table_Int_Src_Ovr_Parse: BUS IRQ %d, Global IRQ %d, polarity ##, trigger ##\n", intsrc->bus_irq, intsrc->global_irq));
                    //intsrc->flags.polarity,
                    //intsrc->flags.trigger,

    return TRUE;

    AROS_USERFUNC_EXIT
}

/* Process the 'Non-Maskable Interrupt Source' MADT Table */
AROS_UFH2(IPTR, ACPI_hook_Table_NMI_Src_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(struct ACPI_TABLE_TYPE_LAPIC_NMI_SRC *, nmi_src, A2))
{
    AROS_USERFUNC_INIT

    D(bug("[APIC-ACPI] (HOOK) ACPI_hook_Table_NMI_Src_Parse()\n"));

    /* FIXME: Uh... shouldn't we do something with this? */

    return TRUE;

    AROS_USERFUNC_EXIT
}

static const struct Hook ACPI_TableParse_LAPIC_count_hook =
{
    .h_Entry = (APTR)ACPI_hook_Table_LAPIC_Count,
};

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
    	struct ACPIBase *ACPIBase = data->acpiBase;
    	struct Hook hook;

	/* Set up NMI for ourselves */
	hook.h_Entry = (APTR)ACPI_hook_Table_LAPIC_NMI_Parse;
	hook.h_Data  = (APTR)cpuNum;
	ACPI_ScanEntries(data->acpi_madt, ACPI_MADT_LAPIC_NMI, &hook, data);
    }
}

/* Initialize APIC from ACPI data */
struct APICData *acpi_APIC_Init(struct ACPIBase *ACPIBase)
{
    ULONG result;
    struct ACPI_TABLE_TYPE_MADT *madt;
    struct APICData *data;

    /* 
     * MADT : If it exists, parse the Multiple APIC Description Table "MADT", 
     * This table provides platform SMP configuration information [the successor to MPS tables]
     */
    madt = ACPI_FindSDT(ACPI_MAKE_ID('A', 'P', 'I', 'C'));
    D(bug("[APIC-ACPI] MADT at 0x%p\n", madt));

    if (madt)
    {
    	/*
    	 * We have MADT from ACPI.
    	 * The first thing to do now is to count APICs and allocate struct APICData.
    	 */
        result = ACPI_ScanEntries(&madt->header, ACPI_MADT_LAPIC, &ACPI_TableParse_LAPIC_count_hook, NULL);
        D(bug("[APIC-ACPI] Found %u enabled APICs\n", result));

	data = AllocMem(sizeof(struct APICData) + result * sizeof(struct CPUData), MEMF_CLEAR);
	if (!data)
	    return NULL;

        data->lapicBase = madt->lapic_address;
        data->acpiBase  = ACPIBase;	/* Cache ACPI data for secondary cores */
        data->acpi_madt = madt;
        data->count	= 1;		/* Only one CPU is running right now */
        data->flags     = madt->flags.pcat_compat ? APF_8259 : 0;

        bug("[APIC-ACPI] Local APIC address 0x%08x; Flags 0x%04X\n", data->lapicBase, data->flags);

        /*
     	 * The local APIC base address is obtained from the MADT (32-bit value) and
     	 * (optionally) overriden by a LAPIC_ADDR_OVR entry (64-bit value).
     	 */
	ACPI_ScanEntries(&madt->header, ACPI_MADT_LAPIC_ADDR_OVR, &ACPI_TableParse_LAPIC_Addr_Ovr_hook, data);

	/* Remember ID of the bootstrap APIC, this is CPU #0 */
	data->cores[0].lapicID = core_APIC_GetID(data->lapicBase);
	D(bug("[APIC-ACPI] BSP ID: 0x%02X\n", data->cores[0].lapicID));

	/* Now fill in IDs (both HW and ACPI) of the rest APICs */
	ACPI_ScanEntries(&madt->header, ACPI_MADT_LAPIC, &ACPI_TableParse_LAPIC_hook, data);
	bug("[APIC-ACPI] System Total APICs: %d\n", data->count);

	/* Initialize LAPIC for ourselves (CPU #0) */
	acpi_APIC_InitCPU(data, 0);

	/* TODO: The following is actually not implemented yet. IOAPIC should be configured here. */

	result = ACPI_ScanEntries(&madt->header, ACPI_MADT_IOAPIC, &ACPI_TableParse_IOAPIC_hook, data);
	D(bug("[APIC-ACPI] ACPI_ScanEntries(ACPI_MADT_IOAPIC) returned %p\n", result));

	ACPI_ScanEntries(&madt->header, ACPI_MADT_INT_SRC_OVR, &ACPI_TableParse_Int_Src_Ovr_hook, data);

	result = ACPI_ScanEntries(&madt->header, ACPI_MADT_NMI_SRC, &ACPI_TableParse_NMI_Src_hook, data);
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
