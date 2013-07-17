#include <aros/debug.h>
#include <libraries/uuid.h>

#include <string.h>

#ifdef DEBUG_UUID

static void PRINT_UUID(uuid_t *id)
{
    unsigned int i;

    bug("[EFI] Table UUID: 0x%08X-%04X-%04X-%02X%02X-",
        id->time_low, id->time_mid, id->time_hi_and_version,
    	id->clock_seq_hi_and_reserved, id->clock_seq_low);

    for (i = 0; i < sizeof(id->node); i++)
    	bug("%02X", id->node[i]);

    RawPutChar('\n');
}

#else
#define PRINT_UUID(id)
#endif

/*****************************************************************************

    NAME */
#include <proto/efi.h>

        AROS_LH1(void *, EFI_FindConfigTable,

/*  SYNOPSIS */
	AROS_LHA(const uuid_t *, Guid, A0),

/*  LOCATION */
	struct EFIBase *, EFIBase, 1, Efi)

/*  FUNCTION
	Locate a configuration table by GUID

    INPUTS
	Guid - a pointer to a GUID structure

    RESULT
	A pointer to a table or NULL if nothing found.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct EFI_Config *conf = EFIBase->System->ConfigTable;
    IPTR i;

    /* Safety */
    if (!conf)
    	return NULL;

    for (i = 0; i < EFIBase->System->NumEntries; i++)
    {
    	PRINT_UUID(&conf[i].VendorGUID);

    	if (!memcmp(&conf[i].VendorGUID, Guid, sizeof(uuid_t)))
    	{
    	    return conf[i].Table;
    	}
    }

    return NULL;

    AROS_LIBFUNC_EXIT
}
