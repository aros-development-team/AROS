/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Discover IPMI system interfaces on the PC from SMBIOS type 38
          records and the ACPI SPMI table, and register one hidd.ipmi
          driver per interface with the system hardware root.
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

#define ACPICABase (csd->cs_ACPICABase)
#include <proto/acpica.h>

#include "smbioslib.h"

#include LC_LIBDEFS_FILE

#undef OOPBase
#define OOPBase (LIBBASE->hsi_csd.cs_OOPBase)
#undef HWBase
#define HWBase (LIBBASE->hsi_csd.hwMethodBase)

/* Decoded interface description, independent of where it came from */
struct IPMIInterface
{
    ULONG ii_Type;
    ULONG ii_SpecMajor;
    ULONG ii_SpecMinor;
    ULONG ii_I2CAddress;
    ULONG ii_NVStorageAddress;
    IPTR  ii_BaseAddress;
    ULONG ii_Modifier;
    ULONG ii_AddressSpace;
    ULONG ii_RegSpacing;
    ULONG ii_Interrupt;
    ULONG ii_Source;
};

#define HWIPMI_MAX_INTERFACES   8

/*
 * SMBIOS type 38, "IPMI Device Information" (DSP0134 section 7.39).
 * Records are 0x10 bytes without, 0x12 with the modifier and interrupt
 * fields. The struct is packed so its size matches the record.
 */
struct SMBIOSIPMIDeviceInfo
{
    struct SMBIOSHeader header;
    UBYTE interface_type;
    UBYTE spec_revision;            /* BCD: 0x20 = 2.0 */
    UBYTE i2c_slave_address;
    UBYTE nv_storage_address;       /* 0xFF = none */
    UQUAD base_address;             /* bit 0: 1 = I/O space, 0 = memory */
    UBYTE base_address_modifier;    /* see SMBIOS_IPMI_MOD_* */
    UBYTE interrupt_number;         /* 0 = unspecified */
} __attribute__((packed));

#define SMBIOS_IPMI_MIN_LENGTH      0x10
#define SMBIOS_IPMI_FULL_LENGTH     0x12

#define SMBIOS_IPMI_MOD_SPACING(m)  (((m) >> 6) & 0x03)
#define SMBIOS_IPMI_MOD_ADDR_LSB    0x10
#define SMBIOS_IPMI_MOD_IRQ_INFO    0x08
#define SMBIOS_IPMI_MOD_IRQ_HIGH    0x02
#define SMBIOS_IPMI_MOD_IRQ_LEVEL   0x01

static ULONG IPMI_RegSpacingFromBytes(ULONG bytes)
{
    switch (bytes)
    {
    case 4:
        return vHidd_IPMI_RegSpacing_4;
    case 16:
        return vHidd_IPMI_RegSpacing_16;
    default:
        return vHidd_IPMI_RegSpacing_1;
    }
}

static BOOL IPMI_DecodeSMBIOS(const struct SMBIOSHeader *hdr, struct IPMIInterface *ii)
{
    const struct SMBIOSIPMIDeviceInfo *info = (const struct SMBIOSIPMIDeviceInfo *)hdr;
    UBYTE modifier = 0, irq = 0;
    UQUAD base;

    if (hdr->sm_Length < SMBIOS_IPMI_MIN_LENGTH)
    {
        D(bug("[HWIPMI] type 38 record too short (0x%02x bytes)\n", hdr->sm_Length));
        return FALSE;
    }
    if (hdr->sm_Length >= SMBIOS_IPMI_FULL_LENGTH)
    {
        modifier = info->base_address_modifier;
        irq = info->interrupt_number;
    }

    memset(ii, 0, sizeof(*ii));
    ii->ii_Source = HWIPMI_SOURCE_SMBIOS;
    ii->ii_Type = info->interface_type;
    ii->ii_SpecMajor = (info->spec_revision >> 4) & 0x0F;
    ii->ii_SpecMinor = info->spec_revision & 0x0F;
    ii->ii_I2CAddress = info->i2c_slave_address;
    ii->ii_NVStorageAddress = info->nv_storage_address;
    ii->ii_Modifier = modifier;

    base = info->base_address;
    if (ii->ii_Type == vHidd_IPMI_Interface_SSIF)
    {
        /* For SSIF the field carries the BMC's 7-bit SMBus address in bits 7:1 */
        ii->ii_BaseAddress = (base >> 1) & 0x7F;
        ii->ii_AddressSpace = vHidd_IPMI_AddressSpace_Memory;
    }
    else
    {
        ii->ii_AddressSpace = (base & 1) ? vHidd_IPMI_AddressSpace_IO
                                         : vHidd_IPMI_AddressSpace_Memory;
        base &= ~(UQUAD)1;
        /* Bit 0 of the real address lives in the modifier */
        if (modifier & SMBIOS_IPMI_MOD_ADDR_LSB)
            base |= 1;
#if __WORDSIZE < 64
        if (base >> 32)
        {
            D(bug("[HWIPMI] base address 0x%llx not addressable\n", (unsigned long long)base));
            return FALSE;
        }
#endif
        ii->ii_BaseAddress = (IPTR)base;
    }

    switch (SMBIOS_IPMI_MOD_SPACING(modifier))
    {
    case 1:
        ii->ii_RegSpacing = vHidd_IPMI_RegSpacing_4;
        break;
    case 2:
        ii->ii_RegSpacing = vHidd_IPMI_RegSpacing_16;
        break;
    default:
        ii->ii_RegSpacing = vHidd_IPMI_RegSpacing_1;
        break;
    }

    /* The interrupt number only means something if the record says so */
    ii->ii_Interrupt = (modifier & SMBIOS_IPMI_MOD_IRQ_INFO) ? irq : 0;

    return TRUE;
}

/*
 * ACPI "Service Processor Management Interface" table (IPMI v2.0
 * specification, annex C3). Interface type values match SMBIOS.
 */
static BOOL IPMI_DecodeSPMI(const ACPI_TABLE_SPMI *spmi, struct IPMIInterface *ii)
{
    const ACPI_GENERIC_ADDRESS *gas = &spmi->IpmiRegister;

    if (spmi->Header.Length < sizeof(*spmi))
    {
        D(bug("[HWIPMI] SPMI table too short (%u bytes)\n", spmi->Header.Length));
        return FALSE;
    }

    memset(ii, 0, sizeof(*ii));
    ii->ii_Source = HWIPMI_SOURCE_ACPI;
    ii->ii_Type = spmi->InterfaceType;
    ii->ii_SpecMajor = (spmi->SpecRevision >> 8) & 0xFF;
    ii->ii_SpecMinor = spmi->SpecRevision & 0xFF;
    ii->ii_NVStorageAddress = 0xFF;

    switch (gas->SpaceId)
    {
    case ACPI_ADR_SPACE_SYSTEM_MEMORY:
        ii->ii_AddressSpace = vHidd_IPMI_AddressSpace_Memory;
        break;
    case ACPI_ADR_SPACE_SYSTEM_IO:
        ii->ii_AddressSpace = vHidd_IPMI_AddressSpace_IO;
        break;
    case ACPI_ADR_SPACE_SMBUS:
        ii->ii_AddressSpace = vHidd_IPMI_AddressSpace_Memory;
        ii->ii_I2CAddress = gas->Address & 0xFF;
        break;
    default:
        D(bug("[HWIPMI] SPMI: unsupported address space %u\n", gas->SpaceId));
        return FALSE;
    }
#if __WORDSIZE < 64
    if (gas->Address >> 32)
        return FALSE;
#endif
    ii->ii_BaseAddress = (IPTR)gas->Address;

    /* The register bit width encodes the register spacing */
    ii->ii_RegSpacing = IPMI_RegSpacingFromBytes(gas->BitWidth ? gas->BitWidth / 8 : 1);

    /* Bit 1 of the interrupt type: a global system interrupt is given */
    if (spmi->InterruptType & 0x02)
        ii->ii_Interrupt = spmi->Interrupt;

    return TRUE;
}

static BOOL IPMI_SameInterface(const struct IPMIInterface *a, const struct IPMIInterface *b)
{
    return a->ii_Type == b->ii_Type && a->ii_BaseAddress == b->ii_BaseAddress
        && a->ii_AddressSpace == b->ii_AddressSpace;
}

static const char *IPMI_TypeName(ULONG type)
{
    static const char *const names[] = { "unknown", "KCS", "SMIC", "BT", "SSIF" };

    return type < sizeof(names) / sizeof(names[0]) ? names[type] : "reserved";
}

static ULONG IPMI_Discover(struct ipmiclass_staticdata *csd, struct IPMIInterface *found, ULONG max)
{
    struct SMBIOSTable st;
    ULONG count = 0;

    if (SMBIOS_Locate(&st))
    {
        const struct SMBIOSHeader *hdr = NULL;

        D(bug("[HWIPMI] SMBIOS %u.%u table @ 0x%p, %u bytes\n", st.st_MajorVersion,
            st.st_MinorVersion, st.st_Table, st.st_TableLength));

        while (count < max && (hdr = SMBIOS_FindStructure(&st, SMBIOS_TYPE_IPMI, hdr)))
        {
            D(bug("[HWIPMI] type 38 record @ 0x%p, length 0x%02x, handle 0x%04x\n",
                hdr, hdr->sm_Length, hdr->sm_Handle));
            if (IPMI_DecodeSMBIOS(hdr, &found[count]))
                count++;
        }
    }
    else
    {
        D(bug("[HWIPMI] no SMBIOS entry point\n"));
    }

    if (ACPICABase)
    {
        ACPI_TABLE_HEADER *table;
        UINT32 instance;

        for (instance = 1; count < max; instance++)
        {
            struct IPMIInterface ii;
            ULONG i;

            if (ACPI_FAILURE(AcpiGetTable(ACPI_SIG_SPMI, instance, &table)))
                break;
            D(bug("[HWIPMI] SPMI table %u @ 0x%p\n", instance, table));
            if (!IPMI_DecodeSPMI((const ACPI_TABLE_SPMI *)table, &ii))
                continue;

            for (i = 0; i < count; i++)
            {
                if (IPMI_SameInterface(&found[i], &ii))
                    break;
            }
            if (i < count)
            {
                D(bug("[HWIPMI] SPMI %s @ 0x%p already known from SMBIOS\n",
                    IPMI_TypeName(ii.ii_Type), (APTR)ii.ii_BaseAddress));
                continue;
            }
            found[count++] = ii;
        }
    }

    return count;
}

static int HWIPMI_Init(LIBBASETYPEPTR LIBBASE)
{
    struct ipmiclass_staticdata *csd = &LIBBASE->hsi_csd;
    struct IPMIInterface found[HWIPMI_MAX_INTERFACES];
    struct OOP_ABDescr attrbases[] =
    {
        { (STRPTR) IID_HW,          &csd->hwAB },
        { (STRPTR) IID_Hidd,        &csd->hiddAB },
        { (STRPTR) IID_Hidd_IPMI,   &csd->hiddIPMIAB },
        { NULL, NULL }
    };
    OOP_Object *root;
    ULONG count, i;

    D(bug("[HWIPMI] %s()\n", __func__));

    /* ACPI is optional: without it only SMBIOS is consulted */
    csd->cs_ACPICABase = OpenLibrary("acpica.library", 0);

    count = IPMI_Discover(csd, found, HWIPMI_MAX_INTERFACES);

    if (csd->cs_ACPICABase)
    {
        CloseLibrary(csd->cs_ACPICABase);
        csd->cs_ACPICABase = NULL;
    }

    if (count == 0)
    {
        D(bug("[HWIPMI] no IPMI interfaces described by firmware\n"));
        return FALSE;
    }

    root = OOP_NewObject(NULL, CLID_Hidd_System, NULL);
    if (!root)
        root = OOP_NewObject(NULL, CLID_HW_Root, NULL);
    if (!root)
        return FALSE;

    OOP_ObtainAttrBases(attrbases);
    HWBase = OOP_GetMethodID(IID_HW, 0);

    for (i = 0; i < count; i++)
    {
        const struct IPMIInterface *ii = &found[i];
        struct TagItem tags[] =
        {
            { csd->hiddIPMIAB + aoHidd_IPMI_InterfaceType,       ii->ii_Type             },
            { csd->hiddIPMIAB + aoHidd_IPMI_SpecVersionMajor,    ii->ii_SpecMajor        },
            { csd->hiddIPMIAB + aoHidd_IPMI_SpecVersionMinor,    ii->ii_SpecMinor        },
            { csd->hiddIPMIAB + aoHidd_IPMI_I2CSlaveAddress,     ii->ii_I2CAddress       },
            { csd->hiddIPMIAB + aoHidd_IPMI_NVStorageAddress,    ii->ii_NVStorageAddress },
            { csd->hiddIPMIAB + aoHidd_IPMI_BaseAddress,         ii->ii_BaseAddress      },
            { csd->hiddIPMIAB + aoHidd_IPMI_BaseAddressModifier, ii->ii_Modifier         },
            { csd->hiddIPMIAB + aoHidd_IPMI_AddressSpace,        ii->ii_AddressSpace     },
            { csd->hiddIPMIAB + aoHidd_IPMI_RegisterSpacing,     ii->ii_RegSpacing       },
            { csd->hiddIPMIAB + aoHidd_IPMI_InterruptNumber,     ii->ii_Interrupt        },
            { TAG_DONE, 0 }
        };
        OOP_Object *drv;

        D(bug("[HWIPMI] %s interface (IPMI %u.%u) from %s: base 0x%p (%s), register spacing %u, irq %u\n",
            IPMI_TypeName(ii->ii_Type), ii->ii_SpecMajor, ii->ii_SpecMinor,
            ii->ii_Source == HWIPMI_SOURCE_ACPI ? "ACPI SPMI" : "SMBIOS",
            (APTR)ii->ii_BaseAddress,
            ii->ii_AddressSpace == vHidd_IPMI_AddressSpace_IO ? "I/O" : "memory",
            ii->ii_RegSpacing == vHidd_IPMI_RegSpacing_16 ? 16 : ii->ii_RegSpacing == vHidd_IPMI_RegSpacing_4 ? 4 : 1,
            ii->ii_Interrupt));

        drv = HW_AddDriver(root, csd->oopclass, tags);
        if (drv)
        {
            struct HWIPMIData *data = OOP_INST_DATA(csd->oopclass, drv);

            data->hwipmi_Source = ii->ii_Source;
            csd->cs_Count++;
        }
        else
        {
            bug("[HWIPMI] failed to add %s interface @ 0x%p\n",
                IPMI_TypeName(ii->ii_Type), (APTR)ii->ii_BaseAddress);
        }
    }

    if (csd->cs_Count == 0)
    {
        OOP_ReleaseAttrBases(attrbases);
        return FALSE;
    }

    return TRUE;
}

ADD2INITLIB(HWIPMI_Init, -2)
