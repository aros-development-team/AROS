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

	AROS_LH1(struct ContextNode *, FindPropContext,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),

/*  LOCATION */
	struct Library *, IFFParseBase, 28, IFFParse)

/*  FUNCTION
	Finds the proper context in which to store a property.
	If we have installed a property entry handler via PropChunk()
	and such a property chunk (for example id is "CMAP" and type is "ILBM"
	inside a form, then the storedproperty will be stored in the enclosing
	FORM chink. If the chunk was inside a PROP chunk inside a LIST, then
	the storedproperty would be installed in the LIST context.

    INPUTS
	iff - pointer to IFFHandle struct.

    RESULT
	cn  -  pointer to contextnode where the property might be installed, or
	      NULL if no such context exists.

    NOTES
	This function is most for internal use.

    EXAMPLE

    BUGS

    SEE ALSO
	ParentChunk(), CurrentChunk(), StoreItemInContext(), PropChunk()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    struct ContextNode	 *node;

    D(bug ("FindPropContext (iff=%p)\n", iff));

    node = TopChunk(iff);

    /* Start at the parent of the top node */
    do
    {
	DB2(bug("   node=%p (%c%c%c%c)\n",
	    node,
	    dmkid(node->cn_ID)
	));

	/* If this node is a FORM or a LIST, then we have acorrect property */
	if ( (node->cn_ID == ID_FORM) || (node->cn_ID == ID_LIST) )
	    ReturnPtr ("FindPropContext",struct ContextNode *,node);

    }
    while ((node = ParentChunk(node) ));

    /* No proper context found */
    ReturnPtr ("FindPropContext",struct ContextNode *,NULL);
    AROS_LIBFUNC_EXIT
} /* FindPropContext */
