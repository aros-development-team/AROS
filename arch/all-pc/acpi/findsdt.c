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

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

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

    D(bug("[ACPI] acpi_LocateSDT: WARNING - %4.4s not present\n", &id));
    return NULL;
    
    AROS_LIBFUNC_EXIT
}
