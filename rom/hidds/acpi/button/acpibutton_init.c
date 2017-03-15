/*
    Copyright (C) 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <stddef.h>
#include <exec/types.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include <aros/symbolsets.h>
#include <hidd/hidd.h>
#include <hidd/system.h>

#include "acpibutton_intern.h"

#include LC_LIBDEFS_FILE

static ACPI_STATUS ACPIButton_DeviceQuery(ACPI_HANDLE handle,
				UINT32 level,
				void *context,
				void **retval)
{
    struct class_static_data    *csd = (struct class_static_data *)context;
    ACPI_DEVICE_INFO            *acpiDevInfo = NULL;
    ACPI_STATUS                 acpiStatus;

    D(bug("[HiddACPIButton] %s(0x%p)\n", __func__, handle));

    acpiStatus = AcpiGetObjectInfo(handle, &acpiDevInfo);
    if (acpiStatus != AE_OK) {
        if (acpiDevInfo) {
            FreeVec(acpiDevInfo);
        }
        return acpiStatus;
    }

    if (acpiDevInfo->Valid & ACPI_VALID_HID)
    {
        D(bug("[HiddACPIButton] %s: HardwareID = '%s'\n", __func__, acpiDevInfo->HardwareId.String));

        if (!strcmp(acpiDevInfo->HardwareId.String, "PNP0C0C"))
        {
            D(bug("[HiddACPIButton] %s: Power Button Device Found\n", __func__));
            csd->acpiPowerBHandle = handle;
            csd->acpiPowerBType = vHidd_ACPIButton_Power;
        }
        else if (!strcmp(acpiDevInfo->HardwareId.String, "ACPI_FPB"))
        {
            D(bug("[HiddACPIButton] %s: Fixed Power Button Device Found\n", __func__));
            csd->acpiPowerBHandle = handle;
            csd->acpiPowerBType = vHidd_ACPIButton_PowerF;
        }
        else if (!strcmp(acpiDevInfo->HardwareId.String, "PNP0C0E"))
        {
            D(bug("[HiddACPIButton] %s: Sleep Button Device Found\n", __func__));
            csd->acpiSleepBHandle = handle;
            csd->acpiSleepBType = vHidd_ACPIButton_Sleep;
        }
        else if (!strcmp(acpiDevInfo->HardwareId.String, "ACPI_FSB"))
        {
            D(bug("[HiddACPIButton] %s: Fixed Sleep Button Device Found\n", __func__));
            csd->acpiSleepBHandle = handle;
            csd->acpiSleepBType = vHidd_ACPIButton_SleepF;
        }
        else if (!strcmp(acpiDevInfo->HardwareId.String, "PNP0C0D"))
        {
            D(bug("[HiddACPIButton] %s: Lid Button Device Found\n", __func__));
            csd->acpibLidBHandle = handle;
        }
    }

    FreeVec(acpiDevInfo);

    return AE_OK;
}

static int ACPIButton_Init(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data    *csd = &LIBBASE->hsi_csd;
    struct Library              *OOPBase = csd->cs_OOPBase;
    OOP_Object                  *root;
    int                         buttonCount = 0;
    int                         retVal = FALSE;
    __unused ACPI_STATUS        acpiStatus;

    D(bug("[HiddACPIButton] %s()\n", __func__));

    csd->cs_ACPICABase = OpenLibrary("acpica.library", 0);
    if (!csd->cs_ACPICABase)
        return FALSE;

    csd->cs_UtilityBase = OpenLibrary("utility.library", 36);
    if (!csd->cs_UtilityBase)
    {
        CloseLibrary(csd->cs_ACPICABase);
        return FALSE;
    }

    root = OOP_NewObject(NULL, CLID_Hidd_System, NULL);
    if (!root)
        root = OOP_NewObject(NULL, CLID_HW_Root, NULL);

    csd->hwAB = OOP_ObtainAttrBase(IID_HW);
    csd->hiddAB = OOP_ObtainAttrBase(IID_Hidd);
    csd->hiddACPIButtonAB = OOP_ObtainAttrBase(IID_Hidd_ACPIButton);

    {
        struct TagItem instanceTags[] =
        {
            { csd->hiddACPIButtonAB + aoHidd_ACPIButton_Type,   0},
            { csd->hiddACPIButtonAB + aoHidd_ACPIButton_Handle, 0},
            { csd->hiddACPIButtonAB + aoHidd_ACPIButton_Hook,   0},
            { TAG_DONE,                                         0}
        };

        acpiStatus = AcpiGetDevices(NULL, ACPIButton_DeviceQuery, csd, NULL);
        if (acpiStatus == AE_OK)
        {
            if (csd->acpiPowerBHandle != NULL)
            {
                instanceTags[0].ti_Data = (IPTR)csd->acpiPowerBType;
                instanceTags[1].ti_Data = (IPTR)csd->acpiPowerBHandle;
                instanceTags[2].ti_Data = 0;

                if (HW_AddDriver(root, csd->oopclass, instanceTags))
                {
                    D(bug("[HiddACPIButton] %s: Power-Button initialised\n", __func__));
                    buttonCount++;
                }
            }

            if (csd->acpiSleepBHandle != NULL)
            {
                instanceTags[0].ti_Data = (IPTR)csd->acpiSleepBType;
                instanceTags[1].ti_Data = (IPTR)csd->acpiSleepBHandle;
                instanceTags[2].ti_Data = 0;

                if (HW_AddDriver(root, csd->oopclass, instanceTags))
                {
                    D(bug("[HiddACPIButton] %s: Sleep-Button initialised\n", __func__));
                    buttonCount++;
                }
            }

            if (csd->acpibLidBHandle != NULL)
            {
                instanceTags[0].ti_Data = vHidd_ACPIButton_Lid;
                instanceTags[1].ti_Data = (IPTR)csd->acpibLidBHandle;
                instanceTags[2].ti_Data = 0;

                if (HW_AddDriver(root, csd->oopclass, instanceTags))
                {
                    D(bug("[HiddACPIButton] %s: Lid-Button initialised\n", __func__));
                    buttonCount++;
                }
            }
        }
    }
    D(bug("[HiddACPIButton] %s: Finished\n", __func__));

    if (buttonCount > 0)
        retVal = TRUE;

    return retVal;
}

static int ACPIButton_Expunge(LIBBASETYPEPTR LIBBASE)
{
    D(struct class_static_data *csd = &LIBBASE->hsi_csd;)
#if (0)
    struct Library *OOPBase = csd->cs_OOPBase;
#endif
    D(bug("[HiddACPIButton] %s(csd=%p)\n", __func__, csd));
    
    return TRUE;
}

ADD2INITLIB(ACPIButton_Init, -2)
ADD2EXPUNGELIB(ACPIButton_Expunge, -2)
