#include <resources/acpi.h>
#include <utility/hooks.h>

struct HeaderData
{
    unsigned int id;
    unsigned int len;
};

static const struct HeaderData headers[] =
{
    {ACPI_MAKE_ID('A','P','I','C'), sizeof(struct ACPI_TABLE_TYPE_MADT)},
    {0, 0}
};

/*****************************************************************************

    NAME */
#include <proto/acpi.h>

	AROS_LH4(ULONG, ACPI_ScanEntries,

/*  SYNOPSIS */
	AROS_LHA(struct ACPI_TABLE_DEF_HEADER *, table, A0),
	AROS_LHA(WORD, type, D0),
	AROS_LHA(const struct Hook *, hook, A1),
	AROS_LHA(APTR, userdata, A2),

/*  LOCATION */
	struct ACPIBase *, ACPIBase, 2, Acpi)

/*  FUNCTION
	Scan entries with the given type in the given table.
	The supplied hook will be called for each entry 

    INPUTS
    	table    - a pointer to a table to scan
    	type     - type of entries needed, or ACPI_ENTRY_TYPE_ALL to
    		    enumerate all entries
    	hook     - a hook to call. The hook will be called with 'object'
    	           argument set to entry pointer, and 'paramPacket' set to
    	           'userdata' argument. You can pass a NULL here in order just
    	           to count the number of entries.
	userdata - a user-supplied data to pass to the hook.

    RESULT
	Total number of processed entries. The entry is considered processed if
	either supplied hook returns nonzero value, or there's no hook supplied.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ACPI_TABLE_DEF_ENTRY_HEADER *entry = NULL;
    ULONG count = 0;
    const struct HeaderData *hdr;
    unsigned long end;

    for (hdr = headers; hdr->id; hdr++)
    {
        if (hdr->id == table->signature)
        {
            /* First entry follows table header */
	    entry = (APTR)table + hdr->len;
            break;
        }
    }

    if (!entry)
    {
    	/* The given table doesn't have array of entries inside */
    	return 0;
    }

    /* Get end of the table */
    end = (unsigned long)table + table->length;

    /* Parse all entries looking for a match. */
    while (((unsigned long)entry) < end)
    {
        if ((type == ACPI_ENTRY_TYPE_ALL) || (type == entry->type))
        {
            if (hook)
            {
            	BOOL res = CALLHOOKPKT((struct Hook *)hook, entry, userdata);

            	if (res)
	    	    count++;
	    }
	    else
	    	count++;
        }
        entry = (struct ACPI_TABLE_DEF_ENTRY_HEADER *)((unsigned long)entry + entry->length);
    }

    return count;

    AROS_LIBFUNC_EXIT
}
