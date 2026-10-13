#define __OOP_NOMETHODBASES__
#define __OOP_NOATTRBASES__

#include <aros/asmcall.h>

#include <hidd/hidd.h>
#include <hidd/storage.h>

#include <oop/oop.h>

#include <proto/oop.h>

#include <utility/hooks.h>

#include <stdio.h>

#include "storage.h"

OOP_AttrBase HiddStorageUnitAB;

OOP_MethodID HWBase;
OOP_MethodID HiddStorageControllerBase;
OOP_MethodID HiddStorageBusBase;
OOP_MethodID HiddStorageUnitBase;

static const struct OOP_ABDescr storage_abd[] =
{
    { IID_Hidd_StorageUnit, &HiddStorageUnitAB },
    { NULL, NULL }
};

struct StorageEnumData
{
    ULONG count;
};

static const char *StorageTypeName(IPTR type)
{
    switch (type)
    {
        case vHidd_StorageUnit_Type_RaidArray:
            return "Raid Array";

        case vHidd_StorageUnit_Type_FixedDisk:
            return "Fixed Disk";

        case vHidd_StorageUnit_Type_SolidStateDisk:
            return "Solid State Disk";

        case vHidd_StorageUnit_Type_OpticalDisc:
            return "Optical Disc";

        case vHidd_StorageUnit_Type_MagneticTape:
            return "Magnetic Tape";

        case vHidd_StorageUnit_Type_Unknown:
        default:
            return "Unknown";
    }
}

static void PrintStorageSize(UQUAD bytes)
{
    UQUAD divisor = 1;
    UQUAD whole;
    UQUAD tenth;
    const char *unit = "B";

    if (bytes >= ((UQUAD)1 << 40))
    {
        divisor = (UQUAD)1 << 40;
        unit = "TiB";
    }
    else if (bytes >= ((UQUAD)1 << 30))
    {
        divisor = (UQUAD)1 << 30;
        unit = "GiB";
    }
    else if (bytes >= ((UQUAD)1 << 20))
    {
        divisor = (UQUAD)1 << 20;
        unit = "MiB";
    }
    else if (bytes >= ((UQUAD)1 << 10))
    {
        divisor = (UQUAD)1 << 10;
        unit = "KiB";
    }

    if (divisor == 1)
    {
        printf("%llu %s", (unsigned long long)bytes, unit);
        return;
    }

    whole = bytes / divisor;
    tenth = ((bytes % divisor) * 10) / divisor;

    printf("%llu.%llu %s",
        (unsigned long long)whole,
        (unsigned long long)tenth,
        unit);
}

AROS_UFH3S(BOOL, StorageUnitEnum,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(OOP_Object *, unitObj, A2),
    AROS_UFHA(struct StorageEnumData *, data, A1))
{
    AROS_USERFUNC_INIT

    IPTR type = vHidd_StorageUnit_Type_Unknown;
    IPTR device = 0;
    IPTR unit = 0;
    IPTR model = 0;
    IPTR revision = 0;
    IPTR removable = FALSE;
    UQUAD blockCount = 0;
    ULONG blockSize = 0;

    (void)hook;

    OOP_GetAttr(unitObj, aHidd_StorageUnit_Type, &type);
    OOP_GetAttr(unitObj, aHidd_StorageUnit_Device, &device);
    OOP_GetAttr(unitObj, aHidd_StorageUnit_Number, &unit);
    OOP_GetAttr(unitObj, aHidd_StorageUnit_Model, &model);
    OOP_GetAttr(unitObj, aHidd_StorageUnit_Revision, &revision);
    OOP_GetAttr(unitObj, aHidd_StorageUnit_Removable, &removable);

    data->count++;

    if (data->count > 1)
        printf("\n");

    printf("  Storage %-9u%s\n",
        (unsigned)data->count,
        StorageTypeName(type));

    printf("    Device         %s/%lu\n",
        device ? (const char *)device : "Unknown",
        (unsigned long)unit);

    if (model && *((const char *)model))
        printf("    Model          %s\n", (const char *)model);

    if (revision && *((const char *)revision))
        printf("    Revision       %s\n", (const char *)revision);

    if (HIDD_StorageUnit_GetCapacity(unitObj, &blockCount, &blockSize) &&
        blockCount && blockSize)
    {
        printf("    Capacity       ");

        if (blockCount <= (~(UQUAD)0) / blockSize)
            PrintStorageSize(blockCount * (UQUAD)blockSize);
        else
            printf("Unknown");

        printf("\n    Block size     ");
        PrintStorageSize((UQUAD)blockSize);
        printf("\n");
    }

    printf("    Removable      %s\n",
        removable ? "Yes" : "No");

    return FALSE;

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(BOOL, StorageBusEnum,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(OOP_Object *, busObj, A2),
    AROS_UFHA(struct StorageEnumData *, data, A1))
{
    AROS_USERFUNC_INIT

    struct Hook unitenum_hook =
    {
        .h_Entry = StorageUnitEnum,
        .h_Data = NULL
    };

    (void)hook;

    HIDD_StorageBus_EnumUnits(busObj, &unitenum_hook, data);

    return FALSE;

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(BOOL, StorageControllerEnum,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(OOP_Object *, controllerObj, A2),
    AROS_UFHA(struct StorageEnumData *, data, A1))
{
    AROS_USERFUNC_INIT

    struct Hook busenum_hook =
    {
        .h_Entry = StorageBusEnum,
        .h_Data = NULL
    };

    (void)hook;

    HIDD_StorageController_EnumBuses(
        controllerObj,
        &busenum_hook,
        data
    );

    return FALSE;

    AROS_USERFUNC_EXIT
}

void PrintStorageInformation(void)
{
    OOP_Object *storageRoot;
    struct StorageEnumData data =
    {
        .count = 0
    };
    struct Hook controllerenum_hook =
    {
        .h_Entry = StorageControllerEnum,
        .h_Data = NULL
    };

    if (!OOP_ObtainAttrBases(storage_abd))
        return;

    HWBase = OOP_GetMethodID(IID_HW, 0);
    HiddStorageControllerBase =
        OOP_GetMethodID(IID_Hidd_StorageController, 0);
    HiddStorageBusBase =
        OOP_GetMethodID(IID_Hidd_StorageBus, 0);
    HiddStorageUnitBase =
        OOP_GetMethodID(IID_Hidd_StorageUnit, 0);

    storageRoot = OOP_NewObject(NULL, CLID_Hidd_Storage, NULL);

    if (storageRoot)
        HW_EnumDrivers(storageRoot, &controllerenum_hook, &data);

    OOP_ReleaseAttrBases(storage_abd);
}
