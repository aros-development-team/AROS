/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>

#include <oop/oop.h>
#include <utility/tagitem.h>

#include <string.h>

#include "acpithermal_intern.h"

CONST_STRPTR    acpiThermal_str = "ACPI Thermal Zone";
static CONST_STRPTR    acpiThermal_telemetryId = "Temperature";
static CONST_STRPTR    acpiThermal_activeEntryIds[ACPITHERMAL_ACTIVE_MAX] =
{
    "Active0",
    "Active1",
    "Active2",
    "Active3",
    "Active4",
    "Active5",
    "Active6",
    "Active7",
    "Active8",
    "Active9"
};

static LONG ACPIThermal_ConvertToCelsius(ULONG temperature)
{
    LONG tenthsCelsius = (LONG)temperature - 2732;

    return tenthsCelsius / 10;
}

static BOOL ACPIThermal_ReadTemperature(struct HWACPIThermalData *data, CONST_STRPTR method, LONG *outValue)
{
    ACPI_BUFFER buffer = { ACPI_ALLOCATE_BUFFER, NULL };
    ACPI_OBJECT *object;
    ACPI_STATUS status;
    BOOL result = FALSE;

    if (!data->acpithermal_Handle)
        return FALSE;

    status = AcpiEvaluateObject(data->acpithermal_Handle, (ACPI_STRING)method, NULL, &buffer);
    if (ACPI_SUCCESS(status) && buffer.Pointer)
    {
        object = (ACPI_OBJECT *)buffer.Pointer;
        if (object->Type == ACPI_TYPE_INTEGER)
        {
            *outValue = ACPIThermal_ConvertToCelsius((ULONG)object->Integer.Value);
            result = TRUE;
        }
    }

    if (buffer.Pointer)
        FreeVec(buffer.Pointer);

    return result;
}

static BOOL ACPIThermal_HasMethod(ACPI_HANDLE handle, CONST_STRPTR method)
{
    ACPI_HANDLE methodHandle = NULL;

    if (!handle)
        return FALSE;

    return ACPI_SUCCESS(AcpiGetHandle(handle, (ACPI_STRING)method, &methodHandle));
}

OOP_Object *ACPIThermal__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct pRoot_New    acpiNewThermalMsg;
    struct Library      *UtilityBase = CSD(cl)->cs_UtilityBase;
    ACPI_HANDLE         acpiHandle = (ACPI_HANDLE)GetTagData(aHW_ACPIThermal_Handle, 0, msg->attrList);
    OOP_Object          *thermalO = NULL;
    ULONG telemetryCount = 1;
    ULONG flags = 0;
    ULONG activeMask = 0;
    ULONG i;

    D(bug("[ACPI:Thermal] %s()\n", __func__));

    struct TagItem new_tags[] =
    {
        { aHidd_Name,                   (IPTR)"acpithermal.hidd" },
        { aHidd_HardwareName,           (IPTR)acpiThermal_str       },
        { TAG_DONE,                     0                       }
    };
    acpiNewThermalMsg.mID      = msg->mID,
    acpiNewThermalMsg.attrList = new_tags;

    if (msg->attrList)
    {
        new_tags[2].ti_Tag  = TAG_MORE;
        new_tags[2].ti_Data = (IPTR)msg->attrList;
    }

    D(bug("[ACPI:Thermal] %s: ACPI Handle @ 0x%p\n", __func__, acpiHandle));

    if ((thermalO = (OOP_Object *)OOP_DoSuperMethod(cl, o, &acpiNewThermalMsg.mID)) != NULL)
    {
        struct HWACPIThermalData *data = OOP_INST_DATA(cl, thermalO);

        D(bug("[ACPI:Thermal] %s: Object @ 0x%p\n", __func__, thermalO));

        if (ACPIThermal_HasMethod(acpiHandle, "_CRT"))
        {
            flags |= ACPITHERMAL_FLAG_CRT;
            telemetryCount++;
        }
        if (ACPIThermal_HasMethod(acpiHandle, "_HOT"))
        {
            flags |= ACPITHERMAL_FLAG_HOT;
            telemetryCount++;
        }
        if (ACPIThermal_HasMethod(acpiHandle, "_PSV"))
        {
            flags |= ACPITHERMAL_FLAG_PSV;
            telemetryCount++;
        }

        for (i = 0; i < ACPITHERMAL_ACTIVE_MAX; ++i)
        {
            char method[5];

            method[0] = '_';
            method[1] = 'A';
            method[2] = 'C';
            method[3] = '0' + i;
            method[4] = '\0';

            if (ACPIThermal_HasMethod(acpiHandle, method))
            {
                activeMask |= (1U << i);
                telemetryCount++;
            }
        }

        data->acpithermal_TelemetryCount = telemetryCount;
        data->acpithermal_Handle = acpiHandle;
        data->acpithermal_Flags = flags;
        data->acpithermal_ActiveMask = activeMask;
    }
    return thermalO;
}

VOID ACPIThermal__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug("[ACPI:Thermal] %s()\n", __func__));
}


VOID ACPIThermal__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct HWACPIThermalData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    D(bug("[ACPI:Thermal] %s()\n", __func__));

    Hidd_Telemetry_Switch(msg->attrID, idx)
    {
    case aoHidd_Telemetry_EntryCount:
        *msg->storage = (IPTR)data->acpithermal_TelemetryCount;
        return;
    }

    HW_ACPIThermal_Switch(msg->attrID, idx)
    {
    case aoHW_ACPIThermal_Handle:
        *msg->storage = (IPTR)data->acpithermal_Handle;
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

BOOL ACPIThermal__Hidd_Telemetry__GetEntryAttribs(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Telemetry_GetEntryAttribs *msg)
{
    struct HWACPIThermalData *data = OOP_INST_DATA(cl, o);
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    struct TagItem *tstate;
    struct TagItem *tag;
    LONG value = 0;
    ULONG units = vHW_TelemetryUnit_Celsius;
    LONG minValue = 0;
    LONG maxValue = 0;
    BOOL readOnly = TRUE;
    CONST_STRPTR entryId = NULL;
    ULONG idx;
    ULONG i;
    BOOL found = FALSE;

    if (msg->index >= data->acpithermal_TelemetryCount)
        return FALSE;

    idx = msg->index;

    if (idx == 0)
    {
        entryId = acpiThermal_telemetryId;
        if (!ACPIThermal_ReadTemperature(data, "_TMP", &value))
            value = 0;
        found = TRUE;
    }
    else
    {
        idx--;
        if ((data->acpithermal_Flags & ACPITHERMAL_FLAG_CRT) != 0)
        {
            if (idx == 0)
            {
                entryId = "Critical";
                if (!ACPIThermal_ReadTemperature(data, "_CRT", &value))
                    value = 0;
                found = TRUE;
            }
            else
            {
                idx--;
            }
        }
        if (!found && (data->acpithermal_Flags & ACPITHERMAL_FLAG_HOT) != 0)
        {
            if (idx == 0)
            {
                entryId = "Hot";
                if (!ACPIThermal_ReadTemperature(data, "_HOT", &value))
                    value = 0;
                found = TRUE;
            }
            else
            {
                idx--;
            }
        }
        if (!found && (data->acpithermal_Flags & ACPITHERMAL_FLAG_PSV) != 0)
        {
            if (idx == 0)
            {
                entryId = "Passive";
                if (!ACPIThermal_ReadTemperature(data, "_PSV", &value))
                    value = 0;
                found = TRUE;
            }
            else
            {
                idx--;
            }
        }

        if (!found)
        {
            for (i = 0; i < ACPITHERMAL_ACTIVE_MAX; ++i)
            {
                if ((data->acpithermal_ActiveMask & (1U << i)) == 0)
                    continue;

                if (idx == 0)
                {
                    char method[5];

                    method[0] = '_';
                    method[1] = 'A';
                    method[2] = 'C';
                    method[3] = '0' + i;
                    method[4] = '\0';
                    entryId = acpiThermal_activeEntryIds[i];
                    if (!ACPIThermal_ReadTemperature(data, method, &value))
                        value = 0;
                    found = TRUE;
                    break;
                }
                idx--;
            }
        }
    }

    if (!found)
        return FALSE;

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
