/*
    Copyright (C) 2022-2025, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/utility.h>

#include <oop/oop.h>
#include <utility/tagitem.h>

#include <string.h>

#include "acpibattery_intern.h"

CONST_STRPTR    acpiBattery_str = "ACPI Generic Battery Device";

static BOOL ACPIBattery_ReadPackageInteger(ACPI_OBJECT *obj, ULONG index, ULONG *value)
{
    if (!obj || obj->Type != ACPI_TYPE_PACKAGE)
        return FALSE;

    if (index >= obj->Package.Count)
        return FALSE;

    if (obj->Package.Elements[index].Type != ACPI_TYPE_INTEGER)
        return FALSE;

    *value = (ULONG)obj->Package.Elements[index].Integer.Value;
    return TRUE;
}

static BOOL ACPIBattery_EvaluatePackageInteger(ACPI_HANDLE handle, const char *method, ULONG index, ULONG *value)
{
    ACPI_BUFFER buffer = { ACPI_ALLOCATE_BUFFER, NULL };
    ACPI_STATUS status;
    BOOL ok = FALSE;

    status = AcpiEvaluateObject(handle, (ACPI_STRING)method, NULL, &buffer);
    if (ACPI_SUCCESS(status))
    {
        ok = ACPIBattery_ReadPackageInteger((ACPI_OBJECT *)buffer.Pointer, index, value);
    }

    if (buffer.Pointer)
        FreeVec(buffer.Pointer);

    return ok;
}

static LONG ACPIBattery_ReadTelemetryValue(struct HWACPIBatteryData *data)
{
    ULONG remaining = 0;
    ULONG full = 0;
    ULONG percent = 0;

    if (!data->acpib_Handle)
        return 0;

    if (!ACPIBattery_EvaluatePackageInteger(data->acpib_Handle, "_BST", 2, &remaining))
        return 0;

    if (!ACPIBattery_EvaluatePackageInteger(data->acpib_Handle, "_BIX", 3, &full))
        ACPIBattery_EvaluatePackageInteger(data->acpib_Handle, "_BIF", 2, &full);

    if (full > 0)
    {
        percent = (remaining * 100) / full;
        if (percent > 100)
            percent = 100;
    }

    return (LONG)percent;
}

OOP_Object *ACPIBattery__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct pRoot_New    acpiNewBatteryMsg;
    struct Library      *UtilityBase = CSD(cl)->cs_UtilityBase;
    ACPI_HANDLE         acpiHandle = (ACPI_HANDLE)GetTagData(aHW_ACPIBattery_Handle, 0, msg->attrList);
    OOP_Object          *batteryO = NULL;

    D(bug("[ACPI:Battery] %s()\n", __func__));

    struct TagItem new_tags[] =
    {
        { aHidd_Name,                   (IPTR)"acpibattery.hidd" },
        { aHidd_HardwareName,           (IPTR)acpiBattery_str       },
        { TAG_DONE,                     0                       }
    };
    acpiNewBatteryMsg.mID      = msg->mID,
    acpiNewBatteryMsg.attrList = new_tags;

    if (msg->attrList)
    {
        new_tags[2].ti_Tag  = TAG_MORE;
        new_tags[2].ti_Data = (IPTR)msg->attrList;
    }

    D(bug("[ACPI:Battery] %s: ACPI Handle @ 0x%p\n", __func__, acpiHandle));

    if ((batteryO = (OOP_Object *)OOP_DoSuperMethod(cl, o, &acpiNewBatteryMsg.mID)) != NULL)
    {
        struct HWACPIBatteryData *data = OOP_INST_DATA(cl, batteryO);
        
        D(bug("[ACPI:Battery] %s: Object @ 0x%p\n", __func__, batteryO));

        data->acpib_State = vHW_PowerState_NotPresent;
        data->acpib_Flags = vHW_PowerFlag_Unknown;

        data->acpib_Handle = acpiHandle;

        OOP_SetAttrsTags(batteryO,
            aHidd_Telemetry_Value, (IPTR)0,
            aHidd_Telemetry_Min, (IPTR)0,
            aHidd_Telemetry_Max, (IPTR)100,
            aHidd_Telemetry_Units, (IPTR)vHW_TelemetryUnit_Percent,
            TAG_DONE);
    }
    return batteryO;
}

VOID ACPIBattery__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug("[ACPI:Battery] %s()\n", __func__));
}


VOID ACPIBattery__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct HWACPIBatteryData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    D(bug("[ACPI:Battery] %s()\n", __func__));

    Hidd_Telemetry_Switch(msg->attrID, idx)
    {
    case aoHidd_Telemetry_Value:
        *msg->storage = (IPTR)ACPIBattery_ReadTelemetryValue(data);
        return;
    }

    HW_ACPIBattery_Switch(msg->attrID, idx)
    {
    case aoHW_ACPIBattery_Handle:
        *msg->storage = (IPTR)data->acpib_Handle;
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}
