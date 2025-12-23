/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/utility.h>

#include <oop/oop.h>
#include <utility/tagitem.h>

#include <string.h>

#include "acpifan_intern.h"

CONST_STRPTR    acpiFan_str = "ACPI Fan Device";

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

        OOP_SetAttrsTags(fanO,
            aHidd_Telemetry_Value, (IPTR)0,
            aHidd_Telemetry_Min, (IPTR)0,
            aHidd_Telemetry_Max, (IPTR)0,
            aHidd_Telemetry_Units, (IPTR)vHW_TelemetryUnit_RPM,
            TAG_DONE);
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
