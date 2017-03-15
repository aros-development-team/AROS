/*
    Copyright (C) 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <proto/utility.h>

#include <oop/oop.h>
#include <utility/tagitem.h>

#include "acpibutton_intern.h"
 
void ACPIButton_ButtonNotifyHandler(ACPI_HANDLE acpiDevice, UINT32 acpiEvent, void *acpiContext)
{
    ACPI_DEVICE_INFO *acpiDevInfo = NULL;
    OOP_Object *o = (OOP_Object *)acpiContext;
    ACPI_STATUS acpiStatus;

    D(bug("[ACPI:Button] %s()\n", __func__));
    D(bug("[ACPI:Button] %s: Event %08x\n", __func__, acpiEvent));
    if (o)
    {
        D(bug("[ACPI:Button] %s: OOP_Object @ 0x%p\n", __func__, o));

        acpiStatus = AcpiGetObjectInfo(acpiDevice, &acpiDevInfo);
        if (acpiStatus != AE_OK) {
            if (acpiDevInfo) {
                FreeVec(acpiDevInfo);
            }
            return;
        }

        if (acpiDevInfo->Valid & ACPI_VALID_HID)
        {
            switch (acpiEvent)
            {
            default:
                D(bug("[ACPI:Button] %s: Unhandled event %08x\n", __func__, acpiEvent));
                break;
            }
        }
        FreeVec(acpiDevInfo);
    }
}

ACPI_STATUS ACPIButton_FixedButtonNotifyHandler(void *acpiContext)
{
    OOP_Object *o = (OOP_Object *)acpiContext;
    struct HIDDACPIButtonData *data;
    OOP_Class *cl;

    D(bug("[ACPI:Button] %s()\n", __func__));

    if (!acpiContext)
        return AE_BAD_PARAMETER;

    cl = OOP_OCLASS(o);
    data = OOP_INST_DATA(cl, o);

    ACPIButton_ButtonNotifyHandler(data->acpib_Handle, ACPI_EVENT_TYPE_FIXED, acpiContext);

    return AE_OK;
}


OOP_Object *ACPIButton__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct pRoot_New    acpiNewButtonMsg;
    ULONG               acpiButtonEvent = ACPI_EVENT_GLOBAL;
    struct Library      *UtilityBase = CSD(cl)->cs_UtilityBase;
    ULONG               buttonType = (ULONG)GetTagData(aHidd_ACPIButton_Type, 0, msg->attrList);
    OOP_Object          **buttonO = NULL;
    BOOL                buttonFixed = FALSE;
    ACPI_HANDLE         acpiHandle = (ACPI_HANDLE)GetTagData(aHidd_ACPIButton_Handle, 0, msg->attrList);

    D(bug("[ACPI:Button] %s()\n", __func__));

    switch (buttonType)
    {
        case vHidd_ACPIButton_PowerF:
            acpiButtonEvent = ACPI_EVENT_POWER_BUTTON;
            buttonFixed = TRUE;
        case vHidd_ACPIButton_Power:
            buttonO = &CSD(cl)->powerButtonObj;
            if (acpiButtonEvent != ACPI_EVENT_POWER_BUTTON) acpiButtonEvent = ACPI_DEVICE_NOTIFY;
            break;

        case vHidd_ACPIButton_SleepF:
            acpiButtonEvent = ACPI_EVENT_SLEEP_BUTTON;
            buttonFixed = TRUE;
        case vHidd_ACPIButton_Sleep:
            buttonO = &CSD(cl)->sleepButtonObj;
            if (acpiButtonEvent != ACPI_EVENT_SLEEP_BUTTON) acpiButtonEvent = ACPI_DEVICE_NOTIFY;
            break;

        case vHidd_ACPIButton_Lid:
            buttonO = &CSD(cl)->lidButtonObj;
            acpiButtonEvent = ACPI_DEVICE_NOTIFY;
            break;

        default:
            D(bug("[ACPI:Button] %s: Unhandled button type %d\n", __func__, buttonType));
            break;
    }

    if (buttonO)
    {
        if (!(*buttonO))
        {
            struct TagItem new_tags[] =
            {
                { aHidd_Name,               (IPTR)"ACPIButton"                      },
                { aHidd_HardwareName,       (IPTR)"ACPI Feature Button Driver"      },
                { TAG_DONE,                 0                                       }
            };
            acpiNewButtonMsg.mID      = msg->mID,
            acpiNewButtonMsg.attrList = new_tags;

            if (msg->attrList)
            {
                new_tags[2].ti_Tag  = TAG_MORE;
                new_tags[2].ti_Data = (IPTR)msg->attrList;
            }

            D(bug("[ACPI:Button] %s: ACPI Handle @ 0x%p\n", __func__, acpiHandle));

            if ((*buttonO = (OOP_Object *)OOP_DoSuperMethod(cl, o, &acpiNewButtonMsg.mID)) != NULL)
            {
                struct HIDDACPIButtonData *data = OOP_INST_DATA(cl, *buttonO);
                data->acpib_Type = buttonType;
                data->acpib_Handle = acpiHandle;

                D(bug("[ACPI:Button] %s: Object @ 0x%p\n", __func__, *buttonO));

                if (buttonFixed)
                    AcpiInstallFixedEventHandler(acpiButtonEvent, ACPIButton_FixedButtonNotifyHandler, *buttonO);
                else
                    AcpiInstallNotifyHandler(acpiHandle, acpiButtonEvent, ACPIButton_ButtonNotifyHandler, *buttonO);
            }
        }
        return *buttonO;
    }

    return NULL;
}

VOID ACPIButton__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug("[ACPI:Button] %s()\n", __func__));

    /* We are singletone. Cannot dispose. */
}
