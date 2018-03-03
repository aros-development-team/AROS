/*
    Copyright © 2017-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/asmcall.h>
#include <proto/exec.h>
#include <proto/acpica.h>
#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>

#include <aros/symbolsets.h>

#include <inttypes.h>
#include <string.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "kernel_syscall.h"

#include "acpi.h"
#include "apic.h"
#include "apic_ia32.h"
#include "i8259a.h"

#define D(x)

/************************************************************************************************/
/************************************************************************************************
                ACPI Functions used by kernel.resource from outside this file ..
 ************************************************************************************************/
/************************************************************************************************/

THIS_PROGRAM_HANDLES_SYMBOLSET(KERNEL__ACPISUPPORT)
DECLARESET(KERNEL__ACPISUPPORT)

int acpi_ScanTableEntries(CONST ACPI_TABLE_HEADER *table, ULONG thl, UINT8 type, const struct Hook *hook, APTR data)
{
    UINT8 *table_end  = (UINT8 *)table + table->Length;
    UINT8 *table_entry = (UINT8 *)table + thl;
    int count;

    for (count = 0; table_entry < table_end; table_entry += ((ACPI_SUBTABLE_HEADER *)table_entry)->Length) {
        ACPI_SUBTABLE_HEADER *sh = (ACPI_SUBTABLE_HEADER *)table_entry;
        if (sh->Type == (UINT8)type) {
            BOOL res;
            if (hook == NULL)
                res = TRUE;
            else
                res = CALLHOOKPKT((struct Hook *)hook, (APTR)sh, data);
            if (res)
                count++;
        }
    }

    return count;
}

/* Initialize ACPI */
void acpi_Init(struct PlatformData *pdata)
{
    acpi_supportinit_t *acpisupportInit;
    void **supportmoduleinit = (void **)SETNAME(KERNEL__ACPISUPPORT);
    struct ACPI_TABLE_HOOK *acpiTableHook = NULL;
    struct ACPI_TABLESCAN_DATA acpiTSData;
    char *tableLast = NULL;
    ACPI_STATUS             Status;
    int pos;

    D(bug("[Kernel:ACPI] %s(0x%p)\n", __func__, pdata));

    if (!pdata->kb_ACPI)
    {
        if ((pdata->kb_ACPI = (struct ACPIData *)AllocMem(sizeof(struct ACPIData), MEMF_CLEAR)) != NULL)
        {
            D(icintrid_t xtpicICInstID;)

            NEWLIST(&pdata->kb_ACPI->acpi_tablehooks);

            D(bug("[Kernel:ACPI] %s: Preparing ACPI support modules...\n", __func__));
            for (pos = 1; supportmoduleinit[pos] != NULL; pos++)
            {
                acpisupportInit = (acpi_supportinit_t *)supportmoduleinit[pos];
                D(bug("[Kernel:ACPI] %s: acpisupportInit @ 0x%p \n", __func__, acpisupportInit));
                acpisupportInit(pdata);
                D(bug("[Kernel:ACPI] %s:     returned!\n", __func__));
            }

            while (NULL != FindTask("ACPICA_InitTask"))
            {
                D(bug("[Kernel:ACPI] %s: Waiting for ACPI to finish Initializing...\n", __func__));
                /*
                 * N.B: We do not have a scheduling heartbeat at this
                 * point, so we must co-operatively yield CPU time to
                 * let the ACPICA Init task run.
                 */
                KrnSchedule();
            }

            if (!IsListEmpty(&pdata->kb_ACPI->acpi_tablehooks))
            {
                D(bug("[Kernel:ACPI] %s: Processing Table Handler Hooks...\n", __func__));
                ForeachNode(&pdata->kb_ACPI->acpi_tablehooks, acpiTableHook)
                {
                    if (acpiTableHook->acpith_Node.ln_Name)
                    {
                        D(bug("[Kernel:ACPI] %s: Table Hooks @ 0x%p for '%s'\n", __func__, acpiTableHook, acpiTableHook->acpith_Node.ln_Name));
                        if ((!tableLast) || (tableLast != acpiTableHook->acpith_Node.ln_Name))
                        {
                            D(bug("[Kernel:ACPI] %s: Trying to obtain Table...\n", __func__));
                            acpiTSData.acpits_Table = NULL;
                            if (AE_OK == (Status = AcpiGetTable(acpiTableHook->acpith_Node.ln_Name, 1, (ACPI_TABLE_HEADER **)&acpiTSData.acpits_Table)))
                            {
                                tableLast = acpiTableHook->acpith_Node.ln_Name;
                            }
                            else
                            {
                                D(bug("[Kernel:ACPI] %s: Failed! status %08x\n", __func__, Status));
                                tableLast = NULL;
                            }
                        }

                        D(bug("[Kernel:ACPI] %s: Table @ 0x%p\n", __func__, acpiTSData.acpits_Table));
                        if (acpiTSData.acpits_Table)
                        {
                            acpiTSData.acpits_UserData = acpiTableHook->acpith_UserData;
                            if (acpiTableHook->acpith_HeaderLen)
                                acpi_ScanTableEntries(acpiTSData.acpits_Table, acpiTableHook->acpith_HeaderLen, acpiTableHook->acpith_EntryType, &acpiTableHook->acpith_Hook, &acpiTSData);
                            else
                            {
                                CALLHOOKPKT((struct Hook *)&acpiTableHook->acpith_Hook, (APTR)acpiTSData.acpits_Table, &acpiTSData);
                            }
                        }
                    }
                    else
                    {
                        bug("[Kernel:ACPI] BUG: missing Table name @ 0x%p", acpiTableHook);
                    }
                }
                bug("[Kernel:ACPI] System Total APICs: %d", pdata->kb_ACPI->acpi_apicCnt);
                if (pdata->kb_APIC)
                {
                    bug(", %d usable", pdata->kb_APIC->apic_count);
                }
                bug("\n");
            }

            /* Initialize legacy 8259A PIC if present. */
            if ((pdata->kb_APIC) && (pdata->kb_APIC->flags & APF_8259))
            {
                D(xtpicICInstID =) krnAddInterruptController(KernelBase, &i8259a_IntrController);
                D(bug("[Kernel:ACPI] %s: Registered i8259A IC ID #%d:%d\n", __func__, ICINTR_ICID(xtpicICInstID), ICINTR_INST(xtpicICInstID)));
            }
        }
    }
}
