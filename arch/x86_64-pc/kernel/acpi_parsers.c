/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <inttypes.h>

#include "exec_intern.h"
#include "etask.h"

#define DEBUG 0
#include <aros/debug.h>

#include <stdio.h>
#include <asm/cpu.h>
#include <asm/io.h>
#include <asm/segments.h>
#include <inttypes.h>
#include <aros/symbolsets.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>

#include "kernel_intern.h"

#define CONFIG_LAPICS

/************************************************************************************************
                                    ACPI TABLE PARSING HOOKS
 ************************************************************************************************/

/* Process the 'Multiple APIC Description Table' Table */
AROS_UFH1(IPTR, ACPI_hook_Table_MADT_Parse,
    AROS_UFHA(struct acpi_table_hook *,	table_hook,	A0))
{
    AROS_USERFUNC_INIT

    D(rkprintf("[Kernel] (HOOK) ACPI_hook_Table_MADT_Parse()\n"));

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
        KernelBase->kb_APIC_BaseMap[0] = madt->lapic_address;
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_MADT_Parse: Local APIC address 0x%08x\n", KernelBase->kb_APIC_BaseMap[0]);
    }
    if (madt->flags.pcat_compat)
    {
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_MADT_Parse: Local APIC has 8259 PIC\n");
    }
    return 1;
 
    AROS_USERFUNC_EXIT
}

/* Process the 'Local APIC' MADT Table */
AROS_UFH1(IPTR, ACPI_hook_Table_LAPIC_Parse,
    AROS_UFHA(struct acpi_madt_entry_hook *,	table_hook,	A0))
{
    AROS_USERFUNC_INIT

    D(rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Parse()\n"));

    struct KernelBase *KernelBase = TLS_GET(KernelBase);
    struct acpi_table_lapic	*processor = NULL;

    D(rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Parse: KernelBase @ %p\n", KernelBase));

    if (!table_hook->header)
    {
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Parse: Illegal LAPIC Table Addr\n");
        return 0;
    }

    processor = (struct acpi_table_lapic *) table_hook->header;
    
    rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Parse: Local APIC %d:%d  [Flags=%08x]\n", processor->acpi_id, processor->id, processor->flags);

#if defined(CONFIG_LAPICS)
    if (((KernelBase->kb_APIC_IDMap[0] & 0xFF) != processor->id) && processor->flags.enabled)
    {
        if (KernelBase->kb_APIC_TrampolineBase != NULL)
        {
            UBYTE apic_count;
            UBYTE apic_newno = KernelBase->kb_APIC_Count++;
            UWORD *apic_oldidmap = KernelBase->kb_APIC_IDMap;
            IPTR *apic_oldbasemap = KernelBase->kb_APIC_BaseMap;

            rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Parse: Registering APIC number %d\n", apic_newno);

            KernelBase->kb_APIC_IDMap = AllocVec((KernelBase->kb_APIC_Count + 1) * sizeof(UWORD), MEMF_CLEAR);
            KernelBase->kb_APIC_BaseMap = AllocVec((KernelBase->kb_APIC_Count + 1) * sizeof(IPTR), MEMF_CLEAR);
            rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Parse: New IDMap Allocated @ %p, BaseMap @ %p\n", KernelBase->kb_APIC_IDMap, KernelBase->kb_APIC_BaseMap);

            for (apic_count = 0; apic_count < apic_newno; apic_count++)
            {
                KernelBase->kb_APIC_IDMap[apic_count] = apic_oldidmap[apic_count];
                KernelBase->kb_APIC_BaseMap[apic_count] = apic_oldbasemap[apic_count];
            }
            rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Parse: Freeing old  IDMap @ %p, BaseMap @ %p\n", apic_oldidmap, apic_oldbasemap);
            FreeVec(apic_oldidmap);
            FreeVec(apic_oldbasemap);

            /* We set the ID here - the APIC will set its own base (which we can use to tell if its up.. */
            KernelBase->kb_APIC_IDMap[apic_newno] = ((processor->acpi_id << 8) | processor->id);
#if (0)
            /* Allow access to page 0 again */
            core_ProtKernelArea(0, 1, 1, 1, 1);

            D(rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Parse: Setting warm reset code ..\n"));
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

            /* 40:67 set to KernelBase->kb_APIC_TrampolineBase so that APIC recieves it in CS:IP */
            D(rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Parse: Setting vector for trampoline @ %p ..\n", KernelBase->kb_APIC_TrampolineBase));
            *((volatile unsigned short *)0x469) = KernelBase->kb_APIC_TrampolineBase >> 4;
            *((volatile unsigned short *)0x467) = KernelBase->kb_APIC_TrampolineBase & 0xf;
#endif
            /* Start IPI sequence */
            IPTR wakeresult = AROS_UFC2(IPTR,
                                ((struct GenericAPIC *)KernelBase->kb_APIC_Drivers[KernelBase->kb_APIC_DriverID])->wake,
                                AROS_UFCA(IPTR, KernelBase->kb_APIC_TrampolineBase, A0),
                                AROS_UFCA(UBYTE, processor->id, D0));

#if (0)
            /* Lock page 0 access again! */
            core_ProtKernelArea(0, 1, 0, 0, 0);
#endif

            rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Parse: core_APICIPIWake returns %d\n", wakeresult);
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
AROS_UFH1(IPTR, ACPI_hook_Table_LAPIC_Addr_Ovr_Parse,
    AROS_UFHA(struct acpi_madt_entry_hook *,	table_hook,	A0))
{
    AROS_USERFUNC_INIT

    struct KernelBase *KernelBase = TLS_GET(KernelBase);

    D(rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Addr_Ovr_Parse()\n"));

    struct acpi_table_lapic_addr_ovr *lapic_addr_ovr = NULL;

    if (!table_hook->header)
    {
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Addr_Ovr_Parse: Illegal LAPIC_Addr_Ovr Table Addr\n");
        return 0;
    }

    lapic_addr_ovr = (struct acpi_table_lapic_addr_ovr *) table_hook->header;

    if (lapic_addr_ovr->address)
    {
        KernelBase->kb_APIC_BaseMap[0] = lapic_addr_ovr->address;
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_Addr_Ovr_Parse: Local APIC address Override to 0x%p\n", KernelBase->kb_APIC_BaseMap[0]);
    }
    
    return 1;
    
    AROS_USERFUNC_EXIT
}

/* Process the 'Local APIC Non-Maskable Interrupt' MADT Table */
AROS_UFH1(IPTR, ACPI_hook_Table_LAPIC_NMI_Parse,
    AROS_UFHA(struct acpi_madt_entry_hook *,	table_hook,	A0))
{
    AROS_USERFUNC_INIT

    D(rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_NMI_Parse()\n"));

    struct acpi_table_lapic_nmi *lapic_nmi = NULL;

    if (!table_hook->header)
    {
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_NMI_Parse: Illegal LAPIC_NMI Table  Addr\n");
        return 0;
    }

    lapic_nmi = (struct acpi_table_lapic_nmi *) table_hook->header;

    if (lapic_nmi->lint != 1)
    {
        rkprintf("[Kernel] (HOOK) ACPI_hook_Table_LAPIC_NMI_Parse: APIC ID %d: WARNING - NMI not connected to LINT1!\n", lapic_nmi->acpi_id);
    }

    return 1;

    AROS_USERFUNC_EXIT
}

/* Process the 'IO-APIC' MADT Table */
AROS_UFH1(IPTR, ACPI_hook_Table_IOAPIC_Parse,
    AROS_UFHA(struct acpi_madt_entry_hook *,	table_hook,	A0))
{
    AROS_USERFUNC_INIT

    D(rkprintf("[Kernel] (HOOK) ACPI_hook_Table_IOAPIC_Parse()\n"));

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
AROS_UFH1(IPTR, ACPI_hook_Table_Int_Src_Ovr_Parse,
    AROS_UFHA(struct acpi_madt_entry_hook *,	table_hook,	A0))
{
    AROS_USERFUNC_INIT

    D(rkprintf("[Kernel] (HOOK) ACPI_hook_Table_Int_Src_Ovr_Parse()\n"));

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
AROS_UFH1(IPTR, ACPI_hook_Table_NMI_Src_Parse,
    AROS_UFHA(struct acpi_madt_entry_hook *,	table_hook,	A0))
{
    AROS_USERFUNC_INIT

    D(rkprintf("[Kernel] (HOOK) ACPI_hook_Table_NMI_Src_Parse()\n"));

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
AROS_UFH1(IPTR, ACPI_hook_Table_HPET_Parse,
    AROS_UFHA(struct acpi_table_hook *,	table_hook,	A0))
{
    AROS_USERFUNC_INIT

    D(rkprintf("[Kernel] (HOOK) ACPI_hook_Table_HPET_Parse()\n"));

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
