/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "intuition_intern.h"
#include <exec/memory.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

	AROS_LH3(APTR, AllocRemember,

/*  SYNOPSIS */
	AROS_LHA(struct Remember **, rememberKey, A0),
	AROS_LHA(ULONG             , size, D0),
	AROS_LHA(ULONG             , flags, D1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 66, Intuition)

/*  FUNCTION
	Allocate some memory and remeber it in the Remember-List.

    INPUTS
	rememberKey - Store information in this list
	size - How many bytes to allocate
	flags - Attributes (see AllocMem())

    RESULT
	Pointer to the allocated memory or NULL.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    struct Remember * newKey;

    for (newKey=*rememberKey
	; newKey && newKey->RememberSize!=0
	; newKey=newKey->NextRemember
    )
	/* nop */;

    if (!newKey)
    {
	if (!(newKey = AllocMem (sizeof (struct Remember), MEMF_ANY)) )
	    return NULL;

	newKey->NextRemember = *rememberKey;
	newKey->RememberSize = 0L;
    }

    *rememberKey = newKey;

    newKey->Memory = AllocMem (size, flags);

    if (!newKey->Memory)
	return NULL;

    newKey->RememberSize = size;

    return newKey->Memory;
    AROS_LIBFUNC_EXIT
} /* AllocRemember */
