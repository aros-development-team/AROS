/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH4(struct LocalContextItem *, AllocLocalItem,

/*  SYNOPSIS */
	AROS_LHA(LONG, type, D0),
	AROS_LHA(LONG, id, D1),
	AROS_LHA(LONG, ident, D2),
	AROS_LHA(ULONG, dataSize, D3),

/*  LOCATION */
	struct Library *, IFFParseBase, 31, IFFParse)

/*  FUNCTION
	Allocates and initializes a LocalContextItem structure. It also allocates
	dataSize user data. User data can be accesseed via LocalItemData function.
	This is the only way to allocate such a item, since the item contains private
	fields. Of course programmers should assume NOTHING about this private
	fields.


    INPUTS
	type, id   - Longword identifications values.
	ident	   - Longword identifier for class of item.
	dataSize      -  Size of a user data area that will be allocated by this funcyion.

    RESULT
	item	  - A initialized LocalContextItem structure.

    NOTES
	Changed dataSize parameter to ULONG, negative-sized memory allocations are undefined.


    EXAMPLE

    BUGS
	See notes.

    SEE ALSO
	FreeLocalItem(), LocalItemData(), StoreLocalItem(), StoreItemInContext(),
	SetLocalItemPurge()

    INTERNALS

    HISTORY
  27-11-96    digulla automatically created from
	  iffparse_lib.fd and clib/iffparse_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    struct IntLocalContextItem *intlci;

    if
    (
	!(intlci = AllocMem
	    (
		sizeof (struct IntLocalContextItem),
		MEMF_ANY|MEMF_CLEAR
	    )
	)
    )
	return (FALSE);

    GetLCI(intlci)->lci_ID     = id;
    GetLCI(intlci)->lci_Type   = type;
    GetLCI(intlci)->lci_Ident  = ident;

    /* Only allocate user date if dataSize > 0 */
    if (dataSize > 0)
    {
	if
	(
	    !(intlci->lci_UserData = AllocMem
		(
		    dataSize,
		    MEMF_ANY|MEMF_CLEAR
		)
	    )
	)
	{
	    FreeMem(intlci,sizeof (struct IntLocalContextItem));
	    return (FALSE);
	}

	/* Remember to set UserDataSize field for use in FreeLocalItem */
	intlci->lci_UserDataSize = dataSize;
    }
    else
    {
	/* If no dataSize, then we set then we do not have userdata */
	intlci->lci_UserData = NULL;
    }


    return ((struct LocalContextItem*)intlci);


    AROS_LIBFUNC_EXIT
} /* AllocLocalItem */
