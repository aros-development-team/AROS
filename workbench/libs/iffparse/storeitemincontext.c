/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH3(void, StoreItemInContext,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle        *, iff, A0),
	AROS_LHA(struct LocalContextItem *, localItem, A1),
	AROS_LHA(struct ContextNode      *, contextNode, A2),

/*  LOCATION */
	struct Library *, IFFParseBase, 37, IFFParse)

/*  FUNCTION
	Stores the given local context item into the given context node.
	If a LCI with the some id, type and class identifier allready exists
	in the context node, the old one will be purged, and the new one
	inserted.

    INPUTS
	iff	    - pointer to IFFHandle struct.
	localItem    -	pointer to LCI to install.
	contextNode  -	pointer to the context node in which the LCI will be stored.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	StoreLocalItem()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    struct LocalContextItem *node,
			    *nextnode;

    struct MinList *lcilist;

    (void) iff;

    DEBUG_STOREITEMINCONTEXT(dprintf("StoreItemInContext: iff 0x%lx item 0x%lx context 0x%lx\n",
			    iff, localItem, contextNode));

    lcilist = &( GetIntCN(contextNode)->cn_LCIList );

    /* Check if there are other similar LCIs stored */
    node = (struct LocalContextItem*)lcilist->mlh_Head;

    while ((nextnode = (struct LocalContextItem*)node->lci_Node.mln_Succ))
    {
	if
	(
	    ( node->lci_ID     == localItem->lci_ID    )
	&&
	    ( node->lci_Type   == localItem->lci_Type  )
	&&
	    ( node->lci_Ident  == localItem->lci_Ident  )
	)
	    PurgeLCI(node, IPB(IFFParseBase));

	node = nextnode;

    }

    /* Insert the LCI */
    AddHead
    (
	(struct List*)lcilist,
	(struct Node*)localItem
    );

    return;

    AROS_LIBFUNC_EXIT
} /* StoreItemInContext */
