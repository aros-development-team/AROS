/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "cgxvideo_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cgxvideo_protos.h>

	AROS_LH1(ULONG, LockVLayer,

/*  SYNOPSIS */
	AROS_LHA(struct VLayerHandle *, VLayerHandle, A0),

/*  LOCATION */
	struct Library *, CGXVideoBase, 10, Cgxvideo)

/*  FUNCTION
	Locks the specified video layer to allow access to source data. Make
	sure that you don't keep that lock for too long. It is only allowed
	to keep it for a short time.

    INPUTS
	VLayerHandle - pointer to a previously created videolayer handle

    RESULT
	result - TRUE if video layer could be locked, FALSE otherwise

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	UnlockVLayer()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,CGXVideoBase)
    extern void aros_print_not_implemented (char *);

    aros_print_not_implemented ("LockVLayer");

    AROS_LIBFUNC_EXIT
} /* LockVLayer */
