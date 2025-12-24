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

static ULONG ACPIFan_ReadSpeed(struct HWACPIFanData *data)
{
    ACPI_BUFFER buffer = { ACPI_ALLOCATE_BUFFER, NULL };
    ACPI_OBJECT *object;
    ULONG speed = 0;
    ACPI_STATUS status;

    if (!data->acpifan_Handle)
        return 0;

    status = AcpiEvaluateObject(data->acpifan_Handle, "_FST", NULL, &buffer);
    if (ACPI_SUCCESS(status) && buffer.Pointer)
    {
        object = (ACPI_OBJECT *)buffer.Pointer;
        if (object->Type == ACPI_TYPE_INTEGER)
        {
            speed = (ULONG)object->Integer.Value;
        }
        else if (object->Type == ACPI_TYPE_PACKAGE &&
                 object->Package.Count > 2 &&
                 object->Package.Elements[2].Type == ACPI_TYPE_INTEGER)
        {
            speed = (ULONG)object->Package.Elements[2].Integer.Value;
        }
    }

    if (buffer.Pointer)
        FreeVec(buffer.Pointer);

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

        data->acpifan_TelemetryCount = 1;
        data->acpifan_Handle = acpiHandle;
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
    LONG value;
    ULONG units;
    LONG minValue;
    LONG maxValue;
    BOOL readOnly;
    CONST_STRPTR entryId;

    if (msg->index >= data->acpifan_TelemetryCount)
        return FALSE;

    entryId = acpiFan_telemetryId;
    units = vHW_TelemetryUnit_RPM;
    minValue = 0;
    maxValue = 0;
    value = (LONG)ACPIFan_ReadSpeed(data);
    readOnly = TRUE;

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
