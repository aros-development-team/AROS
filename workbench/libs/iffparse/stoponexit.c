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

	AROS_LH3(LONG, StopOnExit,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),
	AROS_LHA(LONG              , type, D0),
	AROS_LHA(LONG              , id, D1),

/*  LOCATION */
	struct Library *, IFFParseBase, 25, IFFParse)

/*  FUNCTION
	Inserts an exit handler for the given type and id, that will cause the parser
	to stop when such a chunk is left.

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
	ParseIFF()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    DEBUG_STOPONEXIT(dprintf("StopOnExit: iff 0x%lx type 0x%08lx (%c%c%c%c) id 0x%08lx (%c%c%c%c)\n",
			    iff, type, dmkid(type), id, dmkid(id)));

#if DEBUG
    bug ("StopOnExit (iff=%p, type=%c%c%c%c, id=%c%c%c%c)\n",
	iff,
	dmkid(type),
	dmkid(id)
    );
#endif

    /* Install an ExitHandler */
    return
    (
	ExitHandler
	(
	    iff,
	    type,
	    id,
	    IFFSLI_TOP,
	    &(IPB(IFFParseBase)->exitcontexthook),
	    iff
	)
    );

    AROS_LIBFUNC_EXIT
} /* StopOnExit */


/**********************/
/* Exit entry-handler */
/**********************/

LONG ExitContextFunc
(
    struct Hook *hook,
     APTR         obj,
     APTR         p
)
{
    return (IFFERR_EOC);
}
