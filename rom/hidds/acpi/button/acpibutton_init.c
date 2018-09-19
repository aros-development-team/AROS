/*
    Copyright (C) 2017-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <stddef.h>
#include <exec/types.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include <aros/symbolsets.h>
#include <hidd/hidd.h>
#include <hidd/system.h>

#include <limits.h>

#include "acpibutton_intern.h"

#include LC_LIBDEFS_FILE

AROS_UFH3(IPTR, ACPIButton_PowerEventHandle,
    AROS_UFHA(struct Hook *, hook, A0), 
    AROS_UFHA(APTR, object, A2), 
    AROS_UFHA(APTR, message, A1)
)
{
    AROS_USERFUNC_INIT

    struct Task *sigTask = NULL;
    OOP_Class   *cl;
    IPTR        powerSig = 0;

//    ACPI_HANDLE device = (ACPI_HANDLE)message;

    D(bug("[HWACPIButton] %s()\n", __func__));

    cl = OOP_OCLASS(object);
    OOP_GetAttr(object, aHW_ACPIButton_ServiceTask, (IPTR *)&sigTask);
    OOP_GetAttr(object, aHW_ACPIButton_ServiceSigPower, &powerSig);

    if (sigTask)
    {
        D(bug("[HWACPIButton] %s: Signalling service task to handle Power Button event..\n", __func__));
        Signal(sigTask, (1 << powerSig));
    }

    return 0;

    AROS_USERFUNC_EXIT
}

AROS_UFH3(IPTR, ACPIButton_SleepEventHandle,
    AROS_UFHA(struct Hook *, hook, A0), 
    AROS_UFHA(APTR, object, A2), 
    AROS_UFHA(APTR, message, A1)
)
{
    AROS_USERFUNC_INIT

//    ACPI_HANDLE device = (ACPI_HANDLE)message;

    D(
        bug("[HWACPIButton] %s()\n", __func__);
        bug("[HWACPIButton] %s: Unhandled\n", __func__);
    )

    return 0;

    AROS_USERFUNC_EXIT
}

AROS_UFH3(IPTR, ACPIButton_LidEventHandle,
    AROS_UFHA(struct Hook *, hook, A0), 
    AROS_UFHA(APTR, object, A2), 
    AROS_UFHA(APTR, message, A1)
)
{
    AROS_USERFUNC_INIT

//    ACPI_HANDLE device = (ACPI_HANDLE)message;

    D(
        bug("[HWACPIButton] %s()\n", __func__);
        bug("[HWACPIButton] %s: Unhandled\n", __func__);
    )

    return 0;

    AROS_USERFUNC_EXIT
}

BOOL ACPIButton_MatchDeviceID(ACPI_DEVICE_INFO *acpiDevInfo, char *deviceID)
{
    ACPI_PNP_DEVICE_ID_LIST *cIdList;
    ACPI_PNP_DEVICE_ID *hwId;
    int cmptid;

    hwId = AcpiGetInfoHardwareId(acpiDevInfo);
    if (hwId && !(strcmp(hwId->String, deviceID)))
        return TRUE;
    cIdList = AcpiGetInfoCompatIdList(acpiDevInfo);
    if (cIdList) {
        for (cmptid = 0; cmptid < cIdList->Count; cmptid++) {
            if (!(strcmp(cIdList->Ids[cmptid].String, deviceID)))
                return TRUE;
        }
    }
    return FALSE;
}

#undef _csd

static ACPI_STATUS ACPIButton_DeviceQuery(ACPI_HANDLE handle,
				UINT32 level,
				void *context,
				void **retval)
{
    struct acpibuttonclass_staticdata    *_csd = (struct acpibuttonclass_staticdata *)context;
    ACPI_DEVICE_INFO            *acpiDevInfo = NULL;
    ACPI_STATUS                 acpiStatus;

    D(bug("[HWACPIButton] %s(0x%p)\n", __func__, handle));

    acpiStatus = AcpiGetObjectInfo(handle, &acpiDevInfo);
    if (ACPI_FAILURE(acpiStatus)) {
        if (acpiDevInfo) {
            FreeVec(acpiDevInfo);
        }
        return acpiStatus;
    }

    if (ACPIButton_MatchDeviceID(acpiDevInfo, "PNP0C0C"))
    {
        D(bug("[HWACPIButton] %s: Power Button Device Found\n", __func__));
        if ((!_csd->powerButtonObj) && (!_csd->acpiPowerBHandle))
        {
            _csd->acpiPowerBHandle = handle;
            _csd->acpiPowerBType = vHW_ACPIButton_Power;
        }
    }
    else if (ACPIButton_MatchDeviceID(acpiDevInfo, "ACPI_FPB"))
    {
        D(bug("[HWACPIButton] %s: Fixed Power Button Device Found\n", __func__));
        if ((!_csd->powerButtonObj) && (!_csd->acpiPowerBHandle))
        {
            _csd->acpiPowerBHandle = handle;
            _csd->acpiPowerBType = vHW_ACPIButton_PowerF;
        }
    }
    else if (ACPIButton_MatchDeviceID(acpiDevInfo, "PNP0C0E"))
    {
        D(bug("[HWACPIButton] %s: Sleep Button Device Found\n", __func__));
        if ((!_csd->sleepButtonObj) && (!_csd->acpiSleepBHandle))
        {
            _csd->acpiSleepBHandle = handle;
            _csd->acpiSleepBType = vHW_ACPIButton_Sleep;
        }
    }
    else if (ACPIButton_MatchDeviceID(acpiDevInfo, "ACPI_FSB"))
    {
        D(bug("[HWACPIButton] %s: Fixed Sleep Button Device Found\n", __func__));
        if ((!_csd->sleepButtonObj) && (!_csd->acpiSleepBHandle))
        {
            _csd->acpiSleepBHandle = handle;
            _csd->acpiSleepBType = vHW_ACPIButton_SleepF;
        }
    }
    else if (ACPIButton_MatchDeviceID(acpiDevInfo, "PNP0C0D"))
    {
        D(bug("[HWACPIButton] %s: Lid Button Device Found\n", __func__));
        if ((!_csd->lidButtonObj) && (!_csd->acpibLidBHandle))
        {
            _csd->acpibLidBHandle = handle;
        }
    }

    FreeVec(acpiDevInfo);

    return AE_OK;
}

static int ACPIButton_Init(LIBBASETYPEPTR LIBBASE)
{
    struct acpibuttonclass_staticdata    *_csd = &LIBBASE->hsi_csd;
    OOP_Object                  *root;
    int                         buttonCount = 0;
    int                         retVal = FALSE;
    __unused ACPI_STATUS        acpiStatus;

    D(bug("[HWACPIButton] %s()\n", __func__));
    D(bug("[HWACPIButton] %s: OOPBase @ 0x%p\n", __func__, OOPBase));

    _csd->cs_ACPICABase = OpenLibrary("acpica.library", 0);
    if (!_csd->cs_ACPICABase)
        return FALSE;

    _csd->cs_UtilityBase = OpenLibrary("utility.library", 36);
    if (!_csd->cs_UtilityBase)
    {
        CloseLibrary(_csd->cs_ACPICABase);
        return FALSE;
    }

    root = OOP_NewObject(NULL, CLID_Hidd_System, NULL);
    if (!root)
        root = OOP_NewObject(NULL, CLID_HW_Root, NULL);

    _csd->hwAB = OOP_ObtainAttrBase(IID_HW);
    _csd->hiddAB = OOP_ObtainAttrBase(IID_Hidd);
    _csd->hwACPIButtonAB = OOP_ObtainAttrBase(IID_HW_ACPIButton);

    {
        struct TagItem instanceTags[] =
        {
            { _csd->hwACPIButtonAB + aoHW_ACPIButton_Type,      0},
            { _csd->hwACPIButtonAB + aoHW_ACPIButton_Handle,    0},
            { _csd->hwACPIButtonAB + aoHW_ACPIButton_Hook,      0},
            { TAG_DONE,                                         0}
        };
        ACPI_TABLE_FADT *fadt;
        struct Hook *buttonHook;

        /* check for fixed feature buttons .. */
        if (AcpiGetTable(ACPI_SIG_FADT, 1, (ACPI_TABLE_HEADER **)&fadt) == AE_OK)
        {
            if ((!(fadt->Flags & ACPI_FADT_POWER_BUTTON)) && (!_csd->powerButtonObj))
            {
                D(bug("[HWACPIButton] %s: Fixed Power-Button Enabled in FADT\n", __func__));
                AcpiEnableEvent(ACPI_EVENT_POWER_BUTTON, 0);
                AcpiClearEvent(ACPI_EVENT_POWER_BUTTON);
                _csd->acpiPowerBType = vHW_ACPIButton_PowerF;
                instanceTags[0].ti_Data = (IPTR)_csd->acpiPowerBType;
                instanceTags[1].ti_Data = 0;

                buttonHook = AllocMem(sizeof(struct Hook), MEMF_CLEAR);
                buttonHook->h_Entry = (HOOKFUNC)ACPIButton_PowerEventHandle;
                buttonHook->h_Data = 0;
                instanceTags[2].ti_Data = (IPTR)buttonHook;

                if (HW_AddDriver(root, _csd->oopclass, instanceTags))
                {
                    D(bug("[HWACPIButton] %s: Fixed Power-Button initialised\n", __func__));
                    buttonCount++;
                }
                else
                    FreeMem(buttonHook, sizeof(struct Hook));
            }
            if ((!(fadt->Flags & ACPI_FADT_SLEEP_BUTTON)) && (!_csd->sleepButtonObj))
            {
                D(bug("[HWACPIButton] %s: Fixed Sleep-Button Enabled in FADT\n", __func__));
                AcpiEnableEvent(ACPI_EVENT_SLEEP_BUTTON, 0);
                AcpiClearEvent(ACPI_EVENT_SLEEP_BUTTON);
                _csd->acpiSleepBType = vHW_ACPIButton_SleepF;
                instanceTags[0].ti_Data = (IPTR)_csd->acpiSleepBType;
                instanceTags[1].ti_Data = 0;

                buttonHook = AllocMem(sizeof(struct Hook), MEMF_CLEAR);
                buttonHook->h_Entry = (HOOKFUNC)ACPIButton_SleepEventHandle;
                buttonHook->h_Data = 0;
                instanceTags[2].ti_Data = (IPTR)buttonHook;

                if (HW_AddDriver(root, _csd->oopclass, instanceTags))
                {
                    D(bug("[HWACPIButton] %s: Fixed Sleep-Button initialised\n", __func__));
                    buttonCount++;
                }
                else
                    FreeMem(buttonHook, sizeof(struct Hook));
            }
        }

        /* check for button devices .. */
        acpiStatus = AcpiWalkNamespace(ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT, INT_MAX, ACPIButton_DeviceQuery, NULL, _csd, NULL);
        if (acpiStatus == AE_OK)
        {
            if ((_csd->acpiPowerBHandle != NULL) && (!_csd->powerButtonObj))
            {
                instanceTags[0].ti_Data = (IPTR)_csd->acpiPowerBType;
                instanceTags[1].ti_Data = (IPTR)_csd->acpiPowerBHandle;

                buttonHook = AllocMem(sizeof(struct Hook), MEMF_CLEAR);
                buttonHook->h_Entry = (HOOKFUNC)ACPIButton_PowerEventHandle;
                buttonHook->h_Data = 0;
                instanceTags[2].ti_Data = (IPTR)buttonHook;

                if (HW_AddDriver(root, _csd->oopclass, instanceTags))
                {
                    D(bug("[HWACPIButton] %s: Power-Button initialised\n", __func__));
                    buttonCount++;
                }
                else
                    FreeMem(buttonHook, sizeof(struct Hook));
            }

            if ((_csd->acpiSleepBHandle != NULL) && (!_csd->sleepButtonObj))
            {
                instanceTags[0].ti_Data = (IPTR)_csd->acpiSleepBType;
                instanceTags[1].ti_Data = (IPTR)_csd->acpiSleepBHandle;

                buttonHook = AllocMem(sizeof(struct Hook), MEMF_CLEAR);
                buttonHook->h_Entry = (HOOKFUNC)ACPIButton_SleepEventHandle;
                buttonHook->h_Data = 0;
                instanceTags[2].ti_Data = (IPTR)buttonHook;

                if (HW_AddDriver(root, _csd->oopclass, instanceTags))
                {
                    D(bug("[HWACPIButton] %s: Sleep-Button initialised\n", __func__));
                    buttonCount++;
                }
                else
                    FreeMem(buttonHook, sizeof(struct Hook));
            }

            if ((_csd->acpibLidBHandle != NULL) && (!_csd->lidButtonObj))
            {
                instanceTags[0].ti_Data = vHW_ACPIButton_Lid;
                instanceTags[1].ti_Data = (IPTR)_csd->acpibLidBHandle;

                buttonHook = AllocMem(sizeof(struct Hook), MEMF_CLEAR);
                buttonHook->h_Entry = (HOOKFUNC)ACPIButton_LidEventHandle;
                buttonHook->h_Data = 0;
                instanceTags[2].ti_Data = (IPTR)buttonHook;

                if (HW_AddDriver(root, _csd->oopclass, instanceTags))
                {
                    D(bug("[HWACPIButton] %s: Lid-Button initialised\n", __func__));
                    buttonCount++;
                }
                else
                    FreeMem(buttonHook, sizeof(struct Hook));
            }
        }
    }

    if (buttonCount > 0)
        retVal = TRUE;
    else
    {
        CloseLibrary(_csd->cs_UtilityBase);
        CloseLibrary(_csd->cs_ACPICABase);
    }

    D(bug("[HWACPIButton] %s: Finished\n", __func__));

    return retVal;
}

static int ACPIButton_Expunge(LIBBASETYPEPTR LIBBASE)
{
    D(struct acpibuttonclass_staticdata *_csd = &LIBBASE->hsi_csd;)

    D(
        bug("[HWACPIButton] %s()\n", __func__);
        bug("[HWACPIButton] %s: csd @ %p\n", __func__, _csd);
    )

    return TRUE;
}

ADD2INITLIB(ACPIButton_Init, -2)
ADD2EXPUNGELIB(ACPIButton_Expunge, -2)
