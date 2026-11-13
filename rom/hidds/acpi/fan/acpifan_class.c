/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>

#include <oop/oop.h>
#include <utility/tagitem.h>

#include <string.h>

#include "acpifan_intern.h"

CONST_STRPTR    acpiFan_str = "ACPI Fan Device";

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
    struct HWACPIFanData probe = { acpiHandle };
    ULONG values = vHidd_FanF(vHidd_Fan_Speed);

    D(bug("[ACPI:Fan] %s()\n", __func__));

    /* What _FST reports decides which values the fan has */
    ACPIFan_ReadStatusPackage(&probe, NULL, NULL, NULL, &probe.acpifan_HasStatus, &probe.acpifan_HasControl);
    if (probe.acpifan_HasStatus)
        values |= vHidd_FanF(vHidd_Fan_Running);
    if (probe.acpifan_HasControl)
        values |= vHidd_FanF(vHidd_Fan_Level);

    char devicePath[5 + 128];
    struct TagItem new_tags[] =
    {
        { aHidd_Name,                   (IPTR)"acpifan.hidd" },
        { aHidd_HardwareName,           (IPTR)acpiFan_str       },
        { TAG_IGNORE,                   0                       },
        { aHidd_Fan_Values,             values                  },
        { TAG_DONE,                     0                       }
    };
    acpiNewFanMsg.mID      = msg->mID,
    acpiNewFanMsg.attrList = new_tags;

    if (msg->attrList)
    {
        new_tags[4].ti_Tag  = TAG_MORE;
        new_tags[4].ti_Data = (IPTR)msg->attrList;
    }

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

    D(bug("[ACPI:Fan] %s: ACPI Handle @ 0x%p\n", __func__, acpiHandle));

    if ((fanO = (OOP_Object *)OOP_DoSuperMethod(cl, o, &acpiNewFanMsg.mID)) != NULL)
    {
        struct HWACPIFanData *data = OOP_INST_DATA(cl, fanO);

        D(bug("[ACPI:Fan] %s: Object @ 0x%p\n", __func__, fanO));

        data->acpifan_Handle = acpiHandle;
        data->acpifan_FpsMin = 0;
        data->acpifan_FpsMax = 0;
        data->acpifan_HasFps = FALSE;
        data->acpifan_HasStatus = probe.acpifan_HasStatus;
        data->acpifan_HasControl = probe.acpifan_HasControl;

        ACPIFan_ReadPerformanceStates(data);
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

    HW_ACPIFan_Switch(msg->attrID, idx)
    {
    case aoHW_ACPIFan_Handle:
        *msg->storage = (IPTR)data->acpifan_Handle;
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

BOOL ACPIFan__Hidd_Fan__GetValue(OOP_Class *cl, OOP_Object *o, struct pHidd_Fan_GetValue *msg)
{
    struct HWACPIFanData *data = OOP_INST_DATA(cl, o);
    ULONG statusValue = 0, controlValue = 0, speedValue = 0;
    BOOL hasStatus = FALSE, hasControl = FALSE;

    ACPIFan_ReadStatusPackage(data, &statusValue, &controlValue, &speedValue, &hasStatus, &hasControl);

    *msg->min = 0;
    *msg->max = 0;
    switch (msg->which)
    {
    case vHidd_Fan_Speed:
        /* _FPS lists the speeds the fan can be set to */
        if (data->acpifan_HasFps)
        {
            *msg->min = (LONG)data->acpifan_FpsMin;
            *msg->max = (LONG)data->acpifan_FpsMax;
        }
        *msg->value = (LONG)speedValue;
        return TRUE;
    case vHidd_Fan_Running:
        if (!hasStatus)
            return FALSE;
        *msg->max = 1;
        *msg->value = statusValue ? 1 : 0;
        return TRUE;
    case vHidd_Fan_Level:
        if (!hasControl)
            return FALSE;
        *msg->value = (LONG)controlValue;
        return TRUE;
    }
    return FALSE;
}
