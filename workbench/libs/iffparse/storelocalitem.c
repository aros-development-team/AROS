/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
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
	Which context node this is depends on the valu of the position
	argument:
	    IFFSLI_ROOT - insert into the default contextnode.
	    IFFSLI_PROP  -  insert into the node returned by FindPropContext().
	    IFFSLI_TOP	-  insert item into the current contextnode.


    INPUTS
	iff	   - pointer to IFFHandle struct.
	localItem  -  pointer to local context item.
	position  -  IFFSLI_ROOT, IFFSLI_PROP os IFFSLI_TOP.

    RESULT
	error	  -  0 if succesfull, IFFERR_#? otherwise.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	StoreItemInContext(), FindLocalItem(), EntryHandler(), ExitHandler()

    INTERNALS

    HISTORY
  27-11-96    digulla automatically created from
	  iffparse_lib.fd and clib/iffparse_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)


    LONG err = NULL;
    struct ContextNode *cn;

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

    return (err);


    AROS_LIBFUNC_EXIT
} /* StoreLocalItem */
