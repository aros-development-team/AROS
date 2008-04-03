/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id: acpi_parsers.c,v 1.7 2004/01/07 07:13:03 nicja Exp $
*/
#include <inttypes.h>

#include "exec_intern.h"
#include "etask.h"

#include <exec/lists.h>
#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <asm/segments.h>

#include "kernel_intern.h"

/************************************************************************************************
                                    ACPI TABLE PARSING HOOKS
 ************************************************************************************************/

/* Process the 'Multiple APIC Description Table' Table */
AROS_UFH1(int, ACPI_hook_Table_MADT_Parse,
    AROS_UFHA(struct acpi_table_hook *,	table_hook,	A0))
{
    AROS_USERFUNC_INIT

    rkprintf("[Kernel] (HOOK) ACPI_hook_Table_MADT_Parse()\n");

	struct acpi_table_madt	*madt = NULL;

	if (!table_hook->phys_addr || !table_hook->size)
    {
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_MADT_Parse: Illegal MADT Addr/Size\n");
        return 0;
    }

	madt = (struct acpi_table_madt *) table_hook->phys_addr;

	if (madt->lapic_address)
    {
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_MADT_Parse: Local APIC address 0x%08x\n", madt->lapic_address);
    }
	return 1;
 
    AROS_USERFUNC_EXIT
}

/* Process the 'Local APIC' MADT Table */
AROS_UFH1(int, ACPI_hook_Table_LAPIC_Parse,
    AROS_UFHA(struct acpi_madt_entry_hook *,	table_hook,	A0))
{
    AROS_USERFUNC_INIT

    rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Parse()\n");

    struct acpi_table_lapic	*processor = NULL;

	if (!table_hook->header)
    {
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Parse: Illegal LAPIC Addr\n");
        return 0;
    }

    processor = (struct acpi_table_lapic *) table_hook->header;
    
    rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Parse: Local APIC %d:%d  [Flags=%08x]\n", processor->acpi_id, processor->id, processor->flags);
    
	return 1;
    
    AROS_USERFUNC_EXIT
}

/* Process the 'Local APIC Address Overide' MADT Table */
AROS_UFH1(int, ACPI_hook_Table_LAPIC_Addr_Ovr_Parse,
    AROS_UFHA(struct acpi_madt_entry_hook *,	table_hook,	A0))
{
    AROS_USERFUNC_INIT

    rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Addr_Ovr_Parse()\n");

	struct acpi_table_lapic_addr_ovr *lapic_addr_ovr = NULL;

	if (!table_hook->header)
    {
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Addr_Ovr_Parse: Illegal LAPIC_Addr_Ovr Addr\n");
        return 0;
    }

    lapic_addr_ovr = (struct acpi_table_lapic_addr_ovr *) table_hook->header;

    rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Addr_Ovr_Parse: Local APIC address 0x%08x\n", lapic_addr_ovr->address);
    
    return 1;
    
    AROS_USERFUNC_EXIT
}

/* Process the 'Local APIC Non-Maskable Interrupt' MADT Table */
AROS_UFH1(int, ACPI_hook_Table_LAPIC_NMI_Parse,
    AROS_UFHA(struct acpi_madt_entry_hook *,	table_hook,	A0))
{
    AROS_USERFUNC_INIT

    rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_NMI_Parse()\n");
    
    struct acpi_table_lapic_nmi *lapic_nmi = NULL;

	if (!table_hook->header)
    {
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_NMI_Parse: Illegal LAPIC_NMI Addr\n");
        return 0;
    }

    lapic_nmi = (struct acpi_table_lapic_nmi *) table_hook->header;

	if (lapic_nmi->lint != 1)
    {
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_NMI_Parse: WARNING - NMI not connected to LINT1!\n");
    }

    return 1;

    AROS_USERFUNC_EXIT
}

/* Process the 'IO-APIC' MADT Table */
AROS_UFH1(int, ACPI_hook_Table_IOAPIC_Parse,
    AROS_UFHA(struct acpi_madt_entry_hook *,	table_hook,	A0))
{
    AROS_USERFUNC_INIT

    rkprintf("[Kernel] (HOOK) ACPI_hook_Table_IOAPIC_Parse()\n");

    struct acpi_table_ioapic *ioapic = NULL;

	if (!table_hook->header)
    {
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_IOAPIC_Parse: Illegal IOAPIC Addr\n");
        return 0;
    }

    ioapic = (struct acpi_table_ioapic *) table_hook->header;

    rkprintf("[Kernel] (HOOK) ACPI_hook_Table_IOAPIC_Parse: IOAPIC %d @ %p [irq base = %d]\n", ioapic->id, ioapic->address, ioapic->global_irq_base);

    return 1;
    
    AROS_USERFUNC_EXIT
}

/* Process the 'Interrupt Source Overide' MADT Table */
AROS_UFH1(int, ACPI_hook_Table_Int_Src_Ovr_Parse,
    AROS_UFHA(struct acpi_madt_entry_hook *,	table_hook,	A0))
{
    AROS_USERFUNC_INIT

    rkprintf("[Kernel] (HOOK) ACPI_hook_Table_Int_Src_Ovr_Parse()\n");

    struct acpi_table_int_src_ovr *intsrc = NULL;

	if (!table_hook->header)
    {
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_Int_Src_Ovr_Parse: Illegal Int_Src_Ovr Addr\n");
        return 0;
    }

    intsrc = (struct acpi_table_int_src_ovr *) table_hook->header;

    rkprintf("[Kernel] (HOOK) ACPI_hook_Table_Int_Src_Ovr_Parse: BUS IRQ %d, Global IRQ %d, polarity ##, trigger ##\n", intsrc->bus_irq, intsrc->global_irq);
                    //intsrc->flags.polarity,
                    //intsrc->flags.trigger,

	return 1;

    AROS_USERFUNC_EXIT
}

/* Process the 'Non-Maskable Interrupt Source' MADT Table */
AROS_UFH1(int, ACPI_hook_Table_NMI_Src_Parse,
    AROS_UFHA(struct acpi_madt_entry_hook *,	table_hook,	A0))
{
    AROS_USERFUNC_INIT

    rkprintf("[Kernel] (HOOK) ACPI_hook_Table_NMI_Src_Parse()\n");

    struct acpi_table_nmi_src *nmi_src = NULL;

	if (!table_hook->header)
    {
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_NMI_Src_Parse: Illegal NMI_Src Add\n");
        return 0;
    }

    nmi_src = (struct acpi_table_nmi_src *) table_hook->header;

	return 1;
    
    AROS_USERFUNC_EXIT
}

/* Process the 'High Precision Event Timer' Table */
AROS_UFH1(int, ACPI_hook_Table_HPET_Parse,
    AROS_UFHA(struct acpi_table_hook *,	table_hook,	A0))
{
    AROS_USERFUNC_INIT

    rkprintf("[Kernel] (HOOK) ACPI_hook_Table_HPET_Parse()\n");

    struct acpi_table_hpet *hpet_tbl;

	if (!table_hook->phys_addr || !table_hook->size)
    {
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_HPET_Parse: Illegal HPET Addr/Size\n");
        return 0;
    }

    hpet_tbl = (struct acpi_table_hpet *) table_hook->phys_addr;

	if (hpet_tbl->addr.space_id != ACPI_SPACE_MEM) 
    {
		rkprintf("[Kernel] (HOOK) ACPI_hook_Table_HPET_Parse: HPET timers must be located in memory.\n");
		return -1;
	}

	rkprintf("[Kernel] (HOOK) ACPI_hook_Table_HPET_Parse: INFORMATION - HPET id: %d @ %p\n", hpet_tbl->id, hpet_tbl->addr.addrl);
    
    return 1;

    AROS_USERFUNC_EXIT
}

/************************************************************************************************/

/* Hooks defining our callbacks */
const struct acpi_table_hook ACPI_TableParse_MADT_hook = {
    .h_Entry = (APTR)ACPI_hook_Table_MADT_Parse
};

const struct acpi_table_hook ACPI_TableParse_LAPIC_Addr_Ovr_hook = {
    .h_Entry = (APTR)ACPI_hook_Table_LAPIC_Addr_Ovr_Parse
};

const struct acpi_table_hook ACPI_TableParse_LAPIC_hook = {
    .h_Entry = (APTR)ACPI_hook_Table_LAPIC_Parse
};

const struct acpi_table_hook ACPI_TableParse_LAPIC_NMI_hook = {
    .h_Entry = (APTR)ACPI_hook_Table_LAPIC_NMI_Parse
};

const struct acpi_table_hook ACPI_TableParse_IOAPIC_hook = {
    .h_Entry = (APTR)ACPI_hook_Table_IOAPIC_Parse
};

const struct acpi_table_hook ACPI_TableParse_Int_Src_Ovr_hook = {
    .h_Entry = (APTR)ACPI_hook_Table_Int_Src_Ovr_Parse
};

const struct acpi_table_hook ACPI_TableParse_NMI_Src_hook = {
    .h_Entry = (APTR)ACPI_hook_Table_NMI_Src_Parse
};

const struct acpi_table_hook ACPI_TableParse_HPET_hook = {
    .h_Entry = (APTR)ACPI_hook_Table_HPET_Parse
};
