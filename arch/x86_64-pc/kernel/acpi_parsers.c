/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <inttypes.h>

#include <asm/cpu.h>
#include <asm/io.h>
#include <aros/asmcall.h>
#include <proto/exec.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "acpi.h"
#include "apic.h"
#include "smp.h"

#define D(x)

/*
 * The code here is experimental and very incomplete.
 * Only a small portion of information is actually used.
 */

/************************************************************************************************
                                    ACPI TABLE PARSING FUNCTIONS
 ************************************************************************************************/

/* Process the 'Multiple APIC Description Table' Table */
void ACPI_Table_MADT_Parse(struct ACPI_TABLE_TYPE_MADT *madt)
{
    struct PlatformData *pdata = KernelBase->kb_PlatformData;

    D(bug("[Kernel] ACPI_Table_MADT_Parse(0x%p)\n", madt));

    if (madt->lapic_address)
    {
        pdata->kb_APIC_BaseMap[0] = madt->lapic_address;
        bug("[Kernel] (HOOK) ACPI_hook_Table_MADT_Parse: Local APIC address 0x%08x\n", pdata->kb_APIC_BaseMap[0]);
    }

    D(if (madt->flags.pcat_compat) bug("[Kernel] (HOOK) ACPI_hook_Table_MADT_Parse: Local APIC has 8259 PIC\n");)
}

/*
 * Pre-process the 'Local APIC' MADT Table.
 * This function simply counts enabled APICs in order to determine number of entries in APIC maps.
 */
AROS_UFH2(IPTR, ACPI_hook_Table_LAPIC_Count,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(struct ACPI_TABLE_TYPE_LAPIC *, processor, A2))
{
    AROS_USERFUNC_INIT

    struct PlatformData *pdata = KernelBase->kb_PlatformData;

    D(bug("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Count: Local APIC %d:%d  [Flags=%08x]\n", processor->acpi_id, processor->id, processor->flags));

    if (processor->flags.enabled)
    	table_hook->h_Data++;

    return 1;

    AROS_USERFUNC_EXIT
}

/*
 * Process the 'Local APIC' MADT Table.
 * This function collects APIC IDs into already allocated IDMap.
 */
AROS_UFH2(IPTR, ACPI_hook_Table_LAPIC_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(struct ACPI_TABLE_TYPE_LAPIC *, processor, A2))
{
    AROS_USERFUNC_INIT

    struct PlatformData *pdata = KernelBase->kb_PlatformData;

    if (((pdata->kb_APIC_IDMap[0] & 0xFF) != processor->id) && processor->flags.enabled)
    {
	UBYTE apic_newno = pdata->kb_APIC_Count++;

	pdata->kb_APIC_IDMap[apic_newno] = (processor->acpi_id << 8) | processor->id;
	D(bug("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Parse: Registered APIC number %d [ID=0x%04X]\n", apic_newno, pdata->kb_APIC_IDMap[apic_newno]));
    }

    return 1;

    AROS_USERFUNC_EXIT
}

/* Process the 'Local APIC Address Overide' MADT Table */
AROS_UFH2(IPTR, ACPI_hook_Table_LAPIC_Addr_Ovr_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(struct ACPI_TABLE_TYPE_LAPIC_ADDROVR *, lapic_addr_ovr, A2))
{
    AROS_USERFUNC_INIT

    struct PlatformData *pdata = KernelBase->kb_PlatformData;

    if (lapic_addr_ovr->address)
    {
        pdata->kb_APIC_BaseMap[0] = lapic_addr_ovr->address;
        D(bug("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Addr_Ovr_Parse: Local APIC address Override to 0x%p\n", pdata->kb_APIC_BaseMap[0]));
    }

    return 1;
    
    AROS_USERFUNC_EXIT
}

/* Process the 'Local APIC Non-Maskable Interrupt' MADT Table */
AROS_UFH2(IPTR, ACPI_hook_Table_LAPIC_NMI_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(struct ACPI_TABLE_TYPE_LAPIC_NMI *, lapic_nmi, A2))
{
    AROS_USERFUNC_INIT

    if (lapic_nmi->lint != 1)
    {
        D(bug("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_NMI_Parse: APIC ID %d: WARNING - NMI not connected to LINT1!\n", lapic_nmi->acpi_id));
    }

    return 1;

    AROS_USERFUNC_EXIT
}

/* Process the 'IO-APIC' MADT Table */
AROS_UFH2(IPTR, ACPI_hook_Table_IOAPIC_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(struct ACPI_TABLE_TYPE_IOAPIC *, ioapic, A2))
{
    AROS_USERFUNC_INIT

    bug("[Kernel] (HOOK) ACPI_hook_Table_IOAPIC_Parse: IOAPIC %d @ %p [irq base = %d]\n", ioapic->id, ioapic->address, ioapic->global_irq_base);

    return 1;
    
    AROS_USERFUNC_EXIT
}

/* Process the 'Interrupt Source Overide' MADT Table */
AROS_UFH2(IPTR, ACPI_hook_Table_Int_Src_Ovr_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(struct ACPI_TABLE_TYPE_INT_SRCOVR *, intsrc, A2))
{
    AROS_USERFUNC_INIT

    D(bug("[Kernel] (HOOK) ACPI_hook_Table_Int_Src_Ovr_Parse: BUS IRQ %d, Global IRQ %d, polarity ##, trigger ##\n", intsrc->bus_irq, intsrc->global_irq));
                    //intsrc->flags.polarity,
                    //intsrc->flags.trigger,

    return 1;

    AROS_USERFUNC_EXIT
}

/* Process the 'Non-Maskable Interrupt Source' MADT Table */
AROS_UFH2(IPTR, ACPI_hook_Table_NMI_Src_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(struct ACPI_TABLE_TYPE_LAPIC_NMI_SRC *, nmi_src, A2))
{
    AROS_USERFUNC_INIT

    D(bug("[Kernel] (HOOK) ACPI_hook_Table_NMI_Src_Parse()\n"));

    /* FIXME: Uh... shouldn't we do something with this? */

    return 1;
    
    AROS_USERFUNC_EXIT
}

/* Process the 'High Precision Event Timer' Table */
int ACPI_Table_HPET_Parse(struct ACPI_TABLE_TYPE_HPET *hpet_tbl)
{
    if (hpet_tbl->addr.space_id != ACPI_SPACE_MEM) 
    {
        D(bug("[Kernel] (HOOK) ACPI_hook_Table_HPET_Parse: HPET timers must be located in memory.\n"));
        return -1;
    }

    D(bug("[Kernel] (HOOK) ACPI_hook_Table_HPET_Parse: INFORMATION - HPET id: %d @ 0x%08X%08X\n", hpet_tbl->id, hpet_tbl->addr.addrh, hpet_tbl->addr.addrl));
    return 1;
}
