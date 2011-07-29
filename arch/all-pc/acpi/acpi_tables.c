#include <aros/debug.h>
#include <resources/acpi.h>
#include <proto/exec.h>

#include "acpi_intern.h"

/************************************************************************************************
                                    ACPI RELATED FUNCTIONS
 ************************************************************************************************/

static APTR acpi_LocateSDT(int id, struct ACPIBase *ACPIBase)
{
    struct ACPI_TABLE_DEF_HEADER *header;
    unsigned int i;

    D(bug("[ACPI] acpi_LocateSDT('%4.4s')\n", &id));

    /* Locate the table. */
    for (i = 0; i < ACPIBase->ACPIB_SDT_Count; i++)
    {
        header = ACPIBase->ACPIB_SDT_Entry[i];

        if (header->signature == id)
        {
	    D(bug("[ACPI] acpi_LocateSDT: Table %4.4s pointer 0x%p\n", &header->signature, header));
            return header;
        }
    }

    D(bug("[ACPI] acpi_LocateSDT: WARNING - %s not present\n", &id));
    return NULL;
}

/*
 * core_ACPITableHeaderEarly() .... used by both core_ACPIIsBlacklisted() and core_ACPITableSDTGet()
 */
APTR core_ACPITableHeaderEarly(int id, struct ACPIBase *ACPIBase)
{
    D(bug("[ACPI] core_ACPITableHeaderEarly('%4.4s')\n", &id));

    if (id == ACPI_MAKE_ID('D', 'S', 'D', 'T'))
    {
	/* Map the DSDT header via the pointer in the FADT */
    	struct ACPI_TABLE_TYPE_FADT *FADT = acpi_LocateSDT(ACPI_MAKE_ID('F', 'A', 'D', 'T'), ACPIBase);

	if (!FADT)
	    return NULL;

	D(bug("[ACPI] core_ACPITableHeaderEarly: DSDT pointer from FADT: 0x08X\n", FADT->dsdt_addr));
	return (APTR)(unsigned long)FADT->dsdt_addr;
    }
    else
    {
    	return acpi_LocateSDT(id, ACPIBase);
    }
}

unsigned char acpi_CheckTable(struct ACPI_TABLE_DEF_HEADER *header, ULONG id)
{
    if (header->signature != id)
    {
	D(bug("[ACPI] ERROR - bad signature [table @ %p, wanted '%4.4s', have '%4.4s']\n", header, &id, &header->signature));
    	return 1;
    }

    return acpi_CheckSum(header, header->length);
}
