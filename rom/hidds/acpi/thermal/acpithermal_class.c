/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>

#include <oop/oop.h>
#include <utility/tagitem.h>

#include <string.h>

#include "acpithermal_intern.h"

CONST_STRPTR    acpiThermal_str = "ACPI Thermal Zone";
/* ACPI temperatures are tenths of a Kelvin; the class wants thousandths of a degree Celsius */
static LONG ACPIThermal_ConvertToCelsius(ULONG temperature)
{
    return ((LONG)temperature - 2732) * 100;
}

/* The control method of a temperature point */
static BOOL ACPIThermal_PointMethod(ULONG point, char *method)
{
    switch (point)
    {
    case vHidd_Thermal_Current:  CopyMem("_TMP", method, 5); return TRUE;
    case vHidd_Thermal_Passive:  CopyMem("_PSV", method, 5); return TRUE;
    case vHidd_Thermal_Hot:      CopyMem("_HOT", method, 5); return TRUE;
    case vHidd_Thermal_Critical: CopyMem("_CRT", method, 5); return TRUE;
    }
    if (point >= vHidd_Thermal_Active0 && point <= vHidd_Thermal_Active9)
    {
        CopyMem("_AC0", method, 5);
        method[3] = '0' + (point - vHidd_Thermal_Active0);
        return TRUE;
    }
    return FALSE;
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
    ULONG points = 0;
    ULONG point;

    D(bug("[ACPI:Thermal] %s()\n", __func__));

    char devicePath[5 + 128];
    struct TagItem new_tags[] =
    {
        { aHidd_Name,                   (IPTR)"acpithermal.hidd" },
        { aHidd_HardwareName,           (IPTR)acpiThermal_str       },
        { TAG_IGNORE,                   0                       },
        { aHidd_Thermal_Points,         0                       },
        { TAG_DONE,                     0                       }
    };
    acpiNewThermalMsg.mID      = msg->mID,
    acpiNewThermalMsg.attrList = new_tags;

    if (msg->attrList)
    {
        new_tags[4].ti_Tag  = TAG_MORE;
        new_tags[4].ti_Data = (IPTR)msg->attrList;
    }

    /* The trip points the zone defines */
    for (point = vHidd_Thermal_Passive; point < vHidd_Thermal_PointCount; point++)
    {
        char method[5];

        if (ACPIThermal_PointMethod(point, method) && ACPIThermal_HasMethod(acpiHandle, method))
            points |= vHidd_ThermalF(point);
    }
    new_tags[3].ti_Data = points;

    /* The namespace path tells other components which device this is */
    if (acpiHandle)
    {
        ACPI_BUFFER pathBuffer = { sizeof(devicePath) - 5, &devicePath[5] };

        if (ACPI_SUCCESS(AcpiGetName(acpiHandle, ACPI_FULL_PATHNAME, &pathBuffer)))
        {
            CopyMem("acpi/", devicePath, 5);
            new_tags[2].ti_Tag  = aHidd_DevicePath;
            new_tags[2].ti_Data = (IPTR)devicePath;
        }
    }

    D(bug("[ACPI:Thermal] %s: ACPI Handle @ 0x%p\n", __func__, acpiHandle));

    if ((thermalO = (OOP_Object *)OOP_DoSuperMethod(cl, o, &acpiNewThermalMsg.mID)) != NULL)
    {
        struct HWACPIThermalData *data = OOP_INST_DATA(cl, thermalO);

        D(bug("[ACPI:Thermal] %s: Object @ 0x%p\n", __func__, thermalO));

        data->acpithermal_Handle = acpiHandle;
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

    HW_ACPIThermal_Switch(msg->attrID, idx)
    {
    case aoHW_ACPIThermal_Handle:
        *msg->storage = (IPTR)data->acpithermal_Handle;
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

BOOL ACPIThermal__Hidd_Thermal__GetTemperature(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Thermal_GetTemperature *msg)
{
    struct HWACPIThermalData *data = OOP_INST_DATA(cl, o);
    char method[5];

    if (!ACPIThermal_PointMethod(msg->point, method))
        return FALSE;
    return ACPIThermal_ReadTemperature(data, method, msg->temperature);
}
