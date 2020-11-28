/*
    Copyright (C) 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/utility.h>

#include <oop/oop.h>
#include <utility/tagitem.h>

#include <string.h>

#include "acpibutton_intern.h"

struct ACPIButton_ServiceTask
{
    struct Task *acpist_Task;
    BYTE        acpist_SigPower;
    BYTE        acpist_SigSleep;
    BYTE        acpist_SigLid;
};

struct ACPIButton_ServiceTask acpiButton_Service;

CONST_STRPTR    acpiPowerButton_str = "ACPI Power Button Device";
CONST_STRPTR    acpiFPowerButton_str = "ACPI Fixed Power Button Device";
CONST_STRPTR    acpiSleepButton_str = "ACPI Sleep Button Device";
CONST_STRPTR    acpiFSleepButton_str = "ACPI Fixed Sleep Button Device";
CONST_STRPTR    acpiLidButton_str = "ACPI Lid Device";

CONST_STRPTR    acpiButton_str = "ACPI Button Device";
CONST_STRPTR    acpiFButton_str = "ACPI Fixed Button Device";

void ACPIButtonServiceTask(struct ExecBase *SysBase)
{
    ULONG signals = 0;

    D(bug("[ACPI:Button] %s()\n", __func__));

    acpiButton_Service.acpist_SigPower = AllocSignal(-1);
    acpiButton_Service.acpist_SigSleep = AllocSignal(-1);
    acpiButton_Service.acpist_SigLid = AllocSignal(-1);
    
    while (signals = Wait((1 << acpiButton_Service.acpist_SigPower) | (1 << acpiButton_Service.acpist_SigSleep) | (1 << acpiButton_Service.acpist_SigLid)))
    {
        if (signals & (1 << acpiButton_Service.acpist_SigPower))
        {
            D(bug("[ACPI:Button] %s: Power Button Signal Received..\n", __func__));
            ShutdownA(SD_ACTION_POWEROFF);
        }
        else if (signals & (1 << acpiButton_Service.acpist_SigSleep))
        {
            D(bug("[ACPI:Button] %s: Sleep Button Signal Received..\n", __func__));
        }
        else if (signals & (1 << acpiButton_Service.acpist_SigLid))
        {
            D(bug("[ACPI:Button] %s: Lid Signal Received..\n", __func__));
        }
    }
}

void ACPIButton_ButtonNotifyHandler(ACPI_HANDLE acpiDevice, UINT32 acpiEvent, void *acpiContext)
{
    OOP_Object *o = (OOP_Object *)acpiContext;

    D(bug("[ACPI:Button] %s()\n", __func__));
    D(bug("[ACPI:Button] %s: Device 0x%p, Event %08x\n", __func__, acpiDevice, acpiEvent));

    if (o)
    {
        struct Hook *buttonHook = NULL;
        OOP_Class *cl;

        D(bug("[ACPI:Button] %s: OOP_Object @ 0x%p\n", __func__, o));

        cl = OOP_OCLASS(o);

        OOP_GetAttr(o, aHW_ACPIButton_Hook, (IPTR *)&buttonHook);

        if (buttonHook)
        {
            D(
                bug("[ACPI:Button] %s: Calling Object Hook @ 0x%p\n", __func__, buttonHook);
                bug("[ACPI:Button] %s:                      Func @ 0x%p\n", __func__, buttonHook->h_Entry);
            )
            buttonHook->h_Data = (void *)(IPTR)acpiEvent;
            CALLHOOKPKT(buttonHook, o, (void *)acpiDevice);
        }
    }
}

ACPI_STATUS ACPIButton_FixedButtonNotifyHandler(void *acpiContext)
{
    D(bug("[ACPI:Button] %s()\n", __func__));

    if (!acpiContext)
        return AE_BAD_PARAMETER;

    ACPIButton_ButtonNotifyHandler(NULL, ACPI_EVENT_TYPE_FIXED, acpiContext);

    return AE_OK;
}


OOP_Object *ACPIButton__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct pRoot_New    acpiNewButtonMsg;
    ULONG               acpiButtonEvent = ACPI_EVENT_GLOBAL;
    struct Library      *UtilityBase = CSD(cl)->cs_UtilityBase;
    ULONG               buttonType = (ULONG)GetTagData(aHW_ACPIButton_Type, 0, msg->attrList);
    struct Hook         *buttonHook = (struct Hook *)GetTagData(aHW_ACPIButton_Hook, 0, msg->attrList);
    ACPI_HANDLE         acpiHandle = (ACPI_HANDLE)GetTagData(aHW_ACPIButton_Handle, 0, msg->attrList);
    OOP_Object          **buttonO = NULL;
    BOOL                buttonFixed = FALSE;
    CONST_STRPTR        deviceName;
    __unused CONST_STRPTR        deviceClass;

    D(bug("[ACPI:Button] %s()\n", __func__));

    switch (buttonType)
    {
        case vHW_ACPIButton_PowerF:
            acpiButtonEvent = ACPI_EVENT_POWER_BUTTON;
            buttonFixed = TRUE;
            deviceName = acpiFPowerButton_str;
            deviceClass = acpiFButton_str;
        case vHW_ACPIButton_Power:
            buttonO = &CSD(cl)->powerButtonObj;
            if (acpiButtonEvent != ACPI_EVENT_POWER_BUTTON)
            {
                deviceName = acpiPowerButton_str;
                deviceClass = acpiButton_str;
                acpiButtonEvent = ACPI_DEVICE_NOTIFY;
            }
            break;

        case vHW_ACPIButton_SleepF:
            acpiButtonEvent = ACPI_EVENT_SLEEP_BUTTON;
            buttonFixed = TRUE;
            deviceName = acpiFSleepButton_str;
            deviceClass = acpiFButton_str;
        case vHW_ACPIButton_Sleep:
            buttonO = &CSD(cl)->sleepButtonObj;
            if (acpiButtonEvent != ACPI_EVENT_SLEEP_BUTTON)
            {
                deviceName = acpiSleepButton_str;
                deviceClass = acpiButton_str;
                acpiButtonEvent = ACPI_DEVICE_NOTIFY;
            }
            break;

        case vHW_ACPIButton_Lid:
            buttonO = &CSD(cl)->lidButtonObj;
            acpiButtonEvent = ACPI_DEVICE_NOTIFY;
            deviceName = acpiLidButton_str;
            deviceClass = acpiButton_str;
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
                { aHidd_Name,                   (IPTR)"acpibutton.hidd" },
                { aHidd_HardwareName,           (IPTR)deviceName       },
                { TAG_DONE,                     0                       }
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
                struct HWACPIButtonData *data = OOP_INST_DATA(cl, *buttonO);
                data->acpib_Type = buttonType;
                data->acpib_Handle = acpiHandle;
                data->acpib_Hook = buttonHook;

                D(bug("[ACPI:Button] %s: Object @ 0x%p\n", __func__, *buttonO));

                if (!acpiButton_Service.acpist_Task)
                {
                    acpiButton_Service.acpist_Task = NewCreateTask(TASKTAG_NAME       , "ACPI Button Service Task",
                        TASKTAG_PRI        , 127,
#if defined(__AROSEXEC_SMP__)
                        TASKTAG_AFFINITY, TASKAFFINITY_ANY,
#endif
                        TASKTAG_PC         , ACPIButtonServiceTask,
                        TASKTAG_ARG1       , SysBase,
                        TAG_DONE);
                }

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


VOID ACPIButton__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct HWACPIButtonData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    D(bug("[ACPI:Button] %s()\n", __func__));

    HW_ACPIButton_Switch(msg->attrID, idx)
    {
    case aoHW_ACPIButton_Type:
        *msg->storage = (IPTR)data->acpib_Type;
        return;

    case aoHW_ACPIButton_Handle:
        *msg->storage = (IPTR)data->acpib_Handle;
        return;

    case aoHW_ACPIButton_Hook:
        *msg->storage = (IPTR)data->acpib_Hook;
        return;

    case aoHW_ACPIButton_ServiceTask:
        *msg->storage = (IPTR)acpiButton_Service.acpist_Task;
        return;

    case aoHW_ACPIButton_ServiceSigPower:
        *msg->storage = (IPTR)acpiButton_Service.acpist_SigPower;
        return;

    case aoHW_ACPIButton_ServiceSigSleep:
        *msg->storage = (IPTR)acpiButton_Service.acpist_SigSleep;
        return;

    case aoHW_ACPIButton_ServiceSigLid:
        *msg->storage = (IPTR)acpiButton_Service.acpist_SigLid;
        return;

    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}
