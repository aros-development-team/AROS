/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#define AROS_ALMOST_COMPATIBLE
#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH1(void, FreeIFF,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),

/*  LOCATION */
	struct Library *, IFFParseBase, 9, IFFParse)

/*  FUNCTION
	Frees an IFFHandle struct previously allocated by AllocIFF.

    INPUTS
	iff - pointer to an IFFHandle struct.
    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AllocIFF(), CloseIFF()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)
    struct IntContextNode   * cn;
    struct LocalContextItem * node,
			    * nextnode;

    if (iff != NULL)
    {
	/*
	    We should free the LCIs of the default context-node
	    ( CollectionItems and such )
	*/
	cn = (struct IntContextNode*)RootChunk (iff);

	node = (struct LocalContextItem*)cn->cn_LCIList.mlh_Head;

	while ((nextnode = (struct LocalContextItem*)node->lci_Node.mln_Succ))
	{
	    PurgeLCI (node, IPB(IFFParseBase));

	    node = nextnode;
	}

	FreeMem (iff, sizeof (struct IntIFFHandle));
    }
    
    AROS_LIBFUNC_EXIT
} /* FreeIFF */
