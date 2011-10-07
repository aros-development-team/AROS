/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
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

#define D(x) x

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

    D(bug("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Count: Local APIC %d:%d  [Flags=%08x]\n", processor->acpi_id, processor->id, processor->flags));
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
    D(bug("[APIC] (HOOK) ACPI_hook_Table_LAPIC_Addr_Ovr_Parse: Local APIC address Override to 0x%p\n", data->lapicBase));

    return TRUE;

    AROS_USERFUNC_EXIT
}

/*
 * Process the 'Local APIC' MADT Table.
 * This function collects APIC IDs into already allocated IDMap.
 */
AROS_UFH3(static IPTR, ACPI_hook_Table_LAPIC_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(struct ACPI_TABLE_TYPE_LAPIC *, processor, A2),
	  AROS_UFHA(struct APICData *, data, A1))
{
    AROS_USERFUNC_INIT

    if (processor->flags.enabled)
    {
    	UBYTE apic_newno;

	if ((data->IDMap[0] & 0xFF) == processor->id)
	{
	    /* This is the BSP, slot 0 is always reserved for it. */
	    apic_newno = 0;
	}
	else
	{
	    /* Add one more AP */
	    apic_newno = data->count++;
	}   

	data->IDMap[apic_newno] = (processor->acpi_id << 8) | processor->id;
	D(bug("[APIC] (HOOK) ACPI_hook_Table_LAPIC_Parse: Registered APIC number %d [ID=0x%04X]\n", apic_newno, data->IDMap[apic_newno]));

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
    UBYTE cpu_acpi_id = data->IDMap[cpu_num] >> 8;

    if ((lapic_nmi->acpi_id == cpu_acpi_id) || (lapic_nmi->acpi_id == ACPI_ID_BROADCAST))
    {
        UWORD reg;
        ULONG val = LVT_MT_NMI;	/* This is the default (edge-triggered, active low) */

    	D(bug("[APIC.%u] NMI LINT%u\n", cpu_num, lapic_nmi->lint));

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
	    D(bug("[APIC.%u] NMI active low\n", cpu_num));
            val |= LVT_ACTIVE_LOW;
        } 

	if (lapic_nmi->flags.trigger == INTF_TRIGGER_LEVEL)
	{
	    D(bug("[APIC.%u] NMI level-triggered\n", cpu_num));
	    val |= LVT_TGM_LEVEL;
	}

	APIC_REG(data->lapicBase, reg) = val;
	return TRUE;
    }

    return FALSE;

    AROS_USERFUNC_EXIT
}

/* Process the 'IO-APIC' MADT Table */
AROS_UFH2(IPTR, ACPI_hook_Table_IOAPIC_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(struct ACPI_TABLE_TYPE_IOAPIC *, ioapic, A2))
{
    AROS_USERFUNC_INIT

    D(bug("[APIC] (HOOK) ACPI_hook_Table_IOAPIC_Parse: IOAPIC %d @ %p [irq base = %d]\n", ioapic->id, ioapic->address, ioapic->global_irq_base));

    return TRUE;

    AROS_USERFUNC_EXIT
}

/* Process the 'Interrupt Source Overide' MADT Table */
AROS_UFH2(IPTR, ACPI_hook_Table_Int_Src_Ovr_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(struct ACPI_TABLE_TYPE_INT_SRCOVR *, intsrc, A2))
{
    AROS_USERFUNC_INIT

    D(bug("[APIC] (HOOK) ACPI_hook_Table_Int_Src_Ovr_Parse: BUS IRQ %d, Global IRQ %d, polarity ##, trigger ##\n", intsrc->bus_irq, intsrc->global_irq));
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

    D(bug("[APIC] (HOOK) ACPI_hook_Table_NMI_Src_Parse()\n"));

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
	struct Hook hook;

    	/*
    	 * We have MADT from ACPI.
    	 * The first thing to do now is to count APICs and allocate struct APICData.
    	 */
        result = ACPI_ScanEntries(&madt->header, ACPI_MADT_LAPIC, &ACPI_TableParse_LAPIC_count_hook, NULL);
        D(bug("[APIC-ACPI] Found %u enabled APICs\n", result));

	data = AllocMem(sizeof(struct APICData) + result * sizeof(UWORD), MEMF_ANY);
	if (!data)
	    return NULL;

        data->lapicBase = madt->lapic_address;
        data->count	= 1;	/* Only one CPU is running right now */
        data->flags     = madt->flags.pcat_compat ? APF_8259 : 0;

        D(bug("[APIC-ACPI] Local APIC address 0x%08x; Flags 0x%04X\n", data->lapicBase, data->flags));

        /*
     	 * The local APIC base address is obtained from the MADT (32-bit value) and
     	 * (optionally) overriden by a LAPIC_ADDR_OVR entry (64-bit value).
     	 */
	ACPI_ScanEntries(&madt->header, ACPI_MADT_LAPIC_ADDR_OVR, &ACPI_TableParse_LAPIC_Addr_Ovr_hook, data);

	/* Remember ID of the bootstrap APIC, this is CPU #0 */
	data->IDMap[0] = core_APIC_GetID(data->lapicBase);
	D(bug("[APIC-ACPI] BSP ID: 0x%02X\n", data->IDMap[0]));

	/* Now fill in IDs (both HW and ACPI) of the rest APICs */
	ACPI_ScanEntries(&madt->header, ACPI_MADT_LAPIC, &ACPI_TableParse_LAPIC_hook, data);
	D(bug("[APIC-ACPI] System Total APICs: %d\n", data->count));

	/* Initialize APIC to the default configuration */
	core_APIC_Init(data->lapicBase);

	/* Set up NMI for ourselves (CPU #0) */
	hook.h_Entry = (APTR)ACPI_hook_Table_LAPIC_NMI_Parse;
	hook.h_Data  = (APTR)0;
	ACPI_ScanEntries(&madt->header, ACPI_MADT_LAPIC_NMI, &hook, data);

	/* TODO: The following is actually not implemented yet. IOAPICs should be configured here. */

	result = ACPI_ScanEntries(&madt->header, ACPI_MADT_IOAPIC, &ACPI_TableParse_IOAPIC_hook, NULL);
	D(bug("[APIC-ACPI] ACPI_ScanEntries(ACPI_MADT_IOAPIC) returned %p\n", result));

	ACPI_ScanEntries(&madt->header, ACPI_MADT_INT_SRC_OVR, &ACPI_TableParse_Int_Src_Ovr_hook, NULL);

	result = ACPI_ScanEntries(&madt->header, ACPI_MADT_NMI_SRC, &ACPI_TableParse_NMI_Src_hook, NULL);
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