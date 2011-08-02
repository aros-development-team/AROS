#include <aros/debug.h>
#include <resources/acpi.h>
#include <proto/exec.h>

#include "acpi_intern.h"

/************************************************************************************************
                                    ACPI RELATED FUNCTIONS
 ************************************************************************************************/

unsigned char acpi_CheckTable(struct ACPI_TABLE_DEF_HEADER *header, ULONG id)
{
    /* First of all, pointer can't be NULL */
    if (!header)
    {
    	D(bug("[ACPI] ERROR - NULL pointer for '%4.4s'\n", &id));
    	return 1;
    }

    if (header->signature != id)
    {
	D(bug("[ACPI] ERROR - bad signature [table @ %p, wanted '%4.4s', have '%4.4s']\n", header, &id, &header->signature));
    	return 1;
    }

    return acpi_CheckSum(header, header->length);
}
