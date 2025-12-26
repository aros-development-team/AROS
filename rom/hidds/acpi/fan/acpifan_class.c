/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>

#include <oop/oop.h>
#include <utility/tagitem.h>

#include <string.h>

#include "acpifan_intern.h"

CONST_STRPTR    acpiFan_str = "ACPI Fan Device";
static CONST_STRPTR    acpiFan_telemetryId = "Speed";
static CONST_STRPTR    acpiFan_statusId = "Status";

static void ACPIFan_UpdateMinMax(ACPI_OBJECT *object, ULONG *minValue, ULONG *maxValue, BOOL *found)
{
    ULONG idx;

    if (!object)
        return;

    if (object->Type == ACPI_TYPE_INTEGER)
    {
        ULONG value = (ULONG)object->Integer.Value;

        if (!(*found))
        {
            *minValue = value;
            *maxValue = value;
            *found = TRUE;
        }
        else
        {
            if (value < *minValue)
                *minValue = value;
            if (value > *maxValue)
                *maxValue = value;
        }
        return;
    }

    if (object->Type != ACPI_TYPE_PACKAGE)
        return;

    for (idx = 0; idx < object->Package.Count; idx++)
    {
        ACPI_OBJECT *element = &object->Package.Elements[idx];

        if (element->Type == ACPI_TYPE_INTEGER)
        {
            ACPIFan_UpdateMinMax(element, minValue, maxValue, found);
        }
        else if (element->Type == ACPI_TYPE_PACKAGE)
        {
            ULONG inner;

            for (inner = 0; inner < element->Package.Count; inner++)
            {
                ACPI_OBJECT *innerElement = &element->Package.Elements[inner];
                if (innerElement->Type == ACPI_TYPE_INTEGER)
                    ACPIFan_UpdateMinMax(innerElement, minValue, maxValue, found);
            }
        }
    }
}

static void ACPIFan_ReadPerformanceStates(struct HWACPIFanData *data)
{
    ACPI_BUFFER buffer = { ACPI_ALLOCATE_BUFFER, NULL };
    ACPI_OBJECT *object;
    ACPI_STATUS status;
    ULONG minValue = 0;
    ULONG maxValue = 0;
    BOOL found = FALSE;

    if (!data->acpifan_Handle)
        return;

    status = AcpiEvaluateObject(data->acpifan_Handle, "_FPS", NULL, &buffer);
    if (ACPI_SUCCESS(status) && buffer.Pointer)
    {
        object = (ACPI_OBJECT *)buffer.Pointer;
        ACPIFan_UpdateMinMax(object, &minValue, &maxValue, &found);
    }

    if (buffer.Pointer)
        FreeVec(buffer.Pointer);

    if (found)
    {
        data->acpifan_HasFps = TRUE;
        data->acpifan_FpsMin = minValue;
        data->acpifan_FpsMax = maxValue;
    }
}

static BOOL ACPIFan_ReadStatusPackage(struct HWACPIFanData *data, ULONG *statusValue,
    ULONG *controlValue, ULONG *speedValue, BOOL *hasStatus, BOOL *hasControl)
{
    ACPI_BUFFER buffer = { ACPI_ALLOCATE_BUFFER, NULL };
    ACPI_OBJECT *object;
    ACPI_STATUS status;
    BOOL hasPackage = FALSE;
    BOOL speedFromElement1 = FALSE;

    if (statusValue)
        *statusValue = 0;
    if (controlValue)
        *controlValue = 0;
    if (speedValue)
        *speedValue = 0;
    if (hasStatus)
        *hasStatus = FALSE;
    if (hasControl)
        *hasControl = FALSE;

    if (!data->acpifan_Handle)
        return FALSE;

    status = AcpiEvaluateObject(data->acpifan_Handle, "_FST", NULL, &buffer);
    if (ACPI_SUCCESS(status) && buffer.Pointer)
    {
        object = (ACPI_OBJECT *)buffer.Pointer;
        if (object->Type == ACPI_TYPE_INTEGER)
        {
            if (speedValue)
                *speedValue = (ULONG)object->Integer.Value;
        }
        else if (object->Type == ACPI_TYPE_PACKAGE)
        {
            hasPackage = TRUE;
            if (object->Package.Count > 0 &&
                object->Package.Elements[0].Type == ACPI_TYPE_INTEGER)
            {
                if (statusValue)
                    *statusValue = (ULONG)object->Package.Elements[0].Integer.Value;
                if (hasStatus)
                    *hasStatus = TRUE;
            }
            if (object->Package.Count > 1 &&
                object->Package.Elements[1].Type == ACPI_TYPE_INTEGER)
            {
                speedFromElement1 = TRUE;
                if (speedValue)
                    *speedValue = (ULONG)object->Package.Elements[1].Integer.Value;
            }
            if (object->Package.Count > 2 &&
                object->Package.Elements[2].Type == ACPI_TYPE_INTEGER)
            {
                if (speedFromElement1)
                {
                    if (controlValue)
                        *controlValue = (ULONG)object->Package.Elements[2].Integer.Value;
                    if (hasControl)
                        *hasControl = TRUE;
                }
                else if (speedValue)
                {
                    *speedValue = (ULONG)object->Package.Elements[2].Integer.Value;
                }
            }
        }
    }

    if (buffer.Pointer)
        FreeVec(buffer.Pointer);

    return hasPackage;
}

static ULONG ACPIFan_ReadSpeed(struct HWACPIFanData *data)
{
    ULONG speed = 0;
    ACPIFan_ReadStatusPackage(data, NULL, NULL, &speed, NULL, NULL);
    
    return speed;
}

OOP_Object *ACPIFan__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct pRoot_New    acpiNewFanMsg;
    struct Library      *UtilityBase = CSD(cl)->cs_UtilityBase;
    ACPI_HANDLE         acpiHandle = (ACPI_HANDLE)GetTagData(aHW_ACPIFan_Handle, 0, msg->attrList);
    OOP_Object          *fanO = NULL;

    D(bug("[ACPI:Fan] %s()\n", __func__));

    struct TagItem new_tags[] =
    {
        { aHidd_Name,                   (IPTR)"acpifan.hidd" },
        { aHidd_HardwareName,           (IPTR)acpiFan_str       },
        { TAG_DONE,                     0                       }
    };
    acpiNewFanMsg.mID      = msg->mID,
    acpiNewFanMsg.attrList = new_tags;

    if (msg->attrList)
    {
        new_tags[2].ti_Tag  = TAG_MORE;
        new_tags[2].ti_Data = (IPTR)msg->attrList;
    }

    D(bug("[ACPI:Fan] %s: ACPI Handle @ 0x%p\n", __func__, acpiHandle));

    if ((fanO = (OOP_Object *)OOP_DoSuperMethod(cl, o, &acpiNewFanMsg.mID)) != NULL)
    {
        struct HWACPIFanData *data = OOP_INST_DATA(cl, fanO);

        D(bug("[ACPI:Fan] %s: Object @ 0x%p\n", __func__, fanO));

        data->acpifan_Handle = acpiHandle;
        data->acpifan_TelemetryCount = 1;
        data->acpifan_FpsMin = 0;
        data->acpifan_FpsMax = 0;
        data->acpifan_HasFps = FALSE;
        data->acpifan_HasStatus = FALSE;
        data->acpifan_HasControl = FALSE;

        ACPIFan_ReadPerformanceStates(data);
        ACPIFan_ReadStatusPackage(data, NULL, NULL, NULL,
            &data->acpifan_HasStatus, &data->acpifan_HasControl);
        if (data->acpifan_HasStatus || data->acpifan_HasControl)
            data->acpifan_TelemetryCount = 2;
    }
    return fanO;
}

VOID ACPIFan__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug("[ACPI:Fan] %s()\n", __func__));
}


VOID ACPIFan__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct HWACPIFanData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    D(bug("[ACPI:Fan] %s()\n", __func__));

    Hidd_Telemetry_Switch(msg->attrID, idx)
    {
    case aoHidd_Telemetry_EntryCount:
        *msg->storage = (IPTR)data->acpifan_TelemetryCount;
        return;
    }

    HW_ACPIFan_Switch(msg->attrID, idx)
    {
    case aoHW_ACPIFan_Handle:
        *msg->storage = (IPTR)data->acpifan_Handle;
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

BOOL ACPIFan__Hidd_Telemetry__GetEntryAttribs(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Telemetry_GetEntryAttribs *msg)
{
    struct HWACPIFanData *data = OOP_INST_DATA(cl, o);
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    struct TagItem *tstate;
    struct TagItem *tag;
    LONG value = 0;
    ULONG units = vHW_TelemetryUnit_Unknown;
    LONG minValue = 0;
    LONG maxValue = 0;
    BOOL readOnly = TRUE;
    CONST_STRPTR entryId = NULL;
    ULONG statusValue = 0;
    ULONG controlValue = 0;
    BOOL hasStatus = FALSE;
    BOOL hasControl = FALSE;

    if (msg->index >= data->acpifan_TelemetryCount)
        return FALSE;

    switch (msg->index)
    {
    case 0:
        entryId = acpiFan_telemetryId;
        units = vHW_TelemetryUnit_RPM;
        minValue = data->acpifan_HasFps ? (LONG)data->acpifan_FpsMin : 0;
        maxValue = data->acpifan_HasFps ? (LONG)data->acpifan_FpsMax : 0;
        value = (LONG)ACPIFan_ReadSpeed(data);
        break;
    case 1:
        if (!ACPIFan_ReadStatusPackage(data, &statusValue, &controlValue, NULL,
            &hasStatus, &hasControl))
            return FALSE;
        if (!(hasStatus || hasControl))
            return FALSE;
        entryId = acpiFan_statusId;
        units = vHW_TelemetryUnit_Raw;
        minValue = 0;
        maxValue = hasControl ? 3 : 1;
        value = 0;
        if (hasStatus && statusValue)
            value |= 0x1;
        if (hasControl && controlValue)
            value |= 0x2;
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
