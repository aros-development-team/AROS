/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

    DEBUG_FINDLOCALITEM(dprintf("FindLocalItem: iff 0x%lx type 0x%08lx (%.4s) id 0x%08lx (%.4s) ident 0x%08lx (%.4s)\n",
			    iff, type, &type, id, &id, ident, &ident));

    D(bug("FindLocalItem (iff=%p, type=%c%c%c%c, id=%c%c%c%c, ident=%c%c%c%c)\n",
	iff,
	type>>24, type>>16, type>>8, type,
	id>>24, id>>16, id>>8, id,
	ident>>24,ident>>16,ident>>8,ident
    ));

    /* Get contextnode at top */
    cn_node = (struct IntContextNode*)TopChunk(iff);

    while ((cn_nextnode = (struct IntContextNode*)cn_node->CN.cn_Node.mln_Succ))
    {
#if DEBUG
	if (cn_node)
	{
	    DB2(bug("    node=%p (%c%c%c%c)\n",
		cn_node,
		cn_node->CN.cn_Type>>24,
		cn_node->CN.cn_Type>>16,
		cn_node->CN.cn_Type>>8,
		cn_node->CN.cn_Type
	    ));
	}
	else
	{
	    DB2(bug("    node=%p (----)\n", cn_node));
	}
#endif

	/* Get LCI at top inside contextnode */
	lci_node = (struct LocalContextItem*)cn_node->cn_LCIList.mlh_Head;

	while ((lci_nextnode = (struct LocalContextItem*)lci_node->lci_Node.mln_Succ))
	{
#if DEBUG
	    if (lci_node)
	    {
		DB2(bug("        lci_node=%p (%c%c%c%c, %c%c%c%c, %c%c%c%c)\n",
		    lci_node,
		    lci_node->lci_Type>>24,
		    lci_node->lci_Type>>16,
		    lci_node->lci_Type>>8,
		    lci_node->lci_Type,
		    lci_node->lci_ID>>24,
		    lci_node->lci_ID>>16,
		    lci_node->lci_ID>>8,
		    lci_node->lci_ID,
		    lci_node->lci_Ident>>24,
		    lci_node->lci_Ident>>16,
		    lci_node->lci_Ident>>8,
		    lci_node->lci_Ident
		));
	    }
	    else
	    {
		DB2(bug("        lci_node=%p (----, ----, ----)\n",
		    lci_node
		));
	    }
#endif

	    /* Do we have a match ? */
	    if
	    (
		(lci_node->lci_Type   == type )
	    &&
		(lci_node->lci_ID     == id   )
	    &&
		(lci_node->lci_Ident  == ident)
	    )
		ReturnPtr ("FindLocalItem",struct LocalContextItem *,lci_node);

	    lci_node = lci_nextnode;
	}

	cn_node = cn_nextnode;
    }

    ReturnPtr ("FindLocalItem",struct LocalContextItem *,NULL);
    AROS_LIBFUNC_EXIT
} /* FindLocalItem */
