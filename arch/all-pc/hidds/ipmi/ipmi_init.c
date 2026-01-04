/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <string.h>

#include <exec/types.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include <utility/tagitem.h>

#include <hidd/system.h>

#include <hardware/smbios.h>

#include "ipmi_intern.h"

#include LC_LIBDEFS_FILE

#undef OOPBase
#define OOPBase (LIBBASE->hsi_csd.cs_OOPBase)
#undef HWBase
#define HWBase (LIBBASE->hsi_csd.hwMethodBase)

#define SMBIOS_SCAN_START 0x000F0000
#define SMBIOS_SCAN_END   0x000FFFFF
#define SMBIOS_SCAN_STEP  16

struct SMBIOSIPMIDeviceInfo
{
    struct SMBIOSHeader header;
    UBYTE interface_type;
    UBYTE spec_revision;
    UBYTE i2c_slave_address;
    UBYTE nv_storage_address;
    UQUAD base_address;
    UBYTE base_address_modifier;
    UBYTE interrupt_number;
};

static const struct SMBIOSHeader *SMBIOS_GetNextTable(const struct SMBIOSHeader *table)
{
    const UBYTE *ptr = (const UBYTE *)((IPTR)table + table->sm_Length);

    while (1)
    {
        if (ptr[0] == 0 && ptr[1] == 0)
            return (const struct SMBIOSHeader *)(ptr + 2);
        ptr++;
    }

    return NULL;
}

static const struct SMBIOSHeader *SMBIOS_FindTable(UBYTE type)
{
    const struct SMBIOSHeader *table = NULL;
    const UBYTE *ptr = (const UBYTE *)SMBIOS_SCAN_START;

    while (ptr <= (const UBYTE *)SMBIOS_SCAN_END)
    {
        if (!memcmp(ptr, "_SM_", 4))
        {
            const struct SMBIOSEntryPoint2 *eps = (const struct SMBIOSEntryPoint2 *)ptr;
            table = (const struct SMBIOSHeader *)(IPTR)eps->table_address;
            break;
        }
        if (!memcmp(ptr, "_SM3_", 5))
        {
            const struct SMBIOSEntryPoint3 *eps = (const struct SMBIOSEntryPoint3 *)ptr;
            table = (const struct SMBIOSHeader *)(IPTR)eps->table_address;
            break;
        }
        ptr += SMBIOS_SCAN_STEP;
    }

    if (!table)
        return NULL;

    while (table->sm_Type != 127)
    {
        if (table->sm_Type == type)
            return table;
        table = SMBIOS_GetNextTable(table);
    }

    return NULL;
}

static ULONG IPMI_RegSpacingFromModifier(UBYTE modifier)
{
    switch ((modifier >> 6) & 0x03)
    {
    case 0:
        return vHidd_IPMI_RegSpacing_1;
    case 1:
        return vHidd_IPMI_RegSpacing_4;
    case 2:
        return vHidd_IPMI_RegSpacing_16;
    default:
        return vHidd_IPMI_RegSpacing_1;
    }
}

static void IPMI_ParseSMBIOSInfo(const struct SMBIOSIPMIDeviceInfo *info,
    OOP_AttrBase ipmiAB, struct TagItem *tags)
{
    ULONG versionMajor = (info->spec_revision >> 4) & 0x0f;
    ULONG versionMinor = info->spec_revision & 0x0f;
    ULONG addressSpace = (info->base_address & 0x1) ? vHidd_IPMI_AddressSpace_IO
                                                   : vHidd_IPMI_AddressSpace_Memory;
    IPTR baseAddress = (IPTR)(info->base_address & ~(UQUAD)0x1);

    tags[0].ti_Tag = ipmiAB + aoHidd_IPMI_InterfaceType;
    tags[0].ti_Data = info->interface_type;
    tags[1].ti_Tag = ipmiAB + aoHidd_IPMI_SpecVersionMajor;
    tags[1].ti_Data = versionMajor;
    tags[2].ti_Tag = ipmiAB + aoHidd_IPMI_SpecVersionMinor;
    tags[2].ti_Data = versionMinor;
    tags[3].ti_Tag = ipmiAB + aoHidd_IPMI_I2CSlaveAddress;
    tags[3].ti_Data = info->i2c_slave_address;
    tags[4].ti_Tag = ipmiAB + aoHidd_IPMI_NVStorageAddress;
    tags[4].ti_Data = info->nv_storage_address;
    tags[5].ti_Tag = ipmiAB + aoHidd_IPMI_BaseAddress;
    tags[5].ti_Data = (IPTR)baseAddress;
    tags[6].ti_Tag = ipmiAB + aoHidd_IPMI_BaseAddressModifier;
    tags[6].ti_Data = info->base_address_modifier;
    tags[7].ti_Tag = ipmiAB + aoHidd_IPMI_AddressSpace;
    tags[7].ti_Data = addressSpace;
    tags[8].ti_Tag = ipmiAB + aoHidd_IPMI_RegisterSpacing;
    tags[8].ti_Data = IPMI_RegSpacingFromModifier(info->base_address_modifier);
    tags[9].ti_Tag = ipmiAB + aoHidd_IPMI_InterruptNumber;
    tags[9].ti_Data = info->interrupt_number;
    tags[10].ti_Tag = TAG_DONE;
    tags[10].ti_Data = 0;
}

static int HWIPMI_Init(LIBBASETYPEPTR LIBBASE)
{
    struct ipmiclass_staticdata *csd = &LIBBASE->hsi_csd;
    OOP_Object *root;
    const struct SMBIOSIPMIDeviceInfo *info;
    int retVal = FALSE;

    D(bug("[HWIPMI] %s()\n", __func__));

    root = OOP_NewObject(NULL, CLID_Hidd_System, NULL);
    if (!root)
        root = OOP_NewObject(NULL, CLID_HW_Root, NULL);
    if (!root)
        return FALSE;

    info = (const struct SMBIOSIPMIDeviceInfo *)SMBIOS_FindTable(38);
    if (!info)
        return FALSE;

    if (info->header.sm_Length < sizeof(struct SMBIOSIPMIDeviceInfo))
        return FALSE;

    {
        struct OOP_ABDescr attrbases[] =
        {
            { (STRPTR) IID_HW,          &csd->hwAB },
            { (STRPTR) IID_Hidd,        &csd->hiddAB },
            { (STRPTR) IID_Hidd_IPMI,   &csd->hiddIPMIAB },
            { NULL, NULL }
        };

        struct TagItem instanceTags[11];

        OOP_ObtainAttrBases(attrbases);

        HWBase = OOP_GetMethodID(IID_HW, 0);
        IPMI_ParseSMBIOSInfo(info, csd->hiddIPMIAB, instanceTags);

        if (HW_AddDriver(root, csd->oopclass, instanceTags))
            retVal = TRUE;
        else
            OOP_ReleaseAttrBases(attrbases);
    }

    return retVal;
}

ADD2INITLIB(HWIPMI_Init, -2)
