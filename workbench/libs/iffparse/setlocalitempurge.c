/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH2(void, SetLocalItemPurge,

/*  SYNOPSIS */
	AROS_LHA(struct LocalContextItem *, localItem, A0),
	AROS_LHA(struct Hook             *, purgeHook, A1),

/*  LOCATION */
	struct Library *, IFFParseBase, 33, IFFParse)

/*  FUNCTION
	Inserts a custom purge hook for the given local context item.
	The purge hook will be freed when the system wants to delete a local
	context item.

    INPUTS
	localItem  -  pointer to a local context item.
	purgeHook  -  pointer to a hook sructure initialized with the purge function.

    RESULT

    NOTES
	The purgehook must call FreeLocalItem() on the local context item after
	doing its own resource freeing.


    EXAMPLE

    BUGS

    SEE ALSO
	AllocLocalItem(), FreeLocalItem()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    DEBUG_SETLOCALEITEMPURGE(dprintf("SetLocalItemPurge: localItem %p purgeHook %p\n", localItem, purgeHook));

    (void) IFFParseBase;

    /* Self - explaining */
    GetIntLCI(localItem)->lci_PurgeHook = purgeHook;

    DEBUG_SETLOCALEITEMPURGE(dprintf("SetLocalItemPurge: done\n"));

    AROS_LIBFUNC_EXIT
} /* SetLocalItemPurge */
