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

	AROS_LH3(LONG, StoreLocalItem,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle        *, iff, A0),
	AROS_LHA(struct LocalContextItem *, localItem, A1),
	AROS_LHA(LONG                     , position, D0),

/*  LOCATION */
	struct Library *, IFFParseBase, 36, IFFParse)

/*  FUNCTION
	Stores the given local context item in a context node.
	Which context node this is depends on the value of the position
	argument:
	    IFFSLI_ROOT - insert into the default contextnode.
	    IFFSLI_PROP  -  insert into the node returned by FindPropContext().
	    IFFSLI_TOP	-  insert item into the current contextnode.


    INPUTS
	iff	   - pointer to IFFHandle struct.
	localItem  -  pointer to local context item.
	position  -  IFFSLI_ROOT, IFFSLI_PROP or IFFSLI_TOP.

    RESULT
	error	  -  0 if succesfull, IFFERR_#? otherwise.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	StoreItemInContext(), FindLocalItem(), EntryHandler(), ExitHandler()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    LONG err = 0;
    struct ContextNode *cn;

#if DEBUG
    bug ("StoreLocalItem (iff=%p, localItem=%p, position=%d)\n",
	iff, localItem, position
    );
#endif

    switch (position)
    {
	case IFFSLI_ROOT:
	    /* Store in default context-node */
	    StoreItemInContext
	    (
		iff,
		localItem,
		RootChunk(iff)
	    );
	    break;

	case IFFSLI_TOP:
	    /* Store in top context-node */
	    StoreItemInContext
	    (
		iff,
		localItem,
		TopChunk(iff)
	    );
	    break;

	case IFFSLI_PROP:
	    /* Store in top FORM or LIST chunk */

	    cn = FindPropContext(iff);

	    if (!cn)
		err = IFFERR_NOSCOPE;
	    else
	    {
		StoreItemInContext
		(
		    iff,
		    localItem,
		    cn
		    );
	    }
	    break;


    }	 /* End of switch */

    ReturnInt ("StoreLocalItem",LONG,err);
    AROS_LIBFUNC_EXIT
} /* StoreLocalItem */
