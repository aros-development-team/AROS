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

	AROS_LH4(struct LocalContextItem *, FindLocalItem,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),
	AROS_LHA(LONG              , type, D0),
	AROS_LHA(LONG              , id, D1),
	AROS_LHA(LONG              , ident, D2),

/*  LOCATION */
	struct Library *, IFFParseBase, 35, IFFParse)

/*  FUNCTION
	Goes through the whole context node stack starting at the top
	searching the contecnodes for attached LocalContextItems with the
	specified type, id and ident codes.

    INPUTS
	iff   - pointer to an IFFHandle struct.
	type  - type code for item to find.
	id    -  identifier code for item to find.
	ident - ident code for the class of context item to find.

    RESULT
	lci   - pointer to a local context item if found, or NULL if
		none is found.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	StoreLocalItem()

    INTERNALS

    HISTORY
  27-11-96    digulla automatically created from
	  iffparse_lib.fd and clib/iffparse_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    struct IntContextNode   *cn_node,
			    *cn_nextnode;

    struct LocalContextItem *lci_node,
			    *lci_nextnode;
    /* Get contextnode at top */
    cn_node = (struct IntContextNode*)TopChunk(iff);

    while ((cn_nextnode = (struct IntContextNode*)cn_node->CN.cn_Node.mln_Succ))
    {
	/* Get LCI at top inside contextnode */
	lci_node = (struct LocalContextItem*)cn_node->cn_LCIList.mlh_Head;

	while ((lci_nextnode = (struct LocalContextItem*)lci_node->lci_Node.mln_Succ))
	{
	    /* Do we have a match ? */
	    if
	    (
		(lci_node->lci_Type   == type   )
	    &&
		(lci_node->lci_ID      == id    )
	    &&
		(lci_node->lci_Ident  == ident )
	    )
		return (lci_node);

	    lci_node = lci_nextnode;
	}

	cn_node = cn_nextnode;
    }

    return (NULL);

    AROS_LIBFUNC_EXIT
} /* FindLocalItem */
