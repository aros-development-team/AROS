/*
    Copyright (C) 2022, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/utility.h>

#include <oop/oop.h>
#include <utility/tagitem.h>

#include <string.h>

#include "acpiacad_intern.h"

CONST_STRPTR    acpiACAd_str = "ACPI AC Adapter Device";

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

    D(bug("[ACPI:ACAd] %s()\n", __func__));

    HW_ACPIACAd_Switch(msg->attrID, idx)
    {
    case aoHW_ACPIACAd_Handle:
        *msg->storage = (IPTR)data->acpiacad_Handle;
        return;
    }

    Hidd_Power_Switch(msg->attrID, idx)
    {
    case aoHidd_Power_Type:
        *msg->storage = (IPTR)vHW_PowerType_AC;
        return;

    case aoHidd_Power_State:
        *msg->storage = (IPTR)data->acpiacad_State;
        return;

    case aoHidd_Power_Flags:
        *msg->storage = (IPTR)data->acpiacad_Flags;
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}
