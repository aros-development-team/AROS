/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH1(struct ContextNode *, CurrentChunk,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),

/*  LOCATION */
	struct Library *, IFFParseBase, 29, IFFParse)

/*  FUNCTION
	Returns the top context node for the give IFFHandle struct.
	The top contexte is the node most recently pushed onto the
	context stack

    INPUTS
	iff  - pointer to IFFHandle struct.

    RESULT
	top - Pointer to top contextnode or NULL if none.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	PushChunk(), PopChunk(), ParseIFF(), ParentChunk()

    INTERNALS
	" .. or NULL if none" (see RESULT) is truth with slight modifications,
	since the default context node exists as long as the iffhandle
	itself. However, the user is never given a pointer to this node.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    struct ContextNode *cn;

    (void) IFFParseBase;

    if (iff->iff_Depth)
	cn = TopChunk(iff);
    else
	/* If iffhandle is not opened yet, ther will be no contextnodes */
	cn = NULL;

    return (cn);


    AROS_LIBFUNC_EXIT
} /* CurrentChunk */
