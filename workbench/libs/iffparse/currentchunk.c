/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
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


    HISTORY
  27-11-96    digulla automatically created from
	  iffparse_lib.fd and clib/iffparse_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    struct ContextNode *cn;

    if (iff->iff_Depth)
	cn = TopChunk(iff);
    else
	/* If iffhandle is not opened yet, ther will be no contextnodes */
	cn = NULL;

    return (cn);


    AROS_LIBFUNC_EXIT
} /* CurrentChunk */
