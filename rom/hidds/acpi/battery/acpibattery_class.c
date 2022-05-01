/*
    Copyright (C) 2022, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/utility.h>

#include <oop/oop.h>
#include <utility/tagitem.h>

#include <string.h>

#include "acpibattery_intern.h"

CONST_STRPTR    acpiBattery_str = "ACPI Generic Battery Device";

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

    HW_ACPIBattery_Switch(msg->attrID, idx)
    {
    case aoHW_ACPIBattery_Handle:
        *msg->storage = (IPTR)data->acpib_Handle;
        return;
    }

    Hidd_Power_Switch(msg->attrID, idx)
    {
    case aoHidd_Power_Type:
        *msg->storage = (IPTR)vHW_PowerType_Battery;
        return;

    case aoHidd_Power_State:
        *msg->storage = (IPTR)data->acpib_State;
        return;

    case aoHidd_Power_Flags:
        *msg->storage = (IPTR)data->acpib_Flags;
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}
