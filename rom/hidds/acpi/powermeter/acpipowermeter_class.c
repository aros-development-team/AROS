/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>

#include <oop/oop.h>
#include <utility/tagitem.h>

#include <string.h>

#include "acpipowermeter_intern.h"

CONST_STRPTR acpiPowerMeter_str = "ACPI Power Meter";

static BOOL ACPIPowerMeter_HasMethod(ACPI_HANDLE handle, CONST_STRPTR method)
{
    ACPI_HANDLE methodHandle = NULL;

    if (!handle)
        return FALSE;

    return ACPI_SUCCESS(AcpiGetHandle(handle, (ACPI_STRING)method, &methodHandle));
}

static BOOL ACPIPowerMeter_ReadInteger(ACPI_HANDLE handle, CONST_STRPTR method, LONG *outValue)
{
    ACPI_BUFFER buffer = { ACPI_ALLOCATE_BUFFER, NULL };
    ACPI_OBJECT *object;
    ACPI_STATUS status;
    BOOL result = FALSE;

    if (!handle)
        return FALSE;

    status = AcpiEvaluateObject(handle, (ACPI_STRING)method, NULL, &buffer);
    if (ACPI_SUCCESS(status) && buffer.Pointer)
    {
        object = (ACPI_OBJECT *)buffer.Pointer;
        if (object->Type == ACPI_TYPE_INTEGER)
        {
            *outValue = (LONG)object->Integer.Value;
            result = TRUE;
        }
    }

    if (buffer.Pointer)
        FreeVec(buffer.Pointer);

    return result;
}

static BOOL ACPIPowerMeter_ReadPackageInteger(ACPI_HANDLE handle, CONST_STRPTR method, ULONG index, LONG *outValue)
{
    ACPI_BUFFER buffer = { ACPI_ALLOCATE_BUFFER, NULL };
    ACPI_OBJECT *object;
    ACPI_STATUS status;
    BOOL result = FALSE;

    if (!handle)
        return FALSE;

    status = AcpiEvaluateObject(handle, (ACPI_STRING)method, NULL, &buffer);
    if (ACPI_SUCCESS(status) && buffer.Pointer)
    {
        object = (ACPI_OBJECT *)buffer.Pointer;
        if (object->Type == ACPI_TYPE_PACKAGE && object->Package.Count > index)
        {
            ACPI_OBJECT *element = &object->Package.Elements[index];
            if (element->Type == ACPI_TYPE_INTEGER)
            {
                *outValue = (LONG)element->Integer.Value;
                result = TRUE;
            }
        }
    }

    if (buffer.Pointer)
        FreeVec(buffer.Pointer);

    return result;
}

static LONG ACPIPowerMeter_ApplyScale(LONG value, ULONG scale)
{
    if (scale <= 1)
        return value;

    if (value >= 0)
        return (value + (LONG)(scale / 2)) / (LONG)scale;

    return (value - (LONG)(scale / 2)) / (LONG)scale;
}

static LONG ACPIPowerMeter_ConvertToCelsius(ULONG temperature)
{
    LONG tenthsCelsius = (LONG)temperature - 2732;

    return tenthsCelsius / 10;
}

static void ACPIPowerMeter_AddEntry(struct HWACPIPowerMeterData *data, CONST_STRPTR id,
    CONST_STRPTR method, ULONG units, ULONG scale, ULONG pkgIndex, BOOL usePackage, BOOL tempKelvin)
{
    struct ACPIPowerMeterEntry *entry;

    if (data->acpipm_TelemetryCount >= ACPIPM_MAX_ENTRIES)
        return;

    entry = &data->acpipm_Entries[data->acpipm_TelemetryCount++];
    entry->apm_Id = id;
    entry->apm_Method = method;
    entry->apm_Units = units;
    entry->apm_Scale = scale;
    entry->apm_PackageIndex = pkgIndex;
    entry->apm_UsePackage = usePackage;
    entry->apm_TempKelvin = tempKelvin;
}

static CONST_STRPTR ACPIPowerMeter_FindMethod(ACPI_HANDLE handle, CONST_STRPTR *methods)
{
    ULONG i = 0;

    while (methods[i])
    {
        if (ACPIPowerMeter_HasMethod(handle, methods[i]))
            return methods[i];
        i++;
    }

    return NULL;
}

static void ACPIPowerMeter_BuildEntries(struct HWACPIPowerMeterData *data)
{
    static CONST_STRPTR powerMethods[] = { "PWRS", "PWR", "POWR", NULL };
    static CONST_STRPTR energyMethods[] = { "ENRG", "ENGY", "ENER", NULL };
    static CONST_STRPTR voltageMethods[] = { "VOLT", "_VLT", NULL };
    static CONST_STRPTR temperatureMethods[] = { "TEMP", "_TMP", NULL };
    BOOL hasPower = FALSE;
    BOOL hasEnergy = FALSE;
    CONST_STRPTR method;

    data->acpipm_TelemetryCount = 0;

    if (ACPIPowerMeter_HasMethod(data->acpipm_Handle, "_PMM"))
    {
        ACPIPowerMeter_AddEntry(data, "Power", "_PMM", vHW_TelemetryUnit_Watts, 1000, 0, TRUE, FALSE);
        ACPIPowerMeter_AddEntry(data, "Energy (mWh)", "_PMM", vHW_TelemetryUnit_Raw, 1, 1, TRUE, FALSE);
        hasPower = TRUE;
        hasEnergy = TRUE;
    }

    if (!hasPower)
    {
        method = ACPIPowerMeter_FindMethod(data->acpipm_Handle, powerMethods);
        if (method)
        {
            ACPIPowerMeter_AddEntry(data, "Power", method, vHW_TelemetryUnit_Watts, 1000, 0, FALSE, FALSE);
            hasPower = TRUE;
        }
    }

    if (!hasEnergy)
    {
        method = ACPIPowerMeter_FindMethod(data->acpipm_Handle, energyMethods);
        if (method)
        {
            ACPIPowerMeter_AddEntry(data, "Energy (mWh)", method, vHW_TelemetryUnit_Raw, 1, 0, FALSE, FALSE);
        }
    }

    method = ACPIPowerMeter_FindMethod(data->acpipm_Handle, voltageMethods);
    if (method)
    {
        ACPIPowerMeter_AddEntry(data, "Voltage", method, vHW_TelemetryUnit_Volts, 1000, 0, FALSE, FALSE);
    }

    method = ACPIPowerMeter_FindMethod(data->acpipm_Handle, temperatureMethods);
    if (method)
    {
        BOOL tempKelvin = (method[0] == '_' && method[1] == 'T' && method[2] == 'M' && method[3] == 'P');
        ACPIPowerMeter_AddEntry(data, "Temperature", method, vHW_TelemetryUnit_Celsius, 1, 0, FALSE, tempKelvin);
    }
}

OOP_Object *ACPIPowerMeter__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct pRoot_New    acpiNewPMMsg;
    struct Library      *UtilityBase = CSD(cl)->cs_UtilityBase;
    ACPI_HANDLE         acpiHandle = (ACPI_HANDLE)GetTagData(aHW_ACPIPowerMeter_Handle, 0, msg->attrList);
    OOP_Object          *pmO = NULL;

    D(bug("[ACPI:PowerMeter] %s()\n", __func__));

    struct TagItem new_tags[] =
    {
        { aHidd_Name,                   (IPTR)"acpipowermeter.hidd" },
        { aHidd_HardwareName,           (IPTR)acpiPowerMeter_str       },
        { TAG_DONE,                     0                       }
    };
    acpiNewPMMsg.mID      = msg->mID,
    acpiNewPMMsg.attrList = new_tags;

    if (msg->attrList)
    {
        new_tags[2].ti_Tag  = TAG_MORE;
        new_tags[2].ti_Data = (IPTR)msg->attrList;
    }

    D(bug("[ACPI:PowerMeter] %s: ACPI Handle @ 0x%p\n", __func__, acpiHandle));

    if ((pmO = (OOP_Object *)OOP_DoSuperMethod(cl, o, &acpiNewPMMsg.mID)) != NULL)
    {
        struct HWACPIPowerMeterData *data = OOP_INST_DATA(cl, pmO);

        D(bug("[ACPI:PowerMeter] %s: Object @ 0x%p\n", __func__, pmO));

        data->acpipm_Handle = acpiHandle;
        ACPIPowerMeter_BuildEntries(data);
    }
    return pmO;
}

VOID ACPIPowerMeter__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug("[ACPI:PowerMeter] %s()\n", __func__));
}

VOID ACPIPowerMeter__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct HWACPIPowerMeterData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    D(bug("[ACPI:PowerMeter] %s()\n", __func__));

    Hidd_Telemetry_Switch(msg->attrID, idx)
    {
    case aoHidd_Telemetry_EntryCount:
        *msg->storage = (IPTR)data->acpipm_TelemetryCount;
        return;
    }

    HW_ACPIPowerMeter_Switch(msg->attrID, idx)
    {
    case aoHW_ACPIPowerMeter_Handle:
        *msg->storage = (IPTR)data->acpipm_Handle;
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

static BOOL ACPIPowerMeter_ReadEntry(struct HWACPIPowerMeterData *data, struct ACPIPowerMeterEntry *entry,
    LONG *outValue)
{
    LONG value = 0;
    BOOL ok;

    if (entry->apm_UsePackage)
        ok = ACPIPowerMeter_ReadPackageInteger(data->acpipm_Handle, entry->apm_Method, entry->apm_PackageIndex, &value);
    else
        ok = ACPIPowerMeter_ReadInteger(data->acpipm_Handle, entry->apm_Method, &value);

    if (!ok)
        return FALSE;

    if (entry->apm_TempKelvin)
        value = ACPIPowerMeter_ConvertToCelsius((ULONG)value);
    else
        value = ACPIPowerMeter_ApplyScale(value, entry->apm_Scale);

    *outValue = value;
    return TRUE;
}

BOOL ACPIPowerMeter__Hidd_Telemetry__GetEntryAttribs(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Telemetry_GetEntryAttribs *msg)
{
    struct HWACPIPowerMeterData *data = OOP_INST_DATA(cl, o);
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    struct TagItem *tstate;
    struct TagItem *tag;
    LONG value = 0;
    LONG minValue = 0;
    LONG maxValue = 0;
    BOOL readOnly = TRUE;
    struct ACPIPowerMeterEntry *entry;

    if (msg->index >= data->acpipm_TelemetryCount)
        return FALSE;

    entry = &data->acpipm_Entries[msg->index];

    if (!ACPIPowerMeter_ReadEntry(data, entry, &value))
        value = 0;

    tstate = msg->tags;
    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
        case tHidd_Telemetry_EntryID:
            *(CONST_STRPTR *)tag->ti_Data = entry->apm_Id;
            break;
        case tHidd_Telemetry_EntryUnits:
            *(ULONG *)tag->ti_Data = entry->apm_Units;
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
