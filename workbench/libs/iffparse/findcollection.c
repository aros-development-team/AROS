/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH3(struct CollectionItem *, FindCollection,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),
	AROS_LHA(LONG              , type, D0),
	AROS_LHA(LONG              , id, D1),

/*  LOCATION */
	struct Library *, IFFParseBase, 27, IFFParse)

/*  FUNCTION
	Returns a pointer to a list of CollectionItems the installed
	CollectionChunk of that type and ID.

    INPUTS
	iff   - pointer to an IFFHandle struct.
	type  - type code to search for.
	id    -  id code to search for.


    RESULT
	ci - pointer to the last collection chunk encountered
	     with pointers to previous ones.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CollectionChunk(), CollectionChunks()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    struct LocalContextItem *lci;

    DEBUG_FINDCOLLECTION(dprintf("FindCollection: iff 0x%lx type 0x%08lx (%c%c%c%c) id 0x%08lx (%c%c%c%c)\n",
			    iff, type, dmkid(type), id, dmkid(id)));

    if (!(lci = FindLocalItem(
		iff,
		type,
		id,
		IFFLCI_COLLECTION)))
    {
	DEBUG_FINDCOLLECTION(dprintf("FindCollection: return NULL\n"));
	return NULL;
    }

    /* The userdata of the found LCI contains a struct CIPtr,
    which is just a pointer to the first CollectionItem  */

   return
   (
       ((struct CIPtr*)LocalItemData(lci) )->FirstCI
   );


    AROS_LIBFUNC_EXIT
} /* FindCollection */
