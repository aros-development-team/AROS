/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <dos/dosasl.h>
#include <proto/dos.h>

	AROS_LH1(void, MatchEnd,

/*  SYNOPSIS */
	AROS_LHA(struct AnchorPath *, anchor, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 139, Dos)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    struct AChain *curr = anchor->ap_Base;

    while (anchor->ap_Base != NULL)
    {
	curr = anchor->ap_Base;
	if (curr->an_Lock != NULL)
	    UnLock(curr->an_Lock);
	anchor->ap_Base = curr->an_Child;
	FreeVec(curr);
    }
    AROS_LIBFUNC_EXIT
} /* MatchEnd */
