/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH3(struct StoredProperty *, FindProp,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),
	AROS_LHA(LONG              , type, D0),
	AROS_LHA(LONG              , id, D1),

/*  LOCATION */
	struct Library *, IFFParseBase, 26, IFFParse)

/*  FUNCTION
	Searches for a StoredProperty that is valid in the given context.
	Property chunks are automatically stored by ParseIFF() when pre-declared
	by PropChunk() or PropChunks(). The returned storedproperty contains
	a pointer to the data in the chunk.

    INPUTS
	iff    - a pointer to a an IFFHandle struct.
	type  - type code of property to search for.
	id    -  id code of property to search for.

    RESULT
	sp    - pointer to a storedproperty if found, NULL if none are found.

    NOTES

    EXAMPLE

    BUGS


    SEE ALSO
	PropChunk(), PropChunks()

    INTERNALS

    HISTORY
  27-11-96    digulla automatically created from
	  iffparse_lib.fd and clib/iffparse_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    struct LocalContextItem *lci;

    DEBUG_FINDPROP(dprintf("FindProp: iff 0x%lx type 0x%08lx (%.4s) id 0x%08lx (%.4s)\n",
			    iff, type, &type, id, &id));

    if
    (
	!(lci = FindLocalItem
	    (
		iff,
		type,
		id,
		IFFLCI_PROP
	    )
	)
    )
	return (NULL);

    /* The userdata of the found LCI is the StoredProperty */

    return ( LocalItemData(lci) );

    AROS_LIBFUNC_EXIT
} /* FindProp */
