/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
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
#include <hidd/telemetry.h>

#include <limits.h>

#include "acpifan_intern.h"

#include LC_LIBDEFS_FILE

#define DSCAN(x)

BOOL ACPIFan_MatchDeviceID(ACPI_DEVICE_INFO *acpiDevInfo, char *deviceID)
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

static ACPI_STATUS ACPIFan_DeviceQuery(ACPI_HANDLE handle,
                                UINT32 level,
                                void *context,
                                void **retval)
{
    struct acpifanclass_staticdata    *_csd = (struct acpifanclass_staticdata *)context;
    ACPI_DEVICE_INFO            *acpiDevInfo = NULL;
    ACPI_STATUS                 acpiStatus;

    DSCAN(bug("[HWACPIFan] %s(0x%p)\n", __func__, handle));

    acpiStatus = AcpiGetObjectInfo(handle, &acpiDevInfo);
    if (ACPI_FAILURE(acpiStatus)) {
        if (acpiDevInfo) {
            FreeVec(acpiDevInfo);
        }
        return acpiStatus;
    }

    if (ACPIFan_MatchDeviceID(acpiDevInfo, "PNP0C0B"))
    {
        struct ACPIFanNode *newFan = AllocVec(sizeof(struct ACPIFanNode), MEMF_CLEAR);
        D(bug("[HWACPIFan] %s: Fan Device PNP0C0B Found @ 0x%p\n", __func__, handle));
        if (!newFan)
        {
            FreeVec(acpiDevInfo);
            return AE_NO_MEMORY;
        }
        newFan->afann_Handle = handle;
        AddTail(&_csd->cs_Fans, &newFan->afann_Node);
    }

    FreeVec(acpiDevInfo);

    return AE_OK;
}

static int ACPIFan_Init(LIBBASETYPEPTR LIBBASE)
{
    struct acpifanclass_staticdata    *_csd = &LIBBASE->hsi_csd;
    OOP_Object                  *root;
    int                         fanCount = 0;
    int                         retVal = FALSE;
    __unused ACPI_STATUS        acpiStatus;

    D(bug("[HWACPIFan] %s()\n", __func__));
    D(bug("[HWACPIFan] %s: OOPBase @ 0x%p\n", __func__, OOPBase));

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

    NEWLIST(&_csd->cs_Fans);

    root = OOP_NewObject(NULL, CLID_Hidd_System, NULL);
    if (!root)
        root = OOP_NewObject(NULL, CLID_HW_Root, NULL);
    if (!root)
    {
        LIBBASE->hsi_LibNode.lib_OpenCnt -= 1;
        CloseLibrary(_csd->cs_UtilityBase);
        CloseLibrary(_csd->cs_ACPICABase);
        return FALSE;
    }

    _csd->hwAB = OOP_ObtainAttrBase(IID_HW);
    _csd->hiddAB = OOP_ObtainAttrBase(IID_Hidd);
    _csd->hiddTelemetryAB = OOP_ObtainAttrBase(IID_Hidd_Telemetry);
    _csd->hwACPIFanAB = OOP_ObtainAttrBase(IID_HW_ACPIFan);

    {
        struct TagItem instanceTags[] =
        {
            { _csd->hwACPIFanAB + aoHW_ACPIFan_Handle,    0},
            { TAG_DONE,                                       0}
        };

        acpiStatus = AcpiWalkNamespace(ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT, INT_MAX, ACPIFan_DeviceQuery, NULL, _csd, NULL);
        if (acpiStatus == AE_OK)
        {
            struct ACPIFanNode *newFan, *tmpNode;
            ForeachNodeSafe(&_csd->cs_Fans, newFan, tmpNode)
            {
                instanceTags[0].ti_Data = (IPTR)newFan->afann_Handle;
                if ((newFan->afann_Object = HW_AddDriver(root, _csd->oopclass, instanceTags)))
                {
                    D(bug("[HWACPIFan] %s: ACPIFan instance @ 0x%pp\n", __func__, newFan->afann_Object));
                    fanCount++;
                }
                else
                {
                    /* TODO: Free the node */
                }
            }
        }
    }

    if (fanCount > 0)
        retVal = TRUE;
    else
    {
        LIBBASE->hsi_LibNode.lib_OpenCnt -= 1;
        CloseLibrary(_csd->cs_UtilityBase);
        CloseLibrary(_csd->cs_ACPICABase);
    }

    D(bug("[HWACPIFan] %s: Finished\n", __func__));

    return retVal;
}

static int ACPIFan_Expunge(LIBBASETYPEPTR LIBBASE)
{
    D(struct acpifanclass_staticdata *_csd = &LIBBASE->hsi_csd;)

    D(
        bug("[HWACPIFan] %s()\n", __func__);
        bug("[HWACPIFan] %s: csd @ %p\n", __func__, _csd);
    )

    return TRUE;
}

ADD2INITLIB(ACPIFan_Init, -2)
ADD2EXPUNGELIB(ACPIFan_Expunge, -2)
