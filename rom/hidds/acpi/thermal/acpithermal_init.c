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

#include "acpithermal_intern.h"

#include LC_LIBDEFS_FILE

#define DSCAN(x)

#undef _csd

static ACPI_STATUS ACPIThermal_DeviceQuery(ACPI_HANDLE handle,
                                UINT32 level,
                                void *context,
                                void **retval)
{
    struct acpithermalclass_staticdata    *_csd = (struct acpithermalclass_staticdata *)context;

    DSCAN(bug("[HWACPIThermal] %s(0x%p)\n", __func__, handle));

    if (handle)
    {
        struct ACPIThermalNode *newZone = AllocVec(sizeof(struct ACPIThermalNode), MEMF_CLEAR);
        if (!newZone)
            return AE_NO_MEMORY;

        newZone->atzn_Handle = handle;
        AddTail(&_csd->cs_Thermals, &newZone->atzn_Node);
    }

    return AE_OK;
}

static int ACPIThermal_Init(LIBBASETYPEPTR LIBBASE)
{
    struct acpithermalclass_staticdata    *_csd = &LIBBASE->hsi_csd;
    OOP_Object                  *root;
    int                         zoneCount = 0;
    int                         retVal = FALSE;
    __unused ACPI_STATUS        acpiStatus;

    D(bug("[HWACPIThermal] %s()\n", __func__));
    D(bug("[HWACPIThermal] %s: OOPBase @ 0x%p\n", __func__, OOPBase));

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

    NEWLIST(&_csd->cs_Thermals);

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
    _csd->hwACPIThermalAB = OOP_ObtainAttrBase(IID_HW_ACPIThermal);

    {
        struct TagItem instanceTags[] =
        {
            { _csd->hwACPIThermalAB + aoHW_ACPIThermal_Handle,    0},
            { TAG_DONE,                                       0}
        };

        acpiStatus = AcpiWalkNamespace(ACPI_TYPE_THERMAL, ACPI_ROOT_OBJECT, INT_MAX, ACPIThermal_DeviceQuery, NULL, _csd, NULL);
        if (acpiStatus == AE_OK)
        {
            struct ACPIThermalNode *newZone, *tmpNode;
            ForeachNodeSafe(&_csd->cs_Thermals, newZone, tmpNode)
            {
                instanceTags[0].ti_Data = (IPTR)newZone->atzn_Handle;
                if ((newZone->atzn_Object = HW_AddDriver(root, _csd->oopclass, instanceTags)))
                {
                    D(bug("[HWACPIThermal] %s: ACPIThermal instance @ 0x%pp\n", __func__, newZone->atzn_Object));
                    zoneCount++;
                }
                else
                {
                    /* TODO: Free the node */
                }
            }
        }
    }

    if (zoneCount > 0)
        retVal = TRUE;
    else
    {
        LIBBASE->hsi_LibNode.lib_OpenCnt -= 1;
        CloseLibrary(_csd->cs_UtilityBase);
        CloseLibrary(_csd->cs_ACPICABase);
    }

    D(bug("[HWACPIThermal] %s: Finished\n", __func__));

    return retVal;
}

static int ACPIThermal_Expunge(LIBBASETYPEPTR LIBBASE)
{
    D(struct acpithermalclass_staticdata *_csd = &LIBBASE->hsi_csd;)

    D(
        bug("[HWACPIThermal] %s()\n", __func__);
        bug("[HWACPIThermal] %s: csd @ %p\n", __func__, _csd);
    )

    return TRUE;
}

ADD2INITLIB(ACPIThermal_Init, -2)
ADD2EXPUNGELIB(ACPIThermal_Expunge, -2)
