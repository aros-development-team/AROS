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

	AROS_LH3(struct CollectionItem *, FindCollection,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),
	AROS_LHA(LONG              , type, D0),
	AROS_LHA(LONG              , id, D1),

/*  LOCATION */
	struct Library *, IFFParseBase, 27, IFFParse)

/*  FUNCTION
	Returns a pointer to a list of CollectionItems the installed
	CollectionChunk of that type and ID.

    INPUTS
	iff   - pointer to an IFFHandle struct.
	type  - type code to search for.
	id    -  id code to search for.


    RESULT
	ci - pointer to the last collection chunk encountered
	     with pointers to previous ones.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CollectionChunk(), CollectionChunks()

    INTERNALS

    HISTORY
  27-11-96    digulla automatically created from
	  iffparse_lib.fd and clib/iffparse_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    struct LocalContextItem *lci;

    if
    (
	!(lci = FindLocalItem
	    (
		iff,
		type,
		id,
		IFFLCI_COLLECTION
	    )
	)
    )
	return (NULL);

    /* The userdata of the found LCI contains a struct CIPtr,
    which is just a pointer to the first CollectionItem  */

   return
   (
       ((struct CIPtr*)LocalItemData(lci) )->FirstCI
   );


    AROS_LIBFUNC_EXIT
} /* FindCollection */
