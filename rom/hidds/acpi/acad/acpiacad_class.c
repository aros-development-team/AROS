/*
    Copyright (C) 2022-2025, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/utility.h>

#include <oop/oop.h>
#include <utility/tagitem.h>

#include <string.h>

#include "acpiacad_intern.h"

CONST_STRPTR    acpiACAd_str = "ACPI AC Adapter Device";
static CONST_STRPTR    acpiACAd_telemetryId = "AC Present";

static ULONG ACPIACAd_ReadPowerPresent(struct HWACPIACAdData *data)
{
    ACPI_BUFFER buffer = { ACPI_ALLOCATE_BUFFER, NULL };
    ULONG present = 0;
    ACPI_STATUS status;
    ACPI_OBJECT *object;

    if (!data->acpiacad_Handle)
        return 0;

    status = AcpiEvaluateObject(data->acpiacad_Handle, "_PSR", NULL, &buffer);
    if (ACPI_SUCCESS(status) && buffer.Pointer)
    {
        object = (ACPI_OBJECT *)buffer.Pointer;
        if (object->Type == ACPI_TYPE_INTEGER)
            present = (ULONG)object->Integer.Value;
    }

    if (buffer.Pointer)
        FreeVec(buffer.Pointer);

    return present ? 1 : 0;
}

OOP_Object *ACPIACAd__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct pRoot_New    acpiNewACAdMsg;
    struct Library      *UtilityBase = CSD(cl)->cs_UtilityBase;
    ACPI_HANDLE         acpiHandle = (ACPI_HANDLE)GetTagData(aHW_ACPIACAd_Handle, 0, msg->attrList);
    OOP_Object          *acadO = NULL;

    D(bug("[ACPI:ACAd] %s()\n", __func__));

    struct TagItem new_tags[] =
    {
        { aHidd_Name,                   (IPTR)"acpiacad.hidd" },
        { aHidd_HardwareName,           (IPTR)acpiACAd_str       },
        { TAG_DONE,                     0                       }
    };
    acpiNewACAdMsg.mID      = msg->mID,
    acpiNewACAdMsg.attrList = new_tags;

    if (msg->attrList)
    {
        new_tags[2].ti_Tag  = TAG_MORE;
        new_tags[2].ti_Data = (IPTR)msg->attrList;
    }

    D(bug("[ACPI:ACAd] %s: ACPI Handle @ 0x%p\n", __func__, acpiHandle));

    if ((acadO = (OOP_Object *)OOP_DoSuperMethod(cl, o, &acpiNewACAdMsg.mID)) != NULL)
    {
        struct HWACPIACAdData *data = OOP_INST_DATA(cl, acadO);
        
        D(bug("[ACPI:ACAd] %s: Object @ 0x%p\n", __func__, acadO));

        data->acpiacad_State = vHW_PowerState_NotPresent;
        data->acpiacad_Flags = vHW_PowerFlag_Unknown;
        data->acpiacad_TelemetryCount = 1;

        data->acpiacad_Handle = acpiHandle;
    }
    return acadO;
}

VOID ACPIACAd__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug("[ACPI:ACAd] %s()\n", __func__));
}


VOID ACPIACAd__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct HWACPIACAdData *data = OOP_INST_DATA(cl, o);
    ULONG idx;
    ULONG present;

    D(bug("[ACPI:ACAd] %s()\n", __func__));

    Hidd_Telemetry_Switch(msg->attrID, idx)
    {
    case aoHidd_Telemetry_EntryCount:
        *msg->storage = (IPTR)data->acpiacad_TelemetryCount;
        return;
    }

    Hidd_Power_Switch(msg->attrID, idx)
    {
    case aoHidd_Power_Type:
        *msg->storage = (IPTR)vHW_PowerType_AC;
        return;
    case aoHidd_Power_State:
        present = ACPIACAd_ReadPowerPresent(data);
        *msg->storage = (IPTR)(present ? vHW_PowerState_Charging : vHW_PowerState_NotPresent);
        return;
    case aoHidd_Power_Flags:
        present = ACPIACAd_ReadPowerPresent(data);
        *msg->storage = (IPTR)(present ? vHW_PowerFlag_High : vHW_PowerFlag_Unknown);
        return;
    case aoHidd_Power_Units:
        *msg->storage = (IPTR)vHW_PowerUnit_mW;
        return;
    }

    HW_ACPIACAd_Switch(msg->attrID, idx)
    {
    case aoHW_ACPIACAd_Handle:
        *msg->storage = (IPTR)data->acpiacad_Handle;
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

BOOL ACPIACAd__Hidd_Telemetry__GetEntryAttribs(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Telemetry_GetEntryAttribs *msg)
{
    struct HWACPIACAdData *data = OOP_INST_DATA(cl, o);
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    struct TagItem *tstate;
    struct TagItem *tag;
    LONG value;
    ULONG units;
    LONG minValue;
    LONG maxValue;
    BOOL readOnly;
    CONST_STRPTR entryId;

    if (msg->index >= data->acpiacad_TelemetryCount)
        return FALSE;

    entryId = acpiACAd_telemetryId;
    units = vHW_TelemetryUnit_Boolean;
    minValue = 0;
    maxValue = 1;
    value = (LONG)ACPIACAd_ReadPowerPresent(data);
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
