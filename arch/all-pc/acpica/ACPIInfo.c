/*
 * Copyright (C) 2013, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 * 
 * $Id$
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/acpica.h>

#define SH_GLOBAL_SYSBASE 1
#define SH_GLOBAL_DOSBASE 1

#include <aros/shcommands.h>

const char * const TypeMap[ACPI_TYPE_EXTERNAL_MAX+1] = {
    "Any",
    "Integer",
    "String",
    "Buffer",
    "Package",
    "FieldUnit",
    "Device",
    "Event",
    "Method",
    "Mutex",
    "Region",
    "Power",
    "Processor",
    "Thermal",
    "BufferField",
    "DDBHandle",
    "DebugObject"
};

ACPI_STATUS OnDescend (
    ACPI_HANDLE                     Object,
    UINT32                          NestingLevel,
    void                            *Context,
    void                            **ReturnValue)
{
    int i;
    ACPI_STATUS err;
    ACPI_DEVICE_INFO *info;

    for (i = 0; i < NestingLevel; i++)
        Printf(" ");

    err = AcpiGetObjectInfo(Object, &info);
    if (err == AE_OK) {

        Printf("%c%c%c%c.",
                (info->Name >> 24) & 0xff,
                (info->Name >> 16) & 0xff,
                (info->Name >>  8) & 0xff,
                (info->Name >>  0) & 0xff);
        if (info->Type > ACPI_TYPE_EXTERNAL_MAX) {
            Printf("Type%ld", info->Type);
        } else {
            Printf("%s", TypeMap[info->Type]);
        }
        if (info->Type == ACPI_TYPE_METHOD) {
            Printf("(%ld)", info->ParamCount);
        }
        if (info->Flags & ACPI_PCI_ROOT_BRIDGE) {
            Printf(" [PCI Root Bridge]");
        }
        Printf("\n");
        for (i = 0; i < 8; i++) {
            int j;
            if ((info->Valid & (1 << i)) == 0)
                continue;

            for (j = 0; j < NestingLevel; j++)
                Printf(" ");
            Printf("    ");

            switch (info->Valid & (1 << i)) {
            case ACPI_VALID_STA:
                Printf("_STA: [");
                if (info->CurrentStatus & ACPI_STA_DEVICE_PRESENT)
                    Printf(" Present");
                if (info->CurrentStatus & ACPI_STA_DEVICE_ENABLED)
                    Printf(" Enabled");
                if (info->CurrentStatus & ACPI_STA_DEVICE_UI)
                    Printf(" UI");
                if (info->CurrentStatus & ACPI_STA_DEVICE_OK)
                    Printf(" Ok");
                if (info->CurrentStatus & ACPI_STA_BATTERY_PRESENT)
                    Printf(" Battery");
                Printf(" ]\n");
                break;
            case ACPI_VALID_ADR:
                Printf("_ADR: 0x%llx\n", info->Address);
                break;
            case ACPI_VALID_HID:
                Printf("_HID: %s\n",info->HardwareId.String);
                break;
            case ACPI_VALID_UID:
                Printf("_UID: %s\n",info->UniqueId.String);
                break;
            case ACPI_VALID_SUB:
                Printf("_SUB: %s\n",info->SubsystemId.String);
                break;
            case ACPI_VALID_CID:
                Printf("_CID: [");
                for (j = 0; j < info->CompatibleIdList.Count; j++) {
                    Printf(" %s", info->CompatibleIdList.Ids[j].String);
                }
                Printf(" ]\n");
                break;
            case ACPI_VALID_SXDS:
                Printf("_SxD:");
                for (j = 0; j < 4; j++) {
                    if (info->HighestDstates[j] != 0xff)
                        Printf(" %d", info->HighestDstates[j]);
                }
                Printf("\n");
                break;
            case ACPI_VALID_SXWS:
                Printf("_SxW:");
                for (j = 0; j < 5; j++) {
                    if (info->LowestDstates[j] != 0xff)
                        Printf(" %d", info->LowestDstates[j]);
                }
                Printf("\n");
                break;
            default: break;
            }
        }
    }

    return err;
}

ACPI_STATUS OnAscend (
    ACPI_HANDLE                     Object,
    UINT32                          NestingLevel,
    void                            *Context,
    void                            **ReturnValue)
{
    /* Nothing at the moment */

    return AE_OK;
}

AROS_SH0(ACPIInfo, 1.0)
{
    AROS_SHCOMMAND_INIT

    AcpiWalkNamespace(ACPI_TYPE_ANY, ACPI_ROOT_OBJECT, (UINT32)~0, OnDescend, OnAscend, NULL, NULL);

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
