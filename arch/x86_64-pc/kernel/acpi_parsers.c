/*
    Copyright � 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <inttypes.h>

#include "exec_intern.h"
#include "etask.h"

#include <stdio.h>
#include <asm/cpu.h>
#include <asm/io.h>
#include <asm/segments.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>

#include "kernel_intern.h"

#define CONFIG_LAPICS

/************************************************************************************************
                                    ACPI TABLE PARSING HOOKS
 ************************************************************************************************/

extern IPTR              _Kern_APICTrampolineBase;

/* Process the 'Multiple APIC Description Table' Table */
AROS_UFH1(int, ACPI_hook_Table_MADT_Parse,
    AROS_UFHA(struct acpi_table_hook *,	table_hook,	A0))
{
    AROS_USERFUNC_INIT

    rkprintf("[Kernel] (HOOK) ACPI_hook_Table_MADT_Parse()\n");

	struct acpi_table_madt	*madt = NULL;
    
    struct KernelBase *KernelBase = TLS_GET(KernelBase);

	if (!table_hook->phys_addr || !table_hook->size)
    {
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_MADT_Parse: Illegal MADT Table Addr/Size\n");
        return 0;
    }

	madt = (struct acpi_table_madt *) table_hook->phys_addr;

	if (madt->lapic_address)
    {
        KernelBase->kb_APICBase = madt->lapic_address;
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_MADT_Parse: Local APIC address 0x%08x\n", KernelBase->kb_APICBase);
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

    struct KernelBase *KernelBase = TLS_GET(KernelBase);
    struct acpi_table_lapic	*processor = NULL;

	if (!table_hook->header)
    {
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Parse: Illegal LAPIC Table Addr\n");
        return 0;
    }

    processor = (struct acpi_table_lapic *) table_hook->header;
    
    rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Parse: Local APIC %d:%d  [Flags=%08x]\n", processor->acpi_id, processor->id, processor->flags);

#if defined(CONFIG_LAPICS)
    if ((KernelBase->kb_APICIDMap[0] != processor->id) && processor->flags.enabled)
    {
        if (_Kern_APICTrampolineBase != NULL)
        {
            rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Parse: Registering NEW APIC\n");

            UBYTE apic_count;
            UBYTE apic_newid = KernelBase->kb_APICCount++;
            UBYTE *apic_oldmap = KernelBase->kb_APICIDMap;

            KernelBase->kb_APICIDMap = AllocVec(KernelBase->kb_APICCount, MEMF_CLEAR);
            for (apic_count = 0; apic_count < apic_newid; apic_count ++)
            {
                KernelBase->kb_APICIDMap[apic_count] == apic_oldmap[apic_count];
            }
            FreeVec(apic_oldmap);

            KernelBase->kb_APICIDMap[apic_newid] = processor->id;

#if (0)
            /* Allow access to page 0 again */
            core_ProtKernelArea(0, 1, 1, 1, 1);

            rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Parse: Setting warm reset code ..\n");
            outb(0xf, 0x70);
            outb(0xa, 0x71);

            /* Flush TLB */
            do
            {
                unsigned long scratchreg;

                asm volatile(
                        "movq %%cr3, %0\n\t"
                        "movq %0, %%cr3":"=r"(scratchreg)::"memory");
            } while (0);

            /* 40:67 set to _Kern_APICTrampolineBase so that APIC recieves it in CS:IP */
            rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Parse: Setting vector for trampoline @ %p ..\n", _Kern_APICTrampolineBase);
            *((volatile unsigned short *)0x469) = _Kern_APICTrampolineBase >> 4;
            *((volatile unsigned short *)0x467) = _Kern_APICTrampolineBase & 0xf;
#endif
            /* Start IPI sequence */
            unsigned long wakeresult = core_APICIPIWake(processor->id, _Kern_APICTrampolineBase);

#if (0)
            /* Lock page 0 access again! */
            core_ProtKernelArea(0, 1, 0, 0, 0);
#endif

            rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Parse: core_APICIPIWake returns %d\n",wakeresult);
        }
        else
        {
            rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Parse: Warning - No APIC Trampoline.. Cannot start apic id %d\n", processor->id);
            return 0;
        }
    }
#endif
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
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Addr_Ovr_Parse: Illegal LAPIC_Addr_Ovr Table Addr\n");
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
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_NMI_Parse: Illegal LAPIC_NMI Table  Addr\n");
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
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_IOAPIC_Parse: Illegal IOAPIC Table Addr\n");
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
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_Int_Src_Ovr_Parse: Illegal Int_Src_Ovr Table Addr\n");
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
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_NMI_Src_Parse: Illegal NMI_Src Table Addr\n");
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
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_HPET_Parse: Illegal HPET Table Addr/Size\n");
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
