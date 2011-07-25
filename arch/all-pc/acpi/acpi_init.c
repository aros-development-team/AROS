#define DEBUG 2

#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/multiboot.h>
#include <aros/symbolsets.h>
#include <resources/acpi.h>
#include <proto/arossupport.h>
#include <proto/exec.h>
#include <proto/kernel.h>

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

    DB2(bug("[ACPI] Trying region at 0x%p, length %lu (0x%p)\n", scan_start, scan_length, scan_length));

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

    return 0;
}

/* Attempt to locate the ACPI Root System Description Pointer */
void *core_ACPIRootSystemDescriptionPointerLocate()
{
    APTR ssp;
    APTR RSDP_PhysAddr;
    APTR KernelBase;
    struct TagItem *tags;
    struct mb_mmap *mmap;
    IPTR len;

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

    D(bug("[ACPI] Seaching in memory map...\n"));

    /*
     * EXPERIMENTAL: Search in memory regions marked as MMAP_TYPE_ACPIDATA in the memory map.
     * This was figured out by using 'lsacpi' in GRUB 2 on IntelMac. EFI supplies a pointer
     * to RDSP, however we are using Multiboot protocol, which doesn't pass us any EFI data.
     */
    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
    {
        D(bug("[ACPI] kernel.resource failed to open?!\n"));
    	return NULL;
    }

    tags = KrnGetBootInfo();
    mmap = (struct mb_mmap *)LibGetTagData(KRN_MMAPAddress, 0, tags);
    len  = LibGetTagData(KRN_MMAPLength, 0, tags);

    if (!mmap || !len)
    {
    	D(bug("[ACPI] No memory map supplied by the bootstrap\n"));
    	return NULL;
    }

    while (len >= sizeof(struct mb_mmap))
    {
    	if (mmap->type == MMAP_TYPE_ACPIDATA)
    	{
    	    RSDP_PhysAddr = core_ACPIRootSystemDescriptionPointerScan(mmap->addr, mmap->len);
    	    if (RSDP_PhysAddr)
    	    {
    	    	D(bug("[ACPI] RSDP found in ACPI DATA area in memory map @ 0x%p\n", RSDP_PhysAddr));
		return RSDP_PhysAddr;
	    }
	}

        len -= mmap->size + 4;
        mmap = (struct mb_mmap *)(mmap->size + (unsigned long)mmap + 4);
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
