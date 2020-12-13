/*
    Copyright © 2017-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/asmcall.h>
#include <proto/exec.h>
#include <proto/acpica.h>
#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>

#include <aros/symbolsets.h>

#include <resources/kernel.h>

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

    D(bug("[Kernel:ACPI] %s(0x%p)\n", __func__, table));

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

ACPI_STATUS acpiAttachDevResource(ACPI_RESOURCE *resource, void *Context)
{
    struct TagItem irqAttribs[] =
    {
        { KERNELTAG_IRQ_POLARITY,       0       },
        { KERNELTAG_IRQ_TRIGGERLEVEL,   0       },
        { TAG_DONE,                     0       }
    };
    D(
        bug("[Kernel:ACPI] %s(0x%p)\n", __func__, resource);
        bug("[Kernel:ACPI] %s: Res Type #%u\n", __func__, resource->Type);
    )
    switch (resource->Type) {
        case ACPI_RESOURCE_TYPE_IRQ:
            {
                struct acpi_resource_irq *irq  = &resource->Data.Irq;
                D(
                    bug("[Kernel:ACPI] %s: - ACPI_RESOURCE_TYPE_IRQ\n", __func__);
                    bug("[Kernel:ACPI] %s:       IRQ #%u, Pol = %u, TrigLvl = %u\n", __func__, irq->Interrupts[0], irq->Polarity, irq->Triggering);
                )
                if (irq->Triggering == ACPI_LEVEL_SENSITIVE)
                    irqAttribs[1].ti_Data = 1;
                else
                    irqAttribs[1].ti_Data = 2;

                if (irq->Polarity != ACPI_ACTIVE_HIGH)
                    irqAttribs[0].ti_Data = 2;
                else
                    irqAttribs[0].ti_Data = 1;

                /* Update delivery information */
                KrnModifyIRQA(irq->Interrupts[0], irqAttribs);
            }
            break;
        case ACPI_RESOURCE_TYPE_EXTENDED_IRQ:
            {
                struct acpi_resource_extended_irq *eirq = &resource->Data.ExtendedIrq;
                D(
                    bug("[Kernel:ACPI] %s: - ACPI_RESOURCE_TYPE_EXTENDED_IRQ\n", __func__);
                    bug("[Kernel:ACPI] %s:       IRQ #%u, Pol = %u, TrigLvl = %u\n", __func__, eirq->Interrupts[0], eirq->Polarity, eirq->Triggering);
                )
                if (eirq->Triggering == ACPI_LEVEL_SENSITIVE)
                    irqAttribs[1].ti_Data = 1;
                else
                    irqAttribs[1].ti_Data = 2;

                if (eirq->Polarity != ACPI_ACTIVE_HIGH)
                    irqAttribs[0].ti_Data = 2;
                else
                    irqAttribs[0].ti_Data = 1;

                /* Update delivery information */
                KrnModifyIRQA(eirq->Interrupts[0], irqAttribs);
            }
            break;
    }
    return AE_OK;
}

static ACPI_STATUS acpiInitDevice(ACPI_HANDLE handle, ULONG level, void *context, void **return_value)
{
    ACPI_DEVICE_INFO *devInfo = NULL;
    ACPI_STATUS             Status;

    D(bug("[Kernel:ACPI] %s(0x%p)\n", __func__, handle);)
 
    /* retrieve extra information about the device */
    Status = AcpiGetObjectInfo(handle, &devInfo);
    if (ACPI_SUCCESS(Status))
    {
#if (0)
        char devPath[80];
        ACPI_BUFFER nameBuffer = { .Length = 80, devPath };
        Status = AcpiGetName(handle, ACPI_FULL_PATHNAME, &nameBuffer);
        if (Status != AE_BUFFER_OVERFLOW)
        {
          bug("[Kernel:ACPI] %s: device '%s'\n", __func__, devPath);  
        }
#endif
        Status = AcpiWalkResources(handle, "_CRS", acpiAttachDevResource, NULL);
        if (ACPI_FAILURE(Status))
        {
            D(bug("[Kernel:ACPI] %s: failed to enumerate ACPI Device Resources\n", __func__));
        }
    }
    FreeVec(devInfo);
    return AE_OK;
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
            D(bug("[Kernel:ACPI] %s: Scanning ACPI Device tree..\n", __func__));
            Status = AcpiGetDevices(NULL, acpiInitDevice, pdata->kb_ACPI, NULL);
            if (ACPI_FAILURE(Status))
            {
                D(bug("[Kernel:ACPI] %s: failed to enumerate ACPI Devices\n", __func__));
            }
        }
    }
    D(bug("[Kernel:ACPI] %s: finished\n", __func__));
}
