#define DEBUG 1

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <resources/acpi.h>
#include <resources/efi.h>
#include <proto/efi.h>
#include <proto/exec.h>

#include <string.h>

/* Result of this must be zero */
static inline unsigned char acpi_CheckSum(unsigned char *j, unsigned int size)
{
    unsigned char *k = j + size;
    unsigned char sum = 0;

    for (; j < k; sum += *(j++));
    
    return sum;
}

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
void *core_ACPIRootSystemDescriptionPointerLocate()
{
    struct EFIBase *EFIBase;
    APTR ssp;
    APTR RSDP_PhysAddr;

    EFIBase = OpenResource("efi.resource");
    D(bug("[ACPI] efi.resource 0x%p\n", EFIBase));
    if (EFIBase)
    {
    	/* If we have EFI firmware, the best way to obtain RSDP is to ask it. */
    	RSDP_PhysAddr = EFI_FindConfigTable(&acpi_20_guid);
    	if (RSDP_PhysAddr)
    	{
    	    D(bug("[ACPI] Got RSDP 2.0 from EFI @ 0x%p\n", RSDP_PhysAddr));

    	    if (!acpi_CheckSum(RSDP_PhysAddr, 36))
	 	return RSDP_PhysAddr;
	 	
	    D(bug("[ACPI] Bad checksum\n"));
    	}

    	RSDP_PhysAddr = EFI_FindConfigTable(&acpi_10_guid);
    	if (RSDP_PhysAddr)
    	{
    	    D(bug("[ACPI] Got RSDP 1.0 from EFI @ 0x%p\n", RSDP_PhysAddr));

    	    if (!acpi_CheckSum(RSDP_PhysAddr, 20))
	 	return RSDP_PhysAddr;

	    D(bug("[ACPI] Bad checksum\n"));
    	}
    	
    	/*
    	 * If there's no RSDP in EFI tables, we'll search for it, just in case.
    	 * However, to tell the truth, we are unlikely to find it. For example
    	 * on MacMini RSDP is located neither in EBDA nor in ROM space.
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
        D(bug("[ACPI] RSDP found in EBDA @ %p\n", RSDP_PhysAddr));
        return RSDP_PhysAddr;
    }

    /* Search in BIOS ROM address space */
    if ((RSDP_PhysAddr = core_ACPIRootSystemDescriptionPointerScan(0x000E0000, 0x000FFFFF)) != NULL)
    {
        D(bug("[ACPI] RSDP found in BIOS ROM space @ %p\n", RSDP_PhysAddr));
        return RSDP_PhysAddr;
    }    

    return NULL;
}

static int acpi_Init(struct ACPIBase *ACPIBase)
{
    ACPIBase->ACPIB_RSDP_Addr = core_ACPIRootSystemDescriptionPointerLocate();
    if (!ACPIBase->ACPIB_RSDP_Addr)
    {
    	D(bug("[ACPI] No RSDP found, giving up...\n"));
    	return FALSE;
    }

    return TRUE;
}

ADD2INITLIB(acpi_Init, 0);
