/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>
#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH3(LONG, CollectionChunk,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),
	AROS_LHA(LONG              , type, D0),
	AROS_LHA(LONG              , id, D1),

/*  LOCATION */
	struct Library *, IFFParseBase, 23, IFFParse)

/*  FUNCTION
	Installs an entry handler with the given type and id, so that
	chunks encountered with the same type and id will be stored.
	This is quite like PropChunk(), but CollectionChunk() will
	store the contents of multiple chunks with the same type and id.
	To retrieve the stored collection of chunks one uses FindCollection().
	Remember: the collection is only valid inside the current property scope.

    INPUTS
	iff   - Pointer to IFFHandle struct. (does not need to be open).
	type  - IFF chunk type declarator for chunk to collect.
	id    -  IFF chunk id identifier for chunk to collect.

    RESULT
	error - 0 if successfulle. IFFERR_#? elsewise.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CollectionChunks(), FindCollection, PropChunk

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    DEBUG_COLLECTIONCHUNK(dprintf("CollectionChunk: iff 0x%lx type 0x%08lx (%c%c%c%c) id 0x%08lx (%c%c%c%c)\n",
			    iff, type, dmkid(type), id, dmkid(id)));

    return
    (
	EntryHandler
	(
	    iff,
	    type,
	    id,
	    IFFSLI_TOP,
	    &(IPB(IFFParseBase)->collectionhook),
	    iff
	)
    );

    AROS_LIBFUNC_EXIT
} /* CollectionChunk */
