/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
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

    HISTORY
  27-11-96    digulla automatically created from
	  iffparse_lib.fd and clib/iffparse_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

#if DEBUG
    bug ("StopOnExit (iff=%p, type=%c%c%c%c, id=%c%c%c%c)\n",
	iff,
	type>>24, type>>16, type>>8, type,
	id>>24, id>>16, id>>8, id
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
	    &IPB(IFFParseBase)->exitcontexthook,
	    NULL
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
