#include <aros/debug.h>
#include <resources/acpi.h>

/*****************************************************************************

    NAME */
#include <proto/acpi.h>

AROS_LH1(APTR, ACPI_FindSDT,

/*  SYNOPSIS */
	AROS_LHA(ULONG, id, D0),

/*  LOCATION */
	struct ACPIBase *, ACPIBase, 1, Acpi)

/*  FUNCTION
	Locate a system description table with a given 4-character ID

    INPUTS
	id - a table ID (use ACPI_MAKE_ID() macro to get it)

    RESULT
	A pointer to a table or NULL if can't be found

    NOTES
    	Some systems may include duplicating tables, often it's MADT.
    	In this case a table with the latest revision number will
    	be returned.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ACPI_TABLE_DEF_HEADER *found = NULL;
    struct ACPI_TABLE_DEF_HEADER *header;
    unsigned int i;

    D(bug("[ACPI] acpi_LocateSDT('%4.4s')\n", &id));

    /* Locate the table. */
    for (i = 0; i < ACPIBase->ACPIB_SDT_Count; i++)
    {
        header = ACPIBase->ACPIB_SDT_Entry[i];

        if (header->signature == id)
        {
	    D(bug("[ACPI] acpi_LocateSDT: Table %4.4s pointer 0x%p rev %u\n", &header->signature, header, header->revision));

	    /*
	     * Some firmwares have a strange thing - they contain multiple tables with the same signature
	     * and different revisions. A common example is MADT table.
	     * Here we select a table with the latest revision. ACPI specs don't say anything clear
	     * about this.
	     * Such behavior is exposed for example by MacMini EFI.
	     */
	    if ((found == NULL) || (header->revision > found->revision))
	    	found = header;
        }
    }

    return found;
    
    AROS_LIBFUNC_EXIT
}
