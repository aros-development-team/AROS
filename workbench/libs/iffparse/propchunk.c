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

	AROS_LH3(LONG, PropChunk,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),
	AROS_LHA(LONG              , type, D0),
	AROS_LHA(LONG              , id, D1),

/*  LOCATION */
	struct Library *, IFFParseBase, 19, IFFParse)

/*  FUNCTION
	Installs an entry handler for chunks with the given type and id.
	When such  chunk is encoutered, the entry handler will insert
	a StoredProperty in the proper context.
	A stored property chunk returned by FindProp() will be the valid property
	for the current context.

    INPUTS
	iff    - pointer to IFFHandle struct.
	type  -  type code for chunk to declare.
	id    -  identifier for chunk to declare.

    RESULT
	error  - 0 if successfull, IFFERR_#? otherwise.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	PropChunks(), FindProp(), CollectionChunk()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    DEBUG_PROPCHUNK(dprintf("PropChunk: iff 0x%lx type 0x%08lx (%c%c%c%c) id 0x%08lx (%c%c%c%c)\n",
			    iff, type, dmkid(type), id, dmkid(id)));

    ReturnInt
    (
	"PropChunk",
	LONG,
	EntryHandler
	(
	    iff,
	    type,
	    id,
	    IFFSLI_TOP,
	    &(IPB(IFFParseBase)->prophook),
	    iff
	)
    );

    AROS_LIBFUNC_EXIT
} /* PropChunk */
