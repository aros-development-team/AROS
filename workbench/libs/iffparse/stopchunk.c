/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH3(LONG, StopChunk,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),
	AROS_LHA(LONG              , type, D0),
	AROS_LHA(LONG              , id, D1),

/*  LOCATION */
	struct Library *, IFFParseBase, 21, IFFParse)

/*  FUNCTION
	Inserts an entry handler for the given type and id, that will cause the parser
	to stop when such a chunk is entered.

    INPUTS
	 iff   - Pointer to IFFHandle struct. (does not need to be open).
	type  - IFF chunk type declarator for chunk to stop at.
	id    -  IFF chunk id identifier for chunk to stop at.

    RESULT
	error  -  0 if successfull, IFFERR_#? otherwise.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	StopChunks(), ParseIFF()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    DEBUG_STOPCHUNK(dprintf("StopChunk: iff 0x%lx type 0x%08lx (%c%c%c%c) id 0x%08lx (%c%c%c%c)\n",
			    iff, type, dmkid(type), id, dmkid(id)));

    /* Install an EntryHandler */
    return
    (
	EntryHandler
	(
	    iff,
	    type,
	    id,
	    IFFSLI_TOP,
	    &(IPB(IFFParseBase)->stophook),
	    iff
	)
    );

    AROS_LIBFUNC_EXIT
} /* StopChunk */
