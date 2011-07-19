/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <inttypes.h>
#include <string.h>

#include <exec/lists.h>
#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/arossupport.h>

#include "kernel_base.h"
#include "kernel_bootmem.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "acpi.h"

#define D(x) x

/************************************************************************************************/
struct KernelACPIData *_Kern_ACPIData = NULL;

struct acpi_table_sdt KernACPISDTEntries[ACPI_MAX_TABLES];
/* "Unknown" must always be first entry! */
static const struct acpi_sdtsigids
{
    int         sdtsig_id;
    char        *sdtsig_str;
} KernACPISDTSigs[ACPI_TABLE_COUNT] = 
{
    { ACPI_TABLE_UNKNOWN,       "Unknown"       },
    { ACPI_APIC,                "APIC"          },
    { ACPI_BOOT,                "BOOT"          },
    { ACPI_DBGP,                "DBGP"          },
    { ACPI_DSDT,                "DSDT"          },
    { ACPI_ECDT,                "ECDT"          },
    { ACPI_ETDT,                "ETDT"          },
    { ACPI_FADT,                "FADT"          },
    { ACPI_FACS,                "FACS"          },
    { ACPI_OEMX,                "OEMX"          },
    { ACPI_PSDT,                "PSDT"          },
    { ACPI_SBST,                "SBST"          },
    { ACPI_SLIT,                "SLIT"          },
    { ACPI_SPCR,                "SPCR"          },
    { ACPI_SRAT,                "SRAT"          },
    { ACPI_SSDT,                "SSDT"          },
    { ACPI_SPMI,                "SPMI"          },
    { ACPI_HPET,                "HPET"          }
};

/*
    Everything that doesnt work MUST be put on the OEMBlacklist!!!
    If the problem is critical - mark it as such
*/

enum acpi_oemblacklist_revisionmatch
{
    all_versions,
    less_than_or_equal,
    equal,
    greater_than_or_equal,
};

struct acpi_oemblacklist_entry
{
    char                                      oem_id[7];
    char                                      oem_table_id[9];
    unsigned int                              oem_revision;
    unsigned int                              acpi_tableid;
    enum acpi_oemblacklist_revisionmatch      oem_revision_match;
    char                                     *blacklist_reason;
    unsigned int                              blacklist_critical;
};

struct acpi_oemblacklist_entry _ACPI_OEMBlacklist[] =
{
/* ASUS K7M */
    {"ASUS  ",      "K7M     ", 0x00001000, ACPI_DSDT, less_than_or_equal, "Field beyond end of region", 0},

/* ASUS P2B-S */
    {"ASUS\0\0",    "P2B-S   ", 0, ACPI_DSDT, all_versions, "Bogus PCI routing", 1},

/* Seattle 2 - old BIOS rev. */
    {"INTEL ",      "440BX   ", 0x00001000, ACPI_DSDT, less_than_or_equal, "Field beyond end of region", 0},

/* Intel 810 Motherboard */
    {"MNTRAL",      "MO81010A", 0x00000012, ACPI_DSDT, less_than_or_equal, "Field beyond end of region", 0},

/* Compaq Presario 711FR */
    {"COMAPQ",      "EAGLES",   0x06040000, ACPI_DSDT, less_than_or_equal, "SCI issues (C2 disabled)", 0},

/* Compaq Presario 1700 */
    {"PTLTD ",      "  DSDT  ", 0x06040000, ACPI_DSDT, less_than_or_equal, "Multiple problems", 1},

/* Sony FX120, FX140, FX150? */
    {"SONY  ",      "U0      ", 0x20010313, ACPI_DSDT, less_than_or_equal, "ACPI driver problem", 1},

/* Compaq Presario 800, Insyde BIOS */
    {"INT440",      "SYSFexxx", 0x00001001, ACPI_DSDT, less_than_or_equal, "Does not use _REG to protect EC OpRegions", 1},

/* IBM 600E - _ADR should return 7, but it returns 1 */
    {"IBM   ",      "TP600E  ", 0x00000105, ACPI_DSDT, less_than_or_equal, "Incorrect _ADR", 1},

/* Portege 7020, BIOS 8.10 */
    {"TOSHIB",      "7020CT  ", 0x19991112, ACPI_DSDT, all_versions, "Implicit Return", 0},

/* Portege 4030 */
    {"TOSHIB",      "4030    ", 0x19991112, ACPI_DSDT, all_versions, "Implicit Return", 0},

/* Portege 310/320, BIOS 7.1 */
    {"TOSHIB",      "310     ", 0x19990511, ACPI_DSDT, all_versions, "Implicit Return", 0},

    {"\0"}
};

/************************************************************************************************
                                    ACPI RELATED FUNCTIONS
 ************************************************************************************************/

/************************************************************************************************/
static const struct acpi_sdtsigids *core_ACPIGetSDTSigFromID(int _id)
{
    int id;

    for (id = 1; id < ACPI_TABLE_COUNT; id++)
    {
        if (KernACPISDTSigs[id].sdtsig_id == _id)
        {
            return &KernACPISDTSigs[id];
        }
    }

    return NULL;
};

#if 0
static const struct acpi_sdtsigids *core_ACPIGetSDTSigFromSTR(char *_id_str)
{
    int id;

    for (id = 1; id < ACPI_TABLE_COUNT; id++)
    {
        if (!strncmp(_id_str, KernACPISDTSigs[id].sdtsig_str, 4))
        {
            return &KernACPISDTSigs[id];
        }
    }

    return NULL;
};
#endif

#ifdef DUMP_TABLES

static void core_ACPITableDump(IPTR dump_header, IPTR dump_table)
{
    /* TODO: implement the table dump.. */
    return;
}

#else

#define core_ACPITableDump(header, table)

#endif

/*
 * core_ACPITableHeaderEarly() .... used by both core_ACPIIsBlacklisted() and core_ACPITableSDTGet()
 */
int core_ACPITableHeaderEarly(int id, struct acpi_table_header ** header)
{
    unsigned int                    i;
    const struct acpi_table_sdt     *header_tmp;
    enum acpi_table_id              temp_id;
    const struct acpi_sdtsigids     *sig_id;

    D(bug("[Kernel] core_ACPITableHeaderEarly(id=%d)\n", id));

    /* DSDT is different from the rest */
    if (id == ACPI_DSDT) temp_id = ACPI_FADT;
    else temp_id = id;

    if ((sig_id = core_ACPIGetSDTSigFromID(id)) != NULL)
    {
        /* Locate the table. */
        for (i = 0; i < _Kern_ACPIData->kb_ACPI_SDT_Count; i++)
        {
            header_tmp = (struct acpi_table_sdt *)_Kern_ACPIData->kb_ACPI_SDT_Entry[i];

            D(bug("[Kernel] core_ACPITableHeaderEarly: header_tmp = %p\n", header_tmp));

            if (header_tmp->id != temp_id)
                continue;

            *header = (struct acpi_table_header *)header_tmp->pa;
            if (*header == NULL)
            {
                bug("[Kernel] core_ACPITableHeaderEarly: ERROR - table %s has bad pointer\n", sig_id->sdtsig_str);
                return 0;
            }
            break;
        }

        if (*header == NULL)
        {
            bug("[Kernel] core_ACPITableHeaderEarly: WARNING - %s not present\n",
                sig_id->sdtsig_str);
            return 0;
        }

        if (id == ACPI_DSDT)
        {
            /* Map the DSDT header via the pointer in the FADT */
            struct acpi_table_fadt *FADT = (struct acpi_table_fadt *)*header;

            *header = (struct acpi_table_header *)(unsigned long)FADT->dsdt_addr;
            if (*header == NULL)
            {
                bug("[Kernel] core_ACPITableHeaderEarly: ERROR - bad DSDT pointer\n");
                return 0;
            }
        }
    }

    /* CHECKME: Should this be nonzero ? */
    return 0;
}

/**********************************************************/
int core_ACPITableMADTFamParse(int id, unsigned long madt_size, int entry_id, struct acpi_madt_entry_hook * entry_handler)
{
    IPTR                            madt = 0;
    struct acpi_table_entry_header  *entry = NULL;
    unsigned long                   count = 0;
    unsigned long                   madt_end = 0;
    unsigned int                    i = 0;
    struct acpi_table_sdt           *header_tmp;

    if (!entry_handler )
    	return 0;

    /* Locate the MADT (if exists). There should only be one. */
    for (i = 0; i < _Kern_ACPIData->kb_ACPI_SDT_Count; i++)
    {
        header_tmp = (struct acpi_table_sdt *)_Kern_ACPIData->kb_ACPI_SDT_Entry[i];

        if ( header_tmp->id != id)
            continue;

        madt = header_tmp->pa;
        if (!madt)
        {
            bug("[Kernel] core_ACPITableMADTFamParse: ERROR - table %s has bad pointer\n", KernACPISDTSigs[id]);
            return 0;
        }
        break;
    }

    if (!madt)
    {
        bug("[Kernel] core_ACPITableMADTFamParse: WARNING - %s not present\n", KernACPISDTSigs[id]);
        return 0;
    }

    madt_end = madt + header_tmp->size;

    entry = (struct acpi_table_entry_header *)(madt + madt_size);

    /* Parse all entries looking for a match. */
    while (((unsigned long)entry) < madt_end)
    {
        if (entry->type == entry_id)
        {
            count++;

            entry_handler->header = entry;
            AROS_UFC1(IPTR, entry_handler->h_Entry, AROS_UFCA(struct Hook *, (struct Hook *)entry_handler, A0));
        }
        entry = (struct acpi_table_entry_header *)((unsigned long)entry + entry->length);
    }

    return count;
}

/**********************************************************/
int core_ACPITableMADTParse(int id, struct acpi_madt_entry_hook * table_handler)
{
    return  core_ACPITableMADTFamParse(ACPI_APIC, sizeof(struct acpi_table_madt), id, table_handler);
}

/**********************************************************/
int core_ACPITableParse(int id, struct acpi_table_hook * header_handler)
{
    int			                    count = 0;
    unsigned int		            i = 0;
    struct acpi_table_sdt           *header_tmp;

    if (!header_handler)
    	return 0;

    for ( i = 0; i < _Kern_ACPIData->kb_ACPI_SDT_Count; i++ )
    {
        header_tmp = (struct acpi_table_sdt *)_Kern_ACPIData->kb_ACPI_SDT_Entry[i];

        if ( header_tmp->id != id )	continue;

        header_handler->phys_addr = header_tmp->pa;
        header_handler->size = header_tmp->size;
        AROS_UFC1 ( IPTR, header_handler->h_Entry, AROS_UFCA(struct Hook *, (struct Hook *)header_handler, A0) );
        
        count++;
    }

    return count;
}

/**********************************************************/
IPTR core_ACPITableSDTGet(struct acpi_table_rsdp * RSDP)
{
    struct acpi_table_header  *header = NULL;
    unsigned int		      i, id = 0;

    if (RSDP == NULL)
    	return 0;

    /* First check XSDT (but only on ACPI 2.0-compatible systems) */
    if ((RSDP->revision >= 2) && (((struct acpi20_table_rsdp*)RSDP)->xsdt_address))
    {
        struct acpi_table_xsdt	*XSDT = NULL;

        _Kern_ACPIData->kb_ACPI_SDT_Phys = ((struct acpi20_table_rsdp *)RSDP)->xsdt_address;

        bug("[Kernel] core_ACPITableSDTGet: Checking ACPI v2 SDT (XSDT) @ %p\n", _Kern_ACPIData->kb_ACPI_SDT_Phys);
        if ((XSDT = (struct acpi_table_xsdt *)_Kern_ACPIData->kb_ACPI_SDT_Phys) == NULL)
        {
            bug("[Kernel] core_ACPITableSDTGet: ERROR - bad XSDT pointer\n");
            return 0;
        }
        header = &XSDT->header;

        if (strncmp( header->signature, "XSDT", 4))
        {
            bug("[Kernel] core_ACPITableSDTGet: ERROR - bad XSDT signature [header @ %p, sig='%4.4s']\n",header,header->signature);
            return 0;
        }

        if (core_ACPITableChecksum(header, header->length))
        {
            bug("[Kernel] core_ACPITableSDTGet: ERROR - XSDT checksum invalid\n");
            return 0;
        }

        _Kern_ACPIData->kb_ACPI_SDT_Count = (header->length - sizeof(struct acpi_table_header)) >> 3;
        if (_Kern_ACPIData->kb_ACPI_SDT_Count > ACPI_MAX_TABLES) 
        {
            bug("[Kernel] core_ACPITableSDTGet: WARNING - Truncated XSDT to %lu entries\n", (_Kern_ACPIData->kb_ACPI_SDT_Count - ACPI_MAX_TABLES));
            _Kern_ACPIData->kb_ACPI_SDT_Count = ACPI_MAX_TABLES;
        }

        D(bug("[Kernel] core_ACPITableSDTGet: Copying Tables Start\n"));
        for (i = 0; i < _Kern_ACPIData->kb_ACPI_SDT_Count; i++)
        {
            _Kern_ACPIData->kb_ACPI_SDT_Entry[i] = (IPTR)&KernACPISDTEntries[i];
            bug("[Kernel] core_ACPITableSDTGet: Table %d @ %p\n", i, _Kern_ACPIData->kb_ACPI_SDT_Entry[i]);
            ((struct acpi_table_sdt *)_Kern_ACPIData->kb_ACPI_SDT_Entry[i])->pa = (unsigned long)XSDT->entry[i];
            bug("[Kernel] core_ACPITableSDTGet: Table Entry @ %p\n", ((struct acpi_table_sdt *)_Kern_ACPIData->kb_ACPI_SDT_Entry[i])->pa);
        }
        D(bug("[Kernel] core_ACPITableSDTGet: Copying Tables done!\n"));
    }
    else if (RSDP->rsdt_address)
    {
        /* If there is no XSDT, then check RSDT */
        struct acpi_table_rsdt	*RSDT = NULL;

        _Kern_ACPIData->kb_ACPI_SDT_Phys = RSDP->rsdt_address;
        bug("[Kernel] core_ACPITableSDTGet: Checking ACPI v1 SDT (RSDT) @ %p\n", _Kern_ACPIData->kb_ACPI_SDT_Phys);

        if ((RSDT = (struct acpi_table_rsdt *)_Kern_ACPIData->kb_ACPI_SDT_Phys) == NULL)
        {
            bug("[Kernel] core_ACPITableSDTGet: ERROR - bad RSDT pointer\n");
            return 0;
        }
        header = &RSDT->header;

        if (strncmp(header->signature, "RSDT", 4))
        {
            bug("[Kernel] core_ACPITableSDTGet: ERROR - bad RSDT signature [header @ %p, sig='%4.4s']\n",header,header->signature);
            return 0;
        }

        if (core_ACPITableChecksum(header, header->length))
        {
            bug("[Kernel] core_ACPITableSDTGet: ERROR - RSDT checksum invalid\n");
            return 0;
        }

        _Kern_ACPIData->kb_ACPI_SDT_Count = (header->length - sizeof(struct acpi_table_header)) >> 2;
        if (_Kern_ACPIData->kb_ACPI_SDT_Count > ACPI_MAX_TABLES)
        {
            bug("[Kernel] core_ACPITableSDTGet: WARNING - Truncated RSDT to %lu entries\n", (_Kern_ACPIData->kb_ACPI_SDT_Count - ACPI_MAX_TABLES));
            _Kern_ACPIData->kb_ACPI_SDT_Count = ACPI_MAX_TABLES;
        }

        D(bug("[Kernel] core_ACPITableSDTGet: Copying Tables Start\n"));
        for (i = 0; i < _Kern_ACPIData->kb_ACPI_SDT_Count; i++)
        {   
            _Kern_ACPIData->kb_ACPI_SDT_Entry[i] = (IPTR)&KernACPISDTEntries[i];
            bug("[Kernel] core_ACPITableSDTGet: Table %d @ %p\n", i, _Kern_ACPIData->kb_ACPI_SDT_Entry[i]);
            ((struct acpi_table_sdt *)_Kern_ACPIData->kb_ACPI_SDT_Entry[i])->pa = (unsigned long)RSDT->entry[i];
            bug("[Kernel] core_ACPITableSDTGet: Table Entry @ %p\n", ((struct acpi_table_sdt *)_Kern_ACPIData->kb_ACPI_SDT_Entry[i])->pa);
        }
        D(bug("[Kernel] core_ACPITableSDTGet: Copying Tables done!\n"));
    }
    else 
    {
        bug("[Kernel] core_ACPITableSDTGet: No System Description Table (SDT) specified in RSDP\n");
        return 0;
    }

    bug("[Kernel] core_ACPITableSDTGet: Dumping Root Table\n");
    core_ACPITableDump(header, _Kern_ACPIData->kb_ACPI_SDT_Phys);

    bug("[Kernel] core_ACPITableSDTGet: Checking Tables..\n");
    for (i = 0; i < _Kern_ACPIData->kb_ACPI_SDT_Count; i++) 
    {
        bug("[Kernel] core_ACPITableSDTGet: Table %d\n", i);
        header = (struct acpi_table_header *)(((struct acpi_table_sdt*)_Kern_ACPIData->kb_ACPI_SDT_Entry[i])->pa);
        if (header == NULL)
            continue;

        bug("[Kernel] core_ACPITableSDTGet: Table %d Header @ %p, sig='%4.4s'\n", i, header, header->signature);
        
        core_ACPITableDump(header, ((struct acpi_table_sdt *)_Kern_ACPIData->kb_ACPI_SDT_Entry[i])->pa);

        if (core_ACPITableChecksum(header, header->length))
        {
            bug("[Kernel] core_ACPITableSDTGet: WARNING - SDT %d Checksum invalid\n", i);
            continue;
        }

        ((struct acpi_table_sdt *)_Kern_ACPIData->kb_ACPI_SDT_Entry[i])->size = header->length;

        /* Start at 1 to skip "unknown" */
        ((struct acpi_table_sdt *)_Kern_ACPIData->kb_ACPI_SDT_Entry[i])->id = 0;
        for (id = 1; id < ACPI_TABLE_COUNT; id++)
        {
            if (!strncmp((char *)&header->signature, KernACPISDTSigs[id].sdtsig_str, sizeof(header->signature)))
            {
                ((struct acpi_table_sdt *)_Kern_ACPIData->kb_ACPI_SDT_Entry[i])->id = KernACPISDTSigs[id].sdtsig_id;
                break;
            }
        }
        D(bug("[Kernel] core_ACPITableSDTGet: Table ID = %d\n", ((struct acpi_table_sdt *)_Kern_ACPIData->kb_ACPI_SDT_Entry[i])->id));
    }

    D(bug("[Kernel] core_ACPITableSDTGet: Tables Checked\n"));
    /*  We want to print the DSDT (because this is what people usually blacklist against),
        but it is *not* in the RSDT.  We don't know its physical addr, so just print 0.    */
    header = NULL;
    if (!(core_ACPITableHeaderEarly(ACPI_DSDT, &header)))
    {
        core_ACPITableDump(header, 0);
    }
    D(bug("[Kernel] core_ACPITableSDTGet: Finished\n"));

    return _Kern_ACPIData->kb_ACPI_SDT_Phys;
}

/**********************************************************/
int core_ACPIIsBlacklisted()
{
    int i = 0;
    int blacklisted = 0;
    struct acpi_table_header *table_header;

    D(bug("[Kernel] core_ACPIIsBlacklisted()\n"));
    
    while (_ACPI_OEMBlacklist[i].oem_id[0] != '\0')
    {
        table_header = NULL;
        if (core_ACPITableHeaderEarly(_ACPI_OEMBlacklist[i].acpi_tableid, &table_header)) 
        {
            i++;
            continue;
        }

        if (strncmp(_ACPI_OEMBlacklist[i].oem_id, table_header->oem_id, 6))
        {
            i++;
            continue;
        }

        if (strncmp(_ACPI_OEMBlacklist[i].oem_table_id, table_header->oem_table_id, 8))
        {
            i++;
            continue;
        }

        if (table_header != NULL)
        {
            if ((_ACPI_OEMBlacklist[i].oem_revision_match == all_versions) ||
               ((_ACPI_OEMBlacklist[i].oem_revision_match == less_than_or_equal) &&
                (table_header->oem_revision <= _ACPI_OEMBlacklist[i].oem_revision)) ||
               ((_ACPI_OEMBlacklist[i].oem_revision_match == greater_than_or_equal) &&
                (table_header->oem_revision >= _ACPI_OEMBlacklist[i].oem_revision)) ||
               ((_ACPI_OEMBlacklist[i].oem_revision_match == equal) &&
                (table_header->oem_revision == _ACPI_OEMBlacklist[i].oem_revision)))
            {
                bug("[Kernel] core_ACPIIsBlacklisted: ERROR - Vendor \"%6.6s\" System \"%8.8s\" Revision 0x%x has a known ACPI BIOS problem.\n",
                    _ACPI_OEMBlacklist[i].oem_id, _ACPI_OEMBlacklist[i].oem_table_id, _ACPI_OEMBlacklist[i].oem_revision);
                bug("[Kernel] core_ACPIIsBlacklisted: ERROR - Reason: %s. This is a %s error\n",
                    _ACPI_OEMBlacklist[i].blacklist_reason, ( _ACPI_OEMBlacklist[i].blacklist_critical ? "non-recoverable" : "recoverable" ));

                blacklisted = _ACPI_OEMBlacklist[i].blacklist_critical;
                break;
            }
        }
        else i++;
    }

    return blacklisted;
}

ULONG core_ACPIInitialise(struct PlatformData *pdata)
{
    int                         result = 0;
    D(bug("[Kernel] core_ACPIInitialise()\n"));

    if ((_Kern_ACPIData == NULL) || (pdata == NULL))
        return 0;

    if (_Kern_ACPIData->kb_ACPI_Disabled == TRUE)
        return 0;

    struct acpi_table_hook ACPI_TableParse_MADT_hook =
    {
        .h_Entry = (APTR)ACPI_hook_Table_MADT_Parse,
        .data    = pdata
    };

    struct acpi_madt_entry_hook ACPI_TableParse_LAPIC_Addr_Ovr_hook =
    {
        .h_Entry = (APTR)ACPI_hook_Table_LAPIC_Addr_Ovr_Parse,
        .data    = pdata
    };

    struct acpi_madt_entry_hook ACPI_TableParse_LAPIC_hook =
    {
        .h_Entry = (APTR)ACPI_hook_Table_LAPIC_Parse,
        .data    = pdata
    };

    struct acpi_madt_entry_hook ACPI_TableParse_LAPIC_NMI_hook =
    {
        .h_Entry = (APTR)ACPI_hook_Table_LAPIC_NMI_Parse,
        .data    = pdata
    };

    struct acpi_madt_entry_hook ACPI_TableParse_IOAPIC_hook =
    {
        .h_Entry = (APTR)ACPI_hook_Table_IOAPIC_Parse,
        .data    = pdata
    };

    struct acpi_madt_entry_hook ACPI_TableParse_Int_Src_Ovr_hook =
    {
        .h_Entry = (APTR)ACPI_hook_Table_Int_Src_Ovr_Parse,
        .data    = pdata
    };

    struct acpi_madt_entry_hook ACPI_TableParse_NMI_Src_hook =
    {
        .h_Entry = (APTR)ACPI_hook_Table_NMI_Src_Parse,
        .data    = pdata
    };

    struct acpi_table_hook ACPI_TableParse_HPET_hook =
    {
        .h_Entry = (APTR)ACPI_hook_Table_HPET_Parse,
        .data    = pdata
    };

    /*  MADT : If it exists, parse the Multiple APIC Description Table "MADT", 
        This table provides platform SMP configuration information [the successor to MPS tables]	*/
    result = core_ACPITableParse( ACPI_APIC, &ACPI_TableParse_MADT_hook);
    D(bug("[Kernel] core_ACPIInitialise: core_ACPITableParse(ACPI_APIC) returned %p\n", result));
    if (!result )
        return 0;
    else if ( result < 0 )
    {
        bug("[Kernel] core_ACPIInitialise: ERROR - Error parsing MADT\n");
        return result;
    }
    else if ( result > 1 )
    {
        bug("[Kernel] core_ACPIInitialise: WARNING - Multiple MADT tables exist\n");
    }

    /*  Local APIC : The LAPIC address is obtained from the MADT (32-bit value)
        and (optionally) overriden by a LAPIC_ADDR_OVR entry (64-bit value). */
    result = core_ACPITableMADTParse( ACPI_MADT_LAPIC_ADDR_OVR, &ACPI_TableParse_LAPIC_Addr_Ovr_hook );
    D(bug("[Kernel] core_ACPIInitialise: core_ACPITableMADTParse(ACPI_MADT_LAPIC_ADDR_OVR) returned %p\n", result));
    if ( result < 0 )
    {
        bug("[Kernel] core_ACPIInitialise: ERROR - Error parsing LAPIC address override entry\n");
        return result;
    }

    result = core_ACPITableMADTParse( ACPI_MADT_LAPIC, &ACPI_TableParse_LAPIC_hook);
    bug("[Kernel] core_ACPIInitialise: ACPI found %d APICs, System Total APICs: %d\n", result, pdata->kb_APIC_Count);
    if ( !result )
    { 
        /* TODO: Cleanup to allow fallback to MPS.. */
        bug("[Kernel] core_ACPIInitialise: ERROR - No LAPIC entries present\n");
        return 0;
    }
    else if (result < 0)
    {
        /* TODO: Cleanup to allow fallback to MPS.. */
        bug("[Kernel] core_ACPIInitialise: ERROR - Error parsing LAPIC entry\n");
        return result;
    }

    result = core_ACPITableMADTParse( ACPI_MADT_LAPIC_NMI, &ACPI_TableParse_LAPIC_NMI_hook);
    D(bug("[Kernel] core_ACPIInitialise: core_ACPITableMADTParse(ACPI_MADT_LAPIC_NMI) returned %p\n", result));
    if ( result < 0 )
    {
        /* TODO: Cleanup to allow fallback to MPS.. */
        bug("[Kernel] core_ACPIInitialise: ERROR - Error parsing LAPIC NMI entry\n");
        return result;
    }

    /*  I/O APIC : ACPI interpreter is required to complete interrupt setup,
        so if it is off, don't enumerate the io-apics with ACPI.
        If MPS is present, it will handle them, otherwise the system will stay in PIC mode */
    if (_Kern_ACPIData->kb_ACPI_Disabled || !_Kern_ACPIData->kb_ACPI_IRQ) return 1;

    result = core_ACPITableMADTParse(ACPI_MADT_IOAPIC, &ACPI_TableParse_IOAPIC_hook);
    D(bug("[Kernel] core_ACPIInitialise: core_ACPITableMADTParse(ACPI_MADT_IOAPIC) returned %p\n", result));
    if (!result)
    { 
        bug("[Kernel] core_ACPIInitialise: ERROR - No IOAPIC entries present\n");
        return 0;
    }
    else if (result < 0)
    {
        bug("[Kernel] core_ACPIInitialise: ERROR - Error parsing IOAPIC entry\n");
        return result;
    }

    /* Build a default routing table for legacy (ISA) interrupts. */
    /* TODO: implement legacy irq config.. */
    bug("[Kernel] core_ACPIInitialise: Configuring Legacy IRQs .. Skipped (UNIMPLEMENTED) ..\n");

    result = core_ACPITableMADTParse(ACPI_MADT_INT_SRC_OVR, &ACPI_TableParse_Int_Src_Ovr_hook);
    bug("[Kernel] core_ACPIInitialise: core_ACPITableMADTParse(ACPI_MADT_INT_SRC_OVR) returned %p\n", result);
    if (result < 0)
    {
        /* TODO: Cleanup to allow fallback to MPS.. */
        bug("[Kernel] core_ACPIInitialise: ERROR - Error parsing interrupt source overrides entry\n");
        return result;
    }

    result = core_ACPITableMADTParse(ACPI_MADT_NMI_SRC, &ACPI_TableParse_NMI_Src_hook);
    bug("[Kernel] core_ACPIInitialise: core_ACPITableMADTParse(ACPI_MADT_NMI_SRC) returned %p\n", result);
    if (result < 0)
    {
        /* TODO: Cleanup to allow fallback to MPS.. */
        bug("[Kernel] core_ACPIInitialise: ERROR - Error parsing NMI SRC entry\n");
        return result;
    }

    pdata->kb_APIC_IRQ_Model = ACPI_IRQ_MODEL_IOAPIC;
    pdata->kb_ACPI_IOAPIC = 1;

    if ( pdata->kb_APIC_Count && pdata->kb_ACPI_IOAPIC )
    {
        pdata->kb_SMP_Config = 1;
        bug("[Kernel] core_ACPIInitialise: SMP APICs Configured from ACPI\n");
        /* TODO: implement check for clustered apic's.. */
        //core_APICClusteredCheck();
    }

    core_ACPITableParse( ACPI_HPET, &ACPI_TableParse_HPET_hook );

    return 0;
} /* core_ACPIInit */


/**********************************************************/
static IPTR core_ACPIRootSystemDescriptionPointerScan(IPTR scan_start, IPTR scan_length)
{
    unsigned long		                scan_offset;
    char                                *scan_ptr;

    /* Scan for the Root System Description Pointer signature
       on 16-byte boundaries of the physical memory region */
    for (scan_offset = 0; scan_offset < scan_length; scan_offset += 16)
    {
        scan_ptr = (char *)scan_start + scan_offset;

        if ((scan_ptr[0] != 'R') ||
            (scan_ptr[1] != 'S') ||
            (scan_ptr[2] != 'D') ||
            (scan_ptr[3] != ' ') ||
            (scan_ptr[4] != 'P') ||
            (scan_ptr[5] != 'T') ||
            (scan_ptr[6] != 'R') ||
            (scan_ptr[7] != ' '))
            continue;

        /* RSDP located, return its address */
        return (IPTR)scan_ptr;
    }

    return 0;
}

/* Attempt to locate the ACPI Root System Description Pointer */
IPTR core_ACPIRootSystemDescriptionPointerLocate()
{
    IPTR RSDP_PhysAddr;

    /* Search the first Kilobyte of the Extended BIOS Data Area */
    if ((RSDP_PhysAddr = core_ACPIRootSystemDescriptionPointerScan(0x00000000, 0x00000400)) != 0)
    {
        D(bug("[Kernel] core_ACPIRootSystemDescriptionPointerLocate: RSDP found in EBDA @ %p\n", RSDP_PhysAddr));
        return RSDP_PhysAddr;
    }

    /* Search in BIOS ROM address space */
    if ((RSDP_PhysAddr = core_ACPIRootSystemDescriptionPointerScan(0x000E0000, 0x000FFFFF)) != 0)
    {
        D(bug("[Kernel] core_ACPIRootSystemDescriptionPointerLocate: RSDP found in BIOS ROM space @ %p\n", RSDP_PhysAddr));
        return RSDP_PhysAddr;
    }

    return 0;
}

/**********************************************************/
int core_ACPITableChecksum(void * table_pointer, unsigned long table_length)
{
    UBYTE   	                        *tmp_pointer = (UBYTE *)table_pointer;
    unsigned long		                remains = table_length;
    unsigned long		                sum     = 0;

    if (!tmp_pointer || !table_length)
    	return 0;

    while (remains--) sum += *tmp_pointer++;

    return (sum & 0xFF);
}

/**********************************************************/
IPTR core_ACPIProbe(const struct TagItem *msg, struct KernBootPrivate *__KernBootPrivate)
{
    IPTR             	                RSDP_PhysAddr;
    int			                        checksum = 0;
    struct TagItem *tag;

    D(bug("[Kernel] core_ACPIProbe()\n"));

    _Kern_ACPIData = krnAllocBootMem(sizeof(struct KernelACPIData));
    bug("[Kernel] core_ACPIProbe: ACPI Private Data @ %p\n", _Kern_ACPIData);

    _Kern_ACPIData->kb_ACPI_Disabled = TRUE;

    tag = LibFindTagItem(KRN_CmdLine, msg);
    if (tag)
    {
    	if (strstr((char *)tag->ti_Data, "NOACPI"))
        {
            bug("[Kernel] core_ACPIProbe: ACPI Disabled from boot command line\n");

            return 0;
        }
    }
    
    /* Locate the Root System Description Table (RSDP) */
    RSDP_PhysAddr = core_ACPIRootSystemDescriptionPointerLocate();
    if (RSDP_PhysAddr != 0)
    {
        struct acpi_table_rsdp *RSDP = (struct acpi_table_rsdp *)RSDP_PhysAddr;

        bug("[Kernel] core_ACPIProbe: Root System Description Pointer @ %p\n", RSDP);
        bug("[Kernel] core_ACPIProbe: Root System Description Pointer [ v%3.3d '%6.6s' ]\n", RSDP->revision, RSDP->oem_id);

        if (RSDP->revision >= 2) checksum = core_ACPITableChecksum(RSDP, ((struct acpi20_table_rsdp *)RSDP)->length);
        else checksum = core_ACPITableChecksum(RSDP, sizeof(struct acpi_table_rsdp));

        if (checksum)
        {
            bug("[Kernel] core_ACPIProbe: ERROR: Invalid RSDP checksum (%d)\n", checksum);
            return 0;
        }

        /* Locate and map the System Description table (RSDT/XSDT) */
        if (core_ACPITableSDTGet(RSDP) != 0)
        {
            D(bug("[Kernel] core_ACPIProbe: SDT Scanned\n"));
            if (core_ACPIIsBlacklisted())
            {
                bug("[Kernel] core_ACPIProbe: WARNING - BIOS listed in blacklist, disabling ACPI support\n");
                return 0;
            }
            _Kern_ACPIData->kb_ACPI_Disabled = FALSE;
        }
    }
    else
    {
        bug("[Kernel] core_ACPIProbe: Unable to locate RSDP - no ACPI\n");
    }

    D(bug("[Kernel] core_ACPIProbe: Finished\n"));

    return RSDP_PhysAddr;
}
