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

#include "acpipowermeter_intern.h"

#include LC_LIBDEFS_FILE

#define DSCAN(x)

static BOOL ACPIPowerMeter_MatchDeviceID(ACPI_DEVICE_INFO *acpiDevInfo, const char *deviceID)
{
    ACPI_PNP_DEVICE_ID_LIST *cIdList;
    ACPI_PNP_DEVICE_ID *hwId;
    int cmptid;

    hwId = AcpiGetInfoHardwareId(acpiDevInfo);
    if (hwId && !(strcmp(hwId->String, deviceID)))
        return TRUE;
    cIdList = AcpiGetInfoCompatIdList(acpiDevInfo);
    if (cIdList)
    {
        for (cmptid = 0; cmptid < cIdList->Count; cmptid++)
        {
            if (!(strcmp(cIdList->Ids[cmptid].String, deviceID)))
                return TRUE;
        }
    }
    return FALSE;
}

static BOOL ACPIPowerMeter_HasMethod(ACPI_HANDLE handle, CONST_STRPTR method)
{
    ACPI_HANDLE methodHandle = NULL;

    if (!handle)
        return FALSE;

    return ACPI_SUCCESS(AcpiGetHandle(handle, (ACPI_STRING)method, &methodHandle));
}

static BOOL ACPIPowerMeter_HasAnyMethod(ACPI_HANDLE handle, CONST_STRPTR *methods)
{
    ULONG i = 0;

    while (methods[i])
    {
        if (ACPIPowerMeter_HasMethod(handle, methods[i]))
            return TRUE;
        i++;
    }

    return FALSE;
}

static BOOL ACPIPowerMeter_DeviceHasSensors(ACPI_HANDLE handle)
{
    static CONST_STRPTR powerMethods[] = { "PWRS", "PWR", "POWR", NULL };
    static CONST_STRPTR energyMethods[] = { "ENRG", "ENGY", "ENER", NULL };
    static CONST_STRPTR voltageMethods[] = { "VOLT", "_VLT", NULL };
    static CONST_STRPTR temperatureMethods[] = { "TEMP", "_TMP", NULL };

    if (ACPIPowerMeter_HasMethod(handle, "_PMM"))
        return TRUE;

    if (ACPIPowerMeter_HasAnyMethod(handle, powerMethods))
        return TRUE;
    if (ACPIPowerMeter_HasAnyMethod(handle, energyMethods))
        return TRUE;
    if (ACPIPowerMeter_HasAnyMethod(handle, voltageMethods))
        return TRUE;
    if (ACPIPowerMeter_HasAnyMethod(handle, temperatureMethods))
        return TRUE;

    return FALSE;
}

#undef _csd

static ACPI_STATUS ACPIPowerMeter_DeviceQuery(ACPI_HANDLE handle,
                                UINT32 level,
                                void *context,
                                void **retval)
{
    struct acpipowermeterclass_staticdata    *_csd = (struct acpipowermeterclass_staticdata *)context;
    ACPI_DEVICE_INFO            *acpiDevInfo = NULL;
    ACPI_STATUS                 acpiStatus;
    BOOL                        matched = FALSE;

    DSCAN(bug("[HWACPIPowerMeter] %s(0x%p)\n", __func__, handle));

    acpiStatus = AcpiGetObjectInfo(handle, &acpiDevInfo);
    if (ACPI_FAILURE(acpiStatus))
    {
        if (acpiDevInfo)
        {
            FreeVec(acpiDevInfo);
        }
        return acpiStatus;
    }

    if (ACPIPowerMeter_MatchDeviceID(acpiDevInfo, "ACPI000D"))
        matched = TRUE;
    else if (ACPIPowerMeter_MatchDeviceID(acpiDevInfo, "PNP0C09"))
        matched = TRUE;

    if (matched && ACPIPowerMeter_DeviceHasSensors(handle))
    {
        struct ACPIPowerMeterNode *newDevice = AllocVec(sizeof(struct ACPIPowerMeterNode), MEMF_CLEAR);
        D(bug("[HWACPIPowerMeter] %s: ACPI Power Meter device found @ 0x%p\n", __func__, handle));
        if (!newDevice)
        {
            FreeVec(acpiDevInfo);
            return AE_NO_MEMORY;
        }
        newDevice->apmn_Handle = handle;
        AddTail(&_csd->cs_Devices, &newDevice->apmn_Node);
    }

    FreeVec(acpiDevInfo);

    return AE_OK;
}

static int ACPIPowerMeter_Init(LIBBASETYPEPTR LIBBASE)
{
    struct acpipowermeterclass_staticdata    *_csd = &LIBBASE->hsi_csd;
    OOP_Object                  *root;
    int                         deviceCount = 0;
    int                         retVal = FALSE;
    __unused ACPI_STATUS        acpiStatus;

    D(bug("[HWACPIPowerMeter] %s()\n", __func__));
    D(bug("[HWACPIPowerMeter] %s: OOPBase @ 0x%p\n", __func__, OOPBase));

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

    NEWLIST(&_csd->cs_Devices);

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

    struct OOP_ABDescr attrbases[] =
    {
        { (STRPTR) IID_HW,                  &_csd->hwAB },
        { (STRPTR) IID_Hidd,                &_csd->hiddAB },
        { (STRPTR) IID_Hidd_Telemetry,      &_csd->hiddTelemetryAB },
        { (STRPTR) IID_HW_ACPIPowerMeter,   &_csd->hwACPIPowerMeterAB },
        { NULL, NULL }
    };

    OOP_ObtainAttrBases(attrbases);

    {
        struct TagItem instanceTags[] =
        {
            { _csd->hwACPIPowerMeterAB + aoHW_ACPIPowerMeter_Handle,    0},
            { TAG_DONE,                                         0}
        };

        acpiStatus = AcpiWalkNamespace(ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT, INT_MAX,
            ACPIPowerMeter_DeviceQuery, NULL, _csd, NULL);
        if (acpiStatus == AE_OK)
        {
            struct ACPIPowerMeterNode *newDevice, *tmpNode;
            ForeachNodeSafe(&_csd->cs_Devices, newDevice, tmpNode)
            {
                instanceTags[0].ti_Data = (IPTR)newDevice->apmn_Handle;
                if ((newDevice->apmn_Object = HW_AddDriver(root, _csd->oopclass, instanceTags)))
                {
                    D(bug("[HWACPIPowerMeter] %s: ACPIPowerMeter instance @ 0x%pp\n", __func__, newDevice->apmn_Object));
                    deviceCount++;
                }
                else
                {
                    // TODO: Free the node
                }
            }
        }
    }

    if (deviceCount > 0)
        retVal = TRUE;
    else
    {
        LIBBASE->hsi_LibNode.lib_OpenCnt -= 1;
        OOP_ReleaseAttrBases(attrbases);
        CloseLibrary(_csd->cs_UtilityBase);
        CloseLibrary(_csd->cs_ACPICABase);
    }

    D(bug("[HWACPIPowerMeter] %s: Finished\n", __func__));

    return retVal;
}

static int ACPIPowerMeter_Expunge(LIBBASETYPEPTR LIBBASE)
{
    D(struct acpipowermeterclass_staticdata *_csd = &LIBBASE->hsi_csd;)

    D(
        bug("[HWACPIPowerMeter] %s()\n", __func__);
        bug("[HWACPIPowerMeter] %s: csd @ %p\n", __func__, _csd);
    )

    return TRUE;
}

ADD2INITLIB(ACPIPowerMeter_Init, -2)
ADD2EXPUNGELIB(ACPIPowerMeter_Expunge, -2)
