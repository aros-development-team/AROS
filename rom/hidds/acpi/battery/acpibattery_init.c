/*
    Copyright (C) 2022, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <stddef.h>
#include <string.h>

#include <exec/types.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include <aros/symbolsets.h>
#include <hidd/hidd.h>
#include <hidd/system.h>

#include <limits.h>

#include "acpibattery_intern.h"

#include LC_LIBDEFS_FILE

#define DSCAN(x)

BOOL ACPIBattery_MatchDeviceID(ACPI_DEVICE_INFO *acpiDevInfo, char *deviceID)
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

static ACPI_STATUS ACPIBattery_DeviceQuery(ACPI_HANDLE handle,
                                UINT32 level,
                                void *context,
                                void **retval)
{
    struct acpibatteryclass_staticdata    *_csd = (struct acpibatteryclass_staticdata *)context;
    ACPI_DEVICE_INFO            *acpiDevInfo = NULL;
    ACPI_STATUS                 acpiStatus;

    DSCAN(bug("[HWACPIBattery] %s(0x%p)\n", __func__, handle));

    acpiStatus = AcpiGetObjectInfo(handle, &acpiDevInfo);
    if (ACPI_FAILURE(acpiStatus)) {
        if (acpiDevInfo) {
            FreeVec(acpiDevInfo);
        }
        return acpiStatus;
    }

    if (ACPIBattery_MatchDeviceID(acpiDevInfo, "PNP0C0A"))
    {
        struct ACPIBatNode *newBatt = AllocVec(sizeof(struct ACPIBatNode), MEMF_CLEAR);
        D(bug("[HWACPIBattery] %s: Battery Device PNP0C0A Found @ 0x%p\n", __func__, handle));
        newBatt->abn_Handle = handle;
        AddTail(&_csd->cs_Batteries, &newBatt->abn_Node);
    }

    FreeVec(acpiDevInfo);

    return AE_OK;
}

static int ACPIBattery_Init(LIBBASETYPEPTR LIBBASE)
{
    struct acpibatteryclass_staticdata    *_csd = &LIBBASE->hsi_csd;
    OOP_Object                  *root;
    int                         batteryCount = 0;
    int                         retVal = FALSE;
    __unused ACPI_STATUS        acpiStatus;

    D(bug("[HWACPIBattery] %s()\n", __func__));
    D(bug("[HWACPIBattery] %s: OOPBase @ 0x%p\n", __func__, OOPBase));

    _csd->cs_ACPICABase = OpenLibrary("acpica.library", 0);
    if (!_csd->cs_ACPICABase)
        return FALSE;

    _csd->cs_UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!_csd->cs_UtilityBase)
    {
        CloseLibrary(_csd->cs_ACPICABase);
        return FALSE;
    }

    LIBBASE->hsi_LibNode.lib_OpenCnt += 1;

    NEWLIST(&_csd->cs_Batteries);

    root = OOP_NewObject(NULL, CLID_Hidd_System, NULL);
    if (!root)
        root = OOP_NewObject(NULL, CLID_HW_Root, NULL);

    _csd->hwAB = OOP_ObtainAttrBase(IID_HW);
    _csd->hiddAB = OOP_ObtainAttrBase(IID_Hidd);
    _csd->hiddPowerAB = OOP_ObtainAttrBase(IID_Hidd_Power);
    _csd->hwACPIBatteryAB = OOP_ObtainAttrBase(IID_HW_ACPIBattery);

    {
        struct TagItem instanceTags[] =
        {
            { _csd->hwACPIBatteryAB + aoHW_ACPIBattery_Handle,    0},
            { TAG_DONE,                                         0}
        };
        ACPI_TABLE_FADT *fadt;

        /* check for battery devices .. */
        acpiStatus = AcpiWalkNamespace(ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT, INT_MAX, ACPIBattery_DeviceQuery, NULL, _csd, NULL);
        if (acpiStatus == AE_OK)
        {
            struct ACPIBatNode *newBatt, *tmpNode;
            ForeachNodeSafe(&_csd->cs_Batteries, newBatt, tmpNode)
            {
                instanceTags[0].ti_Data = (IPTR)newBatt->abn_Handle;
                if ((newBatt->abn_Object = HW_AddDriver(root, _csd->oopclass, instanceTags)))
                {
                    D(bug("[HWACPIBattery] %s: ACPIBattery instance @ 0x%pp\n", __func__, newBatt->abn_Object));
                    batteryCount++;
                }
                else
                {
                    // TODO: Free the node
                }
            }
        }
    }

    if (batteryCount > 0)
        retVal = TRUE;
    else
    {
        LIBBASE->hsi_LibNode.lib_OpenCnt -= 1;
        CloseLibrary(_csd->cs_UtilityBase);
        CloseLibrary(_csd->cs_ACPICABase);
    }

    D(bug("[HWACPIBattery] %s: Finished\n", __func__));

    return retVal;
}

static int ACPIBattery_Expunge(LIBBASETYPEPTR LIBBASE)
{
    D(struct acpibatteryclass_staticdata *_csd = &LIBBASE->hsi_csd;)

    D(
        bug("[HWACPIBattery] %s()\n", __func__);
        bug("[HWACPIBattery] %s: csd @ %p\n", __func__, _csd);
    )

    return TRUE;
}

ADD2INITLIB(ACPIBattery_Init, -2)
ADD2EXPUNGELIB(ACPIBattery_Expunge, -2)
