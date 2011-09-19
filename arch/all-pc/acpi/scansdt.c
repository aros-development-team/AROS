#include <resources/acpi.h>
#include <utility/hooks.h>

/*****************************************************************************

    NAME */
#include <proto/acpi.h>

AROS_LH3(ULONG, ACPI_ScanSDT,

/*  SYNOPSIS */
	AROS_LHA(ULONG, id, D0),
	AROS_LHA(const struct Hook *, hook, A0),
	AROS_LHA(APTR, userdata, A1),

/*  LOCATION */
	struct ACPIBase *, ACPIBase, 5, Acpi)

/*  FUNCTION
	Scan multiple system description tables with the given signature.

    INPUTS
    	id    	 - a signature of the table(s) to scan. Supply ACPI_ID_ALL
		   to enumerate all tables.
    	hook     - a hook to call. The hook will be called with 'object'
    	           argument set to table pointer, and 'paramPacket' set to
    	           'userdata' argument. You can pass a NULL here in order just
    	           to count the number of tables.
	userdata - a user-supplied data to pass to the hook.

    RESULT
	Total number of processed tables. The table is considered processed if
	either supplied hook returns nonzero value, or there's no hook supplied.

    NOTES
	Root tables, containing no data but only pointers to other tables
	(like RSDT and XSDT) are not included in the scan.

    EXAMPLE

    BUGS

    SEE ALSO
	ACPI_FindSDT

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG count = 0;
    struct ACPI_TABLE_DEF_HEADER *header;
    ULONG i;

    for (i = 0; i < ACPIBase->ACPIB_SDT_Count; i++)
    {
        header = ACPIBase->ACPIB_SDT_Entry[i];

    	if ((id == ACPI_ID_ALL) || (id == header->signature))
    	{
    	    if (hook)
    	    {
    	    	BOOL res = CALLHOOKPKT((struct Hook *)hook, header, userdata);

		if (res)
		    count++;
	    }
	    else
	    	count++;
	}
    }

    return count;

    AROS_LIBFUNC_EXIT
}
