/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "cgxvideo_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cgxvideo_protos.h>

	AROS_LH1(ULONG, UnlockVLayer,

/*  SYNOPSIS */
	AROS_LHA(struct VLayerHandle *, VLayerHandle, A0),

/*  LOCATION */
	struct Library *, CGXVideoBase, 11, Cgxvideo)

/*  FUNCTION
	Unlocks a previouly locked video layer

    INPUTS
	VLayerHandle - pointer to a previously created videolayer handle

    RESULT
	none

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	LockVLayer()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,CGXVideoBase)
    extern void aros_print_not_implemented (char *);

    aros_print_not_implemented ("UnlockVLayer");

    AROS_LIBFUNC_EXIT
} /* UnlockVLayer */
