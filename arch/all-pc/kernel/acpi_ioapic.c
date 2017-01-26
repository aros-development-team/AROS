/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$
    
    http://download.intel.com/design/chipsets/datashts/29056601.pdf
*/

#include <aros/macros.h>
#include <aros/asmcall.h>
#include <proto/acpica.h>
#include <proto/exec.h>

#include <inttypes.h>
#include <string.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"

#include "acpi.h"
#include "apic.h"
#include "apic_ia32.h"
#include "ioapic.h"

#define D(x)
#define DINTR(x)

#define ACPI_MODPRIO_IOAPIC       50

/************************************************************************************************
                                    ACPI IO-APIC RELATED FUNCTIONS
 ************************************************************************************************/

const char *ACPI_TABLE_MADT_STR __attribute__((weak)) = "APIC";

#define IOREGSEL        0
#define IOREGWIN        10

 ULONG acpi_IOAPIC_ReadReg(APTR apic_base, UBYTE offset)
 {
     *(volatile ULONG *)(apic_base + IOREGSEL) = offset;
     return *(volatile ULONG *)(apic_base + IOREGWIN);
 }

void acpi_IOAPIC_WriteReg(APTR apic_base, UBYTE offset, ULONG val)
 {
     *(volatile ULONG *)(apic_base + IOREGSEL) = offset;
     *(volatile ULONG *)(apic_base + IOREGWIN) = val;
 }


/* IO-APIC Interrupt Functions ... ***************************/

struct IOAPICInt_Private
{
    
};

icid_t IOAPICInt_Register(struct KernelBase *KernelBase)
{
    D(bug("[Kernel:IOAPIC] %s()\n", __func__));

    /* if we have been disabled, fail to register */
    if (IOAPICInt_IntrController.ic_Flags & ICF_DISABLED)
        return -1;

    return (icid_t)IOAPICInt_IntrController.ic_Node.ln_Type;
}

BOOL IOAPICInt_Init(struct KernelBase *KernelBase, icid_t instanceCount)
{
    struct IOAPICCfgData *ioapicData;
    struct IOAPICData *ioapicPrivate = ((struct PlatformData *)KernelBase->kb_PlatformData)->kb_IOAPIC;
    int i;

    D(bug("[Kernel:IOAPIC] %s(%d)\n", __func__, instanceCount));

    IOAPICInt_IntrController.ic_Private = ioapicPrivate;

    for (i = 0; i < instanceCount; i++)
    {
        ioapicData = &ioapicPrivate->ioapics[i];
        D(bug("[Kernel:IOAPIC] %s: Init IOAPIC #%d @ 0x%p\n", __func__, ioapicData->ioapicID, ioapicData->ioapicBase));
    }

    return TRUE;
}

BOOL IOAPICInt_DisableIRQ(APTR icPrivate, icid_t icInstance, icid_t intNum)
{
//    struct IOAPICData *ioapicPrivate = (struct IOAPICData *)icPrivate;

    D(bug("[Kernel:IOAPIC] %s()\n", __func__));

    return TRUE;
}

BOOL IOAPICInt_EnableIRQ(APTR icPrivate, icid_t icInstance, icid_t intNum)
{
//    struct IOAPICData *ioapicPrivate = (struct IOAPICData *)icPrivate;

    D(bug("[Kernel:IOAPIC] %s()\n", __func__));

    return TRUE;
}

BOOL IOAPICInt_AckIntr(APTR icPrivate, icid_t icInstance, icid_t intNum)
{
//    struct IOAPICData *ioapicPrivate = (struct IOAPICData *)icPrivate;

    D(bug("[Kernel:IOAPIC] %s()\n", __func__));

    return TRUE;
}

struct IntrController IOAPICInt_IntrController =
{
    {
        .ln_Name = "82093AA IO-APIC"
    },
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
        D(bug("[Kernel:ACPI-IOAPIC] IO-APIC Private @ 0x%p, for %d IOAPIC's\n", pdata->kb_IOAPIC, pdata->kb_ACPI->acpi_ioapicCnt));
    }
}
 
/* Process the 'Interrupt Source' MADT Table */
AROS_UFH2(IPTR, ACPI_hook_Table_Int_Src_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(ACPI_MADT_INTERRUPT_SOURCE *, intsrc, A2))
{
    AROS_USERFUNC_INIT

    DINTR(bug("[Kernel:ACPI-IOAPIC] ## %s()\n", __func__));
    DINTR(bug("[Kernel:ACPI-IOAPIC]    %s: %d:%d, GSI %d, Flags 0x%x\n", __func__, intsrc->Id, intsrc->Eid,
                intsrc->GlobalIrq, intsrc->IntiFlags));
    DINTR(
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
            bug("[Kernel:ACPI-IOAPIC]    %s: corrected\n", __func__);
        }

    )
    return TRUE;

    AROS_USERFUNC_EXIT
}

/* Process the 'Interrupt Source Overide' MADT Table */
AROS_UFH2(IPTR, ACPI_hook_Table_Int_Src_Ovr_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(ACPI_MADT_INTERRUPT_OVERRIDE *, intsrc, A2))
{
    AROS_USERFUNC_INIT

    DINTR(bug("[Kernel:ACPI-IOAPIC] ## %s()\n", __func__));
    DINTR(bug("[Kernel:ACPI-IOAPIC]    %s: Bus %d, Source IRQ %d, GSI %d, Flags 0x%x\n", __func__, intsrc->Bus, intsrc->SourceIrq,
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

    DINTR(bug("[Kernel:ACPI-IOAPIC] ## %s()\n", __func__));
    DINTR(bug("[Kernel:ACPI-IOAPIC]    %s: GSI %d, Flags 0x%x\n", __func__, nmi_src->GlobalIrq, nmi_src->IntiFlags));

    /* FIXME: Uh... shouldn't we do something with this? */

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

    D(bug("[Kernel:ACPI-IOAPIC] ## %s()\n", __func__));

    if (!pdata->kb_IOAPIC)
    {
        acpi_IOAPIC_AllocPrivate(pdata);
    }

    if (pdata->kb_IOAPIC)
    {
        icintrid_t ioapicICInstID;
        ULONG ioapicval;
        int i;

        bug("[Kernel:ACPI-IOAPIC] Registering IO-APIC #%d [ID=0x%d] @ %p [GSI = %d]\n",
            pdata->kb_IOAPIC->ioapic_count, ioapic->Id, ioapic->Address, ioapic->GlobalIrqBase);

        if ((ioapicICInstID = krnAddInterruptController(KernelBase, &IOAPICInt_IntrController)) != -1)
        {
            D(bug("[Kernel:ACPI-IOAPIC] IO-APIC IC ID #%d:%d\n", ICINTR_ICID(ioapicICInstID), ICINTR_INST(ioapicICInstID)));

            pdata->kb_IOAPIC->ioapics[pdata->kb_IOAPIC->ioapic_count].ioapicBase = (APTR)0 + ioapic->Address;
            pdata->kb_IOAPIC->ioapics[pdata->kb_IOAPIC->ioapic_count].ioapicGSI = ioapic->GlobalIrqBase;

            ioapicval = acpi_IOAPIC_ReadReg(
                pdata->kb_IOAPIC->ioapics[pdata->kb_IOAPIC->ioapic_count].ioapicBase,
                IOAPICREG_ID);
            pdata->kb_IOAPIC->ioapics[pdata->kb_IOAPIC->ioapic_count].ioapicID = ((ioapicval >> 24) & 0xF);

            D(bug("[Kernel:ACPI-IOAPIC]    %s:       #%d,",
                __func__, pdata->kb_IOAPIC->ioapics[pdata->kb_IOAPIC->ioapic_count].ioapicID));
            ioapicval = acpi_IOAPIC_ReadReg(
                pdata->kb_IOAPIC->ioapics[pdata->kb_IOAPIC->ioapic_count].ioapicBase,
                IOAPICREG_VER);
            pdata->kb_IOAPIC->ioapics[pdata->kb_IOAPIC->ioapic_count].ioapicIRQs = ((ioapicval >> 16) & 0xF);
            pdata->kb_IOAPIC->ioapics[pdata->kb_IOAPIC->ioapic_count].ioapicVer = (ioapicval & 0xFF);
            D(bug(" ver %d, max irqs = %d,",
                pdata->kb_IOAPIC->ioapics[pdata->kb_IOAPIC->ioapic_count].ioapicVer, pdata->kb_IOAPIC->ioapics[pdata->kb_IOAPIC->ioapic_count].ioapicIRQs));
            ioapicval = acpi_IOAPIC_ReadReg(
                pdata->kb_IOAPIC->ioapics[pdata->kb_IOAPIC->ioapic_count].ioapicBase,
                IOAPICREG_ARB);
            D(bug("arb %d\n", ((ioapicval >> 24) & 0xF)));

            for (i = 0; i < pdata->kb_IOAPIC->ioapics[pdata->kb_IOAPIC->ioapic_count].ioapicIRQs; i++)
            {
                UQUAD tblentry;

                ioapicval = acpi_IOAPIC_ReadReg(
                    pdata->kb_IOAPIC->ioapics[pdata->kb_IOAPIC->ioapic_count].ioapicBase,
                    IOAPICREG_REDTBLBASE + i);
                tblentry = ((UQUAD)ioapicval << 32);
                D(bug("[Kernel:ACPI-IOAPIC]    %s:       %08x", __func__, ioapicval));
                ioapicval = acpi_IOAPIC_ReadReg(
                    pdata->kb_IOAPIC->ioapics[pdata->kb_IOAPIC->ioapic_count].ioapicBase,
                    IOAPICREG_REDTBLBASE + i + 1);
                tblentry |= ioapicval;
                D(bug("%08x", ioapicval));
                if (tblentry)
                {
                    D(bug(" :"));
                    if (tblentry & (1 << 13))
                    {
                        D(bug(" Active LOW,"));
                    }
                    else
                    {
                        D(bug(" Active HIGH,"));
                    }
                    
                    if (tblentry & (1 << 15))
                    {
                        D(bug(" LEVEL"));
                    }
                    else
                    {
                        D(bug(" EDGE"));
                    }

                    D(bug(" ->"));

                    if (tblentry & (1 << 11))
                    {
                        D(bug(" Logical %d:%d", ((tblentry >> 60) & 0xF), ((tblentry >> 56) & 0xF)));
                    }
                    else
                    {
                        D(bug(" Physical %d", ((tblentry >> 56) & 0xF)));
                    }
                }
                D(bug("\n"));
            }

            /* Build a default routing table for legacy (ISA) interrupts. */
            /* TODO: implement legacy irq config.. */
            D(bug("[Kernel:ACPI-IOAPIC]    %s: Configuring Legacy IRQs .. Skipped (UNIMPLEMENTED) ..\n", __func__));

            pdata->kb_IOAPIC->ioapic_count++;
        }
    }
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

    D(bug("[Kernel:ACPI-IOAPIC] ## %s()\n", __func__));

    if (pdata->kb_ACPI->acpi_ioapicCnt == 0)
    {
        D(bug("[Kernel:ACPI-IOAPIC]    %s: Registering IO-APIC Table Parser...\n", __func__));

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
            DINTR(bug("[Kernel:ACPI-IOAPIC]    %s: Registering Interrupt Source Table Parser...\n", __func__));

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
            DINTR(bug("[Kernel:ACPI-IOAPIC]    %s: Registering Interrupt Override Table Parser...\n", __func__));

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
            DINTR(bug("[Kernel:ACPI-IOAPIC]    %s: Registering NMI Source Table Parser...\n", __func__));

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
