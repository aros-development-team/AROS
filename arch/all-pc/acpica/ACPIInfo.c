/*
 * Copyright (C) 2013-2018, The AROS Development Team
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
#include <acpica/acnames.h>

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
    ACPI_BUFFER buffer = { ACPI_ALLOCATE_BUFFER, NULL };
    ACPI_PNP_DEVICE_ID_LIST *cIdList;
    ACPI_DEVICE_INFO    *info;
    ACPI_OBJECT         *obj;
    ACPI_PNP_DEVICE_ID  *pnpid;
    ACPI_STATUS         err;
    UINT64              addr;
    char                *dstates;
    int                 i;

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
        if (AcpiGetInfoFlags(info) & ACPI_PCI_ROOT_BRIDGE) {
            Printf(" [PCI Root Bridge]");
        }
        Printf("\n");

        addr = AcpiGetInfoAddress(info);
        if (addr) {
            for (i = 0; i < NestingLevel; i++)
                Printf(" ");
            Printf("    %s: 0x%llx\n", METHOD_NAME__ADR, addr);
        }

        pnpid = AcpiGetInfoHardwareId(info);
        if (pnpid) {
            for (i = 0; i < NestingLevel; i++)
                Printf(" ");
            Printf("    %s: %s\n", METHOD_NAME__HID, pnpid->String);
        }
        
        pnpid = AcpiGetInfoUniqueId(info);
        if (pnpid) {
            for (i = 0; i < NestingLevel; i++)
                Printf(" ");
            Printf("    %s: %s\n", METHOD_NAME__UID, pnpid->String);
        }

        cIdList = AcpiGetInfoCompatIdList(info);
        if (cIdList) {
            for (i = 0; i < NestingLevel; i++)
                Printf(" ");
            Printf("    %s: [", METHOD_NAME__CID);
            for (i = 0; i < cIdList->Count; i++) {
                Printf(" %s", cIdList->Ids[i].String);
            }
            Printf(" ]\n");
        }

        dstates = AcpiGetInfoHighDstates(info);
        if (dstates) {
            for (i = 0; i < NestingLevel; i++)
                Printf(" ");
            Printf("    _SxD:");
            for (i = 0; i < 4; i++) {
                if (dstates[i] != 0xff)
                    Printf(" %d", dstates[i]);
            }
            Printf("\n");
        }
        dstates = AcpiGetInfoLowDstates(info);
        if (dstates) {
            for (i = 0; i < NestingLevel; i++)
                Printf(" ");
            Printf("    _SxW:");
            for (i = 0; i < 5; i++) {
                if (dstates[i] != 0xff)
                    Printf(" %d", dstates[i]);
            }
            Printf("\n");
        }

	err = AcpiEvaluateObject (Object, METHOD_NAME__SUB,
            NULL, &buffer);
	if (ACPI_SUCCESS(err)) {
            obj = buffer.Pointer;

            if (obj->Type == ACPI_TYPE_STRING)
            {
                Printf("%s: %s\n", METHOD_NAME__SUB, obj->String.Pointer);
            }
            FreeVec(buffer.Pointer);
	}

 #if (0)
        buffer.Pointer = NULL;
        err = AcpiEvaluateObject (Object, METHOD_NAME__STA,
            NULL, &buffer);
        if (ACPI_SUCCESS(err)) {
            obj = buffer.Pointer;

            for (i = 0; i < NestingLevel; i++)
                Printf(" ");
            Printf("    %s: [", METHOD_NAME__STA);
            if (obj->Integer.Value & ACPI_STA_DEVICE_PRESENT)
                Printf(" Present");
            if (obj->Integer.Value & ACPI_STA_DEVICE_ENABLED)
                Printf(" Enabled");
            if (obj->Integer.Value & ACPI_STA_DEVICE_UI)
                Printf(" UI");
            if (obj->Integer.Value & ACPI_STA_DEVICE_OK)
                Printf(" Ok");
            if (obj->Integer.Value & ACPI_STA_BATTERY_PRESENT)
                Printf(" Battery");
            Printf(" ]\n");
            FreeVec(buffer.Pointer);
        }
#endif
        FreeVec(info);
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
