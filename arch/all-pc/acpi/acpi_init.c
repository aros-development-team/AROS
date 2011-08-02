#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <resources/acpi.h>
#include <resources/efi.h>
#include <proto/efi.h>
#include <proto/exec.h>

#include <string.h>

#include "acpi_intern.h"

static void *core_ACPIRootSystemDescriptionPointerScan(IPTR scan_start, IPTR scan_length)
{
    unsigned long scan_offset;
    unsigned char *scan_ptr;

    /* Scan for the Root System Description Pointer signature
       on 16-byte boundaries of the physical memory region */
    for (scan_offset = 0; scan_offset < scan_length; scan_offset += 16)
    {
        scan_ptr = (unsigned char *)scan_start + scan_offset;

	if (!memcmp(scan_ptr, "RSD PTR", 8))
	{
	    /* We have the signature, let's check the checksum*/ 
	    if (!acpi_CheckSum(scan_ptr, (((struct ACPI_TABLE_TYPE_RSDP *)scan_ptr)->revision < 2) ? 20 : 36))
	    {
	        /* RSDP located, return its address */
	        return scan_ptr;
	    }
	    D(bug("[ACPI] Wrong RDSP checksum at 0x%p\n", scan_ptr));
	}
    }

    return NULL;
}

static const uuid_t acpi_20_guid = ACPI_20_TABLE_GUID;
static const uuid_t acpi_10_guid = ACPI_TABLE_GUID;

/* Attempt to locate the ACPI Root System Description Pointer */
static void *core_ACPIRootSystemDescriptionPointerLocate()
{
    struct EFIBase *EFIBase;
    APTR ssp;
    struct ACPI_TABLE_TYPE_RSDP *RSDP_PhysAddr;

    EFIBase = OpenResource("efi.resource");
    D(bug("[ACPI] efi.resource 0x%p\n", EFIBase));
    if (EFIBase)
    {
    	/* If we have EFI firmware, the best way to obtain RSDP is to ask it. */
    	RSDP_PhysAddr = EFI_FindConfigTable(&acpi_20_guid);
    	if (RSDP_PhysAddr)
    	{
    	    D(bug("[ACPI] Got RSDP 2.0 from EFI @ 0x%p\n", RSDP_PhysAddr));

    	    if (!memcmp(RSDP_PhysAddr, "RSD PTR ", 8) && !acpi_CheckSum(RSDP_PhysAddr, 36))
	 	return RSDP_PhysAddr;
	 	
	    D(bug("[ACPI] Broken RSDP\n"));
    	}

    	RSDP_PhysAddr = EFI_FindConfigTable(&acpi_10_guid);
    	if (RSDP_PhysAddr)
    	{
    	    D(bug("[ACPI] Got RSDP 1.0 from EFI @ 0x%p\n", RSDP_PhysAddr));

    	    if (!memcmp(RSDP_PhysAddr, "RSD PTR ", 8) && !acpi_CheckSum(RSDP_PhysAddr, 20))
	 	return RSDP_PhysAddr;

	    D(bug("[ACPI] Broken RSDP\n"));
    	}

    	/*
    	 * If there's no RSDP in EFI tables, we'll search for it, just in case.
    	 * However, to tell the truth, we are unlikely to find it. For example
    	 * on MacMini RSDP is located neither in EBDA nor in ROM space.
    	 * Specification Rev 4.0a explicitly says that on UEFI systems RSDP
    	 * pointer should be obtained from within EFI system table.
    	 */
    }

    /* 
     * Search the first Kilobyte of the Extended BIOS Data Area.
     * On x86-64 zero page is protected, on i386 it will someday be too.
     * We need to become a Supervisor to access it.
     */
    ssp = SuperState();
    RSDP_PhysAddr = core_ACPIRootSystemDescriptionPointerScan(0x00000000, 0x00000400);
    UserState(ssp);

    if (RSDP_PhysAddr != NULL)
    {
    	void *RSDP_Copy;

        D(bug("[ACPI] RSDP found in EBDA @ %p\n", RSDP_PhysAddr));

	/* Make a user-readable copy */
	RSDP_Copy = AllocMem(36, MEMF_ANY);
	if (RSDP_Copy)
	{
	    ssp = SuperState();
	    CopyMemQuick(RSDP_PhysAddr, RSDP_Copy, (RSDP_PhysAddr->revision < 2) ? 20 : 36);
	    UserState(ssp);

	    return RSDP_Copy;
	}
    }

    /* Search in BIOS ROM address space */
    if ((RSDP_PhysAddr = core_ACPIRootSystemDescriptionPointerScan(0x000E0000, 0x000FFFFF)) != NULL)
    {
        D(bug("[ACPI] RSDP found in BIOS ROM space @ %p\n", RSDP_PhysAddr));
        return RSDP_PhysAddr;
    }    

    return NULL;
}

/**********************************************************/
static int acpi_ParseSDT(struct ACPIBase *ACPIBase)
{
    struct ACPI_TABLE_TYPE_RSDP *RSDP = ACPIBase->ACPIB_RSDP_Addr;
    struct ACPI_TABLE_TYPE_RSDT	*RSDT = (APTR)(IPTR)RSDP->rsdt_address;
    struct ACPI_TABLE_TYPE_XSDT *XSDT = (RSDP->revision >= 2) ? (APTR)(IPTR)RSDP->xsdt_address : NULL;
    unsigned int i;

    D(bug("[ACPI] acpi_ParseSDT: ACPI v2 XSDT @ %p, ACPI v1 RSDT @ %p\n", XSDT, RSDT));

    if (!acpi_CheckTable(&XSDT->header, ACPI_MAKE_ID('X', 'S', 'D', 'T')))
    {
    	ACPIBase->ACPIB_SDT_Addr = &XSDT->header;

        ACPIBase->ACPIB_SDT_Count = (XSDT->header.length - sizeof(struct ACPI_TABLE_DEF_HEADER)) >> 3;
        D(bug("[ACPI] acpi_ParseSDT: XSDT size: %u entries\n", ACPIBase->ACPIB_SDT_Count));

	if (ACPIBase->ACPIB_SDT_Count == 0)
	{
	    /* ???? */
	    return 1;
	}

	/* Plus 1 in order to reserve one pointer for DSDT */
	ACPIBase->ACPIB_SDT_Entry = AllocMem((ACPIBase->ACPIB_SDT_Count + 1) * sizeof(APTR), MEMF_ANY);
	if (!ACPIBase->ACPIB_SDT_Entry)
	{
	    D(bug("[ACPI] Failed to allocate memory for XSDT entries!\n"));
	    return 0;
	}

	D(bug("[ACPI] acpi_ParseSDT: Copying Tables Start\n"));
        for (i = 0; i < ACPIBase->ACPIB_SDT_Count; i++)
        {
            ACPIBase->ACPIB_SDT_Entry[i] = (APTR)(IPTR)XSDT->entry[i];
	    D(bug("[ACPI] acpi_ParseSDT: Table %u Entry @ %p\n", i, ACPIBase->ACPIB_SDT_Entry[i]));
	}

	D(bug("[ACPI] acpi_ParseSDT: Copying Tables done!\n"));
	return 1;
    }

    D(bug("[ACPI] Broken (or no) XSDT, trying RSDT...\n"));

    /* If there is no (or damager) XSDT, then check RSDT */
    if (!acpi_CheckTable(&RSDT->header, ACPI_MAKE_ID('R', 'S', 'D', 'T')))
    {
	ACPIBase->ACPIB_SDT_Addr = &RSDT->header;

        ACPIBase->ACPIB_SDT_Count = (RSDT->header.length - sizeof(struct ACPI_TABLE_DEF_HEADER)) >> 2;
	D(bug("[ACPI] acpi_ParseSDT: RSDT size: %u entries\n", ACPIBase->ACPIB_SDT_Count));

	if (ACPIBase->ACPIB_SDT_Count == 0)
	{
	    /* ???? */
	    return 1;
	}

	/* Plus 1 in order to reserve one pointer for DSDT */
	ACPIBase->ACPIB_SDT_Entry = AllocMem((ACPIBase->ACPIB_SDT_Count + 1) * sizeof(APTR), MEMF_ANY);
	if (!ACPIBase->ACPIB_SDT_Entry)
	{
	    D(bug("[ACPI] Failed to allocate memory for RSDT entries!\n"));
	    return 0;
	}

	D(bug("[ACPI] acpi_ParseSDT: Copying Tables Start\n"));
	for (i = 0; i < ACPIBase->ACPIB_SDT_Count; i++)
        {   
            ACPIBase->ACPIB_SDT_Entry[i] = (APTR)(IPTR)RSDT->entry[i];
            D(bug("[ACPI] acpi_ParseSDT: Table %u Entry @ %p\n", i, ACPIBase->ACPIB_SDT_Entry[i]));
        }

        D(bug("[ACPI] acpi_ParseSDT: Copying Tables done!\n"));
        return 1;
    }

    D(bug("[ACPI] Broken (or no) RSDT\n"));

    return 0;
}

static int acpi_CheckSDT(struct ACPIBase *ACPIBase)
{
    struct ACPI_TABLE_TYPE_FADT *FADT = NULL;
    struct ACPI_TABLE_DEF_HEADER *header;
    unsigned int c = 0;
    unsigned int i;

    D(bug("[ACPI] Checking SDT Tables..\n"));

    for (i = 0; i < ACPIBase->ACPIB_SDT_Count; i++)
    {
    	header = ACPIBase->ACPIB_SDT_Entry[i];

        if (header == NULL)
        {
            D(bug("[ACPI] NULL pointer for table %u\n", i));
            continue;
        }

        D(bug("[ACPI] Table %d Header @ %p, sig='%4.4s'\n", i, header, &header->signature));

        if (acpi_CheckSum(header, header->length))
        {
            D(bug("[ACPI] WARNING - SDT %d Checksum invalid\n", i));
	    /* Throw away broken table */
            continue;
        }

	/* Pack our array, to simplify access to it */
        ACPIBase->ACPIB_SDT_Entry[c++] = header;

        if (header->signature == ACPI_MAKE_ID('F', 'A', 'C', 'P'))
            FADT = (struct ACPI_TABLE_TYPE_FADT *)header;
    }

    D(bug("[ACPI] Tables checked, %u of %u good\n", ACPIBase->ACPIB_SDT_Count, c));

    if (FADT)
    {
	/* Map the DSDT header via the pointer in the FADT */
    	header = (APTR)(IPTR)FADT->dsdt_addr;
    	D(bug("[ACPI] Got DSDT at 0x%p\n", header));

	if (!acpi_CheckTable(header, ACPI_MAKE_ID('D', 'S', 'D', 'T')))
    	{
    	    D(bug("[ACPI] DSDT checked, good\n"));
    	    /* We have reserved this pointer above, when allocated SDT array */
    	    ACPIBase->ACPIB_SDT_Entry[c++] = header;
    	}
    }

    /* Fix up tables count */
    ACPIBase->ACPIB_SDT_Count = c;
    return c;
}

static int acpi_Init(struct ACPIBase *ACPIBase)
{
    ACPIBase->ACPIB_RSDP_Addr = core_ACPIRootSystemDescriptionPointerLocate();
    if (!ACPIBase->ACPIB_RSDP_Addr)
    {
    	D(bug("[ACPI] No RSDP found, giving up...\n"));
    	return FALSE;
    }

    /* Parse SDT, canonicalize addresses of tables pointed to by it */
    if (!acpi_ParseSDT(ACPIBase))
    {    
    	D(bug("[ACPI] No valid System Description Table (SDT) specified in RSDP!\n"));
    	return FALSE;
    }

    /* Validate SDT tables */
    if (!acpi_CheckSDT(ACPIBase))
    {
    	D(bug("[ACPI] None of SDT entries are valid\n"));
    	return FALSE;
    }

    if (acpi_IsBlacklisted(ACPIBase))
    {
    	D(bug("[ACPI] Blacklisted\n"));
    	return FALSE;
    }

    return TRUE;
}

ADD2INITLIB(acpi_Init, 0);
