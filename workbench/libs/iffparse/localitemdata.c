/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH1(APTR, LocalItemData,

/*  SYNOPSIS */
	AROS_LHA(struct LocalContextItem *, localItem, A0),

/*  LOCATION */
	struct Library *, IFFParseBase, 32, IFFParse)

/*  FUNCTION
	Returns a  pointer to the userdata allocated in AllocLocalItem.
	This function returns NULL for an input of NULL.

    INPUTS
	localItem - pointer to a LocalContextItem struct or NULL.

    RESULT
	data	  - pointer to user data or NULL.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AllocLocalItem(), FreeLocalItem()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    (void) IFFParseBase;

    return
    (
	localItem !=NULL ?
	GetIntLCI(localItem)->lci_UserData :
	NULL
    );

    AROS_LIBFUNC_EXIT
} /* LocalItemData */
