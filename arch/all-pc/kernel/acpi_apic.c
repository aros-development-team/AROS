/*
    Copyright � 1995-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

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

#define D(x)

#define ACPI_MODPRIO_APIC       100

#if (__WORDSIZE==64)
extern struct KernBootPrivate *__KernBootPrivate;
#endif

/************************************************************************************************
                                    ACPI APIC RELATED FUNCTIONS
 ************************************************************************************************/

const char *ACPI_TABLE_MADT_STR __attribute__((weak)) = "APIC";

void acpi_APIC_AllocPrivate(struct PlatformData *pdata)
{
    if (!pdata->kb_APIC)
    {
        pdata->kb_APIC = AllocMem(sizeof(struct APICData) + pdata->kb_ACPI->acpi_apicCnt * sizeof(struct CPUData), MEMF_CLEAR);
        pdata->kb_APIC->apic_count	= 1;		/* Only one CPU is running right now */

        D(bug("[Kernel:ACPI-APIC] Local APIC Private @ 0x%p, for %d APIC's\n", pdata->kb_APIC, pdata->kb_ACPI->acpi_apicCnt));
    }
}

/* Process the 'Local APIC Address Overide' MADT Table */
AROS_UFH3(static IPTR, ACPI_hook_Table_LAPIC_Addr_Ovr_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(ACPI_MADT_LOCAL_APIC_OVERRIDE *, lapic_addr_ovr, A2),
	  AROS_UFHA(struct ACPI_TABLESCAN_DATA *, tsdata, A1))
{
    AROS_USERFUNC_INIT

    struct PlatformData *pdata = tsdata->acpits_UserData;

    D(bug("[Kernel:ACPI-APIC] ## %s()\n", __func__));

    if (!pdata->kb_APIC)
    {
        ACPI_TABLE_MADT *madtTable = (ACPI_TABLE_MADT *)tsdata->acpits_Table;

        acpi_APIC_AllocPrivate(pdata);
        pdata->kb_ACPI->acpi_madt = madtTable;	/* Cache ACPI data for secondary cores */
        pdata->kb_APIC->flags = ((madtTable->Flags & ACPI_MADT_PCAT_COMPAT) == ACPI_MADT_MULTIPLE_APIC) ? APF_8259 : 0;
    }

    pdata->kb_APIC->lapicBase = lapic_addr_ovr->Address;

    D(bug("[Kernel:ACPI-APIC]    %s: Local APIC address Override to 0x%p\n", __func__, pdata->kb_APIC->lapicBase));

    return TRUE;

    AROS_USERFUNC_EXIT
}

/* Process the 'Local APIC Non-Maskable Interrupt' MADT Table */
AROS_UFH3(IPTR, ACPI_hook_Table_LAPIC_NMI_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(ACPI_MADT_LOCAL_APIC_NMI *, lapic_nmi, A2),
	  AROS_UFHA(struct PlatformData *, pdata, A1))
{
    AROS_USERFUNC_INIT

    IPTR cpu_num = (IPTR)table_hook->h_Data;

    D(bug("[Kernel:ACPI-APIC] ## %s()\n", __func__));

    if ((lapic_nmi->ProcessorId == pdata->kb_APIC->cores[cpu_num].cpu_PrivateID) || (lapic_nmi->ProcessorId == 0xff))
    {
        UWORD reg;
        ULONG val = LVT_MT_NMI;	/* This is the default (edge-triggered, active low) */

        D(bug("[Kernel:ACPI-APIC.%u]    %s: NMI LINT%u\n", cpu_num, __func__, lapic_nmi->Lint));

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
	    D(bug("[Kernel:ACPI-APIC.%u]    %s: NMI active low\n", cpu_num, __func__));
            val |= LVT_ACTIVE_LOW;
        } 

	if ((lapic_nmi->IntiFlags & ACPI_MADT_TRIGGER_MASK) == ACPI_MADT_TRIGGER_LEVEL)
	{
	    D(bug("[Kernel:ACPI-APIC.%u]    %s: NMI level-triggered\n", cpu_num, __func__));
	    val |= LVT_TGM_LEVEL;
	}

	APIC_REG(pdata->kb_APIC->lapicBase, reg) = val;
	return TRUE;
    }

    return FALSE;

    AROS_USERFUNC_EXIT
}

/*
 * Initialize APIC on a CPU core with specified number.
 * This routine is ran by all cores.
 */
void acpi_APIC_InitCPU(struct PlatformData *pdata, IPTR cpuNum)
{
    D(bug("[Kernel:ACPI-APIC] %s(%d)\n", __func__, cpuNum));

    /* Initialize APIC to the default configuration */
    core_APIC_Init(pdata->kb_APIC, cpuNum);

    if (pdata->kb_ACPI->acpi_madt)
    {
        struct Hook hook;

        D(bug("[Kernel:ACPI-APIC] %s: Parsing NMI ..\n", __func__));

        /* Set up NMI for ourselves */
        hook.h_Entry = (APTR)ACPI_hook_Table_LAPIC_NMI_Parse;
        hook.h_Data  = (APTR)cpuNum;
        acpi_ScanTableEntries(pdata->kb_ACPI->acpi_madt, sizeof(ACPI_TABLE_MADT), ACPI_MADT_TYPE_LOCAL_APIC_NMI, &hook, pdata);
    }
}

/*
 * Process the 'Local APIC' MADT Table.
 * This function collects APIC IDs.
 */
AROS_UFH3(static IPTR, ACPI_hook_Table_LAPIC_Parse,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(ACPI_MADT_LOCAL_APIC *, processor, A2),
	  AROS_UFHA(struct ACPI_TABLESCAN_DATA *, tsdata, A1))
{
    AROS_USERFUNC_INIT

    struct PlatformData *pdata = tsdata->acpits_UserData;

    D(bug("[Kernel:ACPI-APIC] ## %s()\n", __func__));

    if (!pdata->kb_APIC)
    {
        ACPI_TABLE_MADT *madtTable = (ACPI_TABLE_MADT *)tsdata->acpits_Table;

        acpi_APIC_AllocPrivate(pdata);
        pdata->kb_APIC->lapicBase = madtTable->Address;
        pdata->kb_ACPI->acpi_madt = madtTable;	/* Cache ACPI data for secondary cores */
        pdata->kb_APIC->flags = ((madtTable->Flags & ACPI_MADT_PCAT_COMPAT) == ACPI_MADT_MULTIPLE_APIC) ? APF_8259 : 0;

        bug("[Kernel:ACPI-APIC] Local APIC address 0x%p; Flags 0x%04X\n", pdata->kb_APIC->lapicBase, pdata->kb_APIC->flags);
        D(bug("[Kernel:ACPI-APIC] MADT @ 0x%p\n", pdata->kb_ACPI->acpi_madt));
    }

    if ((pdata->kb_APIC) && (processor->LapicFlags & ACPI_MADT_ENABLED))
    {
	if (pdata->kb_APIC->cores[0].cpu_LocalID == processor->Id)
	{
	    /* This is the BSP, slot 0 is always reserved for it. */
	    bug("[Kernel:ACPI-APIC] Registering APIC #0 [ID=0x%02X] as BSP\n", processor->Id);

	    pdata->kb_APIC->cores[0].cpu_PrivateID = processor->ProcessorId;

            /* Remember ID of the bootstrap APIC, this is CPU #0 */
            pdata->kb_APIC->cores[0].cpu_LocalID = core_APIC_GetID(pdata->kb_APIC->lapicBase);
            D(bug("[Kernel:ACPI-APIC] BSP ID: 0x%02X\n", pdata->kb_APIC->cores[0].cpu_LocalID));

#if (__WORDSIZE==64)
            pdata->kb_APIC->cores[0].cpu_GDT = __KernBootPrivate->BOOTGDT;
            pdata->kb_APIC->cores[0].cpu_IDT = __KernBootPrivate->BOOTIDT;
#endif

            /* Initialize LAPIC for ourselves (CPU #0) */
            acpi_APIC_InitCPU(pdata, 0);
	}
	else
	{
	    /* Add one more AP */
	    bug("[Kernel:ACPI-APIC] Registering APIC #%d [ID=0x%02X:0x%02X]\n", pdata->kb_APIC->apic_count, processor->Id, processor->ProcessorId);

	    pdata->kb_APIC->cores[pdata->kb_APIC->apic_count].cpu_LocalID = processor->Id;
	    pdata->kb_APIC->cores[pdata->kb_APIC->apic_count].cpu_PrivateID = processor->ProcessorId;

	    pdata->kb_APIC->apic_count++;
	}

	return TRUE;
    }

    return FALSE;

    AROS_USERFUNC_EXIT
}

/*
 * Process the 'Local APIC' MADT Table.
 * This function counts the available APICs.
 */
AROS_UFH3(static IPTR, ACPI_hook_Table_LAPIC_Count,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(ACPI_MADT_LOCAL_APIC *, processor, A2),
	  AROS_UFHA(struct ACPI_TABLESCAN_DATA *, tsdata, A1))
{
    AROS_USERFUNC_INIT

    struct PlatformData *pdata = tsdata->acpits_UserData;
    struct ACPI_TABLE_HOOK *scanHook;

    D(bug("[Kernel:ACPI-APIC] ## %s()\n", __func__));

    if (pdata->kb_ACPI->acpi_apicCnt == 0)
    {
        /*
     	 * The local APIC base address is obtained from the MADT (32-bit value) and
     	 * (optionally) overriden by a LAPIC_ADDR_OVR entry (64-bit value).
     	 */
        scanHook = (struct ACPI_TABLE_HOOK *)AllocMem(sizeof(struct ACPI_TABLE_HOOK), MEMF_CLEAR);
        if (scanHook)
        {
            scanHook->acpith_Node.ln_Name = (char *)ACPI_TABLE_MADT_STR;
            scanHook->acpith_Node.ln_Pri = ACPI_MODPRIO_APIC - 10;                            /* Queue 10 priority levels after the module parser */
            scanHook->acpith_Hook.h_Entry = (APTR)ACPI_hook_Table_LAPIC_Addr_Ovr_Parse;
            scanHook->acpith_HeaderLen = sizeof(ACPI_TABLE_MADT);
            scanHook->acpith_EntryType = ACPI_MADT_TYPE_LOCAL_APIC_OVERRIDE;
            scanHook->acpith_UserData = pdata;
            Enqueue(&pdata->kb_ACPI->acpi_tablehooks, &scanHook->acpith_Node);
        }

        scanHook = (struct ACPI_TABLE_HOOK *)AllocMem(sizeof(struct ACPI_TABLE_HOOK), MEMF_CLEAR);
        if (scanHook)
        {
            scanHook->acpith_Node.ln_Name = (char *)ACPI_TABLE_MADT_STR;
            scanHook->acpith_Node.ln_Pri = ACPI_MODPRIO_APIC - 20;                            /* Queue 20 priority levels after the module parser */
            scanHook->acpith_Hook.h_Entry = (APTR)ACPI_hook_Table_LAPIC_Parse;
            scanHook->acpith_HeaderLen = sizeof(ACPI_TABLE_MADT);
            scanHook->acpith_EntryType = ACPI_MADT_TYPE_LOCAL_APIC;
            scanHook->acpith_UserData = pdata;
            Enqueue(&pdata->kb_ACPI->acpi_tablehooks, &scanHook->acpith_Node);
        }
    }
    pdata->kb_ACPI->acpi_apicCnt++;

    return TRUE;

    AROS_USERFUNC_EXIT
}

void ACPI_APIC_SUPPORT(struct PlatformData *pdata)
{
    struct ACPI_TABLE_HOOK *scanHook;

    scanHook = (struct ACPI_TABLE_HOOK *)AllocMem(sizeof(struct ACPI_TABLE_HOOK), MEMF_CLEAR);
    if (scanHook)
    {
        D(bug("[Kernel:ACPI-APIC] Registering APIC Table Parser...\n"));
        D(bug("[Kernel:ACPI-APIC] %s: Table Hook @ 0x%p\n", __func__, scanHook));
        scanHook->acpith_Node.ln_Name = (char *)ACPI_TABLE_MADT_STR;
        scanHook->acpith_Node.ln_Pri = ACPI_MODPRIO_APIC;
        scanHook->acpith_Hook.h_Entry = (APTR)ACPI_hook_Table_LAPIC_Count;
        scanHook->acpith_HeaderLen = sizeof(ACPI_TABLE_MADT);
        scanHook->acpith_EntryType = ACPI_MADT_TYPE_LOCAL_APIC;
        scanHook->acpith_UserData = pdata;
        Enqueue(&pdata->kb_ACPI->acpi_tablehooks, &scanHook->acpith_Node);
    }
    D(bug("[Kernel:ACPI-APIC] Registering done\n"));
}

DECLARESET(KERNEL__ACPISUPPORT)
ADD2SET(ACPI_APIC_SUPPORT, KERNEL__ACPISUPPORT, 0)
