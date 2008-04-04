/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id: acpi_tables.c,v 1.7 2004/01/07 07:13:03 nicja Exp $
*/
#include <inttypes.h>

#include "exec_intern.h"
#include "etask.h"

#include <exec/lists.h>
#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <asm/segments.h>

#include <proto/kernel.h>

#include "kernel_intern.h"

/************************************************************************************************/

struct KernelACPIData KernACPIData;
struct acpi_table_sdt KernACPISDTEntries[ACPI_MAX_TABLES];
char *                KernACPISDTSigs[ACPI_TABLE_COUNT] = 
{
	"Unknown",
	"APIC",
	"BOOT",
	"DBGP",
	"DSDT",
	"ECDT",
	"ETDT",
	"FADT",
	"FACS",
	"OEMX",
	"PSDT",
	"SBST",
	"SLIT",
	"SPCR",
	"SRAT",
	"SSDT",
	"SPMI",
	"HPET"
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

    {""}
};

/************************************************************************************************
                                    ACPI RELATED FUNCTIONS
 ************************************************************************************************/

/************************************************************************************************/
void core_ACPITableDump(IPTR dump_header, IPTR dump_table)
{
#warning "TODO: implement the table dump.."
    return;
}

/*
 * core_ACPITableHeaderEarly() .... used by both core_ACPIIsBlacklisted() and core_ACPITableSDTGet()
 */
int core_ACPITableHeaderEarly(int id, struct acpi_table_header ** header)
{
    unsigned int                    i;
    struct acpi_table_sdt           *header_tmp;
    enum acpi_table_id              temp_id;

    /* DSDT is different from the rest */
    if (id == ACPI_DSDT) temp_id = ACPI_FADT;
    else temp_id = id;

    /* Locate the table. */
    for (i = 0; i < KernACPIData.kb_ACPI_SDT_Count; i++)
    {
        header_tmp = KernACPIData.kb_ACPI_SDT_Entry[i];
        if (header_tmp->id != temp_id)
            continue;

        *header = header_tmp->pa;
        if (*header == NULL)
        {
            rkprintf("[Kernel] core_ACPITableHeaderEarly: ERROR - table %s has bad pointer\n", KernACPISDTSigs[id]);
            return NULL;
        }
        break;
    }

    if (*header == NULL)
    {
        rkprintf("[Kernel] core_ACPITableHeaderEarly: WARNING - %s not present\n",
            KernACPISDTSigs[id]);
        return NULL;
    }

    if (id == ACPI_DSDT)
    {
        /* Map the DSDT header via the pointer in the FADT */
        struct acpi_table_fadt *FADT = (struct acpi_table_fadt *)*header;

        *header = FADT->dsdt_addr;
        if (*header == NULL)
        {
            rkprintf("[Kernel] core_ACPITableHeaderEarly: ERROR - bad DSDT pointer\n");
            return NULL;
        }
    }

    return 0;
}

/**********************************************************/
int core_ACPITableMADTFamParse(int id, unsigned long madt_size, int entry_id, struct acpi_madt_entry_hook * entry_handler)
{
    void                            *madt = NULL;
    struct acpi_table_entry_header  *entry = NULL;
    unsigned long                   count = 0;
    unsigned long                   madt_end = 0;
    unsigned int                    i = 0;
    struct acpi_table_sdt           *header_tmp;

    if ( !entry_handler ) return NULL;

    /* Locate the MADT (if exists). There should only be one. */
    for (i = 0; i < KernACPIData.kb_ACPI_SDT_Count; i++)
    {
        header_tmp = KernACPIData.kb_ACPI_SDT_Entry[i];

        if ( header_tmp->id != id)
            continue;

        madt = header_tmp->pa;
        if (madt == NULL)
        {
            rkprintf("[Kernel] core_ACPITableMADTFamParse: ERROR - table %s has bad pointer\n", KernACPISDTSigs[id]);
            return NULL;
        }
        break;
    }

    if (madt == NULL)
    {
        rkprintf("[Kernel] core_ACPITableMADTFamParse: WARNING - %s not present\n", KernACPISDTSigs[id]);
        return NULL;
    }

    madt_end = (unsigned long)madt + header_tmp->size;

    entry = (struct acpi_table_entry_header *)((unsigned long) madt + madt_size);

    /* Parse all entries looking for a match. */
    while (((unsigned long)entry) < madt_end)
    {
        if (entry->type == entry_id)
        {
            count++;

            entry_handler->header = entry;
            AROS_UFC1(IPTR, entry_handler->h_Entry, AROS_UFCA(struct Hook *, entry_handler, A0));
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

    if ( !header_handler ) return NULL;

    for ( i = 0; i < KernACPIData.kb_ACPI_SDT_Count; i++ )
    {
        header_tmp = KernACPIData.kb_ACPI_SDT_Entry[i];

        if ( header_tmp->id != id )	continue;

        header_handler->phys_addr = header_tmp->pa;
        header_handler->size = header_tmp->size;
        AROS_UFC1 ( IPTR, header_handler->h_Entry, AROS_UFCA(struct Hook *, header_handler, A0) );
        
        count++;
    }

    return count;
}

/**********************************************************/
IPTR core_ACPITableSDTGet(struct acpi_table_rsdp * RSDP)
{
    struct acpi_table_header  *header = NULL;
    unsigned int		      i, id = 0;

    if (!RSDP) return NULL;

    /* First check XSDT (but only on ACPI 2.0-compatible systems) */
    if ((RSDP->revision >= 2) && (((struct acpi20_table_rsdp*)RSDP)->xsdt_address))
    {
        rkprintf("[Kernel] core_ACPITableSDTGet: Checking XSDT @ %p\n", RSDP->rsdt_address);

        struct acpi_table_xsdt	*XSDT = NULL;

        KernACPIData.kb_ACPI_SDT_Phys = ((struct acpi20_table_rsdp *)RSDP)->xsdt_address;

        XSDT = (struct acpi_table_xsdt *)KernACPIData.kb_ACPI_SDT_Phys;
        if (!XSDT)
        {
            rkprintf("[Kernel] core_ACPITableSDTGet: ERROR - bad XSDT pointer\n");
            return NULL;
        }
        header = &XSDT->header;

        if (strncmp( header->signature, "XSDT", 4))
        {
            rkprintf("[Kernel] core_ACPITableSDTGet: ERROR - bad XSDT signature [header @ %p, sig='%4.4s']\n",header,header->signature);
            return NULL;
        }

        if (core_ACPITableChecksum(header, header->length))
        {
            rkprintf("[Kernel] core_ACPITableSDTGet: ERROR - XSDT checksum invalid\n");
            return NULL;
        }

        KernACPIData.kb_ACPI_SDT_Count = (header->length - sizeof(struct acpi_table_header)) >> 3;
        if (KernACPIData.kb_ACPI_SDT_Count > ACPI_MAX_TABLES) 
        {
            rkprintf("[Kernel] core_ACPITableSDTGet: WARNING - Truncated %lu XSDT entries\n", (KernACPIData.kb_ACPI_SDT_Count - ACPI_MAX_TABLES));
            KernACPIData.kb_ACPI_SDT_Count = ACPI_MAX_TABLES;
        }

        for (i = 0; i < KernACPIData.kb_ACPI_SDT_Count; i++)
        {   
            KernACPIData.kb_ACPI_SDT_Entry[i] = &KernACPISDTEntries[i];
            ((struct acpi_table_sdt *)KernACPIData.kb_ACPI_SDT_Entry[i])->pa = (unsigned long)XSDT->entry[i];
        }
    }
    else if (RSDP->rsdt_address)
    {
        /* If there is no XSDT, then check RSDT */
        rkprintf("[Kernel] core_ACPITableSDTGet: Checking RSDT @ %p\n", RSDP->rsdt_address);

        struct acpi_table_rsdt	*RSDT = NULL;

        KernACPIData.kb_ACPI_SDT_Phys = RSDP->rsdt_address;

        RSDT = (struct acpi_table_rsdt *)KernACPIData.kb_ACPI_SDT_Phys;
        if (!RSDT)
        {
            rkprintf("[Kernel] core_ACPITableSDTGet: ERROR - bad RSDT pointer\n");
            return NULL;
        }
        header = &RSDT->header;

        if (strncmp(header->signature, "RSDT", 4))
        {
            rkprintf("[Kernel] core_ACPITableSDTGet: ERROR - bad RSDT signature [header @ %p, sig='%4.4s']\n",header,header->signature);
            return NULL;
        }

        if (core_ACPITableChecksum(header, header->length))
        {
            rkprintf("[Kernel] core_ACPITableSDTGet: ERROR - RSDT checksum invalid\n");
            return NULL;
        }

        KernACPIData.kb_ACPI_SDT_Count = (header->length - sizeof(struct acpi_table_header)) >> 2;
        if (KernACPIData.kb_ACPI_SDT_Count > ACPI_MAX_TABLES)
        {
            rkprintf("[Kernel] core_ACPITableSDTGet: WARNING - Truncated %lu RSDT entries\n", (KernACPIData.kb_ACPI_SDT_Count - ACPI_MAX_TABLES));
            KernACPIData.kb_ACPI_SDT_Count = ACPI_MAX_TABLES;
        }

        for (i = 0; i < KernACPIData.kb_ACPI_SDT_Count; i++)
        {   
            KernACPIData.kb_ACPI_SDT_Entry[i] = &KernACPISDTEntries[i];
            ((struct acpi_table_sdt *)KernACPIData.kb_ACPI_SDT_Entry[i])->pa = (unsigned long)RSDT->entry[i];
        }
    }
    else 
    {
        rkprintf("[Kernel] core_ACPITableSDTGet: No System Description Table (RSDT/XSDT) specified in RSDP\n");
        return NULL;
    }

    core_ACPITableDump(header, KernACPIData.kb_ACPI_SDT_Phys);

    for (i = 0; i < KernACPIData.kb_ACPI_SDT_Count; i++) 
    {
        header = (struct acpi_table_header *)(((struct acpi_table_sdt*)KernACPIData.kb_ACPI_SDT_Entry[i])->pa);
        if (header == NULL)
            continue;

        core_ACPITableDump(header, ((struct acpi_table_sdt *)KernACPIData.kb_ACPI_SDT_Entry[i])->pa);

        if (core_ACPITableChecksum(header, header->length))
        {
            rkprintf("[Kernel] core_ACPITableSDTGet: WARNING - SDT %d Checksum invalid\n", i);
            continue;
        }

        ((struct acpi_table_sdt *)KernACPIData.kb_ACPI_SDT_Entry[i])->size = header->length;

        /* Start at 1 to skip "unknown" */
        for (id = 1; id < ACPI_TABLE_COUNT; id++)
        {
            if (!strncmp((char *)&header->signature, KernACPISDTSigs[id], sizeof(header->signature)))
            {
                ((struct acpi_table_sdt *)KernACPIData.kb_ACPI_SDT_Entry[i])->id = id;
            }
        }
    }

    /*  We want to print the DSDT (because this is what people usually blacklist against),
        but it is *not* in the RSDT.  We don't know its phys_addr, so just print 0.    */
    header = NULL;
    if (!(core_ACPITableHeaderEarly(ACPI_DSDT, &header)))
    {
        core_ACPITableDump(header, 0);
    }

    return KernACPIData.kb_ACPI_SDT_Phys;
}

/**********************************************************/
int core_ACPIIsBlacklisted()
{
    int i = 0;
    int blacklisted = 0;
    struct acpi_table_header *table_header;

    while (_ACPI_OEMBlacklist[i].oem_id[0] != '\0')
    {
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

        if ((_ACPI_OEMBlacklist[i].oem_revision_match == all_versions) ||
           ((_ACPI_OEMBlacklist[i].oem_revision_match == less_than_or_equal) &&
            (table_header->oem_revision <= _ACPI_OEMBlacklist[i].oem_revision)) ||
           ((_ACPI_OEMBlacklist[i].oem_revision_match == greater_than_or_equal) &&
            (table_header->oem_revision >= _ACPI_OEMBlacklist[i].oem_revision)) ||
           ((_ACPI_OEMBlacklist[i].oem_revision_match == equal) &&
            (table_header->oem_revision == _ACPI_OEMBlacklist[i].oem_revision)))
        {
            rkprintf("[Kernel] core_ACPIIsBlacklisted: ERROR - Vendor \"%6.6s\" System \"%8.8s\" Revision 0x%x has a known ACPI BIOS problem.\n",
                _ACPI_OEMBlacklist[i].oem_id, _ACPI_OEMBlacklist[i].oem_table_id, _ACPI_OEMBlacklist[i].oem_revision);
            rkprintf("[Kernel] core_ACPIIsBlacklisted: ERROR - Reason: %s. This is a %s error\n",
                _ACPI_OEMBlacklist[i].blacklist_reason, ( _ACPI_OEMBlacklist[i].blacklist_critical ? "non-recoverable" : "recoverable" ));

            blacklisted = _ACPI_OEMBlacklist[i].blacklist_critical;
            break;
        }
        else i++;
    }

    return blacklisted;
}

ULONG core_ACPIInitialise()
{
    int                         result = 0;
    rkprintf("[Kernel] core_ACPIInitialise()\n");

    struct acpi_table_hook ACPI_TableParse_MADT_hook = {
        .h_Entry = (APTR)ACPI_hook_Table_MADT_Parse
    };

    struct acpi_table_hook ACPI_TableParse_LAPIC_Addr_Ovr_hook = {
        .h_Entry = (APTR)ACPI_hook_Table_LAPIC_Addr_Ovr_Parse
    };

    struct acpi_table_hook ACPI_TableParse_LAPIC_hook = {
        .h_Entry = (APTR)ACPI_hook_Table_LAPIC_Parse
    };

    struct acpi_table_hook ACPI_TableParse_LAPIC_NMI_hook = {
        .h_Entry = (APTR)ACPI_hook_Table_LAPIC_NMI_Parse
    };

    struct acpi_table_hook ACPI_TableParse_IOAPIC_hook = {
        .h_Entry = (APTR)ACPI_hook_Table_IOAPIC_Parse
    };

    struct acpi_table_hook ACPI_TableParse_Int_Src_Ovr_hook = {
        .h_Entry = (APTR)ACPI_hook_Table_Int_Src_Ovr_Parse
    };

    struct acpi_table_hook ACPI_TableParse_NMI_Src_hook = {
        .h_Entry = (APTR)ACPI_hook_Table_NMI_Src_Parse
    };

    struct acpi_table_hook ACPI_TableParse_HPET_hook = {
        .h_Entry = (APTR)ACPI_hook_Table_HPET_Parse
    };

    /*  MADT : If it exists, parse the Multiple APIC Description Table "MADT", 
        This table provides platform SMP configuration information [the successor to MPS tables]	*/
    result = core_ACPITableParse( ACPI_APIC, &ACPI_TableParse_MADT_hook);
    rkprintf("[Kernel] core_ACPIInitialise: core_ACPITableParse(ACPI_APIC) returned %p\n", result);
    if ( !result ) return NULL;
    else if ( result < 0 )
    {
        rkprintf("[Kernel] core_ACPIInitialise: ERROR - Error parsing MADT\n");
        return result;
    }
    else if ( result > 1 )
    {
        rkprintf("[Kernel] core_ACPIInitialise: WARNING - Multiple MADT tables exist\n");
    }

    /*  Local APIC : The LAPIC address is obtained from the MADT (32-bit value)
        and (optionally) overriden by a LAPIC_ADDR_OVR entry (64-bit value). */
    result = core_ACPITableMADTParse( ACPI_MADT_LAPIC_ADDR_OVR, &ACPI_TableParse_LAPIC_Addr_Ovr_hook );
    rkprintf("[Kernel] core_ACPIInitialise: core_ACPITableMADTParse(ACPI_MADT_LAPIC_ADDR_OVR) returned %p\n", result);
    if ( result < 0 )
    {
        rkprintf("[Kernel] core_ACPIInitialise: ERROR - Error parsing LAPIC address override entry\n");
        return result;
    }

    result = core_ACPITableMADTParse( ACPI_MADT_LAPIC, &ACPI_TableParse_LAPIC_hook);
    rkprintf("[Kernel] core_ACPIInitialise: core_ACPITableMADTParse(ACPI_MADT_LAPIC) returned %p\n", result);
    if ( !result )
    { 
#warning "TODO: Cleanup to allow fallback to MPS.."
        rkprintf("[Kernel] core_ACPIInitialise: ERROR - No LAPIC entries present\n");
        return NULL;
    }
    else if (result < 0)
    {
#warning "TODO: Cleanup to allow fallback to MPS.."
        rkprintf("[Kernel] core_ACPIInitialise: ERROR - Error parsing LAPIC entry\n");
        return result;
    }

    result = core_ACPITableMADTParse( ACPI_MADT_LAPIC_NMI, &ACPI_TableParse_LAPIC_NMI_hook);
    rkprintf("[Kernel] core_ACPIInitialise: core_ACPITableMADTParse(ACPI_MADT_LAPIC_NMI) returned %p\n", result);
    if ( result < 0 )
    {
#warning "TODO: Cleanup to allow fallback to MPS.."
        rkprintf("[Kernel] core_ACPIInitialise: ERROR - Error parsing LAPIC NMI entry\n");
        return result;
    }

    KernACPIData.kb_ACPI_LAPIC = 1;

    /*  I/O APIC : ACPI interpreter is required to complete interrupt setup,
        so if it is off, don't enumerate the io-apics with ACPI.
        If MPS is present, it will handle them, otherwise the system will stay in PIC mode */
    if (KernACPIData.kb_ACPI_Disabled || !KernACPIData.kb_ACPI_IRQ) return 1;

    result = core_ACPITableMADTParse(ACPI_MADT_IOAPIC, &ACPI_TableParse_IOAPIC_hook);
    rkprintf("[Kernel] core_ACPIInitialise: core_ACPITableMADTParse(ACPI_MADT_IOAPIC) returned %p\n", result);
    if (!result)
    { 
        rkprintf("[Kernel] core_ACPIInitialise: ERROR - No IOAPIC entries present\n");
        return NULL;
    }
    else if (result < 0)
    {
        rkprintf("[Kernel] core_ACPIInitialise: ERROR - Error parsing IOAPIC entry\n");
        return result;
    }

    /* Build a default routing table for legacy (ISA) interrupts. */
#warning "TODO: implement legacy irq config.."
//	mp_config_acpi_legacy_irqs();

    result = core_ACPITableMADTParse(ACPI_MADT_INT_SRC_OVR, &ACPI_TableParse_Int_Src_Ovr_hook);
    rkprintf("[Kernel] core_ACPIInitialise: core_ACPITableMADTParse(ACPI_MADT_INT_SRC_OVR) returned %p\n", result);
    if (result < 0)
    {
#warning "TODO: Cleanup to allow fallback to MPS.."
        rkprintf("[Kernel] core_ACPIInitialise: ERROR - Error parsing interrupt source overrides entry\n");
        return result;
    }

    result = core_ACPITableMADTParse(ACPI_MADT_NMI_SRC, &ACPI_TableParse_NMI_Src_hook);
    rkprintf("[Kernel] core_ACPIInitialise: core_ACPITableMADTParse(ACPI_MADT_NMI_SRC) returned %p\n", result);
    if (result < 0)
    {
#warning "TODO: Cleanup to allow fallback to MPS.."
        rkprintf("[Kernel] core_ACPIInitialise: ERROR - Error parsing NMI SRC entry\n");
        return result;
    }

    KernACPIData.kb_APIC_IRQ_Model = ACPI_IRQ_MODEL_IOAPIC;

    KernACPIData.kb_ACPI_IOAPIC = 1;

    if ( KernACPIData.kb_ACPI_LAPIC && KernACPIData.kb_ACPI_IOAPIC )
    {
        KernACPIData.kb_SMP_Config = 1;
        rkprintf("[Kernel] core_ACPIInitialise: SMP Configured by APIC\n");
#warning "TODO: implement check for clustered apic's.."
        //core_APICClusteredCheck();
    }

    core_ACPITableParse( ACPI_HPET, &ACPI_TableParse_HPET_hook );

    return NULL;
} /* core_ACPIInit */


/**********************************************************/
IPTR core_ACPIRootSystemDescriptionPointerScan(IPTR scan_start, IPTR scan_length)
{
    unsigned long		                scan_offset;
    char                                *scan_ptr;

    /* Scan for the Root System Description Pointer signature
       on 16-byte boundaries of the physical memory region */
    for (scan_offset = 0; scan_offset < scan_length; scan_offset += 16)
    {
        scan_ptr = scan_start + scan_offset;
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
        return scan_ptr;
    }

    return NULL;
}

/* Attempt to locate the ACPI Root System Description Pointer */
IPTR core_ACPIRootSystemDescriptionPointerLocate()
{
    IPTR RSDP_PhysAddr = 0;

    /* Search the first Kilobyte of the Extended BIOS Data Area */
    if ((RSDP_PhysAddr = core_ACPIRootSystemDescriptionPointerScan(0x00000000, 0x00000400)) == NULL)
    {
        /* Search in BIOS ROM address space */
        if ((RSDP_PhysAddr = core_ACPIRootSystemDescriptionPointerScan(0x000E0000, 0x000FFFFF)) != NULL)
        {
            rkprintf("[Kernel] core_ACPIRootSystemDescriptionPointerLocate: RSDP found in BIOS ROM space @ %p\n", RSDP_PhysAddr);
        }
    }
    else
    {
        rkprintf("[Kernel] core_ACPIRootSystemDescriptionPointerLocate: RSDP found in EBDA @ %p\n", RSDP_PhysAddr);
    }

    return RSDP_PhysAddr;
}

/**********************************************************/
int core_ACPITableChecksum(void * table_pointer, unsigned long table_length)
{
    UBYTE   	                        *tmp_pointer = (UBYTE *)table_pointer;
    unsigned long		                remains = table_length;
    unsigned long		                sum     = 0;

    if (!tmp_pointer || !table_length) return NULL;

    while (remains--) sum += *tmp_pointer++;

    return (sum & 0xFF);
}

/**********************************************************/
IPTR core_ACPIProbe()
{
    struct acpi_table_rsdp	            *RSDP;
    IPTR             	                *RSDP_PhysAddr;
    int			                        checksum = 0;

    rkprintf("[Kernel] core_ACPIProbe()\n");

    /* Locate the Root System Description Table (RSDP) */
    RSDP_PhysAddr = core_ACPIRootSystemDescriptionPointerLocate();
    if (RSDP_PhysAddr != NULL)
    {
        RSDP = RSDP_PhysAddr;
        rkprintf("[Kernel] core_ACPIProbe: Root System Description Pointer @ %p\n", RSDP);
        rkprintf("[Kernel] core_ACPIProbe: Root System Description Pointer [ v%3.3d '%6.6s' ]\n", RSDP->revision, RSDP->oem_id);

        if (RSDP->revision >= 2) checksum = core_ACPITableChecksum(RSDP, ((struct acpi20_table_rsdp *)RSDP)->length);
        else checksum = core_ACPITableChecksum(RSDP, sizeof(struct acpi_table_rsdp));

        if (checksum)
        {
            rkprintf("[Kernel] core_ACPIProbe: ERROR: Invalid RSDP checksum (%d)\n", checksum);
            return NULL;
        }

        /* Locate and map the System Description table (RSDT/XSDT) */
        core_ACPITableSDTGet(RSDP);
        
        if (core_ACPIIsBlacklisted())
        {
            rkprintf("[Kernel] core_ACPIProbe: WARNING - BIOS listed in blacklist, disabling ACPI support\n");
            return NULL;
        }
    }
    else
    {
        rkprintf("[Kernel] core_ACPIProbe: Unable to locate RSDP - no ACPI\n");
    }

    return RSDP;
}
