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
static CONST_STRPTR    acpiBattery_telemetryId = "Charge";

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

static BOOL ACPIBattery_ReadBSTValue(struct HWACPIBatteryData *data, ULONG index, ULONG *value)
{
    if (!data->acpib_Handle)
        return FALSE;
    return ACPIBattery_EvaluatePackageInteger(data->acpib_Handle, "_BST", index, value);
}

static ULONG ACPIBattery_ReadRemainingCapacity(struct HWACPIBatteryData *data)
{
    ULONG remaining = 0;

    ACPIBattery_ReadBSTValue(data, 2, &remaining);
    return remaining;
}

static ULONG ACPIBattery_ReadRate(struct HWACPIBatteryData *data)
{
    ULONG rate = 0;

    ACPIBattery_ReadBSTValue(data, 1, &rate);
    return rate;
}

static ULONG ACPIBattery_ReadVoltage(struct HWACPIBatteryData *data)
{
    ULONG voltage = 0;

    ACPIBattery_ReadBSTValue(data, 3, &voltage);
    return voltage;
}

static ULONG ACPIBattery_ReadFullCapacity(struct HWACPIBatteryData *data)
{
    ULONG full = 0;

    if (!data->acpib_Handle)
        return 0;

    if (!ACPIBattery_EvaluatePackageInteger(data->acpib_Handle, "_BIX", 3, &full))
        ACPIBattery_EvaluatePackageInteger(data->acpib_Handle, "_BIF", 2, &full);

    return full;
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
        data->acpib_TelemetryCount = 5;
        
        data->acpib_Handle = acpiHandle;
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
    case aoHidd_Telemetry_EntryCount:
        *msg->storage = (IPTR)data->acpib_TelemetryCount;
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

BOOL ACPIBattery__Hidd_Telemetry__GetEntryAttribs(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Telemetry_GetEntryAttribs *msg)
{
    struct HWACPIBatteryData *data = OOP_INST_DATA(cl, o);
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    struct TagItem *tstate;
    struct TagItem *tag;
    LONG value = 0;
    ULONG units = vHW_TelemetryUnit_Unknown;
    LONG minValue = 0;
    LONG maxValue = 0;
    BOOL readOnly = TRUE;
    CONST_STRPTR entryId = NULL;
    ULONG fullCapacity = 0;
    ULONG remainingCapacity = 0;
    ULONG rate = 0;
    ULONG voltage = 0;

    if (msg->index >= data->acpib_TelemetryCount)
        return FALSE;

    remainingCapacity = ACPIBattery_ReadRemainingCapacity(data);
    fullCapacity = ACPIBattery_ReadFullCapacity(data);
    rate = ACPIBattery_ReadRate(data);
    voltage = ACPIBattery_ReadVoltage(data);

    switch (msg->index)
    {
    case 0:
        entryId = acpiBattery_telemetryId;
        units = vHW_TelemetryUnit_Percent;
        minValue = 0;
        maxValue = 100;
        value = (LONG)ACPIBattery_ReadTelemetryValue(data);
        break;
    case 1:
        entryId = "Capacity";
        units = vHW_TelemetryUnit_Raw;
        minValue = 0;
        maxValue = (LONG)fullCapacity;
        value = (LONG)remainingCapacity;
        break;
    case 2:
        entryId = "Full Capacity";
        units = vHW_TelemetryUnit_Raw;
        minValue = 0;
        maxValue = (LONG)fullCapacity;
        value = (LONG)fullCapacity;
        break;
    case 3:
        entryId = "Rate";
        units = vHW_TelemetryUnit_Watts;
        minValue = 0;
        maxValue = (LONG)rate;
        value = (LONG)rate;
        break;
    case 4:
        entryId = "Voltage";
        units = vHW_TelemetryUnit_Volts;
        minValue = 0;
        maxValue = (LONG)voltage;
        value = (LONG)voltage;
        break;
    default:
        return FALSE;
    }

    tstate = msg->tags;
    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
        case tHidd_Telemetry_EntryID:
            *(CONST_STRPTR *)tag->ti_Data = entryId;
            break;
        case tHidd_Telemetry_EntryUnits:
            *(ULONG *)tag->ti_Data = units;
            break;
        case tHidd_Telemetry_EntryMin:
            *(LONG *)tag->ti_Data = minValue;
            break;
        case tHidd_Telemetry_EntryMax:
            *(LONG *)tag->ti_Data = maxValue;
            break;
        case tHidd_Telemetry_EntryValue:
            *(LONG *)tag->ti_Data = value;
            break;
        case tHidd_Telemetry_EntryReadOnly:
            *(BOOL *)tag->ti_Data = readOnly;
            break;
        }
    }

    return TRUE;
}
